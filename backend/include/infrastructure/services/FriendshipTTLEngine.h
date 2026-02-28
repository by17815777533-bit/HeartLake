/**
 * @brief 石友关系 TTL 引擎 -- 基于 Redis 键过期实现 24h 好友关系自动到期
 *
 * @details
 * HeartLake 的好友关系是"石友"——通过石头互动建立的临时关系，
 * 默认有效期 24 小时。本引擎利用 Redis 的 keyspace notification
 * 监听键过期事件，在好友关系到期时自动执行清理和通知。
 *
 * 数据模型：
 * - TTL 键：heartlake:friendship:ttl:{friendshipId}，设置 EXPIRE
 * - 数据键：heartlake:friendship:data:{friendshipId}，存储 userId/friendId 映射
 *
 * 过期流程：Redis 键过期 -> listenerThread_ 收到通知 ->
 * processExpiredKey 解析 friendshipId -> 更新数据库状态 ->
 * 通过 WebSocket 通知双方 -> 触发 expirationCallback_
 *
 * 支持 C++20 协程接口，与 Drogon 协程调度器无缝配合。
 * 提供批量 TTL 查询接口，避免好友列表页面的 N+1 查询问题。
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <drogon/drogon.h>

namespace heartlake::infrastructure {

class FriendshipTTLEngine {
public:
    static FriendshipTTLEngine& getInstance();

    /// 初始化 Redis 订阅，开始监听键过期事件
    void initialize();
    /// 停止监听线程并清理资源
    void shutdown();

    /**
     * @brief 创建带 TTL 的好友关系
     * @param friendshipId 好友关系唯一标识
     * @param userId 用户 A
     * @param friendId 用户 B
     * @param ttlSeconds 有效期（秒），默认 86400（24h）
     */
    drogon::Task<void> createFriendshipWithTTL(
        const std::string& friendshipId,
        const std::string& userId,
        const std::string& friendId,
        int64_t ttlSeconds = 86400
    );

    /**
     * @brief 刷新好友关系的 TTL（互动续期）
     * @param friendshipId 好友关系 ID
     * @param ttlSeconds 新的有效期（秒）
     * @return 刷新是否成功（关系不存在时返回 false）
     */
    drogon::Task<bool> refreshFriendshipTTL(
        const std::string& friendshipId,
        int64_t ttlSeconds = 86400
    );

    /// 查询单个好友关系的剩余 TTL（秒），-2 表示不存在
    drogon::Task<int64_t> getFriendshipTTL(const std::string& friendshipId);

    /**
     * @brief 批量查询多个好友关系的剩余 TTL
     * @param friendshipIds 好友关系 ID 列表
     * @return TTL 列表，与输入顺序一一对应
     * @note 使用 Redis pipeline 避免 N+1 查询
     */
    drogon::Task<std::vector<int64_t>> getBatchFriendshipTTL(const std::vector<std::string>& friendshipIds);

    /// 手动触发过期处理（用于测试）
    drogon::Task<void> handleFriendshipExpired(const std::string& friendshipId);

    /// 过期回调函数签名
    using ExpirationCallback = std::function<void(const std::string& friendshipId,
                                                   const std::string& userId,
                                                   const std::string& friendId)>;
    /// 设置好友关系过期时的回调
    void setExpirationCallback(ExpirationCallback callback);

private:
    FriendshipTTLEngine() = default;
    ~FriendshipTTLEngine();

    /// 启动 Redis keyspace notification 监听线程
    void startExpirationListener();
    /// 解析过期键并触发清理流程
    void processExpiredKey(const std::string& key);
    /// 通过 WebSocket 通知双方好友关系已结束
    void notifyFriendshipEnded(const std::string& userId, const std::string& friendId);

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> listenerThread_;
    ExpirationCallback expirationCallback_;
    std::mutex callbackMutex_;  ///< 保护 expirationCallback_ 的并发读写

    static constexpr const char* FRIENDSHIP_TTL_PREFIX = "heartlake:friendship:ttl:";
    static constexpr const char* FRIENDSHIP_DATA_PREFIX = "heartlake:friendship:data:";
};

} // namespace heartlake::infrastructure
