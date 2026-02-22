/**
 * @file CacheManager.cpp
 * @brief 缓存管理器实现
 */

#include "infrastructure/cache/CacheManager.h"
#include <mutex>

namespace heartlake::core::cache {

void CacheManager::setMaxSize(size_t maxSize) {
    std::unique_lock lock(mutex_);
    maxSize_ = maxSize;
    evictIfNeeded();
}

void CacheManager::set(const std::string& key, const std::string& value, int ttlSeconds) {
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

void CacheManager::setJson(const std::string& key, const Json::Value& value, int ttlSeconds) {
    Json::FastWriter writer;
    set(key, writer.write(value), ttlSeconds);
}

std::optional<std::string> CacheManager::get(const std::string& key) {
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

std::optional<Json::Value> CacheManager::getJson(const std::string& key) {
    auto val = get(key);
    if (!val) return std::nullopt;
    Json::Value json;
    Json::Reader reader;
    if (reader.parse(*val, json)) return json;
    return std::nullopt;
}

void CacheManager::invalidate(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        lruList_.erase(it->second.lruIt);
        cache_.erase(it);
    }
}

void CacheManager::invalidatePattern(const std::string& pattern) {
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

void CacheManager::clear() {
    std::unique_lock lock(mutex_);
    cache_.clear();
    lruList_.clear();
}

size_t CacheManager::size() const {
    std::shared_lock lock(mutex_);
    return cache_.size();
}

void CacheManager::evictIfNeeded() {
    while (maxSize_ > 0 && cache_.size() > maxSize_ && !lruList_.empty()) {
        auto& oldest = lruList_.back();
        cache_.erase(oldest);
        lruList_.pop_back();
    }
}

} // namespace heartlake::core::cache
