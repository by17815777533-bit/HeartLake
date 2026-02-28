/**
 * 轻量级情感分析引擎
 *
 * 从 EdgeAIEngine 拆分的独立子系统。
 * 三层融合：规则(emoji/标点) + 词典(中英文) + 统计(TTR/词长)
 * 带 LRU 缓存 + 飞行中请求去重。
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cmath>
#include <future>
#include <json/json.h>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef HEARTLAKE_USE_ONNX
#include "infrastructure/ai/OnnxSentimentEngine.h"
#endif

namespace heartlake {
namespace ai {

/**
 * @brief 轻量级情感分析结果
 */
struct EdgeSentimentResult {
    float score;              ///< 情感分数 [-1.0, 1.0]，负值为消极，正值为积极
    std::string mood;         ///< 情绪类型标签 (happy, sad, angry, neutral 等)
    float confidence;         ///< 置信度 [0.0, 1.0]，三层融合加权后的综合置信度
    std::string method;       ///< 分析方法标识 ("rule", "lexicon", "onnx" 等)

    /** @brief 序列化为 JSON */
    Json::Value toJson() const {
        Json::Value j;
        j["score"] = score;
        j["mood"] = mood;
        j["confidence"] = confidence;
        j["method"] = method;
        return j;
    }
};

/**
 * @brief 轻量级情感分析引擎 — EdgeAIEngine 子系统
 *
 * @details 三层融合分析流水线：
 * 1. 规则层：emoji 表情和标点符号模式匹配
 * 2. 词典层：中英文情感词典 + 程度副词 + 否定词处理
 * 3. 统计层：词汇多样性(TTR)、平均词长等文本统计特征
 *
 * 性能优化：
 * - LRU 缓存（默认 4096 条，TTL 300秒），避免重复分析
 * - 飞行中请求去重（in-flight dedup），相同文本的并发请求共享 future
 * - 可选 ONNX Runtime 后端（Erlangshen-Roberta 模型）提升精度
 *
 * 线程安全：shared_mutex 保护缓存，mutex 保护飞行中请求表。
 */
class SentimentAnalyzer {
public:
    /**
     * @brief 配置缓存参数
     * @param cacheTTLSec 缓存条目过期时间（秒）
     * @param cacheMaxSize 缓存最大容量
     */
    void configure(int cacheTTLSec, size_t cacheMaxSize);

    /**
     * @brief 加载中英文情感词典、程度副词表和否定词表
     */
    void loadLexicon();

    /**
     * @brief 分析文本情感（带 LRU 缓存 + 飞行中去重）
     * @param text 待分析文本
     * @param preferOnnx 是否优先使用 ONNX 模型（需编译时启用 HEARTLAKE_USE_ONNX）
     * @return 情感分析结果
     */
    EdgeSentimentResult analyzeSentiment(const std::string& text, bool preferOnnx = false);

    /** @brief 清空 LRU 缓存和飞行中请求状态 */
    void clearCache();

    /** @brief 累计分析调用次数 */
    size_t getTotalCalls() const { return totalSentimentCalls_.load(); }
    /** @brief 缓存命中次数 */
    size_t getCacheHits() const { return sentimentCacheHits_.load(); }
    /** @brief 缓存未命中次数 */
    size_t getCacheMisses() const { return sentimentCacheMisses_.load(); }

#ifdef HEARTLAKE_USE_ONNX
    void setOnnxEngine(std::unique_ptr<OnnxSentimentEngine> engine);
    bool isOnnxEnabled() const { return onnxEnabled_; }
#endif

private:
    // ---- 分析方法 ----
    /** @brief 无缓存的情感分析核心逻辑（三层融合） */
    EdgeSentimentResult analyzeSentimentUncached(const std::string& text, bool preferOnnx = false);
    /** @brief UTF-8 分词（中文按字符、英文按空格/标点） */
    std::vector<std::string> tokenizeUTF8(const std::string& text) const;
    /** @brief 规则层：emoji 和标点模式匹配 */
    float ruleSentiment(const std::string& text) const;
    /** @brief 词典层：情感词典 + 程度副词 + 否定词 */
    float lexiconSentiment(const std::vector<std::string>& tokens) const;
    /** @brief 统计层：TTR、平均词长等文本统计特征 */
    float statisticalSentiment(const std::vector<std::string>& tokens,
                               const std::string& text) const;
    /** @brief 将情感分数映射为情绪类型标签 */
    std::string scoresToMood(float score) const;
    /** @brief 文本预处理：去空白、统一大小写等 */
    std::string normalizeSentimentText(const std::string& text) const;

    // ---- LRU 缓存 ----
    /** @brief 缓存条目，包含结果和过期时间 */
    struct SentimentCacheEntry {
        std::string key;
        EdgeSentimentResult result;
        std::chrono::steady_clock::time_point expiresAt;
    };
    /** @brief 查询缓存，命中时更新 LRU 顺序 */
    bool getSentimentCacheHit(const std::string& key, EdgeSentimentResult& result);
    /** @brief 清理过期条目（需持有 unique_lock） */
    void compactSentimentCacheLocked(std::unique_lock<std::shared_mutex>& lock);
    /** @brief 写入缓存，超容量时淘汰最久未使用的条目 */
    void putSentimentCache(const std::string& key, const EdgeSentimentResult& result);

    std::list<SentimentCacheEntry> sentimentCacheLRU_;  ///< LRU 双向链表
    std::unordered_map<std::string, std::list<SentimentCacheEntry>::iterator> sentimentCacheMap_;  ///< key -> 链表迭代器
    mutable std::shared_mutex sentimentCacheMutex_;     ///< 保护缓存的读写锁
    size_t sentimentCacheMaxSize_ = 4096;               ///< 缓存最大容量
    int sentimentCacheTTLSeconds_ = 300;                ///< 缓存条目 TTL（秒）
    std::atomic<size_t> sentimentCacheHits_{0};         ///< 命中计数
    std::atomic<size_t> sentimentCacheMisses_{0};       ///< 未命中计数

    // ---- 飞行中请求去重 ----
    /// 相同文本的并发请求共享同一个 shared_future，避免重复计算
    std::unordered_map<std::string, std::shared_future<EdgeSentimentResult>> sentimentInFlight_;

    // ---- 词典 ----
    std::unordered_map<std::string, float> sentimentLexicon_;       ///< 基础情感词典 (词 -> 极性分数)
    std::unordered_map<std::string, float> domainSentimentLexicon_; ///< 领域专用情感词典
    std::unordered_map<std::string, float> intensifiers_;           ///< 程度副词 (词 -> 强度倍数)
    std::unordered_map<std::string, float> negators_;               ///< 否定词 (词 -> 翻转系数)

    // ---- 统计 ----
    std::atomic<size_t> totalSentimentCalls_{0};  ///< 累计调用次数

#ifdef HEARTLAKE_USE_ONNX
    std::unique_ptr<OnnxSentimentEngine> onnxEngine_;
    bool onnxEnabled_ = false;
#endif
};

} // namespace ai
} // namespace heartlake
