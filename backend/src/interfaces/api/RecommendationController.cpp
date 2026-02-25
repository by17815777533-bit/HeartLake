/**
 * @file RecommendationController.cpp
 * @brief RecommendationController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/RecommendationController.h"
#include "infrastructure/cache/RedisCache.h"
#include "infrastructure/ai/RecommendationEngine.h"
#include "infrastructure/ai/EmotionResonanceEngine.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include <atomic>
#include <random>
#include <sstream>
#include <ctime>

using namespace heartlake::controllers;
using namespace heartlake::cache;
using namespace heartlake::utils;

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

    // 1. 获取用户最近的情绪状态（容错：表不存在时使用默认值）
    std::string userMood = "calm"; // 默认
    try {
        auto userEmotionResult = dbClient->execSqlSync(
            "SELECT mood_type, avg_emotion_score FROM user_emotion_profile "
            "WHERE user_id = $1 ORDER BY date DESC LIMIT 7",
            userId
        );
        if (!userEmotionResult.empty()) {
            userMood = userEmotionResult[0]["mood_type"].as<std::string>();
        }
    } catch (const std::exception &e) {
        LOG_WARN << "获取用户情绪档案失败(降级为默认mood): " << e.what();
    }

    // 6. 合并结果并添加推荐理由
    Json::Value recommendations(Json::arrayValue);
    std::unordered_set<std::string> addedStones;

    // 辅助lambda：将查询行转为Json并去重添加
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

    // 2. 协同过滤：找到相似用户喜欢的内容（40%）
    try {
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
            userId
        );
        for (size_t i = 0; i < collaborativeStones.size() && recommendations.size() < 8; ++i) {
            appendStone(collaborativeStones[i],
                        "和你有相似感受的人也喜欢这个", "collaborative");
        }
    } catch (const std::exception &e) {
        LOG_WARN << "协同过滤查询失败(跳过): " << e.what();
    }

    // 4. 内容过滤：基于情绪兼容性的内容（40%）
    try {
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
            userId, userMood
        );
        for (size_t i = 0; i < contentStones.size() && recommendations.size() < 16; ++i) {
            auto row = contentStones[i];
            std::string relationshipType = row["relationship_type"].as<std::string>();
            std::string reason = generateRecommendationReason(
                userMood, row["mood_type"].as<std::string>(), relationshipType);
            appendStone(row, reason, "emotion_compatible");
        }
    } catch (const std::exception &e) {
        LOG_WARN << "内容过滤查询失败(跳过): " << e.what();
    }

    // 5. 随机探索：发现新内容（20%）
    try {
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
            userId
        );

        std::vector<orm::Row> explorationCandidates;
        explorationCandidates.reserve(explorationStones.size());
        for (const auto& row : explorationStones) {
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
    } catch (const std::exception &e) {
        LOG_WARN << "随机探索查询失败(尝试降级查询): " << e.what();
        // 降级：不依赖 user_interaction_history 表
        try {
            auto fallbackStones = dbClient->execSqlSync(
                "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
                "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
                "s.created_at, u.nickname AS author_name, "
                "COALESCE(s.ripple_count, 0) AS ripple_count "
                "FROM stones s "
                "JOIN users u ON s.user_id = u.user_id "
                "WHERE s.user_id != $1 "
                "AND s.status = 'published' "
                "AND s.deleted_at IS NULL "
                "ORDER BY s.created_at DESC "
                "LIMIT 160",
                userId
            );
            std::vector<orm::Row> fallbackCandidates;
            fallbackCandidates.reserve(fallbackStones.size());
            for (const auto& row : fallbackStones) {
                fallbackCandidates.push_back(row);
            }
            const auto bucket = static_cast<long long>(std::time(nullptr) / 1800);
            std::mt19937 rng(static_cast<uint32_t>(
                std::hash<std::string>{}(userId + ":" + std::to_string(bucket) + ":fallback")));
            std::shuffle(fallbackCandidates.begin(), fallbackCandidates.end(), rng);
            for (size_t i = 0; i < fallbackCandidates.size() && recommendations.size() < 20; ++i) {
                appendStone(fallbackCandidates[i],
                            "也许你会喜欢这个意外的发现", "exploration");
            }
        } catch (const std::exception &e2) {
            LOG_WARN << "随机探索降级查询也失败(跳过): " << e2.what();
        }
    }

    Json::Value responseData;
    responseData["recommendations"] = recommendations;
    responseData["total"] = static_cast<int>(recommendations.size());
    responseData["user_mood"] = userMood;

    // 缓存结果（5分钟）
    auto& cache = RedisCache::getInstance();
    std::string cacheKey = "recommendations:stones:" + userId;
    Json::StreamWriterBuilder builder;
    std::string jsonStr = Json::writeString(builder, responseData);
    cache.setEx(cacheKey, jsonStr, 300);

    callback(ResponseUtil::success(responseData, "为你精心挑选的内容"));
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
        double weight = 0.1; // view
        if (interactionType == "ripple") weight = 1.0;
        else if (interactionType == "boat") weight = 2.0;
        else if (interactionType == "share") weight = 3.0;

        auto dbClient = drogon::app().getDbClient("default");

        // 记录交互
        dbClient->execSqlAsync(
            "INSERT INTO user_interaction_history "
            "(user_id, stone_id, interaction_type, interaction_weight, dwell_time_seconds) "
            "VALUES ($1, $2, $3, $4, $5)",
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
        "INSERT INTO user_preferences (user_id, preferred_moods, preferred_tags, last_updated) "
        "SELECT "
        "  $1 as user_id, "
        "  ARRAY_AGG(DISTINCT s.mood_type) as preferred_moods, "
        "  ARRAY_AGG(DISTINCT tag) as preferred_tags, "
        "  NOW() as last_updated "
        "FROM user_interaction_history uih "
        "JOIN stones s ON uih.stone_id = s.stone_id "
        "LEFT JOIN LATERAL unnest(s.emotion_tags) as tag ON true "
        "WHERE uih.user_id = $1 "
        "AND uih.created_at >= NOW() - INTERVAL '30 days' "
        "AND uih.interaction_weight >= 1.0 "
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

    try {
        auto userId = req->getAttributes()->get<std::string>("user_id");
        auto dbClient = drogon::app().getDbClient("default");

        // 获取最近30天情绪数据（优先 emotion_score，缺失时按 mood_type 回退估算）
        auto trendsResult = dbClient->execSqlSync(
            "SELECT DATE(created_at) as date, "
            "COALESCE(mood_type, 'neutral') as mood_type, "
            "AVG(COALESCE(emotion_score, "
            "CASE COALESCE(mood_type, 'neutral') "
            "WHEN 'happy' THEN 0.75 "
            "WHEN 'calm' THEN 0.35 "
            "WHEN 'neutral' THEN 0.0 "
            "WHEN 'hopeful' THEN 0.55 "
            "WHEN 'grateful' THEN 0.65 "
            "WHEN 'sad' THEN -0.75 "
            "WHEN 'anxious' THEN -0.45 "
            "WHEN 'angry' THEN -0.65 "
            "WHEN 'lonely' THEN -0.55 "
            "WHEN 'confused' THEN -0.1 "
            "ELSE 0.0 END)) as avg_emotion_score, "
            "COUNT(*) as stone_count "
            "FROM stones "
            "WHERE user_id = $1 "
            "AND status = 'published' "
            "AND deleted_at IS NULL "
            "AND created_at >= CURRENT_DATE - INTERVAL '30 days' "
            "GROUP BY DATE(created_at), COALESCE(mood_type, 'neutral') "
            "ORDER BY date ASC",
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

        Json::Value responseData;
        responseData["trends"] = trends;
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

    try {
        auto userId = req->getAttributes()->get<std::string>("user_id");
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

        Json::Value responseData;
        responseData["mood"] = mood;
        responseData["stones"] = stones;
        responseData["total"] = static_cast<int>(stones.size());

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
    [[maybe_unused]] const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto cb = std::make_shared<std::function<void(const HttpResponsePtr &)>>(std::move(callback));
    try {
        // 检查缓存（热门内容缓存3分钟）
        auto& cache = RedisCache::getInstance();
        std::string cacheKey = "recommendations:trending";

        cache.get(cacheKey, [this, cb](const std::string& cachedData, bool exists) mutable {
            try {
                if (exists && !cachedData.empty()) {
                    Json::Value cached;
                    Json::Reader reader;
                    if (reader.parse(cachedData, cached)) {
                        (*cb)(ResponseUtil::success(cached, "当前热门（来自缓存）"));
                        return;
                    }
                }

                this->calculateTrendingContent([cb](const HttpResponsePtr &resp) { (*cb)(resp); });
            } catch (const std::exception &e) {
                LOG_ERROR << "Error in getTrendingContent Redis callback: " << e.what();
                (*cb)(ResponseUtil::internalError("获取热门内容失败"));
            } catch (...) {
                LOG_ERROR << "Unknown error in getTrendingContent Redis callback";
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
    std::function<void(const HttpResponsePtr &)> &&callback) {

    try {
        auto dbClient = drogon::app().getDbClient("default");

        // 获取最近24小时的热门石头
        auto trendingStonesResult = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "s.created_at, u.nickname AS author_name, "
            "COALESCE(s.ripple_count, 0) AS ripple_count "
            "FROM stones s "
            "JOIN users u ON s.user_id = u.user_id "
            "WHERE s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND s.created_at >= NOW() - INTERVAL '24 hours' "
            "ORDER BY COALESCE(s.ripple_count, 0) DESC, s.created_at DESC "
            "LIMIT 10"
        );

        Json::Value trendingStones(Json::arrayValue);
        for (size_t i = 0; i < trendingStonesResult.size(); ++i) {
            auto row = trendingStonesResult[i];
            Json::Value stone;
            stone["stone_id"] = row["stone_id"].as<std::string>();
            stone["content"] = row["content"].as<std::string>();
            stone["mood_type"] = row["mood_type"].as<std::string>();
            stone["emotion_score"] = row["emotion_score"].as<double>();
            stone["author_name"] = row["author_name"].as<std::string>();
            stone["ripple_count"] = row["ripple_count"].as<int>();
            stone["created_at"] = row["created_at"].as<std::string>();
            trendingStones.append(stone);
        }

        // 获取热门情绪
        auto trendingMoodsResult = dbClient->execSqlSync(
            "SELECT mood_type, COUNT(*) as count "
            "FROM stones "
            "WHERE status = 'published' "
            "AND deleted_at IS NULL "
            "AND created_at >= NOW() - INTERVAL '24 hours' "
            "GROUP BY mood_type "
            "ORDER BY count DESC "
            "LIMIT 5"
        );

        Json::Value trendingMoods(Json::arrayValue);
        for (size_t i = 0; i < trendingMoodsResult.size(); ++i) {
            auto row = trendingMoodsResult[i];
            Json::Value mood;
            mood["mood_type"] = row["mood_type"].as<std::string>();
            mood["count"] = row["count"].as<int>();
            trendingMoods.append(mood);
        }

        Json::Value responseData;
        responseData["trending_stones"] = trendingStones;
        responseData["trending_moods"] = trendingMoods;

        // 缓存结果（3分钟）
        auto& cache = RedisCache::getInstance();
        Json::StreamWriterBuilder builder;
        std::string jsonStr = Json::writeString(builder, responseData);
        cache.setEx("recommendations:trending", jsonStr, 180);

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

    try {
        auto userId = req->getAttributes()->get<std::string>("user_id");
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
            "COALESCE(s.boat_count, 0) AS boat_count "
            "FROM stones s "
            "JOIN users u ON s.user_id = u.user_id "
            "WHERE s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND s.user_id != $1 "
            "AND s.content ILIKE $2 ESCAPE '\\' ";

        // 转义LIKE通配符，防止用户输入的 % _ \ 被当作通配符
        auto escapeLike = [](const std::string& input) -> std::string {
            std::string result;
            result.reserve(input.size());
            for (char c : input) {
                if (c == '%' || c == '_' || c == '\\') {
                    result += '\\';
                }
                result += c;
            }
            return result;
        };
        std::string searchPattern = "%" + escapeLike(query) + "%";

        // 排序和分页（使用参数化查询防止注入）
        searchSql += "ORDER BY s.created_at DESC ";
        searchSql += "LIMIT $3 OFFSET $4";

        int offset = (page - 1) * pageSize;

        // 执行搜索
        auto searchResult = dbClient->execSqlSync(searchSql, userId, searchPattern, pageSize, offset);

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

        // 获取总数
        std::string countSql =
            "SELECT COUNT(*) as total FROM stones s "
            "WHERE s.status = 'published' "
            "AND s.deleted_at IS NULL "
            "AND s.user_id != $1 "
            "AND s.content ILIKE $2 ESCAPE '\\'";

        auto countResult = dbClient->execSqlSync(countSql, userId, searchPattern);
        int total = countResult.empty() ? 0 : countResult[0]["total"].as<int>();

        Json::Value responseData;
        responseData["results"] = results;
        responseData["total"] = total;
        responseData["page"] = page;
        responseData["page_size"] = pageSize;
        responseData["has_more"] = (page * pageSize) < total;

        callback(ResponseUtil::success(responseData, "搜索结果"));

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

    auto cb = std::make_shared<std::function<void(const HttpResponsePtr &)>>(std::move(callback));
    try {
        auto userId = req->getAttributes()->get<std::string>("user_id");
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
            double weight = 0.1; // view
            if (interactionType == "ripple") weight = 1.0;
            else if (interactionType == "boat") weight = 2.0;
            else if (interactionType == "share") weight = 3.0;
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
                    "VALUES ($1, $2, $3, $4, $5)",
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

    try {
        auto userId = req->getAttributes()->get<std::string>("user_id");
        if (userId.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        int limit = 20;
        auto params = req->getParameters();
        if (params.count("limit")) {
            limit = std::clamp(safeInt(params.at("limit"), 20), 1, 50);
        }

        // 如果提供了stone_id，融合情绪共鸣引擎结果
        std::string stoneId;
        if (params.count("stone_id")) {
            stoneId = params.at("stone_id");
        }

        auto& engine = heartlake::ai::RecommendationEngine::getInstance();
        engine.getRecommendations(userId, "stone", limit,
            [callback, userId, stoneId, limit](
                const std::vector<heartlake::ai::RecommendationCandidate>& results,
                const std::string& error
            ) {
                if (!error.empty()) {
                    callback(ResponseUtil::internalError("推荐服务暂不可用"));
                    return;
                }

                Json::Value recommendations(Json::arrayValue);
                std::unordered_set<std::string> addedIds;

                // 如果有stone_id，先添加情绪共鸣结果（权重更高）
                if (!stoneId.empty()) {
                    auto& resonanceEngine = heartlake::ai::EmotionResonanceEngine::getInstance();
                    auto resonanceResults = resonanceEngine.findResonance(userId, stoneId, limit / 2);

                    // 批量查询共鸣石头的基本信息
                    std::map<std::string, Json::Value> stoneInfoMap;
                    if (!resonanceResults.empty()) {
                        try {
                            auto dbClient = drogon::app().getDbClient("default");
                            for (const auto& res : resonanceResults) {
                                auto rows = dbClient->execSqlSync(
                                    "SELECT s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
                                    "COALESCE(s.emotion_score, 0.0) AS emotion_score, s.created_at, "
                                    "u.nickname as author_name, "
                                    "COALESCE(s.ripple_count, 0) as ripple_count "
                                    "FROM stones s JOIN users u ON s.user_id = u.user_id "
                                    "WHERE s.stone_id = $1 AND s.status = 'published' AND s.deleted_at IS NULL LIMIT 1", res.stoneId);
                                if (!rows.empty()) {
                                    Json::Value info;
                                    info["content"] = rows[0]["content"].as<std::string>();
                                    info["mood_type"] = rows[0]["mood_type"].as<std::string>();
                                    info["emotion_score"] = rows[0]["emotion_score"].as<double>();
                                    info["author_name"] = rows[0]["author_name"].as<std::string>();
                                    info["ripple_count"] = rows[0]["ripple_count"].as<int>();
                                    info["created_at"] = rows[0]["created_at"].as<std::string>();
                                    stoneInfoMap[res.stoneId] = info;
                                }
                            }
                        } catch (const std::exception& e) {
                            LOG_WARN << "查询共鸣石头信息失败: " << e.what();
                        }
                    }

                    for (const auto& res : resonanceResults) {
                        Json::Value item;
                        item["stone_id"] = res.stoneId;
                        item["score"] = res.totalScore;
                        item["reason"] = res.resonanceReason;
                        item["algorithm"] = "emotion_temporal_resonance";
                        item["semantic_score"] = res.semanticScore;
                        item["trajectory_score"] = res.trajectoryScore;
                        item["temporal_score"] = res.temporalScore;
                        item["diversity_score"] = res.diversityScore;
                        // 补充石头基本信息
                        auto it = stoneInfoMap.find(res.stoneId);
                        if (it != stoneInfoMap.end()) {
                            item["content"] = it->second["content"];
                            item["mood_type"] = it->second["mood_type"];
                            item["emotion_score"] = it->second["emotion_score"];
                            item["author_name"] = it->second["author_name"];
                            item["ripple_count"] = it->second["ripple_count"];
                            item["created_at"] = it->second["created_at"];
                        }
                        recommendations.append(item);
                        addedIds.insert(res.stoneId);
                    }
                }

                // 添加常规推荐结果（去重）
                for (const auto& cand : results) {
                    if (addedIds.count(cand.itemId)) continue;
                    if (static_cast<int>(recommendations.size()) >= limit) break;

                    Json::Value item;
                    item["stone_id"] = cand.itemId;
                    item["score"] = cand.score;
                    item["reason"] = cand.reason;
                    item["algorithm"] = cand.algorithm;
                    item["content"] = cand.metadata.get("content", "").asString();
                    item["mood_type"] = cand.metadata.get("mood_type", "").asString();
                    item["author_name"] = cand.metadata.get("author_name", "").asString();
                    item["ripple_count"] = cand.metadata.get("ripple_count", 0).asInt();
                    recommendations.append(item);
                }

                Json::Value data;
                data["recommendations"] = recommendations;
                data["count"] = static_cast<int>(recommendations.size());
                data["algorithm"] = stoneId.empty()
                    ? "multi_armed_bandit_mmr"
                    : "emotion_resonance_hybrid";

                callback(ResponseUtil::success(data, "为你精选的内容"));
            }
        );

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getAdvancedRecommendations: " << e.what();
        callback(ResponseUtil::internalError("获取推荐失败"));
    }
}
