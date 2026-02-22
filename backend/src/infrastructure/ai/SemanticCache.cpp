/**
 * @file SemanticCache.cpp
 * @brief 语义缓存实现
 */

#include "infrastructure/ai/SemanticCache.h"
#include <chrono>
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"

namespace heartlake {
namespace ai {

bool SemanticCache::get(const std::string& query, const std::vector<float>& queryEmbedding, std::string& response) {
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

void SemanticCache::put(const std::string& query, const std::vector<float>& embedding, const std::string& response) {
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

std::string SemanticCache::computeHash(const std::string& query) {
    // 简单哈希，生产环境可用xxHash
    std::hash<std::string> hasher;
    return std::to_string(hasher(query));
}

void SemanticCache::evictLRU() {
    if (semanticIndex_.empty()) return;
    auto minIt = semanticIndex_.begin();
    for (auto it = semanticIndex_.begin(); it != semanticIndex_.end(); ++it) {
        if (it->hitCount < minIt->hitCount) minIt = it;
    }
    semanticIndex_.erase(minIt);
}

} // namespace ai
} // namespace heartlake
