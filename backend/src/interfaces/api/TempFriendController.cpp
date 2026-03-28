/**
 * TempFriendController 模块实现
 */
#include "interfaces/api/TempFriendController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/BusinessRules.h"
#include "utils/RealtimeEvent.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <trantor/utils/Logger.h>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace drogon;

namespace {
constexpr const char kTempFriendAliasedColumns[] =
    "tf.temp_friend_id, tf.user1_id, tf.user2_id, tf.source, tf.source_id, "
    "tf.status, tf.upgraded_to_friend, tf.created_at, tf.expires_at";

int readRemainingSeconds(const drogon::orm::Row &row) {
  return row["seconds_remaining"].isNull()
             ? 0
             : clampRemainingSeconds(row["seconds_remaining"].as<int>());
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
} // namespace

void TempFriendController::createTempFriend(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  if (!Validator::getUserId(req)) {
    callback(ResponseUtil::unauthorized("未登录"));
    return;
  }
  callback(ResponseUtil::badRequest(
      "当前关系模式不支持手动创建临时好友，请通过石头或纸船互动建立连接"));
}

void TempFriendController::getMyTempFriends(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt2 = Validator::getUserId(req);
    if (!userIdOpt2) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto &currentUserId = *userIdOpt2;
    auto dbClient = app().getDbClient("default");

    // 查询临时好友列表
    auto querySql =
        std::string("SELECT ") + kTempFriendAliasedColumns +
        ", "
        "CASE "
        "  WHEN tf.user1_id = $1 THEN u2.nickname "
        "  ELSE u1.nickname "
        "END as friend_nickname, "
        "CASE "
        "  WHEN tf.user1_id = $1 THEN u2.user_id "
        "  ELSE u1.user_id "
        "END as friend_user_id, "
        "CASE "
        "  WHEN tf.user1_id = $1 THEN u2.avatar_url "
        "  ELSE u1.avatar_url "
        "END as friend_avatar, "
        "EXTRACT(EPOCH FROM (tf.expires_at - NOW())) as seconds_remaining "
        "FROM temp_friends tf "
        "LEFT JOIN users u1 ON tf.user1_id = u1.user_id "
        "LEFT JOIN users u2 ON tf.user2_id = u2.user_id "
        "WHERE (tf.user1_id = $1 OR tf.user2_id = $1) "
        "AND tf.status = 'active' "
        "AND tf.expires_at >= NOW() "
        "ORDER BY tf.created_at DESC";

    auto result = dbClient->execSqlSync(querySql, currentUserId);

    Json::Value friends(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value friend_;
      friend_["temp_friend_id"] = row["temp_friend_id"].as<std::string>();
      friend_["friend_user_id"] = row["friend_user_id"].as<std::string>();
      friend_["friend_id"] = row["friend_user_id"].as<std::string>();
      friend_["friend_nickname"] =
          row["friend_nickname"].isNull()
              ? "匿名用户"
              : row["friend_nickname"].as<std::string>();
      friend_["friend_avatar"] = row["friend_avatar"].isNull()
                                     ? ""
                                     : row["friend_avatar"].as<std::string>();
      friend_["source"] = row["source"].as<std::string>();
      friend_["created_at"] = row["created_at"].as<std::string>();
      friend_["expires_at"] = row["expires_at"].as<std::string>();
      int secRemaining = readRemainingSeconds(row);
      friend_["seconds_remaining"] = secRemaining;
      friend_["hours_remaining"] = remainingHoursFromSeconds(secRemaining);

      friends.append(friend_);
    }

    const int total = static_cast<int>(result.size());
    Json::Value data = ResponseUtil::buildCollectionPayload(
        "temp_friends", friends, total, 1, total > 0 ? total : 1);
    data["friends"] = friends;
    callback(ResponseUtil::success(data, "获取成功"));

  } catch (const std::exception &e) {
    LOG_ERROR << "获取临时好友列表异常: " << e.what();
    callback(ResponseUtil::internalError("获取临时好友列表失败"));
  }
}

void TempFriendController::getTempFriendDetail(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &tempFriendId) {
  try {
    auto userIdOpt4 = Validator::getUserId(req);
    if (!userIdOpt4) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto &currentUserId = *userIdOpt4;
    auto dbClient = app().getDbClient("default");

    auto querySql =
        std::string("SELECT ") + kTempFriendAliasedColumns +
        ", "
        "CASE "
        "  WHEN tf.status = 'active' AND tf.expires_at < NOW() THEN 'expired' "
        "  ELSE tf.status "
        "END AS effective_status, "
        "u1.nickname as user1_nickname, u1.avatar_url as user1_avatar, "
        "u2.nickname as user2_nickname, u2.avatar_url as user2_avatar, "
        "EXTRACT(EPOCH FROM (tf.expires_at - NOW())) as seconds_remaining "
        "FROM temp_friends tf "
        "LEFT JOIN users u1 ON tf.user1_id = u1.user_id "
        "LEFT JOIN users u2 ON tf.user2_id = u2.user_id "
        "WHERE tf.temp_friend_id = $1 "
        "AND (tf.user1_id = $2 OR tf.user2_id = $2)";

    auto result = dbClient->execSqlSync(querySql, tempFriendId, currentUserId);

    if (result.size() == 0) {
      callback(ResponseUtil::notFound("临时好友不存在"));
      return;
    }

    auto row = *safeRow(result);
    Json::Value data;
    data["temp_friend_id"] = row["temp_friend_id"].as<std::string>();
    data["status"] = row["effective_status"].as<std::string>();
    data["source"] = row["source"].as<std::string>();
    data["created_at"] = row["created_at"].as<std::string>();
    data["expires_at"] = row["expires_at"].as<std::string>();
    int secRemaining = readRemainingSeconds(row);
    data["seconds_remaining"] = secRemaining;
    data["hours_remaining"] = remainingHoursFromSeconds(secRemaining);
    data["upgraded_to_friend"] = row["upgraded_to_friend"].as<bool>();

    // 判断对方是谁
    std::string friendUserId, friendNickname, friendAvatar;
    if (row["user1_id"].as<std::string>() == currentUserId) {
      friendUserId = row["user2_id"].as<std::string>();
      friendNickname = row["user2_nickname"].isNull()
                           ? "匿名用户"
                           : row["user2_nickname"].as<std::string>();
      friendAvatar = row["user2_avatar"].isNull()
                         ? ""
                         : row["user2_avatar"].as<std::string>();
    } else {
      friendUserId = row["user1_id"].as<std::string>();
      friendNickname = row["user1_nickname"].isNull()
                           ? "匿名用户"
                           : row["user1_nickname"].as<std::string>();
      friendAvatar = row["user1_avatar"].isNull()
                         ? ""
                         : row["user1_avatar"].as<std::string>();
    }

    data["friend_user_id"] = friendUserId;
    data["friend_id"] = friendUserId;
    data["friend_nickname"] = friendNickname;
    data["friend_avatar"] = friendAvatar;
    callback(ResponseUtil::success(data, "获取成功"));

  } catch (const std::exception &e) {
    LOG_ERROR << "获取临时好友详情异常: " << e.what();
    callback(ResponseUtil::internalError("获取临时好友详情失败"));
  }
}

void TempFriendController::upgradeToPermanent(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &tempFriendId) {
  (void)tempFriendId;
  if (!Validator::getUserId(req)) {
    callback(ResponseUtil::unauthorized("未登录"));
    return;
  }
  callback(ResponseUtil::badRequest("当前关系模式不支持手动升级永久好友"));
}

void TempFriendController::deleteTempFriend(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &tempFriendId) {
  try {
    auto userIdOpt5 = Validator::getUserId(req);
    if (!userIdOpt5) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto &currentUserId = *userIdOpt5;
    auto dbClient = app().getDbClient("default");

    // 更新状态为expired而不是删除
    auto updateSql = "UPDATE temp_friends "
                     "SET status = 'expired' "
                     "WHERE temp_friend_id = $1 "
                     "AND (user1_id = $2 OR user2_id = $2) "
                     "AND status = 'active' "
                     "AND expires_at >= NOW() "
                     "RETURNING temp_friend_id, user1_id, user2_id";

    auto result = dbClient->execSqlSync(updateSql, tempFriendId, currentUserId);

    if (!result.empty()) {
      Json::Value data;
      data["temp_friend_id"] = result[0]["temp_friend_id"].as<std::string>();
      data["status"] = "expired";
      callback(ResponseUtil::success(data, "临时好友已删除"));

      Json::Value extra(Json::objectValue);
      extra["status"] = "expired";
      extra["triggered_by"] = currentUserId;
      emitTempFriendLifecycleEvent(
          "temp_friend_expired", result[0]["temp_friend_id"].as<std::string>(),
          result[0]["user1_id"].as<std::string>(),
          result[0]["user2_id"].as<std::string>(), extra);
    } else {
      callback(ResponseUtil::notFound("临时好友不存在或已失效"));
    }

  } catch (const std::exception &e) {
    LOG_ERROR << "删除临时好友异常: " << e.what();
    callback(ResponseUtil::internalError("删除临时好友失败"));
  }
}

void TempFriendController::checkTempFriendStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &targetUserId) {
  try {
    auto userIdOpt6 = Validator::getUserId(req);
    if (!userIdOpt6) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto &currentUserId = *userIdOpt6;
    auto dbClient = app().getDbClient("default");

    // 检查临时好友关系
    auto querySql =
        std::string("SELECT ") + kTempFriendAliasedColumns +
        ", "
        "EXTRACT(EPOCH FROM (tf.expires_at - NOW())) as seconds_remaining "
        "FROM temp_friends tf "
        "WHERE ((tf.user1_id = $1 AND tf.user2_id = $2) "
        "OR (tf.user1_id = $2 AND tf.user2_id = $1)) "
        "AND tf.status = 'active' "
        "AND tf.expires_at >= NOW()";

    auto result = dbClient->execSqlSync(querySql, currentUserId, targetUserId);

    Json::Value data(Json::objectValue);
    if (auto rowOpt = safeRow(result)) {
      auto row = *rowOpt;
      data["is_temp_friend"] = true;
      data["temp_friend_id"] = row["temp_friend_id"].as<std::string>();
      data["expires_at"] = row["expires_at"].as<std::string>();
      int secRemaining = readRemainingSeconds(row);
      data["seconds_remaining"] = secRemaining;
      data["hours_remaining"] = remainingHoursFromSeconds(secRemaining);
    } else {
      data["is_temp_friend"] = false;
    }
    callback(ResponseUtil::success(data));

  } catch (const std::exception &e) {
    LOG_ERROR << "检查临时好友状态异常: " << e.what();
    callback(ResponseUtil::internalError("检查临时好友状态失败"));
  }
}
