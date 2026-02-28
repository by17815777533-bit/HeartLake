/**
 * @brief Redis 缓存服务 -- 异步操作 + 内存 LRU 降级
 *
 * @details
 * 封装 Drogon 的 RedisClient，提供统一的异步缓存接口。
 * 当 Redis 不可用时自动降级到进程内 LRU 缓存（最多 10000 条），
 * 并通过指数退避策略在后台尝试重连，恢复后自动切回 Redis。
 *
 * 支持的数据结构：String / Hash / Counter / Lua 脚本，
 * 同时提供三种调用方式：
 *   - 异步回调（主推，适合 IO 密集路径）
 *   - 同步阻塞（getSync / setexSync，仅用于启动阶段等简单场景）
 *   - C++20 协程（getCoro / setExCoro / ttlCoro，配合 Drogon 协程调度器）
 *
 * 连接池支持动态伸缩：负载升高时自动扩容，空闲时回收多余连接。
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <chrono>
#include <json/json.h>
#include <unordered_map>
#include <mutex>
#include <map>
#include <list>
#include <vector>
#include <atomic>
#include <drogon/drogon.h>

namespace heartlake {
namespace cache {

/// Redis 连接池配置参数
struct RedisPoolConfig {
    int initialSize = 30;           ///< 初始连接数
    int maxSize = 100;              ///< 最大连接数
    int idleTimeoutMs = 30000;      ///< 空闲连接超时 (30s)
    int connectionTimeoutMs = 5000; ///< 建连超时 (5s)
    bool enableAutoScale = true;    ///< 是否根据负载动态伸缩
};

/**
 * @brief Redis 缓存客户端，全局单例
 *
 * @details
 * 所有异步方法在 Redis 不可用时自动走内存 LRU 降级路径，
 * 调用方无需关心底层连接状态。key 建议使用 makeKey() 拼接，
 * 统一加上 "heartlake:" 前缀以避免与其他服务冲突。
 */
class RedisCache {
public:
    static RedisCache& getInstance();

    /**
     * @brief 初始化 Redis 连接
     * @param host Redis 主机地址
     * @param port Redis 端口
     * @param password 认证密码，空串表示无密码
     * @param db 数据库编号
     * @param poolConfig 连接池配置
     */
    void initialize(const std::string& host = "127.0.0.1",
                   int port = 6379,
                   const std::string& password = "",
                   int db = 0,
                   const RedisPoolConfig& poolConfig = RedisPoolConfig{});

    // ---- String 操作 ----

    /// 异步 SET（无 TTL）
    void set(const std::string& key, const std::string& value,
             std::function<void(bool success)> callback = nullptr);

    /// 异步 SETEX（带 TTL）
    void setEx(const std::string& key, const std::string& value, int ttlSeconds,
               std::function<void(bool success)> callback = nullptr);

    /// 异步 GET
    void get(const std::string& key,
             std::function<void(const std::string& value, bool exists)> callback);

    /// 异步 DEL
    void del(const std::string& key,
             std::function<void(bool success)> callback = nullptr);

    // ---- JSON 便捷操作 ----

    /// 将 JSON 序列化后存入 Redis
    void setJson(const std::string& key, const Json::Value& value, int ttlSeconds = 0,
                 std::function<void(bool success)> callback = nullptr);

    /// 从 Redis 读取并反序列化为 JSON
    void getJson(const std::string& key,
                 std::function<void(const Json::Value& value, bool exists)> callback);

    // ---- 批量操作 ----

    /// 批量 GET，返回值顺序与 keys 一一对应
    void mget(const std::vector<std::string>& keys,
              std::function<void(const std::vector<std::string>& values)> callback);

    /// 批量 SET
    void mset(const std::vector<std::pair<std::string, std::string>>& kvPairs,
              std::function<void(bool success)> callback = nullptr);

    // ---- Hash 操作 ----

    /// HSET：设置哈希表字段
    void hset(const std::string& key, const std::string& field, const std::string& value,
              std::function<void(bool success)> callback = nullptr);

    /// HGET：读取哈希表单个字段
    void hget(const std::string& key, const std::string& field,
              std::function<void(const std::string& value, bool exists)> callback);

    /// HGETALL：读取哈希表全部字段
    void hgetall(const std::string& key,
                 std::function<void(const std::map<std::string, std::string>& values)> callback);

    // ---- 计数器操作 ----

    /// INCR：原子自增 1
    void incr(const std::string& key,
              std::function<void(int64_t newValue)> callback = nullptr);

    /// INCRBY：原子自增指定步长
    void incrBy(const std::string& key, int64_t delta,
                std::function<void(int64_t newValue)> callback = nullptr);

    // ---- Lua 脚本 ----

    /**
     * @brief 执行 Lua 脚本（EVAL 命令）
     * @param script Lua 脚本内容
     * @param keys KEYS 参数列表
     * @param args ARGV 参数列表
     * @param callback 返回脚本执行结果（整数）
     */
    void eval(const std::string& script, const std::vector<std::string>& keys,
              const std::vector<std::string>& args,
              std::function<void(int64_t)> callback);

    // ---- TTL / 存在性 ----

    /// 设置 key 的过期时间
    void expire(const std::string& key, int seconds,
                std::function<void(bool success)> callback = nullptr);

    /// 查询 key 剩余 TTL（秒），-1 表示永不过期，-2 表示不存在
    void ttl(const std::string& key,
             std::function<void(int ttl)> callback);

    /// 检查 key 是否存在
    void exists(const std::string& key,
                std::function<void(bool exists)> callback);

    /// 按模式匹配查找 key（生产环境慎用 KEYS 命令）
    void keys(const std::string& pattern,
              std::function<void(const std::vector<std::string>& keys)> callback);

    // ---- Key 构造工具 ----

    /// 拼接带前缀的缓存 key：heartlake:{category}:{id}
    static std::string makeKey(const std::string& category, const std::string& id);

    // 各业务域的 key 前缀常量
    static constexpr const char* KEY_PREFIX = "heartlake:";
    static constexpr const char* USER_CACHE = "user:";
    static constexpr const char* STONE_CACHE = "stone:";
    static constexpr const char* AI_CACHE = "ai:";
    static constexpr const char* SESSION_CACHE = "session:";
    static constexpr const char* RATE_LIMIT = "rate:";
    static constexpr const char* STATS_CACHE = "stats:";

    // 常用 TTL 预设值
    static constexpr int TTL_SHORT = 60;         ///< 1 分钟
    static constexpr int TTL_MEDIUM = 300;       ///< 5 分钟
    static constexpr int TTL_LONG = 3600;        ///< 1 小时
    static constexpr int TTL_DAY = 86400;        ///< 1 天

    /// @return 当前是否与 Redis 保持连接
    bool isConnected() const { return connected_; }

    /// 主动尝试建立 Redis 连接
    void tryConnect();

    /// 标记连接断开，后续操作自动降级到内存缓存
    void markDisconnected();

    // ---- 同步方法（阻塞当前线程）----

    /// 同步 GET，仅用于启动阶段等简单场景
    std::string getSync(const std::string& key);
    /// 同步 SETEX
    void setexSync(const std::string& key, const std::string& value, int ttlSeconds);

    // ---- C++20 协程方法 ----

    /// 协程版 GET，返回 {value, exists}
    drogon::Task<std::pair<std::string, bool>> getCoro(const std::string& key);
    /// 协程版 SETEX
    drogon::Task<void> setExCoro(const std::string& key, const std::string& value, int ttlSeconds);
    /// 协程版 TTL 查询
    drogon::Task<int64_t> ttlCoro(const std::string& key);

private:
    RedisCache() = default;

    std::atomic<bool> connected_{false};
    std::string host_;
    int port_;
    std::string password_;
    int db_;
    RedisPoolConfig poolConfig_;

    std::atomic<int> activeConnections_{0};  ///< 当前活跃连接数
    std::atomic<int> poolSize_{0};           ///< 当前池大小

    // ---- 内存 LRU 降级缓存 ----
    static constexpr size_t MAX_CACHE_SIZE = 10000;

    struct CacheEntry {
        std::string value;
        std::chrono::steady_clock::time_point expireTime;
    };
    std::list<std::pair<std::string, CacheEntry>> lruList_;  ///< LRU 链表，头部 = 最近访问
    std::unordered_map<std::string, std::list<std::pair<std::string, CacheEntry>>::iterator> memoryCache_;
    std::mutex cacheMutex_;  ///< 保护内存降级缓存的互斥锁

    /// 降级写入内存缓存
    void fallbackSet(const std::string& key, const std::string& value, int ttlSeconds);
    /// 降级读取内存缓存
    std::pair<std::string, bool> fallbackGet(const std::string& key);

    /// 根据负载动态扩容连接池
    void scalePool();
    /// 回收空闲超时的连接
    void recycleIdleConnections();
    /// 调度指数退避重连
    void scheduleReconnect();

    std::atomic<bool> reconnecting_{false};
    int reconnectDelaySeconds_{5};  ///< 初始重连间隔，每次翻倍
};

/**
 * @brief AI 推理结果缓存 -- 避免对同一文本重复调用 AI 推理
 *
 * @details
 * 按文本内容的哈希值作为 key，分别缓存三类 AI 结果：
 *   - 情感分析（score + mood）
 *   - AI 生成回复
 *   - 内容审核结论（passed + reason）
 *
 * 底层复用 RedisCache，Redis 不可用时自动降级到内存缓存。
 * 缓存命中可将 AI 推理延迟从数百毫秒降至亚毫秒级。
 */
class AIResponseCache {
public:
    static AIResponseCache& getInstance();

    /**
     * @brief 缓存情感分析结果
     * @param text 原始文本
     * @param score 情感分数 [-1, 1]
     * @param mood 检测到的情绪类型
     */
    void cacheSentiment(const std::string& text, float score, const std::string& mood);

    /**
     * @brief 查询已缓存的情感分析结果
     * @param text 原始文本
     * @param[out] score 情感分数
     * @param[out] mood 情绪类型
     * @return 命中返回 true
     */
    bool getSentiment(const std::string& text, float& score, std::string& mood);

    /// 缓存 AI 生成的回复文本
    void cacheReply(const std::string& context, const std::string& reply);
    /// 查询已缓存的 AI 回复，命中返回 true
    bool getReply(const std::string& context, std::string& reply);

    /// 缓存内容审核结论
    void cacheModeration(const std::string& text, bool passed, const std::string& reason);
    /// 查询已缓存的审核结论，命中返回 true
    bool getModeration(const std::string& text, bool& passed, std::string& reason);

private:
    AIResponseCache() = default;

    /// 对文本做哈希，生成缓存 key
    std::string hashText(const std::string& text);
};

} // namespace cache
} // namespace heartlake
