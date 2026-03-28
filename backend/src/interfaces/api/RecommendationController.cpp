/**
 * RecommendationController 模块实现
 */
#include "interfaces/api/RecommendationController.h"
#include "infrastructure/cache/RedisCache.h"
#include "infrastructure/ai/RecommendationEngine.h"
#include "infrastructure/services/ResonanceSearchService.h"
#include "utils/BusinessRules.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include <algorithm>
#include <atomic>
#include <memory>
#include <random>
#include <sstream>
#include <ctime>

using namespace heartlake::controllers;
using namespace heartlake::cache;
using namespace heartlake::utils;

namespace {

Json::Value buildStaticCollectionPayload(const std::string &primaryKey,
                                         const Json::Value &items) {
    const auto total = static_cast<int>(items.size());
    return ResponseUtil::buildCollectionPayload(primaryKey, items, total, 1,
                                                std::max(1, total));
}

Json::Value parseJsonArrayColumn(const drogon::orm::Row &row,
                                 const char *columnName) {
    Json::Value parsed(Json::arrayValue);
    if (columnName == nullptr || *columnName == '\0' || row[columnName].isNull()) {
        return parsed;
    }

    const auto raw = row[columnName].as<std::string>();
    if (raw.empty()) {
        return parsed;
    }

    Json::CharReaderBuilder builder;
    std::string errors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value value;
    if (!reader->parse(raw.data(), raw.data() + raw.size(), &value, &errors) ||
        !value.isArray()) {
        LOG_WARN << "parseJsonArrayColumn failed for " << columnName << ": " << errors;
        return parsed;
    }
    return value;
}

int extractWindowTotal(const drogon::orm::Result &result) {
    return result.empty() || result[0]["total_count"].isNull()
        ? 0
        : result[0]["total_count"].as<int>();
}

int resolveRecommendationLimit(const HttpRequestPtr &req,
                               int defaultLimit = 20,
                               int maxLimit = 50) {
    int limit = defaultLimit;
    const auto params = req->getParameters();
    if (params.count("limit")) {
        limit = safeInt(params.at("limit"), defaultLimit);
    }
    return std::clamp(limit, 1, maxLimit);
}

std::string resolveRecommendationReferenceStoneId(const std::string &userId) {
    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "WITH candidates AS ("
        "  SELECT s.stone_id, s.created_at, 0 AS priority "
        "  FROM stones s "
        "  WHERE s.user_id = $1 "
        "    AND s.status = 'published' "
        "    AND s.deleted_at IS NULL "
        "  UNION ALL "
        "  SELECT h.stone_id, h.created_at, 1 AS priority "
        "  FROM user_interaction_history h "
        "  JOIN stones s ON s.stone_id = h.stone_id "
        "  WHERE h.user_id = $1 "
        "    AND s.status = 'published' "
        "    AND s.deleted_at IS NULL "
        "  UNION ALL "
        "  SELECT r.stone_id, r.created_at, 2 AS priority "
        "  FROM ripples r "
        "  JOIN stones s ON s.stone_id = r.stone_id "
        "  WHERE r.user_id = $1 "
        "    AND s.status = 'published' "
        "    AND s.deleted_at IS NULL "
        "  UNION ALL "
        "  SELECT b.stone_id, b.created_at, 3 AS priority "
        "  FROM paper_boats b "
        "  JOIN stones s ON s.stone_id = b.stone_id "
        "  WHERE b.user_id = $1 "
        "    AND s.status = 'published' "
        "    AND s.deleted_at IS NULL "
        ") "
        "SELECT stone_id "
        "FROM candidates "
        "ORDER BY priority ASC, created_at DESC "
        "LIMIT 1",
        userId);

    const auto row = safeRow(result);
    if (!row || (*row)["stone_id"].isNull()) {
        return "";
    }
    return (*row)["stone_id"].as<std::string>();
}

std::string loadRequiredRecommendationReferenceStoneContent(
    const std::string &stoneId) {
    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "SELECT content FROM stones "
        "WHERE stone_id = $1 "
        "  AND status = 'published' "
        "  AND deleted_at IS NULL "
        "LIMIT 1",
        stoneId);
    const auto row = safeRow(result);
    if (!row || (*row)["content"].isNull()) {
        throw std::runtime_error("Reference stone content is unavailable");
    }

    const auto content = (*row)["content"].as<std::string>();
    if (content.empty()) {
        throw std::runtime_error("Reference stone content is empty");
    }
    return content;
}

bool doesRecommendationTargetUserExist(const std::string &userId) {
    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "SELECT user_id FROM users "
        "WHERE user_id = $1 AND status <> 'deleted' "
        "LIMIT 1",
        userId);
    return !result.empty();
}

bool doesRecommendationReferenceStoneExist(const std::string &stoneId) {
    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "SELECT stone_id FROM stones "
        "WHERE stone_id = $1 "
        "  AND status = 'published' "
        "  AND deleted_at IS NULL "
        "LIMIT 1",
        stoneId);
    return !result.empty();
}

std::string resolveRequiredRecommendationMood(
    const std::string &userId,
    const drogon::orm::DbClientPtr &dbClient) {
    auto userEmotionResult = dbClient->execSqlSync(
        "SELECT mood_type FROM user_emotion_profile "
        "WHERE user_id = $1 "
        "ORDER BY date DESC "
        "LIMIT 1",
        userId);

    if (userEmotionResult.empty() || userEmotionResult[0]["mood_type"].isNull()) {
        return "";
    }

    const auto mood = userEmotionResult[0]["mood_type"].as<std::string>();
    return mood.empty() ? "" : mood;
}

void respondWithAdvancedRecommendations(
    const std::string &userId,
    int limit,
    const std::string &requestedStoneId,
    std::function<void(const HttpResponsePtr &)> callback,
    Json::Value extraData = Json::Value(Json::objectValue)) {
    const std::string referenceStoneId = requestedStoneId.empty()
        ? resolveRecommendationReferenceStoneId(userId)
        : requestedStoneId;
    const bool usedAutoReference =
        requestedStoneId.empty() && !referenceStoneId.empty();
    const std::string referenceStoneContent = referenceStoneId.empty()
        ? ""
        : loadRequiredRecommendationReferenceStoneContent(referenceStoneId);

    auto &engine = heartlake::ai::RecommendationEngine::getInstance();
    engine.getRecommendations(
        userId,
        "stone",
        limit,
        [callback = std::move(callback),
         userId,
         referenceStoneId,
         referenceStoneContent,
         usedAutoReference,
         limit,
         extraData = std::move(extraData)](
            const std::vector<heartlake::ai::RecommendationCandidate> &results,
            const std::string &error) mutable {
            if (!error.empty()) {
                callback(ResponseUtil::internalError("推荐服务暂不可用"));
                return;
            }

            try {
                Json::Value recommendations(Json::arrayValue);
                std::unordered_set<std::string> addedIds;
                bool hasResonanceResults = false;

                if (!referenceStoneId.empty()) {
                    auto &resonanceSearchService =
                        heartlake::infrastructure::ResonanceSearchService::getInstance();
                    auto resonanceResults = resonanceSearchService.searchResonance(
                        userId,
                        referenceStoneId,
                        referenceStoneContent,
                        0.85f,
                        std::max(1, limit / 2));

                    for (const auto &res : resonanceResults) {
                        Json::Value item;
                        item["stone_id"] = res.stoneId;
                        item["score"] = res.totalScore;
                        item["reason"] = res.resonanceReason;
                        item["algorithm"] = "emotion_temporal_resonance";
                        item["semantic_score"] = res.semanticScore;
                        item["trajectory_score"] = res.trajectoryScore;
                        item["temporal_score"] = res.temporalScore;
                        item["diversity_score"] = res.diversityScore;
                        item["content"] = res.content;
                        item["mood_type"] = res.moodType;
                        item["emotion_score"] = res.emotionScore;
                        item["author_id"] = res.userId;
                        item["author_name"] = res.authorName;
                        item["ripple_count"] = res.rippleCount;
                        item["created_at"] = res.createdAt;
                        item["reference_stone_id"] = referenceStoneId;
                        recommendations.append(item);
                        addedIds.insert(res.stoneId);
                        hasResonanceResults = true;
                    }
                }

                for (const auto &cand : results) {
                    if (addedIds.count(cand.itemId)) continue;
                    if (static_cast<int>(recommendations.size()) >= limit) break;

                    Json::Value item;
                    item["stone_id"] = cand.itemId;
                    item["score"] = cand.score;
                    item["reason"] = cand.reason;
                    item["algorithm"] = cand.algorithm;
                    item["content"] = cand.metadata.get("content", "").asString();
                    item["mood_type"] =
                        cand.metadata.get("mood_type", "").asString();
                    item["emotion_score"] =
                        cand.metadata.get("emotion_score", 0.0).asDouble();
                    item["author_id"] =
                        cand.metadata.get("author_id", "").asString();
                    item["author_name"] =
                        cand.metadata.get("author_name", "").asString();
                    item["ripple_count"] =
                        cand.metadata.get("ripple_count", 0).asInt();
                    item["created_at"] =
                        cand.metadata.get("created_at", "").asString();
                    recommendations.append(item);
                }

                Json::Value data =
                    buildStaticCollectionPayload("recommendations", recommendations);
                data["count"] = static_cast<int>(recommendations.size());
                data["algorithm"] = hasResonanceResults
                    ? "emotion_resonance_hybrid"
                    : "multi_armed_bandit_mmr";
                data["user_id"] = userId;
                if (hasResonanceResults) {
                    data["reference_stone_id"] = referenceStoneId;
                    data["reference_source"] =
                        usedAutoReference ? "auto_latest_context" : "explicit";
                }
                if (extraData.isObject()) {
                    for (const auto &member : extraData.getMemberNames()) {
                        data[member] = extraData[member];
                    }
                }

                callback(ResponseUtil::success(data, "为你精选的内容"));
            } catch (const std::exception &e) {
                LOG_ERROR << "Failed to build advanced recommendations for user "
                          << userId << ": " << e.what();
                callback(ResponseUtil::internalError("获取高级推荐失败"));
            }
        });
}

} // namespace

/**
 * 获取推荐的石头
 * 混合推荐算法：协同过滤 + 内容过滤 + 随机探索
 */
void RecommendationController::getRecommendedStones(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;

    auto cb = std::make_shared<std::function<void(const HttpResponsePtr &)>>(std::move(callback));
    try {
        auto dbClient = drogon::app().getDbClient("default");

        // 检查缓存
        auto& cache = RedisCache::getInstance();
        std::string cacheKey = "recommendations:stones:" + userId;

        cache.get(cacheKey, [this, userId, dbClient, cb](const std::string& cachedData, bool exists) mutable {
            if (exists && !cachedData.empty()) {
                Json::Value cached;
                Json::Reader reader;
                if (reader.parse(cachedData, cached)) {
                    (*cb)(ResponseUtil::success(cached, "推荐内容（来自缓存）"));
                    return;
                }
            }

            // 缓存未命中，计算推荐
            this->calculateStoneRecommendations(userId, dbClient, [cb](const HttpResponsePtr &resp) { (*cb)(resp); });
        });

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getRecommendedStones: " << e.what();
        (*cb)(ResponseUtil::internalError("获取推荐失败"));
    }
}

/**
 * 计算石头推荐
 */
void RecommendationController::calculateStoneRecommendations(
    const std::string &userId,
    const orm::DbClientPtr &dbClient,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    // 1. 获取用户最近的情绪状态。没有真实画像时直接失败，不再默认 calm。
    std::string userMood;
    try {
        userMood = resolveRequiredRecommendationMood(userId, dbClient);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Failed to load recommendation mood for user " << userId
                  << ": " << e.base().what();
        callback(ResponseUtil::internalError("读取用户情绪画像失败"));
        return;
    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to resolve recommendation mood for user " << userId
                  << ": " << e.what();
        callback(ResponseUtil::internalError("读取用户情绪画像失败"));
        return;
    }

    if (userMood.empty()) {
        LOG_WARN << "Recommendation mood missing for user " << userId;
        callback(ResponseUtil::conflict("用户情绪画像不存在，无法生成推荐"));
        return;
    }

    try {
        Json::Value recommendations(Json::arrayValue);
        std::unordered_set<std::string> addedStones;

        auto appendStone = [&](const drogon::orm::Row &row,
                               const std::string &reason,
                               const std::string &type) {
            std::string stoneId = row["stone_id"].as<std::string>();
            if (addedStones.find(stoneId) == addedStones.end()) {
                Json::Value stone;
                stone["stone_id"] = stoneId;
                stone["content"] = row["content"].as<std::string>();
                stone["mood_type"] = row["mood_type"].as<std::string>();
                stone["emotion_score"] = row["emotion_score"].as<double>();
                stone["author_name"] = row["author_name"].as<std::string>();
                stone["ripple_count"] = row["ripple_count"].as<int>();
                stone["created_at"] = row["created_at"].as<std::string>();
                stone["recommendation_reason"] = reason;
                stone["recommendation_type"] = type;
                recommendations.append(stone);
                addedStones.insert(stoneId);
            }
        };

        auto collaborativeStones = dbClient->execSqlSync(
            "SELECT DISTINCT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "s.created_at, u.nickname AS author_name, "
            "COALESCE(s.ripple_count, 0) AS ripple_count "
            "FROM stones s "
            "JOIN users u ON s.user_id = u.user_id "
            "JOIN user_interaction_history uih ON s.stone_id = uih.stone_id "
            "WHERE uih.user_id IN ( "
            "  SELECT user2_id FROM user_similarity "
            "  WHERE user1_id = $1 AND similarity_score > 0.6 "
            "  ORDER BY similarity_score DESC LIMIT 10 "
            ") "
            "AND s.user_id != $1 "
            "AND s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND NOT EXISTS (SELECT 1 FROM user_interaction_history h WHERE h.stone_id = s.stone_id AND h.user_id = $1) "
            "ORDER BY COALESCE(s.ripple_count, 0) DESC, s.created_at DESC "
            "LIMIT 20",
            userId);
        for (size_t i = 0; i < collaborativeStones.size() && recommendations.size() < 8; ++i) {
            appendStone(collaborativeStones[i],
                        "和你有相似感受的人也喜欢这个", "collaborative");
        }

        auto contentStones = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "s.created_at, u.nickname AS author_name, "
            "COALESCE(s.ripple_count, 0) AS ripple_count, "
            "ec.compatibility_score, ec.relationship_type "
            "FROM stones s "
            "JOIN users u ON s.user_id = u.user_id "
            "LEFT JOIN emotion_compatibility ec ON "
            "  (ec.mood_type_1 = $2 AND ec.mood_type_2 = s.mood_type) OR "
            "  (ec.mood_type_2 = $2 AND ec.mood_type_1 = s.mood_type) "
            "WHERE s.user_id != $1 "
            "AND s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND NOT EXISTS (SELECT 1 FROM user_interaction_history h WHERE h.stone_id = s.stone_id AND h.user_id = $1) "
            "AND ec.compatibility_score > 0.6 "
            "ORDER BY ec.compatibility_score DESC, s.created_at DESC "
            "LIMIT 20",
            userId, userMood);
        for (size_t i = 0; i < contentStones.size() && recommendations.size() < 16; ++i) {
            auto row = contentStones[i];
            std::string relationshipType = row["relationship_type"].as<std::string>();
            std::string reason = generateRecommendationReason(
                userMood, row["mood_type"].as<std::string>(), relationshipType);
            appendStone(row, reason, "emotion_compatible");
        }

        auto explorationStones = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "s.created_at, u.nickname AS author_name, "
            "COALESCE(s.ripple_count, 0) AS ripple_count "
            "FROM stones s "
            "JOIN users u ON s.user_id = u.user_id "
            "WHERE s.user_id != $1 "
            "AND s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND s.created_at >= NOW() - INTERVAL '14 days' "
            "AND NOT EXISTS (SELECT 1 FROM user_interaction_history h WHERE h.stone_id = s.stone_id AND h.user_id = $1) "
            "ORDER BY s.created_at DESC "
            "LIMIT 160",
            userId);

        std::vector<orm::Row> explorationCandidates;
        explorationCandidates.reserve(explorationStones.size());
        for (const auto &row : explorationStones) {
            explorationCandidates.push_back(row);
        }
        const auto bucket = static_cast<long long>(std::time(nullptr) / 1800);
        std::mt19937 rng(static_cast<uint32_t>(
            std::hash<std::string>{}(userId + ":" + std::to_string(bucket))));
        std::shuffle(explorationCandidates.begin(), explorationCandidates.end(), rng);

        for (size_t i = 0; i < explorationCandidates.size() && recommendations.size() < 20; ++i) {
            appendStone(explorationCandidates[i],
                        "也许你会喜欢这个意外的发现", "exploration");
        }

        Json::Value responseData;
        responseData["recommendations"] = recommendations;
        responseData["total"] = static_cast<int>(recommendations.size());
        responseData["user_mood"] = userMood;

        auto &cache = RedisCache::getInstance();
        std::string cacheKey = "recommendations:stones:" + userId;
        Json::StreamWriterBuilder builder;
        std::string jsonStr = Json::writeString(builder, responseData);
        cache.setEx(cacheKey, jsonStr, 300);

        callback(ResponseUtil::success(responseData, "为你精心挑选的内容"));
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Recommendation query failed for user " << userId
                  << ": " << e.base().what();
        callback(ResponseUtil::internalError("生成推荐失败"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Recommendation assembly failed for user " << userId
                  << ": " << e.what();
        callback(ResponseUtil::internalError("生成推荐失败"));
    }
}

/**
 * 记录用户交互
 */
void RecommendationController::trackInteraction(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;

    try {
        auto json = req->getJsonObject();

        if (!json) {
            callback(ResponseUtil::badRequest("请求体不能为空"));
            return;
        }

        std::string stoneId = (*json)["stone_id"].asString();
        std::string interactionType = (*json)["interaction_type"].asString(); // view, ripple, boat, share
        int dwellTime = (*json).get("dwell_time_seconds", 0).asInt();

        // 计算交互权重
        double weight = interactionWeightForType(interactionType);

        auto dbClient = drogon::app().getDbClient("default");

        // 记录交互
        dbClient->execSqlAsync(
            "INSERT INTO user_interaction_history "
            "(user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds) "
            "VALUES ($1, $2, $3, $4, $5) "
            "ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE SET "
            "interaction_weight = GREATEST(user_interaction_history.interaction_weight, EXCLUDED.interaction_weight), "
            "dwell_time_seconds = GREATEST(user_interaction_history.dwell_time_seconds, EXCLUDED.dwell_time_seconds), "
            "created_at = NOW()",
            [callback](const orm::Result&) {
                Json::Value data;
                data["tracked"] = true;
                callback(ResponseUtil::success(data, "交互已记录"));
            },
            [callback](const orm::DrogonDbException &e) {
                LOG_ERROR << "Failed to track interaction: " << e.base().what();
                callback(ResponseUtil::internalError("记录失败"));
            },
            userId, stoneId, interactionType, weight, dwellTime
        );

        // 异步更新用户偏好
        updateUserPreferences(userId, dbClient);

        // 通知推荐引擎进行在线学习
        heartlake::ai::RecommendationEngine::getInstance()
            .recordInteraction(userId, stoneId, interactionType, weight);

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in trackInteraction: " << e.what();
        callback(ResponseUtil::internalError("记录交互失败"));
    }
}

/**
 * 生成温馨的推荐理由
 */
std::string RecommendationController::generateRecommendationReason(
    const std::string &mood1,
    const std::string &mood2,
    const std::string &relationshipType) {

    if (relationshipType == "similar") {
        // 相似情绪的温馨表达
        if (mood1 == "happy") return "你们的快乐在同一个频率上共鸣";
        if (mood1 == "sad") return "你们的心在同一片云下相遇";
        if (mood1 == "anxious") return "你们都在寻找内心的平静";
        if (mood1 == "calm") return "你们都在享受这份宁静";
        return "你们有着相似的感受";
    } else if (relationshipType == "complementary") {
        // 互补情绪的温馨表达
        if (mood1 == "sad" && mood2 == "happy") {
            return "这份阳光也许能照亮你的心房";
        } else if (mood1 == "anxious" && mood2 == "calm") {
            return "这份平静也许能抚慰你的心灵";
        } else if (mood1 == "angry" && mood2 == "calm") {
            return "这份温柔也许能融化你的情绪";
        } else if (mood1 == "happy" && mood2 == "sad") {
            return "你的快乐也许能温暖TA的心";
        }
        return "这个视角也许能帮助你";
    } else if (relationshipType == "supportive") {
        return "这个人也许能给你需要的支持";
    }

    return "也许你会喜欢这个发现";
}

/**
 * 更新用户偏好
 */
void RecommendationController::updateUserPreferences(
    const std::string &userId,
    const orm::DbClientPtr &dbClient) {

    // 分析最近30天的交互，更新偏好
    dbClient->execSqlAsync(
        "WITH recent_items AS ("
        "  SELECT DISTINCT uih.stone_id "
        "  FROM user_interaction_history uih "
        "  WHERE uih.user_id = $1 "
        "    AND uih.created_at >= NOW() - INTERVAL '30 days' "
        "    AND uih.interaction_weight >= 1.0"
        "), expanded AS ("
        "  SELECT "
        "    COALESCE(s.mood_type, 'neutral') AS mood_type, "
        "    tag "
        "  FROM recent_items ri "
        "  JOIN stones s ON ri.stone_id = s.stone_id "
        "  LEFT JOIN LATERAL unnest("
        "    COALESCE(s.tags, '{}'::text[]) || COALESCE(s.ai_tags, '{}'::text[])"
        "  ) AS tag ON true"
        ") "
        "INSERT INTO user_preferences (user_id, preferred_moods, preferred_tags, last_updated) "
        "SELECT "
        "  $1 as user_id, "
        "  COALESCE(ARRAY_REMOVE(ARRAY_AGG(DISTINCT mood_type), NULL), '{}'::text[]) as preferred_moods, "
        "  COALESCE(ARRAY_REMOVE(ARRAY_AGG(DISTINCT tag), NULL), '{}'::text[]) as preferred_tags, "
        "  NOW() as last_updated "
        "FROM expanded "
        "ON CONFLICT (user_id) DO UPDATE SET "
        "  preferred_moods = EXCLUDED.preferred_moods, "
        "  preferred_tags = EXCLUDED.preferred_tags, "
        "  last_updated = NOW()",
        [](const orm::Result&) {
            LOG_DEBUG << "User preferences updated";
        },
        [](const orm::DrogonDbException &e) {
            LOG_ERROR << "Failed to update preferences: " << e.base().what();
        },
        userId
    );
}

/**
 * 获取情绪趋势
 */
void RecommendationController::getEmotionTrends(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    try {
        auto& userId = *userIdOpt;
        auto dbClient = drogon::app().getDbClient("default");

        // 获取最近30天按“天”聚合的情绪数据，避免同一天返回多条趋势点。
        auto trendsResult = dbClient->execSqlSync(
            "WITH scored_stones AS ("
            "  SELECT DATE(created_at) AS date, "
            "         COALESCE(mood_type, 'neutral') AS mood_type, "
            "         COALESCE(emotion_score, "
            "           CASE COALESCE(mood_type, 'neutral') "
            "             WHEN 'happy' THEN 0.75 "
            "             WHEN 'calm' THEN 0.35 "
            "             WHEN 'neutral' THEN 0.0 "
            "             WHEN 'hopeful' THEN 0.55 "
            "             WHEN 'grateful' THEN 0.65 "
            "             WHEN 'sad' THEN -0.75 "
            "             WHEN 'anxious' THEN -0.45 "
            "             WHEN 'angry' THEN -0.65 "
            "             WHEN 'lonely' THEN -0.55 "
            "             WHEN 'confused' THEN -0.1 "
            "             ELSE 0.0 "
            "           END"
            "         ) AS normalized_score "
            "  FROM stones "
            "  WHERE user_id = $1 "
            "    AND status = 'published' "
            "    AND deleted_at IS NULL "
            "    AND created_at >= CURRENT_DATE - INTERVAL '30 days'"
            "), daily_scores AS ("
            "  SELECT date, AVG(normalized_score) AS avg_emotion_score, COUNT(*) AS stone_count "
            "  FROM scored_stones "
            "  GROUP BY date"
            "), ranked_moods AS ("
            "  SELECT date, mood_type, mood_count, avg_score, "
            "         ROW_NUMBER() OVER ("
            "           PARTITION BY date "
            "           ORDER BY mood_count DESC, ABS(avg_score) DESC, mood_type ASC"
            "         ) AS rn "
            "  FROM ("
            "    SELECT date, mood_type, COUNT(*) AS mood_count, AVG(normalized_score) AS avg_score "
            "    FROM scored_stones "
            "    GROUP BY date, mood_type"
            "  ) mood_stats"
            ") "
            "SELECT ds.date, ds.avg_emotion_score, ds.stone_count, "
            "       COALESCE(rm.mood_type, 'neutral') AS mood_type "
            "FROM daily_scores ds "
            "LEFT JOIN ranked_moods rm ON rm.date = ds.date AND rm.rn = 1 "
            "ORDER BY ds.date ASC",
            userId
        );

        Json::Value trends(Json::arrayValue);

        for (size_t i = 0; i < trendsResult.size(); ++i) {
            auto row = trendsResult[i];

            Json::Value trend;
            trend["date"] = row["date"].as<std::string>();
            trend["emotion_score"] = row["avg_emotion_score"].as<double>();
            trend["mood"] = row["mood_type"].as<std::string>();
            trend["stone_count"] = row["stone_count"].as<int>();
            trend["interaction_count"] = 0;

            trends.append(trend);
        }

        Json::Value responseData = buildStaticCollectionPayload("trends", trends);
        responseData["period_days"] = 30;

        callback(ResponseUtil::success(responseData, "你的情绪旅程"));

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getEmotionTrends: " << e.what();
        callback(ResponseUtil::internalError("获取趋势失败"));
    }
}

/**
 * 基于情绪的发现
 */
void RecommendationController::discoverByMood(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &mood) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    try {
        auto& userId = *userIdOpt;
        auto dbClient = drogon::app().getDbClient("default");

        // 查找当前处于该情绪的用户和内容
        auto stonesResult = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "s.created_at, u.nickname as author_name, "
            "COALESCE(s.ripple_count, 0) as ripple_count "
            "FROM stones s "
            "JOIN users u ON s.user_id = u.user_id "
            "WHERE s.mood_type = $1 "
            "AND s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND s.user_id != $2 "
            "ORDER BY s.created_at DESC "
            "LIMIT 20",
            mood, userId
        );

        Json::Value stones(Json::arrayValue);
        for (size_t i = 0; i < stonesResult.size(); ++i) {
            auto row = stonesResult[i];
            Json::Value stone;
            stone["stone_id"] = row["stone_id"].as<std::string>();
            stone["content"] = row["content"].as<std::string>();
            stone["mood_type"] = row["mood_type"].as<std::string>();
            stone["emotion_score"] = row["emotion_score"].as<double>();
            stone["author_name"] = row["author_name"].as<std::string>();
            stone["ripple_count"] = row["ripple_count"].as<int>();
            stone["created_at"] = row["created_at"].as<std::string>();
            stones.append(stone);
        }

        Json::Value responseData = buildStaticCollectionPayload("stones", stones);
        responseData["mood"] = mood;

        callback(ResponseUtil::success(responseData, "发现同样感受的人"));

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in discoverByMood: " << e.what();
        callback(ResponseUtil::internalError("发现失败"));
    }
}

/**
 * 获取热门内容
 */
void RecommendationController::getTrendingContent(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto cb = std::make_shared<std::function<void(const HttpResponsePtr &)>>(std::move(callback));
    try {
        const int requestedLimit = safeInt(req->getParameter("limit"), 10);
        const int limit = std::clamp(requestedLimit, 1, 50);
        // 检查缓存（热门内容缓存3分钟）
        auto& cache = RedisCache::getInstance();
        std::string cacheKey = "recommendations:trending:" + std::to_string(limit);

        cache.get(cacheKey, [this, cb, limit](const std::string& cachedData, bool exists) mutable {
            try {
                if (exists && !cachedData.empty()) {
                    Json::Value cached;
                    Json::Reader reader;
                    if (reader.parse(cachedData, cached)) {
                        (*cb)(ResponseUtil::success(cached, "当前热门（来自缓存）"));
                        return;
                    }
                }

                this->calculateTrendingContent(
                    limit,
                    [cb](const HttpResponsePtr &resp) { (*cb)(resp); });
            } catch (const std::exception &e) {
                LOG_ERROR << "Error in getTrendingContent Redis callback: " << e.what();
                (*cb)(ResponseUtil::internalError("获取热门内容失败"));
            }
        });

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getTrendingContent: " << e.what();
        (*cb)(ResponseUtil::internalError("获取热门内容失败"));
    }
}

/**
 * 计算热门内容（内部方法）
 */
void RecommendationController::calculateTrendingContent(
    int limit,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "WITH recent_stones AS ("
            "  SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "         COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "         s.created_at, u.nickname AS author_name, "
            "         COALESCE(s.ripple_count, 0) AS ripple_count "
            "  FROM stones s "
            "  JOIN users u ON s.user_id = u.user_id "
            "  WHERE s.status = 'published' "
            "    AND s.deleted_at IS NULL "
            "    AND s.created_at >= NOW() - INTERVAL '24 hours'"
            "), top_stones AS ("
            "  SELECT * FROM recent_stones "
            "  ORDER BY ripple_count DESC, created_at DESC "
            "  LIMIT $1"
            "), top_moods AS ("
            "  SELECT mood_type, COUNT(*)::INTEGER AS count "
            "  FROM recent_stones "
            "  GROUP BY mood_type "
            "  ORDER BY count DESC, mood_type ASC "
            "  LIMIT 5"
            ") "
            "SELECT "
            "  COALESCE(("
            "    SELECT json_agg("
            "      json_build_object("
            "        'stone_id', stone_id, "
            "        'content', content, "
            "        'mood_type', mood_type, "
            "        'emotion_score', emotion_score, "
            "        'author_name', author_name, "
            "        'ripple_count', ripple_count, "
            "        'created_at', created_at"
            "      ) "
            "      ORDER BY ripple_count DESC, created_at DESC"
            "    ) "
            "    FROM top_stones"
            "  ), '[]'::json) AS trending_stones, "
            "  COALESCE(("
            "    SELECT json_agg("
            "      json_build_object("
            "        'mood_type', mood_type, "
            "        'count', count"
            "      ) "
            "      ORDER BY count DESC, mood_type ASC"
            "    ) "
            "    FROM top_moods"
            "  ), '[]'::json) AS trending_moods",
            static_cast<int64_t>(limit));

        Json::Value trendingStones(Json::arrayValue);
        Json::Value trendingMoods(Json::arrayValue);
        if (auto rowOpt = safeRow(result)) {
            trendingStones = parseJsonArrayColumn(*rowOpt, "trending_stones");
            trendingMoods = parseJsonArrayColumn(*rowOpt, "trending_moods");
        }

        Json::Value responseData = buildStaticCollectionPayload(
            "trending_stones", trendingStones);
        responseData["trending_moods"] = trendingMoods;

        // 缓存结果（3分钟）
        auto& cache = RedisCache::getInstance();
        Json::StreamWriterBuilder builder;
        std::string jsonStr = Json::writeString(builder, responseData);
        cache.setEx("recommendations:trending:" + std::to_string(limit), jsonStr, 180);

        callback(ResponseUtil::success(responseData, "当前热门"));

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in calculateTrendingContent: " << e.what();
        callback(ResponseUtil::internalError("获取热门内容失败"));
    }
}

/**
 * 搜索推荐内容
 * 全文搜索 + 智能排序
 */
void RecommendationController::searchRecommendations(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;

    try {
        auto json = req->getJsonObject();

        if (!json) {
            callback(ResponseUtil::badRequest("请求体不能为空"));
            return;
        }

        std::string query = (*json).get("query", "").asString();
        int page = std::max(1, (*json).get("page", 1).asInt());
        int pageSize = std::clamp((*json).get("page_size", 20).asInt(), 1, 100);

        if (query.empty()) {
            callback(ResponseUtil::badRequest("搜索关键词不能为空"));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");

        // 构建搜索SQL
        std::string searchSql =
            "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "s.created_at, u.nickname as author_name, u.user_id as author_id, "
            "COALESCE(s.ripple_count, 0) AS ripple_count, "
            "COALESCE(s.boat_count, 0) AS boat_count, "
            "COUNT(*) OVER() AS total_count "
            "FROM stones s "
            "JOIN users u ON s.user_id = u.user_id "
            "WHERE s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND s.user_id != $1 "
            "AND s.content ILIKE $2 ESCAPE '\\' ";

        std::string searchPattern = "%" + escapeLike(query) + "%";

        // 排序和分页（使用参数化查询防止注入）
        searchSql += "ORDER BY s.created_at DESC ";
        searchSql += "LIMIT $3 OFFSET $4";

        int offset = (page - 1) * pageSize;

        // 执行搜索
        auto searchResult = dbClient->execSqlSync(searchSql, userId, searchPattern, pageSize, offset);
        if (searchResult.empty() && page > 1) {
            callback(ResponseUtil::badRequest("页码超出范围"));
            return;
        }

        Json::Value results(Json::arrayValue);
        for (size_t i = 0; i < searchResult.size(); ++i) {
            auto row = searchResult[i];
            Json::Value stone;
            stone["stone_id"] = row["stone_id"].as<std::string>();
            stone["content"] = row["content"].as<std::string>();
            stone["mood_type"] = row["mood_type"].as<std::string>();
            stone["emotion_score"] = row["emotion_score"].as<double>();
            stone["author_name"] = row["author_name"].as<std::string>();
            stone["author_id"] = row["author_id"].as<std::string>();
            stone["ripple_count"] = row["ripple_count"].as<int>();
            stone["boat_count"] = row["boat_count"].as<int>();
            stone["created_at"] = row["created_at"].as<std::string>();
            stone["recommendation_reason"] = "搜索结果";
            stone["recommendation_type"] = "search";
            results.append(stone);
        }
        const int total = extractWindowTotal(searchResult);

        callback(ResponseUtil::success(
            ResponseUtil::buildCollectionPayload("results", results, total,
                                                 page, pageSize),
            "搜索结果"));

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in searchRecommendations: " << e.what();
        callback(ResponseUtil::internalError("搜索失败"));
    }
}

/**
 * 批量追踪交互
 */
void RecommendationController::trackBatchInteractions(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;

    auto cb = std::make_shared<std::function<void(const HttpResponsePtr &)>>(std::move(callback));
    try {
        auto json = req->getJsonObject();

        if (!json || !json->isMember("interactions")) {
            (*cb)(ResponseUtil::badRequest("请求体不能为空"));
            return;
        }

        const auto& interactions = (*json)["interactions"];
        if (!interactions.isArray() || interactions.size() == 0) {
            (*cb)(ResponseUtil::badRequest("交互数据不能为空"));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");

        struct InteractionPayload {
            std::string stoneId;
            std::string interactionType;
            double weight{0.1};
            int dwellTime{0};
        };
        std::vector<InteractionPayload> validInteractions;
        validInteractions.reserve(interactions.size());

        // 批量插入交互记录
        for (size_t i = 0; i < interactions.size(); ++i) {
            const auto& interaction = interactions[static_cast<int>(i)];

            std::string stoneId = interaction.get("stone_id", "").asString();
            std::string interactionType = interaction.get("interaction_type", "").asString();
            int dwellTime = interaction.get("dwell_time_seconds", 0).asInt();

            if (stoneId.empty() || interactionType.empty()) {
                continue;
            }

            // 计算交互权重
            double weight = interactionWeightForType(interactionType);
            validInteractions.push_back({stoneId, interactionType, weight, dwellTime});
        }

        if (validInteractions.empty()) {
            Json::Value responseData;
            responseData["tracked"] = 0;
            responseData["total"] = static_cast<int>(interactions.size());
            (*cb)(ResponseUtil::success(responseData, "批量追踪成功"));
            return;
        }

        auto pendingCount = std::make_shared<std::atomic<int>>(
            static_cast<int>(validInteractions.size()));
        auto successCount = std::make_shared<std::atomic<int>>(0);
        auto responded = std::make_shared<std::atomic<bool>>(false);
        const int totalCount = static_cast<int>(interactions.size());

        auto finishOne = [this, userId, dbClient, cb, pendingCount, successCount, responded, totalCount]() {
            int left = pendingCount->fetch_sub(1) - 1;
            if (left > 0) {
                return;
            }
            bool expected = false;
            if (!responded->compare_exchange_strong(expected, true)) {
                return;
            }

            // 所有插入完成后再更新偏好，确保统计口径真实。
            updateUserPreferences(userId, dbClient);

            Json::Value responseData;
            responseData["tracked"] = successCount->load();
            responseData["total"] = totalCount;
            (*cb)(ResponseUtil::success(responseData, "批量追踪成功"));
        };

        for (const auto& payload : validInteractions) {
            try {
                dbClient->execSqlAsync(
                    "INSERT INTO user_interaction_history "
                    "(user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds) "
                    "VALUES ($1, $2, $3, $4, $5) "
                    "ON CONFLICT (user_id, stone_id, interaction_type) DO UPDATE SET "
                    "interaction_weight = GREATEST(user_interaction_history.interaction_weight, EXCLUDED.interaction_weight), "
                    "dwell_time_seconds = GREATEST(user_interaction_history.dwell_time_seconds, EXCLUDED.dwell_time_seconds), "
                    "created_at = NOW()",
                    [successCount, finishOne](const orm::Result &) {
                        successCount->fetch_add(1);
                        finishOne();
                    },
                    [finishOne](const orm::DrogonDbException &e) {
                        LOG_ERROR << "Failed to insert interaction: " << e.base().what();
                        finishOne();
                    },
                    userId, payload.stoneId, payload.interactionType, payload.weight, payload.dwellTime
                );
            } catch (const std::exception& e) {
                LOG_ERROR << "Failed to schedule interaction insert: " << e.what();
                finishOne();
            }
        }

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in trackBatchInteractions: " << e.what();
        (*cb)(ResponseUtil::internalError("批量追踪失败"));
    }
}

/**
 * 高级推荐（多算法融合）
 * GET /api/v1/recommendations/advanced
 * 可选参数 stone_id: 提供时启用情绪感知时序共鸣算法
 */
void RecommendationController::getAdvancedRecommendations(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;

    try {
        const int limit = resolveRecommendationLimit(req);
        const auto params = req->getParameters();
        const std::string requestedStoneId =
            params.count("stone_id") ? params.at("stone_id") : "";
        if (!requestedStoneId.empty() &&
            !doesRecommendationReferenceStoneExist(requestedStoneId)) {
            callback(ResponseUtil::notFound("参考石头不存在"));
            return;
        }

        respondWithAdvancedRecommendations(
            userId,
            limit,
            requestedStoneId,
            std::move(callback));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getAdvancedRecommendations: " << e.what();
        callback(ResponseUtil::internalError("获取推荐失败"));
    }
}

void RecommendationController::getAdminAdvancedRecommendations(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        const auto adminId = req->getAttributes()->get<std::string>("admin_id");
        const std::string userId = req->getParameter("user_id");
        if (userId.empty()) {
            callback(ResponseUtil::badRequest("缺少 user_id"));
            return;
        }
        if (!doesRecommendationTargetUserExist(userId)) {
            callback(ResponseUtil::notFound("目标用户不存在"));
            return;
        }

        const int limit = resolveRecommendationLimit(req);
        const std::string requestedStoneId = req->getParameter("stone_id");
        if (!requestedStoneId.empty() &&
            !doesRecommendationReferenceStoneExist(requestedStoneId)) {
            callback(ResponseUtil::notFound("参考石头不存在"));
            return;
        }
        Json::Value extraData(Json::objectValue);
        extraData["inspected_user_id"] = userId;
        if (!adminId.empty()) {
            extraData["requested_by_admin_id"] = adminId;
        }

        respondWithAdvancedRecommendations(
            userId,
            limit,
            requestedStoneId,
            std::move(callback),
            std::move(extraData));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getAdminAdvancedRecommendations: " << e.what();
        callback(ResponseUtil::internalError("获取高级推荐失败"));
    }
}
