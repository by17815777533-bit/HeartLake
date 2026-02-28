/**
 * @brief 进程内 LRU 缓存管理器
 *
 * @details
 * 用于热点数据的本地缓存，减少对 Redis / DB 的访问压力。
 * 内部维护一条 LRU 双向链表 + unordered_map 索引，O(1) 读写；
 * 读写均在 shared_mutex 保护下完成（读共享、写独占），适合读多写少场景。
 * 每条缓存条目带有 TTL，过期后惰性淘汰——get 时检查过期并移除，
 * 同时在容量达到上限时按 LRU 策略驱逐最久未访问的条目。
 *
 * 与 RedisCache 的关系：CacheManager 是纯进程内的 L1 缓存，
 * RedisCache 是分布式 L2 缓存。业务层通常先查 CacheManager，
 * 未命中再走 RedisCache / DB，形成两级缓存体系。
 */

#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <chrono>
#include <optional>
#include <json/json.h>

namespace heartlake::core::cache {

class CacheManager {
public:
    static CacheManager& getInstance() {
        static CacheManager instance;
        return instance;
    }

    /// 设置缓存容量上限，超出后按 LRU 策略淘汰
    void setMaxSize(size_t maxSize);

    /**
     * @brief 写入字符串缓存
     * @param key 缓存键
     * @param value 缓存值
     * @param ttlSeconds 过期时间（秒），默认 300s
     */
    void set(const std::string& key, const std::string& value, int ttlSeconds = 300);

    /**
     * @brief 写入 JSON 缓存（内部序列化为字符串存储）
     * @param key 缓存键
     * @param value JSON 对象
     * @param ttlSeconds 过期时间（秒），默认 300s
     */
    void setJson(const std::string& key, const Json::Value& value, int ttlSeconds = 300);

    /**
     * @brief 读取字符串缓存
     * @param key 缓存键
     * @return 命中返回值，未命中或已过期返回 std::nullopt
     */
    std::optional<std::string> get(const std::string& key);

    /**
     * @brief 读取 JSON 缓存
     * @param key 缓存键
     * @return 命中返回反序列化后的 JSON，未命中返回 std::nullopt
     */
    std::optional<Json::Value> getJson(const std::string& key);

    /// 精确删除指定 key 的缓存
    void invalidate(const std::string& key);

    /**
     * @brief 按前缀模式批量失效缓存
     * @param pattern 前缀字符串，匹配所有以此开头的 key
     * @note 遍历全量 key，大规模缓存下慎用
     */
    void invalidatePattern(const std::string& pattern);

    /// 清空全部缓存
    void clear();

    /// 返回当前缓存条目数
    size_t size() const;

private:
    CacheManager() = default;

    /// 容量满时驱逐 LRU 尾部条目
    void evictIfNeeded();

    /// 单条缓存条目：值 + 过期时间点 + LRU 链表迭代器
    struct CacheEntry {
        std::string value;
        std::chrono::steady_clock::time_point expiry;
        std::list<std::string>::iterator lruIt;
    };

    std::unordered_map<std::string, CacheEntry> cache_;  ///< key -> 缓存条目的哈希索引
    std::list<std::string> lruList_;                      ///< LRU 双向链表，头部为最近访问
    mutable std::shared_mutex mutex_;                     ///< 读写锁：get 共享，set/invalidate 独占
    size_t maxSize_ = 10000;                              ///< 默认最大 10000 条
};

} // namespace heartlake::core::cache
