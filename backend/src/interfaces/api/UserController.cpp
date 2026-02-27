/**
 * 用户控制器 - 匿名登录 + 关键词恢复
 */
#include "interfaces/api/UserController.h"
#include "utils/IdGenerator.h"
#include "utils/PasetoUtil.h"
#include "utils/RecoveryKeyGenerator.h"
#include "utils/ResponseUtil.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>

using namespace heartlake::controllers;
using namespace heartlake::utils;

void UserController::anonymousLogin(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string device_id = (*json)["device_id"].asString();
    if (device_id.empty()) {
      callback(ResponseUtil::badRequest("device_id 不能为空"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT user_id, nickname, is_anonymous FROM users WHERE device_id = "
        "$1 AND status = 'active'",
        device_id);

    Json::Value responseData;
    std::string user_id;
    bool isNewUser = false;

    if (auto rowOpt = safeRow(result)) {
      auto row = *rowOpt;
      user_id = row["user_id"].as<std::string>();

      dbClient->execSqlSync(
          "UPDATE users SET last_active_at = NOW() WHERE user_id = $1",
          user_id);

      responseData["user_id"] = user_id;
      responseData["nickname"] = row["nickname"].as<std::string>();
      responseData["is_anonymous"] = row["is_anonymous"].as<bool>();
      isNewUser = false;
    } else {
      user_id = IdGenerator::generateAnonymousId();
      std::string nickname = IdGenerator::generateNickname();
      const std::string internal_id = user_id;
      const std::string shadow_id = "shadow_" + IdGenerator::generateUUID();

      dbClient->execSqlSync(
          "INSERT INTO users (id, shadow_id, user_id, username, nickname, "
          "device_id, is_anonymous, status, created_at, last_active_at) "
          "VALUES ($1, $2, $3, $4, $5, $6, true, 'active', NOW(), NOW())",
          internal_id, shadow_id, user_id, user_id, nickname, device_id);

      auto recoveryKey = RecoveryKeyGenerator::generate();
      auto recoveryKeyHash = RecoveryKeyGenerator::hash(recoveryKey);

      try {
        dbClient->execSqlSync(
            "UPDATE users SET recovery_key_hash = $1 WHERE user_id = $2",
            recoveryKeyHash, user_id);
      } catch (const drogon::orm::DrogonDbException &e) {
        LOG_WARN << "Failed to persist recovery_key_hash (schema not ready?): " << e.base().what();
      }

      responseData["user_id"] = user_id;
      responseData["nickname"] = nickname;
      responseData["is_anonymous"] = true;
      responseData["recovery_key"] = recoveryKey;
      isNewUser = true;
    }

    std::string key = PasetoUtil::getKey();
    std::string token = PasetoUtil::generateToken(user_id, key, 24);

    responseData["token"] = token;
    responseData["is_new_user"] = isNewUser;
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + 24 * 3600);

    callback(ResponseUtil::success(responseData, "登录成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in anonymousLogin: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in anonymousLogin: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::recoverWithKey(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }

        if (!json->isMember("recovery_key") || (*json)["recovery_key"].asString().empty()) {
            callback(ResponseUtil::badRequest("请提供恢复关键词"));
            return;
        }

        auto recoveryKey = (*json)["recovery_key"].asString();
        std::string deviceId;
        if (json->isMember("device_id")) {
            deviceId = (*json)["device_id"].asString();
        }

        auto dbClient = drogon::app().getDbClient("default");

        // 加盐哈希无法通过SQL等值匹配，需在应用层逐条验证
        // 优先用 device_id 缩小范围；无 device_id 时扫描全部持有恢复密钥的活跃用户
        auto result = !deviceId.empty()
            ? dbClient->execSqlSync(
                "SELECT user_id, nickname, is_anonymous, recovery_key_hash FROM users "
                "WHERE device_id = $1 AND recovery_key_hash IS NOT NULL AND status = 'active'",
                deviceId)
            : dbClient->execSqlSync(
                "SELECT user_id, nickname, is_anonymous, recovery_key_hash FROM users "
                "WHERE recovery_key_hash IS NOT NULL AND status = 'active'");

        if (result.empty()) {
            callback(ResponseUtil::notFound("关键词无效，请检查后重试"));
            return;
        }

        // 逐条验证恢复关键词（加盐哈希无法 SQL 等值匹配）
        size_t matchIdx = result.size(); // 哨兵值，表示未匹配
        for (size_t i = 0; i < result.size(); ++i) {
            auto storedHash = result[i]["recovery_key_hash"].as<std::string>();
            if (RecoveryKeyGenerator::verify(recoveryKey, storedHash)) {
                matchIdx = i;
                break;
            }
        }

        if (matchIdx == result.size()) {
            LOG_WARN << "Recovery attempt with invalid key from device: "
                     << (deviceId.empty() ? "unknown" : deviceId);
            callback(ResponseUtil::notFound("关键词无效，请检查后重试"));
            return;
        }

        auto userId = result[matchIdx]["user_id"].as<std::string>();
        auto nickname = result[matchIdx]["nickname"].as<std::string>();

        dbClient->execSqlSync(
            "UPDATE users SET last_active_at = NOW() WHERE user_id = $1",
            userId);

        std::string key = PasetoUtil::getKey();
        std::string token = PasetoUtil::generateToken(userId, key, 24);

        Json::Value responseData;
        responseData["user_id"] = userId;
        responseData["nickname"] = nickname;
        responseData["token"] = token;
        responseData["is_new_user"] = false;
        responseData["expires_at"] =
            static_cast<Json::Int64>(time(nullptr) + 24 * 3600);

        callback(ResponseUtil::success(responseData, "账号恢复成功"));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in recoverWithKey: " << e.base().what();
        callback(ResponseUtil::internalError("恢复失败，请稍后重试"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in recoverWithKey: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void UserController::refreshToken(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    std::string key = PasetoUtil::getKey();
    std::string token = PasetoUtil::generateToken(user_id, key, 24);

    Json::Value responseData;
    responseData["token"] = token;
    responseData["expires_at"] = static_cast<Json::Int64>(time(nullptr) + 24 * 3600);

    callback(ResponseUtil::success(responseData, "Token刷新成功"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in refreshToken: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::deleteAccount(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string confirmation = (*json).get("confirmation", "").asString();
    if (confirmation != "DELETE") {
      callback(ResponseUtil::badRequest("请输入确认文本 'DELETE'"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    auto transPtr = dbClient->newTransaction();

    try {
      transPtr->execSqlSync("DELETE FROM stones WHERE user_id = $1", user_id);
      transPtr->execSqlSync("DELETE FROM paper_boats WHERE sender_id = $1", user_id);
      transPtr->execSqlSync("DELETE FROM ripples WHERE user_id = $1", user_id);
      transPtr->execSqlSync("DELETE FROM friend_messages WHERE sender_id = $1 OR receiver_id = $1", user_id);
      transPtr->execSqlSync("DELETE FROM friends WHERE user_id = $1 OR friend_id = $1", user_id);
      transPtr->execSqlSync("DELETE FROM temp_friends WHERE user1_id = $1 OR user2_id = $1", user_id);
      transPtr->execSqlSync("DELETE FROM notifications WHERE user_id = $1", user_id);
      transPtr->execSqlSync("DELETE FROM lake_god_messages WHERE user_id = $1", user_id);
      transPtr->execSqlSync("UPDATE users SET status = 'deleted', updated_at = NOW() WHERE user_id = $1", user_id);

      callback(ResponseUtil::success(Json::Value(), "账号已注销"));
    } catch (const std::exception &e) {
      LOG_ERROR << "Transaction error in deleteAccount: " << e.what();
      callback(ResponseUtil::internalError("注销失败"));
    }

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in deleteAccount: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in deleteAccount: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::getUserInfo(
    [[maybe_unused]] const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &userId) {
  try {
    auto dbClient = drogon::app().getDbClient("default");

    auto result =
        dbClient->execSqlSync("SELECT user_id, username, nickname, "
                              "is_anonymous, created_at, avatar_url, bio "
                              "FROM users "
                              "WHERE user_id = $1 AND status = 'active'",
                              userId);

    if (result.empty()) {
      callback(ResponseUtil::notFound("用户不存在"));
      return;
    }

    auto row = *safeRow(result);
    Json::Value user;
    user["user_id"] = row["user_id"].as<std::string>();
    user["username"] = row["username"].as<std::string>();
    user["nickname"] = row["nickname"].as<std::string>();
    user["is_anonymous"] = row["is_anonymous"].as<bool>();
    user["created_at"] = row["created_at"].as<std::string>();
    user["avatar_url"] =
        row["avatar_url"].isNull() ? "" : row["avatar_url"].as<std::string>();
    user["bio"] = row["bio"].isNull() ? "" : row["bio"].as<std::string>();

    auto statsResult =
        dbClient->execSqlSync("SELECT "
                              "  (SELECT COUNT(*) FROM stones WHERE user_id = "
                              "$1 AND status = 'published') as stones_count,"
                              "  (SELECT COALESCE(SUM(ripple_count), 0) FROM "
                              "stones WHERE user_id = $1) as ripples_received,"
                              "  (SELECT COALESCE(SUM(boat_count), 0) FROM "
                              "stones WHERE user_id = $1) as boats_received",
                              userId);

    if (!statsResult.empty()) {
      user["stones_count"] = statsResult[0]["stones_count"].as<int>();
      user["ripples_received"] = statsResult[0]["ripples_received"].as<int>();
      user["boats_received"] = statsResult[0]["boats_received"].as<int>();
    }

    callback(ResponseUtil::success(user));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getUserInfo: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getUserInfo: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::getUserStats(
    const HttpRequestPtr & /*req*/,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &userId) {
  try {
    auto dbClient = drogon::app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT "
        "  (SELECT COUNT(*) FROM stones WHERE user_id = $1 AND status = 'published') as stones_count,"
        "  (SELECT COALESCE(SUM(ripple_count), 0) FROM stones WHERE user_id = $1) as ripples_received,"
        "  (SELECT COALESCE(SUM(boat_count), 0) FROM stones WHERE user_id = $1) as boats_received,"
        "  (SELECT COUNT(*) FROM ripples WHERE user_id = $1) as ripples_sent,"
        "  (SELECT COUNT(*) FROM paper_boats WHERE sender_id = $1) as boats_sent,"
        "  (SELECT DATE_PART('day', NOW() - created_at) FROM users WHERE user_id = $1) as join_days",
        userId);

    if (result.empty()) {
      callback(ResponseUtil::notFound("用户不存在"));
      return;
    }

    auto row = *safeRow(result);
    Json::Value data;
    data["stones_count"] = row["stones_count"].as<int>();
    data["ripples_received"] = row["ripples_received"].as<int>();
    data["boats_received"] = row["boats_received"].as<int>();
    data["ripples_sent"] = row["ripples_sent"].as<int>();
    data["boats_sent"] = row["boats_sent"].as<int>();
    data["join_days"] = row["join_days"].as<int>();

    callback(ResponseUtil::success(data));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getUserStats: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getUserStats: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::searchUsers(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    std::string query = req->getParameter("q");
    if (query.empty()) {
      callback(ResponseUtil::badRequest("搜索关键词不能为空"));
      return;
    }

    auto escapeLike = [](const std::string &input) -> std::string {
      std::string result;
      result.reserve(input.size());
      for (char c : input) {
        if (c == '%' || c == '_' || c == '\\') result += '\\';
        result += c;
      }
      return result;
    };

    auto dbClient = drogon::app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT user_id, username, nickname, is_anonymous, created_at "
        "FROM users "
        "WHERE (username LIKE $1 ESCAPE '\\' OR nickname LIKE $1 ESCAPE '\\') "
        "AND status = 'active' AND user_id != $2 "
        "ORDER BY created_at DESC LIMIT 20",
        "%" + escapeLike(query) + "%", user_id);

    Json::Value users(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value user;
      user["user_id"] = row["user_id"].as<std::string>();
      user["username"] = row["username"].as<std::string>();
      user["nickname"] = row["nickname"].as<std::string>();
      user["is_anonymous"] = row["is_anonymous"].as<bool>();
      users.append(user);
    }

    Json::Value data;
    data["users"] = users;
    data["total"] = static_cast<int>(users.size());

    callback(ResponseUtil::success(data));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in searchUsers: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in searchUsers: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::getMyBoats(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    auto [page, page_size] = safePagination(req);

    auto dbClient = drogon::app().getDbClient("default");

    auto countResult =
        dbClient->execSqlSync("SELECT COUNT(*) as total FROM paper_boats b "
                              "INNER JOIN stones s ON b.stone_id = s.stone_id "
                              "WHERE s.user_id = $1 AND b.sender_id <> $1",
                              user_id);
    int total = safeCount(countResult);

    int64_t offset = static_cast<int64_t>(page - 1) * page_size;
    auto result = dbClient->execSqlSync(
        "SELECT b.boat_id, b.stone_id, b.content, b.boat_style, b.created_at, "
        "b.status, b.sender_id, b.is_anonymous, b.mood, b.response_content, b.response_at "
        "FROM paper_boats b "
        "INNER JOIN stones s ON b.stone_id = s.stone_id "
        "WHERE s.user_id = $1 AND b.sender_id <> $1 "
        "ORDER BY b.created_at DESC LIMIT $2 OFFSET $3",
        user_id, static_cast<int64_t>(page_size), offset);

    Json::Value boats(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value boat;
      boat["boat_id"] = row["boat_id"].as<std::string>();
      boat["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
      boat["content"] = row["content"].as<std::string>();
      boat["boat_style"] = row["boat_style"].isNull() ? "paper" : row["boat_style"].as<std::string>();
      boat["created_at"] = row["created_at"].as<std::string>();
      boat["status"] = row["status"].as<std::string>();
      boat["is_anonymous"] = row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
      boat["mood"] = row["mood"].isNull() ? "" : row["mood"].as<std::string>();
      boat["response_content"] = row["response_content"].isNull() ? "" : row["response_content"].as<std::string>();
      boat["response_at"] = row["response_at"].isNull() ? "" : row["response_at"].as<std::string>();
      boats.append(boat);
    }

    Json::Value data;
    data["items"] = boats;  // 前端兼容字段
    data["boats"] = boats;
    data["total"] = total;
    data["page"] = page;
    data["page_size"] = page_size;
    data["pageSize"] = page_size;
    data["totalPages"] = (total + page_size - 1) / page_size;

    callback(ResponseUtil::success(data));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getMyBoats: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getMyBoats: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::updateNickname(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string nickname = (*json)["nickname"].asString();
    if (nickname.empty() || nickname.length() > 100) {
      callback(ResponseUtil::badRequest("昵称长度必须在1-100个字符之间"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlSync(
        "UPDATE users SET nickname = $1, updated_at = NOW() WHERE user_id = $2",
        nickname, user_id);

    Json::Value responseData;
    responseData["nickname"] = nickname;
    callback(ResponseUtil::success(responseData, "昵称修改成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in updateNickname: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in updateNickname: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::updateProfile(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    std::vector<std::string> setClauses;
    std::vector<std::string> paramValues;
    int paramIdx = 1;

    if (json->isMember("avatar_url") && !(*json)["avatar_url"].asString().empty()) {
      setClauses.push_back("avatar_url = $" + std::to_string(paramIdx++));
      paramValues.push_back((*json)["avatar_url"].asString());
    }
    if (json->isMember("bio") && !(*json)["bio"].asString().empty()) {
      setClauses.push_back("bio = $" + std::to_string(paramIdx++));
      paramValues.push_back((*json)["bio"].asString());
    }
    if (json->isMember("nickname") && !(*json)["nickname"].asString().empty()) {
      setClauses.push_back("nickname = $" + std::to_string(paramIdx++));
      paramValues.push_back((*json)["nickname"].asString());
    }

    if (setClauses.empty()) {
      callback(ResponseUtil::badRequest("没有要更新的字段"));
      return;
    }

    std::string sql = "UPDATE users SET ";
    for (size_t i = 0; i < setClauses.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += setClauses[i];
    }
    sql += ", updated_at = NOW() WHERE user_id = $" + std::to_string(paramIdx);

    if (paramValues.size() == 1) {
      dbClient->execSqlSync(sql, paramValues[0], user_id);
    } else if (paramValues.size() == 2) {
      dbClient->execSqlSync(sql, paramValues[0], paramValues[1], user_id);
    } else if (paramValues.size() == 3) {
      dbClient->execSqlSync(sql, paramValues[0], paramValues[1], paramValues[2], user_id);
    }

    auto result = dbClient->execSqlSync(
        "SELECT user_id, nickname, avatar_url, bio FROM users WHERE user_id = $1",
        user_id);

    if (result.empty()) {
      callback(ResponseUtil::internalError("更新失败"));
      return;
    }

    auto row = *safeRow(result);
    Json::Value responseData;
    responseData["user_id"] = row["user_id"].as<std::string>();
    responseData["nickname"] = row["nickname"].as<std::string>();
    responseData["avatar_url"] = row["avatar_url"].isNull() ? "" : row["avatar_url"].as<std::string>();
    responseData["bio"] = row["bio"].isNull() ? "" : row["bio"].as<std::string>();

    callback(ResponseUtil::success(responseData, "资料更新成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in updateProfile: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in updateProfile: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::getEmotionCalendar(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    std::string month;
    const std::string monthParam = req->getParameter("month");
    const std::string yearParam = req->getParameter("year");

    // 兼容两种传参：
    // 1) month=YYYY-MM
    // 2) year=YYYY & month=MM
    if (!monthParam.empty() && monthParam.size() == 7 && monthParam[4] == '-') {
      month = monthParam;
    } else if (!yearParam.empty() && !monthParam.empty()) {
      int year = safeInt(yearParam, 0);
      int mon = safeInt(monthParam, 0);
      if (year >= 2000 && year <= 2100 && mon >= 1 && mon <= 12) {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) << year
            << "-" << std::setw(2) << mon;
        month = oss.str();
      }
    }

    if (month.empty()) {
      auto now = std::chrono::system_clock::now();
      auto expiryTime = std::chrono::system_clock::to_time_t(now);
      std::tm tm = *std::localtime(&expiryTime);
      char buf[8];
      std::strftime(buf, sizeof(buf), "%Y-%m", &tm);
      month = buf;
    }

    auto dbClient = drogon::app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT DATE(created_at) as date, COALESCE(mood_type, 'neutral') as mood, COUNT(*) as count "
        "FROM stones WHERE user_id = $1 AND status = 'published' "
        "AND TO_CHAR(created_at, 'YYYY-MM') = $2 "
        "GROUP BY DATE(created_at), COALESCE(mood_type, 'neutral') ORDER BY date",
        user_id, month);

    auto moodToScore = [](const std::string &mood) -> double {
      if (mood == "happy") return 0.75;
      if (mood == "calm") return 0.35;
      if (mood == "neutral") return 0.0;
      if (mood == "hopeful") return 0.55;
      if (mood == "grateful") return 0.65;
      if (mood == "sad") return -0.75;
      if (mood == "anxious") return -0.45;
      if (mood == "angry") return -0.65;
      if (mood == "lonely") return -0.55;
      if (mood == "confused") return -0.1;
      return 0.0;
    };

    Json::Value days(Json::objectValue);
    std::map<std::string, std::pair<double, int>> dayScoreAgg;
    for (const auto &row : result) {
      std::string date = row["date"].as<std::string>();
      std::string mood = row["mood"].isNull() ? "neutral" : row["mood"].as<std::string>();
      int count = row["count"].as<int>();
      if (!days.isMember(date)) {
        days[date] = Json::Value(Json::objectValue);
        days[date]["count"] = 0;
        days[date]["moods"] = Json::Value(Json::objectValue);
      }
      days[date]["count"] = days[date]["count"].asInt() + count;
      days[date]["moods"][mood] = days[date]["moods"].get(mood, 0).asInt() + count;
      dayScoreAgg[date].first += moodToScore(mood) * count;
      dayScoreAgg[date].second += count;
    }

    for (const auto &entry : dayScoreAgg) {
      const auto &date = entry.first;
      const auto weightedSum = entry.second.first;
      const auto sampleCount = entry.second.second;
      const double avgRaw = sampleCount > 0 ? weightedSum / sampleCount : 0.0;
      days[date]["score"] = std::clamp((avgRaw + 1.0) / 2.0, 0.0, 1.0);
    }

    Json::Value response;
    response["month"] = month;
    response["days"] = days;
    callback(ResponseUtil::success(response));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getEmotionCalendar: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getEmotionCalendar: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::getEmotionHeatmap(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }
    auto user_id = *userIdOpt;

    int days_count = safeInt(req->getParameter("days"), 30);
    if (days_count < 1 || days_count > 365) days_count = 30;

    auto dbClient = drogon::app().getDbClient("default");

    auto result = dbClient->execSqlSync(
        "SELECT DATE(created_at) as date, "
        "COUNT(*) as count, "
        "AVG(COALESCE(emotion_score, "
        "CASE COALESCE(mood_type, 'neutral') "
        "WHEN 'happy' THEN 0.75 "
        "WHEN 'calm' THEN 0.35 "
        "WHEN 'neutral' THEN 0.0 "
        "WHEN 'hopeful' THEN 0.55 "
        "WHEN 'grateful' THEN 0.65 "
        "WHEN 'sad' THEN -0.75 "
        "WHEN 'anxious' THEN -0.45 "
        "WHEN 'angry' THEN -0.65 "
        "WHEN 'lonely' THEN -0.55 "
        "WHEN 'confused' THEN -0.1 "
        "ELSE 0.0 END)) as avg_score "
        "FROM stones WHERE user_id = $1 AND status = 'published' "
        "AND created_at >= NOW() - make_interval(days => $2) "
        "GROUP BY DATE(created_at) ORDER BY date",
        user_id, days_count);

    Json::Value days(Json::objectValue);
    for (const auto &row : result) {
      std::string date = row["date"].as<std::string>();
      int count = row["count"].as<int>();
      Json::Value dayData;
      const double rawScore = row["avg_score"].isNull() ? 0.0 : row["avg_score"].as<double>();
      const double normalized = std::clamp((rawScore + 1.0) / 2.0, 0.0, 1.0);
      dayData["score"] = normalized;
      dayData["raw_score"] = rawScore;
      dayData["count"] = count;
      days[date] = dayData;
    }

    Json::Value response;
    response["days"] = days;
    callback(ResponseUtil::success(response));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getEmotionHeatmap: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getEmotionHeatmap: " << e.what();
    callback(ResponseUtil::internalError());
  }
}
