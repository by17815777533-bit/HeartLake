/**
 * RateLimiter 模块接口定义
 */


#pragma once

#include <drogon/drogon.h>
#include <string>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace heartlake {
namespace middleware {

/**
 * 限流策略
 */
enum class RateLimitStrategy {
    TOKEN_BUCKET,      // 令牌桶
    SLIDING_WINDOW,    // 滑动窗口
    FIXED_WINDOW       // 固定窗口
};

/**
 * 限流级别
 */
enum class RateLimitLevel {
    USER,              // 用户级别
    IP,                // IP级别
    ENDPOINT,          // 接口级别
    GLOBAL             // 全局级别
};

/**
 * 限流配置
 */
struct RateLimitConfig {
    RateLimitStrategy strategy = RateLimitStrategy::TOKEN_BUCKET;
    int capacity;              // 容量（令牌桶大小或窗口大小）
    int refillRate;            // 填充速率（每秒添加的令牌数）
    int windowSeconds;         // 时间窗口（秒）
    bool useRedis = true;      // 是否使用Redis（分布式）
    std::string redisKeyPrefix = "ratelimit:";
};

/**
 * 限流结果
 */
struct RateLimitResult {
    bool allowed;              // 是否允许
    int remaining;             // 剩余配额
    int limit;                 // 总配额
    int retryAfter;            // 重试等待时间（秒）
    std::string reason;        // 原因
};

/**
 * 分布式限流器
 */
class RateLimiter {
public:
    static RateLimiter& getInstance();

    /**
     * 初始化限流器
     */
    void initialize();

    /**
     * 检查是否允许请求
     * @param key 限流键（用户ID、IP等）
     * @param level 限流级别
     * @param endpoint 接口路径（可选）
     * @return 限流结果
     */
    RateLimitResult checkLimit(
        const std::string& key,
        RateLimitLevel level,
        const std::string& endpoint = ""
    );

    /**
     * 设置限流配置
     * @param level 限流级别
     * @param config 配置
     */
    void setConfig(RateLimitLevel level, const RateLimitConfig& config);

    /**
     * 设置接口特定的限流配置
     * @param endpoint 接口路径
     * @param config 配置
     */
    void setEndpointConfig(const std::string& endpoint, const RateLimitConfig& config);

    /**
     * 获取限流统计
     * @param key 限流键
     * @return 统计信息
     */
    struct RateLimitStats {
        int totalRequests;
        int allowedRequests;
        int blockedRequests;
        float blockRate;
        int64_t lastRequestTime;
    };
    RateLimitStats getStats(const std::string& key);

    /**
     * 重置限流计数
     * @param key 限流键
     */
    void reset(const std::string& key);

private:
    RateLimiter() = default;
    ~RateLimiter() = default;
    RateLimiter(const RateLimiter&) = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;

    bool initialized_ = false;
    std::mutex mutex_;

    // 限流配置
    std::unordered_map<RateLimitLevel, RateLimitConfig> configs_;
    std::unordered_map<std::string, RateLimitConfig> endpointConfigs_;

    // 本地缓存（用于非Redis模式）
    struct LocalBucket {
        int tokens;
        std::chrono::steady_clock::time_point lastRefill;
        int requestCount;
        int blockedCount;
    };
    std::unordered_map<std::string, LocalBucket> localBuckets_;

    // 内部方法
    RateLimitResult checkTokenBucket(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkSlidingWindow(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkFixedWindow(const std::string& key, const RateLimitConfig& config);

    // Redis 实现
    RateLimitResult checkTokenBucketRedis(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkSlidingWindowRedis(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkFixedWindowRedis(const std::string& key, const RateLimitConfig& config);

    // 本地实现
    RateLimitResult checkTokenBucketLocal(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkSlidingWindowLocal(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkFixedWindowLocal(const std::string& key, const RateLimitConfig& config);

    // 辅助方法
    std::string buildRedisKey(const std::string& key, const RateLimitConfig& config);
    void refillTokens(LocalBucket& bucket, const RateLimitConfig& config);
    RateLimitConfig getConfigForLevel(RateLimitLevel level);
};

/**
 * 限流过滤器（Drogon Filter）
 */
class RateLimitFilter {
public:
    /**
     * 应用限流检查
     * @param req HTTP请求
     * @param callback 回调函数
     * @param level 限流级别
     */
    static void apply(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        RateLimitLevel level = RateLimitLevel::USER
    );

    /**
     * 从请求中提取限流键
     * @param req HTTP请求
     * @param level 限流级别
     * @return 限流键
     */
    static std::string extractKey(
        const drogon::HttpRequestPtr& req,
        RateLimitLevel level
    );

    /**
     * 创建限流响应
     * @param result 限流结果
     * @return HTTP响应
     */
    static drogon::HttpResponsePtr createRateLimitResponse(
        const RateLimitResult& result
    );
};

} // namespace middleware
} // namespace heartlake
