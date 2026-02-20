/**
 * @file ResonanceSearchService.cpp
 * @brief 同频共鸣搜索服务实现 - 使用Milvus向量数据库
 */

#include "infrastructure/services/ResonanceSearchService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/vector/MilvusClient.h"
#include <drogon/drogon.h>
#include <algorithm>

namespace heartlake::infrastructure {

ResonanceSearchService& ResonanceSearchService::getInstance() {
    static ResonanceSearchService instance;
    return instance;
}

void ResonanceSearchService::initialize(size_t embeddingDim) {
    embeddingDim_ = embeddingDim;
    ai::AdvancedEmbeddingEngine::getInstance().initialize(embeddingDim);

    auto& milvus = MilvusClient::getInstance();
    if (milvus.isConnected()) {
        if (!milvus.hasCollection(COLLECTION_NAME)) {
            milvus.createCollection(COLLECTION_NAME, embeddingDim);
            milvus.createIndex(COLLECTION_NAME);
        }
        useMilvus_ = true;
    }

    initialized_ = true;
    LOG_INFO << "ResonanceSearchService initialized, useMilvus=" << useMilvus_;
}

void ResonanceSearchService::indexStone(const std::string& stoneId, const std::string& content) {
    auto embedding = ai::AdvancedEmbeddingEngine::getInstance().generateEmbedding(content);
    if (embedding.empty()) return;

    if (useMilvus_) {
        Json::Value meta;
        meta["content"] = content;
        MilvusClient::getInstance().insert(COLLECTION_NAME, stoneId, embedding, meta);
    } else {
        // Fallback to DB
        std::string vecStr;
        for (size_t i = 0; i < embedding.size(); ++i) {
            if (i > 0) vecStr += ",";
            vecStr += std::to_string(embedding[i]);
        }
        auto db = drogon::app().getDbClient("default");
        try {
            db->execSqlSync(
                "INSERT INTO stone_embeddings (stone_id, embedding, created_at) "
                "VALUES ($1, $2, NOW()) ON CONFLICT (stone_id) DO UPDATE SET embedding = $2",
                stoneId, vecStr);
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to index stone: " << e.base().what();
        }
    }
}

std::vector<ResonanceMatch> ResonanceSearchService::searchResonance(
    const std::string& stoneId, const std::string& content, float threshold, int limit) {

    auto queryVec = ai::AdvancedEmbeddingEngine::getInstance().generateEmbedding(content);
    if (queryVec.empty()) return {};

    std::vector<ResonanceMatch> matches;

    if (useMilvus_) {
        auto results = MilvusClient::getInstance().search(
            COLLECTION_NAME, queryVec, limit * 2, "id != \"" + stoneId + "\"");

        auto db = drogon::app().getDbClient("default");
        for (const auto& r : results) {
            float similarity = 1.0f - r.score; // COSINE distance to similarity
            if (similarity < threshold) continue;

            try {
                auto row = db->execSqlSync(
                    "SELECT user_id, content FROM stones WHERE stone_id = $1 AND status = 'published'",
                    r.id);
                if (row.empty()) continue;

                ResonanceMatch m;
                m.stoneId = r.id;
                m.userId = row[0]["user_id"].as<std::string>();
                m.content = row[0]["content"].as<std::string>();
                m.similarity = similarity;
                m.deliveryDelaySeconds = calculateDeliveryDelay(similarity);
                matches.push_back(m);

                if (static_cast<int>(matches.size()) >= limit) break;
            } catch (...) {}
        }
    } else {
        // Fallback: DB-based search
        auto db = drogon::app().getDbClient("default");
        try {
            auto result = db->execSqlSync(
                "SELECT se.stone_id, se.embedding, s.user_id, s.content "
                "FROM stone_embeddings se JOIN stones s ON se.stone_id = s.stone_id "
                "WHERE se.stone_id != $1 AND s.status = 'published'", stoneId);

            for (const auto& row : result) {
                std::vector<float> targetVec;
                std::stringstream ss(row["embedding"].as<std::string>());
                std::string token;
                while (std::getline(ss, token, ',')) targetVec.push_back(std::stof(token));

                if (targetVec.size() != queryVec.size()) continue;

                float similarity = ai::AdvancedEmbeddingEngine::cosineSimilarity(queryVec, targetVec);
                if (similarity >= threshold) {
                    ResonanceMatch m;
                    m.stoneId = row["stone_id"].as<std::string>();
                    m.userId = row["user_id"].as<std::string>();
                    m.content = row["content"].as<std::string>();
                    m.similarity = similarity;
                    m.deliveryDelaySeconds = calculateDeliveryDelay(similarity);
                    matches.push_back(m);
                }
            }

            std::sort(matches.begin(), matches.end(),
                [](const ResonanceMatch& a, const ResonanceMatch& b) { return a.similarity > b.similarity; });
            if (static_cast<int>(matches.size()) > limit) matches.resize(limit);
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Resonance search failed: " << e.base().what();
        }
    }

    return matches;
}

int ResonanceSearchService::calculateDeliveryDelay(float similarity) {
    // 相似度越高，延迟越短：[0.85, 1.0] -> [600, 60]秒
    float normalized = std::max(0.0f, std::min(1.0f, (similarity - 0.85f) / 0.15f));
    return static_cast<int>(600 - normalized * 540);
}

void ResonanceSearchService::removeStone(const std::string& stoneId) {
    if (useMilvus_) {
        MilvusClient::getInstance().remove(COLLECTION_NAME, stoneId);
    }
    auto db = drogon::app().getDbClient("default");
    try {
        db->execSqlSync("DELETE FROM stone_embeddings WHERE stone_id = $1", stoneId);
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to remove stone: " << e.base().what();
    }
}

} // namespace heartlake::infrastructure
