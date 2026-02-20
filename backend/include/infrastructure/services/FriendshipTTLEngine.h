/**
 * @file FriendshipTTLEngine.h
 * @brief 石友关系TTL引擎 - 监听Redis键过期事件，处理24h好友关系到期
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <drogon/drogon.h>

namespace heartlake::infrastructure {

class FriendshipTTLEngine {
public:
    static FriendshipTTLEngine& getInstance();

    void initialize();
    void shutdown();

    // 创建带TTL的好友关系
    drogon::Task<void> createFriendshipWithTTL(
        const std::string& friendshipId,
        const std::string& userId,
        const std::string& friendId,
        int64_t ttlSeconds = 86400  // 24h
    );

    // 刷新好友关系TTL
    drogon::Task<bool> refreshFriendshipTTL(
        const std::string& friendshipId,
        int64_t ttlSeconds = 86400
    );

    // 获取剩余TTL
    drogon::Task<int64_t> getFriendshipTTL(const std::string& friendshipId);

    // BUG-6: 批量获取多个好友关系的剩余TTL，避免 N+1 查询导致超时
    drogon::Task<std::vector<int64_t>> getBatchFriendshipTTL(const std::vector<std::string>& friendshipIds);

    // 手动触发过期处理（用于测试）
    drogon::Task<void> handleFriendshipExpired(const std::string& friendshipId);

    using ExpirationCallback = std::function<void(const std::string& friendshipId,
                                                   const std::string& userId,
                                                   const std::string& friendId)>;
    void setExpirationCallback(ExpirationCallback callback);

private:
    FriendshipTTLEngine() = default;
    ~FriendshipTTLEngine();

    void startExpirationListener();
    void processExpiredKey(const std::string& key);
    void notifyFriendshipEnded(const std::string& userId, const std::string& friendId);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> listenerThread_;
    ExpirationCallback expirationCallback_;

    static constexpr const char* FRIENDSHIP_TTL_PREFIX = "heartlake:friendship:ttl:";
    static constexpr const char* FRIENDSHIP_DATA_PREFIX = "heartlake:friendship:data:";
};

} // namespace heartlake::infrastructure
