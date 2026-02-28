/**
 * @brief 心理风险评估模块
 *
 * 通过语言学标记（自伤意图、绝望感、孤立感、紧迫性）和行为模式分析
 * （发帖频率、参与度变化、社交孤立度）综合评估用户心理风险等级，
 * 为SafeHarbor安全港机制提供风险信号输入。
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace heartlake {
namespace utils {

/** @brief 风险等级，按分数区间划分 */
enum class RiskLevel {
    NONE = 0,       // 无风险
    LOW = 1,        // 低风险 (0.0-0.3)
    MEDIUM = 2,     // 中风险 (0.3-0.6)
    HIGH = 3,       // 高风险 (0.6-0.8)
    CRITICAL = 4    // 危急 (0.8-1.0)
};

/** @brief 单个风险因素的评分与权重 */
struct RiskFactor {
    std::string category;       // 类别: linguistic, behavioral, contextual
    std::string name;
    float score;                // 因素得分 (0.0-1.0)
    float weight;
    std::string description;
};

/** @brief 心理风险评估的完整结果 */
struct PsychologicalRiskResult {
    float overallScore;                     // 总体风险分数 (0.0-1.0)
    RiskLevel riskLevel;
    std::vector<RiskFactor> factors;
    std::vector<std::string> keywords;      // 触发的关键词
    std::string primaryConcern;             // 主要关注点
    std::vector<std::string> interventions; // 干预建议
    bool needsImmediateAttention;           // 是否需要立即关注
    std::string supportMessage;             // 支持性消息
};

/** @brief 用户情绪历史记录条目 */
struct EmotionHistoryEntry {
    std::string userId;
    float sentimentScore;
    std::string emotion;
    std::string content;
    int64_t timestamp;
};

/** @brief 行为模式分析结果，用于辅助风险判定 */
struct BehaviorPattern {
    float negativePostFrequency;    // 负面发帖频率
    float engagementDecline;        // 参与度下降
    std::vector<int> activeHours;   // 活跃时段
    float socialIsolation;          // 社交孤立度
    int consecutiveNegativeDays;    // 连续负面天数
};

/**
 * @brief 心理风险评估器（单例）
 *
 * 融合语言学分析和行为模式分析两个维度，对用户心理状态进行综合评估。
 * 语言学维度检测自伤意图、绝望感、孤立感和紧迫性四类标记；
 * 行为维度追踪发帖频率、参与度变化和社交孤立度。
 */
class PsychologicalRiskAssessment {
public:
    static PsychologicalRiskAssessment& getInstance();

    /**
     * @brief 评估文本内容的心理风险
     * @param text 待评估文本
     * @param userId 用户ID，用于关联历史数据
     * @param sentimentScore 情感分析分数
     * @param emotion 情绪类型标签
     * @return 完整的风险评估结果
     */
    PsychologicalRiskResult assessRisk(
        const std::string& text,
        const std::string& userId,
        float sentimentScore,
        const std::string& emotion
    );

    /**
     * @brief 异步分析用户行为模式
     * @param userId 用户ID
     * @param callback 分析完成后的回调
     */
    void analyzeBehaviorPattern(
        const std::string& userId,
        std::function<void(const BehaviorPattern&)> callback
    );

    /**
     * @brief 记录用户情绪历史，供后续趋势分析使用
     * @param userId 用户ID
     * @param sentimentScore 情感分数
     * @param emotion 情绪类型
     * @param content 原始内容
     */
    void recordEmotionHistory(
        const std::string& userId,
        float sentimentScore,
        const std::string& emotion,
        const std::string& content
    );

    /**
     * @brief 获取用户近N天的情绪分数趋势
     * @param userId 用户ID
     * @param days 回溯天数，默认7天
     * @return 按时间排序的情绪分数序列
     */
    std::vector<float> getEmotionTrend(const std::string& userId, int days = 7);

    /// 获取风险等级的中文描述
    static std::string getRiskLevelDescription(RiskLevel level);
    /// 获取风险等级对应的颜色标识（用于前端展示）
    static std::string getRiskLevelColor(RiskLevel level);

private:
    PsychologicalRiskAssessment();
    ~PsychologicalRiskAssessment() = default;
    PsychologicalRiskAssessment(const PsychologicalRiskAssessment&) = delete;
    PsychologicalRiskAssessment& operator=(const PsychologicalRiskAssessment&) = delete;

    /** @brief 分析文本中的语言学风险标记，返回该维度的综合得分 */
    float analyzeLinguisticMarkers(const std::string& text, std::vector<RiskFactor>& factors);
    /** @brief 检测自伤意图关键词，返回匹配强度 */
    float detectSelfHarmIntent(const std::string& text, std::vector<std::string>& keywords);
    /** @brief 检测绝望感表达 */
    float detectHopelessness(const std::string& text);
    /** @brief 检测社交孤立感表达 */
    float detectIsolation(const std::string& text);
    /** @brief 检测时间紧迫性表达（如"最后一次"） */
    float detectTemporalUrgency(const std::string& text);

    /// 根据风险分数和主要关注点生成干预建议列表
    std::vector<std::string> generateInterventions(float riskScore, const std::string& primaryConcern);
    /// 根据风险等级生成温和的支持性消息
    std::string generateSupportMessage(RiskLevel level, const std::string& primaryConcern);

    /// 各类风险关键词集合
    struct KeywordSet {
        std::vector<std::string> selfHarm;      ///< 自伤意图关键词
        std::vector<std::string> hopelessness;   ///< 绝望感关键词
        std::vector<std::string> isolation;      ///< 孤立感关键词
        std::vector<std::string> urgency;        ///< 紧迫性关键词
    };
    KeywordSet keywords_;   ///< 当前加载的关键词库

    /// 初始化内置关键词库
    void initializeKeywords();
};

} // namespace utils
} // namespace heartlake
