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

std::string normalizeUpperToken(const std::string &raw) {
  auto token = trimAscii(raw);
  std::transform(token.begin(), token.end(), token.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::toupper(c));
                 });
  return token;
}

bool hasCompatibleConfirmation(const Json::Value *json,
                               std::initializer_list<const char *> accepted) {
  if (json == nullptr || !json->isMember("confirmation")) {
    return true;
  }
  if (!(*json)["confirmation"].isString()) {
    return false;
  }

  const auto confirmation = normalizeUpperToken((*json)["confirmation"].asString());
  return std::any_of(accepted.begin(), accepted.end(),
                     [&](const char *candidate) {
                       return confirmation == candidate;
                     });
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

int extractWindowTotal(const drogon::orm::Result &result) {
  if (result.empty() || result[0]["total_count"].isNull()) {
    return 0;
  }
  return result[0]["total_count"].as<int>();
}

int resolveWindowTotalOrFallbackCount(
    const drogon::orm::DbClientPtr &dbClient, const drogon::orm::Result &result,
    int page, const std::string &countSql, const std::string &userId) {
  const int total = extractWindowTotal(result);
  if (!result.empty() || page <= 1) {
    return total;
  }
  return safeCount(dbClient->execSqlSync(countSql, userId));
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
                              "bio, gender, birthday, location, "
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
    data["location"] =
        row["location"].isNull() ? "" : row["location"].as<std::string>();
    data["email"] = row["email"].isNull() ? "" : row["email"].as<std::string>();
    data["status"] =
        row["status"].isNull() ? "active" : row["status"].as<std::string>();
    // BUG-FIX: created_at 可能为 NULL，添加空值保护
    data["created_at"] =
        row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    data["last_active_at"] = row["last_active_at"].isNull()
                                 ? ""
                                 : row["last_active_at"].as<std::string>();

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
    if (!json || (!json->isMember("avatar_url") && !json->isMember("avatar"))) {
      callback(ResponseUtil::badRequest("缺少avatar_url参数"));
      return;
    }

    const auto &avatarNode =
        json->isMember("avatar_url") ? (*json)["avatar_url"] : (*json)["avatar"];
    if (!avatarNode.isString()) {
      callback(ResponseUtil::badRequest("头像地址格式不正确"));
      return;
    }

    std::string avatarUrl = trimAscii(avatarNode.asString());
    if (!avatarUrl.empty()) {
      auto avatarValidation = Validator::url(avatarUrl, "头像地址");
      if (!avatarValidation) {
        callback(ResponseUtil::badRequest(avatarValidation.errorMessage));
        return;
      }
    }
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
    if (json->isMember("avatar_url") || json->isMember("avatar")) {
      const auto &avatarNode = json->isMember("avatar_url")
                                   ? (*json)["avatar_url"]
                                   : (*json)["avatar"];
      if (!avatarNode.isString()) {
        callback(ResponseUtil::badRequest("头像地址格式不正确"));
        return;
      }
      std::string avatarUrl = trimAscii(avatarNode.asString());
      if (!avatarUrl.empty()) {
        auto avatarValidation = Validator::url(avatarUrl, "头像地址");
        if (!avatarValidation) {
          callback(ResponseUtil::badRequest(avatarValidation.errorMessage));
          return;
        }
      }
      updates.push_back("avatar_url = NULLIF($" + std::to_string(paramIndex++) +
                        ", '')");
      params.push_back(avatarUrl);
    }
    if (json->isMember("gender")) {
      if (!(*json)["gender"].isString()) {
        callback(ResponseUtil::badRequest("性别格式不正确"));
        return;
      }
      std::string gender = trimAscii((*json)["gender"].asString());
      auto genderValidation = Validator::length(gender, 0, 32, "性别");
      if (!genderValidation) {
        callback(ResponseUtil::badRequest(genderValidation.errorMessage));
        return;
      }
      updates.push_back("gender = NULLIF($" + std::to_string(paramIndex++) +
                        ", '')");
      params.push_back(gender);
    }
    if (json->isMember("birthday")) {
      if (!(*json)["birthday"].isString()) {
        callback(ResponseUtil::badRequest("生日格式不正确"));
        return;
      }
      updates.push_back("birthday = NULLIF($" + std::to_string(paramIndex++) +
                        ", '')::date");
      params.push_back(trimAscii((*json)["birthday"].asString()));
    }
    if (json->isMember("location")) {
      if (!(*json)["location"].isString()) {
        callback(ResponseUtil::badRequest("地区格式不正确"));
        return;
      }
      std::string location = trimAscii((*json)["location"].asString());
      auto locationValidation = Validator::length(location, 0, 128, "地区");
      if (!locationValidation) {
        callback(ResponseUtil::badRequest(locationValidation.errorMessage));
        return;
      }
      updates.push_back("location = NULLIF($" + std::to_string(paramIndex++) +
                        ", '')");
      params.push_back(location);
    }
    if (json->isMember("email")) {
      if (!(*json)["email"].isString()) {
        callback(ResponseUtil::badRequest("邮箱格式不正确"));
        return;
      }
      std::string email = trimAscii((*json)["email"].asString());
      if (!email.empty()) {
        auto emailValidation = Validator::email(email);
        if (!emailValidation) {
          callback(ResponseUtil::badRequest(emailValidation.errorMessage));
          return;
        }
      }
      updates.push_back("email = NULLIF($" + std::to_string(paramIndex++) +
                        ", '')");
      params.push_back(email);
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
    } else if (params.size() == 5) {
      dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3],
                            params[4], userId);
    } else if (params.size() == 6) {
      dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3],
                            params[4], params[5], userId);
    } else if (params.size() == 7) {
      dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3],
                            params[4], params[5], params[6], userId);
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

    auto statsResult = dbClient->execSqlSync(
        "SELECT "
        "COALESCE(st.stone_count, 0) AS stone_count, "
        "COALESCE(fr.friend_count, 0) AS friend_count, "
        "COALESCE(rp.ripple_received, 0) AS ripple_received "
        "FROM users u "
        "LEFT JOIN LATERAL ("
        "  SELECT COUNT(*) FILTER (WHERE s.deleted_at IS NULL) AS stone_count "
        "  FROM stones s "
        "  WHERE s.user_id = u.user_id"
        ") st ON TRUE "
        "LEFT JOIN LATERAL ("
        "  SELECT COUNT(*) AS friend_count "
        "  FROM friends f "
        "  WHERE (f.user_id = u.user_id OR f.friend_id = u.user_id) "
        "    AND f.status = 'accepted'"
        ") fr ON TRUE "
        "LEFT JOIN LATERAL ("
        "  SELECT COUNT(*) AS ripple_received "
        "  FROM ripples r "
        "  JOIN stones s ON r.stone_id = s.stone_id "
        "  WHERE s.user_id = u.user_id AND s.deleted_at IS NULL"
        ") rp ON TRUE "
        "WHERE u.user_id = $1",
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
    const auto [page, pageSize] = safePagination(req);
    const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

    auto result = dbClient->execSqlSync(
        "SELECT session_id, device_type, device_name, ip_address, "
        "last_active_at, created_at, COUNT(*) OVER() AS total_count "
        "FROM user_sessions WHERE user_id = $1 AND is_active = true "
        "ORDER BY COALESCE(last_active_at, created_at) DESC "
        "LIMIT $2 OFFSET $3",
        userId, static_cast<int64_t>(pageSize), offset);

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

    const int total = resolveWindowTotalOrFallbackCount(
        dbClient, result, page,
        "SELECT COUNT(*) AS total FROM user_sessions "
        "WHERE user_id = $1 AND is_active = true",
        userId);

    callback(ResponseUtil::success(ResponseUtil::buildCollectionPayload(
        "devices", devices, total, page, pageSize)));
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

    // 直接删除会话记录，避免移除设备后仍出现在设备列表中
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
    const auto [page, pageSize] = safePagination(req);
    const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

    auto result = dbClient->execSqlSync(
        "SELECT log_id, login_time, ip_address, device_type, location, success, "
        "COUNT(*) OVER() AS total_count "
        "FROM login_logs WHERE user_id = $1 "
        "ORDER BY login_time DESC LIMIT $2 OFFSET $3",
        userId, static_cast<int64_t>(pageSize), offset);

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

    const int total = resolveWindowTotalOrFallbackCount(
        dbClient, result, page,
        "SELECT COUNT(*) AS total FROM login_logs WHERE user_id = $1", userId);

    callback(ResponseUtil::success(ResponseUtil::buildCollectionPayload(
        "logs", logs, total, page, pageSize)));
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
    const auto [page, pageSize] = safePagination(req);
    const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

    auto result = dbClient->execSqlSync(
        "SELECT event_id, event_type, description, ip_address, created_at, "
        "COUNT(*) OVER() AS total_count "
        "FROM security_events WHERE user_id = $1 "
        "ORDER BY created_at DESC LIMIT $2 OFFSET $3",
        userId, static_cast<int64_t>(pageSize), offset);

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

    const int total = resolveWindowTotalOrFallbackCount(
        dbClient, result, page,
        "SELECT COUNT(*) AS total FROM security_events WHERE user_id = $1",
        userId);

    callback(ResponseUtil::success(ResponseUtil::buildCollectionPayload(
        "events", events, total, page, pageSize)));
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

    auto result = dbClient->execSqlSync(
        "SELECT profile_visibility, show_online_status, allow_friend_request, "
        "allow_message_from_stranger FROM user_privacy_settings WHERE user_id = $1",
        userId);

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
    auto [page, pageSize] = safePagination(req);
    int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

    auto result = dbClient->execSqlSync(
        "SELECT b.blocked_user_id, u.nickname, u.avatar_url, b.created_at, "
        "COUNT(*) OVER() AS total_count "
        "FROM user_blocks b JOIN users u ON b.blocked_user_id = u.user_id "
        "WHERE b.user_id = $1 ORDER BY b.created_at DESC LIMIT $2 OFFSET $3",
        userId, pageSize, offset);

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

    const int total = resolveWindowTotalOrFallbackCount(
        dbClient, result, page,
        "SELECT COUNT(*) AS total FROM user_blocks WHERE user_id = $1",
        userId);

    callback(ResponseUtil::success(ResponseUtil::buildCollectionPayload(
        "blocked_users", users, total, page, pageSize)));
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
    auto result = dbClient->execSqlSync(
        "WITH target AS ("
        "  SELECT user_id FROM users WHERE user_id = $2"
        "), inserted AS ("
        "  INSERT INTO user_blocks (user_id, blocked_user_id) "
        "  SELECT $1, user_id FROM target "
        "  ON CONFLICT DO NOTHING "
        "  RETURNING blocked_user_id"
        ") "
        "SELECT EXISTS(SELECT 1 FROM target) AS target_exists, "
        "       EXISTS(SELECT 1 FROM inserted) AS inserted",
        userId, targetUserId);

    if (result.empty() || result[0]["target_exists"].isNull() ||
        !result[0]["target_exists"].as<bool>()) {
      callback(ResponseUtil::notFound("用户不存在"));
      return;
    }

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

    // 使用 Drogon 异步调度，避免每个导出请求都创建裸线程。
    drogon::async_run([taskId, userId]() -> drogon::Task<void> {
      heartlake::infrastructure::DataExportService::getInstance()
          .processExportTask(taskId, userId);
      co_return;
    });

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
    if (!hasCompatibleConfirmation(json.get(), {"DEACTIVATE", "DELETE"})) {
      callback(ResponseUtil::badRequest(
          "confirmation 仅支持 'DEACTIVATE' 或兼容旧值 'DELETE'"));
      return;
    }

    auto dbClient = app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "UPDATE users "
        "SET status = 'deactivated', updated_at = NOW() "
        "WHERE user_id = $1 AND status <> 'deleted' "
        "RETURNING user_id",
        userId);
    if (result.empty()) {
      callback(ResponseUtil::notFound("用户不存在或已永久删除"));
      return;
    }

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
    if (!hasCompatibleConfirmation(json.get(), {"DELETE"})) {
      callback(ResponseUtil::badRequest("confirmation 仅支持 'DELETE'"));
      return;
    }

    auto dbClient = app().getDbClient("default");
    auto trans = dbClient->newTransaction();

    // 按依赖顺序删除关联数据
    trans->execSqlSync("DELETE FROM notifications WHERE user_id = $1", userId);
    trans->execSqlSync(
        "DELETE FROM user_blocks WHERE user_id = $1 OR blocked_user_id = $1",
        userId);
    trans->execSqlSync("DELETE FROM resonance_points WHERE user_id = $1",
                       userId);
    trans->execSqlSync(
        "DELETE FROM lamp_transfers WHERE from_user_id = $1 OR to_user_id = $1",
        userId);
    trans->execSqlSync("DELETE FROM vip_upgrade_logs WHERE user_id = $1",
                       userId);
    trans->execSqlSync(
        "DELETE FROM user_interaction_history WHERE user_id = $1", userId);
    trans->execSqlSync(
        "DELETE FROM consultation_sessions "
        "WHERE user_id = $1 OR counselor_id = $1",
        userId);
    trans->execSqlSync("DELETE FROM connection_messages WHERE connection_id IN "
                       "(SELECT connection_id FROM connections WHERE user_id "
                       "= $1 OR target_user_id = $1)",
                       userId);
    trans->execSqlSync(
        "DELETE FROM connections WHERE user_id = $1 OR target_user_id = $1",
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
        "DELETE FROM paper_boats WHERE sender_id = $1 OR receiver_id = $1",
        userId);
    trans->execSqlSync("DELETE FROM ripples WHERE user_id = $1", userId);
    trans->execSqlSync("DELETE FROM stone_embeddings WHERE stone_id IN (SELECT "
                       "stone_id FROM stones WHERE user_id = $1)",
                       userId);
    trans->execSqlSync("DELETE FROM stones WHERE user_id = $1", userId);
    trans->execSqlSync(
        "DELETE FROM reports WHERE reporter_id = $1 OR (target_type = 'user' "
        "AND target_id = $1)",
        userId);
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
        "DELETE FROM user_similarity WHERE user1_id = $1 OR user2_id = $1",
        userId);
    // 最后删除用户记录，并对不存在目标返回明确错误而不是假成功
    auto deletedUser = trans->execSqlSync(
        "DELETE FROM users WHERE user_id = $1 RETURNING user_id", userId);
    if (deletedUser.empty()) {
      callback(ResponseUtil::notFound("用户不存在"));
      return;
    }

    callback(ResponseUtil::success(Json::Value(), "账号已永久删除"));
  } catch (const std::exception &e) {
    LOG_ERROR << "deleteAccountPermanently error: " << e.what();
    callback(ResponseUtil::internalError());
  }
}
