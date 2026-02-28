/**
 * WebSocket 广播控制器 - 实时消息推送与房间通信
 *
 * 基于 Drogon WebSocket 框架，集成 WebSocketHub 实现：
 * - 全局广播：向所有在线用户推送系统通知
 * - 定向推送：向指定用户推送个人消息（涟漪、纸船通知等）
 * - 房间通信：按房间分组推送（如湖面实时动态）
 * - 心跳保活：定时发送 ping 帧检测连接存活
 *
 * 客户端通过 ws://host/ws/broadcast 建立连接，
 * 连接时需在 query 参数或 header 中携带 PASETO 令牌。
 */
#pragma once

#include <drogon/WebSocketController.h>
#include "infrastructure/realtime/WebSocketHub.h"

namespace heartlake::controllers {

/**
 * @brief WebSocket 实时广播控制器
 *
 * @details 连接生命周期：
 * 1. handleNewConnection — 验证令牌，注册到 WebSocketHub
 * 2. handleNewMessage — 处理客户端上行消息（心跳响应、房间订阅等）
 * 3. handleConnectionClosed — 从 Hub 注销，清理资源
 *
 * 静态方法供其他 Controller 调用，实现业务事件的实时推送。
 */
class BroadcastWebSocketController : public drogon::WebSocketController<BroadcastWebSocketController> {
public:
    /**
     * @brief 处理客户端上行消息
     * @param conn WebSocket 连接
     * @param message 消息内容
     * @param type 消息类型（Text/Binary/Ping/Pong）
     */
    void handleNewMessage(const drogon::WebSocketConnectionPtr&, std::string&&, const drogon::WebSocketMessageType&) override;

    /**
     * @brief 处理新连接建立
     * @details 从请求中提取用户身份，注册到 WebSocketHub 的连接池
     * @param req 升级前的 HTTP 请求（含认证信息）
     * @param conn 新建立的 WebSocket 连接
     */
    void handleNewConnection(const drogon::HttpRequestPtr&, const drogon::WebSocketConnectionPtr&) override;

    /**
     * @brief 处理连接关闭
     * @details 从 WebSocketHub 注销连接，释放房间订阅
     * @param conn 已关闭的 WebSocket 连接
     */
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr&) override;

    /// 向所有在线用户广播 JSON 消息
    static void broadcast(const Json::Value& message);

    /// 向指定用户推送 JSON 消息（通过 userId 查找连接）
    static void sendToUser(const std::string& userId, const Json::Value& message);

    /// 向指定房间内的所有用户推送 JSON 消息
    static void sendToRoom(const std::string& room, const Json::Value& message);

    /// 启动心跳定时器，定期向所有连接发送 ping 帧
    static void startHeartbeatTimer();

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/broadcast", drogon::Get);
    WS_PATH_LIST_END
};

} // namespace heartlake::controllers
