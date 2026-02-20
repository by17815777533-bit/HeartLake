/**
 * @file RateLimitFilter.cpp
 * @brief RateLimitFilter 模块实现
 * Created by 白洋
 */
#include "infrastructure/filters/RateLimitFilter.h"
#include "utils/ResponseUtil.h"
#include <drogon/drogon.h>

using namespace drogon;
using namespace heartlake::filters;
using namespace heartlake::utils;

std::unordered_map<std::string, RateLimitFilter::RateLimitEntry> RateLimitFilter::rateLimitMap_;
std::mutex RateLimitFilter::mutex_;
int RateLimitFilter::requestsPerWindow_ = 100;  // 默认100请求/分钟
int RateLimitFilter::windowSeconds_ = 60;
int RateLimitFilter::aiRequestsPerWindow_ = 20; // AI API: 20请求/分钟
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
