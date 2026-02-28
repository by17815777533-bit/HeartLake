/**
 * @brief 请求限流过滤器 —— 滑动窗口算法的 HTTP 请求限流
 *
 * 每个客户端（按 token hash 或 IP 标识）在时间窗口内的请求时间戳
 * 被记录在 vector 中，超过阈值则返回 429 Too Many Requests。
 * 普通 API 默认 100 次/分钟，AI API 默认 20 次/分钟。
 * AIRateLimitFilter 额外实现了 map 膨胀保护：超过 10000 个 key 时淘汰最旧的一半。
 */
#include "infrastructure/filters/RateLimitFilter.h"
#include "utils/ResponseUtil.h"
#include <drogon/drogon.h>
#include <algorithm>

using namespace drogon;
using namespace heartlake::filters;
using namespace heartlake::utils;

// 静态成员初始化：限流参数和共享状态
std::unordered_map<std::string, RateLimitFilter::RateLimitEntry> RateLimitFilter::rateLimitMap_;
std::mutex RateLimitFilter::mutex_;
int RateLimitFilter::requestsPerWindow_ = 100;  // 普通 API：100 次/分钟
int RateLimitFilter::windowSeconds_ = 60;
int RateLimitFilter::aiRequestsPerWindow_ = 20; // AI API：20 次/分钟
int RateLimitFilter::aiWindowSeconds_ = 60;

void RateLimitFilter::setRateLimit(int requestsPerWindow, int windowSeconds) {
    requestsPerWindow_ = requestsPerWindow;
    windowSeconds_ = windowSeconds;
}

void RateLimitFilter::setAIRateLimit(int requestsPerWindow, int windowSeconds) {
    aiRequestsPerWindow_ = requestsPerWindow;
    aiWindowSeconds_ = windowSeconds;
}

void RateLimitFilter::doFilter(const HttpRequestPtr& req,
                               FilterCallback&& fcb,
                               FilterChainCallback&& fccb) {
    std::string clientKey = getClientKey(req);
    
    if (!checkRateLimit(clientKey, false)) {
        LOG_WARN << "Rate limit exceeded for: " << clientKey;
        fcb(ResponseUtil::tooManyRequests("请求过于频繁，请稍后再试"));
        return;
    }
    
    fccb();
}

/// 滑动窗口限流核心：清理过期记录后判断窗口内请求数是否超限
bool RateLimitFilter::checkRateLimit(const std::string& key, bool isAI) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto& entry = rateLimitMap_[key];
    
    // 清理过期记录
    cleanupOldEntries(entry, isAI);
    
    int limit = isAI ? aiRequestsPerWindow_ : requestsPerWindow_;
    
    if (entry.requestTimes.size() >= static_cast<size_t>(limit)) {
        return false;
    }
    
    entry.requestTimes.push_back(now);
    return true;
}

/// 移除窗口起点之前的过期时间戳（erase-remove 惯用法）
void RateLimitFilter::cleanupOldEntries(RateLimitEntry& entry, bool isAI) {
    auto now = std::chrono::steady_clock::now();
    int windowSecs = isAI ? aiWindowSeconds_ : windowSeconds_;
    auto windowStart = now - std::chrono::seconds(windowSecs);

    entry.requestTimes.erase(
        std::remove_if(entry.requestTimes.begin(), entry.requestTimes.end(),
            [windowStart](const auto& time) {
                return time < windowStart;
            }),
        entry.requestTimes.end()
    );
}

/// 定期清理长时间无请求的 key，防止 map 无限增长
void RateLimitFilter::cleanupStaleKeys() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto threshold = now - std::chrono::seconds(windowSeconds_ * 2);

    for (auto it = rateLimitMap_.begin(); it != rateLimitMap_.end();) {
        if (it->second.requestTimes.empty() ||
            it->second.requestTimes.back() < threshold) {
            it = rateLimitMap_.erase(it);
        } else {
            ++it;
        }
    }
}

/// 提取客户端标识：优先用 Bearer token 的 hash，其次用 X-Forwarded-For 或直连 IP
std::string RateLimitFilter::getClientKey(const HttpRequestPtr& req) {
    // 优先使用Token识别用户
    std::string authHeader = req->getHeader("Authorization");
    if (!authHeader.empty() && authHeader.find("Bearer ") == 0) {
        std::string token = authHeader.substr(7);
        // 使用token的hash作为key
        return "token:" + std::to_string(std::hash<std::string>{}(token));
    }
    
    // 否则使用IP地址
    std::string ip = req->peerAddr().toIp();
    
    // 检查X-Forwarded-For头（反向代理场景）
    std::string forwardedFor = req->getHeader("X-Forwarded-For");
    if (!forwardedFor.empty()) {
        // 取第一个IP
        auto pos = forwardedFor.find(',');
        if (pos != std::string::npos) {
            ip = forwardedFor.substr(0, pos);
        } else {
            ip = forwardedFor;
        }
    }
    
    return "ip:" + ip;
}

void AIRateLimitFilter::doFilter(const HttpRequestPtr& req,
                                 FilterCallback&& fcb,
                                 FilterChainCallback&& fccb) {
    std::string clientKey;
    
    // 获取客户端标识
    std::string authHeader = req->getHeader("Authorization");
    if (!authHeader.empty() && authHeader.find("Bearer ") == 0) {
        std::string token = authHeader.substr(7);
        clientKey = "ai:token:" + std::to_string(std::hash<std::string>{}(token));
    } else {
        std::string ip = req->peerAddr().toIp();
        std::string forwardedFor = req->getHeader("X-Forwarded-For");
        if (!forwardedFor.empty()) {
            auto pos = forwardedFor.find(',');
            ip = (pos != std::string::npos) ? forwardedFor.substr(0, pos) : forwardedFor;
        }
        clientKey = "ai:ip:" + ip;
    }
    
    // 使用静态map进行AI限流
    static std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> aiRateLimitMap;
    static std::mutex aiMutex;
    
    std::lock_guard<std::mutex> lock(aiMutex);
    auto now = std::chrono::steady_clock::now();
    auto& times = aiRateLimitMap[clientKey];
    
    // 防止 map 无限增长：超过 10000 个 key 时，删除最旧的一半
    if (aiRateLimitMap.size() > 10000) {
        // 收集每个 key 的最新请求时间
        std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> entries;
        entries.reserve(aiRateLimitMap.size());
        for (const auto& [k, v] : aiRateLimitMap) {
            auto latest = v.empty() ? std::chrono::steady_clock::time_point{} : v.back();
            entries.emplace_back(k, latest);
        }
        // 按最新请求时间升序排列，前一半是最旧的
        std::sort(entries.begin(), entries.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        size_t removeCount = entries.size() / 2;
        for (size_t i = 0; i < removeCount; ++i) {
            aiRateLimitMap.erase(entries[i].first);
        }
        LOG_INFO << "AI rate limit map 淘汰了 " << removeCount << " 个过期条目";
    }

    // 清理60秒前的记录
    auto windowStart = now - std::chrono::seconds(60);
    times.erase(
        std::remove_if(times.begin(), times.end(),
            [windowStart](const auto& t) { return t < windowStart; }),
        times.end()
    );

    // AI API限制：每分钟20次
    if (times.size() >= 20) {
        LOG_WARN << "AI rate limit exceeded for: " << clientKey;
        fcb(ResponseUtil::tooManyRequests("AI请求过于频繁，请稍后再试（限制：20次/分钟）"));
        return;
    }
    
    times.push_back(now);
    fccb();
}
