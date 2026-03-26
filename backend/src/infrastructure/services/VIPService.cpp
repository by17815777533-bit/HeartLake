/**
 * VIP 会员服务实现
 *
 * 心湖的"灯火"机制——为情绪低落的用户提供免费心理咨询等关怀权益。
 *
 * 自动发放逻辑：
 * - 计算全网 7 天情绪值的 P20 百分位数作为动态阈值
 * - 用户情绪分数低于 P20 时自动发放 30 天 VIP
 * - 已有有效 VIP 的用户不重复发放
 *
 * 权益体系：
 * - 基于 vip_privileges 表的 RBAC 权益控制
 * - 支持使用次数限制（max_usage / 30 天滚动窗口）
 * - AI 评论频率提升（普通 2h → VIP 30min）
 * - 免费心理咨询预约（每期 VIP 1 次）
 *
 * 过期保护：有进行中的咨询预约时延长 VIP 有效期，不中断服务。
 * 兼容历史数据：expires_at < 2000-01-01 视为永久 VIP。
 */

#include "infrastructure/services/VIPService.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <ctime>

using namespace heartlake::services;
using namespace heartlake::utils;
using namespace drogon;

namespace {
constexpr std::time_t kLegacyPermanentVipCutoff = 946684800;  // 2000-01-01 00:00:00 UTC

bool isLegacyPermanentVip(const std::time_t expiresAt) {
    return expiresAt > 0 && expiresAt < kLegacyPermanentVipCutoff;
}

bool isVipActiveByExpiry(const std::time_t expiresAt) {
    return isLegacyPermanentVip(expiresAt) || expiresAt > std::time(nullptr);
}

bool resolveVipPrivilege(
    const drogon::orm::DbClientPtr& dbClient,
    const std::string& userId,
    const std::string& privilegeKey
) {
    auto userResult = dbClient->execSqlSync(
        "SELECT vip_level, vip_expires_at FROM users WHERE user_id = $1",
        userId
    );

    if (userResult.empty()) {
        return false;
    }

    const auto vipLevel = userResult[0]["vip_level"].as<int>();
    if (vipLevel == 0) {
        return false;
    }

    if (!userResult[0]["vip_expires_at"].isNull()) {
        const auto expiresAt = userResult[0]["vip_expires_at"].as<std::time_t>();
        if (!isVipActiveByExpiry(expiresAt)) {
            return false;
        }
    }

    auto privilegeResult = dbClient->execSqlSync(
        "SELECT vip_level_required, is_active FROM vip_privileges WHERE privilege_key = $1",
        privilegeKey
    );

    if (privilegeResult.empty()) {
        LOG_WARN << "Unknown privilege key: " << privilegeKey;
        return false;
    }

    const bool isActive = privilegeResult[0]["is_active"].as<bool>();
    const int requiredLevel = privilegeResult[0]["vip_level_required"].as<int>();
    return isActive && (vipLevel >= requiredLevel);
}
}  // namespace

bool VIPService::checkEmotionAndGrantVIP(
    const std::string& userId,
    float emotionScore,
    const std::vector<std::string>& /*emotionTags*/
) {
    auto dbClient = app().getDbClient();

    try {
        // 1. 检查用户是否已经是VIP
        auto vipCheckResult = dbClient->execSqlSync(
            "SELECT vip_level, vip_expires_at FROM users WHERE user_id = $1",
            userId
        );

        if (vipCheckResult.size() > 0) {
            auto vipRow = vipCheckResult[0];
            int vipLevel = vipRow["vip_level"].as<int>();
            if (vipLevel > 0 && !vipRow["vip_expires_at"].isNull()) {
                auto expiresAt = vipRow["vip_expires_at"].as<std::time_t>();
                if (isVipActiveByExpiry(expiresAt)) {
                    LOG_DEBUG << "User " << userId << " is already VIP, skip grant";
                    return false;
                }
            }
        }

        // 2. 计算全网情绪值的20%百分位数
        auto percentileResult = dbClient->execSqlSync(
            "SELECT percentile_cont(0.20) WITHIN GROUP (ORDER BY avg_emotion_score) as p20 "
            "FROM user_emotion_profile "
            "WHERE date >= CURRENT_DATE - INTERVAL '7 days' AND avg_emotion_score IS NOT NULL"
        );

        if (percentileResult.size() == 0 || percentileResult[0]["p20"].isNull()) {
            LOG_ERROR << "Cannot calculate emotion percentile for VIP auto grant";
            return false;
        }

        float p20Threshold = percentileResult[0]["p20"].as<float>();
        LOG_DEBUG << "Emotion 20% percentile threshold: " << p20Threshold;

        // 如果用户情绪值不在最低20%，不发放VIP
        if (emotionScore > p20Threshold) {
            LOG_DEBUG << "User " << userId << " emotion score " << emotionScore
                     << " is above 20% threshold " << p20Threshold;
            return false;
        }

        // 3. 发放VIP（30天有效期）
        std::time_t expiresAt = std::time(nullptr) + (30 * 24 * 3600);

        auto trans = dbClient->newTransaction();
        trans->execSqlSync(
            "UPDATE users SET vip_level = 1, vip_expires_at = to_timestamp($1), "
            "updated_at = CURRENT_TIMESTAMP WHERE user_id = $2",
            expiresAt,
            userId
        );

        // 4. 记录升级日志
        trans->execSqlSync(
            "INSERT INTO vip_upgrade_logs (user_id, old_vip_level, new_vip_level, "
            "upgrade_type, reason, expires_at) VALUES ($1, 0, 1, $2, $3, to_timestamp($4))",
            userId,
            "auto_emotion",
            "情绪值低于全网20%，系统自动关怀",
            expiresAt
        );

        // 5. 发送通知
        sendVIPNotification(userId, 1, "情绪关怀");

        LOG_INFO << "VIP granted to user " << userId
                 << " due to low emotion (score: " << emotionScore << ", in bottom 20%)";

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error granting VIP to user " << userId << ": " << e.what();
        return false;
    }
}

bool VIPService::hasPrivilege(
    const std::string& userId,
    const std::string& privilegeKey
) {
    auto dbClient = app().getDbClient();

    try {
        return resolveVipPrivilege(dbClient, userId, privilegeKey);
    } catch (const std::exception& e) {
        LOG_ERROR << "Error checking privilege for user " << userId << ": " << e.what();
        return false;
    }
}

std::vector<std::string> VIPService::getUserPrivileges(const std::string& userId) {
    std::vector<std::string> privileges;
    auto dbClient = app().getDbClient();

    try {
        // 查询用户VIP等级
        auto userResult = dbClient->execSqlSync(
            "SELECT vip_level, vip_expires_at FROM users WHERE user_id = $1",
            userId
        );

        if (userResult.size() == 0) {
            return privileges;
        }

        int vipLevel = userResult[0]["vip_level"].as<int>();

        // 检查VIP是否过期
        if (!userResult[0]["vip_expires_at"].isNull()) {
            auto expiresAt = userResult[0]["vip_expires_at"].as<std::time_t>();
            if (!isVipActiveByExpiry(expiresAt)) {
                return privileges;  // VIP已过期
            }
        }

        if (vipLevel == 0) {
            return privileges;  // 不是VIP
        }

        // 查询所有可用特权
        auto privilegeResult = dbClient->execSqlSync(
            "SELECT privilege_key FROM vip_privileges "
            "WHERE is_active = true AND vip_level_required <= $1",
            vipLevel
        );

        for (size_t i = 0; i < privilegeResult.size(); ++i) {
            privileges.push_back(privilegeResult[i]["privilege_key"].as<std::string>());
        }

        return privileges;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error getting privileges for user " << userId << ": " << e.what();
        return privileges;
    }
}

bool VIPService::isVIPExpired(const std::string& userId) {
    auto dbClient = app().getDbClient();

    try {
        auto result = dbClient->execSqlSync(
            "SELECT vip_level, vip_expires_at FROM users WHERE user_id = $1",
            userId
        );

        if (result.size() == 0) {
            return true;
        }

        auto row = *safeRow(result);
        int vipLevel = row["vip_level"].as<int>();
        if (vipLevel == 0) {
            return true;  // 不是VIP
        }

        if (row["vip_expires_at"].isNull()) {
            return false;  // 永久VIP
        }

        auto expiresAt = row["vip_expires_at"].as<std::time_t>();
        if (isVipActiveByExpiry(expiresAt)) {
            return false;  // 未过期
        }

        // 检查是否有进行中的咨询预约，保护"灯"权限
        auto activeResult = dbClient->execSqlSync(
            "SELECT COUNT(*) as cnt FROM counseling_appointments "
            "WHERE user_id = $1 AND status IN ('scheduled', 'in_progress') "
            "AND is_free_vip = true",
            userId
        );
        if (safeCount(activeResult, "cnt") > 0) {
            return false;  // 有活跃咨询，延长VIP有效期
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error checking VIP expiration for user " << userId << ": " << e.what();
        return true;
    }
}

bool VIPService::upgradeVIP(
    const std::string& userId,
    int vipLevel,
    int durationDays,
    const std::string& reason
) {
    auto dbClient = app().getDbClient();

    try {
        // 查询当前VIP等级
        auto userResult = dbClient->execSqlSync(
            "SELECT vip_level FROM users WHERE user_id = $1",
            userId
        );

        if (userResult.size() == 0) {
            LOG_ERROR << "User not found: " << userId;
            return false;
        }

        int oldVipLevel = userResult[0]["vip_level"].as<int>();

        // 计算过期时间
        std::time_t expiresAt = std::time(nullptr) + (durationDays * 24 * 3600);

        // 更新用户VIP等级
        dbClient->execSqlSync(
            "UPDATE users SET vip_level = $1, vip_expires_at = to_timestamp($2), "
            "updated_at = CURRENT_TIMESTAMP WHERE user_id = $3",
            vipLevel,
            expiresAt,
            userId
        );

        // 记录升级日志
        dbClient->execSqlSync(
            "INSERT INTO vip_upgrade_logs (user_id, old_vip_level, new_vip_level, "
            "upgrade_type, reason, expires_at) VALUES ($1, $2, $3, $4, $5, to_timestamp($6))",
            userId,
            oldVipLevel,
            vipLevel,
            "manual",
            reason,
            expiresAt
        );

        // 发送通知
        sendVIPNotification(userId, vipLevel, reason);

        LOG_INFO << "User " << userId << " upgraded to VIP " << vipLevel
                 << " for " << durationDays << " days. Reason: " << reason;

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error upgrading VIP for user " << userId << ": " << e.what();
        return false;
    }
}

int VIPService::batchCheckEmotionsForVIP() {
    auto dbClient = app().getDbClient();

    try {
        auto result = dbClient->execSqlSync(
            "SELECT user_id, COALESCE(AVG(avg_emotion_score), 0) AS avg_emotion_score "
            "FROM user_emotion_profile "
            "WHERE date >= CURRENT_DATE - INTERVAL '7 days' "
            "GROUP BY user_id"
        );

        int count = 0;
        for (const auto& row : result) {
            const auto userId = row["user_id"].as<std::string>();
            const auto avgEmotionScore = row["avg_emotion_score"].as<float>();
            if (checkEmotionAndGrantVIP(userId, avgEmotionScore, {})) {
                count++;
            }
        }
        LOG_INFO << "Batch VIP emotion check completed. Granted VIP to " << count << " users.";
        return count;
    } catch (const std::exception& e) {
        LOG_ERROR << "Batch VIP emotion check failed: " << e.what();
        return 0;
    }
}

bool VIPService::isLowEmotion(
    float emotionScore,
    const std::vector<std::string>& emotionTags
) {
    // 情绪分数低于-0.3
    if (emotionScore < -0.3f) {
        return true;
    }

    // 或包含负面情绪标签
    static const std::vector<std::string> negativeEmotions = {
        "sad", "depressed", "anxious", "lonely",
        "hopeless", "stressed", "worried", "upset",
        "frustrated", "angry", "fearful"
    };

    for (const auto& tag : emotionTags) {
        if (std::find(negativeEmotions.begin(), negativeEmotions.end(), tag)
            != negativeEmotions.end()) {
            return true;
        }
    }

    return false;
}

void VIPService::sendVIPNotification(
    const std::string& userId,
    int vipLevel,
    const std::string& reason
) {
    auto dbClient = app().getDbClient();

    try {
        std::string notificationId = utils::IdGenerator::generateNotificationId();
        std::string content;

        if (reason.find("情绪") != std::string::npos || reason.find("关怀") != std::string::npos) {
            content = "我们注意到你可能心情不太好，特别为你开通了VIP会员，希望能给你带来更好的体验和关怀 ❤️";
        } else {
            content = "恭喜！你已成功升级为VIP " + std::to_string(vipLevel) + " 会员！";
        }

        dbClient->execSqlSync(
            "INSERT INTO notifications (notification_id, user_id, type, title, content, related_id) "
            "VALUES ($1, $2, $3, $4, $5, $6)",
            notificationId,
            userId,
            "system",
            "VIP会员通知",
            content,
            "vip_upgrade"
        );

        LOG_DEBUG << "VIP notification sent to user " << userId;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error sending VIP notification to user " << userId << ": " << e.what();
    }
}

Json::Value VIPService::getVIPStatus(const std::string& userId) {
    Json::Value status;
    auto dbClient = app().getDbClient();

    try {
        auto result = dbClient->execSqlSync(
            "SELECT vip_level, vip_expires_at FROM users WHERE user_id = $1",
            userId
        );

        if (result.size() == 0) {
            status["is_vip"] = false;
            status["vip_level"] = 0;
            return status;
        }

        auto row = *safeRow(result);
        int vipLevel = row["vip_level"].as<int>();
        status["vip_level"] = vipLevel;

        if (vipLevel > 0) {
            if (row["vip_expires_at"].isNull()) {
                status["is_vip"] = true;
                status["is_permanent"] = true;
            } else {
                auto expiresAt = row["vip_expires_at"].as<std::time_t>();
                if (isLegacyPermanentVip(expiresAt)) {
                    status["is_vip"] = true;
                    status["is_permanent"] = true;
                } else {
                    status["is_vip"] = (expiresAt > std::time(nullptr));
                    status["expires_at"] = static_cast<Json::Int64>(expiresAt);
                    status["is_permanent"] = false;

                    // 计算剩余天数
                    int daysLeft = (expiresAt - std::time(nullptr)) / (24 * 3600);
                    status["days_left"] = daysLeft;
                }
            }
        } else {
            status["is_vip"] = false;
        }

        // 获取用户的所有权益
        auto privileges = getUserPrivileges(userId);
        Json::Value privilegesArray(Json::arrayValue);
        for (const auto& priv : privileges) {
            privilegesArray.append(priv);
        }
        status["privileges"] = privilegesArray;

        return status;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error getting VIP status for user " << userId << ": " << e.what();
        status["is_vip"] = false;
        status["vip_level"] = 0;
        status["error"] = "VIP status query failed";
        return status;
    }
}

bool VIPService::checkAndRecordPrivilegeUsage(
    const std::string& userId,
    const std::string& privilegeKey,
    const std::string& metadata
) {
    auto dbClient = app().getDbClient();

    try {
        // 1. 检查用户是否有该权益
        if (!resolveVipPrivilege(dbClient, userId, privilegeKey)) {
            return false;
        }

        // 2. 获取权益配置
        auto configResult = dbClient->execSqlSync(
            "SELECT config FROM vip_privileges WHERE privilege_key = $1",
            privilegeKey
        );

        if (configResult.size() == 0) {
            return false;
        }

        // 3. 检查使用次数限制（如果有）
        std::string configJson = configResult[0]["config"].as<std::string>();
        Json::Value config;
        Json::Reader reader;

        if (reader.parse(configJson, config)) {
            if (config.isMember("max_usage")) {
                int maxUsage = config["max_usage"].asInt();

                // 查询当前使用次数
                auto usageResult = dbClient->execSqlSync(
                    "SELECT COALESCE(SUM(usage_count), 0) as total FROM vip_privilege_usage "
                    "WHERE user_id = $1 AND privilege_key = $2 "
                    "AND created_at >= NOW() - INTERVAL '30 days'",
                    userId,
                    privilegeKey
                );

                int currentUsage = safeCount(usageResult);
                if (currentUsage >= maxUsage) {
                    LOG_WARN << "User " << userId << " exceeded usage limit for " << privilegeKey;
                    return false;
                }
            }
        }

        // 4. 记录使用
        std::string metadataValue = metadata.empty() ? "{}" : metadata;
        dbClient->execSqlSync(
            "INSERT INTO vip_privilege_usage (user_id, privilege_key, usage_count, metadata) "
            "VALUES ($1, $2, 1, $3::jsonb) "
            "ON CONFLICT (user_id, privilege_key) DO UPDATE SET "
            "usage_count = vip_privilege_usage.usage_count + 1, "
            "last_used_at = NOW()",
            userId,
            privilegeKey,
            metadataValue
        );

        LOG_DEBUG << "Recorded privilege usage: " << privilegeKey << " for user " << userId;
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error checking privilege usage for user " << userId << ": " << e.what();
        return false;
    }
}

float VIPService::getAICommentFrequency(const std::string& userId) {
    auto dbClient = app().getDbClient();

    try {
        // 检查用户是否是VIP
        if (!resolveVipPrivilege(dbClient, userId, "ai_comment_frequent")) {
            // 普通用户：2小时一次
            return 2.0f;
        }

        // VIP用户：0.5小时（30分钟）一次
        return 0.5f;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error getting AI comment frequency for user " << userId << ": " << e.what();
        throw;
    }
}

bool VIPService::hasFreeCounselingQuota(const std::string& userId) {
    auto dbClient = app().getDbClient();

    try {
        // 1. 检查用户是否有免费咨询权益
        if (!resolveVipPrivilege(dbClient, userId, "free_counseling")) {
            return false;
        }

        // 2. 检查是否已经使用过免费额度
        auto usageResult = dbClient->execSqlSync(
            "SELECT COUNT(*) as count FROM counseling_appointments "
            "WHERE user_id = $1 AND is_free_vip = true "
            "AND created_at >= ("
            "  SELECT CASE "
            "    WHEN vip_expires_at IS NULL OR vip_expires_at < to_timestamp(946684800) "
            "      THEN NOW() - INTERVAL '30 days' "
            "    ELSE vip_expires_at - INTERVAL '30 days' "
            "  END "
            "  FROM users WHERE user_id = $1"
            ")",
            userId
        );

        int usedCount = safeCount(usageResult, "count");

        // VIP用户有1次免费咨询机会
        return usedCount < 1;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error checking free counseling quota for user " << userId << ": " << e.what();
        return false;
    }
}

std::string VIPService::bookCounseling(
    const std::string& userId,
    const std::string& appointmentTime,
    bool isFreeVIP
) {
    auto dbClient = app().getDbClient();

    try {
        // 1. 如果是免费VIP预约，检查额度
        if (isFreeVIP && !hasFreeCounselingQuota(userId)) {
            LOG_WARN << "User " << userId << " has no free counseling quota";
            return "";
        }

        // 2. 生成预约ID
        std::string appointmentId = "appt_" + utils::IdGenerator::generateNotificationId().substr(6);

        // 3. 创建预约记录
        dbClient->execSqlSync(
            "INSERT INTO counseling_appointments "
            "(appointment_id, user_id, appointment_time, status, is_free_vip) "
            "VALUES ($1, $2, $3::timestamp, 'scheduled', $4)",
            appointmentId,
            userId,
            appointmentTime,
            isFreeVIP
        );

        // 4. 如果是免费VIP预约，记录权益使用
        if (isFreeVIP) {
            Json::Value meta;
            meta["appointment_id"] = appointmentId;
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            checkAndRecordPrivilegeUsage(userId, "free_counseling",
                Json::writeString(writer, meta));
        }

        // 5. 发送预约确认通知
        std::string notificationId = utils::IdGenerator::generateNotificationId();
        std::string content = isFreeVIP
            ? "您的VIP免费心理咨询已预约成功！预约时间：" + appointmentTime
            : "您的心理咨询已预约成功！预约时间：" + appointmentTime;

        dbClient->execSqlSync(
            "INSERT INTO notifications (notification_id, user_id, type, title, content, related_id) "
            "VALUES ($1, $2, 'system', '预约确认', $3, $4)",
            notificationId,
            userId,
            content,
            appointmentId
        );

        LOG_INFO << "Counseling appointment created: " << appointmentId
                 << " for user " << userId << " (free: " << isFreeVIP << ")";

        return appointmentId;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error booking counseling for user " << userId << ": " << e.what();
        return "";
    }
}
