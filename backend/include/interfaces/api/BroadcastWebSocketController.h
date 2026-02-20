/**
 * @file BroadcastWebSocketController.h
 * @brief WebSocket广播控制器 - 集成WebSocketHub实现实时通信
 */
#pragma once

#include <drogon/WebSocketController.h>
#include "infrastructure/realtime/WebSocketHub.h"

namespace heartlake::controllers {

class BroadcastWebSocketController : public drogon::WebSocketController<BroadcastWebSocketController> {
public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr&, std::string&&, const drogon::WebSocketMessageType&) override;
    void handleNewConnection(const drogon::HttpRequestPtr&, const drogon::WebSocketConnectionPtr&) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr&) override;

    static void broadcast(const Json::Value& message);
    static void sendToUser(const std::string& userId, const Json::Value& message);
    static void sendToRoom(const std::string& room, const Json::Value& message);
    static void startHeartbeatTimer();

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/broadcast", drogon::Get);
    WS_PATH_LIST_END
};

} // namespace heartlake::controllers
