/**
 * @file InteractionApplicationService.cpp
 * @brief 互动应用服务 —— 涟漪、纸船、通知、临时连接的完整业务流程
 *
 * 核心交互模型：
 *   - 涟漪（Ripple）：类似"点赞"，每个用户对同一石头只能涟漪一次（唯一约束幂等）
 *   - 纸船（Boat）：类似"评论"，支持匿名发送，附带情感暖意评分
 *   - 通知（Notification）：系统推送，支持单条/全部已读
 *   - 临时连接（Connection）：24h 有效的匿名聊天通道，可升级为好友
 *
 * 事务策略：涟漪/纸船的创建和计数器更新在同一事务内完成，保证一致性。
 * 缓存策略：每次写操作后主动失效对应石头缓存，确保前端读到最新计数。
 */

#include "application/InteractionApplicationService.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/services/GuardianIncentiveService.h"
#include <drogon/drogon.h>
#include <algorithm>

using namespace heartlake::utils;

namespace heartlake {
namespace application {

// ==================== 涟漪 ====================

/**
 * 创建涟漪（点赞）：
 *   1. 事务内验证石头存在 → 插入涟漪 → 递增计数器
 *   2. 失效缓存 → 发布事件 → 记录激励积分
 *   3. 唯一约束冲突时走幂等分支，返回已有涟漪信息
 */
Json::Value InteractionApplicationService::createRipple(
    const std::string& stoneId,
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 使用事务+唯一约束防止竞态条件
        auto trans = dbClient->newTransaction();

        // 验证石头存在
        auto stoneCheck = trans->execSqlSync(
            "SELECT stone_id FROM stones WHERE stone_id = $1 AND status = 'published'",
            stoneId
        );
        if (stoneCheck.empty()) {
            throw std::runtime_error("石头不存在");
        }

        std::string rippleId = utils::IdGenerator::generateRippleId();

        // 原子插入，依赖唯一约束(stone_id, user_id)防止重复
        trans->execSqlSync(
            "INSERT INTO ripples (ripple_id, stone_id, user_id, created_at) "
            "VALUES ($1, $2, $3, NOW())",
            rippleId, stoneId, userId
        );

        // 同步更新计数器
        trans->execSqlSync(
            "UPDATE stones SET ripple_count = ripple_count + 1 WHERE stone_id = $1",
            stoneId
        );

        // 清除石头缓存，确保计数器实时更新
        if (cacheManager_) {
            cacheManager_->invalidate("stone:" + stoneId);
        }

        // 发布事件
        core::events::RippleCreatedEvent event;
        event.rippleId = rippleId;
        event.stoneId = stoneId;
        event.userId = userId;
        eventBus_->publish(event);

        // 共鸣激励：记录优质涟漪积分
        heartlake::infrastructure::GuardianIncentiveService::getInstance()
            .recordQualityRipple(userId, stoneId);

        // 查询最新计数返回给前端
        auto countResult = trans->execSqlSync(
            "SELECT ripple_count FROM stones WHERE stone_id = $1", stoneId);
        int rippleCount = countResult.empty() ? 1 : countResult[0]["ripple_count"].as<int>();

        Json::Value result;
        result["ripple_id"] = rippleId;
        result["stone_id"] = stoneId;
        result["user_id"] = userId;
        result["ripple_count"] = rippleCount;

        LOG_INFO << "Ripple created: " << rippleId;
        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        std::string err = e.base().what();
        if (err.find("unique") != std::string::npos || err.find("duplicate") != std::string::npos) {
            // 幂等处理：重复点涟漪时返回当前状态，避免前端出现权限类误判
            auto existingRipple = dbClient->execSqlSync(
                "SELECT ripple_id FROM ripples WHERE stone_id = $1 AND user_id = $2 "
                "ORDER BY created_at DESC LIMIT 1",
                stoneId, userId
            );
            auto countResult = dbClient->execSqlSync(
                "SELECT ripple_count FROM stones WHERE stone_id = $1",
                stoneId
            );

            Json::Value result;
            result["ripple_id"] = existingRipple.empty()
                                      ? ""
                                      : existingRipple[0]["ripple_id"].as<std::string>();
            result["stone_id"] = stoneId;
            result["user_id"] = userId;
            result["ripple_count"] = countResult.empty()
                                         ? 0
                                         : countResult[0]["ripple_count"].as<int>();
            result["already_rippled"] = true;

            LOG_INFO << "Ripple already exists for stone " << stoneId << " user " << userId;
            return result;
        }
        LOG_ERROR << "Failed to create ripple: " << err;
        throw std::runtime_error("创建涟漪失败");
    }
}

Json::Value InteractionApplicationService::getRipples(
    const std::string& stoneId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;

        auto result = dbClient->execSqlSync(
            "SELECT r.ripple_id, r.user_id, r.created_at, "
            "u.username, u.nickname, u.avatar_url "
            "FROM ripples r "
            "LEFT JOIN users u ON r.user_id = u.user_id "
            "WHERE r.stone_id = $1 "
            "ORDER BY r.created_at DESC "
            "LIMIT $2 OFFSET $3",
            stoneId, std::to_string(pageSize), std::to_string(offset)
        );

        Json::Value ripples(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value ripple;
            ripple["ripple_id"] = row["ripple_id"].as<std::string>();
            ripple["user_id"] = row["user_id"].as<std::string>();
            ripple["created_at"] = row["created_at"].as<std::string>();

            Json::Value user;
            user["user_id"] = row["user_id"].as<std::string>();
            user["username"] = row["username"].as<std::string>();
            user["nickname"] = row["nickname"].as<std::string>();
            if (!row["avatar_url"].isNull()) {
                user["avatar_url"] = row["avatar_url"].as<std::string>();
            }
            ripple["user"] = user;

            ripples.append(ripple);
        }

        return ripples;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get ripples: " << e.base().what();
        throw std::runtime_error("获取涟漪列表失败");
    }
}

void InteractionApplicationService::deleteRipple(
    const std::string& rippleId,
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        auto trans = dbClient->newTransaction();

        // 获取石头ID并验证权限
        auto result = trans->execSqlSync(
            "SELECT stone_id FROM ripples WHERE ripple_id = $1 AND user_id = $2",
            rippleId, userId
        );

        if (result.empty()) {
            throw std::runtime_error("涟漪不存在或无权删除");
        }

        auto row = *safeRow(result);
        std::string stoneId = row["stone_id"].as<std::string>();

        // 删除涟漪并更新计数器
        trans->execSqlSync("DELETE FROM ripples WHERE ripple_id = $1", rippleId);
        trans->execSqlSync(
            "UPDATE stones SET ripple_count = GREATEST(ripple_count - 1, 0) WHERE stone_id = $1",
            stoneId
        );

        // 清除石头缓存
        if (cacheManager_) {
            cacheManager_->invalidate("stone:" + stoneId);
        }

        LOG_INFO << "Ripple deleted: " << rippleId;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to delete ripple: " << e.base().what();
        throw std::runtime_error("删除涟漪失败");
    }
}

// ==================== 纸船（私信） ====================

/// 发送纸船私信：生成 ID → 入库 → 发布事件 → 返回结果
Json::Value InteractionApplicationService::sendBoat(
    const std::string& stoneId,
    const std::string& senderId,
    const std::string& receiverId,
    const std::string& message
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 1. 生成纸船 ID
        std::string boatId = utils::IdGenerator::generateBoatId();

        // 2. 插入数据库
        dbClient->execSqlSync(
            "INSERT INTO paper_boats (boat_id, stone_id, sender_id, receiver_id, content, "
            "is_anonymous, status, created_at) VALUES ($1, $2, $3, $4, $5, true, 'active', NOW())",
            boatId, stoneId, senderId, receiverId, message
        );

        // 3. 发布事件
        core::events::BoatSentEvent event;
        event.boatId = boatId;
        event.stoneId = stoneId;
        event.senderId = senderId;
        event.content = message;

        eventBus_->publish(event);

        // 4. 返回结果
        Json::Value result;
        result["boat_id"] = boatId;
        result["stone_id"] = stoneId;
        result["sender_id"] = senderId;
        result["receiver_id"] = receiverId;
        result["message"] = message;
        result["status"] = "sent";

        LOG_INFO << "Boat sent: " << boatId;

        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to send boat: " << e.base().what();
        throw std::runtime_error("发送纸船失败");
    }
}

Json::Value InteractionApplicationService::getReceivedBoats(
    const std::string& userId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;

        auto result = dbClient->execSqlSync(
            "SELECT pb.boat_id, pb.stone_id, pb.sender_id, pb.content, pb.status, pb.created_at, "
            "u.username, u.nickname, u.avatar_url, "
            "s.content as stone_content "
            "FROM paper_boats pb "
            "LEFT JOIN users u ON pb.sender_id = u.user_id "
            "LEFT JOIN stones s ON pb.stone_id = s.stone_id "
            "WHERE pb.receiver_id = $1 "
            "ORDER BY pb.created_at DESC "
            "LIMIT $2 OFFSET $3",
            userId, std::to_string(pageSize), std::to_string(offset)
        );

        Json::Value boats(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value boat;
            boat["boat_id"] = row["boat_id"].as<std::string>();
            boat["stone_id"] = row["stone_id"].as<std::string>();
            boat["sender_id"] = row["sender_id"].as<std::string>();
            boat["content"] = row["content"].as<std::string>();
            boat["status"] = row["status"].as<std::string>();
            boat["created_at"] = row["created_at"].as<std::string>();

            Json::Value sender;
            sender["user_id"] = row["sender_id"].as<std::string>();
            sender["username"] = row["username"].as<std::string>();
            sender["nickname"] = row["nickname"].as<std::string>();
            if (!row["avatar_url"].isNull()) {
                sender["avatar_url"] = row["avatar_url"].as<std::string>();
            }
            boat["sender"] = sender;

            if (!row["stone_content"].isNull()) {
                boat["stone_content"] = row["stone_content"].as<std::string>();
            }
            boats.append(boat);
        }

        return boats;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get received boats: " << e.base().what();
        throw std::runtime_error("获取收到的纸船失败");
    }
}

Json::Value InteractionApplicationService::getSentBoats(
    const std::string& userId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;

        auto result = dbClient->execSqlSync(
            "SELECT pb.boat_id, pb.stone_id, pb.receiver_id, pb.content, pb.status, pb.created_at, "
            "u.username, u.nickname, u.avatar_url "
            "FROM paper_boats pb "
            "LEFT JOIN users u ON pb.receiver_id = u.user_id "
            "WHERE pb.sender_id = $1 "
            "ORDER BY pb.created_at DESC "
            "LIMIT $2 OFFSET $3",
            userId, std::to_string(pageSize), std::to_string(offset)
        );

        Json::Value boats(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value boat;
            boat["boat_id"] = row["boat_id"].as<std::string>();
            boat["stone_id"] = row["stone_id"].as<std::string>();
            boat["receiver_id"] = row["receiver_id"].as<std::string>();
            boat["content"] = row["content"].as<std::string>();
            boat["status"] = row["status"].as<std::string>();
            boat["created_at"] = row["created_at"].as<std::string>();

            Json::Value receiver;
            receiver["user_id"] = row["receiver_id"].as<std::string>();
            receiver["username"] = row["username"].as<std::string>();
            receiver["nickname"] = row["nickname"].as<std::string>();
            if (!row["avatar_url"].isNull()) {
                receiver["avatar_url"] = row["avatar_url"].as<std::string>();
            }
            boat["receiver"] = receiver;
            boats.append(boat);
        }

        return boats;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get sent boats: " << e.base().what();
        throw std::runtime_error("获取发送的纸船失败");
    }
}

// ==================== 纸船（评论系统）====================

/**
 * 创建纸船评论：
 *   1. 入库 → 递增石头的 boat_count → 失效缓存
 *   2. 用 EdgeAIEngine 本地情感分析评估暖意分，按比例发放激励积分
 */
Json::Value InteractionApplicationService::createBoat(
    const std::string& stoneId,
    const std::string& userId,
    const std::string& content
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 1. 生成纸船 ID
        std::string boatId = utils::IdGenerator::generateBoatId();

        // 2. 插入数据库（简化版本，不需要receiver_id）
        dbClient->execSqlSync(
            "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, "
            "is_anonymous, status, created_at) VALUES ($1, $2, $3, $4, true, 'active', NOW())",
            boatId, stoneId, userId, content
        );

        // 2.5 更新石头的纸船计数
        dbClient->execSqlSync(
            "UPDATE stones SET boat_count = boat_count + 1 WHERE stone_id = $1",
            stoneId
        );

        // 2.6 清除石头缓存，确保计数器实时更新
        if (cacheManager_) {
            cacheManager_->invalidate("stone:" + stoneId);
        }

        // 查询最新计数返回给前端
        auto countResult = dbClient->execSqlSync(
            "SELECT boat_count FROM stones WHERE stone_id = $1", stoneId);
        int boatCount = countResult.empty() ? 1 : countResult[0]["boat_count"].as<int>();

        // 温暖纸船激励：用本地情绪模型估算暖意分，按比例发放积分
        auto& edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
        auto sentiment = edgeEngine.analyzeSentimentLocal(content);
        const float warmthScore = std::clamp((sentiment.score + 1.0f) / 2.0f, 0.2f, 1.0f);
        heartlake::infrastructure::GuardianIncentiveService::getInstance()
            .recordWarmBoat(userId, boatId, warmthScore);

        // 3. 返回结果
        Json::Value result;
        result["boat_id"] = boatId;
        result["stone_id"] = stoneId;
        result["sender_id"] = userId;
        result["message"] = content;
        result["status"] = "sent";
        result["boat_count"] = boatCount;

        LOG_INFO << "Boat created: " << boatId;

        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to create boat: " << e.base().what();
        throw std::runtime_error("创建纸船失败");
    }
}

/// 删除纸船：事务内删除记录 + RETURNING 递减计数器 + 失效缓存
Json::Value InteractionApplicationService::deleteBoat(
    const std::string& boatId,
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        auto trans = dbClient->newTransaction();

        // 查出 stone_id 用于后续更新计数
        auto boatInfo = trans->execSqlSync(
            "SELECT stone_id FROM paper_boats WHERE boat_id = $1 AND sender_id = $2",
            boatId, userId
        );

        if (boatInfo.empty()) {
            throw std::runtime_error("纸船不存在或无权删除");
        }

        std::string stoneId = boatInfo[0]["stone_id"].as<std::string>();

        // 删除纸船
        trans->execSqlSync(
            "DELETE FROM paper_boats WHERE boat_id = $1",
            boatId
        );

        // 在事务内递减计数并返回最新值
        auto countResult = trans->execSqlSync(
            "UPDATE stones SET boat_count = GREATEST(boat_count - 1, 0) WHERE stone_id = $1 RETURNING boat_count",
            stoneId
        );
        int boatCount = countResult.empty() ? 0 : countResult[0]["boat_count"].as<int>();

        // 清除石头缓存
        if (cacheManager_) {
            cacheManager_->invalidate("stone:" + stoneId);
        }

        Json::Value result;
        result["stone_id"] = stoneId;
        result["boat_count"] = boatCount;

        LOG_INFO << "Boat deleted: " << boatId;
        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to delete boat: " << e.base().what();
        throw std::runtime_error("删除纸船失败");
    }
}

// ==================== 通知 ====================

/// 标记单条通知为已读
void InteractionApplicationService::markNotificationRead(
    const std::string& notificationId,
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        dbClient->execSqlSync(
            "UPDATE notifications SET is_read = true "
            "WHERE notification_id = $1 AND user_id = $2",
            notificationId, userId
        );

        LOG_INFO << "Notification marked as read: " << notificationId;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to mark notification as read: " << e.base().what();
        throw std::runtime_error("标记通知已读失败");
    }
}

/// 一键标记用户所有通知为已读
void InteractionApplicationService::markAllNotificationsRead(
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        dbClient->execSqlSync(
            "UPDATE notifications SET is_read = true WHERE user_id = $1",
            userId
        );

        LOG_INFO << "All notifications marked as read for user: " << userId;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to mark all notifications as read: " << e.base().what();
        throw std::runtime_error("标记所有通知已读失败");
    }
}

Json::Value InteractionApplicationService::getNotifications(
    const std::string& userId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;

        auto result = dbClient->execSqlSync(
            "SELECT notification_id, type, content, related_id, related_type, "
            "is_read, created_at FROM notifications "
            "WHERE user_id = $1 "
            "ORDER BY created_at DESC "
            "LIMIT $2 OFFSET $3",
            userId, static_cast<int64_t>(pageSize), static_cast<int64_t>(offset)
        );

        Json::Value notifications(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value notification;
            notification["notification_id"] = row["notification_id"].as<std::string>();
            notification["type"] = row["type"].as<std::string>();
            notification["content"] = row["content"].as<std::string>();
            notification["related_id"] = row["related_id"].isNull() ? "" : row["related_id"].as<std::string>();
            notification["related_type"] = row["related_type"].isNull() ? "" : row["related_type"].as<std::string>();
            notification["is_read"] = row["is_read"].as<bool>();
            notification["created_at"] = row["created_at"].as<std::string>();

            notifications.append(notification);
        }

        return notifications;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get notifications: " << e.base().what();
        throw std::runtime_error("获取通知列表失败");
    }
}

// ==================== 临时连接 ====================

/// 基于石头创建临时连接（24h 有效），用于匿名聊天
Json::Value InteractionApplicationService::createConnectionForStone(
    const std::string& stoneId,
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 1. 生成连接 ID
        std::string connectionId = utils::IdGenerator::generateConnectionId();

        // 2. 插入数据库
        dbClient->execSqlSync(
            "INSERT INTO temp_connections (connection_id, stone_id, user_id, "
            "status, created_at, expires_at) "
            "VALUES ($1, $2, $3, 'active', NOW(), NOW() + INTERVAL '24 hours')",
            connectionId, stoneId, userId
        );

        // 3. 返回结果
        Json::Value result;
        result["connection_id"] = connectionId;
        result["stone_id"] = stoneId;
        result["user_id"] = userId;
        result["status"] = "active";

        LOG_INFO << "Connection created: " << connectionId;

        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to create connection: " << e.base().what();
        throw std::runtime_error("创建连接失败");
    }
}

/// 基于目标用户创建直接临时连接（24h 有效）
Json::Value InteractionApplicationService::createConnection(
    const std::string& targetUserId,
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        std::string connectionId = utils::IdGenerator::generateConnectionId();

        dbClient->execSqlSync(
            "INSERT INTO temp_connections (connection_id, user_id, target_user_id, "
            "status, created_at, expires_at) "
            "VALUES ($1, $2, $3, 'active', NOW(), NOW() + INTERVAL '24 hours')",
            connectionId, userId, targetUserId
        );

        Json::Value result;
        result["connection_id"] = connectionId;
        result["user_id"] = userId;
        result["target_user_id"] = targetUserId;
        result["status"] = "active";

        LOG_INFO << "Direct connection created: " << connectionId;

        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to create direct connection: " << e.base().what();
        throw std::runtime_error("创建连接失败");
    }
}

/// 在连接内发送消息，先验证用户是连接参与者（防 IDOR）
Json::Value InteractionApplicationService::createConnectionMessage(
    const std::string& connectionId,
    const std::string& userId,
    const std::string& content
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 0. 验证用户是 connection 的参与者（防止 IDOR）
        auto connCheck = dbClient->execSqlSync(
            "SELECT connection_id FROM connections WHERE connection_id = $1 AND (user_id_1 = $2 OR user_id_2 = $2)",
            connectionId, userId
        );
        if (connCheck.empty()) {
            throw std::runtime_error("无权操作此连接");
        }

        // 1. 生成消息 ID
        std::string messageId = utils::IdGenerator::generateMessageId();

        // 2. 插入数据库
        dbClient->execSqlSync(
            "INSERT INTO connection_messages (message_id, connection_id, sender_id, "
            "content, created_at) VALUES ($1, $2, $3, $4, NOW())",
            messageId, connectionId, userId, content
        );

        // 3. 返回结果
        Json::Value result;
        result["message_id"] = messageId;
        result["connection_id"] = connectionId;
        result["sender_id"] = userId;
        result["content"] = content;

        LOG_INFO << "Connection message created: " << messageId;

        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to create connection message: " << e.base().what();
        throw std::runtime_error("发送连接消息失败");
    }
}

Json::Value InteractionApplicationService::getMyRipples(
    const std::string& userId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;

        auto result = dbClient->execSqlSync(
            "SELECT r.ripple_id, r.stone_id, r.created_at, "
            "s.content as stone_content, s.mood_type "
            "FROM ripples r "
            "LEFT JOIN stones s ON r.stone_id = s.stone_id "
            "WHERE r.user_id = $1 "
            "ORDER BY r.created_at DESC "
            "LIMIT $2 OFFSET $3",
            userId, std::to_string(pageSize), std::to_string(offset)
        );

        Json::Value ripples(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value ripple;
            ripple["ripple_id"] = row["ripple_id"].as<std::string>();
            ripple["stone_id"] = row["stone_id"].as<std::string>();
            ripple["created_at"] = row["created_at"].as<std::string>();

            if (!row["stone_content"].isNull()) {
                ripple["stone_content"] = row["stone_content"].as<std::string>();
            }
            if (!row["mood_type"].isNull()) {
                ripple["mood_type"] = row["mood_type"].as<std::string>();
            }

            ripples.append(ripple);
        }

        return ripples;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get my ripples: " << e.base().what();
        throw std::runtime_error("获取我的涟漪失败");
    }
}

Json::Value InteractionApplicationService::getMyBoats(
    const std::string& userId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;

        auto result = dbClient->execSqlSync(
            "SELECT pb.boat_id, pb.stone_id, pb.content, pb.status, pb.created_at, "
            "s.content as stone_content "
            "FROM paper_boats pb "
            "LEFT JOIN stones s ON pb.stone_id = s.stone_id "
            "WHERE pb.sender_id = $1 "
            "ORDER BY pb.created_at DESC "
            "LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset),
            userId
        );

        Json::Value boats(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value boat;
            boat["boat_id"] = row["boat_id"].isNull() ? "" : row["boat_id"].as<std::string>();
            boat["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
            boat["content"] = row["content"].isNull() ? "" : row["content"].as<std::string>();
            boat["status"] = row["status"].isNull() ? "unknown" : row["status"].as<std::string>();
            boat["created_at"] = row["created_at"].as<std::string>();
            if (!row["stone_content"].isNull()) {
                boat["stone_content"] = row["stone_content"].as<std::string>();
            }
            boats.append(boat);
        }

        return boats;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get my boats: " << e.base().what();
        throw std::runtime_error("获取我的纸船失败");
    }
}

Json::Value InteractionApplicationService::getBoats(
    const std::string& stoneId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");
    try {
        int64_t offset = (page - 1) * pageSize;
        auto result = dbClient->execSqlSync(
            "SELECT pb.boat_id, pb.sender_id, pb.content, pb.created_at, "
            "u.nickname as sender_nickname "
            "FROM paper_boats pb "
            "LEFT JOIN users u ON pb.sender_id = u.user_id "
            "WHERE pb.stone_id = $1 "
            "ORDER BY pb.created_at DESC LIMIT $2 OFFSET $3",
            stoneId, static_cast<int64_t>(pageSize), offset
        );
        Json::Value boats(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value boat;
            boat["boat_id"] = row["boat_id"].as<std::string>();
            boat["sender_id"] = row["sender_id"].as<std::string>();
            boat["content"] = row["content"].as<std::string>();
            boat["created_at"] = row["created_at"].as<std::string>();
            Json::Value author;
            author["nickname"] = row["sender_nickname"].isNull() ? "匿名旅人" : row["sender_nickname"].as<std::string>();
            author["is_anonymous"] = true;
            boat["author"] = author;
            boats.append(boat);
        }
        return boats;
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get boats: " << e.base().what();
        throw std::runtime_error("获取纸船列表失败");
    }
}

Json::Value InteractionApplicationService::getConnectionMessages(
    const std::string& connectionId,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");
    try {
        int offset = (page - 1) * pageSize;
        auto result = dbClient->execSqlSync(
            "SELECT message_id, sender_id, content, created_at "
            "FROM connection_messages WHERE connection_id = $1 "
            "ORDER BY created_at ASC LIMIT $2 OFFSET $3",
            connectionId, std::to_string(pageSize), std::to_string(offset)
        );
        Json::Value messages(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value msg;
            msg["message_id"] = row["message_id"].as<std::string>();
            msg["sender_id"] = row["sender_id"].as<std::string>();
            msg["content"] = row["content"].as<std::string>();
            msg["created_at"] = row["created_at"].as<std::string>();
            messages.append(msg);
        }
        return messages;
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get connection messages: " << e.base().what();
        throw std::runtime_error("获取连接消息失败");
    }
}

/// 将临时连接升级为正式好友关系，同时将连接状态标记为 'upgraded'
Json::Value InteractionApplicationService::upgradeConnectionToFriend(
    const std::string& connectionId,
    const std::string& userId
) {
    auto dbClient = drogon::app().getDbClient("default");
    try {
        auto trans = dbClient->newTransaction();

        // 获取连接信息
        auto connResult = trans->execSqlSync(
            "SELECT stone_id, user_id FROM temp_connections WHERE connection_id = $1 AND status = 'active'",
            connectionId
        );
        if (connResult.empty()) {
            throw std::runtime_error("连接不存在或已过期");
        }
        std::string otherUserId = connResult[0]["user_id"].as<std::string>();

        // 创建好友关系
        std::string friendshipId = utils::IdGenerator::generateConnectionId();
        trans->execSqlSync(
            "INSERT INTO friendships (friendship_id, user_id, friend_id, status, created_at) "
            "VALUES ($1, $2, $3, 'accepted', NOW())",
            friendshipId, userId, otherUserId
        );

        // 更新连接状态
        trans->execSqlSync(
            "UPDATE temp_connections SET status = 'upgraded' WHERE connection_id = $1",
            connectionId
        );

        Json::Value result;
        result["friendship_id"] = friendshipId;
        result["friend_id"] = otherUserId;
        return result;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to upgrade connection: " << e.base().what();
        throw std::runtime_error("升级好友失败");
    }
}

} // namespace application
} // namespace heartlake
