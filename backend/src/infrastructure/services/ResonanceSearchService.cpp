/**
 * 同频共鸣搜索服务实现
 *
 * 三阶段检索管线，为每颗石头找到情感共鸣的伙伴：
 *
 * Phase 1 - 候选召回：
 *   优先走 Milvus 向量数据库做 ANN 检索（COSINE 距离），
 *   Milvus 不可用时降级到 DB 全量 cosine similarity 扫描。
 *   使用宽松阈值（threshold - 0.2）多取 5 倍候选。
 *
 * Phase 2 - 四维重排序（EmotionResonanceEngine）：
 *   α=0.30 语义相似度 + β=0.35 DTW 情绪轨迹相似度
 *   + γ=0.20 时间衰减 + δ=0.15 多样性奖励
 *
 * Phase 3 - Top-K 截取 + 投递延迟计算：
 *   共鸣度越高延迟越短（60s ~ 600s），模拟"缘分到来"的体验。
 */

#include "infrastructure/services/ResonanceSearchService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/ai/EmotionResonanceEngine.h"
#include "infrastructure/vector/MilvusClient.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <cmath>

namespace heartlake::infrastructure {

ResonanceSearchService& ResonanceSearchService::getInstance() {
    static ResonanceSearchService instance;
    return instance;
}

void ResonanceSearchService::initialize(size_t embeddingDim) {
    embeddingDim_ = embeddingDim;
    auto& embeddingEngine = ai::AdvancedEmbeddingEngine::getInstance();
    if (!embeddingEngine.isInitialized()) {
        embeddingEngine.initialize(embeddingDim);
    } else if (embeddingEngine.getEmbeddingDimension() != embeddingDim) {
        LOG_WARN << "ResonanceSearchService embedding dim mismatch, requested="
                 << embeddingDim << ", active="
                 << embeddingEngine.getEmbeddingDimension()
                 << ". Using active dimension.";
        embeddingDim_ = embeddingEngine.getEmbeddingDimension();
    }

    auto& milvus = MilvusClient::getInstance();
    if (milvus.ping()) {
        bool collectionReady = milvus.hasCollection(COLLECTION_NAME);
        if (!collectionReady) {
            const bool created = milvus.createCollection(COLLECTION_NAME, static_cast<int>(embeddingDim_));
            const bool indexed = created && milvus.createIndex(COLLECTION_NAME);
            collectionReady = created && indexed;
            if (!collectionReady) {
                LOG_WARN << "Milvus collection bootstrap failed, fallback to DB embeddings";
            }
        }
        useMilvus_ = collectionReady;
    } else {
        LOG_WARN << "Milvus ping failed, fallback to DB embeddings";
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

    // Phase 1: 收集 cosine similarity 候选（宽松阈值，多取候选供重排序）
    const float candidateThreshold = std::max(0.5f, threshold - 0.2f);
    const int candidateLimit = limit * 5;
    std::vector<ResonanceMatch> candidates;

    if (useMilvus_) {
        auto results = MilvusClient::getInstance().search(
            COLLECTION_NAME, queryVec, candidateLimit, "id != \"" + stoneId + "\"");

        auto db = drogon::app().getDbClient("default");
        for (const auto& r : results) {
            float similarity = 1.0f - r.score; // COSINE distance to similarity
            if (similarity < candidateThreshold) continue;

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
                m.semanticScore = similarity;
                candidates.push_back(m);
            } catch (const std::exception& e) {
                LOG_WARN << "HNSW candidate lookup failed: " << e.what();
            }
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
                if (similarity >= candidateThreshold) {
                    ResonanceMatch m;
                    m.stoneId = row["stone_id"].as<std::string>();
                    m.userId = row["user_id"].as<std::string>();
                    m.content = row["content"].as<std::string>();
                    m.similarity = similarity;
                    m.semanticScore = similarity;
                    candidates.push_back(m);
                }
            }
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Resonance search failed: " << e.base().what();
        }
    }

    if (candidates.empty()) return {};

    // Phase 2: 使用 EmotionResonanceEngine 四维重排序
    // ResonanceScore = α·Semantic + β·DTW + γ·Decay + δ·Diversity
    auto& resonanceEngine = ai::EmotionResonanceEngine::getInstance();
    auto db = drogon::app().getDbClient("default");

    // 获取源石头的 mood 和当前用户ID
    std::string sourceMood = "neutral";
    std::string sourceUserId;
    try {
        auto stoneRow = db->execSqlSync(
            "SELECT user_id, mood_type FROM stones WHERE stone_id = $1",
            stoneId);
        if (!stoneRow.empty()) {
            sourceUserId = stoneRow[0]["user_id"].as<std::string>();
            if (!stoneRow[0]["mood_type"].isNull())
                sourceMood = stoneRow[0]["mood_type"].as<std::string>();
        }
    } catch (const std::exception& e) {
        LOG_WARN << "Source stone lookup failed: " << e.what();
    }

    // 加载源用户的情绪轨迹（用于DTW计算）
    ai::EmotionTrajectory userTraj;
    if (!sourceUserId.empty()) {
        try {
            auto trajRows = db->execSqlSync(
                "SELECT score FROM emotion_tracking "
                "WHERE user_id = $1 AND created_at > NOW() - INTERVAL '7 days' "
                "ORDER BY created_at ASC", sourceUserId);
            userTraj.userId = sourceUserId;
            for (const auto& r : trajRows) {
                userTraj.scores.push_back(r["score"].as<float>());
            }
            if (!userTraj.scores.empty())
                userTraj.currentScore = userTraj.scores.back();
        } catch (const std::exception& e) {
            LOG_WARN << "User trajectory load failed: " << e.what();
        }
    }

    std::vector<std::string> recommendedMoods;

    for (auto& m : candidates) {
        // 获取候选石头的 mood 和 timestamp
        std::string candMood = "neutral";
        std::string candTimestamp;
        try {
            auto candRow = db->execSqlSync(
                "SELECT mood_type, created_at FROM stones WHERE stone_id = $1",
                m.stoneId);
            if (!candRow.empty()) {
                if (!candRow[0]["mood_type"].isNull())
                    candMood = candRow[0]["mood_type"].as<std::string>();
                candTimestamp = candRow[0]["created_at"].as<std::string>();
            }
        } catch (const std::exception& e) {
            LOG_WARN << "Candidate stone metadata lookup failed: " << e.what();
        }

        // 维度1: 语义相似度（已有）
        // m.semanticScore 已在 Phase 1 设置

        // 维度2: DTW 情绪轨迹相似度
        ai::EmotionTrajectory candTraj;
        try {
            auto trajRows = db->execSqlSync(
                "SELECT score FROM emotion_tracking "
                "WHERE user_id = $1 AND created_at > NOW() - INTERVAL '7 days' "
                "ORDER BY created_at ASC", m.userId);
            for (const auto& r : trajRows) {
                candTraj.scores.push_back(r["score"].as<float>());
            }
            if (!candTraj.scores.empty())
                candTraj.currentScore = candTraj.scores.back();
        } catch (const std::exception& e) {
            LOG_WARN << "Candidate trajectory load failed: " << e.what();
        }

        if (!userTraj.scores.empty() && !candTraj.scores.empty()) {
            m.trajectoryScore = resonanceEngine.trajectorySimDTW(userTraj.scores, candTraj.scores);
        } else {
            float scoreDiff = std::abs(userTraj.currentScore - candTraj.currentScore);
            m.trajectoryScore = std::exp(-scoreDiff);
        }

        // 维度3: 时间衰减
        m.temporalScore = candTimestamp.empty()
            ? 0.5f
            : resonanceEngine.temporalDecay(candTimestamp);

        // 维度4: 多样性奖励
        m.diversityScore = resonanceEngine.diversityBonus(sourceMood, candMood, recommendedMoods);

        // 加权总分: α=0.30, β=0.35, γ=0.20, δ=0.15
        m.resonanceTotal = resonanceEngine.getAlpha() * m.semanticScore
                         + resonanceEngine.getBeta()  * m.trajectoryScore
                         + resonanceEngine.getGamma() * m.temporalScore
                         + resonanceEngine.getDelta() * m.diversityScore;

        // 生成共鸣原因
        ai::ResonanceResult rr;
        rr.stoneId = m.stoneId;
        rr.totalScore = m.resonanceTotal;
        rr.semanticScore = m.semanticScore;
        rr.trajectoryScore = m.trajectoryScore;
        rr.temporalScore = m.temporalScore;
        rr.diversityScore = m.diversityScore;
        m.resonanceReason = resonanceEngine.generateResonanceReason(rr, sourceMood, candMood);

        recommendedMoods.push_back(candMood);
    }

    // Phase 3: 按四维共鸣总分降序排序
    std::sort(candidates.begin(), candidates.end(),
        [](const ResonanceMatch& a, const ResonanceMatch& b) {
            return a.resonanceTotal > b.resonanceTotal;
        });

    // 截取 top-K 并计算投递延迟
    std::vector<ResonanceMatch> matches;
    for (auto& m : candidates) {
        if (static_cast<int>(matches.size()) >= limit) break;
        // 使用四维总分作为最终 similarity 供外部使用
        m.similarity = m.resonanceTotal;
        m.deliveryDelaySeconds = calculateDeliveryDelay(m.resonanceTotal);
        matches.push_back(std::move(m));
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
