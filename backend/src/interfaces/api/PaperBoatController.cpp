/**
 * @file PaperBoatController.cpp
 * @brief PaperBoatController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/PaperBoatController.h"
#include "utils/ResponseUtil.h"
#include "utils/IdGenerator.h"
#include "utils/ContentFilter.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/PsychologicalRiskAssessment.h"
#include "infrastructure/services/NotificationPushService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/services/ResonanceSearchService.h"
#include <set>

using namespace heartlake::controllers;
using namespace heartlake::utils;


void PaperBoatController::sendBoat(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }

        if (!json->isMember("content") || (*json)["content"].asString().empty()) {
            callback(ResponseUtil::badRequest("content 不能为空"));
            return;
        }

        std::string content = (*json)["content"].asString();

        // SEC: 内容长度限制，防止超大 payload 攻击
        if (content.size() > 2000) {
            callback(ResponseUtil::badRequest("content 长度不能超过 2000 字符"));
            return;
        }
        std::string mood = (*json).get("mood", "hopeful").asString();
        std::string drift_mode = (*json).get("drift_mode", "random").asString();
        std::string receiver_id = (*json).get("receiver_id", "").asString();
        std::string boat_style = (*json).get("boat_style", "paper").asString();

        // SEC: 白名单校验枚举参数，防止非法值写入数据库
        static const std::set<std::string> validMoods = {"hopeful", "sad", "calm", "anxious", "happy", "lonely", "grateful", ""};
        static const std::set<std::string> validDriftModes = {"random", "directed", "wish"};
        static const std::set<std::string> validBoatStyles = {"paper", "origami", "lotus"};
        if (!validMoods.count(mood)) {
            callback(ResponseUtil::badRequest("无效的 mood 值"));
            return;
        }
        if (!validDriftModes.count(drift_mode)) {
            callback(ResponseUtil::badRequest("无效的 drift_mode 值"));
            return;
        }
        if (!validBoatStyles.count(boat_style)) {
            callback(ResponseUtil::badRequest("无效的 boat_style 值"));
            return;
        }

        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        std::string user_id;
        try {
            user_id = req->getAttributes()->get<std::string>("user_id");
        } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 内容安全检查
        std::string safety_level = ContentFilter::checkContentSafety(content);
        if (safety_level == "high_risk") {
            Json::Value warning;
            warning["message"] = ContentFilter::getMentalHealthTip();
            callback(ResponseUtil::error(403, "检测到高危内容，请寻求专业帮助", warning));
            return;
        }

        // 定向漂流需要指定接收者
        if (drift_mode == "directed" && receiver_id.empty()) {
            callback(ResponseUtil::badRequest("定向漂流需要指定接收者"));
            return;
        }

        std::string boat_id = IdGenerator::generateBoatId();
        int drift_delay = calculateDriftDelay();

        auto dbClient = drogon::app().getDbClient("default");

        // BUG-3 修复：定向漂流的检查和插入放入同一个事务，保证原子性
        if (drift_mode == "directed" && !receiver_id.empty()) {
            auto transPtr = dbClient->newTransaction();
            try {
                // 在事务内检查临时好友关系有效性
                auto checkResult = transPtr->execSqlSync(
                    "SELECT temp_friend_id FROM temp_friends "
                    "WHERE ((user1_id = $1 AND user2_id = $2) OR (user1_id = $2 AND user2_id = $1)) "
                    "AND status = 'active' AND expires_at > NOW() "
                    "FOR UPDATE",
                    user_id, receiver_id
                );
                if (checkResult.empty()) {
                    callback(ResponseUtil::error(400, "临时好友关系已过期或不存在"));
                    return;
                }
                // 在同一事务内插入定向纸船
                transPtr->execSqlSync(
                    "INSERT INTO paper_boats (boat_id, sender_id, receiver_id, content, "
                    "mood, boat_style, is_anonymous, status, created_at) "
                    "VALUES ($1, $2, $3, $4, $5, $6, true, 'delivered', NOW())",
                    boat_id, user_id, receiver_id, content, mood, boat_style
                );
            } catch (const std::exception &e) {
                LOG_ERROR << "定向漂流事务失败: " << e.what();
                callback(ResponseUtil::internalError("发送定向纸船失败"));
                return;
            }
        } else {
            // BUG-3 修复：随机漂流时 stone_id 用 NULL 替代空字符串，避免外键约束错误
            dbClient->execSqlSync(
                "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, "
                "mood, boat_style, is_anonymous, status, created_at) "
                "VALUES ($1, NULL, $2, $3, $4, $5, true, 'drifting', NOW())",
                boat_id, user_id, content, mood, boat_style
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
                            auto& pushService = heartlake::services::NotificationPushService::getInstance();
                            for (const auto& row : r) {
                                std::string adminId = row["admin_id"].as<std::string>();
                                std::string message = "用户 " + user_id + " 发送的纸船（ID: " + boat_id +
                                                    "）检测到危机级别心理风险，请及时关注。";
                                pushService.pushSystemNotice(adminId, "⚠️ 危机预警", message);
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
        data["drift_mode"] = drift_mode;
        data["drift_delay_seconds"] = drift_delay;
        data["status"] = "drifting";
        data["created_at"] = static_cast<Json::Int64>(time(nullptr));

        // 广播 new_boat 事件，让前端实时收到新纸船通知
        {
            Json::Value broadcastMsg;
            broadcastMsg["type"] = "new_boat";
            broadcastMsg["boat_id"] = boat_id;
            broadcastMsg["drift_mode"] = drift_mode;
            broadcastMsg["sender_id"] = user_id;
            broadcastMsg["mood"] = mood;
            broadcastMsg["boat_style"] = boat_style;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::broadcast(broadcastMsg);
        }

        // 定向漂流时，额外通知接收者
        if (drift_mode == "directed" && !receiver_id.empty()) {
            Json::Value directMsg;
            directMsg["type"] = "new_boat";
            directMsg["boat_id"] = boat_id;
            directMsg["drift_mode"] = "directed";
            directMsg["sender_id"] = user_id;
            directMsg["content_preview"] = content.size() > 20 ? content.substr(0, 20) + "..." : content;
            directMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToUser(receiver_id, directMsg);
        }

        callback(ResponseUtil::success(data, "纸船已放入水中"));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in sendBoat: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in sendBoat: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

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
        std::string user_id;
        try {
            user_id = req->getAttributes()->get<std::string>("user_id");
        } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

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
                            auto& pushService = heartlake::services::NotificationPushService::getInstance();
                            for (const auto& row : r) {
                                std::string adminId = row["admin_id"].as<std::string>();
                                std::string message = "用户 " + user_id + " 回复的纸船（ID: " + boat_id +
                                                    "）检测到危机级别心理风险，请及时关注。";
                                pushService.pushSystemNotice(adminId, "⚠️ 危机预警", message);
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

        // 广播 boat_update 事件，让所有查看该石头的用户实时看到新评论
        {
            Json::Value broadcastMsg;
            broadcastMsg["type"] = "boat_update";
            broadcastMsg["stone_id"] = stone_id;
            broadcastMsg["boat_count"] = newBoatsCount;
            broadcastMsg["boat_id"] = boat_id;
            broadcastMsg["triggered_by"] = user_id;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::broadcast(broadcastMsg);
        }

        // 自动建立临时好友关系（纸船互动触发，24小时有效）
        if (user_id != stone_owner_id) {
            auto tempFriendId = drogon::utils::getUuid();
            auto expiresAt = trantor::Date::date().after(24 * 3600);
            // 确保 user_id_1 < user_id_2 以满足唯一约束的一致性
            std::string uid1 = std::min(user_id, stone_owner_id);
            std::string uid2 = std::max(user_id, stone_owner_id);
            dbClient->execSqlAsync(
                "INSERT INTO temp_friends (temp_friend_id, user_id_1, user_id_2, source, source_id, status, expires_at) "
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


void PaperBoatController::catchBoat(const HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        std::string user_id;
        try {
            user_id = req->getAttributes()->get<std::string>("user_id");
        } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");

        // 原子操作：随机选取并更新纸船状态，避免竞态条件
        auto result = dbClient->execSqlSync(
            "UPDATE paper_boats SET status = 'caught', catcher_id = $1, updated_at = NOW() "
            "WHERE boat_id = (SELECT b.boat_id FROM paper_boats b "
            "WHERE b.status = 'drifting' AND b.sender_id != $1 "
            "ORDER BY RANDOM() LIMIT 1 FOR UPDATE SKIP LOCKED) "
            "RETURNING boat_id, content, boat_color, is_ai_reply, "
            "EXTRACT(EPOCH FROM created_at) as created_at_ts, sender_id",
            user_id
        );

        if (result.empty()) {
            callback(ResponseUtil::success(Json::Value(), "水面平静，没有漂流的纸船"));
            return;
        }

        auto row = result[0];
        std::string sender_id = row["sender_id"].as<std::string>();

        // 获取发送者昵称
        auto userResult = dbClient->execSqlSync(
            "SELECT nickname FROM users WHERE user_id = $1", sender_id
        );

        Json::Value boat;
        boat["boat_id"] = row["boat_id"].as<std::string>();
        boat["content"] = row["content"].as<std::string>();
        boat["boat_color"] = row["boat_color"].isNull() ? "#F5EFE7" : row["boat_color"].as<std::string>();
        boat["is_ai_reply"] = row["is_ai_reply"].isNull() ? false : row["is_ai_reply"].as<bool>();
        boat["created_at"] = static_cast<Json::Int64>(row["created_at_ts"].as<double>());

        Json::Value author;
        author["nickname"] = userResult.empty() || userResult[0]["nickname"].isNull() ? "匿名旅人" : userResult[0]["nickname"].as<std::string>();
        author["is_anonymous"] = true;
        boat["author"] = author;

        Json::Value data;
        data["boat"] = boat;

        callback(ResponseUtil::success(data, "捞到一艘纸船"));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in catchBoat: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in catchBoat: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void PaperBoatController::respondToBoat(const HttpRequestPtr &req,
                                        std::function<void(const HttpResponsePtr &)> &&callback,
                                        const std::string &boatId) {
    try {
        auto json = req->getJsonObject();
        if (!json || !json->isMember("content") || (*json)["content"].asString().empty()) {
            callback(ResponseUtil::badRequest("content 不能为空"));
            return;
        }

        std::string content = (*json)["content"].asString();
        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        std::string user_id;
        try {
            user_id = req->getAttributes()->get<std::string>("user_id");
        } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 内容安全检查
        std::string safety_level = ContentFilter::checkContentSafety(content);
        if (safety_level == "high_risk") {
            callback(ResponseUtil::error(403, "检测到高危内容"));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");

        // 获取原纸船信息
        auto boatResult = dbClient->execSqlSync(
            "SELECT sender_id, status FROM paper_boats WHERE boat_id = $1",
            boatId
        );

        if (boatResult.empty()) {
            callback(ResponseUtil::notFound("纸船不存在"));
            return;
        }

        std::string boat_owner_id = boatResult[0]["sender_id"].as<std::string>();
        std::string boat_status = boatResult[0]["status"].as<std::string>();

        if (boat_status != "caught" && boat_status != "drifting") {
            callback(ResponseUtil::error(400, "纸船状态不允许回应"));
            return;
        }

        // 更新原纸船状态为已回应
        dbClient->execSqlSync(
            "UPDATE paper_boats SET status = 'responded', updated_at = NOW() WHERE boat_id = $1",
            boatId
        );

        // 创建回复纸船
        std::string reply_boat_id = IdGenerator::generateBoatId();
        dbClient->execSqlSync(
            "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, "
            "is_anonymous, status, created_at) "
            "VALUES ($1, NULL, $2, $3, true, 'delivered', NOW())",
            reply_boat_id, user_id, content
        );

        // 通知原纸船主人
        std::string notif_id = IdGenerator::generateNotificationId();
        dbClient->execSqlAsync(
            "INSERT INTO notifications (notification_id, user_id, type, content, related_id, related_type, is_read, created_at) "
            "VALUES ($1, $2, 'boat_reply', '有人回应了你的纸船', $3, 'boat', false, NOW())",
            [boat_owner_id](const drogon::orm::Result &) {
                Json::Value notifMsg;
                notifMsg["type"] = "new_notification";
                notifMsg["notification_type"] = "boat_reply";
                notifMsg["content"] = "有人回应了你的纸船";
                BroadcastWebSocketController::sendToUser(boat_owner_id, notifMsg);
            },
            [](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "Failed to create reply notification: " << e.base().what();
            },
            notif_id, boat_owner_id, reply_boat_id
        );

        // 自动建立临时好友关系（纸船回应触发，24小时有效）
        if (user_id != boat_owner_id) {
            auto tempFriendId = drogon::utils::getUuid();
            auto expiresAt = trantor::Date::date().after(24 * 3600);
            // 确保 user_id_1 < user_id_2 以满足唯一约束的一致性
            std::string uid1 = std::min(user_id, boat_owner_id);
            std::string uid2 = std::max(user_id, boat_owner_id);
            dbClient->execSqlAsync(
                "INSERT INTO temp_friends (temp_friend_id, user_id_1, user_id_2, source, source_id, status, expires_at) "
                "VALUES ($1, $2, $3, 'boat', $4, 'active', $5) "
                "ON CONFLICT ON CONSTRAINT unique_temp_friendship DO UPDATE SET "
                "expires_at = GREATEST(temp_friends.expires_at, EXCLUDED.expires_at), "
                "status = 'active'",
                [](const drogon::orm::Result &) {
                    LOG_DEBUG << "Temp friendship created/refreshed via boat response";
                },
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Failed to create temp friendship: " << e.base().what();
                },
                tempFriendId, uid1, uid2, reply_boat_id, expiresAt.toDbStringLocal()
            );
        }

        Json::Value data;
        data["reply_boat_id"] = reply_boat_id;
        data["original_boat_id"] = boatId;

        callback(ResponseUtil::success(data, "回应成功"));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in respondToBoat: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in respondToBoat: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void PaperBoatController::releaseBoat(const HttpRequestPtr &req,
                                      std::function<void(const HttpResponsePtr &)> &&callback,
                                      const std::string &boatId) {
    try {
        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        std::string user_id;
        try {
            user_id = req->getAttributes()->get<std::string>("user_id");
        } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");

        // 将纸船放回水中
        auto result = dbClient->execSqlSync(
            "UPDATE paper_boats SET status = 'drifting', updated_at = NOW() "
            "WHERE boat_id = $1 AND status = 'caught' "
            "RETURNING boat_id",
            boatId
        );

        if (result.empty()) {
            callback(ResponseUtil::notFound("纸船不存在或无法释放"));
            return;
        }

        Json::Value data;
        data["boat_id"] = boatId;
        data["status"] = "drifting";

        callback(ResponseUtil::success(data, "纸船已放回水中"));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in releaseBoat: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in releaseBoat: " << e.what();
        callback(ResponseUtil::internalError());
    }
}


void PaperBoatController::getBoatDetail(const HttpRequestPtr &/*req*/,
                                        std::function<void(const HttpResponsePtr &)> &&callback,
                                        const std::string &boatId) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT b.boat_id, b.stone_id, b.content, b.boat_color, b.is_anonymous, "
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

        auto row = result[0];

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
        std::string user_id;
        try {
            user_id = req->getAttributes()->get<std::string>("user_id");
        } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        int page = 1, page_size = 20;
        if (auto p = req->getParameter("page"); !p.empty()) { try { page = std::stoi(p); } catch (...) {} }
        if (auto p = req->getParameter("page_size"); !p.empty()) { try { page_size = std::stoi(p); } catch (...) {} }
        std::string status_filter = req->getParameter("status");

        if (page < 1) page = 1;
        if (page_size < 1 || page_size > 100) page_size = 20;

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
                int t = countResult[0]["total"].as<int>();
                auto r = dbClient->execSqlSync(
                    "SELECT b.boat_id, b.stone_id, b.content, b.boat_color, b.status, "
                    "b.is_ai_reply, "
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
                int t = countResult[0]["total"].as<int>();
                auto r = dbClient->execSqlSync(
                    "SELECT b.boat_id, b.stone_id, b.content, b.boat_color, b.status, "
                    "b.is_ai_reply, "
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
            boat["is_ai_reply"] = row["is_ai_reply"].isNull() ? false : row["is_ai_reply"].as<bool>();
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
        std::string user_id;
        try {
            user_id = req->getAttributes()->get<std::string>("user_id");
        } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        int page = 1, page_size = 20;
        if (auto p = req->getParameter("page"); !p.empty()) { try { page = std::stoi(p); } catch (...) {} }
        if (auto p = req->getParameter("page_size"); !p.empty()) { try { page_size = std::stoi(p); } catch (...) {} }

        if (page < 1) page = 1;
        if (page_size < 1 || page_size > 100) page_size = 20;

        auto dbClient = drogon::app().getDbClient("default");

        // 获取关联到该用户石头的纸船
        auto countResult = dbClient->execSqlSync(
            "SELECT COUNT(*) as total FROM paper_boats b "
            "JOIN stones s ON b.stone_id = s.stone_id "
            "WHERE s.user_id = $1 AND b.sender_id != $1",
            user_id
        );
        int total = countResult[0]["total"].as<int>();

        int offset = (page - 1) * page_size;
        auto result = dbClient->execSqlSync(
            "SELECT b.boat_id, b.stone_id, b.content, b.boat_color, b.status, "
            "b.is_ai_reply, "
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
            boat["is_ai_reply"] = row["is_ai_reply"].isNull() ? false : row["is_ai_reply"].as<bool>();
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

void PaperBoatController::getDriftingCount(const HttpRequestPtr &/*req*/,
                                           std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT COUNT(*) as count FROM paper_boats WHERE status = 'drifting'"
        );

        Json::Value data;
        data["drifting_count"] = result[0]["count"].as<int>();

        callback(ResponseUtil::success(data));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in getDriftingCount: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getDriftingCount: " << e.what();
        callback(ResponseUtil::internalError());
    }
}


void PaperBoatController::getBoatStatus(const HttpRequestPtr &/*req*/,
                                        std::function<void(const HttpResponsePtr &)> &&callback,
                                        const std::string &boatId) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT b.status, b.is_ai_reply, "
            "EXTRACT(EPOCH FROM b.created_at) as created_at_ts, "
            "EXTRACT(EPOCH FROM b.updated_at) as updated_at_ts "
            "FROM paper_boats b WHERE b.boat_id = $1",
            boatId
        );

        if (result.empty()) {
            callback(ResponseUtil::notFound("纸船不存在"));
            return;
        }

        auto row = result[0];

        Json::Value data;
        data["boat_id"] = boatId;
        data["status"] = row["status"].as<std::string>();
        data["is_ai_reply"] = row["is_ai_reply"].isNull() ? false : row["is_ai_reply"].as<bool>();
        data["created_at"] = static_cast<Json::Int64>(row["created_at_ts"].as<double>());
        data["updated_at"] = static_cast<Json::Int64>(row["updated_at_ts"].as<double>());

        callback(ResponseUtil::success(data));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in getBoatStatus: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getBoatStatus: " << e.what();
        callback(ResponseUtil::internalError());
    }
}


int PaperBoatController::calculateDriftDelay() {
    // 基础延迟 1-5分钟（秒）- 使用 thread_local 保证线程安全
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(60, 300);
    return dis(gen);
}

int PaperBoatController::calculateSoulDistanceDelay(const std::string& content, const std::string& targetStoneId) {
    auto db = drogon::app().getDbClient("default");
    try {
        auto result = db->execSqlSync(
            "SELECT content FROM stones WHERE stone_id = $1", targetStoneId
        );
        if (result.empty()) return calculateDriftDelay();

        std::string targetContent = result[0]["content"].isNull() ? "" : result[0]["content"].as<std::string>();

        auto& engine = heartlake::ai::AdvancedEmbeddingEngine::getInstance();
        auto vec1 = engine.generateEmbedding(content);
        auto vec2 = engine.generateEmbedding(targetContent);

        if (vec1.empty() || vec2.empty()) return calculateDriftDelay();

        // 计算余弦相似度
        float dot = 0.0f, norm1 = 0.0f, norm2 = 0.0f;
        for (size_t i = 0; i < vec1.size() && i < vec2.size(); ++i) {
            dot += vec1[i] * vec2[i];
            norm1 += vec1[i] * vec1[i];
            norm2 += vec2[i] * vec2[i];
        }
        float similarity = (norm1 > 0 && norm2 > 0) ? dot / (std::sqrt(norm1) * std::sqrt(norm2)) : 0.0f;

        return heartlake::infrastructure::ResonanceSearchService::getInstance()
            .calculateDeliveryDelay(similarity);
    } catch (...) {
        return calculateDriftDelay();
    }
}

