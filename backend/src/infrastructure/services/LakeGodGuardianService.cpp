/**
 * 湖神守护服务实现
 *
 * 后台守护线程，定期扫描"零互动石头"（无涟漪、无纸船、超过阈值时间），
 * 由 AI 生成暖心评论并以"湖神"身份自动发送纸船，确保每颗石头都能收到回应。
 *
 * 防重复机制：
 * - 先插入 status='pending' 的占位纸船记录，利用唯一约束防止并发重复处理
 * - AI 生成成功后更新为 active，失败则删除占位记录
 * - 定期清理超过 10 分钟仍为 pending 的僵尸记录（AI 超时场景）
 *
 * 发送纸船后自动创建通知并通过 WebSocket 实时推送给石头主人。
 */

#include "infrastructure/services/LakeGodGuardianService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/services/NotificationPushService.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/AdminRealtimeNotifier.h"
#include "utils/IdGenerator.h"
#include "utils/PsychologicalRiskAssessment.h"
#include "utils/RealtimeEvent.h"
#include "utils/EnvUtils.h"
#include <drogon/drogon.h>
#include <chrono>

namespace heartlake::infrastructure {

namespace {
constexpr const char *kLakeGodSenderId = "ai_lakegod";
}

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
    const int scanIntervalMinutes = heartlake::utils::parsePositiveIntEnv(
        "LAKE_GOD_SCAN_INTERVAL_MINUTES", SCAN_INTERVAL_MINUTES);
    for (int i = 0; i < startupDelaySec && running_; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    while (running_) {
        cleanStalePendingBoats();
        processZeroInteractionStones();
        for (int i = 0; i < scanIntervalMinutes * 60 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void LakeGodGuardianService::cleanStalePendingBoats() {
    auto db = drogon::app().getDbClient("default");
    try {
        auto result = db->execSqlSync(
            "DELETE FROM paper_boats WHERE status = 'pending' "
            "AND sender_id = $1 AND created_at < NOW() - INTERVAL '10 minutes' "
            "RETURNING boat_id",
            std::string(kLakeGodSenderId)
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
    const int zeroInteractionThresholdHours = heartlake::utils::parsePositiveIntEnv(
        "LAKE_GOD_ZERO_INTERACTION_THRESHOLD_HOURS", ZERO_INTERACTION_THRESHOLD_HOURS);

    try {
        // 查找零互动石头：无涟漪、无纸船、超过阈值时间
        auto result = db->execSqlSync(
            "SELECT stone_id, content, mood_type FROM stones "
            "WHERE status = 'published' "
            "AND ripple_count = 0 AND boat_count = 0 "
            "AND deleted_at IS NULL "
            "AND created_at < NOW() - make_interval(hours => CAST($1 AS INT)) "
            "AND NOT EXISTS (SELECT 1 FROM paper_boats WHERE stone_id = stones.stone_id AND sender_id = $3) "
            "LIMIT CAST($2 AS INT)",
            zeroInteractionThresholdHours, batchSize,
            std::string(kLakeGodSenderId)
        );

        for (const auto& row : result) {
            std::string stoneId = row["stone_id"].as<std::string>();
            std::string content = row["content"].as<std::string>();
            std::string mood = row["mood_type"].isNull() ? "neutral" : row["mood_type"].as<std::string>();

            // 先插入占位记录防止重复处理
            try {
                std::string boatId = utils::IdGenerator::generateBoatId();
                db->execSqlSync(
                    "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, is_anonymous, is_ai_reply, status, created_at) "
                    "VALUES ($1, $2, $3, '', false, true, 'pending', NOW())",
                    boatId, stoneId, std::string(kLakeGodSenderId)
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
        if (comment.empty()) {
            LOG_ERROR << "LakeGod AI generation returned empty comment";
            db->execSqlSync("DELETE FROM paper_boats WHERE boat_id = $1", boatId);
            return;
        }

        try {
            db->execSqlSync(
                "UPDATE paper_boats SET content = $1, status = 'active' WHERE boat_id = $2",
                comment, boatId
            );

            auto updateResult = db->execSqlSync(
                "UPDATE stones SET boat_count = boat_count + 1, updated_at = NOW() "
                "WHERE stone_id = $1 RETURNING boat_count",
                stoneId
            );
            const int boatCount = updateResult.empty()
                ? 1
                : updateResult[0]["boat_count"].as<int>();

            // 通知石头主人
            db->execSqlAsync(
                "SELECT user_id FROM stones WHERE stone_id = $1",
                [stoneId, boatId, comment, boatCount](const drogon::orm::Result& r) {
                    if (r.empty()) return;
                    std::string ownerId = r[0]["user_id"].as<std::string>();
                    services::NotificationMessage notification;
                    notification.userId = ownerId;
                    notification.type = services::NotificationType::AI_REPLY;
                    notification.title = "湖神来信";
                    notification.content = "湖神给你的石头送来了一封暖心纸船";
                    notification.relatedId = boatId;
                    notification.relatedType = "boat";
                    notification.extraData["stone_id"] = stoneId;
                    notification.extraData["boat_id"] = boatId;
                    notification.extraData["is_ai"] = true;
                    notification.extraData["triggered_by"] = "lake_god";
                    services::NotificationPushService::getInstance().push(
                        std::move(notification));

                    Json::Value broadcastMsg;
                    broadcastMsg["stone_id"] = stoneId;
                    broadcastMsg["boat_id"] = boatId;
                    broadcastMsg["boat_count"] = boatCount;
                    broadcastMsg["boat_content"] = comment;
                    broadcastMsg["is_ai"] = true;
                    broadcastMsg["triggered_by"] = "lake_god";
                    const auto event =
                        heartlake::utils::buildRealtimeEvent("boat_update",
                                                             std::move(broadcastMsg));
                    controllers::BroadcastWebSocketController::sendToRoom(
                        "stone:" + stoneId, event);
                    controllers::BroadcastWebSocketController::broadcast(event);
                    heartlake::utils::broadcastAdminRealtimeStatsUpdate("boat_update");
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
