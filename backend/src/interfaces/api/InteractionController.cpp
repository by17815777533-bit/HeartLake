/**
 * InteractionController 模块实现
 */

#include "interfaces/api/InteractionController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "application/InteractionApplicationService.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include "infrastructure/di/ServiceLocator.h"
#include <memory>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::application;

// 全局ApplicationService实例
static std::shared_ptr<InteractionApplicationService> getInteractionService() {
    return heartlake::core::di::ServiceLocator::instance().resolve<InteractionApplicationService>();
}

// ==================== 涟漪相关 ====================

void InteractionController::createRipple(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto service = getInteractionService();
        auto result = service->createRipple(stoneId, userId);
        bool alreadyRippled = result.get("already_rippled", false).asBool();

        // 仅首次涟漪才广播，重复操作按幂等成功返回
        if (!alreadyRippled) {
            int rippleCount = result["ripple_count"].asInt();

            Json::Value broadcastMsg;
            broadcastMsg["type"] = "ripple_update";
            broadcastMsg["stone_id"] = stoneId;
            broadcastMsg["ripple_count"] = rippleCount;
            broadcastMsg["triggered_by"] = userId;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::broadcast(broadcastMsg);

            // 定向通知石头主人（独立 try-catch，失败不影响主流程）
            try {
                auto dbClient = drogon::app().getDbClient("default");
                auto stoneResult = dbClient->execSqlSync(
                    "SELECT user_id FROM stones WHERE stone_id = $1", stoneId);
                if (!stoneResult.empty()) {
                    std::string stoneOwnerId = stoneResult[0]["user_id"].as<std::string>();
                    if (stoneOwnerId != userId) {  // 不通知自己
                        Json::Value notifyMsg;
                        notifyMsg["type"] = "new_notification";
                        notifyMsg["notification_type"] = "ripple";
                        notifyMsg["stone_id"] = stoneId;
                        notifyMsg["from_user_id"] = userId;
                        notifyMsg["ripple_count"] = rippleCount;
                        notifyMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
                        BroadcastWebSocketController::sendToUser(stoneOwnerId, notifyMsg);
                    }
                }
            } catch (const std::exception& ex) {
                LOG_WARN << "Failed to notify stone owner for stone " << stoneId << ": " << ex.what();
            }
        }

        callback(ResponseUtil::success(result, alreadyRippled ? "已经点过涟漪了" : "涟漪成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createRipple: " << e.what();
        callback(ResponseUtil::error(400, e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createRipple: " << e.what();
        callback(ResponseUtil::internalError("创建涟漪失败"));
    }
}

void InteractionController::deleteRipple(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& rippleId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        // 先查出 stone_id 用于广播
        auto dbClient = drogon::app().getDbClient("default");
        auto rippleInfo = dbClient->execSqlSync(
            "SELECT stone_id FROM ripples WHERE ripple_id = $1", rippleId);
        std::string stoneId = rippleInfo.empty() ? "" : rippleInfo[0]["stone_id"].as<std::string>();

        auto service = getInteractionService();
        service->deleteRipple(rippleId, userId);

        // 广播涟漪删除事件
        if (!stoneId.empty()) {
            auto countResult = dbClient->execSqlSync(
                "SELECT ripple_count FROM stones WHERE stone_id = $1", stoneId);
            int rippleCount = countResult.empty() ? 0 : countResult[0]["ripple_count"].as<int>();

            Json::Value broadcastMsg;
            broadcastMsg["type"] = "ripple_deleted";
            broadcastMsg["stone_id"] = stoneId;
            broadcastMsg["ripple_id"] = rippleId;
            broadcastMsg["ripple_count"] = rippleCount;
            broadcastMsg["triggered_by"] = userId;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToRoom("stone:" + stoneId, broadcastMsg);
            BroadcastWebSocketController::broadcast(broadcastMsg);
        }

        callback(ResponseUtil::success(Json::Value(), "删除涟漪成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in deleteRipple: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in deleteRipple: " << e.what();
        callback(ResponseUtil::internalError("删除涟漪失败"));
    }
}

void InteractionController::getMyRipples(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto [page, pageSize] = safePagination(req);

        auto service = getInteractionService();
        auto result = service->getMyRipples(userId, page, pageSize);

        callback(ResponseUtil::success(result));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getMyRipples: " << e.what();
        callback(ResponseUtil::internalError("获取我的涟漪失败"));
    }
}

// ==================== 纸船相关 ====================

void InteractionController::createBoat(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
            return;
        }

        if (!json->isMember("content")) {
            callback(ResponseUtil::badRequest("缺少content字段"));
            return;
        }

        std::string content = (*json)["content"].asString();
        if (content.empty()) {
            callback(ResponseUtil::badRequest("content不能为空"));
            return;
        }

        auto service = getInteractionService();
        auto result = service->createBoat(stoneId, userId, content);

        // 广播纸船更新事件
        {
            int boatCount = result["boat_count"].asInt();

            Json::Value broadcastMsg;
            broadcastMsg["type"] = "boat_update";
            broadcastMsg["stone_id"] = stoneId;
            broadcastMsg["boat_count"] = boatCount;
            broadcastMsg["boat_id"] = result["boat_id"].asString();
            broadcastMsg["triggered_by"] = userId;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToRoom("stone:" + stoneId, broadcastMsg);
            BroadcastWebSocketController::broadcast(broadcastMsg);

            // 通知石头主人有新纸船评论
            auto dbClient = drogon::app().getDbClient("default");
            auto stoneResult = dbClient->execSqlSync(
                "SELECT user_id FROM stones WHERE stone_id = $1", stoneId);
            if (!stoneResult.empty()) {
                std::string stoneOwnerId = stoneResult[0]["user_id"].as<std::string>();
                if (stoneOwnerId != userId) {
                    Json::Value notifyMsg;
                    notifyMsg["type"] = "new_notification";
                    notifyMsg["notification_type"] = "boat";
                    notifyMsg["stone_id"] = stoneId;
                    notifyMsg["boat_id"] = result["boat_id"].asString();
                    notifyMsg["from_user_id"] = userId;
                    notifyMsg["boat_count"] = boatCount;
                    notifyMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
                    BroadcastWebSocketController::sendToUser(stoneOwnerId, notifyMsg);
                }
            }
        }

        callback(ResponseUtil::success(result, "纸船发送成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createBoat: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createBoat: " << e.what();
        callback(ResponseUtil::internalError("纸船发送失败"));
    }
}

void InteractionController::getBoats(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    try {
        auto [page, pageSize] = safePagination(req);

        auto service = getInteractionService();
        auto result = service->getBoats(stoneId, page, pageSize);

        Json::Value data(Json::objectValue);
        if (result.isArray()) {
            data["boats"] = result;
            data["items"] = result;  // 兼容旧前端字段
            data["total"] = static_cast<Json::UInt64>(result.size());
        } else {
            data = result;
            if (!data.isMember("boats") && data.isMember("items") && data["items"].isArray()) {
                data["boats"] = data["items"];
            }
            if (!data.isMember("total") && data.isMember("boats") && data["boats"].isArray()) {
                data["total"] = static_cast<Json::UInt64>(data["boats"].size());
            }
        }
        data["page"] = page;
        data["page_size"] = pageSize;

        callback(ResponseUtil::success(data));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getBoats: " << e.what();
        callback(ResponseUtil::internalError("获取纸船列表失败"));
    }
}

void InteractionController::deleteBoat(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& boatId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto service = getInteractionService();
        auto result = service->deleteBoat(boatId, userId);

        // 广播纸船删除事件
        {
            std::string stoneId = result["stone_id"].asString();
            int boatCount = result["boat_count"].asInt();

            Json::Value broadcastMsg;
            broadcastMsg["type"] = "boat_deleted";
            broadcastMsg["stone_id"] = stoneId;
            broadcastMsg["boat_id"] = boatId;
            broadcastMsg["boat_count"] = boatCount;
            broadcastMsg["triggered_by"] = userId;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToRoom("stone:" + stoneId, broadcastMsg);
            BroadcastWebSocketController::broadcast(broadcastMsg);
        }

        callback(ResponseUtil::success(Json::Value(), "删除纸船成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in deleteBoat: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in deleteBoat: " << e.what();
        callback(ResponseUtil::internalError("删除纸船失败"));
    }
}

void InteractionController::getMyBoats(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto [page, pageSize] = safePagination(req);

        auto service = getInteractionService();
        auto result = service->getMyBoats(userId, page, pageSize);

        callback(ResponseUtil::success(result));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getMyBoats: " << e.what();
        callback(ResponseUtil::internalError("获取我的纸船失败"));
    }
}

// ==================== 通知相关 ====================

void InteractionController::getNotifications(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto [page, pageSize] = safePagination(req);

        auto service = getInteractionService();
        auto result = service->getNotifications(userId, page, pageSize);

        callback(ResponseUtil::success(result));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getNotifications: " << e.what();
        callback(ResponseUtil::internalError("获取通知列表失败"));
    }
}

void InteractionController::markNotificationRead(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& notificationId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto service = getInteractionService();
        service->markNotificationRead(notificationId, userId);

        callback(ResponseUtil::success(Json::Value(), "标记已读成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in markNotificationRead: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in markNotificationRead: " << e.what();
        callback(ResponseUtil::internalError("标记已读失败"));
    }
}

void InteractionController::markAllNotificationsRead(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto service = getInteractionService();
        service->markAllNotificationsRead(userId);

        callback(ResponseUtil::success(Json::Value(), "全部标记已读成功"));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in markAllNotificationsRead: " << e.what();
        callback(ResponseUtil::internalError("全部标记已读失败"));
    }
}

void InteractionController::getUnreadCount(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto dbClient = drogon::app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "SELECT COUNT(*) as unread_count FROM notifications "
            "WHERE user_id = $1 AND is_read = false",
            userId);

        int unreadCount = 0;
        if (auto rowOpt = safeRow(result)) {
            unreadCount = (*rowOpt)["unread_count"].as<int>();
        }

        Json::Value response;
        response["unread_count"] = unreadCount;

        callback(ResponseUtil::success(response));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in getUnreadCount: " << e.what();
        callback(ResponseUtil::error(401, e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in getUnreadCount: " << e.what();
        callback(ResponseUtil::internalError("获取未读数量失败"));
    }
}

// ==================== 连接相关 ====================

void InteractionController::createConnectionForStone(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto service = getInteractionService();
        auto result = service->createConnectionForStone(stoneId, userId);

        callback(ResponseUtil::success(result, "创建连接成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createConnectionForStone: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createConnectionForStone: " << e.what();
        callback(ResponseUtil::internalError("创建连接失败"));
    }
}

void InteractionController::createConnection(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
            return;
        }

        std::string targetUserId = (*json).get("target_user_id", "").asString();
        if (targetUserId.empty()) {
            callback(ResponseUtil::badRequest("目标用户ID不能为空"));
            return;
        }

        auto service = getInteractionService();
        auto result = service->createConnection(targetUserId, userId);

        callback(ResponseUtil::success(result, "创建连接成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createConnection: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createConnection: " << e.what();
        callback(ResponseUtil::internalError("创建连接失败"));
    }
}

void InteractionController::upgradeConnectionToFriend(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& connectionId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto service = getInteractionService();
        auto result = service->upgradeConnectionToFriend(connectionId, userId);

        // 广播好友升级事件
        if (result.isMember("friend_id")) {
            Json::Value broadcastMsg;
            broadcastMsg["type"] = "friend_accepted";
            broadcastMsg["friendship_id"] = result["friendship_id"].asString();
            broadcastMsg["from_user_id"] = userId;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToUser(result["friend_id"].asString(), broadcastMsg);
        }

        callback(ResponseUtil::success(result, "升级为好友成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in upgradeConnectionToFriend: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in upgradeConnectionToFriend: " << e.what();
        callback(ResponseUtil::internalError("升级为好友失败"));
    }
}

void InteractionController::getConnectionMessages(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& connectionId
) {
    try {
        auto [page, pageSize] = safePagination(req);
        if (pageSize > 50) pageSize = 50; // 消息列表限制更严格

        auto service = getInteractionService();
        auto result = service->getConnectionMessages(connectionId, page, pageSize);

        callback(ResponseUtil::success(result));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getConnectionMessages: " << e.what();
        callback(ResponseUtil::internalError("获取消息列表失败"));
    }
}

void InteractionController::createConnectionMessage(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& connectionId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
            return;
        }

        if (!json->isMember("content")) {
            callback(ResponseUtil::badRequest("缺少content字段"));
            return;
        }

        std::string content = (*json)["content"].asString();
        if (content.empty()) {
            callback(ResponseUtil::badRequest("content不能为空"));
            return;
        }

        auto service = getInteractionService();
        auto result = service->createConnectionMessage(connectionId, userId, content);

        callback(ResponseUtil::success(result, "发送消息成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createConnectionMessage: " << e.what();
        callback(ResponseUtil::error(400, "操作失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createConnectionMessage: " << e.what();
        callback(ResponseUtil::internalError("发送消息失败"));
    }
}
