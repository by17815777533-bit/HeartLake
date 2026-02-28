/**
 * AI 服务统一入口 - 大模型 API 调用层
 *
 * 封装情感分析、内容审核、智能回复、嵌入向量生成等 AI 能力，
 * 支持 DeepSeek / OpenAI / Ollama 等多种后端。
 *
 * 核心设计：
 * - 三级降级策略：大模型 API → EdgeAIEngine 本地推理 → EmotionManager 规则兜底
 * - 熔断器保护：连续失败超阈值后自动熔断，冷却后半开探测恢复
 * - 双层缓存：L1 精确哈希命中 + L2 语义缓存（HNSW ANN 加速）
 * - 飞行中请求合并（in-flight coalescing）：相同文本的并发请求只发一次 API
 * - 自适应本地置信度阈值：高并发时动态降低阈值，减少尾延迟
 */

#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpClient.h>
#include <string>
#include <vector>
#include <functional>
#include <json/json.h>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <atomic>
#include "SemanticCache.h"

namespace heartlake {
namespace ai {

/**
 * AI 服务单例
 *
 * 线程安全。内部通过 shared_mutex / mutex / atomic 分别保护
 * 情感缓存、审核缓存、飞行中请求表和统计计数器。
 *
 * 调用链路示例（情感分析）：
 * analyzeSentiment → L1缓存命中? → 飞行中合并? → 本地高置信度? → 大模型API → 降级兜底
 */
class AIService {
public:
    static AIService& getInstance();

    /**
     * 初始化 AI 服务
     *
     * 从配置中读取 provider / apiKey / baseUrl / model 等参数，
     * 创建复用的 HTTP 客户端，初始化语义缓存和熔断器。
     *
     * @param config JSON 配置（provider, api_key, base_url, model, timeout 等）
     */
    void initialize(const Json::Value& config);
    
    
    /**
     * 分析文本情感极性
     * @param text 待分析文本
     * @param callback 回调函数，参数为(情感分数, 情绪类型, 错误信息)
     *                 情感分数: -1.0(极负面) 到 1.0(极正面)
     */
    void analyzeSentiment(
        const std::string& text,
        std::function<void(float score, const std::string& mood, const std::string& error)> callback
    );
    
    /**
     * 批量分析情感
     */
    void analyzeSentimentBatch(
        const std::vector<std::string>& texts,
        std::function<void(const std::vector<std::pair<float, std::string>>& results, const std::string& error)> callback
    );
    
    
    /**
     * 审核文本内容
     * @param text 待审核文本
     * @param callback 回调函数，参数为(是否通过, 风险类别, 置信度, 原因)
     */
    void moderateText(
        const std::string& text,
        std::function<void(bool passed, const std::vector<std::string>& categories, 
                          float confidence, const std::string& reason)> callback
    );
    
    /**
     * 审核图片内容
     * @param imageUrl 图片URL
     * @param callback 回调函数
     */
    void moderateImage(
        const std::string& imageUrl,
        std::function<void(bool passed, const std::vector<std::string>& categories,
                          float confidence, const std::string& reason)> callback
    );
    
    
    /**
     * 生成AI回复（心理咨询师角色）
     * @param userMessage 用户消息
     * @param context 上下文信息（可选）
     * @param callback 回调函数，参数为(AI回复内容, 错误信息)
     */
    void generateReply(
        const std::string& userMessage,
        const std::string& context,
        std::function<void(const std::string& reply, const std::string& error)> callback
    );
    
    /**
     * 生成纸船AI回复
     * @param boatContent 纸船内容
     * @param senderMood 发送者情绪
     * @param callback 回调函数
     */
    void generateBoatReply(
        const std::string& boatContent,
        const std::string& senderMood,
        std::function<void(const std::string& reply, const std::string& error)> callback
    );

    /**
     * 为石头生成AI暖心评论
     * @param stoneContent 石头内容
     * @param stoneMood 石头情绪类型
     * @param callback 回调函数，参数为(AI评论内容, 错误信息)
     */
    void generateStoneComment(
        const std::string& stoneContent,
        const std::string& stoneMood,
        std::function<void(const std::string& comment, const std::string& error)> callback
    );

    
    /**
     * 生成文本嵌入向量
     * @param text 文本内容
     * @param callback 回调函数，参数为(向量, 错误信息)
     */
    void generateEmbedding(
        const std::string& text,
        std::function<void(const std::vector<float>& embedding, const std::string& error)> callback
    );
    
    
    /**
     * 本地敏感词快速检测（不调用 AI API，委托 ContentFilter AC 自动机）
     * @param text 待检测文本
     * @return 检测结果，包含是否通过、匹配词列表、风险类别、是否需要紧急关注
     */
    struct LocalModerationResult {
        bool passed;
        std::vector<std::string> matchedWords;
        std::string category;  // "self_harm", "violence", "sexual", "normal"
        bool needsAlert;       // 是否需要特别关注（如自伤倾向）
    };
    LocalModerationResult localModerate(const std::string& text);
    
    /**
     * 加载敏感词库
     */
    void loadSensitiveWords();
    
    /**
     * 加载默认敏感词
     */
    void loadDefaultSensitiveWords();

    /**
     * 清除审核缓存
     */
    void clearModerationCache();

    /**
     * 获取AI服务统计信息
     */
    struct AIStats {
        size_t totalAPICalls;
        size_t totalLocalCalls;
        size_t moderationCacheHits;
        size_t moderationCacheMisses;
        float moderationCacheHitRate;
        size_t totalCost;  // 单位：分（人民币）
    };
    AIStats getStats() const;

    /**
     * 获取语义缓存统计
     */
    SemanticCacheStats getSemanticCacheStats() const;

private:
    AIService() = default;
    ~AIService() = default;
    AIService(const AIService&) = delete;
    AIService& operator=(const AIService&) = delete;
    
    std::string provider_;      // "deepseek", "openai", "qwen"
    std::string apiKey_;
    std::string baseUrl_;
    std::string model_;
    int timeout_;
    
    struct SensitiveWord {
        std::string word;
        std::string level;      // "critical", "high", "medium", "low"
        std::string category;
        std::string action;     // "block", "alert", "filter"
    };
    std::vector<SensitiveWord> sensitiveWords_;

    // 审核结果缓存
    struct ModerationCacheEntry {
        bool passed;
        std::vector<std::string> categories;
        float confidence;
        std::string reason;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::unordered_map<std::string, ModerationCacheEntry> moderationCache_;
    mutable std::mutex moderationCacheMutex_;
    size_t moderationCacheMaxSize_ = 10000;
    int moderationCacheTTLSeconds_ = 3600;  // 1小时

    // 情绪分析缓存（L1 精确命中）+ 并发合并（in-flight coalescing）
    struct SentimentCacheEntry {
        float score{0.0f};
        std::string mood{"neutral"};
        std::chrono::steady_clock::time_point timestamp;
    };
    std::unordered_map<std::string, SentimentCacheEntry> sentimentCache_;
    mutable std::shared_mutex sentimentCacheMutex_;
    size_t sentimentCacheMaxSize_ = 30000;
    int sentimentCacheTTLSeconds_ = 7200;  // 2小时
    std::unordered_map<
        std::string,
        std::vector<std::function<void(float, const std::string&, const std::string&)>>
    > sentimentInFlight_;
    mutable std::mutex sentimentInFlightMutex_;
    int sentimentAdaptiveInflightThreshold_ = 8;
    float sentimentAdaptiveLocalConfDelta_ = 0.20f;

    // 统计信息 - 使用原子变量避免数据竞争
    std::atomic<size_t> totalAPICalls_{0};
    std::atomic<size_t> totalLocalCalls_{0};
    std::atomic<size_t> moderationCacheHits_{0};
    std::atomic<size_t> moderationCacheMisses_{0};
    std::atomic<size_t> sentimentCacheHits_{0};
    std::atomic<size_t> sentimentCacheMisses_{0};
    std::atomic<size_t> sentimentCoalesced_{0};

    void callAIAPI(
        const std::string& endpoint,
        const Json::Value& payload,
        std::function<void(const Json::Value& response, const std::string& error)> callback
    );

    void callAIAPIWithRetry(
        const std::string& endpoint,
        const Json::Value& payload,
        std::function<void(const Json::Value& response, const std::string& error)> callback,
        int retryCount
    );

    std::string getReqResultError(drogon::ReqResult result) const;
    bool isRetryableError(drogon::ReqResult result) const;
    int getRetryDelay(int retryCount) const;

    bool isCircuitOpen();
    void onRequestSuccess();
    void onRequestFailure();

    int maxRetries_ = 3;
    int circuitFailureThreshold_ = 3;
    int circuitCooldownSeconds_ = 20;
    float localSentimentConfidenceThreshold_ = 0.72f;
    drogon::HttpClientPtr ollamaClient_;  // 复用连接
    std::mutex circuitMutex_;
    bool circuitOpen_ = false;
    int consecutiveFailures_ = 0;
    std::chrono::steady_clock::time_point circuitOpenedAt_{};
    
    std::string buildSentimentPrompt(const std::string& text);
    std::string buildModerationPrompt(const std::string& text);
    std::string buildReplyPrompt(const std::string& message, const std::string& context);
    std::string buildBoatReplyPrompt(const std::string& content, const std::string& mood);
    std::string buildStoneCommentPrompt(const std::string& content, const std::string& mood);

    std::string parseMoodFromScore(float score);

    // 辅助方法
    std::string computeTextHash(const std::string& text) const;
    std::string normalizeSentimentText(const std::string& text) const;
    bool getModerationFromCache(const std::string& textHash, ModerationCacheEntry& entry);
    void putModerationToCache(const std::string& textHash, const ModerationCacheEntry& entry);
    void cleanExpiredCache();
    bool getSentimentFromCache(const std::string& textHash, float& score, std::string& mood);
    void putSentimentToCache(const std::string& textHash, float score, const std::string& mood);
    void cleanExpiredSentimentCacheLocked();
    bool registerSentimentInFlight(
        const std::string& textHash,
        std::function<void(float, const std::string&, const std::string&)> callback
    );
    void completeSentimentInFlight(
        const std::string& textHash,
        float score,
        const std::string& mood,
        const std::string& error
    );
    size_t currentSentimentInFlightCount();
};

} // namespace ai
} // namespace heartlake
