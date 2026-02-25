/**
 * @file ContentModerator.h
 * @brief 内容审核子系统 - 从 EdgeAIEngine 提取的独立模块
 *
 * 两阶段审核：
 * 1. AC自动机快速多模式匹配（O(n)复杂度）
 * 2. 语义规则引擎二次判定（上下文分析 + 五因子心理风险评估）
 *
 * 支持分类：self_harm, violence, sexual, profanity
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <optional>
#include <json/json.h>
#include "utils/HighPerformance.h"

namespace heartlake {
namespace ai {

/**
 * @brief 五因子心理风险评估详情
 */
struct FiveFactorRiskDetail {
    float selfHarmIntent;       ///< 自残意图 (weight 0.9)
    float hopelessness;         ///< 绝望表达 (weight 0.6)
    float socialIsolation;      ///< 社交孤立 (weight 0.1)
    float temporalUrgency;      ///< 时间紧迫性 (weight 0.5)
    float linguisticMarkers;    ///< 语言风险标记 (weight 0.3)
    float compositeScore;       ///< 五因子加权综合分数

    Json::Value toJson() const {
        Json::Value j;
        j["self_harm_intent"] = selfHarmIntent;
        j["hopelessness"] = hopelessness;
        j["social_isolation"] = socialIsolation;
        j["temporal_urgency"] = temporalUrgency;
        j["linguistic_markers"] = linguisticMarkers;
        j["composite_score"] = compositeScore;
        return j;
    }
};

/**
 * @brief 本地审核结果
 */
struct EdgeModerationResult {
    bool passed;                              ///< 是否通过审核
    std::string riskLevel;                    ///< 风险等级: safe, low_risk, medium_risk, high_risk
    std::vector<std::string> matchedPatterns; ///< 匹配到的敏感模式
    std::vector<std::string> categories;      ///< 风险类别
    float confidence;                         ///< 置信度
    bool needsAlert;                          ///< 是否需要紧急关注（自伤倾向等）
    std::string suggestion;                   ///< 处理建议
    std::optional<FiveFactorRiskDetail> fiveFactorDetail; ///< 五因子心理风险详情

    Json::Value toJson() const {
        Json::Value j;
        j["passed"] = passed;
        j["risk_level"] = riskLevel;
        j["confidence"] = confidence;
        j["needs_alert"] = needsAlert;
        j["suggestion"] = suggestion;
        Json::Value pats(Json::arrayValue);
        for (const auto& p : matchedPatterns) pats.append(p);
        j["matched_patterns"] = pats;
        Json::Value cats(Json::arrayValue);
        for (const auto& c : categories) cats.append(c);
        j["categories"] = cats;
        if (fiveFactorDetail.has_value()) {
            j["five_factor_detail"] = fiveFactorDetail->toJson();
        }
        return j;
    }
};

/**
 * @brief 内容审核器 - AC自动机 + 五因子心理风险评估
 *
 * 线程安全。使用前需调用 buildModerationAC() 构建自动机。
 */
class ContentModerator {
public:
    /**
     * @brief 构建AC自动机敏感词库
     *
     * 包含四大类别：
     * - category 1: 自伤/自杀 (self_harm)
     * - category 2: 暴力 (violence)
     * - category 3: 不当内容 (sexual)
     * - category 0: 一般脏话/侮辱 (profanity)
     */
    void buildModerationAC();

    /**
     * @brief 本地文本审核
     *
     * 两阶段审核：
     * 1. AC自动机快速多模式匹配（O(n)复杂度）
     * 2. 语义规则引擎二次判定（上下文分析 + 五因子心理风险评估）
     *
     * @param text 待审核文本
     * @return 审核结果
     */
    EdgeModerationResult moderateTextLocal(const std::string& text);

    /**
     * @brief 获取总审核调用次数
     */
    size_t getTotalCalls() const { return totalModerationCalls_.load(); }

    /**
     * @brief 设置启用状态
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief 检查是否启用
     */
    bool isEnabled() const { return enabled_; }

private:
    /**
     * @brief 语义风险分析（融合AC匹配风险 + 五因子心理风险评估）
     */
    float semanticRiskAnalysis(const std::string& text,
                               const std::vector<perf::ACAutomaton::Match>& matches) const;

    perf::ACAutomaton moderationAC_;
    bool moderationACBuilt_ = false;
    std::atomic<bool> enabled_{true};  ///< 多线程读写，必须原子
    std::mutex moderationMutex_;
    std::atomic<size_t> totalModerationCalls_{0};
};

} // namespace ai
} // namespace heartlake
