/**
 * @file GuardianIncentiveService.cpp
 * @brief 守望者激励系统实现
 */

#include "infrastructure/services/GuardianIncentiveService.h"
#include "infrastructure/services/NotificationPushService.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <cmath>

namespace heartlake::infrastructure {

GuardianIncentiveService& GuardianIncentiveService::getInstance() {
    static GuardianIncentiveService instance;
    return instance;
}

void GuardianIncentiveService::recordQualityRipple(const std::string& userId, const std::string& stoneId) {
    addResonancePoints(userId, RIPPLE_POINTS, "quality_ripple:" + stoneId);
    checkAndGrantGuardian(userId);
}

void GuardianIncentiveService::recordWarmBoat(const std::string& userId, const std::string& boatId, float warmthScore) {
    const float clamped = std::clamp(warmthScore, 0.2f, 1.0f);
    int points = std::max(1, static_cast<int>(std::round(WARM_BOAT_POINTS * clamped)));
    if (points > 0) {
        addResonancePoints(userId, points, "warm_boat:" + boatId);
        checkAndGrantGuardian(userId);
    }
}

void GuardianIncentiveService::addResonancePoints(const std::string& userId, int points, const std::string& reason) {
    auto db = drogon::app().getDbClient("default");
    try {
        auto trans = db->newTransaction();
        trans->execSqlSync(
            "INSERT INTO resonance_points (user_id, points, reason, created_at) "
            "VALUES ($1, $2, $3, NOW())",
            userId, points, reason
        );

        trans->execSqlSync(
            "UPDATE users SET resonance_total = COALESCE(resonance_total, 0) + $1 WHERE user_id = $2",
            points, userId
        );
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "addResonancePoints failed: " << e.base().what();
    }
}

GuardianStats GuardianIncentiveService::getGuardianStats(const std::string& userId) {
    GuardianStats stats;
    stats.userId = userId;
    stats.totalResonancePoints = 0;
    stats.qualityRipples = 0;
    stats.warmBoats = 0;
    stats.isGuardian = false;
    stats.canTransferLamp = false;

    auto db = drogon::app().getDbClient("default");
    try {
        auto result = db->execSqlSync(
            "SELECT COALESCE(resonance_total, 0) as total, "
            "COALESCE(is_guardian, false) as guardian "
            "FROM users WHERE user_id = $1",
            userId
        );

        if (!result.empty()) {
            stats.totalResonancePoints = result[0]["total"].as<int>();
            stats.isGuardian = result[0]["guardian"].as<bool>();
            stats.canTransferLamp = stats.isGuardian && stats.totalResonancePoints >= GUARDIAN_THRESHOLD * 2;
        }

        auto countResult = db->execSqlSync(
            "SELECT "
            "SUM(CASE WHEN reason LIKE 'quality_ripple%' THEN 1 ELSE 0 END) as ripples, "
            "SUM(CASE WHEN reason LIKE 'warm_boat%' THEN 1 ELSE 0 END) as boats "
            "FROM resonance_points WHERE user_id = $1",
            userId
        );

        if (!countResult.empty()) {
            stats.qualityRipples = countResult[0]["ripples"].isNull() ? 0 : countResult[0]["ripples"].as<int>();
            stats.warmBoats = countResult[0]["boats"].isNull() ? 0 : countResult[0]["boats"].as<int>();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "getGuardianStats failed: " << e.base().what();
    }

    return stats;
}

bool GuardianIncentiveService::checkAndGrantGuardian(const std::string& userId) {
    auto stats = getGuardianStats(userId);

    if (!stats.isGuardian && stats.totalResonancePoints >= GUARDIAN_THRESHOLD) {
        auto db = drogon::app().getDbClient("default");
        try {
            db->execSqlSync(
                "UPDATE users SET is_guardian = true, guardian_since = NOW() WHERE user_id = $1",
                userId
            );

            auto& pushService = services::NotificationPushService::getInstance();
            pushService.pushSystemNotice(
                userId,
                "守护者成就达成",
                "感谢你在心湖中传递的温暖。你已成为守护者，解锁了「转赠灯火」功能，可以将你的灯火传递给需要的人。"
            );

            LOG_INFO << "User " << userId << " granted Guardian status";
            return true;
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "checkAndGrantGuardian failed: " << e.base().what();
        }
    }

    return false;
}

bool GuardianIncentiveService::transferLamp(const std::string& fromUserId, const std::string& toUserId) {
    auto stats = getGuardianStats(fromUserId);
    if (!stats.canTransferLamp) {
        return false;
    }

    auto db = drogon::app().getDbClient("default");
    try {
        // 灯火转赠涉及多表写入，必须在事务中执行
        auto trans = db->newTransaction();

        trans->execSqlSync(
            "INSERT INTO lamp_transfers (from_user_id, to_user_id, created_at) VALUES ($1, $2, NOW())",
            fromUserId, toUserId
        );

        // 扣除积分
        trans->execSqlSync(
            "UPDATE users SET resonance_total = resonance_total - $1 WHERE user_id = $2",
            GUARDIAN_THRESHOLD, fromUserId
        );

        // 给接收者发放灯——内联 VIP 升级 SQL，避免 upgradeVIP() 走独立连接绕过事务
        trans->execSqlSync(
            "UPDATE users SET vip_level = GREATEST(vip_level, 1), "
            "vip_expires_at = COALESCE(vip_expires_at, NOW()) + INTERVAL '7 days', "
            "updated_at = CURRENT_TIMESTAMP WHERE user_id = $1",
            toUserId
        );
        trans->execSqlSync(
            "INSERT INTO vip_upgrade_logs (user_id, old_vip_level, new_vip_level, "
            "upgrade_type, reason, expires_at) "
            "VALUES ($1, 0, 1, 'lamp_transfer', $2, NOW() + INTERVAL '7 days')",
            toUserId, "lamp_transfer_from:" + fromUserId
        );

        auto& pushService = heartlake::services::NotificationPushService::getInstance();
        pushService.pushSystemNotice(
            toUserId,
            "收到一盏灯火",
            "一位守护者将灯火传递给了你。这盏灯为你点亮7天，你可以免费预约心理咨询。"
        );

        LOG_INFO << "Lamp transferred from " << fromUserId << " to " << toUserId;
        return true;
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "transferLamp failed: " << e.base().what();
        return false;
    }
}

} // namespace heartlake::infrastructure
