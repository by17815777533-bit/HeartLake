/**
 * @file CacheManager.h
 * @brief 缓存管理器 - 支持LRU淘汰和容量限制
 */

#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <optional>
#include <json/json.h>

namespace heartlake::core::cache {

class CacheManager {
public:
    static CacheManager& getInstance() {
        static CacheManager instance;
        return instance;
    }

    void setMaxSize(size_t maxSize) {
        std::unique_lock lock(mutex_);
        maxSize_ = maxSize;
        evictIfNeeded();
    }

    void set(const std::string& key, const std::string& value, int ttlSeconds = 300) {
        std::unique_lock lock(mutex_);
        auto expiry = std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds);

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            lruList_.erase(it->second.lruIt);
        }

        lruList_.push_front(key);
        cache_[key] = {value, expiry, lruList_.begin()};

        evictIfNeeded();
    }

    void setJson(const std::string& key, const Json::Value& value, int ttlSeconds = 300) {
        Json::FastWriter writer;
        set(key, writer.write(value), ttlSeconds);
    }

    std::optional<std::string> get(const std::string& key) {
        std::unique_lock lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) return std::nullopt;
        if (std::chrono::steady_clock::now() > it->second.expiry) {
            lruList_.erase(it->second.lruIt);
            cache_.erase(it);
            return std::nullopt;
        }
        // Move to front (most recently used)
        lruList_.erase(it->second.lruIt);
        lruList_.push_front(key);
        it->second.lruIt = lruList_.begin();
        return it->second.value;
    }

    std::optional<Json::Value> getJson(const std::string& key) {
        auto val = get(key);
        if (!val) return std::nullopt;
        Json::Value json;
        Json::Reader reader;
        if (reader.parse(*val, json)) return json;
        return std::nullopt;
    }

    void invalidate(const std::string& key) {
        std::unique_lock lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            lruList_.erase(it->second.lruIt);
            cache_.erase(it);
        }
    }

    void invalidatePattern(const std::string& pattern) {
        std::unique_lock lock(mutex_);
        std::string prefix = pattern.substr(0, pattern.find('*'));
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (it->first.find(prefix) == 0) {
                lruList_.erase(it->second.lruIt);
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void clear() {
        std::unique_lock lock(mutex_);
        cache_.clear();
        lruList_.clear();
    }

    size_t size() const {
        std::shared_lock lock(mutex_);
        return cache_.size();
    }

private:
    CacheManager() = default;

    void evictIfNeeded() {
        while (maxSize_ > 0 && cache_.size() > maxSize_ && !lruList_.empty()) {
            auto& oldest = lruList_.back();
            cache_.erase(oldest);
            lruList_.pop_back();
        }
    }

    struct CacheEntry {
        std::string value;
        std::chrono::steady_clock::time_point expiry;
        std::list<std::string>::iterator lruIt;
    };

    std::unordered_map<std::string, CacheEntry> cache_;
    std::list<std::string> lruList_;
    mutable std::shared_mutex mutex_;
    size_t maxSize_ = 10000;  // Default max 10000 entries
};

} // namespace heartlake::core::cache
