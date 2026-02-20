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
#include <chrono>
#include "AdvancedEmbeddingEngine.h"

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
        return total > 0 ? float(exactHits + semanticHits) / total : 0.0f;
    }
    size_t estimatedSavings() const { return (exactHits + semanticHits) * 2; } // 每次命中约省2分钱
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
        initialized_ = true;
    }

    /**
     * @brief 查询缓存（L1精确匹配 + L2语义匹配）
     * @return true if cache hit
     */
    bool get(const std::string& query, const std::vector<float>& queryEmbedding, std::string& response) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now().time_since_epoch();
        int64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(now).count();

        // L1: 精确匹配
        auto hash = computeHash(query);
        auto it = exactCache_.find(hash);
        if (it != exactCache_.end() && (currentTime - it->second.timestamp) < ttlSeconds_) {
            response = it->second.response;
            it->second.hitCount++;
            stats_.exactHits++;
            return true;
        }

        // L2: 语义匹配
        float maxSim = 0;
        CacheEntry* bestMatch = nullptr;
        for (auto& entry : semanticIndex_) {
            if ((currentTime - entry.timestamp) >= ttlSeconds_) continue;

            float sim = AdvancedEmbeddingEngine::cosineSimilarity(queryEmbedding, entry.embedding);
            if (sim > maxSim && sim >= similarityThreshold_) {
                maxSim = sim;
                bestMatch = &entry;
            }
        }

        if (bestMatch) {
            response = bestMatch->response;
            bestMatch->hitCount++;
            stats_.semanticHits++;
            return true;
        }

        stats_.misses++;
        return false;
    }

    void put(const std::string& query, const std::vector<float>& embedding, const std::string& response) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now().time_since_epoch();
        int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

        CacheEntry entry{query, embedding, response, timestamp, 0};

        // L1: 精确缓存
        exactCache_[computeHash(query)] = entry;

        // L2: 语义索引
        if (semanticIndex_.size() >= maxSize_) {
            evictLRU();
        }
        semanticIndex_.push_back(entry);
    }

    SemanticCacheStats getStats() const { return stats_; }
    void clearStats() { stats_ = SemanticCacheStats{}; }

private:
    SemanticCache() = default;

    std::string computeHash(const std::string& query) {
        // 简单哈希，生产环境可用xxHash
        std::hash<std::string> hasher;
        return std::to_string(hasher(query));
    }

    void evictLRU() {
        if (semanticIndex_.empty()) return;
        auto minIt = semanticIndex_.begin();
        for (auto it = semanticIndex_.begin(); it != semanticIndex_.end(); ++it) {
            if (it->hitCount < minIt->hitCount) minIt = it;
        }
        semanticIndex_.erase(minIt);
    }

    float similarityThreshold_ = 0.92f;
    size_t maxSize_ = 5000;
    int ttlSeconds_ = 86400;
    bool initialized_ = false;

    std::unordered_map<std::string, CacheEntry> exactCache_;
    std::vector<CacheEntry> semanticIndex_;
    SemanticCacheStats stats_;
    mutable std::mutex mutex_;
};

} // namespace ai
} // namespace heartlake
