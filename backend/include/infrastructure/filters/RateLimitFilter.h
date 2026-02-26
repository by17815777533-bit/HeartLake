/**
 * @file RateLimitFilter.h
 * @brief RateLimitFilter 模块接口定义
 * Created by 白洋
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
 * Rate Limiting Filter - 限流中间件
 * 支持:
 * - 基于IP的限流
 * - 基于用户Token的限流
 * - 滑动窗口算法
 */
/**
 * @brief 速率限制过滤器，用于防止API滥用
 *
 * 详细说明
 *
 * @note 注意事项
 */
class RateLimitFilter : public HttpFilter<RateLimitFilter> {
    friend class RateLimitFilterTest;
    friend class RateLimitFilterTest_CleanupStaleKeysNoCrash_Test;
    friend class RateLimitFilterTest_CheckRateLimitAIMode_Test;
public:
    RateLimitFilter() = default;
    
    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;
    
    /**
     * @brief setRateLimit方法
     *
     * @param requestsPerWindow 参数说明
     * @param windowSeconds 参数说明
     */
    static void setRateLimit(int requestsPerWindow, int windowSeconds);
    /**
     * @brief setAIRateLimit方法
     *
     * @param requestsPerWindow 参数说明
     * @param windowSeconds 参数说明
     */
    static void setAIRateLimit(int requestsPerWindow, int windowSeconds);
    
private:
    struct RateLimitEntry {
        std::vector<std::chrono::steady_clock::time_point> requestTimes;
        std::chrono::steady_clock::time_point lastCleanup;
    };
    
    static std::unordered_map<std::string, RateLimitEntry> rateLimitMap_;
    static std::mutex mutex_;
    static int requestsPerWindow_;
    static int windowSeconds_;
    static int aiRequestsPerWindow_;
    static int aiWindowSeconds_;
    
    /**
     * @brief checkRateLimit方法
     *
     * @param key 参数说明
     * @param isAI 参数说明
     * @return 返回值说明
     */
    bool checkRateLimit(const std::string& key, bool isAI = false);
    /**
     * @brief cleanupOldEntries方法
     *
     * @param entry 参数说明
     * @param isAI 参数说明
     */
    void cleanupOldEntries(RateLimitEntry& entry, bool isAI = false);
    static void cleanupStaleKeys();
    std::string getClientKey(const HttpRequestPtr& req);
};

/**
 * AI专用限流过滤器
 */
/**
 * @brief AIRateLimitFilter类
 *
 * 详细说明
 *
 * @note 注意事项
 */
class AIRateLimitFilter : public HttpFilter<AIRateLimitFilter> {
public:
    AIRateLimitFilter() = default;
    
    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;
};

} // namespace filters
} // namespace heartlake
