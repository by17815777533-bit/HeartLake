/**
 * @file ContentModerator.cpp
 * @brief 内容审核子系统 —— AC 自动机多模式匹配 + 五因子心理风险融合
 *
 * 审核流水线：
 *   1. buildModerationAC() 构建 Aho-Corasick 自动机，覆盖 5 大类别：
 *      自伤(1)、暴力(2)、不当内容(3)、违法犯罪(4)、脏话侮辱(0)
 *   2. moderateTextLocal() 三阶段审核：
 *      阶段1: AC 自动机 O(n) 快速扫描全部敏感模式
 *      阶段2: semanticRiskAnalysis() 综合匹配密度、文本长度、
 *             五因子心理风险评估计算融合风险分
 *      阶段3: 自伤检测特殊路径 —— AC 匹配或五因子模型任一触发即升级为紧急告警
 *
 * 从 EdgeAIEngine 提取的独立模块。
 */

#include "infrastructure/ai/ContentModerator.h"
#include "utils/PsychologicalRiskAssessment.h"
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <set>
#include <cmath>

namespace heartlake {
namespace ai {

/// 构建 AC 自动机：注册中英文敏感词模式，按类别(0-4)和风险等级(1-3)标注
void ContentModerator::buildModerationAC() {
    std::lock_guard<std::mutex> lock(moderationMutex_);

    // category: 1=self_harm, 2=violence, 3=sexual, 0=general_profanity
    // level: 1=low, 2=medium, 3=high

    // 自伤/自杀相关 (category=1, high risk)
    moderationAC_.addPattern("suicide", 1, 3);
    moderationAC_.addPattern("kill myself", 1, 3);
    moderationAC_.addPattern("end my life", 1, 3);
    moderationAC_.addPattern("self harm", 1, 3);
    moderationAC_.addPattern("want to die", 1, 3);
    moderationAC_.addPattern("自杀", 1, 3);
    moderationAC_.addPattern("自残", 1, 3);
    moderationAC_.addPattern("不想活", 1, 3);
    moderationAC_.addPattern("轻生", 1, 3);
    moderationAC_.addPattern("结束生命", 1, 3);

    // 暴力相关 (category=2)
    moderationAC_.addPattern("murder", 2, 3);
    moderationAC_.addPattern("kill you", 2, 3);
    moderationAC_.addPattern("beat you up", 2, 2);
    moderationAC_.addPattern("violence", 2, 2);
    moderationAC_.addPattern("attack", 2, 1);
    moderationAC_.addPattern("杀人", 2, 3);
    moderationAC_.addPattern("杀了", 2, 2);
    moderationAC_.addPattern("杀掉", 2, 2);
    moderationAC_.addPattern("想杀", 2, 3);
    moderationAC_.addPattern("去杀", 2, 3);
    moderationAC_.addPattern("打死", 2, 3);
    moderationAC_.addPattern("暴力", 2, 2);
    moderationAC_.addPattern("殴打", 2, 2);
    moderationAC_.addPattern("杀戮", 2, 3);
    moderationAC_.addPattern("血腥", 2, 2);
    moderationAC_.addPattern("恐怖袭击", 2, 3);
    moderationAC_.addPattern("炸掉", 2, 3);
    moderationAC_.addPattern("炸弹", 2, 3);
    moderationAC_.addPattern("爆炸", 2, 2);
    moderationAC_.addPattern("枪支", 2, 3);
    moderationAC_.addPattern("弹药", 2, 3);
    moderationAC_.addPattern("贩卖人口", 2, 3);

    // 不当内容 (category=3)
    moderationAC_.addPattern("porn", 3, 3);
    moderationAC_.addPattern("nude", 3, 2);
    moderationAC_.addPattern("sexual", 3, 2);
    moderationAC_.addPattern("色情", 3, 3);
    moderationAC_.addPattern("裸体", 3, 2);
    moderationAC_.addPattern("招嫖", 3, 3);
    moderationAC_.addPattern("偷拍", 3, 2);
    moderationAC_.addPattern("偷窥", 3, 2);

    // 违法犯罪 (category=4)
    moderationAC_.addPattern("毒品", 4, 3);
    moderationAC_.addPattern("吸毒", 4, 3);
    moderationAC_.addPattern("贩毒", 4, 3);
    moderationAC_.addPattern("赌博", 4, 3);
    moderationAC_.addPattern("传销", 4, 3);
    moderationAC_.addPattern("洗钱", 4, 3);
    moderationAC_.addPattern("诈骗", 4, 3);
    moderationAC_.addPattern("非法集资", 4, 3);
    moderationAC_.addPattern("黄赌毒", 4, 3);
    moderationAC_.addPattern("代开发票", 4, 2);

    // 一般脏话/侮辱 (category=0)
    moderationAC_.addPattern("fuck", 0, 2);
    moderationAC_.addPattern("shit", 0, 1);
    moderationAC_.addPattern("damn", 0, 1);
    moderationAC_.addPattern("idiot", 0, 1);
    moderationAC_.addPattern("stupid", 0, 1);
    moderationAC_.addPattern("傻逼", 0, 2);
    moderationAC_.addPattern("操你", 0, 2);
    moderationAC_.addPattern("我操", 0, 2);
    moderationAC_.addPattern("操他", 0, 2);
    moderationAC_.addPattern("垃圾", 0, 1);
    moderationAC_.addPattern("白痴", 0, 1);

    moderationAC_.build();
    moderationACBuilt_ = true;

    LOG_INFO << "[ContentModerator] Moderation AC automaton built successfully";
}

/**
 * 语义风险分析：融合 AC 匹配结果与五因子心理风险评估。
 *
 * 计算逻辑：
 *   1. 每个匹配按 level 映射基础风险（low=0.3, medium=0.6, high=0.9），自伤类 ×1.2
 *   2. 多匹配叠加：combinedRisk = maxRisk*0.6 + avgRisk*0.4，再乘密度因子 log2(N)
 *   3. 短文本惩罚：<50字 ×1.3，<200字 ×1.1，≥200字 ×0.9
 *   4. 与五因子心理风险取 max，确保任一维度高风险不被低估
 */
float ContentModerator::semanticRiskAnalysis(const std::string& text,
                                              const std::vector<perf::ACAutomaton::Match>& matches) const {
    if (matches.empty()) return 0.0f;

    float maxRisk = 0.0f;
    float totalRisk = 0.0f;

    for (const auto& m : matches) {
        float baseRisk = 0.0f;
        switch (m.level) {
            case 1: baseRisk = 0.3f; break;
            case 2: baseRisk = 0.6f; break;
            case 3: baseRisk = 0.9f; break;
            default: baseRisk = 0.2f; break;
        }

        // 自伤类别额外加权
        if (m.category == 1) {
            baseRisk = std::min(1.0f, baseRisk * 1.2f);
        }

        maxRisk = std::max(maxRisk, baseRisk);
        totalRisk += baseRisk;
    }

    // 多个匹配叠加风险
    float densityFactor = 1.0f + std::log2(static_cast<float>(matches.size()));
    float combinedRisk = maxRisk * 0.6f + (totalRisk / static_cast<float>(matches.size())) * 0.4f;
    combinedRisk *= std::min(densityFactor, 2.0f);

    // 文本长度归一化：短文本中出现敏感词风险更高
    float lengthFactor = 1.0f;
    if (text.size() < 50) {
        lengthFactor = 1.3f;
    } else if (text.size() < 200) {
        lengthFactor = 1.1f;
    } else {
        lengthFactor = 0.9f;
    }

    float acRisk = std::clamp(combinedRisk * lengthFactor, 0.0f, 1.0f);

    // 五因子心理风险评估融合
    auto& pra = utils::PsychologicalRiskAssessment::getInstance();
    auto praResult = pra.assessRisk(text, "", 0.0f, "");
    float fiveFactorScore = praResult.overallScore;

    // 融合策略：取AC匹配风险和五因子风险的最大值
    // 确保任一维度的高风险都不会被低估
    return std::clamp(std::max(acRisk, fiveFactorScore), 0.0f, 1.0f);
}

/**
 * 本地文本审核入口 —— 三阶段流水线。
 *
 * 阶段1: AC 自动机扫描（转小写后匹配），无命中直接返回 safe
 * 阶段2: semanticRiskAnalysis 融合风险评分 + 五因子心理评估
 * 阶段3: 自伤特殊路径 —— AC 匹配到 category=1 或五因子 selfHarmIntent>0.5
 *         均触发 needsAlert，强制升级为 high_risk
 */
EdgeModerationResult ContentModerator::moderateTextLocal(const std::string& text) {
    ++totalModerationCalls_;

    EdgeModerationResult result;
    result.passed = true;
    result.riskLevel = "safe";
    result.confidence = 0.9f;
    result.needsAlert = false;
    result.suggestion = "";

    if (!enabled_ || text.empty()) {
        return result;
    }

    // 转小写用于匹配
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 阶段1: AC自动机快速多模式匹配
    auto matches = moderationAC_.match(lowerText);

    if (matches.empty()) {
        result.passed = true;
        result.riskLevel = "safe";
        result.confidence = 0.85f;
        return result;
    }

    // 收集匹配到的模式和类别
    std::set<std::string> categorySet;
    bool hasSelfHarm = false;
    uint8_t maxLevel = 0;

    for (const auto& m : matches) {
        // 从匹配位置提取模式文本
        result.matchedPatterns.push_back(
            lowerText.substr(std::max(0, m.position - 20), 40));

        switch (m.category) {
            case 0: categorySet.insert("profanity"); break;
            case 1: categorySet.insert("self_harm"); hasSelfHarm = true; break;
            case 2: categorySet.insert("violence"); break;
            case 3: categorySet.insert("sexual"); break;
            default: categorySet.insert("other"); break;
        }
        maxLevel = std::max(maxLevel, m.level);
    }

    result.categories.assign(categorySet.begin(), categorySet.end());

    // 阶段2: 语义风险分析（已融合五因子模型）
    float riskScore = semanticRiskAnalysis(text, matches);

    // 阶段3: 提取五因子详情填充到结果中
    auto& pra = utils::PsychologicalRiskAssessment::getInstance();
    auto praResult = pra.assessRisk(text, "", 0.0f, "");

    FiveFactorRiskDetail fiveFactorDetail;
    fiveFactorDetail.selfHarmIntent = 0.0f;
    fiveFactorDetail.hopelessness = 0.0f;
    fiveFactorDetail.socialIsolation = 0.0f;
    fiveFactorDetail.temporalUrgency = 0.0f;
    fiveFactorDetail.linguisticMarkers = 0.0f;
    fiveFactorDetail.compositeScore = praResult.overallScore;

    // 从 PsychologicalRiskResult 的 factors 中提取各因子分数
    for (const auto& factor : praResult.factors) {
        if (factor.name == "self_harm_intent") {
            fiveFactorDetail.selfHarmIntent = factor.score;
        } else if (factor.name == "hopelessness") {
            fiveFactorDetail.hopelessness = factor.score;
        } else if (factor.name == "social_isolation") {
            fiveFactorDetail.socialIsolation = factor.score;
        } else if (factor.name == "temporal_urgency") {
            fiveFactorDetail.temporalUrgency = factor.score;
        } else if (factor.category == "linguistic") {
            // analyzeLinguisticMarkers 产生的因子
            fiveFactorDetail.linguisticMarkers = std::max(fiveFactorDetail.linguisticMarkers, factor.score);
        }
    }

    result.fiveFactorDetail = fiveFactorDetail;

    // 确定风险等级
    if (riskScore >= 0.8f || maxLevel >= 3) {
        result.riskLevel = "high_risk";
        result.passed = false;
        result.confidence = std::min(0.95f, riskScore);
    } else if (riskScore >= 0.75f) {
        result.riskLevel = "medium_risk";
        result.passed = false;
        result.confidence = std::min(0.85f, riskScore + 0.1f);
    } else {
        result.riskLevel = "low_risk";
        result.passed = true;
        result.confidence = 0.7f;
    }

    // 自伤检测需要紧急关注（AC匹配或五因子模型检出）
    bool fiveFactorSelfHarm = fiveFactorDetail.selfHarmIntent > 0.5f;
    if (hasSelfHarm || fiveFactorSelfHarm || praResult.needsImmediateAttention) {
        result.needsAlert = true;
        result.passed = false;
        result.riskLevel = "high_risk";
        result.suggestion = "Detected potential self-harm content. "
                           "Please provide crisis support resources and escalate to human moderator.";
        if (!praResult.supportMessage.empty()) {
            result.suggestion += " " + praResult.supportMessage;
        }
    } else if (result.riskLevel == "high_risk") {
        result.suggestion = "High-risk content detected. Recommend blocking and review by human moderator.";
    } else if (result.riskLevel == "medium_risk") {
        result.suggestion = "Medium-risk content detected. Consider flagging for review.";
    }

    return result;
}

} // namespace ai
} // namespace heartlake
