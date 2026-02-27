/**
 * WebSocket实时通信中心 - 房间管理、心跳检测、消息广播
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

struct ConnectionInfo {
    std::string userId;
    std::unordered_set<std::string> rooms;
    std::chrono::steady_clock::time_point lastPing;
    bool supportsCompression = false;
};

class WebSocketHub {
public:
    struct RealtimeEvent {
        uint64_t seq = 0;
        int64_t tsMs = 0;
        std::string scope;
        std::string target;
        std::string payload;
    };

    static WebSocketHub& getInstance() {
        static WebSocketHub instance;
        return instance;
    }

    // 连接管理
    void addConnection(const drogon::WebSocketConnectionPtr& conn, const std::string& userId) {
        std::unique_lock lock(mutex_);
        connections_[conn] = {userId, {}, std::chrono::steady_clock::now(), false};
        userConnections_[userId].insert(conn);
    }

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

    // 房间管理
    void joinRoom(const drogon::WebSocketConnectionPtr& conn, const std::string& room) {
        std::unique_lock lock(mutex_);
        if (auto it = connections_.find(conn); it != connections_.end()) {
            it->second.rooms.insert(room);
            rooms_[room].insert(conn);
        }
    }

    void leaveRoom(const drogon::WebSocketConnectionPtr& conn, const std::string& room) {
        std::unique_lock lock(mutex_);
        if (auto it = connections_.find(conn); it != connections_.end()) {
            it->second.rooms.erase(room);
            rooms_[room].erase(conn);
            if (rooms_[room].empty()) rooms_.erase(room);
        }
    }

    // 消息发送
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

    // 心跳处理
    void updatePing(const drogon::WebSocketConnectionPtr& conn) {
        std::unique_lock lock(mutex_);
        if (auto it = connections_.find(conn); it != connections_.end()) {
            it->second.lastPing = std::chrono::steady_clock::now();
        }
    }

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

    // 查询
    size_t getConnectionCount() const {
        std::shared_lock lock(mutex_);
        return connections_.size();
    }

    size_t getRoomSize(const std::string& room) const {
        std::shared_lock lock(mutex_);
        if (auto it = rooms_.find(room); it != rooms_.end()) {
            return it->second.size();
        }
        return 0;
    }

    bool isUserOnline(const std::string& userId) const {
        std::shared_lock lock(mutex_);
        return userConnections_.count(userId) > 0;
    }

private:
    WebSocketHub() = default;

    static int64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

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

    static constexpr size_t MAX_MESSAGE_SIZE = 64 * 1024;  // 单条消息上限 64KB

    mutable std::shared_mutex mutex_;
    std::unordered_map<drogon::WebSocketConnectionPtr, ConnectionInfo> connections_;
    std::unordered_map<std::string, std::unordered_set<drogon::WebSocketConnectionPtr>> userConnections_;
    std::unordered_map<std::string, std::unordered_set<drogon::WebSocketConnectionPtr>> rooms_;
    uint64_t nextEventSeq_ = 1;  // 仅在 unique_lock 下访问（appendEventLocked），线程安全
};

} // namespace heartlake::realtime
