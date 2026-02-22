/**
 * @file DualMemoryRAG.h
 * @brief 双记忆RAG情感守护系统
 *
 * 创新点：基于2024年SoulSpeak论文的双记忆架构
 * - 短期记忆：最近5次交互上下文
 * - 长期记忆：用户情绪画像（从emotion_tracking聚合）
 * - 隐私保护：所有记忆使用shadow_id，不关联真实身份
 *
 * 参考论文：SoulSpeak (arXiv, Dec 2024) - Dual-Memory RAG for Psychotherapy
 */
#pragma once

#include <string>
#include <vector>
#include <json/json.h>
#include <drogon/drogon.h>
#include <mutex>
#include <unordered_map>

namespace heartlake::ai {

struct EmotionMemory {
    std::string userId;
    // 短期记忆：最近交互
    struct ShortTermEntry {
        std::string content;
        std::string emotion;
        float score;
        std::string timestamp;
    };
    std::vector<ShortTermEntry> shortTerm;  // max 5 entries

    // 长期记忆：情绪画像
    struct LongTermProfile {
        float avgEmotionScore = 0.0f;
        int totalPosts = 0;
        std::string dominantMood = "neutral";
        float emotionVolatility = 0.0f;  // 情绪波动度
        std::string emotionTrend = "stable";  // rising/falling/stable
        int consecutiveNegativeDays = 0;
        std::string lastActiveDate;
    };
    LongTermProfile longTerm;
};

class DualMemoryRAG {
public:
    static DualMemoryRAG& getInstance();

    /**
     * @brief 生成带有双记忆上下文的AI回复
     * @param userId 用户ID（使用shadow_id保护隐私）
     * @param currentContent 当前内容
     * @param currentEmotion 当前情绪
     * @param emotionScore 情绪分数
     * @return AI生成的个性化回复
     */
    std::string generateResponse(
        const std::string& userId,
        const std::string& currentContent,
        const std::string& currentEmotion,
        float emotionScore
    );

    /**
     * @brief 更新用户的短期记忆
     */
    void updateShortTermMemory(
        const std::string& userId,
        const std::string& content,
        const std::string& emotion,
        float score
    );

    /**
     * @brief 从数据库加载/刷新长期记忆
     */
    void refreshLongTermMemory(const std::string& userId);

    /**
     * @brief 构建RAG提示词（含双记忆上下文）
     */
    std::string buildRAGPrompt(
        const EmotionMemory& memory,
        const std::string& currentContent,
        const std::string& currentEmotion
    );

    /**
     * @brief 获取用户情绪画像（用于insights API）
     */
    Json::Value getEmotionInsights(const std::string& userId);

    /**
     * @brief 获取RAG系统运行指标
     */
    Json::Value getStats() const;

private:
    DualMemoryRAG() = default;
    std::unordered_map<std::string, EmotionMemory> memories_;
    mutable std::mutex mutex_;

    static constexpr int MAX_SHORT_TERM = 5;

    EmotionMemory& getOrCreateMemory(const std::string& userId);
    std::string calculateTrend(const std::vector<float>& scores);
    float calculateVolatility(const std::vector<float>& scores);
};

} // namespace heartlake::ai
