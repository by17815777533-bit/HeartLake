/**
 * @file PaperBoatController.cpp
 * @brief PaperBoatController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/PaperBoatController.h"
#include "utils/ResponseUtil.h"
#include "utils/IdGenerator.h"
#include "utils/ContentFilter.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/PsychologicalRiskAssessment.h"
#include "infrastructure/services/NotificationPushService.h"
#include "infrastructure/services/GuardianIncentiveService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include <algorithm>
#include <set>

using namespace heartlake::controllers;
using namespace heartlake::utils;

namespace {

float estimateWarmthScore(const std::string& content) {
    auto& edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
    auto sentiment = edgeEngine.analyzeSentimentLocal(content);
    const float normalized = std::clamp((sentiment.score + 1.0f) / 2.0f, 0.2f, 1.0f);
    return normalized;
}

void rewardWarmBoat(const std::string& userId, const std::string& boatId, const std::string& content) {
    try {
        heartlake::infrastructure::GuardianIncentiveService::getInstance().recordWarmBoat(
            userId,
            boatId,
            estimateWarmthScore(content)
        );
    } catch (const std::exception& e) {
        LOG_WARN << "rewardWarmBoat failed: " << e.what();
    }
}

} // namespace


void PaperBoatController::replyToStone(const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }

        std::string stone_id = (*json).get("stone_id", "").asString();
        std::string content = (*json).get("content", "").asString();
        std::string mood = (*json).get("mood", "").asString();

        if (stone_id.empty() || content.empty()) {
            callback(ResponseUtil::badRequest("stone_id 和 content 不能为空"));
            return;
        }

        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        auto userIdOpt = Validator::getUserId(req);
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }
        auto& user_id = *userIdOpt;

        // 内容安全检查
        std::string safety_level = ContentFilter::checkContentSafety(content);
        if (safety_level == "high_risk") {
            Json::Value warning;
            warning["message"] = ContentFilter::getMentalHealthTip();
            callback(ResponseUtil::error(403, "检测到高危内容，请寻求专业帮助", warning));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");

        // 验证石头存在
        auto stoneResult = dbClient->execSqlSync(
            "SELECT user_id FROM stones WHERE stone_id = $1 AND status = 'published'",
            stone_id
        );

        if (stoneResult.empty()) {
            callback(ResponseUtil::notFound("石头不存在"));
            return;
        }

        std::string stone_owner_id = stoneResult[0]["user_id"].as<std::string>();
        std::string boat_id = IdGenerator::generateBoatId();

        // 使用事务确保原子性操作
        int newBoatsCount = 0;
        {
            auto transPtr = dbClient->newTransaction();

            // 插入纸船（关联石头）
            transPtr->execSqlSync(
                "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, "
                "is_anonymous, status, created_at) "
                "VALUES ($1, $2, $3, $4, true, 'active', NOW())",
                boat_id, stone_id, user_id, content
            );

            // 更新石头的纸船计数（原子操作）
            auto updateResult = transPtr->execSqlSync(
                "UPDATE stones SET boat_count = boat_count + 1, updated_at = NOW() "
                "WHERE stone_id = $1 RETURNING boat_count",
                stone_id
            );

            newBoatsCount = updateResult.empty() ? 0 : updateResult[0]["boat_count"].as<int>();

            // 事务在作用域结束时自动提交
        }

        // 广播 boat_update 事件到 stone 房间，让查看该石头的用户实时更新纸船计数
        {
            Json::Value broadcastMsg;
            broadcastMsg["type"] = "boat_update";
            broadcastMsg["stone_id"] = stone_id;
            broadcastMsg["boat_id"] = boat_id;
            broadcastMsg["boat_count"] = newBoatsCount;
            broadcastMsg["triggered_by"] = user_id;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToRoom("stone:" + stone_id, broadcastMsg);
        }

        // 通知石头主人
        if (stone_owner_id != user_id) {
            std::string notif_id = IdGenerator::generateNotificationId();
            dbClient->execSqlAsync(
                "INSERT INTO notifications (notification_id, user_id, type, content, related_id, related_type, is_read, created_at) "
                "VALUES ($1, $2, 'boat', '有人给你的石头回了一封纸船', $3, 'boat', false, NOW())",
                [stone_owner_id](const drogon::orm::Result &) {
                    Json::Value notifMsg;
                    notifMsg["type"] = "new_notification";
                    notifMsg["notification_type"] = "boat";
                    notifMsg["content"] = "有人给你的石头回了一封纸船";
                    BroadcastWebSocketController::sendToUser(stone_owner_id, notifMsg);
                },
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Failed to create notification: " << e.base().what();
                },
                notif_id, stone_owner_id, boat_id
            );
        }

        // 异步进行心理风险评估
        auto& aiService = heartlake::ai::AIService::getInstance();
        aiService.analyzeSentiment(content, [boat_id, user_id, content](float score, const std::string& detectedMood, const std::string& error) {
            if (!error.empty()) {
                return;
            }

            auto& riskAssessment = heartlake::utils::PsychologicalRiskAssessment::getInstance();
            auto riskResult = riskAssessment.assessRisk(content, user_id, score, detectedMood);

            if (riskResult.needsImmediateAttention) {
                auto& pushService = heartlake::services::NotificationPushService::getInstance();
                pushService.pushSystemNotice(user_id, "心理健康关怀", riskResult.supportMessage);

                if (riskResult.riskLevel == heartlake::utils::RiskLevel::CRITICAL) {
                    auto db = drogon::app().getDbClient("default");
                    db->execSqlAsync(
                        "SELECT admin_id FROM admins WHERE is_active = true AND role IN ('admin', 'moderator')",
                        [user_id, boat_id](const drogon::orm::Result& r) {
                            auto& pushSvc = heartlake::services::NotificationPushService::getInstance();
                            for (const auto& row : r) {
                                std::string adminId = row["admin_id"].as<std::string>();
                                std::string message = "用户 " + user_id + " 回复的纸船（ID: " + boat_id +
                                                    "）检测到危机级别心理风险，请及时关注。";
                                pushSvc.pushSystemNotice(adminId, "⚠️ 危机预警", message);
                            }
                        },
                        [](const drogon::orm::DrogonDbException& e) {
                            LOG_ERROR << "Failed to notify administrators: " << e.base().what();
                        }
                    );
                }
            }
        });

        Json::Value data;
        data["boat_id"] = boat_id;
        data["stone_id"] = stone_id;
        data["boat_count"] = newBoatsCount;

        rewardWarmBoat(user_id, boat_id, content);

        // 广播 boat_update 事件，让所有查看该石头的用户实时看到新评论
        {
            Json::Value broadcastMsg;
            broadcastMsg["type"] = "boat_update";
            broadcastMsg["stone_id"] = stone_id;
            broadcastMsg["boat_count"] = newBoatsCount;
            broadcastMsg["boat_id"] = boat_id;
            broadcastMsg["triggered_by"] = user_id;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToRoom("stone:" + stone_id, broadcastMsg);
        }

        // 自动建立临时好友关系（纸船互动触发，24小时有效）
        if (user_id != stone_owner_id) {
            auto tempFriendId = drogon::utils::getUuid();
            auto expiresAt = trantor::Date::date().after(24 * 3600);
            // 确保 user1_id < user2_id 以满足唯一约束的一致性
            std::string uid1 = std::min(user_id, stone_owner_id);
            std::string uid2 = std::max(user_id, stone_owner_id);
            dbClient->execSqlAsync(
                "INSERT INTO temp_friends (temp_friend_id, user1_id, user2_id, source, source_id, status, expires_at) "
                "VALUES ($1, $2, $3, 'boat', $4, 'active', $5) "
                "ON CONFLICT ON CONSTRAINT unique_temp_friendship DO UPDATE SET "
                "expires_at = GREATEST(temp_friends.expires_at, EXCLUDED.expires_at), "
                "status = 'active'",
                [](const drogon::orm::Result &) {
                    LOG_DEBUG << "Temp friendship created/refreshed via boat reply to stone";
                },
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Failed to create temp friendship: " << e.base().what();
                },
                tempFriendId, uid1, uid2, boat_id, expiresAt.toDbStringLocal()
            );
        }

        callback(ResponseUtil::success(data, "纸船已放置"));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in replyToStone: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in replyToStone: " << e.what();
        callback(ResponseUtil::internalError());
    }
}


void PaperBoatController::getBoatDetail(const HttpRequestPtr &/*req*/,
                                        std::function<void(const HttpResponsePtr &)> &&callback,
                                        const std::string &boatId) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT b.boat_id, b.stone_id, b.content, b.boat_style AS boat_color, b.is_anonymous, "
            "b.is_ai_reply, b.status, "
            "EXTRACT(EPOCH FROM b.created_at) as created_at_ts, "
            "u.nickname "
            "FROM paper_boats b "
            "LEFT JOIN users u ON b.sender_id = u.user_id "
            "WHERE b.boat_id = $1",
            boatId
        );

        if (result.empty()) {
            callback(ResponseUtil::notFound("纸船不存在"));
            return;
        }

        auto row = *safeRow(result);

        Json::Value boat;
        boat["boat_id"] = row["boat_id"].as<std::string>();
        boat["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
        boat["content"] = row["content"].as<std::string>();
        boat["boat_color"] = row["boat_color"].isNull() ? "#F5EFE7" : row["boat_color"].as<std::string>();
        boat["is_anonymous"] = row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
        boat["is_ai_reply"] = row["is_ai_reply"].isNull() ? false : row["is_ai_reply"].as<bool>();
        boat["status"] = row["status"].as<std::string>();
        boat["created_at"] = static_cast<Json::Int64>(row["created_at_ts"].as<double>());

        Json::Value author;
        author["nickname"] = row["nickname"].isNull() ? "匿名旅人" : row["nickname"].as<std::string>();
        author["is_anonymous"] = row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
        boat["author"] = author;

        callback(ResponseUtil::success(boat));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in getBoatDetail: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getBoatDetail: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void PaperBoatController::getMySentBoats(const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        auto userIdOpt2 = Validator::getUserId(req);
        if (!userIdOpt2) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }
        auto& user_id = *userIdOpt2;

        auto [page, page_size] = safePagination(req);
        std::string status_filter = req->getParameter("status");

        auto dbClient = drogon::app().getDbClient("default");

        // 构建查询条件 - 使用白名单 + 参数化查询防止SQL注入
        static const std::set<std::string> validStatuses = {"published", "draft", "deleted"};
        bool hasStatusFilter = !status_filter.empty() && status_filter != "all" && validStatuses.count(status_filter);

        int offset = (page - 1) * page_size;

        auto queryBoats = [&]() -> std::pair<int, drogon::orm::Result> {
            if (hasStatusFilter) {
                auto countResult = dbClient->execSqlSync(
                    "SELECT COUNT(*) as total FROM paper_boats b WHERE b.sender_id = $1 AND b.status = $2",
                    user_id, status_filter);
                int t = safeCount(countResult);
                auto r = dbClient->execSqlSync(
                    "SELECT b.boat_id, b.stone_id, b.content, b.boat_style AS boat_color, b.status, "
                    "EXTRACT(EPOCH FROM b.created_at) as created_at_ts "
                    "FROM paper_boats b "
                    "WHERE b.sender_id = $1 AND b.status = $2 "
                    "ORDER BY b.created_at DESC "
                    "LIMIT $3 OFFSET $4",
                    user_id, status_filter, std::to_string(page_size), std::to_string(offset));
                return {t, std::move(r)};
            } else {
                auto countResult = dbClient->execSqlSync(
                    "SELECT COUNT(*) as total FROM paper_boats b WHERE b.sender_id = $1",
                    user_id);
                int t = safeCount(countResult);
                auto r = dbClient->execSqlSync(
                    "SELECT b.boat_id, b.stone_id, b.content, b.boat_style AS boat_color, b.status, "
                    "EXTRACT(EPOCH FROM b.created_at) as created_at_ts "
                    "FROM paper_boats b "
                    "WHERE b.sender_id = $1 "
                    "ORDER BY b.created_at DESC "
                    "LIMIT $2 OFFSET $3",
                    user_id, std::to_string(page_size), std::to_string(offset));
                return {t, std::move(r)};
            }
        };
        auto [total, result] = queryBoats();

        Json::Value boats(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value boat;
            boat["boat_id"] = row["boat_id"].as<std::string>();
            boat["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
            boat["content"] = row["content"].as<std::string>();
            boat["boat_color"] = row["boat_color"].isNull() ? "#F5EFE7" : row["boat_color"].as<std::string>();
            boat["status"] = row["status"].as<std::string>();
            boat["is_ai_reply"] = false;
            boat["created_at"] = static_cast<Json::Int64>(row["created_at_ts"].as<double>());
            boats.append(boat);
        }

        callback(ResponseUtil::paged(boats, total, page, page_size));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in getMySentBoats: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getMySentBoats: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void PaperBoatController::getMyReceivedBoats(const HttpRequestPtr &req,
                                             std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        auto userIdOpt3 = Validator::getUserId(req);
        if (!userIdOpt3) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }
        auto& user_id = *userIdOpt3;

        auto [page, page_size] = safePagination(req);

        auto dbClient = drogon::app().getDbClient("default");

        // 获取关联到该用户石头的纸船
        auto countResult = dbClient->execSqlSync(
            "SELECT COUNT(*) as total FROM paper_boats b "
            "JOIN stones s ON b.stone_id = s.stone_id "
            "WHERE s.user_id = $1 AND b.sender_id != $1",
            user_id
        );
        int total = safeCount(countResult);

        int offset = (page - 1) * page_size;
        auto result = dbClient->execSqlSync(
            "SELECT b.boat_id, b.stone_id, b.content, b.boat_style AS boat_color, b.status, "
            "EXTRACT(EPOCH FROM b.created_at) as created_at_ts, "
            "s.content as stone_content, "
            "u.nickname as sender_nickname "
            "FROM paper_boats b "
            "JOIN stones s ON b.stone_id = s.stone_id "
            "LEFT JOIN users u ON b.sender_id = u.user_id "
            "WHERE s.user_id = $1 AND b.sender_id != $1 "
            "ORDER BY b.created_at DESC "
            "LIMIT $2 OFFSET $3",
            user_id, std::to_string(page_size), std::to_string(offset));

        Json::Value boats(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value boat;
            boat["boat_id"] = row["boat_id"].as<std::string>();
            boat["stone_id"] = row["stone_id"].as<std::string>();
            boat["content"] = row["content"].as<std::string>();
            boat["boat_color"] = row["boat_color"].isNull() ? "#F5EFE7" : row["boat_color"].as<std::string>();
            boat["status"] = row["status"].as<std::string>();
            boat["is_ai_reply"] = false;
            boat["created_at"] = static_cast<Json::Int64>(row["created_at_ts"].as<double>());
            boat["stone_content"] = row["stone_content"].as<std::string>();

            Json::Value sender;
            sender["nickname"] = row["sender_nickname"].isNull() ? "匿名旅人" : row["sender_nickname"].as<std::string>();
            sender["is_anonymous"] = true;
            boat["sender"] = sender;

            boats.append(boat);
        }

        callback(ResponseUtil::paged(boats, total, page, page_size));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in getMyReceivedBoats: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getMyReceivedBoats: " << e.what();
        callback(ResponseUtil::internalError());
    }
}
