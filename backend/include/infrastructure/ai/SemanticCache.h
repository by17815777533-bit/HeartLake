/**
 * @file SemanticCache.h
 * @brief 语义缓存 - 基于向量相似度的智能缓存（前沿技术）
 *
 * 核心创新：不仅缓存精确匹配，还能通过语义相似度命中近似查询
 * 预期效果：缓存命中率从30%提升到60-70%，减少40-50% AI API调用
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace heartlake {
namespace ai {

struct CacheEntry {
    std::string query;
    std::vector<float> embedding;
    std::string response;
    int64_t timestamp;
    int hitCount = 0;
};

struct SemanticCacheStats {
    size_t exactHits = 0;
    size_t semanticHits = 0;
    size_t misses = 0;
    float hitRate() const {
        size_t total = exactHits + semanticHits + misses;
        return total > 0 ? static_cast<float>(exactHits + semanticHits) / static_cast<float>(total) : 0.0f;
    }
    size_t estimatedSavings() const { return (exactHits + semanticHits) * 2; }
};

class SemanticCache {
public:
    static SemanticCache& getInstance() {
        static SemanticCache instance;
        return instance;
    }

    void initialize(float similarityThreshold = 0.92f, size_t maxSize = 5000, int ttlSeconds = 86400) {
        similarityThreshold_ = similarityThreshold;
        maxSize_ = maxSize;
        ttlSeconds_ = ttlSeconds;
    }

    /**
     * @brief 查询缓存（L1精确匹配 + L2语义匹配）
     * @return true if cache hit
     */
    bool get(const std::string& query, const std::vector<float>& queryEmbedding, std::string& response);

    void put(const std::string& query, const std::vector<float>& embedding, const std::string& response);

    SemanticCacheStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    void clearStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = SemanticCacheStats{};
    }

private:
    SemanticCache() = default;

    std::string computeHash(const std::string& query);
    void evictLFU();
    void purgeExpired();

    float similarityThreshold_ = 0.92f;
    size_t maxSize_ = 5000;
    int ttlSeconds_ = 86400;

    std::unordered_map<std::string, CacheEntry> exactCache_;
    std::vector<CacheEntry> semanticIndex_;
    SemanticCacheStats stats_;
    mutable std::mutex mutex_;
};

} // namespace ai
} // namespace heartlake
