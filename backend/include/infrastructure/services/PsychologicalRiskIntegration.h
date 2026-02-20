/**
 * @file PsychologicalRiskIntegration.h
 * @brief 心理风险评估集成服务
 * @author 白洋
 * @date 2026-02-08
 */

#pragma once

#include "PsychologicalRiskAssessment.h"
#include "services/NotificationPushService.h"
#include <drogon/drogon.h>
#include <memory>

namespace heartlake {
namespace services {

/**
 * @brief 风险等级
 */
enum class RiskLevel {
    SAFE = 0,      // 安全
    LOW = 1,       // 低风险
    MEDIUM = 2,    // 中风险
    HIGH = 3,      // 高风险
    CRITICAL = 4   // 危机级别
};

/**
 * @brief 风险评估结果
 */
struct RiskAssessmentResult {
    RiskLevel level;
    float score;                    // 0-100
    std::vector<std::string> keywords;
    std::vector<std::string> suggestions;
    bool needsIntervention;
    bool needsNotification;
    std::string interventionMessage;
};

/**
 * @brief 心理风险评估集成服务
 */
class PsychologicalRiskIntegration {
public:
    static PsychologicalRiskIntegration& getInstance() {
        static PsychologicalRiskIntegration instance;
        return instance;
    }

    /**
     * @brief 评估内容风险
     */
    RiskAssessmentResult assessContent(const std::string& content, const std::string& userId) {
        RiskAssessmentResult result;
        result.level = RiskLevel::SAFE;
        result.score = 0.0f;
        result.needsIntervention = false;
        result.needsNotification = false;

        // 1. 检测自杀/自残关键词
        auto suicideKeywords = detectSuicideKeywords(content);
        if (!suicideKeywords.empty()) {
            result.keywords.insert(result.keywords.end(),
                                  suicideKeywords.begin(),
                                  suicideKeywords.end());
            result.score += 80.0f;
            result.level = RiskLevel::CRITICAL;
            result.needsIntervention = true;
            result.needsNotification = true;
        }

        // 2. 检测抑郁情绪
        auto depressionScore = detectDepression(content);
        if (depressionScore > 0.7f) {
            result.score += depressionScore * 50.0f;
            if (result.level < RiskLevel::HIGH) {
                result.level = RiskLevel::HIGH;
            }
            result.needsNotification = true;
        }

        // 3. 检测焦虑情绪
        auto anxietyScore = detectAnxiety(content);
        if (anxietyScore > 0.7f) {
            result.score += anxietyScore * 30.0f;
            if (result.level < RiskLevel::MEDIUM) {
                result.level = RiskLevel::MEDIUM;
            }
        }

        // 4. 检测孤独感
        auto lonelinessScore = detectLoneliness(content);
        if (lonelinessScore > 0.7f) {
            result.score += lonelinessScore * 20.0f;
            if (result.level < RiskLevel::LOW) {
                result.level = RiskLevel::LOW;
            }
        }

        // 5. 生成建议和干预消息
        generateSuggestions(result);

        // 6. 记录评估结果
        recordAssessment(userId, result);

        return result;
    }

    /**
     * @brief 处理高风险内容
     */
    void handleHighRiskContent(const std::string& userId,
                               const std::string& contentId,
                               const RiskAssessmentResult& result) {
        if (result.level >= RiskLevel::HIGH) {
            // 1. 发送通知给用户
            sendSupportNotification(userId, result);

            // 2. 记录到数据库
            logHighRiskEvent(userId, contentId, result);

            // 3. 如果是危机级别，通知管理员
            if (result.level == RiskLevel::CRITICAL) {
                notifyAdministrators(userId, contentId, result);
            }
        }
    }

private:
    PsychologicalRiskIntegration() = default;
    ~PsychologicalRiskIntegration() = default;
    PsychologicalRiskIntegration(const PsychologicalRiskIntegration&) = delete;
    PsychologicalRiskIntegration& operator=(const PsychologicalRiskIntegration&) = delete;

    /**
     * @brief 检测自杀/自残关键词
     */
    std::vector<std::string> detectSuicideKeywords(const std::string& content) {
        std::vector<std::string> keywords;
        std::vector<std::string> suicideWords = {
            "自杀", "轻生", "不想活", "想死", "结束生命",
            "自残", "割腕", "跳楼", "上吊", "服毒"
        };

        for (const auto& word : suicideWords) {
            if (content.find(word) != std::string::npos) {
                keywords.push_back(word);
            }
        }

        return keywords;
    }

    /**
     * @brief 检测抑郁情绪
     */
    float detectDepression(const std::string& content) {
        std::vector<std::string> depressionWords = {
            "抑郁", "绝望", "无助", "痛苦", "难过",
            "悲伤", "失望", "空虚", "麻木", "没意义"
        };

        int count = 0;
        for (const auto& word : depressionWords) {
            if (content.find(word) != std::string::npos) {
                count++;
            }
        }

        return std::min(1.0f, count / 3.0f);
    }

    /**
     * @brief 检测焦虑情绪
     */
    float detectAnxiety(const std::string& content) {
        std::vector<std::string> anxietyWords = {
            "焦虑", "紧张", "害怕", "恐惧", "担心",
            "不安", "烦躁", "慌张", "压力", "崩溃"
        };

        int count = 0;
        for (const auto& word : anxietyWords) {
            if (content.find(word) != std::string::npos) {
                count++;
            }
        }

        return std::min(1.0f, count / 3.0f);
    }

    /**
     * @brief 检测孤独感
     */
    float detectLoneliness(const std::string& content) {
        std::vector<std::string> lonelinessWords = {
            "孤独", "寂寞", "孤单", "无人理解", "没人关心",
            "独自", "一个人", "没朋友", "被遗弃", "被忽视"
        };

        int count = 0;
        for (const auto& word : lonelinessWords) {
            if (content.find(word) != std::string::npos) {
                count++;
            }
        }

        return std::min(1.0f, count / 3.0f);
    }

    /**
     * @brief 生成建议
     */
    void generateSuggestions(RiskAssessmentResult& result) {
        switch (result.level) {
            case RiskLevel::CRITICAL:
                result.suggestions = {
                    "请立即寻求专业心理咨询帮助",
                    "全国心理危机干预热线：400-161-9995",
                    "24小时生命热线：400-821-1215",
                    "如果情况紧急，请拨打120或联系家人朋友"
                };
                result.interventionMessage =
                    "我们注意到你可能正在经历困难时期。请记住，你并不孤单，"
                    "总有人愿意倾听和帮助。请考虑联系专业的心理咨询服务。\n\n"
                    "全国心理危机干预热线：400-161-9995（24小时）\n"
                    "生命热线：400-821-1215（24小时）";
                break;

            case RiskLevel::HIGH:
                result.suggestions = {
                    "建议寻求心理咨询帮助",
                    "可以尝试与信任的朋友或家人倾诉",
                    "保持规律作息，适当运动",
                    "心理咨询热线：400-161-9995"
                };
                result.interventionMessage =
                    "感谢你愿意分享你的感受。如果你感到难以应对，"
                    "建议寻求专业心理咨询的帮助。记住，寻求帮助是勇敢的表现。";
                break;

            case RiskLevel::MEDIUM:
                result.suggestions = {
                    "尝试与朋友倾诉，分享你的感受",
                    "进行一些放松活动，如散步、听音乐",
                    "保持良好的睡眠和饮食习惯",
                    "如果情况持续，建议咨询心理专家"
                };
                break;

            case RiskLevel::LOW:
                result.suggestions = {
                    "继续保持与他人的交流",
                    "尝试培养兴趣爱好",
                    "适当运动有助于改善心情",
                    "保持积极的生活态度"
                };
                break;

            default:
                break;
        }
    }

    /**
     * @brief 发送支持通知
     */
    void sendSupportNotification(const std::string& userId, const RiskAssessmentResult& result) {
        if (!result.interventionMessage.empty()) {
            auto& pushService = NotificationPushService::getInstance();
            pushService.pushSystemNotice(
                userId,
                "心理健康关怀",
                result.interventionMessage
            );
        }
    }

    /**
     * @brief 记录评估结果
     */
    void recordAssessment(const std::string& userId, const RiskAssessmentResult& result) {
        try {
            auto dbClient = drogon::app().getDbClient("default");

            dbClient->execSqlAsync(
                "INSERT INTO psychological_assessments "
                "(user_id, risk_level, risk_score, keywords, created_at) "
                "VALUES ($1, $2, $3, $4, NOW())",
                [](const drogon::orm::Result&) {
                    LOG_DEBUG << "Psychological assessment recorded";
                },
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "Failed to record assessment: " << e.base().what();
                },
                userId,
                static_cast<int>(result.level),
                result.score,
                Json::arrayValue // keywords array
            );
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to record assessment: " << e.what();
        }
    }

    /**
     * @brief 记录高风险事件
     */
    void logHighRiskEvent(const std::string& userId,
                         const std::string& contentId,
                         const RiskAssessmentResult& result) {
        try {
            auto dbClient = drogon::app().getDbClient("default");

            dbClient->execSqlAsync(
                "INSERT INTO high_risk_events "
                "(user_id, content_id, risk_level, risk_score, intervention_sent, created_at) "
                "VALUES ($1, $2, $3, $4, $5, NOW())",
                [](const drogon::orm::Result&) {
                    LOG_INFO << "High risk event logged";
                },
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "Failed to log high risk event: " << e.base().what();
                },
                userId,
                contentId,
                static_cast<int>(result.level),
                result.score,
                result.needsIntervention
            );
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to log high risk event: " << e.what();
        }
    }

    /**
     * @brief 通知管理员
     */
    void notifyAdministrators(const std::string& userId,
                             const std::string& contentId,
                             const RiskAssessmentResult& result) {
        try {
            auto dbClient = drogon::app().getDbClient("default");

            // 获取所有管理员
            dbClient->execSqlAsync(
                "SELECT admin_id FROM admins WHERE is_active = true AND role IN ('admin', 'moderator')",
                [userId, contentId, result](const drogon::orm::Result& r) {
                    auto& pushService = NotificationPushService::getInstance();

                    for (const auto& row : r) {
                        std::string adminId = row["admin_id"].as<std::string>();

                        std::string message = "用户 " + userId + " 发布的内容（ID: " + contentId +
                                            "）检测到危机级别心理风险，请及时关注。";

                        pushService.pushSystemNotice(adminId, "⚠️ 危机预警", message);
                    }

                    LOG_WARN << "Critical psychological risk detected for user: " << userId;
                },
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "Failed to notify administrators: " << e.base().what();
                }
            );
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to notify administrators: " << e.what();
        }
    }
};

} // namespace services
} // namespace heartlake
