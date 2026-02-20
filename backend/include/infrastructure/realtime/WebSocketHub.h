/**
 * @file WebSocketHub.h
 * @brief WebSocket实时通信中心 - 房间管理、心跳检测、消息广播
 */
#pragma once

#include <drogon/WebSocketController.h>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <chrono>
#include <functional>

namespace heartlake::realtime {

struct ConnectionInfo {
    std::string userId;
    std::unordered_set<std::string> rooms;
    std::chrono::steady_clock::time_point lastPing;
    bool supportsCompression = false;
};

class WebSocketHub {
public:
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
        std::shared_lock lock(mutex_);
        if (auto it = userConnections_.find(userId); it != userConnections_.end()) {
            for (const auto& conn : it->second) {
                conn->send(message);
            }
        }
    }

    void sendToRoom(const std::string& room, const std::string& message,
                    const drogon::WebSocketConnectionPtr& exclude = nullptr) {
        std::shared_lock lock(mutex_);
        if (auto it = rooms_.find(room); it != rooms_.end()) {
            for (const auto& conn : it->second) {
                if (conn != exclude) conn->send(message);
            }
        }
    }

    void broadcast(const std::string& message) {
        std::shared_lock lock(mutex_);
        for (const auto& [conn, _] : connections_) {
            conn->send(message);
        }
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
                conn->send(msg);
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

    mutable std::shared_mutex mutex_;
    std::unordered_map<drogon::WebSocketConnectionPtr, ConnectionInfo> connections_;
    std::unordered_map<std::string, std::unordered_set<drogon::WebSocketConnectionPtr>> userConnections_;
    std::unordered_map<std::string, std::unordered_set<drogon::WebSocketConnectionPtr>> rooms_;
};

} // namespace heartlake::realtime
