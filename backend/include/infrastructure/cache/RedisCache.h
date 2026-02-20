/**
 * @file RedisCache.h
 * @brief RedisCache 模块接口定义
 * Created by 王璐瑶
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

/**
 * Redis缓存服务
 * 提供异步缓存操作，支持：
 * - 字符串缓存
 * - JSON对象缓存
 * - 带过期时间的缓存
 * - 缓存失效
 */
/**
 * @brief Redis缓存管理器
 *
 * 详细说明
 *
 * @note 注意事项
 */
// Connection pool configuration
struct RedisPoolConfig {
    int initialSize = 30;           // Initial pool size
    int maxSize = 100;              // Maximum pool size
    int idleTimeoutMs = 30000;      // Idle connection timeout (30s)
    int connectionTimeoutMs = 5000; // Connection timeout (5s)
    bool enableAutoScale = true;    // Dynamic scaling
};

class RedisCache {
public:
    static RedisCache& getInstance();

    void initialize(const std::string& host = "127.0.0.1",
                   int port = 6379,
                   const std::string& password = "",
                   int db = 0,
                   const RedisPoolConfig& poolConfig = RedisPoolConfig{});
    
    void set(const std::string& key, const std::string& value,
             std::function<void(bool success)> callback = nullptr);
             
    void setEx(const std::string& key, const std::string& value, int ttlSeconds,
               std::function<void(bool success)> callback = nullptr);
               
    void get(const std::string& key,
             std::function<void(const std::string& value, bool exists)> callback);
             
    void del(const std::string& key,
             std::function<void(bool success)> callback = nullptr);
    
    void setJson(const std::string& key, const Json::Value& value, int ttlSeconds = 0,
                 std::function<void(bool success)> callback = nullptr);
                 
    void getJson(const std::string& key,
                 std::function<void(const Json::Value& value, bool exists)> callback);
    
    void mget(const std::vector<std::string>& keys,
              std::function<void(const std::vector<std::string>& values)> callback);
              
    void mset(const std::vector<std::pair<std::string, std::string>>& kvPairs,
              std::function<void(bool success)> callback = nullptr);
    
    void hset(const std::string& key, const std::string& field, const std::string& value,
              std::function<void(bool success)> callback = nullptr);
              
    void hget(const std::string& key, const std::string& field,
              std::function<void(const std::string& value, bool exists)> callback);
              
    void hgetall(const std::string& key,
                 std::function<void(const std::map<std::string, std::string>& values)> callback);
    
    void incr(const std::string& key,
              std::function<void(int64_t newValue)> callback = nullptr);
              
    void incrBy(const std::string& key, int64_t delta,
                std::function<void(int64_t newValue)> callback = nullptr);

    // Lua脚本执行
    void eval(const std::string& script, const std::vector<std::string>& keys,
              const std::vector<std::string>& args,
              std::function<void(int64_t)> callback);
    
    void expire(const std::string& key, int seconds,
                std::function<void(bool success)> callback = nullptr);
                
    void ttl(const std::string& key,
             std::function<void(int ttl)> callback);
    
    void exists(const std::string& key,
                std::function<void(bool exists)> callback);
                
    void keys(const std::string& pattern,
              std::function<void(const std::vector<std::string>& keys)> callback);
    
    static std::string makeKey(const std::string& category, const std::string& id);
    
    static constexpr const char* KEY_PREFIX = "heartlake:";
    static constexpr const char* USER_CACHE = "user:";
    static constexpr const char* STONE_CACHE = "stone:";
    static constexpr const char* AI_CACHE = "ai:";
    static constexpr const char* SESSION_CACHE = "session:";
    static constexpr const char* RATE_LIMIT = "rate:";
    static constexpr const char* STATS_CACHE = "stats:";
    
    static constexpr int TTL_SHORT = 60;         // 1分钟
    static constexpr int TTL_MEDIUM = 300;       // 5分钟
    static constexpr int TTL_LONG = 3600;        // 1小时
    static constexpr int TTL_DAY = 86400;        // 1天
    
    /**
     * @brief isConnected方法
     * @return 返回值说明
     */
    bool isConnected() const { return connected_; }
    void tryConnect();
    void markDisconnected();

    // 同步方法（阻塞）- 用于简单场景
    std::string getSync(const std::string& key);
    void setexSync(const std::string& key, const std::string& value, int ttlSeconds);

    // 协程方法（非阻塞）
    drogon::Task<std::pair<std::string, bool>> getCoro(const std::string& key);
    drogon::Task<void> setExCoro(const std::string& key, const std::string& value, int ttlSeconds);
    drogon::Task<int64_t> ttlCoro(const std::string& key);
    
private:
    RedisCache() = default;

    std::atomic<bool> connected_{false};
    std::string host_;
    int port_;
    std::string password_;
    int db_;
    RedisPoolConfig poolConfig_;

    // Connection pool state
    std::atomic<int> activeConnections_{0};
    std::atomic<int> poolSize_{0};

    // LRU fallback memory cache
    static constexpr size_t MAX_CACHE_SIZE = 10000;

    struct CacheEntry {
        std::string value;
        std::chrono::steady_clock::time_point expireTime;
    };
    // LRU list: front = most recently used
    std::list<std::pair<std::string, CacheEntry>> lruList_;
    std::unordered_map<std::string, std::list<std::pair<std::string, CacheEntry>>::iterator> memoryCache_;
    std::mutex cacheMutex_;

    void fallbackSet(const std::string& key, const std::string& value, int ttlSeconds);
    std::pair<std::string, bool> fallbackGet(const std::string& key);
    void scalePool();
    void recycleIdleConnections();
    void scheduleReconnect();

    std::atomic<bool> reconnecting_{false};
    int reconnectDelaySeconds_{5};
};

/**
 * AI回复缓存
 * 缓存AI生成的回复，避免重复调用API
 */
/**
 * @brief AIResponseCache类
 *
 * 详细说明
 *
 * @note 注意事项
 */
class AIResponseCache {
public:
    static AIResponseCache& getInstance();
    
    /**
     * @brief cacheSentiment方法
     *
     * @param text 参数说明
     * @param score 参数说明
     * @param mood 参数说明
     */
    void cacheSentiment(const std::string& text, float score, const std::string& mood);
    /**
     * @brief getSentiment方法
     *
     * @param text 参数说明
     * @param score 参数说明
     * @param mood 参数说明
     * @return 返回值说明
     */
    bool getSentiment(const std::string& text, float& score, std::string& mood);
    
    /**
     * @brief cacheReply方法
     *
     * @param context 参数说明
     * @param reply 参数说明
     */
    void cacheReply(const std::string& context, const std::string& reply);
    /**
     * @brief getReply方法
     *
     * @param context 参数说明
     * @param reply 参数说明
     * @return 返回值说明
     */
    bool getReply(const std::string& context, std::string& reply);
    
    /**
     * @brief cacheModeration方法
     *
     * @param text 参数说明
     * @param passed 参数说明
     * @param reason 参数说明
     */
    void cacheModeration(const std::string& text, bool passed, const std::string& reason);
    /**
     * @brief getModeration方法
     *
     * @param text 参数说明
     * @param passed 参数说明
     * @param reason 参数说明
     * @return 返回值说明
     */
    bool getModeration(const std::string& text, bool& passed, std::string& reason);
    
private:
    AIResponseCache() = default;
    
    std::string hashText(const std::string& text);
};

} // namespace cache
} // namespace heartlake
