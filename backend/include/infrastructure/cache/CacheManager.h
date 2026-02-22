/**
 * @file CacheManager.h
 * @brief 缓存管理器 - 支持LRU淘汰和容量限制
 */

#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <shared_mutex>
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

    void setMaxSize(size_t maxSize);
    void set(const std::string& key, const std::string& value, int ttlSeconds = 300);
    void setJson(const std::string& key, const Json::Value& value, int ttlSeconds = 300);
    std::optional<std::string> get(const std::string& key);
    std::optional<Json::Value> getJson(const std::string& key);
    void invalidate(const std::string& key);
    void invalidatePattern(const std::string& pattern);
    void clear();
    size_t size() const;

private:
    CacheManager() = default;

    void evictIfNeeded();

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
