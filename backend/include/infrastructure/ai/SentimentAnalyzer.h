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
 * 轻量级情感分析结果
 */
struct EdgeSentimentResult {
    float score;              ///< 情感分数 [-1.0, 1.0]
    std::string mood;         ///< 情绪类型
    float confidence;         ///< 置信度 [0.0, 1.0]
    std::string method;       ///< 分析方法

    Json::Value toJson() const {
        Json::Value j;
        j["score"] = score;
        j["mood"] = mood;
        j["confidence"] = confidence;
        j["method"] = method;
        return j;
    }
};

class SentimentAnalyzer {
public:
    /**
     * 配置缓存参数
     */
    void configure(int cacheTTLSec, size_t cacheMaxSize);

    /**
     * 加载中英文情感词典
     */
    void loadLexicon();

    /**
     * 分析文本情感（带缓存 + 飞行中去重）
     */
    EdgeSentimentResult analyzeSentiment(const std::string& text);

    /**
     * 清空缓存和飞行中状态
     */
    void clearCache();

    size_t getTotalCalls() const { return totalSentimentCalls_.load(); }
    size_t getCacheHits() const { return sentimentCacheHits_.load(); }
    size_t getCacheMisses() const { return sentimentCacheMisses_.load(); }

#ifdef HEARTLAKE_USE_ONNX
    void setOnnxEngine(std::unique_ptr<OnnxSentimentEngine> engine);
    bool isOnnxEnabled() const { return onnxEnabled_; }
#endif

private:
    // ---- 分析方法 ----
    EdgeSentimentResult analyzeSentimentUncached(const std::string& text);
    std::vector<std::string> tokenizeUTF8(const std::string& text) const;
    float ruleSentiment(const std::string& text) const;
    float lexiconSentiment(const std::vector<std::string>& tokens) const;
    float statisticalSentiment(const std::vector<std::string>& tokens,
                               const std::string& text) const;
    std::string scoresToMood(float score) const;
    std::string normalizeSentimentText(const std::string& text) const;

    // ---- 缓存 ----
    struct SentimentCacheEntry {
        std::string key;
        EdgeSentimentResult result;
        std::chrono::steady_clock::time_point expiresAt;
    };
    bool getSentimentCacheHit(const std::string& key, EdgeSentimentResult& result);
    void compactSentimentCacheLocked(std::unique_lock<std::shared_mutex>& lock);
    void putSentimentCache(const std::string& key, const EdgeSentimentResult& result);

    std::list<SentimentCacheEntry> sentimentCacheLRU_;
    std::unordered_map<std::string, std::list<SentimentCacheEntry>::iterator> sentimentCacheMap_;
    mutable std::shared_mutex sentimentCacheMutex_;
    size_t sentimentCacheMaxSize_ = 4096;
    int sentimentCacheTTLSeconds_ = 300;
    std::atomic<size_t> sentimentCacheHits_{0};
    std::atomic<size_t> sentimentCacheMisses_{0};

    // ---- 飞行中请求去重 ----
    std::unordered_map<std::string, std::shared_future<EdgeSentimentResult>> sentimentInFlight_;

    // ---- 词典 ----
    std::unordered_map<std::string, float> sentimentLexicon_;
    std::unordered_map<std::string, float> domainSentimentLexicon_;
    std::unordered_map<std::string, float> intensifiers_;
    std::unordered_map<std::string, float> negators_;

    // ---- 统计 ----
    std::atomic<size_t> totalSentimentCalls_{0};

#ifdef HEARTLAKE_USE_ONNX
    std::unique_ptr<OnnxSentimentEngine> onnxEngine_;
    bool onnxEnabled_ = false;
#endif
};

} // namespace ai
} // namespace heartlake
