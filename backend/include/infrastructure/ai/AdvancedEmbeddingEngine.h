/**
 * @file AdvancedEmbeddingEngine.h
 * @brief AdvancedEmbeddingEngine 模块接口定义
 * Created by 王璐瑶
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <list>
#include <shared_mutex>
#include <thread>

namespace heartlake {
namespace ai {

/**
 * @brief 高性能嵌入向量引擎（纯C++实现）
 *
 * 特性：
 * 1. 多层次特征提取（TF-IDF、N-gram、情感、统计）
 * 2. 特征哈希降维
 * 3. LRU缓存机制
 * 4. 批量处理支持
 * 5. 增量学习能力
 */
class AdvancedEmbeddingEngine {
public:
    static AdvancedEmbeddingEngine& getInstance();

    /**
     * @brief 初始化引擎
     * @param embeddingDim 嵌入向量维度（默认128）
     * @param cacheSize LRU缓存大小（默认10000）
     */
    void initialize(size_t embeddingDim = 128, size_t cacheSize = 10000);

    /**
     * @brief 生成文本嵌入向量（单个）
     * @param text 输入文本
     * @return 嵌入向量
     */
    std::vector<float> generateEmbedding(const std::string& text);

    /**
     * @brief 批量生成嵌入向量（高性能）
     * @param texts 文本列表
     * @return 嵌入向量列表
     */
    std::vector<std::vector<float>> generateEmbeddingBatch(const std::vector<std::string>& texts);

    /**
     * @brief 计算两个向量的余弦相似度
     * @param vec1 向量1
     * @param vec2 向量2
     * @return 相似度 [0, 1]
     */
    static float cosineSimilarity(const std::vector<float>& vec1, const std::vector<float>& vec2);

    /**
     * @brief 从数据库加载训练数据并更新模型
     * @param texts 训练文本集合
     */
    void trainFromCorpus(const std::vector<std::string>& texts);

    /**
     * @brief 清除缓存
     */
    void clearCache();

    /**
     * @brief 获取缓存统计信息
     */
    struct CacheStats {
        size_t hits;
        size_t misses;
        size_t size;
        float hitRate;
    };
    CacheStats getCacheStats() const;

    bool isInitialized() const { return initialized_; }
    size_t getEmbeddingDimension() const { return embeddingDim_; }

private:
    AdvancedEmbeddingEngine() = default;
    ~AdvancedEmbeddingEngine() = default;
    AdvancedEmbeddingEngine(const AdvancedEmbeddingEngine&) = delete;
    AdvancedEmbeddingEngine& operator=(const AdvancedEmbeddingEngine&) = delete;

    // 配置参数
    size_t embeddingDim_ = 128;
    size_t cacheSize_ = 10000;
    bool initialized_ = false;

    // IDF权重（词 -> IDF值）
    std::unordered_map<std::string, float> idfWeights_;
    size_t totalDocs_ = 0;
    mutable std::shared_mutex idfMutex_;

    // 情感词典
    std::unordered_map<std::string, float> sentimentLexicon_;

    // LRU缓存
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

        void put(const std::string& key, const std::vector<float>& value) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                lruList.erase(it->second.second);
            } else if (cache.size() >= maxSize) {
                // 删除最久未使用的
                auto last = lruList.back();
                cache.erase(last);
                lruList.pop_back();
            }
            lruList.push_front(key);
            cache[key] = {value, lruList.begin()};
        }

        void clear() {
            cache.clear();
            lruList.clear();
            hits = 0;
            misses = 0;
        }
    };
    std::unique_ptr<LRUCache> embeddingCache_;
    mutable std::mutex cacheMutex_;

    // 内部方法
    std::vector<std::string> tokenize(const std::string& text) const;
    std::vector<std::string> extractNGrams(const std::vector<std::string>& tokens, int n) const;
    std::vector<float> extractTFIDFFeatures(const std::vector<std::string>& tokens) const;
    std::vector<float> extractSentimentFeatures(const std::vector<std::string>& tokens) const;
    std::vector<float> extractStatisticalFeatures(const std::vector<std::string>& tokens, const std::string& text) const;
    std::vector<float> extractNGramFeatures(const std::vector<std::string>& tokens) const;
    std::vector<float> combineFeatures(
        const std::vector<float>& tfidf,
        const std::vector<float>& sentiment,
        const std::vector<float>& statistical,
        const std::vector<float>& ngram
    ) const;
    static void normalizeVector(std::vector<float>& vec);
    size_t featureHash(const std::string& feature, size_t numBuckets) const;
    void computeIDF(const std::vector<std::string>& texts);
    void loadSentimentLexicon();
    void warmupTrainingDataAsync();

    // 后台预热训练线程，jthread 析构时自动 request_stop + join
    std::jthread warmupThread_;
};

} // namespace ai
} // namespace heartlake
