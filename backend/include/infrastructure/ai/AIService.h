/**
 * @file AIService.h
 * @brief AIService 模块接口定义
 * Created by 王璐瑶
 */

#pragma once

#include <drogon/HttpController.h>
#include <string>
#include <vector>
#include <functional>
#include <json/json.h>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <atomic>
#include "SemanticCache.h"

using namespace drogon;

namespace heartlake {
namespace ai {

/**
 * AI服务 - 情感分析、内容审核、智能回复
 * 支持 DeepSeek、OpenAI 等大模型API
 */
/**
 * @brief AI服务，提供AI相关功能
 *
 * 详细说明
 *
 * @note 注意事项
 */
class AIService {
public:
    static AIService& getInstance();
    
    /**
     * @brief initialize方法
     *
     * @param config 参数说明
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
     * 本地敏感词快速检测（不调用AI API）
     * @param text 待检测文本
     * @return 检测结果: {passed, matched_words, category}
     */
    struct LocalModerationResult {
        bool passed;
        std::vector<std::string> matchedWords;
        std::string category;  // "self_harm", "violence", "sexual", "normal"
        bool needsAlert;       // 是否需要特别关注（如自伤倾向）
    };
    /**
     * @brief localModerate方法
     *
     * @param text 参数说明
     * @return 返回值说明
     */
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

    // 统计信息 - 使用原子变量避免数据竞争
    std::atomic<size_t> totalAPICalls_{0};
    std::atomic<size_t> totalLocalCalls_{0};
    std::atomic<size_t> moderationCacheHits_{0};
    std::atomic<size_t> moderationCacheMisses_{0};

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

    std::string getReqResultError(ReqResult result) const;
    bool isRetryableError(ReqResult result) const;
    int getRetryDelay(int retryCount) const;

    int maxRetries_ = 3;
    
    std::string buildSentimentPrompt(const std::string& text);
    std::string buildModerationPrompt(const std::string& text);
    std::string buildReplyPrompt(const std::string& message, const std::string& context);
    std::string buildBoatReplyPrompt(const std::string& content, const std::string& mood);
    std::string buildStoneCommentPrompt(const std::string& content, const std::string& mood);

    std::string parseMoodFromScore(float score);

    // 辅助方法
    std::string computeTextHash(const std::string& text) const;
    bool getModerationFromCache(const std::string& textHash, ModerationCacheEntry& entry);
    void putModerationToCache(const std::string& textHash, const ModerationCacheEntry& entry);
    void cleanExpiredCache();
};

} // namespace ai
} // namespace heartlake
