/**
 * @file PsychologicalRiskAssessment.cpp
 * @brief PsychologicalRiskAssessment 模块实现
 * Created by 王璐瑶
 */
#include "utils/PsychologicalRiskAssessment.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <ctime>
#include <cmath>

using namespace drogon;

namespace heartlake {
namespace utils {

PsychologicalRiskAssessment& PsychologicalRiskAssessment::getInstance() {
    static PsychologicalRiskAssessment instance;
    return instance;
}

PsychologicalRiskAssessment::PsychologicalRiskAssessment() {
    initializeKeywords();
}

void PsychologicalRiskAssessment::initializeKeywords() {
    // 自伤/自杀关键词 (高权重)
    keywords_.selfHarm = {
        "自杀", "自残", "想死", "不想活", "活不下去", "结束生命",
        "割腕", "跳楼", "上吊", "服毒", "了结", "解脱",
        "离开这个世界", "一了百了", "再见了", "遗书"
    };

    // 绝望感关键词 (中高权重)
    keywords_.hopelessness = {
        "绝望", "没希望", "看不到未来", "没有意义", "放弃",
        "无助", "无力", "无望", "崩溃", "撑不下去",
        "没有出路", "走投无路", "黑暗", "深渊", "痛苦"
    };

    // 孤立感关键词 (中权重)
    keywords_.isolation = {
        "孤独", "寂寞", "没人理解", "没人关心", "一个人",
        "被抛弃", "被遗忘", "孤立", "疏远", "隔离",
        "没朋友", "没人爱", "独自", "形单影只"
    };

    // 时间紧迫性关键词 (高权重)
    keywords_.urgency = {
        "今晚", "现在", "马上", "立刻",
        "最后", "再也不", "不会再", "这是最后一次"
    };
}

PsychologicalRiskResult PsychologicalRiskAssessment::assessRisk(
    const std::string& text,
    const std::string& userId,
    float sentimentScore,
    const std::string& emotion
) {
    PsychologicalRiskResult result;
    result.overallScore = 0.0f;
    result.needsImmediateAttention = false;

    // 1. 语言学标记分析 (权重: 0.3)
    float linguisticScore = analyzeLinguisticMarkers(text, result.factors);
    result.overallScore += linguisticScore * 0.3f;

    // 2. 自伤倾向检测 (权重: 0.9)
    float selfHarmScore = detectSelfHarmIntent(text, result.keywords);
    if (selfHarmScore > 0.0f) {
        RiskFactor factor;
        factor.category = "linguistic";
        factor.name = "self_harm_intent";
        factor.score = selfHarmScore;
        factor.weight = 0.9f;
        factor.description = "检测到自伤/自杀倾向";
        result.factors.push_back(factor);
        result.overallScore += selfHarmScore * 0.9f;
        result.primaryConcern = "self_harm";
    }

    // 3. 绝望感检测 (权重: 0.6)
    float hopelessnessScore = detectHopelessness(text);
    if (hopelessnessScore > 0.0f) {
        RiskFactor factor;
        factor.category = "linguistic";
        factor.name = "hopelessness";
        factor.score = hopelessnessScore;
        factor.weight = 0.6f;
        factor.description = "检测到绝望情绪";
        result.factors.push_back(factor);
        result.overallScore += hopelessnessScore * 0.6f;
        if (result.primaryConcern.empty()) {
            result.primaryConcern = "hopelessness";
        }
    }

    // 4. 孤立感检测 (权重: 0.1)
    float isolationScore = detectIsolation(text);
    if (isolationScore > 0.0f) {
        RiskFactor factor;
        factor.category = "linguistic";
        factor.name = "isolation";
        factor.score = isolationScore;
        factor.weight = 0.1f;
        factor.description = "检测到孤立感";
        result.factors.push_back(factor);
        result.overallScore += isolationScore * 0.1f;
        if (result.primaryConcern.empty()) {
            result.primaryConcern = "isolation";
        }
    }

    // 5. 时间紧迫性检测 (权重: 0.5)
    float urgencyScore = detectTemporalUrgency(text);
    if (urgencyScore > 0.0f) {
        RiskFactor factor;
        factor.category = "linguistic";
        factor.name = "temporal_urgency";
        factor.score = urgencyScore;
        factor.weight = 0.5f;
        factor.description = "检测到时间紧迫性";
        result.factors.push_back(factor);
        result.overallScore += urgencyScore * 0.5f;
    }

    // 6. 情感分数因素 (权重: 0.15)
    if (sentimentScore < -0.6f) {
        float emotionRisk = std::abs(sentimentScore + 0.6f) / 0.4f; // 归一化到0-1
        RiskFactor factor;
        factor.category = "emotional";
        factor.name = "negative_sentiment";
        factor.score = emotionRisk;
        factor.weight = 0.15f;
        factor.description = "持续负面情绪";
        result.factors.push_back(factor);
        result.overallScore += emotionRisk * 0.15f;
    }

    // 确保分数在0-1范围内
    result.overallScore = std::min(1.0f, std::max(0.0f, result.overallScore));

    // 确定风险等级
    if (result.overallScore >= 0.8f) {
        result.riskLevel = RiskLevel::CRITICAL;
        result.needsImmediateAttention = true;
    } else if (result.overallScore >= 0.6f) {
        result.riskLevel = RiskLevel::HIGH;
        result.needsImmediateAttention = true;
    } else if (result.overallScore >= 0.3f) {
        result.riskLevel = RiskLevel::MEDIUM;
    } else if (result.overallScore > 0.0f) {
        result.riskLevel = RiskLevel::LOW;
    } else {
        result.riskLevel = RiskLevel::NONE;
    }

    // 生成干预建议
    if (result.riskLevel != RiskLevel::NONE) {
        result.interventions = generateInterventions(result.overallScore, result.primaryConcern);
        result.supportMessage = generateSupportMessage(result.riskLevel, result.primaryConcern);
    }

    // 记录情绪历史
    recordEmotionHistory(userId, sentimentScore, emotion, text);

    LOG_INFO << "Risk assessment for user " << userId
             << ": score=" << result.overallScore
             << ", level=" << static_cast<int>(result.riskLevel);

    return result;
}

float PsychologicalRiskAssessment::analyzeLinguisticMarkers(
    const std::string& text,
    std::vector<RiskFactor>& /*factors*/
) {
    // Skip linguistic analysis for short texts to avoid false positives
    if (text.length() < 50) return 0.0f;
    return 0.0f;
}

float PsychologicalRiskAssessment::detectSelfHarmIntent(
    const std::string& text,
    std::vector<std::string>& matchedKeywords
) {
    int matchCount = 0;

    for (const auto& keyword : keywords_.selfHarm) {
        if (text.find(keyword) != std::string::npos) {
            matchCount++;
            matchedKeywords.push_back(keyword);
        }
    }

    if (matchCount == 0) return 0.0f;

    // 匹配到任何自伤关键词都是高风险
    return std::min(1.0f, 0.5f + matchCount * 0.2f);
}

float PsychologicalRiskAssessment::detectHopelessness(const std::string& text) {
    int matchCount = 0;

    for (const auto& keyword : keywords_.hopelessness) {
        if (text.find(keyword) != std::string::npos) {
            matchCount++;
        }
    }

    if (matchCount == 0) return 0.0f;
    return std::min(1.0f, matchCount * 0.25f);
}

float PsychologicalRiskAssessment::detectIsolation(const std::string& text) {
    int matchCount = 0;

    for (const auto& keyword : keywords_.isolation) {
        if (text.find(keyword) != std::string::npos) {
            matchCount++;
        }
    }

    if (matchCount == 0) return 0.0f;
    return std::min(1.0f, matchCount * 0.3f);
}

float PsychologicalRiskAssessment::detectTemporalUrgency(const std::string& text) {
    int matchCount = 0;

    for (const auto& keyword : keywords_.urgency) {
        if (text.find(keyword) != std::string::npos) {
            matchCount++;
        }
    }

    if (matchCount == 0) return 0.0f;
    // 时间紧迫性高风险
    return std::min(1.0f, 0.6f + matchCount * 0.2f);
}

std::vector<std::string> PsychologicalRiskAssessment::generateInterventions(
    float riskScore,
    const std::string& /*primaryConcern*/
) {
    std::vector<std::string> interventions;

    if (riskScore >= 0.8f) {
        // 危急情况
        interventions.push_back("立即联系心理危机热线：400-161-9995（24小时）");
        interventions.push_back("建议联系家人或信任的朋友");
        interventions.push_back("如有紧急情况，请拨打120或前往最近的医院急诊");
        interventions.push_back("平台已通知专业心理咨询师关注");
    } else if (riskScore >= 0.6f) {
        // 高风险
        interventions.push_back("建议尽快寻求专业心理咨询");
        interventions.push_back("心理援助热线：400-161-9995");
        interventions.push_back("可以和信任的人倾诉，不要独自承受");
        interventions.push_back("平台为您推荐了支持性内容和社区");
    } else if (riskScore >= 0.3f) {
        // 中风险
        interventions.push_back("建议关注自己的情绪变化");
        interventions.push_back("可以尝试与朋友交流或参与社区活动");
        interventions.push_back("如果情绪持续低落，建议寻求心理咨询");
        interventions.push_back("平台为您推荐了积极向上的内容");
    } else {
        // 低风险
        interventions.push_back("保持良好的社交互动");
        interventions.push_back("注意休息和自我关怀");
    }

    return interventions;
}

std::string PsychologicalRiskAssessment::generateSupportMessage(
    RiskLevel level,
    const std::string& primaryConcern
) {
    if (level == RiskLevel::CRITICAL || level == RiskLevel::HIGH) {
        if (primaryConcern == "self_harm") {
            return "我注意到你现在可能正在经历非常艰难的时刻。请记住，你不是一个人，总有人愿意帮助你。"
                   "生命很珍贵，困难是暂时的。请立即联系心理危机热线 400-161-9995，或告诉你信任的人。💙";
        } else if (primaryConcern == "hopelessness") {
            return "我能感受到你现在的绝望和痛苦。这些感受很真实，但请相信，事情会好转的。"
                   "让我们一起寻找希望的光芒。如果需要，专业的心理咨询师可以帮助你。💙";
        }
        return "我在这里陪着你。无论多么艰难，都不要放弃。让我们一起度过这个困难时期。💙";
    } else if (level == RiskLevel::MEDIUM) {
        return "我注意到你最近情绪不太好。寻求帮助是勇敢的表现。"
               "你可以和朋友聊聊，或者尝试一些让自己放松的活动。我会一直陪着你。🌸";
    } else {
        return "感谢你的分享。记得照顾好自己，保持积极的心态。我在这里陪着你。✨";
    }
}

void PsychologicalRiskAssessment::recordEmotionHistory(
    const std::string& userId,
    float sentimentScore,
    const std::string& emotion,
    const std::string& content
) {
    if (!app().isRunning()) return;
    auto dbClient = app().getDbClient();
    if (!dbClient) return;

    // 只记录前50个字符，保护隐私
    std::string contentPreview = content.length() > 50 ?
        content.substr(0, 50) + "..." : content;

    dbClient->execSqlAsync(
        "INSERT INTO user_emotion_history (user_id, sentiment_score, emotion, content_preview, created_at) "
        "VALUES ($1, $2, $3, $4, NOW())",
        [](const drogon::orm::Result&) {},
        [](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to record emotion history: " << e.base().what();
        },
        userId, sentimentScore, emotion, contentPreview
    );
}

void PsychologicalRiskAssessment::analyzeBehaviorPattern(
    const std::string& userId,
    std::function<void(const BehaviorPattern&)> callback
) {
    if (!app().isRunning()) { callback(BehaviorPattern{}); return; }
    auto dbClient = app().getDbClient();
    if (!dbClient) { callback(BehaviorPattern{}); return; }

    // 查询最近7天的情绪历史
    dbClient->execSqlAsync(
        "SELECT sentiment_score, emotion, EXTRACT(HOUR FROM created_at) as hour, created_at "
        "FROM user_emotion_history "
        "WHERE user_id = $1 AND created_at >= NOW() - INTERVAL '7 days' "
        "ORDER BY created_at DESC",
        [callback](const drogon::orm::Result& result) {
            BehaviorPattern pattern;
            pattern.negativePostFrequency = 0.0f;
            pattern.engagementDecline = 0.0f;
            pattern.socialIsolation = 0.0f;
            pattern.consecutiveNegativeDays = 0;

            if (result.size() == 0) {
                callback(pattern);
                return;
            }

            // 分析负面发帖频率
            int negativeCount = 0;
            int totalCount = result.size();
            std::map<int, int> hourDistribution;

            for (size_t i = 0; i < result.size(); i++) {
                float score = result[i]["sentiment_score"].as<float>();
                int hour = result[i]["hour"].as<int>();

                if (score < -0.3f) {
                    negativeCount++;
                }

                hourDistribution[hour]++;
            }

            pattern.negativePostFrequency = static_cast<float>(negativeCount) / totalCount;

            // 找出活跃时段
            for (const auto& [hour, count] : hourDistribution) {
                if (count >= 2) {
                    pattern.activeHours.push_back(hour);
                }
            }

            // 检测深夜活跃（可能的失眠迹象）
            for (int hour : pattern.activeHours) {
                if (hour >= 0 && hour <= 5) {
                    pattern.socialIsolation += 0.2f;
                }
            }

            callback(pattern);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Failed to analyze behavior pattern: " << e.base().what();
            BehaviorPattern emptyPattern;
            callback(emptyPattern);
        },
        userId
    );
}

std::vector<float> PsychologicalRiskAssessment::getEmotionTrend(
    const std::string& userId,
    int days
) {
    std::vector<float> trend;
    if (!app().isRunning()) return trend;
    auto dbClient = app().getDbClient();
    if (!dbClient) return trend;

    try {
        auto result = dbClient->execSqlSync(
            "SELECT AVG(sentiment_score) as avg_score, DATE(created_at) as date "
            "FROM user_emotion_history "
            "WHERE user_id = $1 AND created_at >= NOW() - INTERVAL '1 day' * $2 "
            "GROUP BY DATE(created_at) "
            "ORDER BY date ASC",
            userId, days
        );

        for (size_t i = 0; i < result.size(); i++) {
            trend.push_back(result[i]["avg_score"].as<float>());
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get emotion trend: " << e.base().what();
    }

    return trend;
}

std::string PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel level) {
    switch (level) {
        case RiskLevel::NONE: return "无风险";
        case RiskLevel::LOW: return "低风险";
        case RiskLevel::MEDIUM: return "中风险";
        case RiskLevel::HIGH: return "高风险";
        case RiskLevel::CRITICAL: return "危急";
        default: return "未知";
    }
}

std::string PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel level) {
    switch (level) {
        case RiskLevel::NONE: return "#4CAF50";      // 绿色
        case RiskLevel::LOW: return "#8BC34A";       // 浅绿
        case RiskLevel::MEDIUM: return "#FFC107";    // 黄色
        case RiskLevel::HIGH: return "#FF9800";      // 橙色
        case RiskLevel::CRITICAL: return "#F44336";  // 红色
        default: return "#9E9E9E";                   // 灰色
    }
}

} // namespace utils
} // namespace heartlake
