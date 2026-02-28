/**
 * 语义缓存 - 基于向量相似度的智能缓存（前沿技术）
 *
 * 核心创新：不仅缓存精确匹配，还能通过语义相似度命中近似查询。
 * L2 语义搜索已从 O(n) 线性扫描升级为 HNSW ANN 检索（O(log n)），
 * 复用项目已有的 HNSWIndex 子系统，大幅降低高缓存量下的查询延迟。
 *
 * 预期效果：缓存命中率从30%提升到60-70%，减少40-50% AI API调用
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>

#include "infrastructure/ai/HNSWIndex.h"

namespace heartlake {
namespace ai {

/**
 * @brief 缓存条目，存储查询文本、embedding 向量、响应内容及访问统计
 */
struct CacheEntry {
    std::string query;                ///< 原始查询文本
    std::vector<float> embedding;     ///< 查询的 embedding 向量
    std::string response;             ///< 缓存的响应内容
    int64_t timestamp;                ///< 写入时间戳（epoch seconds）
    int hitCount = 0;                 ///< 命中次数，用于 LFU 淘汰
};

/**
 * @brief 语义缓存命中率统计
 */
struct SemanticCacheStats {
    size_t exactHits = 0;             ///< L1 精确匹配命中次数
    size_t semanticHits = 0;          ///< L2 语义相似度命中次数
    size_t misses = 0;                ///< 未命中次数

    /** @brief 计算总命中率 (exactHits + semanticHits) / total */
    float hitRate() const {
        size_t total = exactHits + semanticHits + misses;
        return total > 0 ? static_cast<float>(exactHits + semanticHits) / static_cast<float>(total) : 0.0f;
    }

    /** @brief 估算节省的 API 调用成本（每次命中约省 2 个 token 计费单位） */
    size_t estimatedSavings() const { return (exactHits + semanticHits) * 2; }
};

/**
 * @brief 语义缓存 — 基于向量相似度的智能查询缓存
 *
 * @details 两级缓存架构：
 * - L1 精确匹配：对查询文本做哈希，O(1) 查找
 * - L2 语义匹配：将查询 embedding 插入 HNSW 索引，O(log n) ANN 检索
 *
 * 当 L2 命中的最近邻相似度超过 similarityThreshold_ 时视为语义命中，
 * 直接复用已缓存的响应。淘汰策略为 LFU（最少使用频率优先），
 * 同时定期清理过期条目。
 *
 * 单例模式，线程安全（mutex 保护所有状态）。
 */
class SemanticCache {
public:
    /** @brief 获取全局单例 */
    static SemanticCache& getInstance() {
        static SemanticCache instance;
        return instance;
    }

    /**
     * @brief 初始化缓存参数并构建内部 HNSW 索引
     * @param similarityThreshold 语义命中阈值，cosine 相似度超过此值视为命中，默认 0.92
     * @param maxSize 最大缓存条目数，默认 5000
     * @param ttlSeconds 条目过期时间（秒），默认 86400（1天）
     */
    void initialize(float similarityThreshold = 0.92f, size_t maxSize = 5000, int ttlSeconds = 86400) {
        similarityThreshold_ = similarityThreshold;
        maxSize_ = maxSize;
        ttlSeconds_ = ttlSeconds;
        initHNSWIndex();
    }

    /**
     * @brief 查询缓存（L1 精确匹配 + L2 HNSW ANN 语义匹配）
     * @param query 查询文本（用于 L1 精确匹配）
     * @param queryEmbedding 查询 embedding 向量（用于 L2 语义匹配）
     * @param[out] response 命中时填充缓存的响应内容
     * @return true 表示缓存命中
     */
    bool get(const std::string& query, const std::vector<float>& queryEmbedding, std::string& response);

    /**
     * @brief 写入缓存条目，同时更新 L1 哈希表和 L2 HNSW 索引
     * @param query 查询文本
     * @param embedding 查询 embedding 向量
     * @param response 响应内容
     */
    void put(const std::string& query, const std::vector<float>& embedding, const std::string& response);

    /** @brief 获取命中率统计快照 */
    SemanticCacheStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /** @brief 重置统计计数器 */
    void clearStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = SemanticCacheStats{};
    }

private:
    SemanticCache() = default;

    /** @brief 初始化内部 HNSW 索引实例 */
    void initHNSWIndex();
    /** @brief 计算查询文本的 SHA256 哈希（用于 L1 精确匹配） */
    std::string computeHash(const std::string& query);
    /** @brief 根据序列号生成 HNSW 节点 ID */
    std::string makeCacheNodeId(size_t seq);
    /** @brief L2 归一化向量，确保 cosine 相似度计算正确 */
    static std::vector<float> normalizeVec(const std::vector<float>& v);
    /** @brief LFU 淘汰：移除命中次数最少的条目 */
    void evictLFU();
    /** @brief 清理超过 TTL 的过期条目 */
    void purgeExpired();

    float similarityThreshold_ = 0.92f;  ///< 语义命中阈值
    size_t maxSize_ = 5000;              ///< 最大缓存容量
    int ttlSeconds_ = 86400;             ///< 条目过期时间（秒）

    std::unordered_map<std::string, CacheEntry> exactCache_;  ///< L1 精确匹配哈希表

    std::unique_ptr<HNSWIndex> hnswIndex_;  ///< L2 语义索引，HNSW ANN 加速
    /// nodeId -> CacheEntry 映射，HNSW 返回 nodeId 后在此查找完整缓存条目
    std::unordered_map<std::string, CacheEntry> semanticEntries_;
    size_t nextSeq_ = 0;  ///< 单调递增序列号，用于生成唯一 HNSW nodeId

    SemanticCacheStats stats_;       ///< 命中率统计
    mutable std::mutex mutex_;       ///< 保护所有内部状态的互斥锁
};

} // namespace ai
} // namespace heartlake
