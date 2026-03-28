/**
 * 双记忆RAG情感守护系统
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
#include <shared_mutex>
#include <unordered_map>

namespace heartlake::ai {

struct RagReplyResult {
    std::string reply;
    std::string source = "ai";
    std::string error;
    bool degraded = false;
};

/**
 * @brief 用户情绪记忆体，包含短期交互记忆和长期情绪画像
 * @details 所有记忆使用 shadow_id 存储，不关联真实身份，保护用户隐私。
 */
struct EmotionMemory {
    std::string userId;  ///< 用户 shadow_id

    /**
     * @brief 短期记忆条目 — 记录单次交互的情绪快照
     */
    struct ShortTermEntry {
        std::string content;           ///< 交互文本内容
        std::string emotion;           ///< 情绪类型标签
        float score;                   ///< 情绪分数 [-1.0, 1.0]
        std::string timestamp;         ///< ISO 8601 时间戳
        std::vector<float> embedding;  ///< 预计算 embedding，避免检索时重复生成
        int accessCount = 0;           ///< 被 RAG 检索命中的次数（用于相关性淘汰）
        double lastAccessTime = 0;     ///< 最近被检索的时间（epoch seconds）
    };
    std::vector<ShortTermEntry> shortTerm;  ///< 短期记忆队列，最多 MAX_SHORT_TERM 条

    /**
     * @brief 长期记忆画像 — 从 emotion_tracking 表聚合的用户情绪统计
     */
    struct LongTermProfile {
        float avgEmotionScore = 0.0f;          ///< 历史平均情绪分数
        float decayWeightedScore = 0.0f;       ///< Ebbinghaus 指数衰减加权情绪分
        int totalPosts = 0;                    ///< 历史发帖总数
        std::string dominantMood = "neutral";  ///< 主导情绪类型
        float emotionVolatility = 0.0f;        ///< 情绪波动度（标准差）
        std::string emotionTrend = "stable";   ///< 情绪趋势: rising / falling / stable
        int consecutiveNegativeDays = 0;       ///< 连续负面情绪天数（用于风险预警）
        std::string lastActiveDate;            ///< 最后活跃日期
        double lastRefreshTime = 0.0;          ///< 最近一次从 DB 刷新画像的时间
        bool refreshInFlight = false;          ///< 是否已有线程在刷新长期画像
    };
    LongTermProfile longTerm;  ///< 长期情绪画像
};

/**
 * @brief 双记忆 RAG 情感守护系统
 *
 * @details 基于 SoulSpeak (arXiv, Dec 2024) 论文的双记忆架构，将短期交互上下文
 * 和长期情绪画像融合到 RAG 提示词中，生成个性化的心理支持回复。
 *
 * 记忆管理策略：
 * - 短期记忆：保留最近 5 条交互，超限时按相关性淘汰最不相关的条目
 * - 长期记忆：从 emotion_tracking 表聚合最近 30 天数据，Ebbinghaus 指数衰减加权
 *
 * 单例模式，线程安全（shared_mutex 读多写少场景优化）。
 */
class DualMemoryRAG {
public:
    /** @brief 获取全局单例 */
    static DualMemoryRAG& getInstance();

    /**
     * 生成带有双记忆上下文的AI回复
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

    RagReplyResult generateResponseResult(
        const std::string& userId,
        const std::string& currentContent,
        const std::string& currentEmotion,
        float emotionScore
    );

    /**
     * 更新用户的短期记忆
     */
    void updateShortTermMemory(
        const std::string& userId,
        const std::string& content,
        const std::string& emotion,
        float score
    );

    /**
     * 从数据库加载/刷新长期记忆
     */
    void refreshLongTermMemory(const std::string& userId);

    /**
     * 构建RAG提示词（含双记忆上下文）
     */
    std::string buildRAGPrompt(
        const EmotionMemory& memory,
        const std::string& currentContent,
        const std::string& currentEmotion,
        const std::vector<size_t>* selectedShortTerm = nullptr
    );

    /**
     * 获取用户情绪画像（用于insights API）
     */
    Json::Value getEmotionInsights(const std::string& userId);

    /**
     * 获取RAG系统运行指标
     */
    Json::Value getStats() const;

private:
    DualMemoryRAG() = default;
    std::unordered_map<std::string, EmotionMemory> memories_;  ///< shadow_id -> 记忆体映射
    mutable std::shared_mutex mutex_;  ///< 读多写少场景，用共享锁提升并发读性能

    static constexpr int MAX_SHORT_TERM = 5;              ///< 短期记忆最大保留条数
    static constexpr int LONG_TERM_RETENTION_DAYS = 30;   ///< 长期记忆聚合回溯天数
    static constexpr int LONG_TERM_REFRESH_INTERVAL_SECONDS = 180;  ///< 长期画像最小刷新间隔
    static constexpr float DECAY_LAMBDA = 0.05f;          ///< Ebbinghaus 指数衰减系数

    /**
     * @brief 获取或创建用户记忆体
     * @param userId 用户 shadow_id
     * @return 记忆体引用（调用方需持有锁）
     */
    EmotionMemory& getOrCreateMemory(const std::string& userId);

    /**
     * @brief 获取用户记忆快照，昂贵计算在锁外进行
     */
    EmotionMemory getMemorySnapshot(const std::string& userId);

    /**
     * @brief 刷新长期画像（可选按刷新间隔节流）
     */
    void refreshLongTermMemoryImpl(const std::string& userId, bool forceRefresh, double nowEpoch);

    /**
     * @brief 将当前对话写入短期记忆，并复用已生成 embedding
     */
    void updateShortTermMemoryInternal(
        const std::string& userId,
        const std::string& content,
        const std::string& emotion,
        float score,
        const std::vector<float>& contentEmbedding
    );

    /**
     * @brief 根据情绪分数序列计算趋势方向
     * @return "rising" / "falling" / "stable"
     */
    std::string calculateTrend(const std::vector<float>& scores);

    /**
     * @brief 计算情绪波动度（标准差）
     */
    float calculateVolatility(const std::vector<float>& scores);

    /**
     * @brief 判断当前用户长期画像是否需要刷新
     */
    bool shouldRefreshLongTermMemory(const std::string& userId, double nowEpoch) const;

    /**
     * @brief 记录被命中的短期记忆访问统计
     */
    void markShortTermEntriesAccessed(
        const std::string& userId,
        const std::vector<size_t>& indices
    );

    /**
     * @brief 基于相关性的短期记忆淘汰
     * @details 当短期记忆超过 MAX_SHORT_TERM 时，综合考虑与当前上下文的
     *          情绪匹配度、访问频率和时间新鲜度，淘汰最不相关的条目。
     * @param entries 短期记忆条目列表（会被修改）
     * @param currentContent 当前交互内容
     * @param currentEmotion 当前情绪类型
     */
    void evictLeastRelevant(
        std::vector<EmotionMemory::ShortTermEntry>& entries,
        const std::string& currentContent,
        const std::string& currentEmotion,
        const std::vector<float>* currentEmbedding = nullptr
    );
};

} // namespace heartlake::ai
