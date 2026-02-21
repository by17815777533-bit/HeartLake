/**
 * @file InteractionController.cpp
 * @brief InteractionController 模块实现
 * Created by 白洋
 */

#include "interfaces/api/InteractionController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "application/InteractionApplicationService.h"
#include "utils/ResponseUtil.h"
#include "infrastructure/di/ServiceLocator.h"
#include <memory>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::application;

// 全局ApplicationService实例
static std::shared_ptr<InteractionApplicationService> getInteractionService() {
    return heartlake::core::di::ServiceLocator::instance().resolve<InteractionApplicationService>();
}

// 辅助函数：从请求中提取用户ID
static std::string extractUserId(const HttpRequestPtr& req) {
    try {
        auto userId = req->getAttributes()->get<std::string>("user_id");
        if (userId.empty()) {
            throw std::runtime_error("未登录");
        }
        return userId;
    } catch (...) {
        throw std::runtime_error("未登录");
    }
}

// 辅助函数：安全解析分页参数，防止溢出
static void parsePaginationParams(const HttpRequestPtr& req, int& page, int& pageSize) {
    page = 1;
    pageSize = 20;
    if (auto p = req->getParameter("page"); !p.empty()) {
        try { page = std::stoi(p); } catch (...) {}
    }
    if (auto p = req->getParameter("page_size"); !p.empty()) {
        try { pageSize = std::stoi(p); } catch (...) {}
    }
    // 边界验证防止溢出
    if (page < 1 || page > 10000) page = 1;
    if (pageSize < 1 || pageSize > 100) pageSize = 20;
}

// ==================== 涟漪相关 ====================

void InteractionController::createRipple(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    try {
        std::string userId = extractUserId(req);

        auto service = getInteractionService();
        auto result = service->createRipple(stoneId, userId);

        // 广播涟漪更新事件
        {
            int rippleCount = result["ripple_count"].asInt();

            Json::Value broadcastMsg;
            broadcastMsg["type"] = "ripple_update";
            broadcastMsg["stone_id"] = stoneId;
            broadcastMsg["ripple_count"] = rippleCount;
            broadcastMsg["triggered_by"] = userId;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::broadcast(broadcastMsg);

            // 定向通知石头主人
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
        }

        callback(ResponseUtil::success(result, "涟漪成功"));

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
    try {
        std::string userId = extractUserId(req);

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
            BroadcastWebSocketController::broadcast(broadcastMsg);
        }

        callback(ResponseUtil::success(Json::Value(), "删除涟漪成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in deleteRipple: " << e.what();
        callback(ResponseUtil::error(400, e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in deleteRipple: " << e.what();
        callback(ResponseUtil::internalError("删除涟漪失败"));
    }
}

void InteractionController::getMyRipples(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        std::string userId = extractUserId(req);

        int page, pageSize;
        parsePaginationParams(req, page, pageSize);

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

        std::string userId = extractUserId(req);

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
        callback(ResponseUtil::error(400, e.what()));
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
        int page, pageSize;
        parsePaginationParams(req, page, pageSize);

        auto service = getInteractionService();
        auto result = service->getBoats(stoneId, page, pageSize);

        callback(ResponseUtil::success(result));

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
    try {
        std::string userId = extractUserId(req);

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
            BroadcastWebSocketController::broadcast(broadcastMsg);
        }

        callback(ResponseUtil::success(Json::Value(), "删除纸船成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in deleteBoat: " << e.what();
        callback(ResponseUtil::error(400, e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in deleteBoat: " << e.what();
        callback(ResponseUtil::internalError("删除纸船失败"));
    }
}

void InteractionController::getMyBoats(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        std::string userId = extractUserId(req);

        int page, pageSize;
        parsePaginationParams(req, page, pageSize);

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
    try {
        std::string userId = extractUserId(req);

        int page, pageSize;
        parsePaginationParams(req, page, pageSize);

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
    try {
        std::string userId = extractUserId(req);

        auto service = getInteractionService();
        service->markNotificationRead(notificationId, userId);

        callback(ResponseUtil::success(Json::Value(), "标记已读成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in markNotificationRead: " << e.what();
        callback(ResponseUtil::error(400, e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in markNotificationRead: " << e.what();
        callback(ResponseUtil::internalError("标记已读失败"));
    }
}

void InteractionController::markAllNotificationsRead(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        std::string userId = extractUserId(req);

        auto service = getInteractionService();
        service->markAllNotificationsRead(userId);

        callback(ResponseUtil::success(Json::Value(), "全部标记已读成功"));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in markAllNotificationsRead: " << e.what();
        callback(ResponseUtil::internalError("全部标记已读失败"));
    }
}

// ==================== 连接相关 ====================

void InteractionController::createConnectionForStone(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& stoneId
) {
    try {
        std::string userId = extractUserId(req);

        auto service = getInteractionService();
        auto result = service->createConnectionForStone(stoneId, userId);

        callback(ResponseUtil::success(result, "创建连接成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createConnectionForStone: " << e.what();
        callback(ResponseUtil::error(400, e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createConnectionForStone: " << e.what();
        callback(ResponseUtil::internalError("创建连接失败"));
    }
}

void InteractionController::createConnection(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        std::string userId = extractUserId(req);
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
        callback(ResponseUtil::error(400, e.what()));
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
    try {
        std::string userId = extractUserId(req);

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
        callback(ResponseUtil::error(400, e.what()));
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
        int page, pageSize;
        parsePaginationParams(req, page, pageSize);
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

        std::string userId = extractUserId(req);

        auto service = getInteractionService();
        auto result = service->createConnectionMessage(connectionId, userId, content);

        callback(ResponseUtil::success(result, "发送消息成功"));

    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Error in createConnectionMessage: " << e.what();
        callback(ResponseUtil::error(400, e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Unexpected error in createConnectionMessage: " << e.what();
        callback(ResponseUtil::internalError("发送消息失败"));
    }
}
