/**
 * @file StoneApplicationService.cpp
 * @brief 石头应用服务 —— 石头发布、详情、列表、删除的完整业务编排
 *
 * 发布石头的异步处理管线（processStoneAsync）：
 *   1. 情感分析 → 更新 emotion_score/mood_type → 心理风险评估 → 暖心语录推送
 *   2. 向量嵌入 → 持久化到 stone_embeddings → 插入 HNSW 索引
 *   3. 长文本自动摘要（> MIN_LENGTH 才触发）
 *   4. DualMemoryRAG 生成 AI 暖心评论（湖神纸船）
 *
 * 浏览量采用 Redis 聚合策略：每次 INCR，累积 10 次后批量写回数据库。
 */

#include "application/StoneApplicationService.h"
#include "infrastructure/cache/RedisCache.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/DualMemoryRAG.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/ai/SummaryService.h"
#include "infrastructure/services/NotificationPushService.h"
#include "infrastructure/services/WarmQuoteService.h"
#include "infrastructure/services/VIPService.h"
#include "utils/PsychologicalRiskAssessment.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include "infrastructure/services/EmotionTrackingService.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include <drogon/drogon.h>

#ifdef _WIN32
#undef ERROR
#endif

using namespace heartlake::utils;

namespace heartlake {
namespace application {

/**
 * 发布石头：
 *   1. INSERT RETURNING 获取完整记录
 *   2. 发布 StonePublishedEvent 触发异步处理管线
 *   3. 本地情感分析 → 记录情绪追踪 → 负面情绪触发自动赠灯判定
 *   4. 失效列表缓存
 */
Json::Value StoneApplicationService::publishStone(
    const std::string& userId,
    const std::string& content,
    const std::string& stoneType,
    const std::string& stoneColor,
    const std::string& moodType,
    bool isAnonymous
) {
    auto dbClient = drogon::app().getDbClient("default");
    std::string stoneId = "stone_" + drogon::utils::getUuid();

    try {
        // 创建石头
        auto result = dbClient->execSqlSync(
            "INSERT INTO stones (stone_id, user_id, content, stone_type, stone_color, mood_type, is_anonymous, created_at) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, NOW()) "
            "RETURNING stone_id, user_id, content, stone_type, stone_color, mood_type, is_anonymous, created_at",
            stoneId, userId, content, stoneType, stoneColor, moodType, isAnonymous
        );

        if (result.empty()) {
            throw std::runtime_error("创建石头失败");
        }

        auto row = *safeRow(result);
        Json::Value stone;
        stone["stone_id"] = row["stone_id"].as<std::string>();
        stone["user_id"] = row["user_id"].as<std::string>();
        stone["content"] = row["content"].as<std::string>();
        stone["mood_type"] = row["mood_type"].as<std::string>();
        stone["is_anonymous"] = row["is_anonymous"].as<bool>();
        stone["created_at"] = row["created_at"].as<std::string>();

        // 发布事件 (异步处理情感分析等)
        if (eventBus_) {
            core::events::StonePublishedEvent event;
            event.stoneId = stone["stone_id"].asString();
            event.userId = userId;
            event.content = content;
            event.moodType = moodType;
            eventBus_->publish(event);
        }

        // 记录情绪到追踪服务（用于异常检测和干预）
        try {
            auto& emotionTracker = heartlake::infrastructure::EmotionTrackingService::getInstance();
            auto& aiEngine = heartlake::ai::EdgeAIEngine::getInstance();
            auto sentiment = aiEngine.analyzeSentimentLocal(content);
            emotionTracker.recordEmotion(userId, sentiment.score, content);

            // 情绪明显偏负时触发自动赠灯判定（VIPService 内部会做全局20%阈值与重复发放保护）
            if (sentiment.score < -0.1f) {
                heartlake::services::VIPService::checkEmotionAndGrantVIP(userId, sentiment.score, {});
            }
        } catch (const std::exception& e) {
            LOG_WARN << "EmotionTracking recordEmotion failed for user: " << userId << ": " << e.what();
        }

        // 清除缓存
        if (cacheManager_) {
            cacheManager_->invalidatePattern("stone_list:*");
        }

        return stone;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to publish stone: " << e.base().what();
        throw std::runtime_error("发布石头失败");
    }
}

/// 获取石头详情：cache-aside 策略，has_rippled 每次单独查询（用户相关不可缓存）
Json::Value StoneApplicationService::getStoneDetail(const std::string& stoneId, const std::string& currentUserId) {
    // 定义数据库查询函数
    auto fetchFromDb = [&stoneId]() -> Json::Value {
        auto dbClient = drogon::app().getDbClient("default");

        try {
            auto result = dbClient->execSqlSync(
                "SELECT s.stone_id, s.user_id, s.content, s.stone_type, s.stone_color, "
                "s.mood_type, s.is_anonymous, s.created_at, s.view_count, s.ripple_count, s.boat_count, "
                "u.username, u.nickname, u.avatar_url "
                "FROM stones s "
                "LEFT JOIN users u ON s.user_id = u.user_id "
                "WHERE s.stone_id = $1 AND s.deleted_at IS NULL",
                stoneId
            );

            if (result.empty()) {
                throw std::runtime_error("石头不存在");
            }

            auto row = *safeRow(result);
            Json::Value stone;
            // BUG-FIX: stone_id/user_id 等列可能为 NULL（旧数据），添加空值保护避免 500
            stone["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
            stone["user_id"] = row["user_id"].isNull() ? "" : row["user_id"].as<std::string>();
            stone["content"] = row["content"].as<std::string>();
            stone["stone_type"] = row["stone_type"].isNull() ? "medium" : row["stone_type"].as<std::string>();
            stone["stone_color"] = row["stone_color"].isNull() ? "#7A92A3" : row["stone_color"].as<std::string>();
            stone["mood_type"] = row["mood_type"].isNull() ? "calm" : row["mood_type"].as<std::string>();
            stone["is_anonymous"] = row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
            stone["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            stone["view_count"] = row["view_count"].isNull() ? 0 : row["view_count"].as<int>();
            stone["ripple_count"] = row["ripple_count"].isNull() ? 0 : row["ripple_count"].as<int>();
            stone["boat_count"] = row["boat_count"].isNull() ? 0 : row["boat_count"].as<int>();

            // 用户信息
            bool isAnon = row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
            if (!isAnon && !row["username"].isNull()) {
                Json::Value user;
                user["username"] = row["username"].isNull() ? "" : row["username"].as<std::string>();
                user["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
                if (!row["avatar_url"].isNull()) {
                    user["avatar_url"] = row["avatar_url"].as<std::string>();
                }
                stone["user"] = user;
            }

            return stone;

        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to get stone detail: " << e.base().what();
            throw std::runtime_error("获取石头详情失败");
        }
    };

    // 尝试从缓存获取
    std::string cacheKey = "stone:" + stoneId;
    Json::Value result;
    bool fromCache = false;
    if (cacheManager_) {
        auto cached = cacheManager_->getJson(cacheKey);
        if (cached) {
            result = *cached;
            fromCache = true;
        }
    }

    if (!fromCache) {
        // 从数据库获取
        result = fetchFromDb();

        // 缓存结果
        if (cacheManager_) {
            cacheManager_->setJson(cacheKey, result, 300);
        }
    }

    // has_rippled 是用户相关的，不能缓存，每次单独查询
    if (!currentUserId.empty()) {
        try {
            auto dbClient = drogon::app().getDbClient("default");
            auto rippleResult = dbClient->execSqlSync(
                "SELECT 1 FROM ripples WHERE stone_id = $1 AND user_id = $2 LIMIT 1",
                stoneId, currentUserId
            );
            result["has_rippled"] = !rippleResult.empty();
        } catch (const std::exception& e) {
            LOG_WARN << "Check has_rippled failed: " << e.what();
            result["has_rippled"] = false;
        }
    } else {
        result["has_rippled"] = false;
    }

    return result;
}

/// 石头列表：支持按用户/情绪过滤、多种排序方式、分页，附带 has_rippled 状态
Json::Value StoneApplicationService::getStoneList(
    int page,
    int pageSize,
    const std::string& sortBy,
    const std::string& filterMood,
    const std::string& userId,
    const std::string& currentUserId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;

        // 构建查询
        std::string sql =
            "SELECT s.stone_id, s.user_id, s.content, s.stone_type, s.stone_color, "
            "s.mood_type, s.is_anonymous, s.created_at, s.view_count, s.ripple_count, s.boat_count, "
            "u.username, u.nickname, u.avatar_url "
            "FROM stones s "
            "LEFT JOIN users u ON s.user_id = u.user_id "
            "WHERE s.deleted_at IS NULL ";

        std::vector<std::string> params;
        int paramIndex = 1;

        // 用户ID过滤
        if (!userId.empty()) {
            sql += "AND s.user_id = $" + std::to_string(paramIndex++) + " ";
            params.push_back(userId);
        }

        if (!filterMood.empty()) {
            sql += "AND s.mood_type = $" + std::to_string(paramIndex++) + " ";
            params.push_back(filterMood);
        }

        sql += "ORDER BY ";
        if (sortBy == "view_count") {
            sql += "s.view_count DESC ";
        } else if (sortBy == "ripple_count") {
            sql += "s.ripple_count DESC ";
        } else {
            sql += "s.created_at DESC ";
        }

        sql += "LIMIT " + std::to_string(pageSize) + " ";
        sql += "OFFSET " + std::to_string(offset);

        // 执行查询 - 根据参数组合选择正确的调用方式
        auto result = [&]() {
            if (!userId.empty() && !filterMood.empty()) {
                return dbClient->execSqlSync(sql, userId, filterMood);
            } else if (!userId.empty()) {
                return dbClient->execSqlSync(sql, userId);
            } else if (!filterMood.empty()) {
                return dbClient->execSqlSync(sql, filterMood);
            } else {
                return dbClient->execSqlSync(sql);
            }
        }();

        Json::Value stones(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value stone;
            // BUG-FIX: stone_id/user_id 等列可能为 NULL（旧数据），添加空值保护避免 500
            stone["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
            stone["user_id"] = row["user_id"].isNull() ? "" : row["user_id"].as<std::string>();
            stone["content"] = row["content"].as<std::string>();
            stone["stone_type"] = row["stone_type"].isNull() ? "medium" : row["stone_type"].as<std::string>();
            stone["stone_color"] = row["stone_color"].isNull() ? "#7A92A3" : row["stone_color"].as<std::string>();
            stone["mood_type"] = row["mood_type"].isNull() ? "calm" : row["mood_type"].as<std::string>();
            stone["is_anonymous"] = row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
            stone["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            stone["view_count"] = row["view_count"].isNull() ? 0 : row["view_count"].as<int>();
            stone["ripple_count"] = row["ripple_count"].isNull() ? 0 : row["ripple_count"].as<int>();
            stone["boat_count"] = row["boat_count"].isNull() ? 0 : row["boat_count"].as<int>();

            // 用户信息
            bool isAnon = row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
            if (!isAnon && !row["username"].isNull()) {
                Json::Value user;
                user["username"] = row["username"].isNull() ? "" : row["username"].as<std::string>();
                user["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
                if (!row["avatar_url"].isNull()) {
                    user["avatar_url"] = row["avatar_url"].as<std::string>();
                }
                stone["user"] = user;
            }

            // has_rippled: 当前用户是否已涟漪过该石头
            if (!currentUserId.empty()) {
                try {
                    auto rippleCheck = dbClient->execSqlSync(
                        "SELECT 1 FROM ripples WHERE stone_id = $1 AND user_id = $2 LIMIT 1",
                        row["stone_id"].as<std::string>(), currentUserId
                    );
                    stone["has_rippled"] = !rippleCheck.empty();
                } catch (const std::exception& e) {
                    LOG_WARN << "Check has_rippled in list failed: " << e.what();
                    stone["has_rippled"] = false;
                }
            } else {
                stone["has_rippled"] = false;
            }

            stones.append(stone);
        }

        // 获取总数
        std::string countSql = "SELECT COUNT(*) as total FROM stones WHERE deleted_at IS NULL ";
        if (!userId.empty()) {
            countSql += "AND user_id = $1 ";
        }
        if (!filterMood.empty()) {
            countSql += userId.empty() ? "AND mood_type = $1" : "AND mood_type = $2";
        }

        auto countResult = [&]() {
            if (!userId.empty() && !filterMood.empty()) {
                return dbClient->execSqlSync(countSql, userId, filterMood);
            } else if (!userId.empty()) {
                return dbClient->execSqlSync(countSql, userId);
            } else if (!filterMood.empty()) {
                return dbClient->execSqlSync(countSql, filterMood);
            } else {
                return dbClient->execSqlSync(countSql);
            }
        }();

        int total = countResult[0]["total"].as<int>();

        Json::Value response;
        response["stones"] = stones;
        response["total"] = total;
        response["page"] = page;
        response["page_size"] = pageSize;
        response["total_pages"] = (total + pageSize - 1) / pageSize;

        return response;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get stone list: " << e.base().what();
        throw std::runtime_error("获取石头列表失败");
    }
}

/// 软删除石头：校验所有权后标记 deleted_at，同时失效缓存
void StoneApplicationService::deleteStone(const std::string& stoneId, const std::string& userId) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 软删除
        auto result = dbClient->execSqlSync(
            "UPDATE stones SET deleted_at = NOW() "
            "WHERE stone_id = $1 AND user_id = $2 AND deleted_at IS NULL",
            stoneId, userId
        );

        if (result.affectedRows() == 0) {
            throw std::runtime_error("石头不存在或无权删除");
        }

        // 清除缓存
        if (cacheManager_) {
            cacheManager_->invalidate("stone:" + stoneId);
            cacheManager_->invalidatePattern("stone_list:*");
        }

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to delete stone: " << e.base().what();
        throw std::runtime_error("删除石头失败");
    }
}

/// 浏览量递增：Redis INCR 聚合，每累积 10 次批量写回数据库，降低 DB 写压力
void StoneApplicationService::incrementViewCount(const std::string& stoneId) {
    // 使用Redis聚合浏览量，带持久化保障
    auto& redis = cache::RedisCache::getInstance();
    std::string viewKey = "view_count:" + stoneId;

    if (!redis.isConnected()) {
        // Redis不可用时直接写数据库
        auto dbClient = drogon::app().getDbClient("default");
        dbClient->execSqlAsync(
            "UPDATE stones SET view_count = view_count + 1 WHERE stone_id = $1",
            [](const drogon::orm::Result&) {},
            [](const drogon::orm::DrogonDbException&) {},
            stoneId
        );
        return;
    }

    redis.incr(viewKey, [stoneId](int64_t count) {
        if (count > 0 && count % 10 == 0) {
            auto dbClient = drogon::app().getDbClient("default");
            dbClient->execSqlAsync(
                "UPDATE stones SET view_count = view_count + 10 WHERE stone_id = $1",
                [](const drogon::orm::Result&) {},
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "Failed to batch update view count: " << e.base().what();
                },
                stoneId
            );
        }
    });
}

/**
 * 石头发布后的异步处理管线（由事件驱动触发）：
 *   管线 1: 情感分析 → 更新 DB → 社区脉搏 → 心理风险评估 → 暖心语录
 *   管线 2: 向量嵌入 → 持久化 stone_embeddings → HNSW 索引插入
 *   管线 3: 长文本自动摘要
 *   管线 4: DualMemoryRAG 生成 AI 暖心评论 → WebSocket 广播
 */
void StoneApplicationService::processStoneAsync(
    const std::string& stoneId,
    const std::string& userId,
    const std::string& content,
    const std::string& moodType
) {
    auto& aiService = heartlake::ai::AIService::getInstance();

    // 1. 情感分析 + 风险评估 + 通知
    aiService.analyzeSentiment(content, [stoneId, userId, content](float score, const std::string& mood, const std::string& error) {
        if (!error.empty()) {
            LOG_WARN << "AI sentiment analysis failed for stone " << stoneId << ": " << error;
            return;
        }
        auto db = drogon::app().getDbClient("default");
        db->execSqlAsync(
            "UPDATE stones SET emotion_score = $1, mood_type = $2, updated_at = NOW() WHERE stone_id = $3",
            [](const drogon::orm::Result&) {},
            [stoneId](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "Failed to update emotion for stone " << stoneId << ": " << e.base().what();
            },
            score, mood, stoneId
        );

        // 提交情绪样本到社区脉搏
        try {
            auto& edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
            if (edgeEngine.isEnabled()) {
                edgeEngine.submitEmotionSample(score, mood);
            }
        } catch (const std::exception& e) {
            LOG_WARN << "submitEmotionSample failed: " << e.what();
        }

        // 心理风险评估
        auto& riskAssessment = heartlake::utils::PsychologicalRiskAssessment::getInstance();
        auto riskResult = riskAssessment.assessRisk(content, userId, score, mood);

        if (riskResult.needsImmediateAttention) {
            auto& pushService = heartlake::services::NotificationPushService::getInstance();
            pushService.pushSystemNotice(userId, "心理健康关怀", riskResult.supportMessage);

            if (riskResult.riskLevel == heartlake::utils::RiskLevel::CRITICAL) {
                db->execSqlAsync(
                    "SELECT admin_id FROM admins WHERE is_active = true AND role IN ('admin', 'moderator')",
                    [userId, stoneId](const drogon::orm::Result& r) {
                        auto& pushSvc = heartlake::services::NotificationPushService::getInstance();
                        for (const auto& row : r) {
                            std::string adminId = row["admin_id"].as<std::string>();
                            pushSvc.pushSystemNotice(adminId, "⚠️ 危机预警",
                                "用户 " + userId + " 发布的石头（ID: " + stoneId + "）检测到危机级别心理风险，请及时关注。");
                        }
                    },
                    [](const drogon::orm::DrogonDbException& e) {
                        LOG_ERROR << "Failed to notify administrators: " << e.base().what();
                    }
                );
            }
        }

        // 暖心语录
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm tm_buf;
        localtime_r(&time_t_now, &tm_buf);
        int hour = tm_buf.tm_hour;
        auto scene = heartlake::infrastructure::WarmQuoteService::detectScene(hour, false, false, 0, 1, 0, score);
        if (scene != heartlake::infrastructure::WarmScene::Default) {
            auto msg = heartlake::infrastructure::WarmQuoteService::getInstance().getQuoteForScene(scene);
            heartlake::services::NotificationPushService::getInstance().pushSystemNotice(userId, msg.title, msg.content);
        }
    });

    // 2. 向量嵌入
    aiService.generateEmbedding(content, [stoneId](const std::vector<float>& embedding, const std::string& error) {
        if (!error.empty() || embedding.empty()) {
            LOG_WARN << "Embedding generation failed for stone " << stoneId << ": " << error;
            return;
        }
        std::string vecStr = "[";
        for (size_t i = 0; i < embedding.size(); ++i) {
            vecStr += std::to_string(embedding[i]);
            if (i < embedding.size() - 1) vecStr += ",";
        }
        vecStr += "]";

        drogon::app().getDbClient("default")->execSqlAsync(
            // stone_embeddings 才是向量持久化表，避免误写 stones 导致连接进入 abort pipeline。
            "INSERT INTO stone_embeddings (stone_id, embedding, created_at) "
            "VALUES ($1, $2, NOW()) "
            "ON CONFLICT (stone_id) DO UPDATE SET embedding = EXCLUDED.embedding",
            [](const drogon::orm::Result&) {},
            [stoneId](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "Failed to persist embedding for stone " << stoneId << ": " << e.base().what();
            },
            stoneId, vecStr
        );

        // 同步插入HNSW索引，使向量搜索可用
        try {
            auto& edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
            if (edgeEngine.isEnabled()) {
                edgeEngine.hnswInsert(stoneId, embedding);
            }
        } catch (const std::exception& e) {
            LOG_WARN << "HNSW insert failed for stone " << stoneId << ": " << e.what();
        }
    });

    // 3. 长文本自动摘要
    if (content.size() >= heartlake::ai::SummaryService::MIN_LENGTH) {
        heartlake::ai::SummaryService::getInstance().generateSummary(
            stoneId, content,
            [stoneId](const std::string& summary, const std::string& error) {
                if (!error.empty()) {
                    LOG_WARN << "Summary generation failed for stone " << stoneId << ": " << error;
                    return;
                }
                drogon::app().getDbClient("default")->execSqlAsync(
                    "UPDATE stones SET summary = $1 WHERE stone_id = $2",
                    [](const drogon::orm::Result&) {},
                    [stoneId](const drogon::orm::DrogonDbException& e) {
                        LOG_ERROR << "Failed to update summary for stone " << stoneId << ": " << e.base().what();
                    },
                    summary, stoneId
                );
            }
        );
    }

    // 4. AI暖心评论（基于双记忆RAG）
    // 使用DualMemoryRAG生成个性化回复，融合用户长期情绪画像和近期交互上下文
    auto cacheManagerCopy = cacheManager_;
    drogon::async_run([stoneId, userId, content, moodType, cacheManagerCopy]() -> drogon::Task<void> {
        try {
            auto& dualMemory = heartlake::ai::DualMemoryRAG::getInstance();
            std::string comment = dualMemory.generateResponse(userId, content, moodType, 0.0f);

            if (comment.empty()) {
                LOG_WARN << "DualMemoryRAG returned empty for stone " << stoneId;
                co_return;
            }

            std::string aiBoatId = heartlake::utils::IdGenerator::generateBoatId();
            auto db = drogon::app().getDbClient("default");
            auto transPtr = db->newTransaction();
            transPtr->execSqlSync(
                "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, is_anonymous, status, created_at) "
                "VALUES ($1, $2, 'ai_lakegod', $3, false, 'active', NOW())",
                aiBoatId, stoneId, comment
            );
            auto updateResult = transPtr->execSqlSync(
                "UPDATE stones SET boat_count = boat_count + 1, updated_at = NOW() WHERE stone_id = $1 RETURNING boat_count",
                stoneId
            );
            int newBoatsCount = updateResult.empty() ? 1 : updateResult[0]["boat_count"].as<int>();

            if (cacheManagerCopy) {
                cacheManagerCopy->invalidate("stone:" + stoneId);
            }

            Json::Value broadcastMsg;
            broadcastMsg["type"] = "boat_update";
            broadcastMsg["stone_id"] = stoneId;
            broadcastMsg["boat_count"] = newBoatsCount;
            broadcastMsg["boat_content"] = comment;
            broadcastMsg["boat_id"] = aiBoatId;
            broadcastMsg["is_ai"] = true;
            broadcastMsg["triggered_by"] = "ai_lakegod_rag";
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));

            drogon::app().getLoop()->queueInLoop([stoneId, msg = std::move(broadcastMsg)]() mutable {
                heartlake::controllers::BroadcastWebSocketController::sendToRoom("stone:" + stoneId, msg);
            });
        } catch (const std::exception& e) {
            LOG_ERROR << "DualMemoryRAG comment creation failed: " << e.what();
        }
        co_return;
    });
}

} // namespace application
} // namespace heartlake
