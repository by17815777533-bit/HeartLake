/**
 * @brief 速率限制过滤器 - 滑动窗口限流防止 API 滥用
 *
 * @details
 * 基于滑动窗口算法实现请求频率控制，支持两种限流维度：
 * - 普通接口限流：基于客户端 IP 或用户 Token
 * - AI 接口限流：针对 AI 推理类接口的独立限流策略（更严格）
 *
 * 限流键的生成优先使用 PASETO 令牌中的用户ID，
 * 未登录时降级为客户端 IP 地址。
 *
 * 超限时返回 429 Too Many Requests，响应头包含：
 * - X-RateLimit-Limit: 窗口内允许的最大请求数
 * - X-RateLimit-Remaining: 剩余可用次数
 * - Retry-After: 建议重试等待秒数
 */

#pragma once

#include <drogon/HttpFilter.h>
#include <unordered_map>
#include <chrono>
#include <mutex>

using namespace drogon;

namespace heartlake {
namespace filters {

/**
 * @brief 通用速率限制过滤器
 *
 * @details 内部维护一个 key → RateLimitEntry 的哈希表，
 * 每个 entry 记录滑动窗口内的请求时间戳列表。
 * 定期清理过期的 entry 防止内存膨胀。
 *
 * 线程安全：所有对 rateLimitMap_ 的访问通过 mutex_ 保护。
 */
class RateLimitFilter : public HttpFilter<RateLimitFilter> {
    friend class RateLimitFilterTest;
    friend class RateLimitFilterTest_CleanupStaleKeysNoCrash_Test;
    friend class RateLimitFilterTest_CheckRateLimitAIMode_Test;
public:
    RateLimitFilter() = default;

    /**
     * @brief 执行限流检查
     *
     * @param req 入站 HTTP 请求
     * @param fcb 过滤失败回调 — 超限时返回 429
     * @param fccb 过滤通过回调 — 未超限时放行
     */
    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;

    /**
     * @brief 设置普通接口的限流参数
     * @param requestsPerWindow 窗口内允许的最大请求数
     * @param windowSeconds 滑动窗口大小（秒）
     */
    static void setRateLimit(int requestsPerWindow, int windowSeconds);

    /**
     * @brief 设置 AI 接口的限流参数（通常更严格）
     * @param requestsPerWindow 窗口内允许的最大请求数
     * @param windowSeconds 滑动窗口大小（秒）
     */
    static void setAIRateLimit(int requestsPerWindow, int windowSeconds);

private:
    /// 单个限流键的滑动窗口记录
    struct RateLimitEntry {
        std::vector<std::chrono::steady_clock::time_point> requestTimes; ///< 窗口内的请求时间戳
        std::chrono::steady_clock::time_point lastCleanup;              ///< 上次清理过期记录的时间
    };

    static std::unordered_map<std::string, RateLimitEntry> rateLimitMap_; ///< 限流记录表
    static std::mutex mutex_;                ///< 保护 rateLimitMap_ 的互斥锁
    static int requestsPerWindow_;           ///< 普通接口：窗口内最大请求数
    static int windowSeconds_;               ///< 普通接口：窗口大小（秒）
    static int aiRequestsPerWindow_;         ///< AI 接口：窗口内最大请求数
    static int aiWindowSeconds_;             ///< AI 接口：窗口大小（秒）

    /**
     * @brief 检查指定 key 是否超过速率限制
     * @param key 限流键（用户ID 或 IP 地址）
     * @param isAI 是否使用 AI 接口的限流参数
     * @return true 未超限（允许通过），false 已超限（应拒绝）
     */
    bool checkRateLimit(const std::string& key, bool isAI = false);

    /**
     * @brief 清理 entry 中超出窗口的过期时间戳
     * @param entry 待清理的限流记录
     * @param isAI 是否使用 AI 接口的窗口大小
     */
    void cleanupOldEntries(RateLimitEntry& entry, bool isAI = false);

    /// 清理长时间无请求的 stale key，防止内存泄漏
    static void cleanupStaleKeys();

    /**
     * @brief 从请求中提取限流键
     * @details 优先使用 PASETO 令牌中的用户ID，降级为客户端 IP
     * @param req HTTP 请求
     * @return 限流键字符串
     */
    std::string getClientKey(const HttpRequestPtr& req);
};

/**
 * @brief AI 专用限流过滤器
 *
 * @details 复用 RateLimitFilter 的限流逻辑，但强制使用 AI 限流参数。
 * 挂载在 /api/edge-ai/* 和 /api/guardian/chat 等 AI 推理端点上。
 */
class AIRateLimitFilter : public HttpFilter<AIRateLimitFilter> {
public:
    AIRateLimitFilter() = default;

    /**
     * @brief 执行 AI 接口限流检查
     *
     * @param req 入站 HTTP 请求
     * @param fcb 过滤失败回调
     * @param fccb 过滤通过回调
     */
    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;
};

} // namespace filters
} // namespace heartlake
