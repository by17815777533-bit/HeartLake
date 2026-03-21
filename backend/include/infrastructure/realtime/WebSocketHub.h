/**
 * @brief WebSocket 实时通信中心 -- 房间管理、心跳检测、消息广播
 *
 * @details
 * 全局单例，管理所有 WebSocket 长连接的生命周期。核心能力：
 *
 * 1. 连接管理：维护 conn -> userId 的双向映射，支持同一用户多端在线
 * 2. 房间机制：用户可加入/离开多个房间，支持房间级消息广播
 * 3. 心跳检测：定时发送 ping 帧，超时未响应的连接自动踢出
 * 4. 消息投递：支持单用户推送、房间广播、全局广播三种模式
 * 5. 事件溯源：每条消息分配单调递增序号，通过 Redis Pub/Sub 发布，
 *    便于多实例间同步和客户端断线重连后的消息补偿
 *
 * 线程安全策略：使用 shared_mutex 实现读写分离——
 * 消息发送时先在读锁下拷贝连接列表，再在锁外逐个发送，
 * 避免持锁期间阻塞其他连接的加入/退出操作。
 *
 * 单条消息上限 64KB，超限直接丢弃并记录警告日志。
 */
#pragma once

#include <drogon/WebSocketController.h>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <chrono>
#include <functional>
#include <vector>
#include <ctime>
#include <json/json.h>

namespace heartlake::realtime {

/// 单条 WebSocket 连接的元信息
struct ConnectionInfo {
    std::string userId;                                  ///< 连接所属用户
    std::unordered_set<std::string> rooms;               ///< 已加入的房间集合
    std::chrono::steady_clock::time_point lastPing;      ///< 最近一次心跳时间
    bool supportsCompression = false;                    ///< 客户端是否支持压缩
};

class WebSocketHub {
public:
    /// 实时事件描述，每条消息发送后生成，用于 Redis Pub/Sub 广播
    struct RealtimeEvent {
        uint64_t seq = 0;       ///< 单调递增序号，用于客户端断线补偿
        int64_t tsMs = 0;       ///< 事件产生的毫秒时间戳
        std::string scope;      ///< 投递范围："user" / "room" / "broadcast"
        std::string target;     ///< 投递目标：userId 或 roomName
        std::string payload;    ///< 消息体 JSON 字符串
    };

    static WebSocketHub& getInstance() {
        static WebSocketHub instance;
        return instance;
    }

    // ---- 连接管理 ----

    /// 注册新连接，建立 conn <-> userId 双向映射
    void addConnection(const drogon::WebSocketConnectionPtr& conn, const std::string& userId) {
        std::unique_lock lock(mutex_);
        connections_[conn] = {userId, {}, std::chrono::steady_clock::now(), false};
        userConnections_[userId].insert(conn);
    }

    /// 移除连接，同时清理其所在的所有房间和用户映射
    void removeConnection(const drogon::WebSocketConnectionPtr& conn) {
        std::unique_lock lock(mutex_);
        auto it = connections_.find(conn);
        if (it == connections_.end()) return;

        for (const auto& room : it->second.rooms) {
            rooms_[room].erase(conn);
            if (rooms_[room].empty()) rooms_.erase(room);
        }
        userConnections_[it->second.userId].erase(conn);
        if (userConnections_[it->second.userId].empty()) {
            userConnections_.erase(it->second.userId);
        }
        connections_.erase(it);
    }

    // ---- 房间管理 ----

    /// 将连接加入指定房间
    void joinRoom(const drogon::WebSocketConnectionPtr& conn, const std::string& room) {
        std::unique_lock lock(mutex_);
        if (auto it = connections_.find(conn); it != connections_.end()) {
            it->second.rooms.insert(room);
            rooms_[room].insert(conn);
        }
    }

    /// 将连接从指定房间移除，房间为空时自动销毁
    void leaveRoom(const drogon::WebSocketConnectionPtr& conn, const std::string& room) {
        std::unique_lock lock(mutex_);
        if (auto it = connections_.find(conn); it != connections_.end()) {
            it->second.rooms.erase(room);
            rooms_[room].erase(conn);
            if (rooms_[room].empty()) rooms_.erase(room);
        }
    }

    // ---- 消息投递 ----

    /// 向指定用户的所有连接推送消息（多端同步）
    void sendToUser(const std::string& userId, const std::string& message) {
        if (message.size() > MAX_MESSAGE_SIZE) {
            LOG_WARN << "消息体超限(" << message.size() << " bytes)，丢弃";
            return;
        }
        // 先复制连接列表，锁外发送，避免持锁期间阻塞其他操作
        std::vector<drogon::WebSocketConnectionPtr> conns;
        {
            std::shared_lock lock(mutex_);
            if (auto it = userConnections_.find(userId); it != userConnections_.end()) {
                conns.assign(it->second.begin(), it->second.end());
            }
        }
        for (const auto& conn : conns) {
            try {
                conn->send(message);
            } catch (const std::exception& e) {
                LOG_WARN << "Failed to send to user " << userId << ": " << e.what();
            }
        }
        RealtimeEvent event;
        {
            std::unique_lock lock(mutex_);
            event = appendEventLocked("user", userId, message);
        }
        publishEventAsync(event);
    }

    /**
     * @brief 向房间内所有连接广播消息
     * @param room 房间名
     * @param message 消息体
     * @param exclude 排除的连接（通常是消息发送者自身），可为 nullptr
     */
    void sendToRoom(const std::string& room, const std::string& message,
                    const drogon::WebSocketConnectionPtr& exclude = nullptr) {
        if (message.size() > MAX_MESSAGE_SIZE) {
            LOG_WARN << "消息体超限(" << message.size() << " bytes)，丢弃";
            return;
        }
        std::vector<drogon::WebSocketConnectionPtr> conns;
        {
            std::shared_lock lock(mutex_);
            if (auto it = rooms_.find(room); it != rooms_.end()) {
                for (const auto& conn : it->second) {
                    if (conn != exclude) conns.push_back(conn);
                }
            }
        }
        for (const auto& conn : conns) {
            try {
                conn->send(message);
            } catch (const std::exception& e) {
                LOG_WARN << "Failed to send to room " << room << ": " << e.what();
            }
        }
        RealtimeEvent event;
        {
            std::unique_lock lock(mutex_);
            event = appendEventLocked("room", room, message);
        }
        publishEventAsync(event);
    }

    /// 向所有在线连接广播消息（全局推送，慎用）
    void broadcast(const std::string& message) {
        if (message.size() > MAX_MESSAGE_SIZE) {
            LOG_WARN << "消息体超限(" << message.size() << " bytes)，丢弃";
            return;
        }
        std::vector<drogon::WebSocketConnectionPtr> conns;
        {
            std::shared_lock lock(mutex_);
            conns.reserve(connections_.size());
            for (const auto& [conn, _] : connections_) {
                conns.push_back(conn);
            }
        }
        for (const auto& conn : conns) {
            try {
                conn->send(message);
            } catch (const std::exception& e) {
                LOG_WARN << "Failed to broadcast to connection: " << e.what();
            }
        }
        RealtimeEvent event;
        {
            std::unique_lock lock(mutex_);
            event = appendEventLocked("broadcast", "", message);
        }
        publishEventAsync(event);
    }

    // ---- 心跳检测 ----

    /// 更新连接的最近心跳时间（客户端发来 pong 时调用）
    void updatePing(const drogon::WebSocketConnectionPtr& conn) {
        std::unique_lock lock(mutex_);
        if (auto it = connections_.find(conn); it != connections_.end()) {
            it->second.lastPing = std::chrono::steady_clock::now();
        }
    }

    /// 扫描所有连接，强制关闭超过 timeoutSec 未心跳的连接
    void checkTimeouts(int timeoutSec = 90) {
        auto now = std::chrono::steady_clock::now();
        std::vector<drogon::WebSocketConnectionPtr> stale;
        {
            std::shared_lock lock(mutex_);
            for (const auto& [conn, info] : connections_) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - info.lastPing).count();
                if (elapsed > timeoutSec) stale.push_back(conn);
            }
        }
        for (const auto& conn : stale) {
            removeConnection(conn);
            conn->forceClose();
        }
    }

    /**
     * @brief 启动心跳定时器
     * @param intervalSec 心跳发送间隔（秒），默认 30s
     * @param timeoutSec 超时阈值（秒），默认 90s，超过则踢出连接
     */
    void startHeartbeat(int intervalSec = 30, int timeoutSec = 90) {
        drogon::app().getLoop()->runEvery(intervalSec, [this, timeoutSec]() {
            checkTimeouts(timeoutSec);
            std::shared_lock lock(mutex_);
            Json::Value ping;
            ping["type"] = "ping";
            ping["ts"] = static_cast<Json::Int64>(std::time(nullptr));
            auto msg = Json::FastWriter().write(ping);
            for (const auto& [conn, _] : connections_) {
                try {
                    conn->send(msg);
                } catch (const std::exception& e) {
                    LOG_WARN << "Failed to send heartbeat: " << e.what();
                }
            }
        });
    }

    // ---- 状态查询 ----

    /// 当前在线连接总数
    size_t getConnectionCount() const {
        std::shared_lock lock(mutex_);
        return connections_.size();
    }

    /// 指定房间的在线连接数
    size_t getRoomSize(const std::string& room) const {
        std::shared_lock lock(mutex_);
        if (auto it = rooms_.find(room); it != rooms_.end()) {
            return it->second.size();
        }
        return 0;
    }

    /// 判断指定用户是否有至少一个在线连接
    bool isUserOnline(const std::string& userId) const {
        std::shared_lock lock(mutex_);
        return userConnections_.count(userId) > 0;
    }

    /// 判断指定连接是否已加入某个房间
    bool isInRoom(const drogon::WebSocketConnectionPtr& conn,
                  const std::string& room) const {
        std::shared_lock lock(mutex_);
        if (auto it = connections_.find(conn); it != connections_.end()) {
            return it->second.rooms.count(room) > 0;
        }
        return false;
    }

private:
    WebSocketHub() = default;

    /// 获取当前毫秒时间戳
    static int64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    /// 在写锁保护下追加事件并分配序号（调用方须持有 unique_lock）
    RealtimeEvent appendEventLocked(const std::string& scope,
                                    const std::string& target,
                                    const std::string& payload) {
        RealtimeEvent event;
        event.seq = nextEventSeq_++;
        event.tsMs = nowMs();
        event.scope = scope;
        event.target = target;
        event.payload = payload;
        return event;
    }

    /// 将事件通过 Redis PUBLISH 异步广播到 heartlake:realtime 频道
    static void publishEventAsync(const RealtimeEvent& event) {
        try {
            Json::Value envelope;
            envelope["seq"] = static_cast<Json::UInt64>(event.seq);
            envelope["ts_ms"] = static_cast<Json::Int64>(event.tsMs);
            envelope["scope"] = event.scope;
            envelope["target"] = event.target;
            envelope["payload"] = event.payload;

            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            std::string message = Json::writeString(builder, envelope);

            // RedisClientImpl 在高并发跨线程 submit 时存在稳定性风险；
            // 统一切到主 loop 串行发布，避免回调对象并发竞争。
            auto* loop = drogon::app().getLoop();
            loop->queueInLoop([msg = std::move(message)]() {
                try {
                    auto redisClient = drogon::app().getRedisClient("default");
                    redisClient->execCommandAsync(
                        [](const drogon::nosql::RedisResult&) {},
                        [](const std::exception& e) {
                            LOG_WARN << "Realtime event publish failed: " << e.what();
                        },
                        "PUBLISH %s %s",
                        "heartlake:realtime",
                        msg.c_str()
                    );
                } catch (const std::exception& e) {
                    LOG_WARN << "Realtime publish skipped in loop: " << e.what();
                }
            });
        } catch (const std::exception& e) {
            LOG_WARN << "Realtime publish skipped: " << e.what();
        }
    }

    static constexpr size_t MAX_MESSAGE_SIZE = 64 * 1024;  ///< 单条消息上限 64KB

    mutable std::shared_mutex mutex_;  ///< 读写锁：查询/发送共享，连接增删独占
    std::unordered_map<drogon::WebSocketConnectionPtr, ConnectionInfo> connections_;  ///< conn -> 连接元信息
    std::unordered_map<std::string, std::unordered_set<drogon::WebSocketConnectionPtr>> userConnections_;  ///< userId -> 该用户的所有连接
    std::unordered_map<std::string, std::unordered_set<drogon::WebSocketConnectionPtr>> rooms_;  ///< roomName -> 房间内的所有连接
    uint64_t nextEventSeq_ = 1;  ///< 事件序号，仅在 unique_lock 下递增
};

} // namespace heartlake::realtime
