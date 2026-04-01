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
#include "utils/MoodUtils.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace heartlake::infrastructure {

namespace {

std::string trimAscii(std::string value) {
    const auto begin = value.find_first_not_of(" \t\n\r");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\n\r");
    return value.substr(begin, end - begin + 1);
}

std::vector<float> parsePgFloatArray(const std::string& raw);

struct CandidateContext {
    ResonanceMatch match;
    std::string mood = "neutral";
    std::string createdAt;
};

std::vector<float> parseEmbeddingCsv(const std::string& raw) {
    const auto normalized = trimAscii(raw);
    if (!normalized.empty() &&
        ((normalized.front() == '{' && normalized.back() == '}') ||
         (normalized.front() == '[' && normalized.back() == ']'))) {
        return parsePgFloatArray(normalized.front() == '['
            ? "{" + normalized.substr(1, normalized.size() - 2) + "}"
            : normalized);
    }

    std::vector<float> values;
    std::stringstream ss(normalized);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trimAscii(token);
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
        token = trimAscii(token);
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
        "SELECT s.stone_id, s.user_id, s.content, "
        "       COALESCE(s.stone_type, 'medium') AS stone_type, "
        "       COALESCE(s.stone_color, '#7A92A3') AS stone_color, "
        "       " + heartlake::utils::sqlCanonicalMoodExpr("s.mood_type") + " AS mood_type, "
        "       " + heartlake::utils::sqlMoodScoreExpr("s.mood_type", "s.emotion_score") + " AS emotion_score, "
        "       COALESCE(s.boat_count, 0) AS boat_count, "
        "       COALESCE(u.nickname, '') AS author_name, "
        "       created_at::text AS created_at "
        "FROM stones s "
        "LEFT JOIN users u ON s.user_id = u.user_id "
        "WHERE s.stone_id = ANY(string_to_array($1, ',')::text[]) "
        "  AND s.status = 'published' "
        "  AND s.deleted_at IS NULL",
        joinedStoneIds);

    contexts.reserve(rows.size());
    for (const auto& row : rows) {
        CandidateContext context;
        context.match.stoneId = row["stone_id"].as<std::string>();
        context.match.userId = row["user_id"].as<std::string>();
        context.match.content = row["content"].as<std::string>();
        context.match.stoneType = row["stone_type"].isNull()
            ? "medium"
            : row["stone_type"].as<std::string>();
        context.match.stoneColor = row["stone_color"].isNull()
            ? "#7A92A3"
            : row["stone_color"].as<std::string>();
        context.match.moodType = row["mood_type"].isNull()
            ? "neutral"
            : heartlake::utils::normalizeMood(row["mood_type"].as<std::string>());
        context.match.emotionScore = row["emotion_score"].isNull()
            ? static_cast<float>(heartlake::utils::scoreForMood(context.match.moodType))
            : row["emotion_score"].as<float>();
        context.match.boatCount = row["boat_count"].isNull()
            ? 0
            : row["boat_count"].as<int>();
        context.match.authorName = row["author_name"].isNull()
            ? ""
            : row["author_name"].as<std::string>();
        context.match.createdAt = row["created_at"].isNull()
            ? ""
            : row["created_at"].as<std::string>();
        context.mood = row["mood_type"].isNull()
            ? "neutral"
            : heartlake::utils::normalizeMood(row["mood_type"].as<std::string>());
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
        "WITH requested_users AS ("
        "  SELECT DISTINCT unnest(string_to_array($1, ',')) AS user_id"
        "), tracking_scores AS ("
        "  SELECT et.user_id, ARRAY_AGG(et.score ORDER BY et.created_at ASC)::text AS scores "
        "  FROM emotion_tracking et "
        "  JOIN requested_users ru ON ru.user_id = et.user_id "
        "  WHERE et.created_at > NOW() - INTERVAL '7 days' "
        "  GROUP BY et.user_id"
        "), profile_scores AS ("
        "  SELECT uep.user_id, ARRAY_AGG(COALESCE(uep.avg_emotion_score, 0.0) ORDER BY uep.date ASC)::text AS scores "
        "  FROM user_emotion_profile uep "
        "  JOIN requested_users ru ON ru.user_id = uep.user_id "
        "  WHERE uep.date >= CURRENT_DATE - INTERVAL '7 days' "
        "  GROUP BY uep.user_id"
        "), stone_scores AS ("
        "  SELECT s.user_id, ARRAY_AGG("
        + heartlake::utils::sqlMoodScoreExpr("s.mood_type", "s.emotion_score")
        + " ORDER BY s.created_at ASC)::text AS scores "
        "  FROM stones s "
        "  JOIN requested_users ru ON ru.user_id = s.user_id "
        "  WHERE s.status = 'published' "
        "    AND s.deleted_at IS NULL "
        "    AND s.created_at > NOW() - INTERVAL '7 days' "
        "  GROUP BY s.user_id"
        ") "
        "SELECT ru.user_id, "
        "       COALESCE(ts.scores, ps.scores, ss.scores, '{}'::text) AS scores "
        "FROM requested_users ru "
        "LEFT JOIN tracking_scores ts ON ts.user_id = ru.user_id "
        "LEFT JOIN profile_scores ps ON ps.user_id = ru.user_id "
        "LEFT JOIN stone_scores ss ON ss.user_id = ru.user_id",
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

std::vector<float> loadRequesterTrajectoryScores(
    const drogon::orm::DbClientPtr& db,
    const std::string& requesterUserId,
    const std::string& stoneId
) {
    std::vector<float> scores;

    auto trackingRows = db->execSqlSync(
        "SELECT score "
        "FROM emotion_tracking "
        "WHERE user_id = $1 "
        "  AND created_at > NOW() - INTERVAL '7 days' "
        "ORDER BY created_at ASC",
        requesterUserId);
    for (const auto& row : trackingRows) {
        scores.push_back(row["score"].as<float>());
    }
    if (!scores.empty()) {
        return scores;
    }

    auto profileRows = db->execSqlSync(
        "SELECT avg_emotion_score "
        "FROM user_emotion_profile "
        "WHERE user_id = $1 "
        "  AND date >= CURRENT_DATE - INTERVAL '7 days' "
        "ORDER BY date ASC",
        requesterUserId);
    for (const auto& row : profileRows) {
        if (!row["avg_emotion_score"].isNull()) {
            scores.push_back(row["avg_emotion_score"].as<float>());
        }
    }
    if (!scores.empty()) {
        return scores;
    }

    auto authoredStoneRows = db->execSqlSync(
        "SELECT " + heartlake::utils::sqlMoodScoreExpr("mood_type", "emotion_score") + " AS score "
        "FROM stones "
        "WHERE user_id = $1 "
        "  AND status = 'published' "
        "  AND deleted_at IS NULL "
        "  AND created_at > NOW() - INTERVAL '7 days' "
        "ORDER BY created_at ASC",
        requesterUserId);
    for (const auto& row : authoredStoneRows) {
        scores.push_back(row["score"].as<float>());
    }
    if (!scores.empty()) {
        return scores;
    }

    auto interactedStoneRows = db->execSqlSync(
        "SELECT " + heartlake::utils::sqlMoodScoreExpr("s.mood_type", "s.emotion_score") + " AS score "
        "FROM user_interaction_history h "
        "JOIN stones s ON s.stone_id = h.stone_id "
        "WHERE h.user_id = $1 "
        "  AND s.status = 'published' "
        "  AND s.deleted_at IS NULL "
        "  AND h.created_at > NOW() - INTERVAL '7 days' "
        "ORDER BY h.created_at ASC",
        requesterUserId);
    for (const auto& row : interactedStoneRows) {
        scores.push_back(row["score"].as<float>());
    }
    if (!scores.empty()) {
        return scores;
    }

    auto referenceStoneRows = db->execSqlSync(
        "SELECT " + heartlake::utils::sqlMoodScoreExpr("mood_type", "emotion_score") + " AS score, "
        "       " + heartlake::utils::sqlCanonicalMoodExpr("mood_type") + " AS mood_type "
        "FROM stones "
        "WHERE stone_id = $1 "
        "  AND status = 'published' "
        "  AND deleted_at IS NULL "
        "LIMIT 1",
        stoneId);
    if (!referenceStoneRows.empty()) {
        const auto& row = referenceStoneRows[0];
        if (!row["score"].isNull()) {
            scores.push_back(row["score"].as<float>());
        } else if (!row["mood_type"].isNull()) {
            scores.push_back(static_cast<float>(
                heartlake::utils::scoreForMood(row["mood_type"].as<std::string>())));
        }
    }

    return scores;
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
            throw std::runtime_error(std::string("Failed to index stone embedding: ") + e.base().what());
        }
    }
}

std::vector<ResonanceMatch> ResonanceSearchService::searchResonance(
    const std::string& requesterUserId,
    const std::string& stoneId,
    const std::string& content,
    float threshold,
    int limit) {

    if (requesterUserId.empty()) {
        throw std::invalid_argument("searchResonance requires requester user id");
    }
    if (content.empty()) {
        throw std::invalid_argument("searchResonance requires non-empty content");
    }
    auto queryVec = ai::AdvancedEmbeddingEngine::getInstance().generateEmbedding(content);
    if (queryVec.empty()) {
        throw std::runtime_error("Failed to generate query embedding for resonance search");
    }

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
            throw std::runtime_error(std::string("Resonance candidate lookup failed: ") + e.what());
        }
    } else {
        // Fallback: DB-based search
        auto db = drogon::app().getDbClient("default");
        try {
            auto result = db->execSqlSync(
                "SELECT se.stone_id, se.embedding, s.user_id, s.content, "
                "       COALESCE(s.stone_type, 'medium') AS stone_type, "
                "       COALESCE(s.stone_color, '#7A92A3') AS stone_color, "
                "       " + heartlake::utils::sqlCanonicalMoodExpr("s.mood_type") + " AS mood_type, "
                "       " + heartlake::utils::sqlMoodScoreExpr("s.mood_type", "s.emotion_score") + " AS emotion_score, "
                "       COALESCE(s.boat_count, 0) AS boat_count, "
                "       COALESCE(u.nickname, '') AS author_name, "
                "       s.created_at::text AS created_at "
                "FROM stone_embeddings se "
                "JOIN stones s ON se.stone_id = s.stone_id "
                "LEFT JOIN users u ON s.user_id = u.user_id "
                "WHERE se.stone_id != $1 "
                "  AND s.user_id != $2 "
                "  AND s.status = 'published' "
                "  AND s.deleted_at IS NULL "
                "  AND NOT EXISTS ("
                "    SELECT 1 FROM user_interaction_history h "
                "    WHERE h.user_id = $2 AND h.stone_id = s.stone_id"
                "  )",
                stoneId, requesterUserId);

            for (const auto& row : result) {
                auto targetVec = parseEmbeddingCsv(row["embedding"].as<std::string>());

                if (targetVec.size() != queryVec.size()) continue;

                float similarity = ai::AdvancedEmbeddingEngine::cosineSimilarity(queryVec, targetVec);
                if (similarity >= candidateThreshold) {
                    CandidateContext context;
                    context.match.stoneId = row["stone_id"].as<std::string>();
                    context.match.userId = row["user_id"].as<std::string>();
                    context.match.content = row["content"].as<std::string>();
                    context.match.stoneType = row["stone_type"].isNull()
                        ? "medium"
                        : row["stone_type"].as<std::string>();
                    context.match.stoneColor = row["stone_color"].isNull()
                        ? "#7A92A3"
                        : row["stone_color"].as<std::string>();
                    context.match.moodType = row["mood_type"].isNull()
                        ? "neutral"
                        : heartlake::utils::normalizeMood(row["mood_type"].as<std::string>());
                    context.match.emotionScore = row["emotion_score"].isNull()
                        ? static_cast<float>(
                            heartlake::utils::scoreForMood(context.match.moodType))
                        : row["emotion_score"].as<float>();
                    context.match.boatCount = row["boat_count"].isNull()
                        ? 0
                        : row["boat_count"].as<int>();
                    context.match.authorName = row["author_name"].isNull()
                        ? ""
                        : row["author_name"].as<std::string>();
                    context.match.createdAt = row["created_at"].isNull()
                        ? ""
                        : row["created_at"].as<std::string>();
                    context.match.similarity = similarity;
                    context.match.semanticScore = similarity;
                    context.mood = row["mood_type"].isNull()
                        ? "neutral"
                        : heartlake::utils::normalizeMood(row["mood_type"].as<std::string>());
                    context.createdAt = row["created_at"].isNull()
                        ? ""
                        : row["created_at"].as<std::string>();
                    candidates.push_back(std::move(context));
                }
            }
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Resonance search failed: " << e.base().what();
            throw std::runtime_error(std::string("Resonance DB search failed: ") + e.base().what());
        }
    }

    if (candidates.empty()) return {};

    // Phase 2: 使用 EmotionResonanceEngine 四维重排序
    // ResonanceScore = α·Semantic + β·DTW + γ·Decay + δ·Diversity
    auto& resonanceEngine = ai::EmotionResonanceEngine::getInstance();
    auto db = drogon::app().getDbClient("default");

    // 获取源石头的 mood
    std::string sourceMood = "neutral";
    try {
        auto stoneRow = db->execSqlSync(
            "SELECT mood_type FROM stones "
            "WHERE stone_id = $1 "
            "  AND status = 'published' "
            "  AND deleted_at IS NULL",
            stoneId);
        if (!stoneRow.empty()) {
            if (!stoneRow[0]["mood_type"].isNull())
                sourceMood = heartlake::utils::normalizeMood(
                    stoneRow[0]["mood_type"].as<std::string>());
        } else {
            throw std::runtime_error("Source stone not found during resonance search");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Source stone lookup failed: ") + e.what());
    }

    // 加载请求用户的真实情绪轨迹。优先 emotion_tracking，
    // 其后使用画像、已发布石头、已交互石头和参考石头情绪信号补齐。
    ai::EmotionTrajectory userTraj;
    try {
        userTraj.userId = requesterUserId;
        userTraj.scores = loadRequesterTrajectoryScores(db, requesterUserId, stoneId);
        if (!userTraj.scores.empty()) {
            userTraj.currentScore = userTraj.scores.back();
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("User trajectory load failed: ") + e.what());
    }
    if (userTraj.scores.empty()) {
        throw std::runtime_error("Requester trajectory is unavailable for resonance search");
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
        if (candidateTrajectoryIt == trajectoryScoresByUser.end() ||
            candidateTrajectoryIt->second.empty()) {
            continue;
        }
        candTraj.scores = candidateTrajectoryIt->second;
        candTraj.currentScore = candTraj.scores.back();
        m.trajectoryScore =
            resonanceEngine.trajectorySimDTW(userTraj.scores, candTraj.scores);

        // 维度3: 时间衰减
        if (candidate.createdAt.empty()) {
            continue;
        }
        m.temporalScore = resonanceEngine.temporalDecay(candidate.createdAt);

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
        throw std::runtime_error(std::string("Failed to remove stone embedding: ") + e.base().what());
    }
}

} // namespace heartlake::infrastructure
