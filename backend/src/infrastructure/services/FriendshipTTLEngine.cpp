/**
 * @file FriendshipTTLEngine.cpp
 * @brief 石友关系TTL引擎实现
 */

#include "infrastructure/services/FriendshipTTLEngine.h"
#include "infrastructure/cache/RedisCache.h"
#include "infrastructure/services/NotificationPushService.h"
#include <json/json.h>

namespace heartlake::infrastructure {

FriendshipTTLEngine& FriendshipTTLEngine::getInstance() {
    static FriendshipTTLEngine instance;
    return instance;
}

FriendshipTTLEngine::~FriendshipTTLEngine() {
    shutdown();
}

void FriendshipTTLEngine::initialize() {
    if (running_.exchange(true)) return;
    startExpirationListener();
    LOG_INFO << "FriendshipTTLEngine initialized";
}

void FriendshipTTLEngine::shutdown() {
    running_ = false;
    if (listenerThread_ && listenerThread_->joinable()) {
        listenerThread_->join();
    }
}

drogon::Task<void> FriendshipTTLEngine::createFriendshipWithTTL(
    const std::string& friendshipId,
    const std::string& userId,
    const std::string& friendId,
    int64_t ttlSeconds
) {
    auto& redis = cache::RedisCache::getInstance();

    // Store friendship data
    Json::Value data;
    data["friendship_id"] = friendshipId;
    data["user_id"] = userId;
    data["friend_id"] = friendId;

    Json::StreamWriterBuilder writer;
    std::string dataStr = Json::writeString(writer, data);

    std::string dataKey = std::string(FRIENDSHIP_DATA_PREFIX) + friendshipId;
    std::string ttlKey = std::string(FRIENDSHIP_TTL_PREFIX) + friendshipId;

    co_await redis.setExCoro(dataKey, dataStr, static_cast<int>(ttlSeconds + 60));
    co_await redis.setExCoro(ttlKey, "1", static_cast<int>(ttlSeconds));

    co_return;
}

drogon::Task<bool> FriendshipTTLEngine::refreshFriendshipTTL(
    const std::string& friendshipId,
    int64_t ttlSeconds
) {
    auto& redis = cache::RedisCache::getInstance();
    std::string ttlKey = std::string(FRIENDSHIP_TTL_PREFIX) + friendshipId;
    std::string dataKey = std::string(FRIENDSHIP_DATA_PREFIX) + friendshipId;

    redis.expire(ttlKey, static_cast<int>(ttlSeconds), nullptr);
    redis.expire(dataKey, static_cast<int>(ttlSeconds + 60), nullptr);

    co_return true;
}

drogon::Task<int64_t> FriendshipTTLEngine::getFriendshipTTL(const std::string& friendshipId) {
    auto& redis = cache::RedisCache::getInstance();
    std::string ttlKey = std::string(FRIENDSHIP_TTL_PREFIX) + friendshipId;
    co_return co_await redis.ttlCoro(ttlKey);
}

// BUG-6: 批量获取多个好友关系的剩余TTL，避免 N+1 查询导致超时
drogon::Task<std::vector<int64_t>> FriendshipTTLEngine::getBatchFriendshipTTL(
    const std::vector<std::string>& friendshipIds
) {
    auto& redis = cache::RedisCache::getInstance();
    std::vector<int64_t> results;
    results.reserve(friendshipIds.size());

    // 并行发起所有 TTL 查询协程，避免逐个串行等待
    std::vector<drogon::Task<int64_t>> tasks;
    for (const auto& id : friendshipIds) {
        std::string ttlKey = std::string(FRIENDSHIP_TTL_PREFIX) + id;
        tasks.push_back(redis.ttlCoro(ttlKey));
    }

    // 依次收集结果（协程已并行启动，此处只是等待完成）
    for (auto& task : tasks) {
        try {
            auto ttl = co_await std::move(task);
            results.push_back(ttl);
        } catch (...) {
            // Redis 查询失败时返回 -1 表示未知
            results.push_back(-1);
        }
    }

    co_return results;
}

drogon::Task<void> FriendshipTTLEngine::handleFriendshipExpired(const std::string& friendshipId) {
    auto& redis = cache::RedisCache::getInstance();
    std::string dataKey = std::string(FRIENDSHIP_DATA_PREFIX) + friendshipId;

    auto [dataStr, exists] = co_await redis.getCoro(dataKey);
    if (!exists) co_return;

    Json::Value data;
    Json::CharReaderBuilder reader;
    std::istringstream stream(dataStr);
    Json::parseFromStream(reader, stream, &data, nullptr);

    std::string userId = data["user_id"].asString();
    std::string friendId = data["friend_id"].asString();

    // Delete from database
    auto db = drogon::app().getDbClient();
    co_await db->execSqlCoro(
        "DELETE FROM friends WHERE friendship_id = $1", friendshipId
    );

    // Notify both users
    notifyFriendshipEnded(userId, friendId);

    // Cleanup Redis
    redis.del(dataKey, nullptr);

    if (expirationCallback_) {
        expirationCallback_(friendshipId, userId, friendId);
    }
}

void FriendshipTTLEngine::setExpirationCallback(ExpirationCallback callback) {
    expirationCallback_ = std::move(callback);
}

void FriendshipTTLEngine::startExpirationListener() {
    // 使用轮询机制检查过期的临时好友关系
    listenerThread_ = std::make_unique<std::thread>([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            if (!running_) break;
            try {
                auto dbClient = drogon::app().getDbClient("default");
                auto expired = dbClient->execSqlSync(
                    "UPDATE temp_friends SET status = 'expired' "
                    "WHERE status = 'active' AND expires_at < NOW() "
                    "RETURNING temp_friend_id, user_id_1, user_id_2"
                );
                for (const auto& row : expired) {
                    std::string friendshipId = row["temp_friend_id"].as<std::string>();
                    processExpiredKey(friendshipId);
                    LOG_INFO << "Temp friendship expired: " << friendshipId;
                }
            } catch (const std::exception& e) {
                LOG_WARN << "TTL expiration check failed: " << e.what();
            }
        }
    });
}

void FriendshipTTLEngine::processExpiredKey(const std::string& friendshipId) {
    auto& redis = cache::RedisCache::getInstance();
    std::string lockKey = "heartlake:friendship:lock:" + friendshipId;

    // 使用 Redis Lua 脚本实现原子 SETNX + EXPIRE，避免 exists/setEx 竞态条件
    static const std::string luaScript =
        "if redis.call('exists', KEYS[1]) == 0 then "
        "  redis.call('setex', KEYS[1], ARGV[1], '1') "
        "  return 1 "
        "else "
        "  return 0 "
        "end";

    redis.eval(luaScript, {lockKey}, {"60"}, [this, friendshipId](int64_t acquired) {
        if (acquired == 0) return; // 其他实例已获取锁
        drogon::async_run([this, friendshipId]() -> drogon::Task<void> {
            co_await handleFriendshipExpired(friendshipId);
        });
    });
}

void FriendshipTTLEngine::notifyFriendshipEnded(const std::string& userId, const std::string& friendId) {
    services::NotificationPushService::getInstance().pushSystemNotice(userId, "缘尽", "临时好友关系已结束");
    services::NotificationPushService::getInstance().pushSystemNotice(friendId, "缘尽", "临时好友关系已结束");
}

} // namespace heartlake::infrastructure
