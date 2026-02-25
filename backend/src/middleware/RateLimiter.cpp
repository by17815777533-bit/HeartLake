/**
 * @file RateLimiter.cpp
 * @brief RateLimiter 模块实现
 * Created by 白洋
 */


#include "middleware/RateLimiter.h"
#include "infrastructure/cache/RedisCache.h"
#include "utils/ResponseUtil.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <sstream>
#include <thread>

using namespace drogon;

namespace heartlake {
namespace middleware {

RateLimiter& RateLimiter::getInstance() {
    static RateLimiter instance;
    return instance;
}

void RateLimiter::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        LOG_WARN << "RateLimiter already initialized";
        return;
    }

    LOG_INFO << "Initializing RateLimiter";

    // 加载默认配置（不需要再加锁，已在锁内）
    // 用户级别限流
    RateLimitConfig userConfig;
    userConfig.strategy = RateLimitStrategy::TOKEN_BUCKET;
    userConfig.capacity = 20;
    userConfig.refillRate = 10;
    userConfig.windowSeconds = 1;
    userConfig.useRedis = true;
    configs_[RateLimitLevel::USER] = userConfig;

    // IP级别限流
    RateLimitConfig ipConfig;
    ipConfig.strategy = RateLimitStrategy::SLIDING_WINDOW;
    ipConfig.capacity = 60;
    ipConfig.refillRate = 30;
    ipConfig.windowSeconds = 1;
    ipConfig.useRedis = true;
    configs_[RateLimitLevel::IP] = ipConfig;

    // 接口级别限流
    RateLimitConfig endpointConfig;
    endpointConfig.strategy = RateLimitStrategy::FIXED_WINDOW;
    endpointConfig.capacity = 100;
    endpointConfig.refillRate = 100;
    endpointConfig.windowSeconds = 1;
    endpointConfig.useRedis = true;
    configs_[RateLimitLevel::ENDPOINT] = endpointConfig;

    // 全局限流
    RateLimitConfig globalConfig;
    globalConfig.strategy = RateLimitStrategy::FIXED_WINDOW;
    globalConfig.capacity = 1000;
    globalConfig.refillRate = 1000;
    globalConfig.windowSeconds = 1;
    globalConfig.useRedis = true;
    configs_[RateLimitLevel::GLOBAL] = globalConfig;

    initialized_ = true;
    LOG_INFO << "RateLimiter initialized successfully";
}

void RateLimiter::loadDefaultConfigs() {
    // 已移至 initialize() 中以避免死锁
    LOG_INFO << "Loaded default rate limit configs";
}

void RateLimiter::setConfig(RateLimitLevel level, const RateLimitConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    configs_[level] = config;
}

void RateLimiter::setEndpointConfig(const std::string& endpoint, const RateLimitConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    endpointConfigs_[endpoint] = config;
}

RateLimitConfig RateLimiter::getConfigForLevel(RateLimitLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configs_.find(level);
    if (it != configs_.end()) {
        return it->second;
    }

    // 返回默认配置
    RateLimitConfig defaultConfig;
    defaultConfig.capacity = 100;
    defaultConfig.refillRate = 10;
    defaultConfig.windowSeconds = 1;
    return defaultConfig;
}

RateLimitResult RateLimiter::checkLimit(
    const std::string& key,
    RateLimitLevel level,
    const std::string& endpoint
) {
    // 获取配置 - 注意避免死锁，不要在持有锁时调用getConfigForLevel
    RateLimitConfig config;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!endpoint.empty()) {
            auto it = endpointConfigs_.find(endpoint);
            if (it != endpointConfigs_.end()) {
                config = it->second;
            } else {
                auto levelIt = configs_.find(level);
                if (levelIt != configs_.end()) {
                    config = levelIt->second;
                }
            }
        } else {
            auto levelIt = configs_.find(level);
            if (levelIt != configs_.end()) {
                config = levelIt->second;
            }
        }
    }

    // 根据策略选择算法
    switch (config.strategy) {
        case RateLimitStrategy::TOKEN_BUCKET:
            return checkTokenBucket(key, config);
        case RateLimitStrategy::SLIDING_WINDOW:
            return checkSlidingWindow(key, config);
        case RateLimitStrategy::FIXED_WINDOW:
            return checkFixedWindow(key, config);
        default:
            return checkTokenBucket(key, config);
    }
}

RateLimitResult RateLimiter::checkTokenBucket(const std::string& key, const RateLimitConfig& config) {
    if (config.useRedis) {
        return checkTokenBucketRedis(key, config);
    } else {
        return checkTokenBucketLocal(key, config);
    }
}

RateLimitResult RateLimiter::checkTokenBucketRedis(const std::string& key, const RateLimitConfig& config) {
    auto& redis = cache::RedisCache::getInstance();
    if (!redis.isConnected()) {
        return checkTokenBucketLocal(key, config);
    }

    // Lua脚本实现原子令牌桶限流
    static const char* LUA_TOKEN_BUCKET = R"(
        local key = KEYS[1]
        local capacity = tonumber(ARGV[1])
        local refill_rate = tonumber(ARGV[2])
        local now = tonumber(ARGV[3])
        local bucket = redis.call('HMGET', key, 'tokens', 'last_refill')
        local tokens = tonumber(bucket[1]) or capacity
        local last_refill = tonumber(bucket[2]) or now
        local elapsed = (now - last_refill) / 1000
        tokens = math.min(capacity, tokens + elapsed * refill_rate)
        if tokens >= 1 then
            tokens = tokens - 1
            redis.call('HMSET', key, 'tokens', tokens, 'last_refill', now)
            redis.call('EXPIRE', key, 60)
            return {1, math.floor(tokens), capacity}
        else
            redis.call('HMSET', key, 'tokens', tokens, 'last_refill', now)
            redis.call('EXPIRE', key, 60)
            return {0, 0, capacity}
        end
    )";

    std::string redisKey = buildRedisKey(key, config);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 使用shared_ptr确保回调中数据有效
    auto resultPtr = std::make_shared<RateLimitResult>();
    resultPtr->limit = config.capacity;
    auto gotResultPtr = std::make_shared<std::atomic<bool>>(false);

    try {
        auto redisClient = drogon::app().getRedisClient("default");
        redisClient->execCommandAsync(
            [resultPtr, gotResultPtr, config](const drogon::nosql::RedisResult& r) {
                if (r.type() == drogon::nosql::RedisResultType::kArray && r.asArray().size() >= 3) {
                    auto arr = r.asArray();
                    resultPtr->allowed = arr[0].asInteger() == 1;
                    resultPtr->remaining = static_cast<int>(arr[1].asInteger());
                    resultPtr->limit = static_cast<int>(arr[2].asInteger());
                    resultPtr->retryAfter = resultPtr->allowed ? 0 : static_cast<int>(1.0 / config.refillRate);
                    if (!resultPtr->allowed) resultPtr->reason = "Rate limit exceeded";
                    gotResultPtr->store(true);
                }
            },
            [](const std::exception& e) {
                LOG_WARN << "Redis rate limit error: " << e.what();
            },
            "EVAL %s 1 %s %d %d %lld",
            LUA_TOKEN_BUCKET, redisKey.c_str(), config.capacity, config.refillRate, now
        );

        // 等待结果（简单自旋，最多10ms）
        for (int i = 0; i < 100 && !gotResultPtr->load(); ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    } catch (...) {
        LOG_WARN << "Redis token bucket rate limit failed, falling back to local";
    }

    return gotResultPtr->load() ? *resultPtr : checkTokenBucketLocal(key, config);
}

RateLimitResult RateLimiter::checkTokenBucketLocal(const std::string& key, const RateLimitConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& bucket = localBuckets_[key];
    auto now = std::chrono::steady_clock::now();

    // 初始化
    if (bucket.tokens == 0 && bucket.lastRefill.time_since_epoch().count() == 0) {
        bucket.tokens = config.capacity;
        bucket.lastRefill = now;
    }

    // 填充令牌
    refillTokens(bucket, config);
    bucket.lastRefill = now;

    RateLimitResult result;
    result.limit = config.capacity;

    if (bucket.tokens >= 1) {
        bucket.tokens--;
        bucket.requestCount++;
        result.allowed = true;
        result.remaining = bucket.tokens;
        result.retryAfter = 0;
    } else {
        bucket.blockedCount++;
        result.allowed = false;
        result.remaining = 0;
        result.retryAfter = static_cast<int>(1.0 / config.refillRate);
        result.reason = "Rate limit exceeded";
    }

    return result;
}

void RateLimiter::refillTokens(LocalBucket& bucket, const RateLimitConfig& config) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - bucket.lastRefill).count() / 1000.0;

    int tokensToAdd = static_cast<int>(elapsed * config.refillRate);
    bucket.tokens = std::min(config.capacity, bucket.tokens + tokensToAdd);
}

RateLimitResult RateLimiter::checkSlidingWindow(const std::string& key, const RateLimitConfig& config) {
    if (config.useRedis) {
        return checkSlidingWindowRedis(key, config);
    } else {
        return checkSlidingWindowLocal(key, config);
    }
}

RateLimitResult RateLimiter::checkSlidingWindowRedis(const std::string& key, const RateLimitConfig& config) {
    auto& redis = cache::RedisCache::getInstance();
    if (!redis.isConnected()) {
        return checkSlidingWindowLocal(key, config);
    }

    // Lua脚本实现滑动窗口限流
    static const char* LUA_SLIDING_WINDOW = R"(
        local key = KEYS[1]
        local capacity = tonumber(ARGV[1])
        local window = tonumber(ARGV[2])
        local now = tonumber(ARGV[3])
        redis.call('ZREMRANGEBYSCORE', key, 0, now - window * 1000)
        local count = redis.call('ZCARD', key)
        if count < capacity then
            redis.call('ZADD', key, now, now .. math.random())
            redis.call('EXPIRE', key, window + 1)
            return {1, capacity - count - 1, capacity}
        else
            return {0, 0, capacity}
        end
    )";

    std::string redisKey = buildRedisKey(key, config);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto resultPtr = std::make_shared<RateLimitResult>();
    resultPtr->limit = config.capacity;
    auto gotResultPtr = std::make_shared<std::atomic<bool>>(false);

    try {
        auto redisClient = drogon::app().getRedisClient("default");
        redisClient->execCommandAsync(
            [resultPtr, gotResultPtr, config](const drogon::nosql::RedisResult& r) {
                if (r.type() == drogon::nosql::RedisResultType::kArray && r.asArray().size() >= 3) {
                    auto arr = r.asArray();
                    resultPtr->allowed = arr[0].asInteger() == 1;
                    resultPtr->remaining = static_cast<int>(arr[1].asInteger());
                    resultPtr->limit = static_cast<int>(arr[2].asInteger());
                    resultPtr->retryAfter = resultPtr->allowed ? 0 : config.windowSeconds;
                    if (!resultPtr->allowed) resultPtr->reason = "Rate limit exceeded";
                    gotResultPtr->store(true);
                }
            },
            [](const std::exception& e) {
                LOG_WARN << "Redis sliding window error: " << e.what();
            },
            "EVAL %s 1 %s %d %d %lld",
            LUA_SLIDING_WINDOW, redisKey.c_str(), config.capacity, config.windowSeconds, now
        );

        for (int i = 0; i < 100 && !gotResultPtr->load(); ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    } catch (...) {
        LOG_WARN << "Redis sliding window rate limit failed, falling back to local";
    }

    return gotResultPtr->load() ? *resultPtr : checkSlidingWindowLocal(key, config);
}

RateLimitResult RateLimiter::checkSlidingWindowLocal(const std::string& key, const RateLimitConfig& config) {
    // 简化实现：使用固定窗口
    return checkFixedWindowLocal(key, config);
}

RateLimitResult RateLimiter::checkFixedWindow(const std::string& key, const RateLimitConfig& config) {
    if (config.useRedis) {
        return checkFixedWindowRedis(key, config);
    } else {
        return checkFixedWindowLocal(key, config);
    }
}

RateLimitResult RateLimiter::checkFixedWindowRedis(const std::string& key, const RateLimitConfig& config) {
    auto& redis = cache::RedisCache::getInstance();
    if (!redis.isConnected()) {
        return checkFixedWindowLocal(key, config);
    }

    // Lua脚本实现固定窗口限流
    static const char* LUA_FIXED_WINDOW = R"(
        local key = KEYS[1]
        local capacity = tonumber(ARGV[1])
        local window = tonumber(ARGV[2])
        local count = redis.call('INCR', key)
        if count == 1 then
            redis.call('EXPIRE', key, window)
        end
        if count <= capacity then
            return {1, capacity - count, capacity}
        else
            return {0, 0, capacity}
        end
    )";

    std::string redisKey = buildRedisKey(key, config);
    auto resultPtr = std::make_shared<RateLimitResult>();
    resultPtr->limit = config.capacity;
    auto gotResultPtr = std::make_shared<std::atomic<bool>>(false);

    try {
        auto redisClient = drogon::app().getRedisClient("default");
        redisClient->execCommandAsync(
            [resultPtr, gotResultPtr, config](const drogon::nosql::RedisResult& r) {
                if (r.type() == drogon::nosql::RedisResultType::kArray && r.asArray().size() >= 3) {
                    auto arr = r.asArray();
                    resultPtr->allowed = arr[0].asInteger() == 1;
                    resultPtr->remaining = static_cast<int>(arr[1].asInteger());
                    resultPtr->limit = static_cast<int>(arr[2].asInteger());
                    resultPtr->retryAfter = resultPtr->allowed ? 0 : config.windowSeconds;
                    if (!resultPtr->allowed) resultPtr->reason = "Rate limit exceeded";
                    gotResultPtr->store(true);
                }
            },
            [](const std::exception& e) {
                LOG_WARN << "Redis fixed window error: " << e.what();
            },
            "EVAL %s 1 %s %d %d",
            LUA_FIXED_WINDOW, redisKey.c_str(), config.capacity, config.windowSeconds
        );

        for (int i = 0; i < 100 && !gotResultPtr->load(); ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    } catch (...) {}

    return gotResultPtr->load() ? *resultPtr : checkFixedWindowLocal(key, config);
}

RateLimitResult RateLimiter::checkFixedWindowLocal(const std::string& key, const RateLimitConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& bucket = localBuckets_[key];
    auto now = std::chrono::steady_clock::now();

    // 检查是否需要重置窗口
    if (bucket.lastRefill.time_since_epoch().count() == 0) {
        bucket.lastRefill = now;
        bucket.requestCount = 0;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - bucket.lastRefill).count();
    if (elapsed >= config.windowSeconds) {
        bucket.requestCount = 0;
        bucket.lastRefill = now;
    }

    RateLimitResult result;
    result.limit = config.capacity;

    if (bucket.requestCount < config.capacity) {
        bucket.requestCount++;
        result.allowed = true;
        result.remaining = config.capacity - bucket.requestCount;
        result.retryAfter = 0;
    } else {
        bucket.blockedCount++;
        result.allowed = false;
        result.remaining = 0;
        result.retryAfter = config.windowSeconds - elapsed;
        result.reason = "Rate limit exceeded";
    }

    return result;
}

std::string RateLimiter::buildRedisKey(const std::string& key, const RateLimitConfig& config) {
    return config.redisKeyPrefix + key;
}

RateLimiter::RateLimitStats RateLimiter::getStats(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    RateLimitStats stats{};
    auto it = localBuckets_.find(key);
    if (it != localBuckets_.end()) {
        stats.totalRequests = it->second.requestCount + it->second.blockedCount;
        stats.allowedRequests = it->second.requestCount;
        stats.blockedRequests = it->second.blockedCount;
        stats.blockRate = stats.totalRequests > 0 ?
            static_cast<float>(stats.blockedRequests) / stats.totalRequests : 0.0f;
        stats.lastRequestTime = std::chrono::duration_cast<std::chrono::seconds>(
            it->second.lastRefill.time_since_epoch()).count();
    }

    return stats;
}

void RateLimiter::reset(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    localBuckets_.erase(key);

    // 也清除 Redis
    auto& redis = cache::RedisCache::getInstance();
    try {
        redis.del(key);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to reset Redis rate limit: " << e.what();
    }
}

// ============================================================================
// RateLimitFilter 实现
// ============================================================================

void RateLimitFilter::apply(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    RateLimitLevel level
) {
    auto& limiter = RateLimiter::getInstance();

    // 提取限流键
    std::string key = extractKey(req, level);
    std::string endpoint = req->getPath();

    // 检查限流
    auto result = limiter.checkLimit(key, level, endpoint);

    if (!result.allowed) {
        // 返回限流响应
        callback(createRateLimitResponse(result));
        return;
    }

    // 请求被允许，返回nullptr表示继续处理
    callback(nullptr);
}

std::string RateLimitFilter::extractKey(
    const HttpRequestPtr& req,
    RateLimitLevel level
) {
    switch (level) {
        case RateLimitLevel::USER: {
            std::string userId = req->getAttributes()->get<std::string>("user_id");
            return userId.empty() ? "anonymous" : "user:" + userId;
        }
        case RateLimitLevel::IP: {
            std::string ip = req->getPeerAddr().toIp();
            return "ip:" + ip;
        }
        case RateLimitLevel::ENDPOINT: {
            return "endpoint:" + req->getPath();
        }
        case RateLimitLevel::GLOBAL: {
            return "global";
        }
        default:
            return "unknown";
    }
}

HttpResponsePtr RateLimitFilter::createRateLimitResponse(const RateLimitResult& result) {
    Json::Value data;
    data["code"] = 429;
    data["message"] = "请求过于频繁，请稍后再试";
    data["limit"] = result.limit;
    data["remaining"] = result.remaining;
    data["retry_after"] = result.retryAfter;

    auto resp = HttpResponse::newHttpJsonResponse(data);
    resp->setStatusCode(k429TooManyRequests);
    resp->addHeader("X-RateLimit-Limit", std::to_string(result.limit));
    resp->addHeader("X-RateLimit-Remaining", std::to_string(result.remaining));
    resp->addHeader("X-RateLimit-Reset", std::to_string(result.retryAfter));
    resp->addHeader("Retry-After", std::to_string(result.retryAfter));

    return resp;
}

} // namespace middleware
} // namespace heartlake
