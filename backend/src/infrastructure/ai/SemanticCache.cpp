/**
 * @file SemanticCache.cpp
 * @brief 语义缓存实现
 *
 * L2 语义搜索已从 O(n) 线性扫描升级为 HNSW ANN 检索。
 * 插入缓存时同步插入 HNSW 索引，查询时用 HNSW 做近似最近邻检索，
 * 再将 L2 距离转换为 cosine similarity 与阈值比较。
 *
 * 归一化技巧：存入 HNSW 前对向量做 L2 归一化，
 * 这样 squared_L2 = 2 - 2*cosine，可直接换算。
 */

#include "infrastructure/ai/SemanticCache.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"

#include <chrono>
#include <cmath>
#include <algorithm>

namespace heartlake {
namespace ai {

// ============================================================================
// 内部工具
// ============================================================================

void SemanticCache::initHNSWIndex() {
    hnswIndex_ = std::make_unique<HNSWIndex>();
    // 语义缓存场景：精度优先，M=16 / efSearch=64 足够覆盖高相似度候选
    hnswIndex_->configure(/*m=*/16, /*mMax0=*/32, /*efConstruction=*/200, /*efSearch=*/64);
}

std::string SemanticCache::makeCacheNodeId(size_t seq) {
    return "sc_" + std::to_string(seq);
}

std::vector<float> SemanticCache::normalizeVec(const std::vector<float>& v) {
    float norm = 0.0f;
    for (float x : v) norm += x * x;
    norm = std::sqrt(norm);
    if (norm < 1e-12f) return v;  // 零向量不归一化
    std::vector<float> out(v.size());
    float invNorm = 1.0f / norm;
    for (size_t i = 0; i < v.size(); ++i) {
        out[i] = v[i] * invNorm;
    }
    return out;
}

// ============================================================================
// 查询：L1 精确匹配 + L2 HNSW ANN 语义匹配
// ============================================================================

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

    // L2: HNSW ANN 语义匹配
    if (!hnswIndex_ || hnswIndex_->getVectorCount() == 0) {
        stats_.misses++;
        return false;
    }

    // 归一化查询向量后检索，取 top-5 候选再精确比较 cosine
    auto normQuery = normalizeVec(queryEmbedding);
    auto candidates = hnswIndex_->searchKNN(normQuery, 5);

    CacheEntry* bestMatch = nullptr;
    float maxSim = 0.0f;

    for (const auto& cand : candidates) {
        auto entryIt = semanticEntries_.find(cand.id);
        if (entryIt == semanticEntries_.end()) continue;

        auto& entry = entryIt->second;
        // 跳过过期条目
        if ((currentTime - entry.timestamp) >= ttlSeconds_) continue;

        // 归一化向量下 squared_L2 = 2 - 2*cosine => cosine = 1 - dist/2
        float cosine = 1.0f - cand.distance / 2.0f;
        if (cosine > maxSim && cosine >= similarityThreshold_) {
            maxSim = cosine;
            bestMatch = &entry;
            if (cosine >= 0.99f) break;  // 近似完美匹配，提前终止
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

// ============================================================================
// 插入：同时写入精确缓存和 HNSW 索引
// ============================================================================

void SemanticCache::put(const std::string& query, const std::vector<float>& embedding, const std::string& response) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now().time_since_epoch();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    CacheEntry entry{query, embedding, response, timestamp, 0};

    // L1: 精确缓存
    exactCache_[computeHash(query)] = entry;

    // L2: HNSW 语义索引
    purgeExpired();
    if (semanticEntries_.size() >= maxSize_) {
        evictLFU();
    }

    // 延迟初始化 HNSW（首次 put 时若 initialize 未调用也能工作）
    if (!hnswIndex_) {
        initHNSWIndex();
    }

    std::string nodeId = makeCacheNodeId(nextSeq_++);
    auto normVec = normalizeVec(embedding);
    hnswIndex_->addVector(nodeId, normVec);
    semanticEntries_[nodeId] = std::move(entry);
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string SemanticCache::computeHash(const std::string& query) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(query));
}

void SemanticCache::evictLFU() {
    if (semanticEntries_.empty()) return;

    // 找到命中次数最少的条目
    auto minIt = semanticEntries_.begin();
    for (auto it = semanticEntries_.begin(); it != semanticEntries_.end(); ++it) {
        if (it->second.hitCount < minIt->second.hitCount) {
            minIt = it;
        }
    }

    // 从 HNSW 索引中移除对应向量
    hnswIndex_->removeVector(minIt->first);
    semanticEntries_.erase(minIt);
}

void SemanticCache::purgeExpired() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    int64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    // 清理 exactCache_ 中过期条目
    for (auto it = exactCache_.begin(); it != exactCache_.end(); ) {
        if ((currentTime - it->second.timestamp) >= ttlSeconds_) {
            it = exactCache_.erase(it);
        } else {
            ++it;
        }
    }

    // 清理 semanticEntries_ 中过期条目，同步从 HNSW 移除
    for (auto it = semanticEntries_.begin(); it != semanticEntries_.end(); ) {
        if ((currentTime - it->second.timestamp) >= ttlSeconds_) {
            if (hnswIndex_) {
                hnswIndex_->removeVector(it->first);
            }
            it = semanticEntries_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ai
} // namespace heartlake
