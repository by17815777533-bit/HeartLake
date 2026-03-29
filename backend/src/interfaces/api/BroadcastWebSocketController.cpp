/**
 * @file BroadcastWebSocketController.cpp
 * @brief WebSocket 广播控制器 — PASETO 鉴权、房间管理、消息路由
 *
 * 基于 WebSocketHub 实现实时通信：连接建立时通过 URL token
 * 完成 PASETO v4 身份验证，支持 join/leave 房间、
 * 私有房间权限校验（private:{uid1}_{uid2}）、64KB 消息体限制、
 * 心跳保活（30s 间隔 / 90s 超时）。静态方法 broadcast / sendToUser /
 * sendToRoom 供其他 Controller 调用以推送实时事件。
 */
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/PasetoUtil.h"
#include "utils/RealtimeEvent.h"
#include <sstream>

namespace heartlake::controllers {

using Hub = realtime::WebSocketHub;

namespace {
void sendAuthSuccess(const drogon::WebSocketConnectionPtr &conn,
                     const std::string &userId) {
    Json::Value ok;
    ok["type"] = "auth_success";
    ok["user_id"] = userId;
    ok["authenticated"] = true;
    ok["timestamp"] = static_cast<Json::Int64>(time(nullptr));
    conn->send(Json::FastWriter().write(ok));
}

void sendWsError(const drogon::WebSocketConnectionPtr &conn,
                 const std::string &message) {
    Json::Value err;
    err["type"] = "error";
    err["message"] = message;
    err["timestamp"] = static_cast<Json::Int64>(time(nullptr));
    conn->send(Json::FastWriter().write(err));
}

bool isPrivateRoomAuthorized(const std::string &room,
                             const std::string &userId) {
    if (room.find("private:") != 0) {
        return true;
    }
    if (userId.empty()) {
        return false;
    }

    std::string roomBody = room.substr(8);
    std::istringstream ss(roomBody);
    std::string participant;
    while (std::getline(ss, participant, '_')) {
        if (participant == userId) {
            return true;
        }
    }
    return false;
}
} // namespace

void BroadcastWebSocketController::handleNewConnection(
    const drogon::HttpRequestPtr& req,
    const drogon::WebSocketConnectionPtr& conn
) {
    std::string token = req->getParameter("token");

    if (token.empty()) {
        conn->shutdown(drogon::CloseCode::kViolation, "认证失败：缺少 token");
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
    sendAuthSuccess(conn, userId);
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

    if (msgType == "ping") {
        Json::Value pong;
        pong["type"] = "pong";
        conn->send(Json::FastWriter().write(pong));
        return;
    }

    // 仅允许已认证连接处理业务消息
    if (connUserId.empty()) {
        conn->shutdown(drogon::CloseCode::kViolation, "认证失败：无效连接上下文");
        return;
    }

    if (msgType == "join") {
        std::string room = json["room"].asString();
        if (room.empty()) {
            sendWsError(conn, "room 不能为空");
            return;
        }
        if (!isPrivateRoomAuthorized(room, connUserId)) {
            sendWsError(conn, "无权加入此私有房间");
            return;
        }
        Hub::getInstance().joinRoom(conn, room);
    } else if (msgType == "leave") {
        Hub::getInstance().leaveRoom(conn, json["room"].asString());
    } else if (msgType == "room_message") {
        std::string room = json["room"].asString();
        if (room.empty()) {
            sendWsError(conn, "room 不能为空");
            return;
        }
        if (!isPrivateRoomAuthorized(room, connUserId)) {
            sendWsError(conn, "无权在此私有房间发送消息");
            return;
        }
        if (!Hub::getInstance().isInRoom(conn, room)) {
            sendWsError(conn, "请先加入房间");
            return;
        }

        Json::Value outbound;
        outbound["type"] = "room_message";
        outbound["room"] = room;
        outbound["sender_id"] = connUserId;
        outbound["senderId"] = connUserId;
        outbound["timestamp"] = static_cast<Json::Int64>(time(nullptr));
        if (json.isMember("content")) {
            outbound["content"] = json["content"];
        } else if (json.isMember("message")) {
            outbound["content"] = json["message"];
        }
        if (json.isMember("payload")) {
            outbound["payload"] = json["payload"];
        }
        if (json.isMember("client_message_id")) {
            outbound["client_message_id"] = json["client_message_id"];
        }

        Hub::getInstance().sendToRoom(room, Json::FastWriter().write(outbound), conn);
    }
    } catch (const std::exception& e) {
        LOG_ERROR << "Error handling WebSocket message: " << e.what();
    }
}

void BroadcastWebSocketController::broadcast(const Json::Value& message) {
    auto payload = message;
    if (payload.isObject() && payload.isMember("type")) {
        payload = heartlake::utils::buildRealtimeEvent(
            payload["type"].asString(), std::move(payload));
    }
    Hub::getInstance().broadcast(Json::FastWriter().write(payload));
}

void BroadcastWebSocketController::sendToUser(const std::string& userId, const Json::Value& message) {
    auto payload = message;
    if (payload.isObject() && payload.isMember("type")) {
        payload = heartlake::utils::buildRealtimeEvent(
            payload["type"].asString(), std::move(payload));
    }
    Hub::getInstance().sendToUser(userId, Json::FastWriter().write(payload));
}

void BroadcastWebSocketController::sendToRoom(const std::string& room, const Json::Value& message) {
    auto payload = message;
    if (payload.isObject() && payload.isMember("type")) {
        payload = heartlake::utils::buildRealtimeEvent(
            payload["type"].asString(), std::move(payload));
    }
    Hub::getInstance().sendToRoom(room, Json::FastWriter().write(payload));
}

void BroadcastWebSocketController::startHeartbeatTimer() {
    Hub::getInstance().startHeartbeat(30, 90);
}

} // namespace heartlake::controllers
