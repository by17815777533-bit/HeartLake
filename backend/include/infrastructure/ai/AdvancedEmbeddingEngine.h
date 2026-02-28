/**
 * 高性能文本嵌入向量引擎（纯 C++ 实现，无外部模型依赖）
 *
 * 多层次特征提取流水线：
 *   1. TF-IDF 特征（60% 维度）— 签名特征哈希降维，IDF 权重可增量学习
 *   2. 情感特征（15 维）— 中英文情感词典匹配，提取极性/强度/波动/复杂度
 *   3. 统计特征（15 维）— 词汇多样性(TTR)、词频熵、词长分布
 *   4. N-gram 特征（10% 维度）— 2-gram/3-gram 签名哈希，捕捉局部语序
 *
 * 性能优化：
 *   - LRU 缓存避免重复计算（默认 10000 条）
 *   - 4 路循环展开 + double 累加减少浮点误差（normalizeVector / cosineSimilarity）
 *   - N-gram 内联滑窗哈希，零中间 string 分配
 *   - jthread 后台异步预热训练语料
 */

#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <list>
#include <shared_mutex>
#include <thread>
#include <utility>

namespace heartlake {
namespace ai {

/**
 * @brief 高性能嵌入向量引擎（纯 C++ 实现，无外部模型依赖）
 *
 * @details 单例模式。多层次特征提取流水线：
 * 1. TF-IDF 特征（60% 维度）— 签名特征哈希降维，IDF 权重可增量学习
 * 2. 情感特征（15 维）— 中英文情感词典匹配，提取极性/强度/波动/复杂度
 * 3. 统计特征（15 维）— 词汇多样性(TTR)、词频熵、词长分布
 * 4. N-gram 特征（10% 维度）— 2-gram/3-gram 签名哈希，捕捉局部语序
 *
 * 性能优化：
 * - LRU 缓存避免重复计算（默认 10000 条）
 * - 4 路循环展开 + double 累加减少浮点误差
 * - N-gram 内联滑窗哈希，零中间 string 分配
 * - jthread 后台异步预热训练语料
 */
class AdvancedEmbeddingEngine {
public:
    /** @brief 获取全局单例 */
    static AdvancedEmbeddingEngine& getInstance();

    /**
     * 初始化引擎
     * @param embeddingDim 嵌入向量维度（默认128）
     * @param cacheSize LRU缓存大小（默认10000）
     */
    void initialize(size_t embeddingDim = 128, size_t cacheSize = 10000);

    /**
     * 生成文本嵌入向量（单个）
     * @param text 输入文本
     * @return 嵌入向量
     */
    std::vector<float> generateEmbedding(const std::string& text);

    /**
     * 批量生成嵌入向量（高性能）
     * @param texts 文本列表
     * @return 嵌入向量列表
     */
    std::vector<std::vector<float>> generateEmbeddingBatch(const std::vector<std::string>& texts);

    /**
     * 计算两个向量的余弦相似度
     * @param vec1 向量1
     * @param vec2 向量2
     * @return 相似度 [0, 1]
     */
    static float cosineSimilarity(const std::vector<float>& vec1, const std::vector<float>& vec2);

    /**
     * 从数据库加载训练数据并更新模型
     * @param texts 训练文本集合
     */
    void trainFromCorpus(const std::vector<std::string>& texts);

    /**
     * 清除缓存
     */
    void clearCache();

    /**
     * @brief 获取缓存统计信息
     */
    struct CacheStats {
        size_t hits;      ///< 缓存命中次数
        size_t misses;    ///< 缓存未命中次数
        size_t size;      ///< 当前缓存条目数
        float hitRate;    ///< 命中率 [0, 1]
    };
    CacheStats getCacheStats() const;

    /** @brief 是否已完成初始化 */
    bool isInitialized() const { return initialized_; }
    /** @brief 获取当前 embedding 维度 */
    size_t getEmbeddingDimension() const { return embeddingDim_; }

private:
    AdvancedEmbeddingEngine() = default;
    ~AdvancedEmbeddingEngine() = default;
    AdvancedEmbeddingEngine(const AdvancedEmbeddingEngine&) = delete;
    AdvancedEmbeddingEngine& operator=(const AdvancedEmbeddingEngine&) = delete;

    // 配置参数
    size_t embeddingDim_ = 128;                ///< embedding 向量维度
    size_t cacheSize_ = 10000;                 ///< LRU 缓存最大容量
    std::atomic<bool> initialized_{false};     ///< 初始化标记

    // IDF 权重（词 -> IDF 值），支持增量更新
    std::unordered_map<std::string, float> idfWeights_;
    size_t totalDocs_ = 0;                     ///< 已处理文档总数（用于 IDF 计算）
    mutable std::shared_mutex idfMutex_;       ///< 保护 IDF 权重表的读写锁

    std::unordered_map<std::string, float> sentimentLexicon_;  ///< 中英文情感词典 (词 -> 极性分数)

    /**
     * @brief LRU 缓存，避免对相同文本重复计算 embedding
     * @details 使用 list + unordered_map 实现 O(1) 的 get/put 操作
     */
    struct LRUCache {
        std::unordered_map<std::string, std::pair<std::vector<float>, std::list<std::string>::iterator>> cache;
        std::list<std::string> lruList;
        size_t maxSize;
        size_t hits = 0;
        size_t misses = 0;

        LRUCache(size_t size) : maxSize(size) {}

        bool get(const std::string& key, std::vector<float>& value) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                // 移到最前面
                lruList.erase(it->second.second);
                lruList.push_front(key);
                it->second.second = lruList.begin();
                value = it->second.first;
                hits++;
                return true;
            }
            misses++;
            return false;
        }

        void put(const std::string& key, std::vector<float>&& value) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                it->second.first = std::move(value);
                lruList.erase(it->second.second);
                lruList.push_front(key);
                it->second.second = lruList.begin();
            } else {
                if (cache.size() >= maxSize) {
                    // 删除最久未使用的
                    auto& last = lruList.back();
                    cache.erase(last);
                    lruList.pop_back();
                }
                lruList.push_front(key);
                cache.emplace(key, std::pair{std::move(value), lruList.begin()});
            }
        }

        void clear() {
            cache.clear();
            lruList.clear();
            hits = 0;
            misses = 0;
        }
    };
    std::unique_ptr<LRUCache> embeddingCache_;  ///< embedding 结果缓存
    mutable std::mutex cacheMutex_;              ///< 保护缓存的互斥锁

    // ---- 内部特征提取方法 ----

    /** @brief UTF-8 分词（中文按字、英文按空格/标点） */
    std::vector<std::string> tokenize(const std::string& text) const;
    /** @brief 提取 N-gram 序列 */
    std::vector<std::string> extractNGrams(const std::vector<std::string>& tokens, int n) const;
    /** @brief TF-IDF 特征提取（签名哈希降维到 60% 维度） */
    std::vector<float> extractTFIDFFeatures(const std::vector<std::string>& tokens) const;
    /** @brief 情感特征提取（极性/强度/波动/复杂度，15 维） */
    std::vector<float> extractSentimentFeatures(const std::vector<std::string>& tokens) const;
    /** @brief 统计特征提取（TTR/词频熵/词长分布，15 维） */
    std::vector<float> extractStatisticalFeatures(const std::vector<std::string>& tokens, const std::string& text) const;
    /** @brief N-gram 特征提取（2-gram/3-gram 签名哈希，10% 维度） */
    std::vector<float> extractNGramFeatures(const std::vector<std::string>& tokens) const;

    /**
     * @brief 拼接四类特征向量为最终 embedding
     * @details 按 TF-IDF(60%) + sentiment(15d) + statistical(15d) + ngram(10%) 拼接
     */
    std::vector<float> combineFeatures(
        const std::vector<float>& tfidf,
        const std::vector<float>& sentiment,
        const std::vector<float>& statistical,
        const std::vector<float>& ngram
    ) const;

    /** @brief L2 归一化（4 路展开 + double 累加减少浮点误差） */
    static void normalizeVector(std::vector<float>& vec);

    /**
     * @brief 签名特征哈希：将特征字符串映射到桶索引和符号
     * @param feature 特征字符串
     * @param numBuckets 桶数量
     * @return {桶索引, 符号(+1/-1)}
     */
    std::pair<size_t, int> featureHash(const std::string& feature, size_t numBuckets) const;

    /** @brief 从文本集合计算 IDF 权重 */
    void computeIDF(const std::vector<std::string>& texts);
    /** @brief 加载内置中英文情感词典 */
    void loadSentimentLexicon();
    /** @brief 后台异步预热：从数据库加载训练语料更新 IDF */
    void warmupTrainingDataAsync();

    /// 后台预热训练线程，jthread 析构时自动 request_stop + join
    std::jthread warmupThread_;
};

} // namespace ai
} // namespace heartlake
