/**
 * @brief 多策略分布式限流器
 *
 * 支持三种限流算法（令牌桶、滑动窗口、固定窗口），四种限流粒度
 * （用户级、IP级、接口级、全局级），可选 Redis 后端实现分布式限流，
 * 也可退化为本地内存模式。
 *
 * @details 典型用法：在 Drogon Filter 中调用 RateLimitFilter::apply()，
 *          自动从请求中提取限流键并检查配额，超限时返回 429 响应。
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

/// 限流算法策略
enum class RateLimitStrategy {
    TOKEN_BUCKET,      ///< 令牌桶：平滑限速，允许突发
    SLIDING_WINDOW,    ///< 滑动窗口：精确统计时间窗口内的请求数
    FIXED_WINDOW       ///< 固定窗口：按固定时间段计数，实现简单但有边界突发问题
};

/// 限流粒度级别
enum class RateLimitLevel {
    USER,              ///< 按用户ID限流
    IP,                ///< 按客户端IP限流
    ENDPOINT,          ///< 按接口路径限流
    GLOBAL             ///< 全局统一限流
};

/// 限流配置参数
struct RateLimitConfig {
    RateLimitStrategy strategy = RateLimitStrategy::TOKEN_BUCKET;
    int capacity;              ///< 令牌桶容量 / 窗口内最大请求数
    int refillRate;            ///< 令牌填充速率（个/秒）
    int windowSeconds;         ///< 时间窗口长度（秒）
    bool useRedis = true;      ///< 是否使用 Redis 实现分布式限流
    std::string redisKeyPrefix = "ratelimit:";
};

/// 限流检查结果
struct RateLimitResult {
    bool allowed;              ///< 本次请求是否放行
    int remaining;             ///< 剩余可用配额
    int limit;                 ///< 总配额上限
    int retryAfter;            ///< 被限流时建议的重试等待秒数
    std::string reason;        ///< 限流原因描述
};

/**
 * @brief 限流器核心（单例）
 *
 * 根据配置的策略和级别执行限流检查，内部同时维护
 * Redis 远程实现和本地内存实现，按 useRedis 配置自动切换。
 */
class RateLimiter {
public:
    static RateLimiter& getInstance();

    /// 加载默认限流配置，服务启动时调用
    void initialize();

    /**
     * @brief 执行限流检查
     * @param key 限流键（用户ID、IP地址等，取决于 level）
     * @param level 限流粒度
     * @param endpoint 接口路径（仅 ENDPOINT 级别使用）
     * @return 限流结果，包含是否放行、剩余配额等信息
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

    /// 限流统计信息
    struct RateLimitStats {
        int totalRequests;      ///< 总请求数
        int allowedRequests;    ///< 放行请求数
        int blockedRequests;    ///< 被拦截请求数
        float blockRate;        ///< 拦截率（0.0~1.0）
        int64_t lastRequestTime; ///< 最近一次请求的时间戳
    };
    /// 获取指定限流键的统计数据
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

    /// 本地令牌桶状态（非 Redis 模式下使用）
    struct LocalBucket {
        int tokens;                                    ///< 当前可用令牌数
        std::chrono::steady_clock::time_point lastRefill; ///< 上次填充时间
        int requestCount;                              ///< 累计请求数
        int blockedCount;                              ///< 累计拦截数
    };
    std::unordered_map<std::string, LocalBucket> localBuckets_;

    // ---- 策略分发：根据 config.strategy 选择算法 ----
    RateLimitResult checkTokenBucket(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkSlidingWindow(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkFixedWindow(const std::string& key, const RateLimitConfig& config);

    // ---- Redis 分布式实现 ----
    RateLimitResult checkTokenBucketRedis(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkSlidingWindowRedis(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkFixedWindowRedis(const std::string& key, const RateLimitConfig& config);

    // ---- 本地内存实现（单机模式或 Redis 不可用时的降级） ----
    RateLimitResult checkTokenBucketLocal(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkSlidingWindowLocal(const std::string& key, const RateLimitConfig& config);
    RateLimitResult checkFixedWindowLocal(const std::string& key, const RateLimitConfig& config);

    /// 构建 Redis key（prefix + 限流键）
    std::string buildRedisKey(const std::string& key, const RateLimitConfig& config);
    /// 根据时间差补充本地令牌桶的令牌
    void refillTokens(LocalBucket& bucket, const RateLimitConfig& config);
    /// 获取指定级别的限流配置，接口级别优先查 endpointConfigs_
    RateLimitConfig getConfigForLevel(RateLimitLevel level);
};

/**
 * @brief 限流 Drogon Filter 适配层
 *
 * 封装 RateLimiter 的调用逻辑，提供从 HTTP 请求中提取限流键、
 * 执行检查、构建 429 响应的一站式方法，供 Controller Filter 直接使用。
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
