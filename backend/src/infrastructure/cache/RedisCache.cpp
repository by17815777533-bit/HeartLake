/**
 * RedisCache 模块实现
 */
#include "infrastructure/cache/RedisCache.h"
#include <drogon/drogon.h>
#include <functional>
#include <future>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <mutex>

using namespace heartlake::cache;

RedisCache& RedisCache::getInstance() {
    static RedisCache instance;
    return instance;
}

void RedisCache::initialize(const std::string& host, int port,
                           const std::string& password, int db,
                           const RedisPoolConfig& poolConfig) {
    host_ = host;
    port_ = port;
    password_ = password;
    db_ = db;
    poolConfig_ = poolConfig;

    tryConnect();

    if (!connected_) {
        LOG_INFO << "Redis cache initialized with memory fallback (host: " << host << ":" << port << ")";
    }
}

void RedisCache::tryConnect() {
    try {
        auto redisClient = drogon::app().getRedisClient("default");
        if (redisClient) {
            connected_ = true;
            LOG_INFO << "Redis connected successfully (host: " << host_ << ":" << port_ << ")";
        }
    } catch (const std::exception& e) {
        connected_ = false;
        LOG_WARN << "Redis connection failed, using memory fallback: " << e.what();
    }
}

void RedisCache::markDisconnected() {
    if (connected_) {
        connected_ = false;
        LOG_WARN << "Redis disconnected, switching to memory fallback";
        scheduleReconnect();
    }
}

void RedisCache::scheduleReconnect() {
    if (reconnecting_) return;
    reconnecting_ = true;

    drogon::app().getLoop()->runAfter(reconnectDelaySeconds_, [this]() {
        LOG_INFO << "Attempting Redis reconnection...";
        tryConnect();
        reconnecting_ = false;

        if (!connected_ && reconnectDelaySeconds_ < 60) {
            reconnectDelaySeconds_ = std::min(reconnectDelaySeconds_ * 2, 60);
            scheduleReconnect();
        } else if (connected_) {
            reconnectDelaySeconds_ = 5;
            LOG_INFO << "Redis reconnected successfully";
        }
    });
}

void RedisCache::set(const std::string& key, const std::string& value,
                     std::function<void(bool)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (callback) callback(result.type() != drogon::nosql::RedisResultType::kError);
                },
                [this, callback, fullKey, value](const std::exception& e) {
                    LOG_ERROR << "Redis SET error: " << e.what();
                    markDisconnected();
                    fallbackSet(fullKey, value, 0);
                    if (callback) callback(true);
                },
                "SET %s %s", fullKey.c_str(), value.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis SET dispatch failed: " << e.what();
            fallbackSet(fullKey, value, 0);
            if (callback) callback(true);
        }
    } else {
        fallbackSet(fullKey, value, 0);
        if (callback) callback(true);
    }
}

void RedisCache::setEx(const std::string& key, const std::string& value, int ttlSeconds,
                       std::function<void(bool)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (callback) callback(result.type() != drogon::nosql::RedisResultType::kError);
                },
                [this, callback, fullKey, value, ttlSeconds](const std::exception& e) {
                    LOG_ERROR << "Redis SETEX error: " << e.what();
                    markDisconnected();
                    fallbackSet(fullKey, value, ttlSeconds);
                    if (callback) callback(true);
                },
                "SETEX %s %d %s", fullKey.c_str(), ttlSeconds, value.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis SETEX dispatch failed: " << e.what();
            fallbackSet(fullKey, value, ttlSeconds);
            if (callback) callback(true);
        }
    } else {
        fallbackSet(fullKey, value, ttlSeconds);
        if (callback) callback(true);
    }
}

void RedisCache::get(const std::string& key,
                     std::function<void(const std::string&, bool)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (result.type() == drogon::nosql::RedisResultType::kNil) {
                        callback("", false);
                    } else {
                        callback(result.asString(), true);
                    }
                },
                [callback, fullKey, this](const std::exception& e) {
                    LOG_ERROR << "Redis GET error: " << e.what();
                    markDisconnected();
                    auto [value, exists] = fallbackGet(fullKey);
                    callback(value, exists);
                },
                "GET %s", fullKey.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis GET dispatch failed: " << e.what();
            auto [value, exists] = fallbackGet(fullKey);
            callback(value, exists);
        }
    } else {
        auto [value, exists] = fallbackGet(fullKey);
        callback(value, exists);
    }
}

void RedisCache::del(const std::string& key, std::function<void(bool)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult&) {
                    if (callback) callback(true);
                },
                [this, callback, fullKey](const std::exception& e) {
                    LOG_ERROR << "Redis DEL error: " << e.what();
                    markDisconnected();
                    {
                        std::lock_guard<std::mutex> lock(cacheMutex_);
                        auto it = memoryCache_.find(fullKey);
                        if (it != memoryCache_.end()) {
                            lruList_.erase(it->second);
                            memoryCache_.erase(it);
                        }
                    }
                    if (callback) callback(true);
                },
                "DEL %s", fullKey.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis DEL dispatch failed: " << e.what();
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                auto it = memoryCache_.find(fullKey);
                if (it != memoryCache_.end()) {
                    lruList_.erase(it->second);
                    memoryCache_.erase(it);
                }
            }
            if (callback) callback(true);
        }
    } else {
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            auto it = memoryCache_.find(fullKey);
            if (it != memoryCache_.end()) {
                lruList_.erase(it->second);
                memoryCache_.erase(it);
            }
        }
        if (callback) callback(true);
    }
}

void RedisCache::setJson(const std::string& key, const Json::Value& value, int ttlSeconds,
                         std::function<void(bool)> callback) {
    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, value);
    
    if (ttlSeconds > 0) {
        setEx(key, jsonStr, ttlSeconds, callback);
    } else {
        set(key, jsonStr, callback);
    }
}

void RedisCache::getJson(const std::string& key,
                         std::function<void(const Json::Value&, bool)> callback) {
    get(key, [callback](const std::string& value, bool exists) {
        if (!exists) {
            callback(Json::Value(), false);
            return;
        }
        
        Json::Value json;
        Json::CharReaderBuilder reader;
        std::istringstream stream(value);
        std::string errors;
        
        if (Json::parseFromStream(reader, stream, &json, &errors)) {
            callback(json, true);
        } else {
            callback(Json::Value(), false);
        }
    });
}

void RedisCache::incr(const std::string& key, std::function<void(int64_t)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (callback) callback(result.asInteger());
                },
                [this, callback](const std::exception& e) {
                    LOG_ERROR << "Redis INCR error: " << e.what();
                    markDisconnected();
                    if (callback) callback(0);
                },
                "INCR %s", fullKey.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis INCR dispatch failed: " << e.what();
            if (callback) callback(0);
        }
    } else {
        if (callback) callback(0);
    }
}

void RedisCache::incrBy(const std::string& key, int64_t delta,
                        std::function<void(int64_t)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;

    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (callback) callback(result.asInteger());
                },
                [this, callback](const std::exception& e) {
                    LOG_ERROR << "Redis INCRBY error: " << e.what();
                    markDisconnected();
                    if (callback) callback(0);
                },
                "INCRBY %s %lld", fullKey.c_str(), delta
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis INCRBY dispatch failed: " << e.what();
            if (callback) callback(0);
        }
    } else {
        if (callback) callback(0);
    }
}

void RedisCache::eval(const std::string& script, const std::vector<std::string>& keys,
                      const std::vector<std::string>& args,
                      std::function<void(int64_t)> callback) {
    if (!connected_) {
        if (callback) callback(-1);
        return;
    }
    try {
        auto redisClient = drogon::app().getRedisClient("default");
        std::string cmd = "EVAL " + script + " " + std::to_string(keys.size());
        for (const auto& k : keys) cmd += " " + std::string(KEY_PREFIX) + k;
        for (const auto& a : args) cmd += " " + a;
        redisClient->execCommandAsync(
            [callback](const drogon::nosql::RedisResult& result) {
                if (callback) callback(result.asInteger());
            },
            [this, callback](const std::exception& e) {
                LOG_ERROR << "Redis EVAL error: " << e.what();
                markDisconnected();
                if (callback) callback(-1);
            },
            cmd.c_str()
        );
    } catch (const std::exception& e) {
        LOG_WARN << "Redis eval dispatch failed: " << e.what();
        if (callback) callback(-1);
    }
}

void RedisCache::expire(const std::string& key, int seconds,
                        std::function<void(bool)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (callback) callback(result.asInteger() == 1);
                },
                [callback](const std::exception&) {
                    if (callback) callback(false);
                },
                "EXPIRE %s %d", fullKey.c_str(), seconds
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis EXPIRE dispatch failed: " << e.what();
            if (callback) callback(false);
        }
    } else {
        if (callback) callback(false);
    }
}

void RedisCache::ttl(const std::string& key, std::function<void(int)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (callback) callback(static_cast<int>(result.asInteger()));
                },
                [callback](const std::exception&) {
                    if (callback) callback(-2);
                },
                "TTL %s", fullKey.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis TTL dispatch failed: " << e.what();
            if (callback) callback(-2);
        }
    } else {
        if (callback) callback(-2);
    }
}

void RedisCache::exists(const std::string& key, std::function<void(bool)> callback) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    if (callback) callback(result.asInteger() == 1);
                },
                [callback](const std::exception&) {
                    if (callback) callback(false);
                },
                "EXISTS %s", fullKey.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis EXISTS dispatch failed: " << e.what();
            if (callback) callback(false);
        }
    } else {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        bool found = memoryCache_.find(fullKey) != memoryCache_.end();
        if (found) {
            // Check expiry
            auto& entry = memoryCache_[fullKey]->second;
            if (std::chrono::steady_clock::now() > entry.expireTime) {
                lruList_.erase(memoryCache_[fullKey]);
                memoryCache_.erase(fullKey);
                found = false;
            }
        }
        if (callback) callback(found);
    }
}

void RedisCache::keys(const std::string& pattern, std::function<void(const std::vector<std::string>&)> callback) {
    std::string fullPattern = std::string(KEY_PREFIX) + pattern;
    if (connected_) {
        try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [callback](const drogon::nosql::RedisResult& result) {
                    std::vector<std::string> keys;
                    if (result.type() == drogon::nosql::RedisResultType::kArray) {
                        for (auto& item : result.asArray()) {
                            keys.push_back(item.asString());
                        }
                    }
                    if (callback) callback(keys);
                },
                [callback](const std::exception&) {
                    if (callback) callback({});
                },
                "KEYS %s", fullPattern.c_str()
            );
        } catch (const std::exception& e) {
            LOG_WARN << "Redis KEYS dispatch failed: " << e.what();
            if (callback) callback({});
        }
    } else {
        if (callback) callback({});
    }
}

std::string RedisCache::makeKey(const std::string& category, const std::string& id) {
    return category + id;
}

void RedisCache::fallbackSet(const std::string& key, const std::string& value, int ttlSeconds) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto expireTime = ttlSeconds > 0
        ? std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds)
        : std::chrono::steady_clock::time_point::max();

    // If key already exists, remove old entry from LRU list
    auto it = memoryCache_.find(key);
    if (it != memoryCache_.end()) {
        lruList_.erase(it->second);
        memoryCache_.erase(it);
    }

    // Evict LRU entries if at capacity
    while (memoryCache_.size() >= MAX_CACHE_SIZE && !lruList_.empty()) {
        auto& oldest = lruList_.back();
        memoryCache_.erase(oldest.first);
        lruList_.pop_back();
    }

    // Insert at front (most recently used)
    lruList_.emplace_front(key, CacheEntry{value, expireTime});
    memoryCache_[key] = lruList_.begin();
}

std::pair<std::string, bool> RedisCache::fallbackGet(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = memoryCache_.find(key);
    if (it == memoryCache_.end()) {
        return {"", false};
    }

    auto& entry = it->second->second;
    if (std::chrono::steady_clock::now() > entry.expireTime) {
        lruList_.erase(it->second);
        memoryCache_.erase(it);
        return {"", false};
    }

    // Move to front (most recently used)
    lruList_.splice(lruList_.begin(), lruList_, it->second);
    return {entry.value, true};
}

// 同步方法实现
std::string RedisCache::getSync(const std::string& key) {
    auto [value, exists] = fallbackGet(std::string(KEY_PREFIX) + key);
    return value;
}

void RedisCache::setexSync(const std::string& key, const std::string& value, int ttlSeconds) {
    fallbackSet(std::string(KEY_PREFIX) + key, value, ttlSeconds);
}

// 协程方法实现
drogon::Task<std::pair<std::string, bool>> RedisCache::getCoro(const std::string& key) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    if (!connected_) {
        co_return fallbackGet(fullKey);
    }
    try {
        auto redisClient = drogon::app().getRedisClient("default");
        auto result = co_await redisClient->execCommandCoro("GET %s", fullKey.c_str());
        if (result.type() == drogon::nosql::RedisResultType::kNil) {
            co_return std::make_pair("", false);
        }
        co_return std::make_pair(result.asString(), true);
    } catch (const std::exception& e) {
        LOG_WARN << "Redis getCoro failed: " << e.what();
        co_return fallbackGet(fullKey);
    }
}

drogon::Task<void> RedisCache::setExCoro(const std::string& key, const std::string& value, int ttlSeconds) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    if (!connected_) {
        fallbackSet(fullKey, value, ttlSeconds);
        co_return;
    }
    try {
        auto redisClient = drogon::app().getRedisClient("default");
        co_await redisClient->execCommandCoro("SETEX %s %d %s", fullKey.c_str(), ttlSeconds, value.c_str());
    } catch (const std::exception& e) {
        LOG_WARN << "Redis setCoro failed: " << e.what();
        fallbackSet(fullKey, value, ttlSeconds);
    }
}

drogon::Task<int64_t> RedisCache::ttlCoro(const std::string& key) {
    std::string fullKey = std::string(KEY_PREFIX) + key;
    if (!connected_) co_return -2;
    try {
        auto redisClient = drogon::app().getRedisClient("default");
        if (!redisClient) co_return -2;
        auto result = co_await redisClient->execCommandCoro("TTL %s", fullKey.c_str());
        co_return result.asInteger();
    } catch (const std::exception& e) {
        LOG_WARN << "Redis ttlCoro failed: " << e.what();
        co_return -2;
    }
}

AIResponseCache& AIResponseCache::getInstance() {
    static AIResponseCache instance;
    return instance;
}

std::string AIResponseCache::hashText(const std::string& text) {
    // 简单hash
    std::hash<std::string> hasher;
    return std::to_string(hasher(text));
}

void AIResponseCache::cacheSentiment(const std::string& text, float score, const std::string& mood) {
    Json::Value data;
    data["score"] = score;
    data["mood"] = mood;
    
    std::string key = std::string(RedisCache::AI_CACHE) + "sentiment:" + hashText(text);
    RedisCache::getInstance().setJson(key, data, RedisCache::TTL_LONG);
}

bool AIResponseCache::getSentiment(const std::string& text, float& score, std::string& mood) {
    std::string key = std::string(RedisCache::AI_CACHE) + "sentiment:" + hashText(text);
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    auto pScore = std::make_shared<float>(0.0f);
    auto pMood = std::make_shared<std::string>();

    RedisCache::getInstance().getJson(key, [promise, pScore, pMood](const Json::Value& value, bool exists) {
        if (exists) {
            *pScore = value["score"].asFloat();
            *pMood = value["mood"].asString();
        }
        promise->set_value(exists);
    });

    bool found = future.get();
    if (found) {
        score = *pScore;
        mood = *pMood;
    }
    return found;
}

void AIResponseCache::cacheReply(const std::string& context, const std::string& reply) {
    std::string key = std::string(RedisCache::AI_CACHE) + "reply:" + hashText(context);
    RedisCache::getInstance().setEx(key, reply, RedisCache::TTL_LONG);
}

bool AIResponseCache::getReply(const std::string& context, std::string& reply) {
    std::string key = std::string(RedisCache::AI_CACHE) + "reply:" + hashText(context);
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    auto pReply = std::make_shared<std::string>();

    RedisCache::getInstance().get(key, [promise, pReply](const std::string& value, bool exists) {
        if (exists) {
            *pReply = value;
        }
        promise->set_value(exists);
    });

    bool found = future.get();
    if (found) {
        reply = *pReply;
    }
    return found;
}

void AIResponseCache::cacheModeration(const std::string& text, bool passed, const std::string& reason) {
    Json::Value data;
    data["passed"] = passed;
    data["reason"] = reason;
    
    std::string key = std::string(RedisCache::AI_CACHE) + "moderate:" + hashText(text);
    RedisCache::getInstance().setJson(key, data, RedisCache::TTL_DAY);
}

bool AIResponseCache::getModeration(const std::string& text, bool& passed, std::string& reason) {
    std::string key = std::string(RedisCache::AI_CACHE) + "moderate:" + hashText(text);
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    auto pPassed = std::make_shared<bool>(false);
    auto pReason = std::make_shared<std::string>();

    RedisCache::getInstance().getJson(key, [promise, pPassed, pReason](const Json::Value& value, bool exists) {
        if (exists) {
            *pPassed = value["passed"].asBool();
            *pReason = value["reason"].asString();
        }
        promise->set_value(exists);
    });

    bool found = future.get();
    if (found) {
        passed = *pPassed;
        reason = *pReason;
    }
    return found;
}
