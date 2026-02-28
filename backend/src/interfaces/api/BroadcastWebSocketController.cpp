/**
 * @file BroadcastWebSocketController.cpp
 * @brief WebSocket 广播控制器 — PASETO 鉴权、房间管理、消息路由
 *
 * 基于 WebSocketHub 实现实时通信：连接建立时通过 URL token 或首包 auth
 * 完成 PASETO v4 身份验证（5 秒超时自动断开），支持 join/leave 房间、
 * 私有房间权限校验（private:{uid1}_{uid2}）、64KB 消息体限制、
 * 心跳保活（30s 间隔 / 90s 超时）。静态方法 broadcast / sendToUser /
 * sendToRoom 供其他 Controller 调用以推送实时事件。
 */
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/PasetoUtil.h"
#include <sstream>

namespace heartlake::controllers {

using Hub = realtime::WebSocketHub;

void BroadcastWebSocketController::handleNewConnection(
    const drogon::HttpRequestPtr& req,
    const drogon::WebSocketConnectionPtr& conn
) {
    std::string token = req->getParameter("token");

    // 兼容旧客户端：允许首包 auth 认证；URL token 仍为优先鉴权路径
    if (token.empty()) {
        conn->setContext(std::make_shared<std::string>(""));
        auto weakConn = std::weak_ptr<drogon::WebSocketConnection>(conn);
        drogon::app().getLoop()->runAfter(5.0, [weakConn]() {
            if (auto locked = weakConn.lock()) {
                auto ctx = locked->getContext<std::string>();
                if (!ctx || ctx->empty()) {
                    locked->shutdown(drogon::CloseCode::kViolation, "认证超时");
                }
            }
        });
        return;
    }

    std::string userId;
    try {
        userId = utils::PasetoUtil::verifyToken(token, utils::PasetoUtil::getKey());
    } catch (const std::exception& e) {
        LOG_WARN << "WebSocket 连接被拒绝：token 验证失败 - " << e.what();
        conn->shutdown(drogon::CloseCode::kViolation, "认证失败：无效 token");
        return;
    }

    if (userId.empty()) {
        LOG_WARN << "WebSocket 连接被拒绝：token 解析出空用户ID";
        conn->shutdown(drogon::CloseCode::kViolation, "认证失败：无效用户");
        return;
    }

    Hub::getInstance().addConnection(conn, userId);
    conn->setContext(std::make_shared<std::string>(userId));
}

void BroadcastWebSocketController::handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) {
    try {
        Hub::getInstance().removeConnection(conn);
    } catch (const std::exception& e) {
        LOG_ERROR << "Error handling connection close: " << e.what();
    }
}

void BroadcastWebSocketController::handleNewMessage(
    const drogon::WebSocketConnectionPtr& conn,
    std::string&& message,
    const drogon::WebSocketMessageType& type
) {
    try {
    // 限制单条消息大小，防止恶意客户端发送超大报文耗尽内存
    static constexpr size_t MAX_WS_MESSAGE_SIZE = 64 * 1024;  // 64KB
    if (message.size() > MAX_WS_MESSAGE_SIZE) {
        conn->shutdown(drogon::CloseCode::kViolation, "message too large");
        return;
    }

    Hub::getInstance().updatePing(conn);

    if (type == drogon::WebSocketMessageType::Ping) {
        conn->send("", drogon::WebSocketMessageType::Pong);
        return;
    }

    if (type != drogon::WebSocketMessageType::Text) return;

    Json::Value json;
    Json::Reader reader;
    if (!reader.parse(message, json)) return;

    const std::string msgType = json["type"].asString();

    // 获取已认证的用户ID，用于房间级别权限控制
    auto ctx = conn->getContext<std::string>();
    std::string connUserId = ctx ? *ctx : "";

    if (msgType == "auth") {
        if (!connUserId.empty()) {
            return;
        }
        const std::string token = json["token"].asString();
        if (token.empty()) {
            conn->shutdown(drogon::CloseCode::kViolation, "认证失败：缺少 token");
            return;
        }
        try {
            connUserId = utils::PasetoUtil::verifyToken(token, utils::PasetoUtil::getKey());
        } catch (const std::exception& e) {
            LOG_WARN << "WebSocket 首包认证失败：token 验证失败 - " << e.what();
            conn->shutdown(drogon::CloseCode::kViolation, "认证失败：无效 token");
            return;
        }
        if (connUserId.empty()) {
            conn->shutdown(drogon::CloseCode::kViolation, "认证失败：无效用户");
            return;
        }
        Hub::getInstance().addConnection(conn, connUserId);
        conn->setContext(std::make_shared<std::string>(connUserId));
        return;
    }

    if (msgType == "ping") {
        Json::Value pong;
        pong["type"] = "pong";
        conn->send(Json::FastWriter().write(pong));
        return;
    }

    // 仅允许已认证连接处理业务消息
    if (connUserId.empty()) {
        conn->shutdown(drogon::CloseCode::kViolation, "认证失败：请先发送 auth");
        return;
    }

    if (msgType == "join") {
        std::string room = json["room"].asString();
        // 私有房间权限验证
        if (room.find("private:") == 0) {
            if (connUserId.empty()) {
                Json::Value err;
                err["type"] = "error";
                err["message"] = "认证用户才能加入私有房间";
                conn->send(Json::FastWriter().write(err));
                return;
            }
            // 私有房间格式: private:{userId1}_{userId2}
            // 验证当前用户是参与方之一
            std::string roomBody = room.substr(8); // 去掉 "private:" 前缀
            // 按 '_' 分割参与者ID，逐个精确匹配
            bool authorized = false;
            std::istringstream ss(roomBody);
            std::string participant;
            while (std::getline(ss, participant, '_')) {
                if (participant == connUserId) {
                    authorized = true;
                    break;
                }
            }
            if (!authorized) {
                Json::Value err;
                err["type"] = "error";
                err["message"] = "无权加入此私有房间";
                conn->send(Json::FastWriter().write(err));
                return;
            }
        }
        Hub::getInstance().joinRoom(conn, room);
    } else if (msgType == "leave") {
        Hub::getInstance().leaveRoom(conn, json["room"].asString());
    } else if (msgType == "room_message") {
        std::string room = json["room"].asString();
        // VUL-02: 未认证用户不能在私有房间发送消息
        if (room.find("private:") == 0 && connUserId.empty()) {
            Json::Value err;
            err["type"] = "error";
            err["message"] = "认证用户才能在私有房间发送消息";
            conn->send(Json::FastWriter().write(err));
            return;
        }
        Hub::getInstance().sendToRoom(room, message, conn);
    }
    } catch (const std::exception& e) {
        LOG_ERROR << "Error handling WebSocket message: " << e.what();
    }
}

void BroadcastWebSocketController::broadcast(const Json::Value& message) {
    Hub::getInstance().broadcast(Json::FastWriter().write(message));
}

void BroadcastWebSocketController::sendToUser(const std::string& userId, const Json::Value& message) {
    Hub::getInstance().sendToUser(userId, Json::FastWriter().write(message));
}

void BroadcastWebSocketController::sendToRoom(const std::string& room, const Json::Value& message) {
    Hub::getInstance().sendToRoom(room, Json::FastWriter().write(message));
}

void BroadcastWebSocketController::startHeartbeatTimer() {
    Hub::getInstance().startHeartbeat(30, 90);
}

} // namespace heartlake::controllers
