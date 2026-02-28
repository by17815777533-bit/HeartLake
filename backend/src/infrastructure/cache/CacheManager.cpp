/**
 * 缓存管理器实现
 *
 * 进程内 LRU 缓存，用于热点数据的本地加速。
 * 底层数据结构：unordered_map + doubly-linked list，
 * get/set 均为 O(1) 均摊复杂度。
 * 读写通过 shared_mutex 保护，读操作使用 shared_lock 允许并发读取。
 */

#include "infrastructure/cache/CacheManager.h"
#include <mutex>

namespace heartlake::core::cache {

/// 动态调整缓存容量上限，调整后立即触发淘汰检查
void CacheManager::setMaxSize(size_t maxSize) {
    std::unique_lock lock(mutex_);
    maxSize_ = maxSize;
    evictIfNeeded();
}

/// 写入缓存条目，同时维护 LRU 链表顺序
/// 如果 key 已存在，先从链表中摘除旧节点再重新插入头部
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

/// JSON 便捷写入：序列化后委托给 set()
void CacheManager::setJson(const std::string& key, const Json::Value& value, int ttlSeconds) {
    Json::FastWriter writer;
    set(key, writer.write(value), ttlSeconds);
}

/// 读取缓存，命中时将节点提升到 LRU 链表头部（最近使用）
/// 如果条目已过期则惰性删除并返回 nullopt
std::optional<std::string> CacheManager::get(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end()) return std::nullopt;
    if (std::chrono::steady_clock::now() > it->second.expiry) {
        lruList_.erase(it->second.lruIt);
        cache_.erase(it);
        return std::nullopt;
    }
    // 提升到链表头部，标记为最近使用
    lruList_.erase(it->second.lruIt);
    lruList_.push_front(key);
    it->second.lruIt = lruList_.begin();
    return it->second.value;
}

/// JSON 便捷读取：反序列化 get() 返回的字符串
std::optional<Json::Value> CacheManager::getJson(const std::string& key) {
    auto val = get(key);
    if (!val) return std::nullopt;
    Json::Value json;
    Json::Reader reader;
    if (reader.parse(*val, json)) return json;
    return std::nullopt;
}

/// 精确删除单个缓存条目
void CacheManager::invalidate(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        lruList_.erase(it->second.lruIt);
        cache_.erase(it);
    }
}

/// 按前缀通配符批量失效，pattern 中 '*' 之前的部分作为前缀匹配
/// 典型用法：invalidatePattern("user:123:*") 清除某用户的全部缓存
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

/// 清空全部缓存
void CacheManager::clear() {
    std::unique_lock lock(mutex_);
    cache_.clear();
    lruList_.clear();
}

/// 返回当前缓存条目数（shared_lock 允许并发读）
size_t CacheManager::size() const {
    std::shared_lock lock(mutex_);
    return cache_.size();
}

/// LRU 淘汰：从链表尾部（最久未使用）开始逐个移除，直到满足容量限制
/// 注意：调用方必须已持有写锁
void CacheManager::evictIfNeeded() {
    while (maxSize_ > 0 && cache_.size() > maxSize_ && !lruList_.empty()) {
        auto& oldest = lruList_.back();
        cache_.erase(oldest);
        lruList_.pop_back();
    }
}

} // namespace heartlake::core::cache
