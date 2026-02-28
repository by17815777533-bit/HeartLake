#include "interfaces/api/AccountController.h"
#include "infrastructure/services/DataExportService.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/SecurityLogger.h"
#include "utils/Validator.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <cctype>
#include <thread>

using namespace heartlake::controllers;
using namespace heartlake::utils;

namespace {
std::string trimAscii(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }

  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(start, end - start);
}

std::string normalizeNickname(const std::string &raw) {
  return Validator::sanitizeHtml(trimAscii(raw));
}

void ensureUserPrivacySettingsTable(const drogon::orm::DbClientPtr &dbClient) {
  dbClient->execSqlSync(
      "CREATE TABLE IF NOT EXISTS user_privacy_settings ("
      "user_id VARCHAR(64) PRIMARY KEY,"
      "profile_visibility VARCHAR(16) NOT NULL DEFAULT 'public',"
      "show_online_status BOOLEAN NOT NULL DEFAULT true,"
      "allow_friend_request BOOLEAN NOT NULL DEFAULT true,"
      "allow_message_from_stranger BOOLEAN NOT NULL DEFAULT false"
      ")");
}

bool parseBoolCompat(const Json::Value &json, const char *key,
                     bool defaultValue) {
  if (!json.isMember(key)) return defaultValue;
  const auto &v = json[key];
  if (v.isBool()) return v.asBool();
  if (v.isInt() || v.isUInt()) return v.asInt() != 0;
  if (v.isString()) {
    auto s = v.asString();
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (s == "true" || s == "1" || s == "yes" || s == "on") return true;
    if (s == "false" || s == "0" || s == "no" || s == "off") return false;
  }
  return defaultValue;
}

std::string parseVisibilityCompat(const Json::Value &json) {
  std::string visibility = "public";
  if (json.isMember("profile_visibility")) {
    const auto &value = json["profile_visibility"];
    if (value.isString()) {
      visibility = value.asString();
    } else if (value.isBool()) {
      visibility = value.asBool() ? "public" : "private";
    } else if (value.isInt() || value.isUInt()) {
      visibility = value.asInt() == 0 ? "private" : "public";
    }
  }

  std::transform(visibility.begin(), visibility.end(), visibility.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  if (visibility != "public" && visibility != "private" &&
      visibility != "friends") {
    return "public";
  }
  return visibility;
}
} // namespace

// ==================== 个人信息管理 ====================

void AccountController::getAccountInfo(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    auto result =
        dbClient->execSqlSync("SELECT user_id, username, nickname, avatar_url, "
                              "bio, gender, birthday, "
                              "email, status, created_at, last_active_at "
                              "FROM users WHERE user_id = $1",
                              userId);

    if (result.empty()) {
      callback(ResponseUtil::notFound("用户不存在"));
      return;
    }

    auto row = *safeRow(result);
    Json::Value data;
    data["user_id"] = row["user_id"].as<std::string>();
    data["username"] =
        row["username"].isNull() ? "" : row["username"].as<std::string>();
    data["nickname"] =
        row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
    data["avatar_url"] =
        row["avatar_url"].isNull() ? "" : row["avatar_url"].as<std::string>();
    data["bio"] = row["bio"].isNull() ? "" : row["bio"].as<std::string>();
    data["gender"] =
        row["gender"].isNull() ? "" : row["gender"].as<std::string>();
    data["birthday"] =
        row["birthday"].isNull() ? "" : row["birthday"].as<std::string>();
    data["email"] = row["email"].isNull() ? "" : row["email"].as<std::string>();
    data["status"] =
        row["status"].isNull() ? "active" : row["status"].as<std::string>();
    // BUG-FIX: created_at 可能为 NULL，添加空值保护
    data["created_at"] =
        row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();

    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getAccountInfo error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::updateAvatar(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("用户未认证"));
      return;
    }
    auto userId = *userIdOpt;
    auto json = req->getJsonObject();
    if (!json || !json->isMember("avatar_url")) {
      callback(ResponseUtil::badRequest("缺少avatar_url参数"));
      return;
    }

    std::string avatarUrl = (*json)["avatar_url"].asString();
    auto dbClient = app().getDbClient("default");

    dbClient->execSqlSync("UPDATE users SET avatar_url = $1 WHERE user_id = $2",
                          avatarUrl, userId);

    Json::Value data;
    data["avatar_url"] = avatarUrl;
    callback(ResponseUtil::success(data, "头像更新成功"));
  } catch (const std::exception &e) {
    LOG_ERROR << "updateAvatar error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::updateProfile(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
      return;
    }

    auto dbClient = app().getDbClient("default");
    std::vector<std::string> updates;
    std::vector<std::string> params;
    int paramIndex = 1;

    if (json->isMember("nickname")) {
      if (!(*json)["nickname"].isString()) {
        callback(ResponseUtil::badRequest("昵称格式不正确"));
        return;
      }
      std::string nickname = normalizeNickname((*json)["nickname"].asString());
      auto nicknameValidation = ValidationRules::nickname(nickname);
      if (!nicknameValidation) {
        callback(ResponseUtil::badRequest(nicknameValidation.errorMessage));
        return;
      }
      updates.push_back("nickname = $" + std::to_string(paramIndex++));
      params.push_back(nickname);
    }
    if (json->isMember("bio")) {
      if (!(*json)["bio"].isString()) {
        callback(ResponseUtil::badRequest("个性签名格式不正确"));
        return;
      }
      std::string bio = Validator::sanitizeHtml((*json)["bio"].asString());
      auto bioValidation = Validator::length(bio, 0, 200, "个性签名");
      if (!bioValidation) {
        callback(ResponseUtil::badRequest(bioValidation.errorMessage));
        return;
      }
      updates.push_back("bio = $" + std::to_string(paramIndex++));
      params.push_back(bio);
    }
    if (json->isMember("gender")) {
      updates.push_back("gender = $" + std::to_string(paramIndex++));
      params.push_back((*json)["gender"].asString());
    }
    if (json->isMember("birthday")) {
      updates.push_back("birthday = $" + std::to_string(paramIndex++));
      params.push_back((*json)["birthday"].asString());
    }

    if (updates.empty()) {
      callback(ResponseUtil::badRequest("没有要更新的字段"));
      return;
    }

    std::string sql = "UPDATE users SET " + updates[0];
    for (size_t i = 1; i < updates.size(); ++i) {
      sql += ", " + updates[i];
    }
    sql += " WHERE user_id = $" + std::to_string(paramIndex);

    // 动态执行SQL
    if (params.size() == 1) {
      dbClient->execSqlSync(sql, params[0], userId);
    } else if (params.size() == 2) {
      dbClient->execSqlSync(sql, params[0], params[1], userId);
    } else if (params.size() == 3) {
      dbClient->execSqlSync(sql, params[0], params[1], params[2], userId);
    } else if (params.size() == 4) {
      dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3],
                            userId);
    }

    callback(ResponseUtil::success(Json::Value(), "资料更新成功"));
  } catch (const std::exception &e) {
    LOG_ERROR << "updateProfile error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::getAccountStats(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免 attributes
    // 不存在时抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    Json::Value data;

    // 合并 3 个 COUNT 查询为 1 个子查询，消除 N+1 问题
    auto statsResult = dbClient->execSqlSync(
        "SELECT "
        "(SELECT COUNT(*) FROM stones WHERE user_id = $1) AS stone_count, "
        "(SELECT COUNT(*) FROM friends WHERE (user_id = $1 OR friend_id = $1) "
        "AND status = 'accepted') AS friend_count, "
        "(SELECT COUNT(*) FROM ripples r JOIN stones s ON r.stone_id = "
        "s.stone_id WHERE s.user_id = $1) AS ripple_received",
        userId);
    data["stone_count"] = statsResult[0]["stone_count"].as<int>();
    data["friend_count"] = statsResult[0]["friend_count"].as<int>();
    data["ripple_received"] = statsResult[0]["ripple_received"].as<int>();

    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getAccountStats error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

// ==================== 账号安全 ====================

void AccountController::getLoginDevices(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT session_id, device_type, device_name, ip_address, "
        "last_active_at, created_at "
        "FROM user_sessions WHERE user_id = $1 AND is_active = true "
        "ORDER BY created_at DESC",
        userId);

    Json::Value devices(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value device;
      device["session_id"] = row["session_id"].as<std::string>();
      device["device_type"] = row["device_type"].isNull()
                                  ? "unknown"
                                  : row["device_type"].as<std::string>();
      device["device_name"] = row["device_name"].isNull()
                                  ? "未知设备"
                                  : row["device_name"].as<std::string>();
      device["ip_address"] =
          row["ip_address"].isNull() ? "" : row["ip_address"].as<std::string>();
      device["last_active_at"] = row["last_active_at"].isNull()
                                     ? ""
                                     : row["last_active_at"].as<std::string>();
      devices.append(device);
    }

    Json::Value data;
    data["devices"] = devices;
    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getLoginDevices error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::removeDevice(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &sessionId) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    // BUG-FIX: user_sessions 表没有 is_active 列，改用 DELETE 删除会话记录
    auto result =
        dbClient->execSqlSync("DELETE FROM user_sessions "
                              "WHERE session_id = $1 AND user_id = $2",
                              sessionId, userId);

    if (result.affectedRows() > 0) {
      callback(ResponseUtil::success(Json::Value(), "设备已移除"));
    } else {
      callback(ResponseUtil::notFound("设备不存在"));
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "removeDevice error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::getLoginLogs(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT log_id, login_time, ip_address, device_type, location, success "
        "FROM login_logs WHERE user_id = $1 ORDER BY login_time DESC LIMIT 50",
        userId);

    Json::Value logs(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value log;
      log["log_id"] = row["log_id"].as<std::string>();
      log["login_time"] = row["login_time"].as<std::string>();
      log["ip_address"] =
          row["ip_address"].isNull() ? "" : row["ip_address"].as<std::string>();
      log["device_type"] = row["device_type"].isNull()
                               ? ""
                               : row["device_type"].as<std::string>();
      log["location"] =
          row["location"].isNull() ? "" : row["location"].as<std::string>();
      log["success"] = row["success"].as<bool>();
      logs.append(log);
    }

    Json::Value data;
    data["logs"] = logs;
    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getLoginLogs error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::getSecurityEvents(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT event_id, event_type, description, ip_address, created_at "
        "FROM security_events WHERE user_id = $1 ORDER BY created_at DESC "
        "LIMIT 50",
        userId);

    Json::Value events(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value event;
      event["event_id"] = row["event_id"].as<std::string>();
      event["event_type"] = row["event_type"].as<std::string>();
      event["description"] = row["description"].isNull()
                                 ? ""
                                 : row["description"].as<std::string>();
      event["ip_address"] =
          row["ip_address"].isNull() ? "" : row["ip_address"].as<std::string>();
      event["created_at"] = row["created_at"].as<std::string>();
      events.append(event);
    }

    Json::Value data;
    data["events"] = events;
    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getSecurityEvents error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

// ==================== 隐私设置 ====================

void AccountController::getPrivacySettings(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");
    ensureUserPrivacySettingsTable(dbClient);

    auto result = dbClient->execSqlSync(
        "SELECT * FROM user_privacy_settings WHERE user_id = $1", userId);

    Json::Value data;
    if (result.empty()) {
      // 返回默认设置
      data["profile_visibility"] = "public";
      data["show_online_status"] = true;
      data["allow_friend_request"] = true;
      data["allow_message_from_stranger"] = false;
    } else {
      auto row = *safeRow(result);
      data["profile_visibility"] = row["profile_visibility"].as<std::string>();
      data["show_online_status"] = row["show_online_status"].as<bool>();
      data["allow_friend_request"] = row["allow_friend_request"].as<bool>();
      data["allow_message_from_stranger"] =
          row["allow_message_from_stranger"].as<bool>();
    }

    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getPrivacySettings error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::updatePrivacySettings(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
      return;
    }

    auto dbClient = app().getDbClient("default");
    ensureUserPrivacySettingsTable(dbClient);

    std::string visibility = parseVisibilityCompat(*json);
    bool showOnline = parseBoolCompat(*json, "show_online_status", true);
    bool allowFriend = parseBoolCompat(*json, "allow_friend_request", true);
    // 兼容 allow_stranger_boat 历史字段
    bool allowStranger = parseBoolCompat(*json, "allow_message_from_stranger",
                                         parseBoolCompat(*json, "allow_stranger_boat", false));

    dbClient->execSqlSync(
        "INSERT INTO user_privacy_settings (user_id, profile_visibility, "
        "show_online_status, "
        "allow_friend_request, allow_message_from_stranger) VALUES ($1, $2, "
        "$3, $4, $5) "
        "ON CONFLICT (user_id) DO UPDATE SET profile_visibility = $2, "
        "show_online_status = $3, "
        "allow_friend_request = $4, allow_message_from_stranger = $5",
        userId, visibility, showOnline, allowFriend, allowStranger);

    callback(ResponseUtil::success(Json::Value(), "隐私设置已更新"));
  } catch (const std::exception &e) {
    LOG_ERROR << "updatePrivacySettings error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::getBlockedUsers(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT b.blocked_user_id, u.nickname, u.avatar_url, b.created_at "
        "FROM user_blocks b JOIN users u ON b.blocked_user_id = u.user_id "
        "WHERE b.user_id = $1 ORDER BY b.created_at DESC",
        userId);

    Json::Value users(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value user;
      user["user_id"] = row["blocked_user_id"].as<std::string>();
      user["nickname"] =
          row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
      user["avatar_url"] =
          row["avatar_url"].isNull() ? "" : row["avatar_url"].as<std::string>();
      user["blocked_at"] = row["created_at"].as<std::string>();
      users.append(user);
    }

    Json::Value data;
    data["blocked_users"] = users;
    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getBlockedUsers error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::blockUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &targetUserId) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    if (userId == targetUserId) {
      callback(ResponseUtil::badRequest("不能拉黑自己"));
      return;
    }

    auto dbClient = app().getDbClient("default");

    dbClient->execSqlSync(
        "INSERT INTO user_blocks (user_id, blocked_user_id) VALUES ($1, $2) "
        "ON CONFLICT DO NOTHING",
        userId, targetUserId);

    callback(ResponseUtil::success(Json::Value(), "用户已拉黑"));
  } catch (const std::exception &e) {
    LOG_ERROR << "blockUser error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::unblockUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &targetUserId) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    dbClient->execSqlSync(
        "DELETE FROM user_blocks WHERE user_id = $1 AND blocked_user_id = $2",
        userId, targetUserId);

    callback(ResponseUtil::success(Json::Value(), "已取消拉黑"));
  } catch (const std::exception &e) {
    LOG_ERROR << "unblockUser error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

// ==================== 数据管理 ====================

void AccountController::exportData(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    std::string taskId = IdGenerator::generateUserId();

    // 创建导出任务
    dbClient->execSqlSync("INSERT INTO data_export_tasks (task_id, user_id, "
                          "status) VALUES ($1, $2, 'pending')",
                          taskId, userId);

    // 异步执行导出任务
    std::thread([taskId, userId]() {
      heartlake::infrastructure::DataExportService::getInstance()
          .processExportTask(taskId, userId);
    }).detach();

    Json::Value data;
    data["task_id"] = taskId;
    data["status"] = "pending";
    data["message"] = "数据导出任务已创建，请稍后查询状态";

    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "exportData error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::getExportStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &taskId) {
  try {
    // BUG-FIX: 使用安全的 safeGetUserId 替代直接 get，避免抛异常导致 500
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto dbClient = app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT task_id, status, download_url, created_at, completed_at "
        "FROM data_export_tasks WHERE task_id = $1 AND user_id = $2",
        taskId, userId);

    if (result.empty()) {
      callback(ResponseUtil::notFound("导出任务不存在"));
      return;
    }

    auto row = *safeRow(result);
    Json::Value data;
    data["task_id"] = row["task_id"].as<std::string>();
    data["status"] = row["status"].as<std::string>();
    data["download_url"] = row["download_url"].isNull()
                               ? ""
                               : row["download_url"].as<std::string>();
    data["created_at"] = row["created_at"].as<std::string>();
    data["completed_at"] = row["completed_at"].isNull()
                               ? ""
                               : row["completed_at"].as<std::string>();

    callback(ResponseUtil::success(data));
  } catch (const std::exception &e) {
    LOG_ERROR << "getExportStatus error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::deactivateAccount(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;

    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }
    std::string confirmation = (*json).get("confirmation", "").asString();
    if (confirmation != "DEACTIVATE") {
      callback(ResponseUtil::badRequest("请输入确认文本 'DEACTIVATE'"));
      return;
    }

    auto dbClient = app().getDbClient("default");
    dbClient->execSqlSync(
        "UPDATE users SET status = 'deactivated' WHERE user_id = $1", userId);

    SecurityLogger::logEventFromRequest(
        req, userId, SecurityEventType::ACCOUNT_DELETED, SecuritySeverity::HIGH,
        "用户注销账号（30天内可恢复）");

    callback(ResponseUtil::success(Json::Value(), "账号已注销，30天内可恢复"));
  } catch (const std::exception &e) {
    LOG_ERROR << "deactivateAccount error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void AccountController::deleteAccountPermanently(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto userId = *userIdOpt;
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
      return;
    }

    std::string confirmation = (*json).get("confirmation", "").asString();
    if (confirmation != "DELETE") {
      callback(ResponseUtil::badRequest("请输入确认文本 'DELETE'"));
      return;
    }

    auto dbClient = app().getDbClient("default");
    auto trans = dbClient->newTransaction();

    // 按依赖顺序删除关联数据
    trans->execSqlSync("DELETE FROM notification_reads WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM notifications WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM device_tokens WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM resonance_points WHERE user_id = $1",
                       userId);
    trans->execSqlSync(
        "DELETE FROM lamp_transfers WHERE from_user_id = $1 OR to_user_id = $1",
        userId);
    trans->execSqlSync("DELETE FROM vip_upgrade_logs WHERE user_id = $1",
                       userId);
    trans->execSqlSync(
        "DELETE FROM user_interaction_history WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM user_items WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM emotion_records WHERE user_id = $1",
                       userId);
    trans->execSqlSync(
        "DELETE FROM consultation_appointments WHERE user_id = $1", userId);
    trans->execSqlSync(
        "DELETE FROM consultation_messages WHERE session_id IN (SELECT "
        "session_id FROM consultation_sessions WHERE user_id = $1)",
        userId);
    trans->execSqlSync("DELETE FROM consultation_sessions WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM connection_messages WHERE connection_id IN "
                       "(SELECT connection_id FROM connections WHERE user_id_1 "
                       "= $1 OR user_id_2 = $1)",
                       userId);
    trans->execSqlSync(
        "DELETE FROM connections WHERE user_id_1 = $1 OR user_id_2 = $1",
        userId);
    trans->execSqlSync(
        "DELETE FROM friend_messages WHERE sender_id = $1 OR receiver_id = $1",
        userId);
    trans->execSqlSync(
        "DELETE FROM temp_friends WHERE user1_id = $1 OR user2_id = $1",
        userId);
    trans->execSqlSync(
        "DELETE FROM friends WHERE user_id = $1 OR friend_id = $1", userId);
    trans->execSqlSync(
        "DELETE FROM messages WHERE sender_id = $1 OR receiver_id = $1",
        userId);
    trans->execSqlSync(
        "DELETE FROM conversations WHERE user_id_1 = $1 OR user_id_2 = $1",
        userId);
    trans->execSqlSync("DELETE FROM comments WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM likes WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM paper_boats WHERE sender_id = $1", userId);
    trans->execSqlSync("DELETE FROM ripples WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM stone_embeddings WHERE stone_id IN (SELECT "
                       "stone_id FROM stones WHERE author_id = $1)",
                       userId);
    trans->execSqlSync("DELETE FROM stones WHERE author_id = $1", userId);
    trans->execSqlSync(
        "DELETE FROM warm_boats WHERE sender_id = $1 OR receiver_id = $1",
        userId);
    trans->execSqlSync("DELETE FROM reports WHERE reporter_id = $1", userId);
    trans->execSqlSync("DELETE FROM feedback WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM lake_god_messages WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM user_emotion_history WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM emotion_tracking WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM intervention_log WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM user_followups WHERE user_id = $1", userId);
    trans->execSqlSync(
        "DELETE FROM differential_privacy_budget WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM data_export_tasks WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM user_sessions WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM login_logs WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM security_events WHERE user_id = $1",
                       userId);
    trans->execSqlSync("DELETE FROM user_privacy_settings WHERE user_id = $1",
                       userId);
    trans->execSqlSync(
        "DELETE FROM user_similarity WHERE user_id_1 = $1 OR user_id_2 = $1",
        userId);
    trans->execSqlSync("DELETE FROM operation_logs WHERE operator_id = $1",
                       userId);
    // 最后删除用户记录
    trans->execSqlSync("DELETE FROM users WHERE user_id = $1", userId);

    callback(ResponseUtil::success(Json::Value(), "账号已永久删除"));
  } catch (const std::exception &e) {
    LOG_ERROR << "deleteAccountPermanently error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}
