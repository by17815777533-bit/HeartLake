/**
 * 用户控制器 - 匿名登录 + 关键词恢复
 */
#include "interfaces/api/UserController.h"
#include "application/UserApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include "utils/IdGenerator.h"
#include "utils/BusinessRules.h"
#include "utils/PasetoUtil.h"
#include "utils/RecoveryKeyGenerator.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/SecurityLogger.h"
#include "utils/Validator.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>

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

  const auto confirmation =
      normalizeUpperToken((*json)["confirmation"].asString());
  return std::any_of(accepted.begin(), accepted.end(),
                     [&](const char *candidate) {
                       return confirmation == candidate;
                     });
}

int extractWindowTotal(const drogon::orm::Result &result,
                       const std::string &column = "total_count") {
  return result.empty() || result[0][column].isNull()
             ? 0
             : result[0][column].as<int>();
}

int64_t paginationOffset(int page, int pageSize) {
  return static_cast<int64_t>(page - 1) * static_cast<int64_t>(pageSize);
}

std::shared_ptr<heartlake::application::UserApplicationService>
getUserService() {
  return heartlake::core::di::ServiceLocator::instance()
      .resolve<heartlake::application::UserApplicationService>();
}
} // namespace

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
        "SELECT user_id, nickname, is_anonymous, status "
        "FROM users "
        "WHERE device_id = $1 AND status IN ('active', 'deactivated') "
        "ORDER BY CASE WHEN status = 'active' THEN 0 ELSE 1 END, "
        "updated_at DESC NULLS LAST, created_at DESC "
        "LIMIT 1",
        device_id);

    Json::Value responseData;
    std::string user_id;
    bool isNewUser = false;

    if (auto rowOpt = safeRow(result)) {
      auto row = *rowOpt;
      user_id = row["user_id"].as<std::string>();
      const auto status = row["status"].as<std::string>();

      if (status == "deactivated") {
        dbClient->execSqlSync(
            "UPDATE users SET status = 'active', updated_at = NOW(), "
            "last_active_at = NOW() WHERE user_id = $1",
            user_id);
        responseData["reactivated"] = true;
      } else {
        dbClient->execSqlSync(
            "UPDATE users SET last_active_at = NOW() WHERE user_id = $1",
            user_id);
      }

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
        LOG_WARN << "Failed to persist recovery_key_hash (schema not ready?): "
                 << e.base().what();
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

    if (!json->isMember("recovery_key") ||
        (*json)["recovery_key"].asString().empty()) {
      callback(ResponseUtil::badRequest("请提供恢复关键词"));
      return;
    }

    auto recoveryKey = (*json)["recovery_key"].asString();
    std::string deviceId;
    if (json->isMember("device_id")) {
      deviceId = (*json)["device_id"].asString();
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 加盐哈希无法通过 SQL 等值匹配，需在应用层逐条验证。
    // 若传入 device_id，仅作为排序优先级而非硬过滤条件，避免换机后无法恢复。
    auto result =
        !deviceId.empty()
            ? dbClient->execSqlSync(
                  "SELECT user_id, nickname, is_anonymous, recovery_key_hash, "
                  "status, "
                  "CASE WHEN device_id = $1 THEN 0 ELSE 1 END AS device_rank "
                  "FROM users "
                  "WHERE recovery_key_hash IS NOT NULL AND "
                  "status IN ('active', 'deactivated') "
                  "ORDER BY device_rank ASC, updated_at DESC NULLS LAST, "
                  "created_at DESC",
                  deviceId)
            : dbClient->execSqlSync(
                  "SELECT user_id, nickname, is_anonymous, recovery_key_hash, "
                  "status "
                  "FROM users "
                  "WHERE recovery_key_hash IS NOT NULL AND "
                  "status IN ('active', 'deactivated') "
                  "ORDER BY updated_at DESC NULLS LAST, created_at DESC");

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
    const auto status = result[matchIdx]["status"].as<std::string>();

    if (status == "deactivated") {
      if (!deviceId.empty()) {
        dbClient->execSqlSync(
            "UPDATE users SET status = 'active', device_id = $2, "
            "updated_at = NOW(), last_active_at = NOW() WHERE user_id = $1",
            userId, deviceId);
      } else {
        dbClient->execSqlSync(
            "UPDATE users SET status = 'active', updated_at = NOW(), "
            "last_active_at = NOW() WHERE user_id = $1",
            userId);
      }
    } else if (!deviceId.empty()) {
      dbClient->execSqlSync(
          "UPDATE users SET device_id = $2, last_active_at = NOW(), "
          "updated_at = NOW() WHERE user_id = $1",
          userId, deviceId);
    } else {
      dbClient->execSqlSync(
          "UPDATE users SET last_active_at = NOW() WHERE user_id = $1", userId);
    }

    std::string key = PasetoUtil::getKey();
    std::string token = PasetoUtil::generateToken(userId, key, 24);

    Json::Value responseData;
    responseData["user_id"] = userId;
    responseData["nickname"] = nickname;
    responseData["token"] = token;
    responseData["is_new_user"] = false;
    responseData["reactivated"] = (status == "deactivated");
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
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + 24 * 3600);

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
    if (!hasCompatibleConfirmation(json.get(), {"DELETE", "DEACTIVATE"})) {
      callback(ResponseUtil::badRequest(
          "confirmation 仅支持 'DEACTIVATE' 或兼容旧值 'DELETE'"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "UPDATE users "
        "SET status = 'deactivated', updated_at = NOW() "
        "WHERE user_id = $1 AND status <> 'deleted' "
        "RETURNING user_id",
        user_id);
    if (result.empty()) {
      callback(ResponseUtil::notFound("用户不存在或已永久删除"));
      return;
    }

    SecurityLogger::logEventFromRequest(
        req, user_id, SecurityEventType::ACCOUNT_DELETED,
        SecuritySeverity::HIGH, "兼容路由停用账号（30天内可恢复）");

    Json::Value data;
    data["status"] = "deactivated";
    data["legacy_route"] = true;
    data["recovery_window_days"] = 30;
    callback(ResponseUtil::success(data, "账号已停用，30天内可恢复"));

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
    auto service = getUserService();
    const auto profile = service->getUserProfile(userId);

    Json::Value user;
    user["user_id"] = profile.get("user_id", "").asString();
    user["username"] = profile.get("username", "").asString();
    user["nickname"] = profile.get("nickname", "").asString();
    user["is_anonymous"] = profile.get("is_anonymous", true).asBool();
    user["created_at"] = profile.get("created_at", "").asString();
    user["avatar_url"] = profile.get("avatar_url", "").asString();
    user["bio"] = profile.get("bio", "").asString();
    user["stones_count"] = profile.get("stone_count", 0).asInt();
    user["ripples_received"] = profile.get("ripples_received", 0).asInt();
    user["boats_received"] = profile.get("boats_received", 0).asInt();

    callback(ResponseUtil::success(user));

  } catch (const std::runtime_error &e) {
    if (std::string(e.what()) == "用户不存在") {
      callback(ResponseUtil::notFound("用户不存在"));
      return;
    }
    LOG_ERROR << "Error in getUserInfo: " << e.what();
    callback(ResponseUtil::internalError());
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
        "  COALESCE(st.stones_count, 0) AS stones_count, "
        "  COALESCE(st.ripples_received, 0) AS ripples_received, "
        "  COALESCE(st.boats_received, 0) AS boats_received, "
        "  COALESCE(rp.ripples_sent, 0) AS ripples_sent, "
        "  COALESCE(pb.boats_sent, 0) AS boats_sent, "
        "  GREATEST(DATE_PART('day', NOW() - u.created_at), 0) AS join_days "
        "FROM users u "
        "LEFT JOIN LATERAL ("
        "  SELECT "
        "    COUNT(*) FILTER (WHERE s.status = 'published' AND s.deleted_at IS NULL) AS stones_count, "
        "    COALESCE(SUM(CASE WHEN s.status = 'published' AND s.deleted_at IS NULL THEN s.ripple_count ELSE 0 END), 0) AS ripples_received, "
        "    COALESCE(SUM(CASE WHEN s.status = 'published' AND s.deleted_at IS NULL THEN s.boat_count ELSE 0 END), 0) AS boats_received "
        "  FROM stones s "
        "  WHERE s.user_id = u.user_id"
        ") st ON TRUE "
        "LEFT JOIN LATERAL ("
        "  SELECT COUNT(*) AS ripples_sent "
        "  FROM ripples r "
        "  WHERE r.user_id = u.user_id"
        ") rp ON TRUE "
        "LEFT JOIN LATERAL ("
        "  SELECT COUNT(*) FILTER (WHERE COALESCE(pb.status, 'active') != 'deleted') AS boats_sent "
        "  FROM paper_boats pb "
        "  WHERE pb.sender_id = u.user_id"
        ") pb ON TRUE "
        "WHERE u.user_id = $1 AND u.status = 'active'",
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

    auto [page, pageSize] = safePagination(req);
    auto service = getUserService();
    auto result = service->searchUsers(query, page, pageSize, user_id);
    callback(ResponseUtil::success(result));

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
    const int64_t offset = paginationOffset(page, page_size);
    auto result = dbClient->execSqlSync(
        "SELECT b.boat_id, b.stone_id, b.content, b.boat_style, b.created_at, "
        "b.status, b.sender_id, b.is_anonymous, b.mood, b.response_content, "
        "b.response_at, COUNT(*) OVER() AS total_count "
        "FROM paper_boats b "
        "INNER JOIN stones s ON b.stone_id = s.stone_id "
        "WHERE s.user_id = $1 AND b.sender_id <> $1 "
        "ORDER BY b.created_at DESC LIMIT $2 OFFSET $3",
        user_id, static_cast<int64_t>(page_size), offset);
    const int total = extractWindowTotal(result);

    Json::Value boats(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value boat;
      boat["boat_id"] = row["boat_id"].as<std::string>();
      boat["stone_id"] =
          row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
      boat["content"] = row["content"].as<std::string>();
      boat["boat_style"] = row["boat_style"].isNull()
                               ? "paper"
                               : row["boat_style"].as<std::string>();
      boat["created_at"] = row["created_at"].as<std::string>();
      boat["status"] = row["status"].as<std::string>();
      boat["is_anonymous"] =
          row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
      boat["mood"] = row["mood"].isNull() ? "" : row["mood"].as<std::string>();
      boat["response_content"] =
          row["response_content"].isNull()
              ? ""
              : row["response_content"].as<std::string>();
      boat["response_at"] = row["response_at"].isNull()
                                ? ""
                                : row["response_at"].as<std::string>();
      boats.append(boat);
    }

    callback(ResponseUtil::success(ResponseUtil::buildCollectionPayload(
        "boats", boats, total, page, page_size)));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getMyBoats: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getMyBoats: " << e.what();
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

    Json::Value updates(Json::objectValue);

    if (json->isMember("avatar_url") &&
        !(*json)["avatar_url"].asString().empty()) {
      updates["avatar_url"] = (*json)["avatar_url"].asString();
    }
    if (json->isMember("bio") && !(*json)["bio"].asString().empty()) {
      updates["bio"] = (*json)["bio"].asString();
    }
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
      updates["nickname"] = nickname;
    }

    if (updates.empty()) {
      callback(ResponseUtil::badRequest("没有要更新的字段"));
      return;
    }

    auto service = getUserService();
    const auto result = service->updateUserProfile(user_id, updates);
    Json::Value responseData;
    responseData["user_id"] = result.get("user_id", "").asString();
    responseData["nickname"] = result.get("nickname", "").asString();
    responseData["avatar_url"] = result.get("avatar_url", "").asString();
    responseData["bio"] = result.get("bio", "").asString();

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
        oss << std::setfill('0') << std::setw(4) << year << "-" << std::setw(2)
            << mon;
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
        "SELECT DATE(created_at) as date, COALESCE(mood_type, 'neutral') as "
        "mood, COUNT(*) as count "
        "FROM stones WHERE user_id = $1 AND status = 'published' "
        "AND TO_CHAR(created_at, 'YYYY-MM') = $2 "
        "GROUP BY DATE(created_at), COALESCE(mood_type, 'neutral') ORDER BY "
        "date",
        user_id, month);

    auto moodToScore = [](const std::string &mood) -> double {
      if (mood == "happy")
        return 0.75;
      if (mood == "calm")
        return 0.35;
      if (mood == "neutral")
        return 0.0;
      if (mood == "hopeful")
        return 0.55;
      if (mood == "grateful")
        return 0.65;
      if (mood == "sad")
        return -0.75;
      if (mood == "anxious")
        return -0.45;
      if (mood == "angry")
        return -0.65;
      if (mood == "lonely")
        return -0.55;
      if (mood == "confused")
        return -0.1;
      return 0.0;
    };

    Json::Value days(Json::objectValue);
    std::map<std::string, std::pair<double, int>> dayScoreAgg;
    for (const auto &row : result) {
      std::string date = row["date"].as<std::string>();
      std::string mood =
          row["mood"].isNull() ? "neutral" : row["mood"].as<std::string>();
      int count = row["count"].as<int>();
      if (!days.isMember(date)) {
        days[date] = Json::Value(Json::objectValue);
        days[date]["count"] = 0;
        days[date]["moods"] = Json::Value(Json::objectValue);
      }
      days[date]["count"] = days[date]["count"].asInt() + count;
      days[date]["moods"][mood] =
          days[date]["moods"].get(mood, 0).asInt() + count;
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
    if (days_count < 1 || days_count > 365)
      days_count = 30;

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
      const double rawScore =
          row["avg_score"].isNull() ? 0.0 : row["avg_score"].as<double>();
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
