/**
 * 湖神守护服务实现
 */

#include "infrastructure/services/LakeGodGuardianService.h"
#include "infrastructure/ai/AIService.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/IdGenerator.h"
#include "utils/PsychologicalRiskAssessment.h"
#include "utils/EnvUtils.h"
#include <drogon/drogon.h>
#include <chrono>

namespace heartlake::infrastructure {

LakeGodGuardianService& LakeGodGuardianService::getInstance() {
    static LakeGodGuardianService instance;
    return instance;
}

LakeGodGuardianService::~LakeGodGuardianService() {
    stop();
}

void LakeGodGuardianService::start() {
    if (running_) return;
    running_ = true;
    scanThread_ = std::make_unique<std::thread>(&LakeGodGuardianService::scanLoop, this);
    LOG_INFO << "LakeGod Guardian Service started";
}

void LakeGodGuardianService::stop() {
    if (!running_) return;
    running_ = false;
    if (scanThread_ && scanThread_->joinable()) {
        scanThread_->join();
    }
    LOG_INFO << "LakeGod Guardian Service stopped";
}

void LakeGodGuardianService::scanLoop() {
    const int startupDelaySec = heartlake::utils::parsePositiveIntEnv("LAKE_GOD_STARTUP_DELAY_SEC", 60);
    for (int i = 0; i < startupDelaySec && running_; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    while (running_) {
        cleanStalePendingBoats();
        processZeroInteractionStones();
        for (int i = 0; i < SCAN_INTERVAL_MINUTES * 60 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void LakeGodGuardianService::cleanStalePendingBoats() {
    auto db = drogon::app().getDbClient("default");
    try {
        auto result = db->execSqlSync(
            "DELETE FROM paper_boats WHERE status = 'pending' "
            "AND sender_id = 'lake_god' AND created_at < NOW() - INTERVAL '10 minutes' "
            "RETURNING boat_id"
        );
        if (!result.empty()) {
            LOG_WARN << "Cleaned " << result.size() << " stale pending boats";
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to clean stale pending boats: " << e.base().what();
    }
}

void LakeGodGuardianService::scanOnce() {
    processZeroInteractionStones();
}

void LakeGodGuardianService::processZeroInteractionStones() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto db = drogon::app().getDbClient("default");
    const int batchSize = heartlake::utils::parsePositiveIntEnv("LAKE_GOD_SCAN_BATCH_SIZE", 3);

    try {
        // 查找零互动石头：无涟漪、无纸船、超过阈值时间
        auto result = db->execSqlSync(
            "SELECT stone_id, content, mood_type FROM stones "
            "WHERE status = 'published' "
            "AND ripple_count = 0 AND boat_count = 0 "
            "AND deleted_at IS NULL "
            "AND created_at < NOW() - make_interval(hours => CAST($1 AS INT)) "
            "AND NOT EXISTS (SELECT 1 FROM paper_boats WHERE stone_id = stones.stone_id AND sender_id = 'lake_god') "
            "LIMIT CAST($2 AS INT)",
            ZERO_INTERACTION_THRESHOLD_HOURS, batchSize
        );

        for (const auto& row : result) {
            std::string stoneId = row["stone_id"].as<std::string>();
            std::string content = row["content"].as<std::string>();
            std::string mood = row["mood_type"].isNull() ? "neutral" : row["mood_type"].as<std::string>();

            // 先插入占位记录防止重复处理
            try {
                std::string boatId = utils::IdGenerator::generateBoatId();
                db->execSqlSync(
                    "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, is_anonymous, status, created_at) "
                    "VALUES ($1, $2, 'lake_god', '', true, 'pending', NOW())",
                    boatId, stoneId
                );
                sendAutoBoat(stoneId, content, mood, boatId);
            } catch (const drogon::orm::DrogonDbException& e) {
                LOG_WARN << "Stone " << stoneId << " already being processed";
            }
        }

        if (!result.empty()) {
            LOG_INFO << "LakeGod processed " << result.size() << " zero-interaction stones";
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "LakeGod scan failed: " << e.base().what();
    }
}

void LakeGodGuardianService::sendAutoBoat(const std::string& stoneId, const std::string& stoneContent, const std::string& mood, const std::string& boatId) {
    auto& aiService = ai::AIService::getInstance();

    aiService.generateStoneComment(stoneContent, mood, [stoneId, boatId](const std::string& comment, const std::string& error) {
        auto db = drogon::app().getDbClient("default");
        if (!error.empty()) {
            LOG_ERROR << "LakeGod AI generation failed: " << error;
            db->execSqlSync("DELETE FROM paper_boats WHERE boat_id = $1", boatId);
            return;
        }

        try {
            db->execSqlSync(
                "UPDATE paper_boats SET content = $1, status = 'active' WHERE boat_id = $2",
                comment, boatId
            );

            db->execSqlSync(
                "UPDATE stones SET boat_count = boat_count + 1, updated_at = NOW() WHERE stone_id = $1",
                stoneId
            );

            // 通知石头主人
            db->execSqlAsync(
                "SELECT user_id FROM stones WHERE stone_id = $1",
                [boatId](const drogon::orm::Result& r) {
                    if (r.empty()) return;
                    std::string ownerId = r[0]["user_id"].as<std::string>();
                    std::string notifId = utils::IdGenerator::generateNotificationId();
                    auto db2 = drogon::app().getDbClient("default");
                    db2->execSqlAsync(
                        "INSERT INTO notifications (notification_id, user_id, type, content, related_id, related_type, is_read, created_at) "
                        "VALUES ($1, $2, 'lake_god', '湖神给你的石头送来了一封暖心纸船', $3, 'boat', false, NOW())",
                        [ownerId](const drogon::orm::Result&) {
                            Json::Value notifMsg;
                            notifMsg["type"] = "new_notification";
                            notifMsg["notification_type"] = "lake_god";
                            notifMsg["content"] = "湖神给你的石头送来了一封暖心纸船";
                            controllers::BroadcastWebSocketController::sendToUser(ownerId, notifMsg);
                        },
                        [](const drogon::orm::DrogonDbException& e) {
                            LOG_ERROR << "Failed to create lake_god notification: " << e.base().what();
                        },
                        notifId, ownerId, boatId
                    );
                },
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "Failed to get stone owner: " << e.base().what();
                },
                stoneId
            );

            LOG_INFO << "LakeGod sent auto-boat to stone: " << stoneId;
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "LakeGod failed to send boat: " << e.base().what();
        }
    });
}

} // namespace heartlake::infrastructure
