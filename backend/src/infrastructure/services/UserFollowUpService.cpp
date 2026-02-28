/**
 * 用户回访与持续关怀系统实现
 *
 * 后台定时服务，对曾被识别为负重者的用户进行阶梯式回访：
 * - Day 1：首次关怀，询问近况
 * - Day 3：持续跟进，提供倾诉渠道
 * - Day 7：长期关注，提醒免费心理咨询仍然保留
 *
 * 同时检测 7 天未活跃的普通用户，发送温和的召回通知。
 * 每个回访阶段通过 user_followups 表去重，确保不重复打扰。
 */

#include "infrastructure/services/UserFollowUpService.h"
#include "infrastructure/services/WarmQuoteService.h"
#include "infrastructure/services/NotificationPushService.h"
#include "utils/EnvUtils.h"
#include <drogon/drogon.h>
#include <chrono>

namespace heartlake::infrastructure {

UserFollowUpService& UserFollowUpService::getInstance() {
    static UserFollowUpService instance;
    return instance;
}

UserFollowUpService::~UserFollowUpService() {
    stop();
}

void UserFollowUpService::start() {
    if (running_) return;
    running_ = true;
    scanThread_ = std::make_unique<std::thread>(&UserFollowUpService::scanLoop, this);
    LOG_INFO << "UserFollowUpService started";
}

void UserFollowUpService::stop() {
    if (!running_) return;
    running_ = false;
    if (scanThread_ && scanThread_->joinable()) {
        scanThread_->join();
    }
    LOG_INFO << "UserFollowUpService stopped";
}

void UserFollowUpService::scanLoop() {
    const int startupDelaySec = heartlake::utils::parsePositiveIntEnv("USER_FOLLOWUP_STARTUP_DELAY_SEC", 120);
    for (int i = 0; i < startupDelaySec && running_; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    while (running_) {
        scanOnce();
        for (int i = 0; i < SCAN_INTERVAL_HOURS * 3600 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void UserFollowUpService::scanOnce() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto db = drogon::app().getDbClient();

    try {
        // 查找需要回访的用户（曾被识别为负重者）
        auto result = db->execSqlSync(
            "SELECT user_id, EXTRACT(DAY FROM NOW() - MAX(created_at))::int as days "
            "FROM vip_upgrade_logs "
            "WHERE (reason LIKE 'extreme_burden%' OR reason LIKE 'lamp%') "
            "AND created_at > NOW() - INTERVAL '14 days' "
            "GROUP BY user_id"
        );

        for (const auto& row : result) {
            std::string userId = row["user_id"].as<std::string>();
            int days = row["days"].as<int>();
            followUpBurdenedUser(userId, days);
        }

        checkInactiveUsers();
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "UserFollowUpService scan failed: " << e.base().what();
    }
}

void UserFollowUpService::followUpBurdenedUser(const std::string& userId, int daysSinceIntervention) {
    auto db = drogon::app().getDbClient();

    try {
        // 检查是否已发送过该阶段的回访
        auto checkResult = db->execSqlSync(
            "SELECT COUNT(*) as cnt FROM user_followups "
            "WHERE user_id = $1 AND followup_day = $2",
            userId, daysSinceIntervention
        );

        if (!checkResult.empty() && checkResult[0]["cnt"].as<int>() > 0) {
            return;
        }

        std::string messageType;
        if (daysSinceIntervention == FOLLOWUP_DAY_1) {
            messageType = "day1";
        } else if (daysSinceIntervention == FOLLOWUP_DAY_3) {
            messageType = "day3";
        } else if (daysSinceIntervention == FOLLOWUP_DAY_7) {
            messageType = "day7";
        } else {
            return;
        }

        sendFollowUpMessage(userId, messageType);

        db->execSqlSync(
            "INSERT INTO user_followups (user_id, followup_day, created_at) VALUES ($1, $2, NOW())",
            userId, daysSinceIntervention
        );
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "followUpBurdenedUser failed: " << e.base().what();
    }
}

void UserFollowUpService::checkInactiveUsers() {
    auto db = drogon::app().getDbClient();

    try {
        auto result = db->execSqlSync(
            "SELECT user_id FROM users "
            "WHERE last_active_at < NOW() - INTERVAL '7 days' "
            "AND last_active_at > NOW() - INTERVAL '8 days' "
            "AND is_guardian = false"
        );

        for (const auto& row : result) {
            std::string userId = row["user_id"].as<std::string>();
            sendFollowUpMessage(userId, "inactive");
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "checkInactiveUsers failed: " << e.base().what();
    }
}

void UserFollowUpService::sendFollowUpMessage(const std::string& userId, const std::string& messageType) {
    auto& pushService = heartlake::services::NotificationPushService::getInstance();
    auto& warmService = WarmQuoteService::getInstance();

    WarmScene scene = WarmScene::FollowUpDay1;
    if (messageType == "day3") scene = WarmScene::FollowUpDay3;
    else if (messageType == "day7") scene = WarmScene::FollowUpDay7;
    else if (messageType == "inactive") scene = WarmScene::Inactive;

    auto msg = warmService.getQuoteForScene(scene);
    pushService.pushSystemNotice(userId, msg.title, msg.content);
    LOG_INFO << "Follow-up message sent to user " << userId << " type: " << messageType;
}

} // namespace heartlake::infrastructure
