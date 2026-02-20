/**
 * @file BroadcastWebSocketController.cpp
 * @brief WebSocket广播控制器实现 - 集成WebSocketHub
 */
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/PasetoUtil.h"

namespace heartlake::controllers {

using Hub = realtime::WebSocketHub;

void BroadcastWebSocketController::handleNewConnection(
    const drogon::HttpRequestPtr& req,
    const drogon::WebSocketConnectionPtr& conn
) {
    std::string token = req->getParameter("token");

    // VUL-02 修复：token 验证失败时拒绝 WebSocket 连接
    if (token.empty()) {
        LOG_WARN << "WebSocket 连接被拒绝：缺少 token";
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

    // VUL-02 增强：获取已认证的用户ID，用于房间级别权限控制
    auto ctx = conn->getContext<std::string>();
    std::string connUserId = ctx ? *ctx : "";

    if (msgType == "ping") {
        Json::Value pong;
        pong["type"] = "pong";
        conn->send(Json::FastWriter().write(pong));
    } else if (msgType == "join") {
        std::string room = json["room"].asString();
        // VUL-02: 私有房间（以 "private:" 开头）需要验证用户身份
        if (room.find("private:") == 0 && connUserId.empty()) {
            Json::Value err;
            err["type"] = "error";
            err["message"] = "认证用户才能加入私有房间";
            conn->send(Json::FastWriter().write(err));
            return;
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
