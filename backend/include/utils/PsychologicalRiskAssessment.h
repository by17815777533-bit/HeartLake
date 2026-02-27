/**
 * PsychologicalRiskAssessment 模块接口定义
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace heartlake {
namespace utils {

/**
 * 风险等级
 */
enum class RiskLevel {
    NONE = 0,       // 无风险
    LOW = 1,        // 低风险 (0.0-0.3)
    MEDIUM = 2,     // 中风险 (0.3-0.6)
    HIGH = 3,       // 高风险 (0.6-0.8)
    CRITICAL = 4    // 危急 (0.8-1.0)
};

/**
 * 风险因素
 */
struct RiskFactor {
    std::string category;       // 类别: linguistic, behavioral, contextual
    std::string name;           // 因素名称
    float score;                // 因素得分 (0.0-1.0)
    float weight;               // 权重
    std::string description;    // 描述
};

/**
 * 心理风险评估结果
 */
struct PsychologicalRiskResult {
    float overallScore;                     // 总体风险分数 (0.0-1.0)
    RiskLevel riskLevel;                    // 风险等级
    std::vector<RiskFactor> factors;        // 风险因素列表
    std::vector<std::string> keywords;      // 触发的关键词
    std::string primaryConcern;             // 主要关注点
    std::vector<std::string> interventions; // 干预建议
    bool needsImmediateAttention;           // 是否需要立即关注
    std::string supportMessage;             // 支持性消息
};

/**
 * 用户情绪历史记录
 */
struct EmotionHistoryEntry {
    std::string userId;
    float sentimentScore;
    std::string emotion;
    std::string content;
    int64_t timestamp;
};

/**
 * 行为模式分析结果
 */
struct BehaviorPattern {
    float negativePostFrequency;    // 负面发帖频率
    float engagementDecline;        // 参与度下降
    std::vector<int> activeHours;   // 活跃时段
    float socialIsolation;          // 社交孤立度
    int consecutiveNegativeDays;    // 连续负面天数
};

/**
 * 心理风险评估器
 */
/**
 * 心理风险评估工具
 *
 * 详细说明
 *
 * @note 注意事项
 */
class PsychologicalRiskAssessment {
public:
    static PsychologicalRiskAssessment& getInstance();

    /**
     * 评估文本内容的心理风险
     * @param text 文本内容
     * @param userId 用户ID
     * @param sentimentScore 情感分数
     * @param emotion 情绪类型
     * @return 风险评估结果
     */
    PsychologicalRiskResult assessRisk(
        const std::string& text,
        const std::string& userId,
        float sentimentScore,
        const std::string& emotion
    );

    /**
     * 分析用户行为模式
     * @param  用户ID
     * @param callback 回调函数
     */
    void analyzeBehaviorPattern(
        const std::string& userId,
        std::function<void(const BehaviorPattern&)> callback
    );

    /**
     * 记录用户情绪历史
     * @param userId 用户ID
     * @param sentimentScore 情感分数
     * @param emotion 情绪类型
     * @param content 内容
     */
    void recordEmotionHistory(
        const std::string& userId,
        float sentimentScore,
        const std::string& emotion,
        const std::string& content
    );

    /**
     * 获取用户情绪趋势
     * @param userId 用户ID
     * @param days 天数
     * @return 情绪分数列表
     */
    std::vector<float> getEmotionTrend(const std::string& userId, int days = 7);

    /**
     * 获取风险等级描述
     */
    static std::string getRiskLevelDescription(RiskLevel level);

    /**
     * 获取风险等级对应的颜色
     */
    static std::string getRiskLevelColor(RiskLevel level);

private:
    PsychologicalRiskAssessment();
    ~PsychologicalRiskAssessment() = default;
    PsychologicalRiskAssessment(const PsychologicalRiskAssessment&) = delete;
    PsychologicalRiskAssessment& operator=(const PsychologicalRiskAssessment&) = delete;

    /**
     * analyzeLinguisticMarkers方法
     *
     * @param text 参数说明
     * @param factors 参数说明
     * @return 返回值说明
     */
    float analyzeLinguisticMarkers(const std::string& text, std::vector<RiskFactor>& factors);

    /**
     * detectSelfHarmIntent方法
     *
     * @param text 参数说明
     * @param keywords 参数说明
     * @return 返回值说明
     */
    float detectSelfHarmIntent(const std::string& text, std::vector<std::string>& keywords);

    /**
     * detectHopelessness方法
     *
     * @param text 参数说明
     * @return 返回值说明
     */
    float detectHopelessness(const std::string& text);

    /**
     * detectIsolation方法
     *
     * @param text 参数说明
     * @return 返回值说明
     */
    float detectIsolation(const std::string& text);

    /**
     * detectTemporalUrgency方法
     *
     * @param text 参数说明
     * @return 返回值说明
     */
    float detectTemporalUrgency(const std::string& text);

    std::vector<std::string> generateInterventions(float riskScore, const std::string& primaryConcern);

    std::string generateSupportMessage(RiskLevel level, const std::string& primaryConcern);

    struct KeywordSet {
        std::vector<std::string> selfHarm;   // 自伤关键词
        std::vector<std::string> hopelessness;  // 绝望关键词
        std::vector<std::string> isolation;     // 孤立关键词
        std::vector<std::string> urgency;       // 紧迫性关键词
    };
    KeywordSet keywords_;

    /**
     * initializeKeywords方法
     */
    void initializeKeywords();
};

} // namespace utils
} // namespace heartlake
