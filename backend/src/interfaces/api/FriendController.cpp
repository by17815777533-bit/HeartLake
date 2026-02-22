/**
 * @file FriendController.cpp
 * @brief FriendController 模块实现
 * Created by 白洋
 */

#include "interfaces/api/FriendController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "application/FriendApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include "utils/ResponseUtil.h"
#include <memory>
#include <functional>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::application;

static std::shared_ptr<FriendApplicationService> getFriendService() {
    return heartlake::core::di::ServiceLocator::instance().resolve<FriendApplicationService>();
}

/**
 * @brief 从请求头中提取用户ID
 * @param req HTTP请求对象
 * @return 用户ID，未认证时返回空字符串
 */
// SEC-1: 安全地从 attributes 获取 user_id（由认证中间件注入），避免直接读取可伪造的 Header
static std::string extractUserId(const HttpRequestPtr& req) {
    try {
        return req->getAttributes()->get<std::string>("user_id");
    } catch (...) {
        return "";
    }
}

// ==================== 好友请求相关 ====================

void FriendController::sendFriendRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
        return;
    }

    if (!json->isMember("user_id")) {
        callback(ResponseUtil::badRequest("缺少user_id字段"));
        return;
    }

    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    std::string targetUserId = (*json)["user_id"].asString();
    std::string message = json->isMember("message") ? (*json)["message"].asString() : "";
    if (message.size() > 500) {
        callback(ResponseUtil::badRequest("message长度不能超过500"));
        return;
    }

    auto service = getFriendService();
    if (!service) {
        callback(ResponseUtil::internalError("服务未初始化"));
        return;
    }

    drogon::async_run([service, userId, targetUserId, message, callback]() -> drogon::Task<void> {
        try {
            auto result = co_await service->sendFriendRequestAsync(userId, targetUserId, message);

            // 广播好友请求事件给目标用户
            Json::Value broadcastMsg;
            broadcastMsg["type"] = "friend_request";
            broadcastMsg["from_user_id"] = userId;
            broadcastMsg["message"] = message;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToUser(targetUserId, broadcastMsg);

            callback(ResponseUtil::success(result, "好友请求已发送"));
        } catch (const std::runtime_error& e) {
            LOG_ERROR << "Error in sendFriendRequest: " << e.what();
            callback(ResponseUtil::error(400, "好友请求发送失败"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Unexpected error in sendFriendRequest: " << e.what();
            callback(ResponseUtil::internalError("发送好友请求失败"));
        }
    });
}

void FriendController::acceptFriendRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& userId
) {
    std::string currentUserId = extractUserId(req);
    if (currentUserId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }

    auto service = getFriendService();
    if (!service) {
        callback(ResponseUtil::internalError("服务未初始化"));
        return;
    }

    drogon::async_run([service, currentUserId, userId, callback]() -> drogon::Task<void> {
        try {
            auto result = co_await service->acceptFriendRequestAsync(currentUserId, userId);

            // 广播好友接受事件给请求发起者
            Json::Value broadcastMsg;
            broadcastMsg["type"] = "friend_accepted";
            broadcastMsg["friendship_id"] = result.isMember("friendship_id") ? result["friendship_id"].asString() : userId;
            broadcastMsg["from_user_id"] = currentUserId;
            broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToUser(userId, broadcastMsg);

            callback(ResponseUtil::success(result, "已接受好友请求"));
        } catch (const std::runtime_error& e) {
            LOG_ERROR << "Error in acceptFriendRequest: " << e.what();
            callback(ResponseUtil::error(400, "接受好友请求失败"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Unexpected error in acceptFriendRequest: " << e.what();
            callback(ResponseUtil::internalError("接受好友请求失败"));
        }
    });
}

void FriendController::rejectFriendRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& userId
) {
    std::string currentUserId = extractUserId(req);
    if (currentUserId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }

    auto service = getFriendService();
    if (!service) {
        callback(ResponseUtil::internalError("服务未初始化"));
        return;
    }

    drogon::async_run([service, currentUserId, userId, callback]() -> drogon::Task<void> {
        try {
            auto result = co_await service->rejectFriendRequestAsync(currentUserId, userId);
            callback(ResponseUtil::success(result, "已拒绝好友请求"));
        } catch (const std::runtime_error& e) {
            LOG_ERROR << "Error in rejectFriendRequest: " << e.what();
            callback(ResponseUtil::error(400, "拒绝好友请求失败"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Unexpected error in rejectFriendRequest: " << e.what();
            callback(ResponseUtil::internalError("拒绝好友请求失败"));
        }
    });
}

void FriendController::removeFriend(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& friendId
) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }

    auto service = getFriendService();
    if (!service) {
        callback(ResponseUtil::internalError("服务未初始化"));
        return;
    }

    drogon::async_run([service, userId, friendId, callback]() -> drogon::Task<void> {
        try {
            auto result = co_await service->removeFriendAsync(userId, friendId);
            callback(ResponseUtil::success(result, "已删除好友"));
        } catch (const std::runtime_error& e) {
            LOG_ERROR << "Error in removeFriend: " << e.what();
            callback(ResponseUtil::error(400, "删除好友失败"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Unexpected error in removeFriend: " << e.what();
            callback(ResponseUtil::internalError("删除好友失败"));
        }
    });
}

// ==================== 好友列表相关 ====================

void FriendController::getFriends(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }

    auto service = getFriendService();
    if (!service) {
        callback(ResponseUtil::internalError("服务未初始化"));
        return;
    }

    drogon::async_run([service, userId, callback]() -> drogon::Task<void> {
        try {
            auto result = co_await service->getFriendsListAsync(userId);
            callback(ResponseUtil::success(result));
        } catch (const std::exception& e) {
            LOG_ERROR << "Error in getFriends: " << e.what();
            callback(ResponseUtil::internalError("获取好友列表失败"));
        }
    });
}

void FriendController::getPendingRequests(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }

    auto service = getFriendService();
    if (!service) {
        callback(ResponseUtil::internalError("服务未初始化"));
        return;
    }

    drogon::async_run([service, userId, callback]() -> drogon::Task<void> {
        try {
            auto result = co_await service->getReceivedRequestsAsync(userId);
            callback(ResponseUtil::success(result));
        } catch (const std::exception& e) {
            LOG_ERROR << "Error in getPendingRequests: " << e.what();
            callback(ResponseUtil::internalError("获取待处理请求失败"));
        }
    });
}

// ==================== 好友消息相关 ====================

void FriendController::sendMessage(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& friendId
) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("content")) {
        callback(ResponseUtil::badRequest("缺少content字段"));
        return;
    }

    std::string content = (*json)["content"].asString();
    if (content.empty() || content.size() > 5000) {
        callback(ResponseUtil::badRequest("content长度必须在1-5000之间"));
        return;
    }

    // BUG-7 修复：将嵌套回调改为协程模式，避免回调嵌套导致的连接池竞争和潜在死锁
    drogon::async_run([userId, friendId, content, callback]() -> drogon::Task<void> {
        try {
            auto db = drogon::app().getDbClient("default");
            // 验证好友关系存在
            auto r = co_await db->execSqlCoro(
                "SELECT 1 FROM friends WHERE ((user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)) AND status = 'accepted' LIMIT 1",
                userId, friendId
            );
            if (r.empty()) {
                callback(ResponseUtil::forbidden("你们还不是好友，无法发送消息"));
                co_return;
            }
            // 插入消息
            co_await db->execSqlCoro(
                "INSERT INTO friend_messages (sender_id, receiver_id, content, created_at) VALUES ($1, $2, $3, NOW())",
                userId, friendId, content
            );

            // 推送实时消息给接收方
            Json::Value wsMsg;
            wsMsg["type"] = "new_friend_message";
            wsMsg["sender_id"] = userId;
            wsMsg["receiver_id"] = friendId;
            wsMsg["content"] = content;
            wsMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToUser(friendId, wsMsg);

            callback(ResponseUtil::success("消息已发送"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Error in sendMessage: " << e.what();
            callback(ResponseUtil::internalError("发送消息失败"));
        }
    });
}

void FriendController::getMessages(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& friendId
) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }

    // BUG-7 修复：将嵌套回调改为协程模式，避免回调嵌套导致的连接池竞争和潜在死锁
    drogon::async_run([userId, friendId, callback]() -> drogon::Task<void> {
        try {
            auto db = drogon::app().getDbClient("default");
            // 验证好友关系存在
            auto fr = co_await db->execSqlCoro(
                "SELECT 1 FROM friends WHERE ((user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)) AND status = 'accepted' LIMIT 1",
                userId, friendId
            );
            if (fr.empty()) {
                callback(ResponseUtil::forbidden("你们还不是好友，无法查看消息"));
                co_return;
            }
            // SEC-04: 查询消息记录 — 添加 LIMIT 防止海量数据拖垮服务
            auto r = co_await db->execSqlCoro(
                "SELECT id, sender_id, receiver_id, content, created_at FROM friend_messages "
                "WHERE (sender_id = $1 AND receiver_id = $2) OR (sender_id = $2 AND receiver_id = $1) "
                "ORDER BY created_at ASC LIMIT 200",
                userId, friendId
            );
            Json::Value messages(Json::arrayValue);
            for (const auto& row : r) {
                Json::Value msg;
                msg["id"] = row["id"].as<int64_t>();
                msg["sender_id"] = row["sender_id"].as<std::string>();
                msg["receiver_id"] = row["receiver_id"].as<std::string>();
                msg["content"] = row["content"].as<std::string>();
                msg["created_at"] = row["created_at"].as<std::string>();
                messages.append(msg);
            }
            callback(ResponseUtil::success(messages));
        } catch (const std::exception& e) {
            LOG_ERROR << "Error in getMessages: " << e.what();
            callback(ResponseUtil::internalError("获取消息失败"));
        }
    });
}
