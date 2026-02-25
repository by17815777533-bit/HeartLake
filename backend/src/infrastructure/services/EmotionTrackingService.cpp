/**
 * @file EmotionTrackingService.cpp
 * @brief 负重者监测系统实现
 */

#include "infrastructure/services/EmotionTrackingService.h"
#include "infrastructure/services/VIPService.h"
#include "infrastructure/services/WarmQuoteService.h"
#include "infrastructure/services/NotificationPushService.h"
#include "utils/IdentityShadowMap.h"
#include "utils/EnvUtils.h"
#include <drogon/drogon.h>
#include <chrono>

namespace heartlake::infrastructure {

EmotionTrackingService& EmotionTrackingService::getInstance() {
    static EmotionTrackingService instance;
    return instance;
}

EmotionTrackingService::~EmotionTrackingService() {
    stop();
}

void EmotionTrackingService::start() {
    if (running_) return;
    running_ = true;
    scanThread_ = std::make_unique<std::thread>(&EmotionTrackingService::scanLoop, this);
    LOG_INFO << "EmotionTrackingService started";
}

void EmotionTrackingService::stop() {
    if (!running_) return;
    running_ = false;
    cv_.notify_all();
    if (scanThread_ && scanThread_->joinable()) {
        scanThread_->join();
    }
    LOG_INFO << "EmotionTrackingService stopped";
}

void EmotionTrackingService::scanLoop() {
    const int startupDelaySec = heartlake::utils::parsePositiveIntEnv("EMOTION_TRACKING_STARTUP_DELAY_SEC", 90);
    for (int i = 0; i < startupDelaySec && running_; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    while (running_) {
        scanOnce();
        std::unique_lock<std::mutex> lock(cvMutex_);
        cv_.wait_for(lock, std::chrono::minutes(SCAN_INTERVAL_MINUTES), [this]() {
            return !running_.load();
        });
    }
}

void EmotionTrackingService::scanOnce() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto db = drogon::app().getDbClient("default");

    // 30天数据自动清理
    try {
        db->execSqlAsync("DELETE FROM emotion_tracking WHERE created_at < NOW() - INTERVAL '30 days'",
            [](const drogon::orm::Result&) {}, [](const drogon::orm::DrogonDbException&) {});
    } catch (const std::exception& e) {
        LOG_WARN << "Emotion tracking 30-day cleanup failed: " << e.what();
    }

    try {
        // 使用 make_interval 参数化查询，避免 SQL 字符串拼接
        auto result = db->execSqlSync(
            "SELECT DISTINCT user_id FROM stones "
            "WHERE created_at > NOW() - make_interval(hours => $1) "
            "AND status = 'published' "
            "AND deleted_at IS NULL",
            TRACKING_HOURS
        );

        for (const auto& row : result) {
            std::string userId = row["user_id"].as<std::string>();
            processUser(userId);
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "EmotionTrackingService scan failed: " << e.base().what();
    }
}

void EmotionTrackingService::processUser(const std::string& userId) {
    auto result = checkUserBurden(userId);
    if (result.isExtremelyBurdened) {
        triggerIntervention(result);
    }
}

void EmotionTrackingService::recordEmotion(const std::string& userId, float score, const std::string& content) {
    auto db = drogon::app().getDbClient("default");
    try {
        db->execSqlAsync(
            "INSERT INTO emotion_tracking (user_id, score, content_hash, created_at) "
            "VALUES ($1, $2, md5($3), NOW())",
            [](const drogon::orm::Result&) {},
            [](const drogon::orm::DrogonDbException&) {},
            userId, score, content
        );
    } catch (const std::exception& e) {
        LOG_WARN << "Emotion tracking record failed: " << e.what();
    }
}

EmotionTrackingResult EmotionTrackingService::checkUserBurden(const std::string& userId) {
    EmotionTrackingResult result;
    result.userId = userId;
    result.isExtremelyBurdened = false;

    auto db = drogon::app().getDbClient("default");
    try {
        // 使用 make_interval 参数化查询，避免 SQL 字符串拼接
        auto queryResult = db->execSqlSync(
            "SELECT "
            "COUNT(*) FILTER (WHERE base_score < 0) as post_count, "
            "AVG(CASE WHEN base_score < 0 THEN base_score END) as avg_score "
            "FROM ("
            "  SELECT COALESCE(emotion_score, sentiment_score, "
            "    CASE COALESCE(mood_type, 'neutral') "
            "      WHEN 'happy' THEN 0.70 "
            "      WHEN 'calm' THEN 0.35 "
            "      WHEN 'grateful' THEN 0.60 "
            "      WHEN 'hopeful' THEN 0.45 "
            "      WHEN 'neutral' THEN 0.0 "
            "      WHEN 'confused' THEN -0.10 "
            "      WHEN 'anxious' THEN -0.45 "
            "      WHEN 'sad' THEN -0.65 "
            "      WHEN 'angry' THEN -0.55 "
            "      WHEN 'lonely' THEN -0.50 "
            "      ELSE 0.0 END"
            "  ) as base_score "
            "  FROM stones WHERE user_id = $1 "
            "  AND created_at > NOW() - make_interval(hours => $2) "
            "  AND status = 'published' AND deleted_at IS NULL"
            ") score_view",
            userId, TRACKING_HOURS
        );

        if (!queryResult.empty()) {
            result.postCount72h = queryResult[0]["post_count"].as<int>();
            result.avgNegativeScore = queryResult[0]["avg_score"].isNull() ? 0.0f : queryResult[0]["avg_score"].as<float>();

            if (result.postCount72h >= MIN_POST_COUNT && result.avgNegativeScore <= EXTREME_BURDEN_THRESHOLD) {
                result.isExtremelyBurdened = true;
                result.triggerReason = "72小时内发布" + std::to_string(result.postCount72h) +
                    "条负面内容，平均情绪分数: " + std::to_string(result.avgNegativeScore);
            }
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "checkUserBurden failed: " << e.base().what();
    }

    return result;
}

void EmotionTrackingService::triggerIntervention(const EmotionTrackingResult& result) {
    auto db = drogon::app().getDbClient("default");

    // 检查72小时内是否已干预过
    try {
        auto check = db->execSqlSync(
            "SELECT id FROM intervention_log WHERE user_id = $1 "
            "AND created_at > NOW() - INTERVAL '72 hours'",
            result.userId
        );
        if (!check.empty()) {
            return; // 已干预，跳过
        }
    } catch (const std::exception& e) {
        LOG_WARN << "Intervention check query failed: " << e.what();
    }

    LOG_WARN << "Extreme burden detected for user: " << result.userId << " - " << result.triggerReason;

    // 记录干预
    try {
        db->execSqlSync(
            "INSERT INTO intervention_log (user_id, reason, created_at) VALUES ($1, $2, NOW())",
            result.userId, result.triggerReason
        );
    } catch (const std::exception& e) {
        LOG_WARN << "Intervention log insert failed: " << e.what();
    }

    // 自动发放灯（免费心理咨询权限）
    services::VIPService::upgradeVIP(result.userId, 1, 7, "extreme_burden_lamp");

    // 获取个性化暖心语录
    auto warmMsg = WarmQuoteService::getInstance().getQuoteForBurden(result.avgNegativeScore);

    // 推送暖心通知
    auto& pushService = services::NotificationPushService::getInstance();
    pushService.pushSystemNotice(
        result.userId,
        warmMsg.title,
        warmMsg.content + " " + warmMsg.vipGuide
    );

    std::function<void(const EmotionTrackingResult&)> callback;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        callback = triggerCallback_;
    }
    if (callback) {
        callback(result);
    }
}

void EmotionTrackingService::setTriggerCallback(std::function<void(const EmotionTrackingResult&)> callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    triggerCallback_ = std::move(callback);
}

} // namespace heartlake::infrastructure
