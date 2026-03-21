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
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace heartlake::infrastructure {

namespace {

struct CandidateContext {
    ResonanceMatch match;
    std::string mood = "neutral";
    std::string createdAt;
};

std::vector<float> parseEmbeddingCsv(const std::string& raw) {
    std::vector<float> values;
    std::stringstream ss(raw);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            values.push_back(std::stof(token));
        }
    }
    return values;
}

std::vector<float> parsePgFloatArray(const std::string& raw) {
    std::vector<float> values;
    if (raw.size() < 2) {
        return values;
    }

    const std::string body = raw.substr(1, raw.size() - 2);
    std::stringstream ss(body);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty() && token != "NULL") {
            values.push_back(std::stof(token));
        }
    }
    return values;
}

std::string joinDelimitedIds(const std::vector<std::string>& values) {
    std::string joined;
    for (const auto& value : values) {
        if (value.empty()) {
            continue;
        }
        if (!joined.empty()) {
            joined += ',';
        }
        joined += value;
    }
    return joined;
}

std::unordered_map<std::string, CandidateContext> loadCandidateContexts(
    const drogon::orm::DbClientPtr& db,
    const std::vector<std::string>& stoneIds
) {
    std::unordered_map<std::string, CandidateContext> contexts;
    if (stoneIds.empty()) {
        return contexts;
    }

    const std::string joinedStoneIds = joinDelimitedIds(stoneIds);
    if (joinedStoneIds.empty()) {
        return contexts;
    }

    auto rows = db->execSqlSync(
        "SELECT stone_id, user_id, content, "
        "       COALESCE(mood_type, 'neutral') AS mood_type, "
        "       created_at::text AS created_at "
        "FROM stones "
        "WHERE stone_id = ANY(string_to_array($1, ',')::text[]) "
        "  AND status = 'published' "
        "  AND deleted_at IS NULL",
        joinedStoneIds);

    contexts.reserve(rows.size());
    for (const auto& row : rows) {
        CandidateContext context;
        context.match.stoneId = row["stone_id"].as<std::string>();
        context.match.userId = row["user_id"].as<std::string>();
        context.match.content = row["content"].as<std::string>();
        context.mood = row["mood_type"].isNull()
            ? "neutral"
            : row["mood_type"].as<std::string>();
        context.createdAt = row["created_at"].isNull()
            ? ""
            : row["created_at"].as<std::string>();
        contexts.emplace(context.match.stoneId, std::move(context));
    }
    return contexts;
}

std::unordered_map<std::string, std::vector<float>> loadTrajectoryScores(
    const drogon::orm::DbClientPtr& db,
    const std::vector<std::string>& userIds
) {
    std::unordered_map<std::string, std::vector<float>> trajectories;
    if (userIds.empty()) {
        return trajectories;
    }

    const std::string joinedUserIds = joinDelimitedIds(userIds);
    if (joinedUserIds.empty()) {
        return trajectories;
    }

    auto rows = db->execSqlSync(
        "SELECT et.user_id, ARRAY_AGG(et.score ORDER BY et.created_at ASC)::text AS scores "
        "FROM emotion_tracking et "
        "WHERE et.user_id = ANY(string_to_array($1, ',')::text[]) "
        "  AND et.created_at > NOW() - INTERVAL '7 days' "
        "GROUP BY et.user_id",
        joinedUserIds);

    trajectories.reserve(rows.size());
    for (const auto& row : rows) {
        const auto userId = row["user_id"].as<std::string>();
        const auto rawScores = row["scores"].isNull()
            ? std::string("{}")
            : row["scores"].as<std::string>();
        trajectories.emplace(userId, parsePgFloatArray(rawScores));
    }
    return trajectories;
}

}  // namespace

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
    std::vector<CandidateContext> candidates;
    candidates.reserve(static_cast<size_t>(std::max(candidateLimit, limit)));

    if (useMilvus_) {
        auto results = MilvusClient::getInstance().search(
            COLLECTION_NAME, queryVec, candidateLimit, "id != \"" + stoneId + "\"");

        auto db = drogon::app().getDbClient("default");
        std::vector<std::pair<std::string, float>> acceptedStoneIds;
        acceptedStoneIds.reserve(results.size());
        std::vector<std::string> stoneIds;
        stoneIds.reserve(results.size());
        for (const auto& r : results) {
            float similarity = 1.0f - r.score; // COSINE distance to similarity
            if (similarity < candidateThreshold) continue;
            acceptedStoneIds.emplace_back(r.id, similarity);
            stoneIds.push_back(r.id);
        }

        try {
            auto contexts = loadCandidateContexts(db, stoneIds);
            for (const auto& [candidateStoneId, similarity] : acceptedStoneIds) {
                auto it = contexts.find(candidateStoneId);
                if (it == contexts.end()) {
                    continue;
                }
                auto context = std::move(it->second);
                context.match.similarity = similarity;
                context.match.semanticScore = similarity;
                candidates.push_back(std::move(context));
            }
        } catch (const std::exception& e) {
            LOG_WARN << "HNSW candidate batch lookup failed: " << e.what();
        }
    } else {
        // Fallback: DB-based search
        auto db = drogon::app().getDbClient("default");
        try {
            auto result = db->execSqlSync(
                "SELECT se.stone_id, se.embedding, s.user_id, s.content, "
                "       COALESCE(s.mood_type, 'neutral') AS mood_type, "
                "       s.created_at::text AS created_at "
                "FROM stone_embeddings se JOIN stones s ON se.stone_id = s.stone_id "
                "WHERE se.stone_id != $1 "
                "  AND s.status = 'published' "
                "  AND s.deleted_at IS NULL",
                stoneId);

            for (const auto& row : result) {
                auto targetVec = parseEmbeddingCsv(row["embedding"].as<std::string>());

                if (targetVec.size() != queryVec.size()) continue;

                float similarity = ai::AdvancedEmbeddingEngine::cosineSimilarity(queryVec, targetVec);
                if (similarity >= candidateThreshold) {
                    CandidateContext context;
                    context.match.stoneId = row["stone_id"].as<std::string>();
                    context.match.userId = row["user_id"].as<std::string>();
                    context.match.content = row["content"].as<std::string>();
                    context.match.similarity = similarity;
                    context.match.semanticScore = similarity;
                    context.mood = row["mood_type"].isNull()
                        ? "neutral"
                        : row["mood_type"].as<std::string>();
                    context.createdAt = row["created_at"].isNull()
                        ? ""
                        : row["created_at"].as<std::string>();
                    candidates.push_back(std::move(context));
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
    recommendedMoods.reserve(candidates.size());
    std::unordered_set<std::string> seenUserIds;
    seenUserIds.reserve(candidates.size());
    std::vector<std::string> candidateUserIds;
    candidateUserIds.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        if (seenUserIds.insert(candidate.match.userId).second) {
            candidateUserIds.push_back(candidate.match.userId);
        }
    }

    const auto trajectoryScoresByUser = loadTrajectoryScores(db, candidateUserIds);

    for (auto& candidate : candidates) {
        auto& m = candidate.match;

        // 维度1: 语义相似度（已有）
        // m.semanticScore 已在 Phase 1 设置

        // 维度2: DTW 情绪轨迹相似度
        ai::EmotionTrajectory candTraj;
        auto candidateTrajectoryIt = trajectoryScoresByUser.find(m.userId);
        if (candidateTrajectoryIt != trajectoryScoresByUser.end()) {
            candTraj.scores = candidateTrajectoryIt->second;
            if (!candTraj.scores.empty()) {
                candTraj.currentScore = candTraj.scores.back();
            }
        }

        if (!userTraj.scores.empty() && !candTraj.scores.empty()) {
            m.trajectoryScore = resonanceEngine.trajectorySimDTW(userTraj.scores, candTraj.scores);
        } else {
            float scoreDiff = std::abs(userTraj.currentScore - candTraj.currentScore);
            m.trajectoryScore = std::exp(-scoreDiff);
        }

        // 维度3: 时间衰减
        m.temporalScore = candidate.createdAt.empty()
            ? 0.5f
            : resonanceEngine.temporalDecay(candidate.createdAt);

        // 维度4: 多样性奖励
        m.diversityScore = resonanceEngine.diversityBonus(sourceMood, candidate.mood, recommendedMoods);

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
        m.resonanceReason = resonanceEngine.generateResonanceReason(rr, sourceMood, candidate.mood);

        recommendedMoods.push_back(candidate.mood);
    }

    // Phase 3: 按四维共鸣总分降序排序
    std::sort(candidates.begin(), candidates.end(),
        [](const CandidateContext& a, const CandidateContext& b) {
            return a.match.resonanceTotal > b.match.resonanceTotal;
        });

    // 截取 top-K 并计算投递延迟
    std::vector<ResonanceMatch> matches;
    matches.reserve(std::min(candidates.size(), static_cast<size_t>(limit)));
    for (auto& candidate : candidates) {
        if (static_cast<int>(matches.size()) >= limit) break;
        auto& m = candidate.match;
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
