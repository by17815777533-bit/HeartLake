/**
 * @file HNSWIndex.h
 * @brief HNSW (Hierarchical Navigable Small World) 向量检索引擎
 *
 * 从 EdgeAIEngine 拆分的独立子系统，提供高性能近似最近邻搜索。
 * 特性：
 *   - 多层图结构 + 贪心 beam search
 *   - Ada-EF 自适应搜索宽度
 *   - Matryoshka 融合 cosine 重排序
 *   - 线程安全（shared_mutex 读写锁）
 */

#pragma once

#include <json/json.h>

#include <atomic>
#include <cmath>
#include <random>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace heartlake::ai {

/**
 * @brief HNSW 图节点
 */
struct HNSWNode {
    std::string id;                                    ///< 节点唯一标识
    std::vector<float> vector;                         ///< 向量数据
    std::vector<std::vector<size_t>> neighbors;        ///< 各层邻居列表
    int maxLevel;                                      ///< 节点最高层级
};

/**
 * @brief 向量检索结果
 */
struct VectorSearchResult {
    std::string id;       ///< 节点标识
    float distance;       ///< 距离（越小越相似）
    float similarity;     ///< 相似度 [0, 1]
};

/**
 * @brief HNSW 向量检索引擎
 *
 * 独立的 HNSW 索引，支持插入、删除、KNN 搜索和 Matryoshka 重排序。
 * 线程安全：所有公开方法均可并发调用。
 */
class HNSWIndex {
public:
    HNSWIndex();
    ~HNSWIndex() = default;

    // 禁止拷贝和移动（shared_mutex 不可移动）
    HNSWIndex(const HNSWIndex&) = delete;
    HNSWIndex& operator=(const HNSWIndex&) = delete;
    HNSWIndex(HNSWIndex&&) = delete;
    HNSWIndex& operator=(HNSWIndex&&) = delete;

    /**
     * @brief 插入向量
     * @param id 唯一标识
     * @param vec 向量数据（维度必须与已有向量一致）
     */
    void addVector(const std::string& id, const std::vector<float>& vec);

    /**
     * @brief KNN 搜索（带 Ada-EF 自适应搜索宽度）
     * @param query 查询向量
     * @param k 返回最近邻数量
     * @return 按相似度降序排列的结果
     */
    std::vector<VectorSearchResult> searchKNN(const std::vector<float>& query, int k);

    /**
     * @brief 删除向量（逻辑删除 + 邻居断链）
     * @param id 要删除的向量标识
     * @return 是否成功删除
     */
    bool removeVector(const std::string& id);

    /**
     * @brief Matryoshka 融合 cosine 重排序
     * @param query 查询向量
     * @param candidates 候选结果集
     * @param topK 返回数量
     * @return 重排序后的结果
     */
    std::vector<VectorSearchResult> rerankCandidates(
        const std::vector<float>& query,
        const std::vector<VectorSearchResult>& candidates,
        int topK) const;

    /** @brief 获取索引中向量数量 */
    size_t getVectorCount() const;

    /** @brief 获取索引统计信息（JSON） */
    Json::Value getHNSWStats() const;

    /** @brief 获取当前向量维度（无向量时返回 0） */
    size_t getHNSWVectorDimension() const;

    /**
     * @brief 配置 HNSW 参数
     * @param m 每层最大邻居数
     * @param mMax0 第0层最大邻居数
     * @param efConstruction 构建搜索宽度
     * @param efSearch 查询搜索宽度
     */
    void configure(int m, int mMax0, int efConstruction, int efSearch);

    /** @brief 清空索引 */
    void clear();

private:
    // ---- 索引数据 ----
    std::vector<HNSWNode> nodes_;                           ///< 所有节点
    std::unordered_map<std::string, size_t> idMap_;         ///< ID → 索引映射

    // ---- 参数 ----
    static constexpr int DEFAULT_M = 16;
    static constexpr int DEFAULT_M_MAX0 = 32;
    static constexpr int DEFAULT_EF_CONSTRUCTION = 200;
    static constexpr int DEFAULT_EF_SEARCH = 50;

    int m_ = DEFAULT_M;
    int mMax0_ = DEFAULT_M_MAX0;
    int efConstruction_ = DEFAULT_EF_CONSTRUCTION;
    int efSearch_ = DEFAULT_EF_SEARCH;

    // ---- 图状态 ----
    int maxLevel_ = 0;
    size_t entryPoint_ = 0;
    float levelMult_ = 1.0f / std::log(static_cast<float>(DEFAULT_M));

    // ---- 并发 ----
    mutable std::shared_mutex mutex_;
    std::mt19937 rng_{std::random_device{}()};
    std::atomic<size_t> totalSearches_{0};

    // ---- 内部算法 ----
    int randomLevel();
    float vectorDistance(const std::vector<float>& a, const std::vector<float>& b) const;
    std::vector<std::pair<float, size_t>> searchLayer(
        const std::vector<float>& query, size_t entryPoint, int ef, int level) const;
    void connectNeighbors(size_t nodeIdx, const std::vector<size_t>& neighbors,
                          int level, int maxM);
};

}  // namespace heartlake::ai
