/**
 * @file InteractionApplicationService.cpp
 * @brief 互动应用服务 —— 涟漪、纸船、通知、临时连接的完整业务流程
 *
 * 核心交互模型：
 *   -
 * 涟漪（Ripple）：类似"点赞"，每个用户对同一石头只能涟漪一次（唯一约束幂等）
 *   - 纸船（Boat）：类似"评论"，支持匿名发送，附带情感暖意评分
 *   - 通知（Notification）：系统推送，支持单条/全部已读
 *   - 临时连接（Connection）：24h 有效的匿名聊天通道
 *
 * 事务策略：涟漪/纸船的创建和计数器更新在同一事务内完成，保证一致性。
 * 缓存策略：每次写操作后主动失效对应石头缓存，确保前端读到最新计数。
 */

#include "application/InteractionApplicationService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/services/GuardianIncentiveService.h"
#include "infrastructure/services/NotificationPushService.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/IdGenerator.h"
#include "utils/PsychologicalRiskAssessment.h"
#include "utils/RealtimeEvent.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/StoneCacheKeys.h"
#include <algorithm>
#include <drogon/drogon.h>
#include <set>

using namespace heartlake::utils;

namespace {

constexpr int kStoneRippleStateCacheTtlSeconds = 60;
constexpr const char *kLakeGodSenderId = "ai_lakegod";
constexpr const char *kLegacyLakeGodSenderId = "lake_god";

std::string safeStringColumn(const drogon::orm::Row &row, const char *column,
                             const std::string &fallback = "") {
  return row[column].isNull() ? fallback : row[column].as<std::string>();
}

bool safeBoolColumn(const drogon::orm::Row &row, const char *column,
                    bool fallback = false) {
  return row[column].isNull() ? fallback : row[column].as<bool>();
}

int extractTotalCount(const drogon::orm::Result &result) {
  if (result.empty() || result[0]["total_count"].isNull()) {
    return 0;
  }
  return result[0]["total_count"].as<int>();
}

Json::Value buildUserPayload(const std::string &userId,
                             const std::string &username,
                             const std::string &nickname,
                             const std::string &avatarUrl) {
  Json::Value user(Json::objectValue);
  if (!userId.empty()) {
    user["user_id"] = userId;
    user["id"] = userId;
    user["userId"] = userId;
  }
  if (!username.empty()) {
    user["username"] = username;
  }
  if (!nickname.empty()) {
    user["nickname"] = nickname;
  }
  if (!avatarUrl.empty()) {
    user["avatar_url"] = avatarUrl;
    user["avatarUrl"] = avatarUrl;
  }
  return user;
}

Json::Value buildAnonymousActor(const std::string &userId,
                                const std::string &username,
                                const std::string &nickname,
                                const std::string &avatarUrl,
                                bool isAnonymous) {
  auto actor = buildUserPayload(userId, username, nickname, avatarUrl);
  actor["is_anonymous"] = isAnonymous;
  actor["isAnonymous"] = isAnonymous;
  if (isAnonymous) {
    actor["nickname"] = "匿名旅人";
    actor.removeMember("avatar_url");
    actor.removeMember("avatarUrl");
  }
  return actor;
}

bool isLakeGodActor(const std::string &userId, bool isAiReply) {
  return isAiReply || userId == kLakeGodSenderId ||
         userId == kLegacyLakeGodSenderId;
}

Json::Value buildBoatAuthorPayload(const std::string &userId,
                                   const std::string &username,
                                   const std::string &nickname,
                                   const std::string &avatarUrl,
                                   bool isAnonymous, bool isAiReply) {
  if (isLakeGodActor(userId, isAiReply)) {
    auto actor = buildUserPayload(userId, username, "湖神", avatarUrl);
    actor["is_anonymous"] = false;
    actor["isAnonymous"] = false;
    actor["is_ai_reply"] = true;
    actor["isAiReply"] = true;
    actor["agent"] = "lake_god";
    actor["agent_name"] = "湖神";
    return actor;
  }
  return buildAnonymousActor(userId, username, nickname, avatarUrl, isAnonymous);
}

struct ConnectionTargetPolicy {
  bool targetExists = false;
  bool isFriend = false;
  bool allowStrangerMessage = false;
  bool isBlocked = false;
};

ConnectionTargetPolicy loadConnectionTargetPolicy(
    const drogon::orm::DbClientPtr &dbClient, const std::string &userId,
    const std::string &targetUserId) {
  auto result = dbClient->execSqlSync(
      "SELECT "
      "  EXISTS(SELECT 1 FROM users WHERE user_id = $2 AND status = 'active') "
      "    AS target_exists, "
      "  EXISTS(SELECT 1 FROM friends "
      "         WHERE ((user_id = $1 AND friend_id = $2) "
      "             OR (user_id = $2 AND friend_id = $1)) "
      "           AND status = 'accepted') AS is_friend, "
      "  EXISTS(SELECT 1 FROM user_blocks "
      "         WHERE (user_id = $1 AND blocked_user_id = $2) "
      "            OR (user_id = $2 AND blocked_user_id = $1)) "
      "    AS is_blocked, "
      "  COALESCE((SELECT allow_message_from_stranger "
      "            FROM user_privacy_settings "
      "            WHERE user_id = $2), false) AS allow_message_from_stranger",
      userId, targetUserId);

  if (result.empty()) {
    return {};
  }

  const auto row = *safeRow(result);
  ConnectionTargetPolicy policy;
  policy.targetExists = safeBoolColumn(row, "target_exists");
  policy.isFriend = safeBoolColumn(row, "is_friend");
  policy.isBlocked = safeBoolColumn(row, "is_blocked");
  policy.allowStrangerMessage =
      safeBoolColumn(row, "allow_message_from_stranger");
  return policy;
}

void ensureTargetUserAvailable(const drogon::orm::DbClientPtr &dbClient,
                               const std::string &userId,
                               const std::string &targetUserId) {
  const auto policy = loadConnectionTargetPolicy(dbClient, userId, targetUserId);
  if (!policy.targetExists) {
    throw std::runtime_error("目标用户不存在或不可用");
  }
  if (policy.isBlocked) {
    throw std::runtime_error("你与对方存在拉黑关系，无法建立连接");
  }
}

void ensureDirectConnectionTargetAllowed(
    const drogon::orm::DbClientPtr &dbClient, const std::string &userId,
    const std::string &targetUserId) {
  const auto policy = loadConnectionTargetPolicy(dbClient, userId, targetUserId);
  if (!policy.targetExists) {
    throw std::runtime_error("目标用户不存在或不可用");
  }
  if (policy.isBlocked) {
    throw std::runtime_error("你与对方存在拉黑关系，无法建立连接");
  }
  if (!policy.isFriend && !policy.allowStrangerMessage) {
    throw std::runtime_error("对方未开启陌生人消息");
  }
}

Json::Value buildConnectionPayload(const drogon::orm::Row &row) {
  Json::Value result(Json::objectValue);
  const auto connectionId = safeStringColumn(row, "connection_id");
  result["connection_id"] = connectionId;
  result["connectionId"] = connectionId;
  result["id"] = connectionId;
  result["user_id"] = safeStringColumn(row, "user_id");
  result["userId"] = result["user_id"];
  result["target_user_id"] = safeStringColumn(row, "target_user_id");
  result["targetUserId"] = result["target_user_id"];
  if (!row["stone_id"].isNull()) {
    result["stone_id"] = row["stone_id"].as<std::string>();
    result["stoneId"] = result["stone_id"];
  }
  result["status"] = safeStringColumn(row, "status", "active");
  result["created_at"] = safeStringColumn(row, "created_at");
  result["createdAt"] = result["created_at"];
  result["expires_at"] = safeStringColumn(row, "expires_at");
  result["expiresAt"] = result["expires_at"];
  return result;
}

std::string normalizeBoatStatusFilter(const std::string &rawStatus) {
  if (rawStatus == "published") {
    return "active";
  }
  return rawStatus;
}

std::string serializeRiskFactors(
    const std::vector<heartlake::utils::RiskFactor> &factors) {
  Json::Value payload(Json::arrayValue);
  for (const auto &factor : factors) {
    Json::Value item(Json::objectValue);
    item["category"] = factor.category;
    item["name"] = factor.name;
    item["score"] = factor.score;
    item["weight"] = factor.weight;
    item["description"] = factor.description;
    payload.append(item);
  }

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  return Json::writeString(writer, payload);
}

std::string createInteractionNotificationAsync(
    const std::string &recipientId, const std::string &actorUserId,
    const std::string &type, const std::string &content,
    const std::string &relatedId, const std::string &relatedType) {
  if (recipientId.empty() || recipientId == actorUserId) {
    return "";
  }

  auto dbClient = drogon::app().getDbClient("default");
  const auto notificationId =
      heartlake::utils::IdGenerator::generateNotificationId();
  dbClient->execSqlAsync(
      "INSERT INTO notifications (notification_id, user_id, type, content, "
      "related_id, related_type, is_read, created_at) "
      "VALUES ($1, $2, $3, $4, $5, $6, false, NOW())",
      [](const drogon::orm::Result &) {},
      [recipientId, type](const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Failed to create " << type << " notification for user "
                  << recipientId << ": " << e.base().what();
      },
      notificationId, recipientId, type, content, relatedId, relatedType);
  return notificationId;
}

void emitTempFriendLifecycleEvent(
    const std::string &eventType, const std::string &tempFriendId,
    const std::string &user1Id, const std::string &user2Id,
    const Json::Value &extra = Json::Value(Json::objectValue)) {
  if (tempFriendId.empty() || user1Id.empty() || user2Id.empty()) {
    return;
  }

  Json::Value payload(Json::objectValue);
  payload["temp_friend_id"] = tempFriendId;
  payload["tempFriendId"] = tempFriendId;
  payload["user1_id"] = user1Id;
  payload["user2_id"] = user2Id;
  if (extra.isObject()) {
    for (const auto &member : extra.getMemberNames()) {
      payload[member] = extra[member];
    }
  }

  heartlake::controllers::BroadcastWebSocketController::sendToUser(
      user1Id, heartlake::utils::buildRealtimeEvent(eventType, payload));
  if (user2Id != user1Id) {
    heartlake::controllers::BroadcastWebSocketController::sendToUser(
        user2Id, heartlake::utils::buildRealtimeEvent(eventType, payload));
  }
}

void refreshBoatTempFriendshipAsync(const std::string &userId,
                                    const std::string &stoneOwnerId,
                                    const std::string &boatId) {
  if (stoneOwnerId.empty() || stoneOwnerId == userId) {
    return;
  }

  const auto tempFriendId = drogon::utils::getUuid();
  const auto expiresAt = trantor::Date::date().after(24 * 3600);
  const std::string uid1 = std::min(userId, stoneOwnerId);
  const std::string uid2 = std::max(userId, stoneOwnerId);

  auto dbClient = drogon::app().getDbClient("default");
  dbClient->execSqlAsync(
      "INSERT INTO temp_friends (temp_friend_id, user1_id, user2_id, "
      "requester_id, source, source_id, status, expires_at) "
      "VALUES ($1, $2, $3, $4, 'boat', $5, 'active', $6) "
      "ON CONFLICT ON CONSTRAINT unique_temp_friendship DO UPDATE SET "
      "requester_id = EXCLUDED.requester_id, "
      "source = 'boat', "
      "source_id = EXCLUDED.source_id, "
      "expires_at = GREATEST(temp_friends.expires_at, EXCLUDED.expires_at), "
      "status = 'active' "
      "RETURNING temp_friend_id, expires_at",
      [uid1, uid2, userId](const drogon::orm::Result &result) {
        if (result.empty()) {
          return;
        }
        Json::Value extra(Json::objectValue);
        extra["source"] = "boat";
        extra["expires_at"] = result[0]["expires_at"].as<std::string>();
        extra["triggered_by"] = userId;
        emitTempFriendLifecycleEvent(
            "temp_friend_created",
            result[0]["temp_friend_id"].as<std::string>(),
            uid1,
            uid2,
            extra);
      },
      [boatId](const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Failed to refresh boat temp friendship for boat " << boatId
                  << ": " << e.base().what();
      },
      tempFriendId, uid1, uid2, userId, boatId, expiresAt.toDbStringLocal());
}

void assessBoatPsychologicalRiskAsync(const std::string &userId,
                                      const std::string &boatId,
                                      const std::string &content) {
  auto &aiService = heartlake::ai::AIService::getInstance();
  aiService.analyzeSentiment(
      content, [userId, boatId, content](float score,
                                         const std::string &detectedMood,
                                         const std::string &error) {
        if (!error.empty()) {
          return;
        }

        auto &riskAssessment =
            heartlake::utils::PsychologicalRiskAssessment::getInstance();
        auto riskResult =
            riskAssessment.assessRisk(content, userId, score, detectedMood);

        if (!riskResult.needsImmediateAttention) {
          return;
        }

        auto &pushService =
            heartlake::services::NotificationPushService::getInstance();
        pushService.pushSystemNotice(userId, "心理健康关怀",
                                     riskResult.supportMessage);

        try {
          auto db = drogon::app().getDbClient("default");
          auto trans = db->newTransaction();
          const auto keywordsLiteral = toPgTextArrayLiteral(riskResult.keywords);
          const auto factorsJson = serializeRiskFactors(riskResult.factors);
          auto assessmentResult = trans->execSqlSync(
              "INSERT INTO psychological_assessments "
              "(user_id, content_id, content_type, risk_level, risk_score, "
              "primary_concern, needs_immediate_attention, keywords, factors, "
              "support_message, created_at) "
              "VALUES ($1, $2, 'boat', $3, $4, $5, $6, "
              "NULLIF($7, '{}')::text[], $8::jsonb, $9, NOW()) "
              "RETURNING assessment_id",
              userId, boatId, static_cast<int>(riskResult.riskLevel),
              riskResult.overallScore, riskResult.primaryConcern,
              riskResult.needsImmediateAttention, keywordsLiteral, factorsJson,
              riskResult.supportMessage);

          const auto assessmentId =
              assessmentResult.empty()
                  ? 0
                  : assessmentResult[0]["assessment_id"].as<int64_t>();

          if (static_cast<int>(riskResult.riskLevel) >=
              static_cast<int>(heartlake::utils::RiskLevel::HIGH)) {
            if (assessmentId == 0) {
              trans->execSqlSync(
                  "INSERT INTO high_risk_events "
                  "(user_id, content_id, content_type, risk_level, risk_score, "
                  "intervention_sent, admin_notified, status, created_at) "
                  "VALUES ($1, $2, 'boat', $3, $4, $5, false, 'pending', NOW())",
                  userId, boatId, static_cast<int>(riskResult.riskLevel),
                  riskResult.overallScore, !riskResult.supportMessage.empty());
            } else {
              trans->execSqlSync(
                  "INSERT INTO high_risk_events "
                  "(user_id, content_id, content_type, risk_level, risk_score, "
                  "intervention_sent, admin_notified, status, assessment_id, "
                  "created_at) "
                  "VALUES ($1, $2, 'boat', $3, $4, $5, false, 'pending', $6, "
                  "NOW())",
                  userId, boatId, static_cast<int>(riskResult.riskLevel),
                  riskResult.overallScore, !riskResult.supportMessage.empty(),
                  assessmentId);
            }
          }
        } catch (const std::exception &e) {
          LOG_ERROR << "Failed to persist psychological assessment for boat "
                    << boatId << ": " << e.what();
        }

        if (riskResult.riskLevel == heartlake::utils::RiskLevel::CRITICAL) {
          auto db = drogon::app().getDbClient("default");
          db->execSqlAsync(
              "SELECT id AS admin_id FROM admin_users "
              "WHERE role IN ('admin', 'moderator', 'super_admin')",
              [userId, boatId](const drogon::orm::Result &r) {
                auto &pushSvc =
                    heartlake::services::NotificationPushService::getInstance();
                for (const auto &row : r) {
                  const auto adminId = row["admin_id"].as<std::string>();
                  const auto message =
                      "用户 " + userId + " 回复的纸船（ID: " + boatId +
                      "）检测到危机级别心理风险，请及时关注。";
                  pushSvc.pushSystemNotice(adminId, "⚠️ 危机预警", message);
                }
              },
              [](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "Failed to notify administrators: "
                          << e.base().what();
              });
        }
      });
}

using heartlake::utils::stone_cache::buildStoneDetailCacheKey;
using heartlake::utils::stone_cache::buildStoneRippleStateCacheKey;

void invalidateStoneReadCaches(
    const std::shared_ptr<heartlake::core::cache::CacheManager> &cacheManager,
    const std::string &stoneId, const std::string &stoneOwnerId,
    const std::string &moodType) {
  if (!cacheManager) {
    return;
  }

  cacheManager->invalidate(buildStoneDetailCacheKey(stoneId));
  heartlake::utils::stone_cache::bumpStoneListNamespacesForStone(stoneOwnerId,
                                                                 moodType);
}

} // namespace

namespace heartlake {
namespace application {

// ==================== 涟漪 ====================

/**
 * 创建涟漪（点赞）：
 *   1. 事务内验证石头存在 → 插入涟漪 → 递增计数器
 *   2. 失效缓存 → 发布事件 → 记录激励积分
 *   3. 唯一约束冲突时走幂等分支，返回已有涟漪信息
 */
Json::Value
InteractionApplicationService::createRipple(const std::string &stoneId,
                                            const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    auto trans = dbClient->newTransaction();

    std::string rippleId = utils::IdGenerator::generateRippleId();

    // 单条 CTE 合并石头校验、涟漪写入和计数更新，减少事务内往返
    auto writeResult = trans->execSqlSync(
        "WITH target AS ("
        "  SELECT stone_id, user_id AS stone_owner_id, mood_type FROM stones "
        "  WHERE stone_id = $2 AND status = 'published' AND deleted_at IS NULL"
        "), inserted AS ("
        "  INSERT INTO ripples (ripple_id, stone_id, user_id, created_at) "
        "  SELECT $1, stone_id, $3, NOW() FROM target "
        "  RETURNING ripple_id, stone_id, user_id"
        "), updated AS ("
        "  UPDATE stones SET ripple_count = ripple_count + 1 "
        "  WHERE stone_id IN (SELECT stone_id FROM target) "
        "  RETURNING ripple_count"
        ") "
        "SELECT i.ripple_id, i.stone_id, i.user_id, "
        "       t.stone_owner_id, t.mood_type, "
        "       COALESCE(u.ripple_count, 0) AS ripple_count "
        "FROM inserted i "
        "JOIN target t ON t.stone_id = i.stone_id "
        "LEFT JOIN updated u ON TRUE",
        rippleId, stoneId, userId);

    if (writeResult.empty()) {
      throw std::runtime_error("石头不存在");
    }

    const auto &row = writeResult[0];
    const auto stoneOwnerId = safeStringColumn(row, "stone_owner_id");
    const auto moodType = safeStringColumn(row, "mood_type");

    // 清除石头缓存，确保计数器实时更新
    invalidateStoneReadCaches(cacheManager_, stoneId, stoneOwnerId, moodType);
    if (cacheManager_) {
      cacheManager_->set(buildStoneRippleStateCacheKey(userId, stoneId), "1",
                         kStoneRippleStateCacheTtlSeconds);
    }

    // 共鸣激励：记录优质涟漪积分
    heartlake::infrastructure::GuardianIncentiveService::getInstance()
        .recordQualityRipple(userId, stoneId);

    Json::Value result;
    result["ripple_id"] = safeStringColumn(row, "ripple_id", rippleId);
    result["stone_id"] = safeStringColumn(row, "stone_id", stoneId);
    result["user_id"] = safeStringColumn(row, "user_id", userId);
    result["stone_owner_id"] = stoneOwnerId;
    result["ripple_count"] = row["ripple_count"].isNull()
                                 ? 1
                                 : row["ripple_count"].as<int>();
    result["notification_id"] = createInteractionNotificationAsync(
        stoneOwnerId, userId, "ripple", "有人给你的石头点了涟漪", stoneId,
        "stone");

    LOG_INFO << "Ripple created: " << rippleId;
    return result;

  } catch (const drogon::orm::DrogonDbException &e) {
    std::string err = e.base().what();
    if (err.find("unique") != std::string::npos ||
        err.find("duplicate") != std::string::npos) {
      // 幂等处理：重复点涟漪时用一条查询返回已有记录与当前计数
      auto currentState = dbClient->execSqlSync(
          "SELECT "
          "  (SELECT ripple_id FROM ripples "
          "   WHERE stone_id = $1 AND user_id = $2 "
          "   ORDER BY created_at DESC LIMIT 1) AS ripple_id, "
          "  s.user_id AS stone_owner_id, "
          "  s.ripple_count "
          "FROM stones s WHERE s.stone_id = $1 AND s.deleted_at IS NULL",
          stoneId, userId);

      Json::Value result;
      result["ripple_id"] =
          currentState.empty() || currentState[0]["ripple_id"].isNull()
              ? ""
              : currentState[0]["ripple_id"].as<std::string>();
      result["stone_id"] = stoneId;
      result["user_id"] = userId;
      result["stone_owner_id"] =
          currentState.empty() || currentState[0]["stone_owner_id"].isNull()
              ? ""
              : currentState[0]["stone_owner_id"].as<std::string>();
      result["ripple_count"] =
          currentState.empty() || currentState[0]["ripple_count"].isNull()
              ? 0
              : currentState[0]["ripple_count"].as<int>();
      result["already_rippled"] = true;
      if (cacheManager_) {
        cacheManager_->set(buildStoneRippleStateCacheKey(userId, stoneId), "1",
                           kStoneRippleStateCacheTtlSeconds);
      }

      LOG_INFO << "Ripple already exists for stone " << stoneId << " user "
               << userId;
      return result;
    }
    LOG_ERROR << "Failed to create ripple: " << err;
    throw std::runtime_error("创建涟漪失败");
  }
}

Json::Value
InteractionApplicationService::getRipples(const std::string &stoneId, int page,
                                          int pageSize) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int64_t offset = static_cast<int64_t>((page - 1) * pageSize);

    auto result = dbClient->execSqlSync(
        "WITH total_ripples AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM ripples "
        "  WHERE stone_id = $1"
        ") "
        "SELECT paged.ripple_id, paged.user_id, paged.created_at, "
        "       paged.username, paged.nickname, paged.avatar_url, "
        "       tr.total_count "
        "FROM total_ripples tr "
        "LEFT JOIN LATERAL ("
        "  SELECT r.ripple_id, r.user_id, r.created_at, "
        "         u.username, u.nickname, u.avatar_url "
        "  FROM ripples r "
        "  LEFT JOIN users u ON r.user_id = u.user_id "
        "  WHERE r.stone_id = $1 "
        "  ORDER BY r.created_at DESC "
        "  LIMIT $2 OFFSET $3"
        ") paged ON TRUE",
        stoneId, static_cast<int64_t>(pageSize), offset);

    Json::Value ripples(Json::arrayValue);
    for (const auto &row : result) {
      if (row["ripple_id"].isNull()) {
        continue;
      }
      Json::Value ripple;
      const auto rippleId = safeStringColumn(row, "ripple_id");
      const auto userId = safeStringColumn(row, "user_id");
      ripple["ripple_id"] = rippleId;
      ripple["id"] = rippleId;
      ripple["stone_id"] = stoneId;
      ripple["user_id"] = userId;
      ripple["userId"] = userId;
      ripple["created_at"] = safeStringColumn(row, "created_at");
      ripple["createdAt"] = ripple["created_at"];

      auto user = buildUserPayload(userId, safeStringColumn(row, "username"),
                                   safeStringColumn(row, "nickname"),
                                   safeStringColumn(row, "avatar_url"));
      ripple["user"] = user;

      ripples.append(ripple);
    }

    return ResponseUtil::buildCollectionPayload(
        "ripples", ripples, extractTotalCount(result), page, pageSize);

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get ripples: " << e.base().what();
    throw std::runtime_error("获取涟漪列表失败");
  }
}

Json::Value InteractionApplicationService::deleteRipple(
    const std::string &rippleId, const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    auto trans = dbClient->newTransaction();

    auto result = trans->execSqlSync(
        "WITH deleted AS ("
        "  DELETE FROM ripples "
        "  WHERE ripple_id = $1 AND user_id = $2 "
        "  RETURNING stone_id"
        "), updated AS ("
        "  UPDATE stones "
        "  SET ripple_count = GREATEST(ripple_count - 1, 0) "
        "  WHERE stone_id IN (SELECT stone_id FROM deleted) "
        "  RETURNING stone_id, ripple_count"
        ") "
        "SELECT d.stone_id, s.user_id AS stone_owner_id, s.mood_type, "
        "       COALESCE(u.ripple_count, 0) AS ripple_count "
        "FROM deleted d "
        "LEFT JOIN updated u ON u.stone_id = d.stone_id "
        "LEFT JOIN stones s ON s.stone_id = d.stone_id",
        rippleId, userId);

    if (result.empty()) {
      throw std::runtime_error("涟漪不存在或无权删除");
    }

    auto row = *safeRow(result);
    std::string stoneId = row["stone_id"].as<std::string>();
    const auto stoneOwnerId = safeStringColumn(row, "stone_owner_id");
    const auto moodType = safeStringColumn(row, "mood_type");

    invalidateStoneReadCaches(cacheManager_, stoneId, stoneOwnerId, moodType);
    if (cacheManager_) {
      cacheManager_->set(buildStoneRippleStateCacheKey(userId, stoneId), "0",
                         kStoneRippleStateCacheTtlSeconds);
    }

    Json::Value response;
    response["stone_id"] = stoneId;
    response["ripple_count"] =
        row["ripple_count"].isNull() ? 0 : row["ripple_count"].as<int>();

    LOG_INFO << "Ripple deleted: " << rippleId;
    return response;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to delete ripple: " << e.base().what();
    throw std::runtime_error("删除涟漪失败");
  }
}

// ==================== 纸船（私信） ====================

/// 发送纸船私信：生成 ID → 入库 → 发布事件 → 返回结果
Json::Value InteractionApplicationService::sendBoat(
    const std::string &stoneId, const std::string &senderId,
    const std::string &receiverId, const std::string &message) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    // 1. 生成纸船 ID
    std::string boatId = utils::IdGenerator::generateBoatId();

    // 2. 入库并同步石头 boat_count，避免旁路写入导致计数漂移
    auto writeResult = dbClient->execSqlSync(
        "WITH target AS ("
        "  SELECT stone_id, user_id AS stone_owner_id, mood_type "
        "  FROM stones "
        "  WHERE stone_id = $2 AND deleted_at IS NULL"
        "), updated AS ("
        "  UPDATE stones "
        "  SET boat_count = boat_count + 1, updated_at = NOW() "
        "  WHERE stone_id IN (SELECT stone_id FROM target) "
        "  RETURNING stone_id, boat_count"
        "), inserted AS ("
        "  INSERT INTO paper_boats (boat_id, stone_id, sender_id, receiver_id, "
        "                           content, is_anonymous, status, created_at) "
        "  SELECT $1, $2, $3, $4, $5, true, 'active', NOW() "
        "  FROM target "
        "  RETURNING boat_id, stone_id, sender_id, receiver_id, content, status"
        ") "
        "SELECT i.boat_id, i.stone_id, i.sender_id, i.receiver_id, i.content, "
        "       i.status, t.stone_owner_id, t.mood_type, "
        "       COALESCE(u.boat_count, 1) AS boat_count "
        "FROM inserted i "
        "JOIN target t ON t.stone_id = i.stone_id "
        "LEFT JOIN updated u ON u.stone_id = i.stone_id",
        boatId, stoneId, senderId, receiverId, message);
    if (writeResult.empty()) {
      throw std::runtime_error("石头不存在");
    }

    invalidateStoneReadCaches(cacheManager_, stoneId,
                              safeStringColumn(writeResult[0], "stone_owner_id"),
                              safeStringColumn(writeResult[0], "mood_type"));

    // 3. 发布事件
    core::events::BoatSentEvent event;
    event.boatId = boatId;
    event.stoneId = stoneId;
    event.senderId = senderId;
    event.content = message;

    eventBus_->publish(event);

    // 4. 返回结果
    Json::Value result;
    result["boat_id"] = boatId;
    result["stone_id"] = stoneId;
    result["sender_id"] = senderId;
    result["senderId"] = senderId;
    result["receiver_id"] = receiverId;
    result["receiverId"] = receiverId;
    result["content"] = message;
    result["message"] = message;
    result["status"] = "sent";
    result["boat_count"] =
        writeResult[0]["boat_count"].isNull() ? 1 : writeResult[0]["boat_count"].as<int>();

    LOG_INFO << "Boat sent: " << boatId;

    return result;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to send boat: " << e.base().what();
    throw std::runtime_error("发送纸船失败");
  }
}

Json::Value
InteractionApplicationService::getReceivedBoats(const std::string &userId,
                                                int page, int pageSize) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int64_t offset = static_cast<int64_t>((page - 1) * pageSize);

    auto result = dbClient->execSqlSync(
        "WITH total_boats AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM paper_boats "
        "  WHERE receiver_id = $1 "
        "    AND COALESCE(status, 'active') != 'deleted'"
        ") "
        "SELECT paged.boat_id, paged.stone_id, paged.sender_id, paged.receiver_id, "
        "       paged.content, paged.status, paged.created_at, "
        "       paged.boat_color, paged.is_anonymous, paged.username, "
        "       paged.nickname, paged.avatar_url, paged.stone_content, "
        "       paged.stone_mood_type, tb.total_count "
        "FROM total_boats tb "
        "LEFT JOIN LATERAL ("
        "  SELECT pb.boat_id, pb.stone_id, pb.sender_id, pb.receiver_id, "
        "         pb.content, pb.status, pb.created_at, "
        "         pb.boat_style AS boat_color, pb.is_anonymous, "
        "         pb.is_ai_reply, "
        "         u.username, u.nickname, u.avatar_url, "
        "         s.content as stone_content, s.mood_type AS stone_mood_type "
        "  FROM paper_boats pb "
        "  LEFT JOIN users u ON pb.sender_id = u.user_id "
        "  LEFT JOIN stones s ON pb.stone_id = s.stone_id "
        "  WHERE pb.receiver_id = $1 AND COALESCE(pb.status, 'active') != "
        "        'deleted' "
        "  ORDER BY pb.created_at DESC "
        "  LIMIT $2 OFFSET $3"
        ") paged ON TRUE",
        userId, static_cast<int64_t>(pageSize), offset);

    Json::Value boats(Json::arrayValue);
    for (const auto &row : result) {
      if (row["boat_id"].isNull()) {
        continue;
      }
      Json::Value boat;
      const auto boatId = safeStringColumn(row, "boat_id");
      const auto senderId = safeStringColumn(row, "sender_id");
      const auto receiverId = safeStringColumn(row, "receiver_id", userId);
      const bool isAiReply = safeBoolColumn(row, "is_ai_reply", false);
      boat["boat_id"] = boatId;
      boat["id"] = boatId;
      boat["stone_id"] = safeStringColumn(row, "stone_id");
      boat["sender_id"] = senderId;
      boat["senderId"] = senderId;
      boat["receiver_id"] = receiverId;
      boat["receiverId"] = receiverId;
      boat["content"] = safeStringColumn(row, "content");
      boat["status"] = safeStringColumn(row, "status", "active");
      boat["boat_color"] = safeStringColumn(row, "boat_color", "#F5EFE7");
      boat["is_anonymous"] = safeBoolColumn(row, "is_anonymous", true);
      boat["isAnonymous"] = boat["is_anonymous"];
      boat["is_ai_reply"] = isAiReply;
      boat["isAiReply"] = isAiReply;
      boat["created_at"] = safeStringColumn(row, "created_at");
      boat["createdAt"] = boat["created_at"];

      auto sender =
          buildBoatAuthorPayload(senderId, safeStringColumn(row, "username"),
                                 safeStringColumn(row, "nickname"),
                                 safeStringColumn(row, "avatar_url"),
                                 safeBoolColumn(row, "is_anonymous", true),
                                 isAiReply);
      boat["sender"] = sender;
      boat["author"] = sender;
      boat["sender_name"] = safeStringColumn(row, "nickname");
      if (isLakeGodActor(senderId, isAiReply)) {
        boat["sender_name"] = "湖神";
        boat["senderName"] = "湖神";
        boat["agent_name"] = "湖神";
      }

      if (!row["stone_content"].isNull()) {
        boat["stone_content"] = row["stone_content"].as<std::string>();
        boat["content_preview"] = boat["stone_content"];
      }
      if (!row["stone_mood_type"].isNull()) {
        boat["stone_mood_type"] = row["stone_mood_type"].as<std::string>();
      }
      boats.append(boat);
    }

    return ResponseUtil::buildCollectionPayload(
        "boats", boats, extractTotalCount(result), page, pageSize);

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get received boats: " << e.base().what();
    throw std::runtime_error("获取收到的纸船失败");
  }
}

Json::Value
InteractionApplicationService::getSentBoats(const std::string &userId, int page,
                                            int pageSize,
                                            const std::string &statusFilter) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int64_t offset = static_cast<int64_t>((page - 1) * pageSize);
    const std::string normalizedStatus = normalizeBoatStatusFilter(statusFilter);
    static const std::set<std::string> validStatuses = {"active", "pending",
                                                        "draft", "deleted"};
    const bool hasStatusFilter = !normalizedStatus.empty() &&
                                 normalizedStatus != "all" &&
                                 validStatuses.count(normalizedStatus) > 0;

    auto result = [&]() -> drogon::orm::Result {
      if (hasStatusFilter) {
        return dbClient->execSqlSync(
            "WITH total_boats AS ("
            "  SELECT COUNT(*)::INTEGER AS total_count "
            "  FROM paper_boats "
            "  WHERE sender_id = $1 AND status = $2"
            ") "
            "SELECT paged.boat_id, paged.stone_id, paged.receiver_id, "
            "       paged.content, paged.status, paged.created_at, "
            "       paged.boat_color, paged.is_anonymous, paged.username, "
            "       paged.nickname, paged.avatar_url, tb.total_count "
            "FROM total_boats tb "
            "LEFT JOIN LATERAL ("
            "  SELECT pb.boat_id, pb.stone_id, pb.receiver_id, "
            "         pb.content, pb.status, pb.created_at, "
            "         pb.boat_style AS boat_color, pb.is_anonymous, "
            "         u.username, u.nickname, u.avatar_url "
            "  FROM paper_boats pb "
            "  LEFT JOIN users u ON pb.receiver_id = u.user_id "
            "  WHERE pb.sender_id = $1 AND pb.status = $2 "
            "  ORDER BY pb.created_at DESC "
            "  LIMIT $3 OFFSET $4"
            ") paged ON TRUE",
            userId, normalizedStatus, static_cast<int64_t>(pageSize), offset);
      }

      return dbClient->execSqlSync(
          "WITH total_boats AS ("
          "  SELECT COUNT(*)::INTEGER AS total_count "
          "  FROM paper_boats "
          "  WHERE sender_id = $1 AND COALESCE(status, 'active') != 'deleted'"
          ") "
          "SELECT paged.boat_id, paged.stone_id, paged.receiver_id, "
          "       paged.content, paged.status, paged.created_at, "
          "       paged.boat_color, paged.is_anonymous, paged.username, "
          "       paged.nickname, paged.avatar_url, tb.total_count "
          "FROM total_boats tb "
          "LEFT JOIN LATERAL ("
          "  SELECT pb.boat_id, pb.stone_id, pb.receiver_id, "
          "         pb.content, pb.status, pb.created_at, "
          "         pb.boat_style AS boat_color, pb.is_anonymous, "
          "         u.username, u.nickname, u.avatar_url "
          "  FROM paper_boats pb "
          "  LEFT JOIN users u ON pb.receiver_id = u.user_id "
          "  WHERE pb.sender_id = $1 AND COALESCE(pb.status, 'active') != "
          "        'deleted' "
          "  ORDER BY pb.created_at DESC "
          "  LIMIT $2 OFFSET $3"
          ") paged ON TRUE",
          userId, static_cast<int64_t>(pageSize), offset);
    }();

    Json::Value boats(Json::arrayValue);
    for (const auto &row : result) {
      if (row["boat_id"].isNull()) {
        continue;
      }
      Json::Value boat;
      const auto boatId = safeStringColumn(row, "boat_id");
      const auto receiverId = safeStringColumn(row, "receiver_id");
      boat["boat_id"] = boatId;
      boat["id"] = boatId;
      boat["stone_id"] = safeStringColumn(row, "stone_id");
      boat["sender_id"] = userId;
      boat["senderId"] = userId;
      boat["receiver_id"] = receiverId;
      boat["receiverId"] = receiverId;
      boat["content"] = safeStringColumn(row, "content");
      boat["status"] = safeStringColumn(row, "status", "active");
      boat["boat_color"] = safeStringColumn(row, "boat_color", "#F5EFE7");
      boat["is_anonymous"] = safeBoolColumn(row, "is_anonymous", true);
      boat["isAnonymous"] = boat["is_anonymous"];
      boat["created_at"] = safeStringColumn(row, "created_at");
      boat["createdAt"] = boat["created_at"];

      auto receiver =
          buildUserPayload(receiverId, safeStringColumn(row, "username"),
                           safeStringColumn(row, "nickname"),
                           safeStringColumn(row, "avatar_url"));
      boat["receiver"] = receiver;
      boats.append(boat);
    }

    return ResponseUtil::buildCollectionPayload(
        "boats", boats, extractTotalCount(result), page, pageSize);

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get sent boats: " << e.base().what();
    throw std::runtime_error("获取发送的纸船失败");
  }
}

// ==================== 纸船（评论系统）====================

/**
 * 创建纸船评论：
 *   1. 入库 → 递增石头的 boat_count → 失效缓存
 *   2. 用 EdgeAIEngine 本地情感分析评估暖意分，按比例发放激励积分
 */
Json::Value
InteractionApplicationService::createBoat(const std::string &stoneId,
                                          const std::string &userId,
                                          const std::string &content) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    std::string boatId = utils::IdGenerator::generateBoatId();

    // 单条 CTE 合并石头计数更新与纸船写入，避免额外 count 查询
    auto writeResult = dbClient->execSqlSync(
        "WITH target AS ("
        "  SELECT stone_id, user_id AS stone_owner_id, mood_type "
        "  FROM stones "
        "  WHERE stone_id = $2 AND status = 'published' AND deleted_at IS NULL"
        "), updated AS ("
        "  UPDATE stones SET boat_count = boat_count + 1, updated_at = NOW() "
        "  WHERE stone_id IN (SELECT stone_id FROM target) "
        "  RETURNING stone_id, boat_count"
        "), inserted AS ("
        "  INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, "
        "                           is_anonymous, status, created_at) "
        "  SELECT $1, $2, $3, $4, true, 'active', NOW() "
        "  FROM target "
        "  RETURNING boat_id, stone_id, sender_id, content"
        ") "
        "SELECT i.boat_id, i.stone_id, i.sender_id, i.content, "
        "       t.stone_owner_id, t.mood_type, u.boat_count "
        "FROM inserted i "
        "JOIN target t ON t.stone_id = i.stone_id "
        "JOIN updated u ON u.stone_id = i.stone_id",
        boatId, stoneId, userId, content);

    if (writeResult.empty()) {
      throw std::runtime_error("石头不存在");
    }

    const auto &row = writeResult[0];
    const auto stoneOwnerId = safeStringColumn(row, "stone_owner_id");
    const auto moodType = safeStringColumn(row, "mood_type");

    // 2.6 清除石头缓存，确保计数器实时更新
    invalidateStoneReadCaches(cacheManager_, stoneId, stoneOwnerId, moodType);

    // 温暖纸船激励：用本地情绪模型估算暖意分，按比例发放积分
    auto &edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
    auto sentiment = edgeEngine.analyzeSentimentLocal(content);
    const float warmthScore =
        std::clamp((sentiment.score + 1.0f) / 2.0f, 0.2f, 1.0f);
    heartlake::infrastructure::GuardianIncentiveService::getInstance()
        .recordWarmBoat(userId, boatId, warmthScore);

    core::events::BoatSentEvent event;
    event.boatId = boatId;
    event.stoneId = stoneId;
    event.senderId = userId;
    event.content = content;
    eventBus_->publish(event);

    const auto notificationId = createInteractionNotificationAsync(
        stoneOwnerId, userId, "boat", "有人给你的石头回了一封纸船", boatId,
        "boat");
    refreshBoatTempFriendshipAsync(userId, stoneOwnerId, boatId);
    assessBoatPsychologicalRiskAsync(userId, boatId, content);

    // 3. 返回结果
    Json::Value result;
    result["boat_id"] = safeStringColumn(row, "boat_id", boatId);
    result["stone_id"] = safeStringColumn(row, "stone_id", stoneId);
    result["sender_id"] = safeStringColumn(row, "sender_id", userId);
    result["senderId"] = result["sender_id"];
    result["content"] = safeStringColumn(row, "content", content);
    result["message"] = result["content"];
    result["status"] = "sent";
    result["stone_owner_id"] = stoneOwnerId;
    result["boat_count"] =
        row["boat_count"].isNull() ? 1 : row["boat_count"].as<int>();
    result["notification_id"] = notificationId;

    LOG_INFO << "Boat created: " << boatId;

    return result;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to create boat: " << e.base().what();
    throw std::runtime_error("创建纸船失败");
  }
}

/// 删除纸船：事务内删除记录 + RETURNING 递减计数器 + 失效缓存
Json::Value
InteractionApplicationService::deleteBoat(const std::string &boatId,
                                          const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    auto trans = dbClient->newTransaction();

    auto deleteResult = trans->execSqlSync(
        "WITH deleted AS ("
        "  DELETE FROM paper_boats "
        "  WHERE boat_id = $1 AND sender_id = $2 "
        "  RETURNING stone_id"
        "), updated AS ("
        "  UPDATE stones "
        "  SET boat_count = GREATEST(boat_count - 1, 0) "
        "  WHERE stone_id IN (SELECT stone_id FROM deleted WHERE stone_id IS NOT NULL) "
        "  RETURNING stone_id, boat_count"
        ") "
        "SELECT d.stone_id, s.user_id AS stone_owner_id, s.mood_type, "
        "       COALESCE(u.boat_count, 0) AS boat_count "
        "FROM deleted d "
        "LEFT JOIN updated u ON u.stone_id = d.stone_id "
        "LEFT JOIN stones s ON s.stone_id = d.stone_id",
        boatId, userId);

    if (deleteResult.empty()) {
      throw std::runtime_error("纸船不存在或无权删除");
    }

    const auto &row = deleteResult[0];
    std::string stoneId =
        row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
    const auto stoneOwnerId = safeStringColumn(row, "stone_owner_id");
    const auto moodType = safeStringColumn(row, "mood_type");
    const int boatCount =
        row["boat_count"].isNull() ? 0 : row["boat_count"].as<int>();

    if (!stoneId.empty()) {
      invalidateStoneReadCaches(cacheManager_, stoneId, stoneOwnerId, moodType);
    }

    Json::Value response;
    response["stone_id"] = stoneId;
    response["boat_count"] = boatCount;

    LOG_INFO << "Boat deleted: " << boatId;
    return response;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to delete boat: " << e.base().what();
    throw std::runtime_error("删除纸船失败");
  }
}

// ==================== 通知 ====================

/// 标记单条通知为已读
void InteractionApplicationService::markNotificationRead(
    const std::string &notificationId, const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    dbClient->execSqlSync("UPDATE notifications SET is_read = true "
                          "WHERE notification_id = $1 AND user_id = $2",
                          notificationId, userId);

    LOG_INFO << "Notification marked as read: " << notificationId;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to mark notification as read: " << e.base().what();
    throw std::runtime_error("标记通知已读失败");
  }
}

/// 一键标记用户所有通知为已读
void InteractionApplicationService::markAllNotificationsRead(
    const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    dbClient->execSqlSync(
        "UPDATE notifications SET is_read = true WHERE user_id = $1", userId);

    LOG_INFO << "All notifications marked as read for user: " << userId;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to mark all notifications as read: "
              << e.base().what();
    throw std::runtime_error("标记所有通知已读失败");
  }
}

Json::Value
InteractionApplicationService::getNotifications(const std::string &userId,
                                                int page, int pageSize) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int64_t offset = static_cast<int64_t>((page - 1) * pageSize);

    auto result = dbClient->execSqlSync(
        "WITH notification_totals AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count, "
        "         COUNT(*) FILTER (WHERE is_read = false)::INTEGER AS unread_count "
        "  FROM notifications "
        "  WHERE user_id = $1"
        ") "
        "SELECT paged.notification_id, paged.type, paged.title, paged.content, "
        "       paged.related_id, paged.related_type, paged.is_read, "
        "       paged.created_at, paged.stone_id, totals.total_count, totals.unread_count "
        "FROM notification_totals totals "
        "LEFT JOIN LATERAL ("
        "  SELECT n.notification_id, n.type, n.title, n.content, n.related_id, n.related_type, "
        "         n.is_read, n.created_at, "
        "         COALESCE(CASE "
        "                    WHEN n.related_type = 'stone' THEN n.related_id "
        "                    ELSE NULL "
        "                  END, pb.stone_id) AS stone_id "
        "  FROM notifications n "
        "  LEFT JOIN paper_boats pb "
        "    ON n.related_type = 'boat' AND pb.boat_id = n.related_id "
        "  WHERE n.user_id = $1 "
        "  ORDER BY created_at DESC "
        "  LIMIT $2 OFFSET $3"
        ") paged ON TRUE",
        userId, static_cast<int64_t>(pageSize), static_cast<int64_t>(offset));

    Json::Value notifications(Json::arrayValue);
    for (const auto &row : result) {
      if (row["notification_id"].isNull()) {
        continue;
      }
      Json::Value notification;
      const auto notificationId = safeStringColumn(row, "notification_id");
      notification["notification_id"] = notificationId;
      notification["id"] = notificationId;
      notification["notificationId"] = notificationId;
      notification["type"] = safeStringColumn(row, "type");
      notification["notification_type"] = notification["type"];
      notification["title"] = safeStringColumn(row, "title");
      notification["content"] = safeStringColumn(row, "content");
      notification["related_id"] = safeStringColumn(row, "related_id");
      notification["relatedId"] = notification["related_id"];
      notification["related_type"] = safeStringColumn(row, "related_type");
      notification["relatedType"] = notification["related_type"];
      notification["stone_id"] = safeStringColumn(row, "stone_id");
      notification["stoneId"] = notification["stone_id"];
      notification["is_read"] = safeBoolColumn(row, "is_read");
      notification["isRead"] = notification["is_read"];
      notification["created_at"] = safeStringColumn(row, "created_at");
      notification["createdAt"] = notification["created_at"];

      notifications.append(notification);
    }

    const int total = extractTotalCount(result);
    const int unreadCount =
        result.empty() || result[0]["unread_count"].isNull()
            ? 0
            : result[0]["unread_count"].as<int>();

    auto response = ResponseUtil::buildCollectionPayload(
        "notifications", notifications, total, page, pageSize);
    response["unread_count"] = unreadCount;
    response["unreadCount"] = response["unread_count"];
    return response;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get notifications: " << e.base().what();
    throw std::runtime_error("获取通知列表失败");
  }
}

// ==================== 临时连接 ====================

/// 基于石头创建临时连接（24h 有效），用于匿名聊天
Json::Value InteractionApplicationService::createConnectionForStone(
    const std::string &stoneId, const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    auto stoneResult = dbClient->execSqlSync(
        "SELECT user_id FROM stones "
        "WHERE stone_id = $1 AND status = 'published' AND deleted_at IS NULL",
        stoneId);
    if (stoneResult.empty()) {
      throw std::runtime_error("石头不存在");
    }

    const auto targetUserId = stoneResult[0]["user_id"].as<std::string>();
    if (targetUserId.empty() || targetUserId == userId) {
      throw std::runtime_error("不能和自己建立连接");
    }
    ensureTargetUserAvailable(dbClient, userId, targetUserId);

    auto existing = dbClient->execSqlSync(
        "SELECT connection_id, user_id, target_user_id, stone_id, status, "
        "created_at, expires_at "
        "FROM connections "
        "WHERE stone_id = $1 "
        "AND ((user_id = $2 AND target_user_id = $3) "
        "  OR (user_id = $3 AND target_user_id = $2)) "
        "AND status IN ('pending', 'active') "
        "AND (expires_at IS NULL OR expires_at > NOW()) "
        "ORDER BY created_at DESC LIMIT 1",
        stoneId, userId, targetUserId);
    if (!existing.empty()) {
      return buildConnectionPayload(existing[0]);
    }

    const std::string connectionId = utils::IdGenerator::generateConnectionId();
    auto insertResult = dbClient->execSqlSync(
        "INSERT INTO connections (connection_id, user_id, target_user_id, "
        "stone_id, status, created_at, expires_at) "
        "VALUES ($1, $2, $3, $4, 'active', NOW(), NOW() + INTERVAL '24 hours') "
        "RETURNING connection_id, user_id, target_user_id, stone_id, status, "
        "created_at, expires_at",
        connectionId, userId, targetUserId, stoneId);

    LOG_INFO << "Connection created: " << connectionId;
    return buildConnectionPayload(*safeRow(insertResult));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to create connection: " << e.base().what();
    throw std::runtime_error("创建连接失败");
  }
}

/// 基于目标用户创建直接临时连接（24h 有效）
Json::Value
InteractionApplicationService::createConnection(const std::string &targetUserId,
                                                const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    if (targetUserId.empty() || targetUserId == userId) {
      throw std::runtime_error("目标用户无效");
    }

    ensureDirectConnectionTargetAllowed(dbClient, userId, targetUserId);

    auto existing = dbClient->execSqlSync(
        "SELECT connection_id, user_id, target_user_id, stone_id, status, "
        "created_at, expires_at "
        "FROM connections "
        "WHERE stone_id IS NULL "
        "AND ((user_id = $1 AND target_user_id = $2) "
        "  OR (user_id = $2 AND target_user_id = $1)) "
        "AND status IN ('pending', 'active') "
        "AND (expires_at IS NULL OR expires_at > NOW()) "
        "ORDER BY created_at DESC LIMIT 1",
        userId, targetUserId);
    if (!existing.empty()) {
      return buildConnectionPayload(existing[0]);
    }

    const std::string connectionId = utils::IdGenerator::generateConnectionId();
    auto insertResult = dbClient->execSqlSync(
        "INSERT INTO connections (connection_id, user_id, target_user_id, "
        "status, created_at, expires_at) "
        "VALUES ($1, $2, $3, 'active', NOW(), NOW() + INTERVAL '24 hours') "
        "RETURNING connection_id, user_id, target_user_id, stone_id, status, "
        "created_at, expires_at",
        connectionId, userId, targetUserId);

    LOG_INFO << "Direct connection created: " << connectionId;
    return buildConnectionPayload(*safeRow(insertResult));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to create direct connection: " << e.base().what();
    throw std::runtime_error("创建连接失败");
  }
}

/// 在连接内发送消息，先验证用户是连接参与者（防 IDOR）
Json::Value InteractionApplicationService::createConnectionMessage(
    const std::string &connectionId, const std::string &userId,
    const std::string &content) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    auto insertResult = dbClient->execSqlSync(
        "WITH authorized AS ("
        "  SELECT connection_id FROM connections c "
        "  WHERE connection_id = $1 "
        "    AND (c.user_id = $2 OR c.target_user_id = $2) "
        "    AND status IN ('pending', 'active') "
        "    AND (expires_at IS NULL OR expires_at > NOW()) "
        "    AND NOT EXISTS(SELECT 1 FROM user_blocks ub "
        "                   WHERE (ub.user_id = c.user_id "
        "                          AND ub.blocked_user_id = c.target_user_id) "
        "                      OR (ub.user_id = c.target_user_id "
        "                          AND ub.blocked_user_id = c.user_id)) "
        "    AND ("
        "      c.stone_id IS NOT NULL "
        "      OR EXISTS(SELECT 1 FROM friends f "
        "                WHERE ((f.user_id = c.user_id AND f.friend_id = c.target_user_id) "
        "                    OR (f.user_id = c.target_user_id AND f.friend_id = c.user_id)) "
        "                  AND f.status = 'accepted') "
        "      OR COALESCE((SELECT allow_message_from_stranger "
        "                   FROM user_privacy_settings ups "
        "                   WHERE ups.user_id = CASE "
        "                     WHEN c.user_id = $2 THEN c.target_user_id "
        "                     ELSE c.user_id END), false)"
        "    )"
        "), inserted AS ("
        "  INSERT INTO connection_messages "
        "  (connection_id, sender_id, content, created_at) "
        "  SELECT connection_id, $2, $3, NOW() FROM authorized "
        "  RETURNING id, connection_id, sender_id, content, created_at"
        ") "
        "SELECT id, connection_id, sender_id, content, created_at "
        "FROM inserted",
        connectionId, userId, content);

    if (insertResult.empty()) {
      throw std::runtime_error("连接不存在、已过期或无权操作");
    }

    auto row = *safeRow(insertResult);

    Json::Value result;
    result["message_id"] = std::to_string(row["id"].as<int>());
    result["id"] = result["message_id"];
    result["messageId"] = result["message_id"];
    result["connection_id"] = connectionId;
    result["sender_id"] = userId;
    result["senderId"] = userId;
    result["content"] = content;
    result["created_at"] = row["created_at"].as<std::string>();
    result["createdAt"] = result["created_at"];

    LOG_INFO << "Connection message created: "
             << result["message_id"].asString();

    return result;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to create connection message: " << e.base().what();
    throw std::runtime_error("发送连接消息失败");
  }
}

Json::Value
InteractionApplicationService::getMyRipples(const std::string &userId, int page,
                                            int pageSize) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int64_t offset = static_cast<int64_t>((page - 1) * pageSize);

    auto result = dbClient->execSqlSync(
        "WITH total_ripples AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM ripples "
        "  WHERE user_id = $1"
        ") "
        "SELECT paged.ripple_id, paged.stone_id, paged.created_at, "
        "       paged.stone_content, paged.stone_mood_type, paged.stone_user_id, "
        "       paged.stone_status, paged.ripple_count, paged.boat_count, "
        "       tr.total_count "
        "FROM total_ripples tr "
        "LEFT JOIN LATERAL ("
        "  SELECT r.ripple_id, r.stone_id, r.created_at, "
        "         s.content as stone_content, s.mood_type AS stone_mood_type, "
        "         s.user_id AS stone_user_id, s.status AS stone_status, "
        "         s.ripple_count, s.boat_count "
        "  FROM ripples r "
        "  LEFT JOIN stones s ON r.stone_id = s.stone_id "
        "  WHERE r.user_id = $1 "
        "  ORDER BY r.created_at DESC "
        "  LIMIT $2 OFFSET $3"
        ") paged ON TRUE",
        userId, static_cast<int64_t>(pageSize), offset);

    Json::Value ripples(Json::arrayValue);
    for (const auto &row : result) {
      if (row["ripple_id"].isNull()) {
        continue;
      }
      Json::Value ripple;
      const auto rippleId = safeStringColumn(row, "ripple_id");
      ripple["ripple_id"] = rippleId;
      ripple["id"] = rippleId;
      ripple["stone_id"] = safeStringColumn(row, "stone_id");
      ripple["created_at"] = safeStringColumn(row, "created_at");
      ripple["createdAt"] = ripple["created_at"];

      if (!row["stone_content"].isNull()) {
        ripple["stone_content"] = row["stone_content"].as<std::string>();
        ripple["content"] = ripple["stone_content"];
      }
      if (!row["stone_mood_type"].isNull()) {
        ripple["stone_mood_type"] = row["stone_mood_type"].as<std::string>();
        ripple["mood_type"] = row["stone_mood_type"].as<std::string>();
      }
      if (!row["stone_user_id"].isNull()) {
        ripple["stone_user_id"] = row["stone_user_id"].as<std::string>();
        ripple["user_id"] = row["stone_user_id"].as<std::string>();
        ripple["userId"] = row["stone_user_id"].as<std::string>();
      }
      if (!row["stone_status"].isNull()) {
        ripple["status"] = row["stone_status"].as<std::string>();
      }
      if (!row["ripple_count"].isNull()) {
        ripple["ripple_count"] = row["ripple_count"].as<int>();
      }
      if (!row["boat_count"].isNull()) {
        ripple["boat_count"] = row["boat_count"].as<int>();
      }

      ripples.append(ripple);
    }

    return ResponseUtil::buildCollectionPayload(
        "ripples", ripples, extractTotalCount(result), page, pageSize);

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get my ripples: " << e.base().what();
    throw std::runtime_error("获取我的涟漪失败");
  }
}

Json::Value InteractionApplicationService::getMyBoats(const std::string &userId,
                                                      int page, int pageSize) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int64_t offset = static_cast<int64_t>((page - 1) * pageSize);

    auto result = dbClient->execSqlSync(
        "WITH total_boats AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM paper_boats "
        "  WHERE sender_id = $1 AND COALESCE(status, 'active') != 'deleted'"
        ") "
        "SELECT paged.boat_id, paged.stone_id, paged.content, paged.status, "
        "       paged.created_at, paged.boat_color, paged.is_anonymous, "
        "       paged.stone_content, paged.stone_mood_type, "
        "       paged.stone_user_id, paged.stone_status, tb.total_count "
        "FROM total_boats tb "
        "LEFT JOIN LATERAL ("
        "  SELECT pb.boat_id, pb.stone_id, pb.content, pb.status, pb.created_at, "
        "         pb.boat_style AS boat_color, pb.is_anonymous, "
        "         s.content as stone_content, s.mood_type AS stone_mood_type, "
        "         s.user_id AS stone_user_id, s.status AS stone_status "
        "  FROM paper_boats pb "
        "  LEFT JOIN stones s ON pb.stone_id = s.stone_id "
        "  WHERE pb.sender_id = $1 AND COALESCE(pb.status, 'active') != "
        "        'deleted' "
        "  ORDER BY pb.created_at DESC LIMIT $2 OFFSET $3"
        ") paged ON TRUE",
        userId, pageSize, offset);

    Json::Value boats(Json::arrayValue);
    for (const auto &row : result) {
      if (row["boat_id"].isNull()) {
        continue;
      }
      Json::Value boat;
      const auto boatId = safeStringColumn(row, "boat_id");
      boat["boat_id"] = boatId;
      boat["id"] = boatId;
      boat["stone_id"] = safeStringColumn(row, "stone_id");
      boat["sender_id"] = userId;
      boat["senderId"] = userId;
      boat["content"] = safeStringColumn(row, "content");
      boat["status"] = safeStringColumn(row, "status", "unknown");
      boat["boat_color"] = safeStringColumn(row, "boat_color", "#F5EFE7");
      boat["is_anonymous"] = safeBoolColumn(row, "is_anonymous", true);
      boat["isAnonymous"] = boat["is_anonymous"];
      boat["created_at"] = safeStringColumn(row, "created_at");
      boat["createdAt"] = boat["created_at"];
      if (!row["stone_content"].isNull()) {
        boat["stone_content"] = row["stone_content"].as<std::string>();
      }
      if (!row["stone_mood_type"].isNull()) {
        boat["stone_mood_type"] = row["stone_mood_type"].as<std::string>();
      }
      if (!row["stone_user_id"].isNull()) {
        boat["stone_user_id"] = row["stone_user_id"].as<std::string>();
      }
      if (!row["stone_status"].isNull()) {
        boat["stone_status"] = row["stone_status"].as<std::string>();
      }
      boats.append(boat);
    }

    return ResponseUtil::buildCollectionPayload(
        "boats", boats, extractTotalCount(result), page, pageSize);

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get my boats: " << e.base().what();
    throw std::runtime_error("获取我的纸船失败");
  }
}

Json::Value InteractionApplicationService::getBoats(const std::string &stoneId,
                                                    int page, int pageSize) {
  auto dbClient = drogon::app().getDbClient("default");
  try {
    int64_t offset = (page - 1) * pageSize;
    auto result = dbClient->execSqlSync(
        "WITH total_boats AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM paper_boats "
        "  WHERE stone_id = $1 AND COALESCE(status, 'active') != 'deleted'"
        ") "
        "SELECT paged.boat_id, paged.stone_id, paged.sender_id, paged.content, "
        "       paged.created_at, paged.status, paged.boat_color, "
        "       paged.is_anonymous, paged.is_ai_reply, "
        "       paged.username, paged.sender_nickname, "
        "       paged.avatar_url, tb.total_count "
        "FROM total_boats tb "
        "LEFT JOIN LATERAL ("
        "  SELECT pb.boat_id, pb.stone_id, pb.sender_id, pb.content, "
        "         pb.created_at, pb.status, pb.boat_style AS boat_color, "
        "         pb.is_anonymous, pb.is_ai_reply, u.username, "
        "         u.nickname as sender_nickname, u.avatar_url "
        "  FROM paper_boats pb "
        "  LEFT JOIN users u ON pb.sender_id = u.user_id "
        "  WHERE pb.stone_id = $1 AND COALESCE(pb.status, 'active') != "
        "        'deleted' "
        "  ORDER BY pb.created_at DESC LIMIT $2 OFFSET $3"
        ") paged ON TRUE",
        stoneId, static_cast<int64_t>(pageSize), offset);
    Json::Value boats(Json::arrayValue);
    for (const auto &row : result) {
      if (row["boat_id"].isNull()) {
        continue;
      }
      Json::Value boat;
      const auto boatId = safeStringColumn(row, "boat_id");
      const auto senderId = safeStringColumn(row, "sender_id");
      const bool isAnonymous = safeBoolColumn(row, "is_anonymous", true);
      const bool isAiReply = safeBoolColumn(row, "is_ai_reply", false);
      boat["boat_id"] = boatId;
      boat["id"] = boatId;
      boat["stone_id"] = safeStringColumn(row, "stone_id", stoneId);
      boat["sender_id"] = senderId;
      boat["senderId"] = senderId;
      boat["content"] = safeStringColumn(row, "content");
      boat["status"] = safeStringColumn(row, "status", "active");
      boat["boat_color"] = safeStringColumn(row, "boat_color", "#F5EFE7");
      boat["is_anonymous"] = isAnonymous;
      boat["isAnonymous"] = isAnonymous;
      boat["is_ai_reply"] = isAiReply;
      boat["isAiReply"] = isAiReply;
      boat["created_at"] = safeStringColumn(row, "created_at");
      boat["createdAt"] = boat["created_at"];
      auto author = buildBoatAuthorPayload(
          senderId, safeStringColumn(row, "username"),
          safeStringColumn(row, "sender_nickname"),
          safeStringColumn(row, "avatar_url"), isAnonymous, isAiReply);
      boat["author"] = author;
      boat["sender"] = author;
      boat["sender_name"] = isLakeGodActor(senderId, isAiReply)
                                ? "湖神"
                                : safeStringColumn(row, "sender_nickname");
      if (isLakeGodActor(senderId, isAiReply)) {
        boat["senderName"] = "湖神";
        boat["agent_name"] = "湖神";
      }
      boats.append(boat);
    }
    return ResponseUtil::buildCollectionPayload(
        "boats", boats, extractTotalCount(result), page, pageSize);
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get boats: " << e.base().what();
    throw std::runtime_error("获取纸船列表失败");
  }
}

Json::Value InteractionApplicationService::getConnectionMessages(
    const std::string &connectionId, const std::string &userId, int page,
    int pageSize) {
  auto dbClient = drogon::app().getDbClient("default");
  try {
    const int64_t offset = static_cast<int64_t>((page - 1) * pageSize);
    auto result = dbClient->execSqlSync(
        "WITH authorized AS ("
        "  SELECT connection_id FROM connections c "
        "  WHERE connection_id = $1 "
        "    AND (c.user_id = $2 OR c.target_user_id = $2) "
        "    AND status IN ('pending', 'active') "
        "    AND (expires_at IS NULL OR expires_at > NOW()) "
        "    AND NOT EXISTS(SELECT 1 FROM user_blocks ub "
        "                   WHERE (ub.user_id = c.user_id "
        "                          AND ub.blocked_user_id = c.target_user_id) "
        "                      OR (ub.user_id = c.target_user_id "
        "                          AND ub.blocked_user_id = c.user_id)) "
        "    AND ("
        "      c.stone_id IS NOT NULL "
        "      OR EXISTS(SELECT 1 FROM friends f "
        "                WHERE ((f.user_id = c.user_id AND f.friend_id = c.target_user_id) "
        "                    OR (f.user_id = c.target_user_id AND f.friend_id = c.user_id)) "
        "                  AND f.status = 'accepted') "
        "      OR COALESCE((SELECT allow_message_from_stranger "
        "                   FROM user_privacy_settings ups "
        "                   WHERE ups.user_id = CASE "
        "                     WHEN c.user_id = $2 THEN c.target_user_id "
        "                     ELSE c.user_id END), false)"
        "    )"
        "), totals AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM connection_messages cm "
        "  JOIN authorized a ON a.connection_id = cm.connection_id"
        ") "
        "SELECT paged.id, paged.sender_id, paged.content, paged.created_at, "
        "       paged.username, paged.nickname, paged.avatar_url, "
        "       COALESCE(t.total_count, 0) AS total_count "
        "FROM authorized a "
        "CROSS JOIN totals t "
        "LEFT JOIN LATERAL ("
        "  SELECT cm.id, cm.sender_id, cm.content, cm.created_at, "
        "         u.username, u.nickname, u.avatar_url "
        "  FROM connection_messages cm "
        "  LEFT JOIN users u ON cm.sender_id = u.user_id "
        "  WHERE cm.connection_id = a.connection_id "
        "  ORDER BY cm.created_at ASC "
        "  LIMIT $3 OFFSET $4"
        ") paged ON TRUE",
        connectionId, userId, static_cast<int64_t>(pageSize), offset);
    if (result.empty()) {
      throw std::runtime_error("连接不存在、已过期或无权访问");
    }

    Json::Value messages(Json::arrayValue);
    for (const auto &row : result) {
      if (row["id"].isNull()) {
        continue;
      }

      Json::Value msg;
      const auto messageId = std::to_string(row["id"].as<int>());
      const auto senderId = safeStringColumn(row, "sender_id");
      msg["message_id"] = messageId;
      msg["id"] = messageId;
      msg["messageId"] = messageId;
      msg["connection_id"] = connectionId;
      msg["sender_id"] = senderId;
      msg["senderId"] = senderId;
      msg["content"] = safeStringColumn(row, "content");
      msg["created_at"] = safeStringColumn(row, "created_at");
      msg["createdAt"] = msg["created_at"];
      msg["sender"] =
          buildUserPayload(senderId, safeStringColumn(row, "username"),
                           safeStringColumn(row, "nickname"),
                           safeStringColumn(row, "avatar_url"));
      messages.append(msg);
    }
    return ResponseUtil::buildCollectionPayload(
        "messages", messages, extractTotalCount(result), page, pageSize);
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get connection messages: " << e.base().what();
    throw std::runtime_error("获取连接消息失败");
  }
}

} // namespace application
} // namespace heartlake
