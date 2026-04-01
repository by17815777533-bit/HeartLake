/**
 * 用户控制器 - 匿名登录 + 关键词恢复
 */
#include "interfaces/api/UserController.h"
#include "application/UserApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include "infrastructure/services/IntimacyService.h"
#include "utils/IdGenerator.h"
#include "utils/BusinessRules.h"
#include "utils/PasetoUtil.h"
#include "utils/RecoveryKeyGenerator.h"
#include "utils/MoodUtils.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/SecurityLogger.h"
#include "utils/Validator.h"
#include <openssl/sha.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>

using namespace heartlake::controllers;
using namespace heartlake::utils;

namespace {
std::string intimacyLevelZhForUser(const std::string &level) {
  if (level == "soulmate")
    return "灵魂同频";
  if (level == "close")
    return "深度共鸣";
  if (level == "warm")
    return "温暖连接";
  return "初识";
}

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

std::string normalizeBio(const std::string &raw) {
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
    return false;
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

constexpr int64_t kRefreshTokenTtlSeconds = 30LL * 24 * 3600;

std::string toLowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  return value;
}

std::string sha256Hex(const std::string &value) {
  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char *>(value.data()), value.size(),
         digest);

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char byte : digest) {
    oss << std::setw(2) << static_cast<int>(byte);
  }
  return oss.str();
}

std::string extractClientIp(const HttpRequestPtr &req) {
  const auto xRealIp = trimAscii(req->getHeader("X-Real-IP"));
  if (!xRealIp.empty()) {
    return xRealIp.substr(0, 45);
  }
  return req->getPeerAddr().toIp();
}

std::string deriveDeviceType(const HttpRequestPtr &req) {
  const auto userAgent = toLowerAscii(trimAscii(req->getHeader("User-Agent")));
  if (userAgent.find("android") != std::string::npos) {
    return "android";
  }
  if (userAgent.find("iphone") != std::string::npos ||
      userAgent.find("ipad") != std::string::npos ||
      userAgent.find("ios") != std::string::npos) {
    return "ios";
  }
  if (userAgent.find("windows") != std::string::npos ||
      userAgent.find("mac os") != std::string::npos ||
      userAgent.find("linux") != std::string::npos) {
    return "desktop";
  }
  if (userAgent.find("mozilla") != std::string::npos) {
    return "web";
  }
  if (userAgent.find("dart") != std::string::npos ||
      userAgent.find("flutter") != std::string::npos) {
    return "app";
  }
  return "unknown";
}

std::string deriveDeviceName(const HttpRequestPtr &req) {
  auto deviceName = trimAscii(req->getHeader("X-Device-Name"));
  if (deviceName.empty()) {
    deviceName = trimAscii(req->getHeader("User-Agent"));
  }
  if (deviceName.empty()) {
    deviceName = "HeartLake Client";
  }
  if (deviceName.size() > 128) {
    deviceName.resize(128);
  }
  return deviceName;
}

std::string generateRefreshToken() {
  return "rt_" + IdGenerator::generateUUID() + IdGenerator::generateUUID();
}

std::optional<std::string> extractBearerToken(const HttpRequestPtr &req) {
  const auto authHeader = req->getHeader("Authorization");
  if (authHeader.rfind("Bearer ", 0) != 0) {
    return std::nullopt;
  }
  const auto token = trimAscii(authHeader.substr(7));
  if (token.empty()) {
    return std::nullopt;
  }
  return token;
}

std::optional<std::string> verifyBearerUserId(const HttpRequestPtr &req) {
  const auto token = extractBearerToken(req);
  if (!token) {
    return std::nullopt;
  }

  try {
    const auto userId = PasetoUtil::verifyToken(*token, PasetoUtil::getKey());
    if (userId.empty()) {
      return std::nullopt;
    }
    return userId;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

ValidationResult validateAvatarUrlOrMediaPath(const std::string &avatarUrl) {
  if (avatarUrl.empty()) {
    return ValidationResult::valid();
  }
  if (avatarUrl.rfind("/api/media/", 0) == 0 ||
      avatarUrl.rfind("api/media/", 0) == 0 ||
      avatarUrl.rfind("/media/", 0) == 0 ||
      avatarUrl.rfind("media/", 0) == 0) {
    return Validator::checkPathTraversal(avatarUrl, "头像地址");
  }
  return Validator::url(avatarUrl, "头像地址");
}

void recordLoginLog(const drogon::orm::DbClientPtr &dbClient,
                    const HttpRequestPtr &req, const std::string &userId) {
  dbClient->execSqlAsync(
      "INSERT INTO login_logs (user_id, ip_address, device_type, location, "
      "success, login_time) VALUES ($1, $2, $3, '', true, NOW())",
      [](const drogon::orm::Result &) {},
      [](const drogon::orm::DrogonDbException &e) {
        LOG_WARN << "Failed to write login log: " << e.base().what();
      },
      userId, extractClientIp(req), deriveDeviceType(req));
}

struct SessionIssueResult {
  std::string sessionId;
  std::string refreshToken;
  Json::Int64 refreshExpiresAt{0};
};

SessionIssueResult issueRefreshSession(const drogon::orm::DbClientPtr &dbClient,
                                       const HttpRequestPtr &req,
                                       const std::string &userId,
                                       const std::string *existingSessionId =
                                           nullptr,
                                       const std::string *existingRefreshToken =
                                           nullptr) {
  const auto refreshToken =
      existingRefreshToken != nullptr ? *existingRefreshToken
                                      : generateRefreshToken();
  const auto refreshTokenHash = sha256Hex(refreshToken);
  const auto deviceType = deriveDeviceType(req);
  const auto deviceName = deriveDeviceName(req);
  const auto ipAddress = extractClientIp(req);

  std::optional<drogon::orm::Result> result;
  if (existingSessionId != nullptr) {
    result = dbClient->execSqlSync(
        "UPDATE user_sessions "
        "SET user_id = $2, device_type = NULLIF($3, ''), "
        "device_name = NULLIF($4, ''), ip_address = NULLIF($5, ''), "
        "is_active = true, refresh_token_hash = $6, "
        "refresh_expires_at = NOW() + INTERVAL '30 days', "
        "last_active_at = NOW() "
        "WHERE session_id = $1::uuid "
        "RETURNING session_id::text AS session_id, "
        "EXTRACT(EPOCH FROM refresh_expires_at)::BIGINT AS refresh_expires_at",
        *existingSessionId, userId, deviceType, deviceName, ipAddress,
        refreshTokenHash);
  }

  if (!result || result->empty()) {
    result = dbClient->execSqlSync(
        "INSERT INTO user_sessions "
        "(user_id, device_type, device_name, ip_address, is_active, "
        "refresh_token_hash, refresh_expires_at, last_active_at, created_at) "
        "VALUES ($1, NULLIF($2, ''), NULLIF($3, ''), NULLIF($4, ''), true, "
        "$5, NOW() + INTERVAL '30 days', NOW(), NOW()) "
        "RETURNING session_id::text AS session_id, "
        "EXTRACT(EPOCH FROM refresh_expires_at)::BIGINT AS refresh_expires_at",
        userId, deviceType, deviceName, ipAddress, refreshTokenHash);
  }

  SessionIssueResult issued;
  issued.sessionId = (*result)[0]["session_id"].as<std::string>();
  issued.refreshToken = refreshToken;
  issued.refreshExpiresAt =
      (*result)[0]["refresh_expires_at"].isNull()
          ? static_cast<Json::Int64>(time(nullptr) + kRefreshTokenTtlSeconds)
          : static_cast<Json::Int64>(
                (*result)[0]["refresh_expires_at"].as<int64_t>());
  return issued;
}

struct RefreshSessionContext {
  std::string sessionId;
  std::string userId;
  Json::Int64 refreshExpiresAt{0};
};

std::optional<RefreshSessionContext>
findRefreshSession(const drogon::orm::DbClientPtr &dbClient,
                   const std::string &refreshToken) {
  if (refreshToken.empty()) {
    return std::nullopt;
  }

  const auto result = dbClient->execSqlSync(
      "SELECT session_id::text AS session_id, user_id, "
      "EXTRACT(EPOCH FROM refresh_expires_at)::BIGINT AS refresh_expires_at "
      "FROM user_sessions "
      "WHERE refresh_token_hash = $1 "
      "AND is_active = true "
      "AND refresh_expires_at IS NOT NULL "
      "AND refresh_expires_at > NOW() "
      "LIMIT 1",
      sha256Hex(refreshToken));

  if (result.empty()) {
    return std::nullopt;
  }

  RefreshSessionContext context;
  context.sessionId = result[0]["session_id"].as<std::string>();
  context.userId = result[0]["user_id"].as<std::string>();
  context.refreshExpiresAt =
      result[0]["refresh_expires_at"].isNull()
          ? static_cast<Json::Int64>(time(nullptr) + kRefreshTokenTtlSeconds)
          : static_cast<Json::Int64>(
                result[0]["refresh_expires_at"].as<int64_t>());
  return context;
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

    const auto issuedSession = issueRefreshSession(dbClient, req, user_id);
    std::string key = PasetoUtil::getKey();
    std::string token = PasetoUtil::generateToken(user_id, key, 24);

    responseData["token"] = token;
    responseData["refresh_token"] = issuedSession.refreshToken;
    responseData["refresh_expires_at"] = issuedSession.refreshExpiresAt;
    responseData["session_id"] = issuedSession.sessionId;
    responseData["is_new_user"] = isNewUser;
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + 24 * 3600);

    recordLoginLog(dbClient, req, user_id);
    SecurityLogger::logEventFromRequest(req, user_id,
                                        SecurityEventType::SESSION_CREATED,
                                        SecuritySeverity::LOW,
                                        "匿名会话创建成功");
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

    const auto issuedSession = issueRefreshSession(dbClient, req, userId);
    std::string key = PasetoUtil::getKey();
    std::string token = PasetoUtil::generateToken(userId, key, 24);

    Json::Value responseData;
    responseData["user_id"] = userId;
    responseData["nickname"] = nickname;
    responseData["token"] = token;
    responseData["refresh_token"] = issuedSession.refreshToken;
    responseData["refresh_expires_at"] = issuedSession.refreshExpiresAt;
    responseData["session_id"] = issuedSession.sessionId;
    responseData["is_new_user"] = false;
    responseData["reactivated"] = (status == "deactivated");
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + 24 * 3600);

    recordLoginLog(dbClient, req, userId);
    SecurityLogger::logEventFromRequest(req, userId,
                                        SecurityEventType::SESSION_CREATED,
                                        SecuritySeverity::LOW,
                                        "恢复会话创建成功");
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
    auto dbClient = drogon::app().getDbClient("default");
    const auto json = req->getJsonObject();
    const auto requestRefreshToken =
        json && json->isMember("refresh_token")
            ? trimAscii((*json)["refresh_token"].asString())
            : std::string{};

    const auto refreshSession =
        requestRefreshToken.empty()
            ? std::nullopt
            : findRefreshSession(dbClient, requestRefreshToken);
    const auto bearerUserId = verifyBearerUserId(req);

    if (refreshSession && bearerUserId &&
        refreshSession->userId != *bearerUserId) {
      callback(ResponseUtil::unauthorized("会话凭证不匹配"));
      return;
    }

    std::string user_id;
    std::optional<std::string> existingSessionId;
    std::optional<std::string> existingRefreshToken;

    if (refreshSession) {
      user_id = refreshSession->userId;
      existingSessionId = refreshSession->sessionId;
      existingRefreshToken = requestRefreshToken;
    } else if (bearerUserId) {
      user_id = *bearerUserId;
    } else {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    auto userResult = dbClient->execSqlSync(
        "SELECT user_id FROM users "
        "WHERE user_id = $1 AND status IN ('active', 'deactivated') "
        "LIMIT 1",
        user_id);
    if (userResult.empty()) {
      callback(ResponseUtil::unauthorized("用户不存在或已失效"));
      return;
    }

    const auto issuedSession = issueRefreshSession(
        dbClient, req, user_id,
        existingSessionId ? &*existingSessionId : nullptr,
        existingRefreshToken ? &*existingRefreshToken : nullptr);

    std::string key = PasetoUtil::getKey();
    std::string token = PasetoUtil::generateToken(user_id, key, 24);

    Json::Value responseData;
    responseData["user_id"] = user_id;
    responseData["token"] = token;
    responseData["refresh_token"] = issuedSession.refreshToken;
    responseData["refresh_expires_at"] = issuedSession.refreshExpiresAt;
    responseData["session_id"] = issuedSession.sessionId;
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
    const HttpRequestPtr &req,
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
    user["is_friend"] = false;
    user["can_chat"] = false;

    if (auto currentUserIdOpt = Validator::getUserId(req);
        currentUserIdOpt && *currentUserIdOpt != userId) {
      auto &intimacy =
          heartlake::infrastructure::IntimacyService::getInstance();
      const double intimacyScore =
          intimacy.getIntimacyScore(*currentUserIdOpt, userId);
      const bool canChat = intimacy.canChat(*currentUserIdOpt, userId);
      const auto intimacyLevel =
          heartlake::infrastructure::IntimacyService::levelFromScore(
              intimacyScore);
      user["intimacy_score"] = intimacyScore;
      user["intimacy_level"] = intimacyLevel;
      user["intimacy_label"] = intimacyLevelZhForUser(intimacyLevel);
      user["is_friend"] = canChat;
      user["can_chat"] = canChat;
    }

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
        "SELECT b.boat_id, b.stone_id, s.user_id AS stone_user_id, "
        "b.content, b.boat_style, b.created_at, "
        "b.status, b.sender_id, b.is_anonymous, "
        "(b.sender_id IN ('ai_lakegod', 'lake_god')) AS is_ai_reply, "
        "b.mood, b.response_content, b.response_at, "
        "u.nickname AS sender_nickname, COUNT(*) OVER() AS total_count "
        "FROM paper_boats b "
        "INNER JOIN stones s ON b.stone_id = s.stone_id "
        "LEFT JOIN users u ON b.sender_id = u.user_id "
        "WHERE s.user_id = $1 AND b.sender_id <> $1 "
        "ORDER BY b.created_at DESC LIMIT $2 OFFSET $3",
        user_id, static_cast<int64_t>(page_size), offset);
    if (result.empty() && page > 1) {
      callback(ResponseUtil::badRequest("页码超出范围，请返回上一页重试"));
      return;
    }
    const int total = extractWindowTotal(result);

    Json::Value boats(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value boat;
      boat["boat_id"] = row["boat_id"].as<std::string>();
      boat["stone_id"] =
          row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
      boat["stone_user_id"] = row["stone_user_id"].isNull()
                                  ? user_id
                                  : row["stone_user_id"].as<std::string>();
      boat["content"] = row["content"].as<std::string>();
      boat["boat_style"] = row["boat_style"].isNull()
                               ? "paper"
                               : row["boat_style"].as<std::string>();
      boat["created_at"] = row["created_at"].as<std::string>();
      boat["status"] = row["status"].as<std::string>();
      boat["is_anonymous"] =
          row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
      const bool isAiReply =
          row["is_ai_reply"].isNull() ? false : row["is_ai_reply"].as<bool>();
      boat["is_ai_reply"] = isAiReply;
      boat["mood"] = row["mood"].isNull() ? "" : row["mood"].as<std::string>();
      boat["sender_name"] =
          isAiReply ? "湖神"
                    : (row["sender_nickname"].isNull()
                           ? "匿名旅人"
                           : row["sender_nickname"].as<std::string>());
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

    if (json->isMember("avatar_url")) {
      if (!(*json)["avatar_url"].isString()) {
        callback(ResponseUtil::badRequest("头像地址格式不正确"));
        return;
      }
      const auto avatarUrl = trimAscii((*json)["avatar_url"].asString());
      auto avatarValidation = validateAvatarUrlOrMediaPath(avatarUrl);
      if (!avatarValidation) {
        callback(ResponseUtil::badRequest(avatarValidation.errorMessage));
        return;
      }
      updates["avatar_url"] = avatarUrl;
    }
    if (json->isMember("bio")) {
      if (!(*json)["bio"].isString()) {
        callback(ResponseUtil::badRequest("个性签名格式不正确"));
        return;
      }
      const auto bio = normalizeBio((*json)["bio"].asString());
      auto bioValidation = Validator::length(bio, 0, 200, "个性签名");
      if (!bioValidation) {
        callback(ResponseUtil::badRequest(bioValidation.errorMessage));
        return;
      }
      updates["bio"] = bio;
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
    const auto normalizedMoodExpr =
        heartlake::utils::sqlCanonicalMoodExpr("mood_type");

    auto result = dbClient->execSqlSync(
        "SELECT DATE(created_at) as date, " + normalizedMoodExpr + " as mood, "
        "COUNT(*) as count "
        "FROM stones WHERE user_id = $1 AND status = 'published' "
        "AND TO_CHAR(created_at, 'YYYY-MM') = $2 "
        "GROUP BY DATE(created_at), " + normalizedMoodExpr + " ORDER BY date",
        user_id, month);

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
      dayScoreAgg[date].first += heartlake::utils::scoreForMood(mood) * count;
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

    int days_count = safeInt(req->getParameter("days"), 365);
    if (days_count < 1 || days_count > 365)
      days_count = 365;

    auto dbClient = drogon::app().getDbClient("default");
    const auto normalizedScoreExpr =
        heartlake::utils::sqlMoodScoreExpr("mood_type", "emotion_score");

    auto result = dbClient->execSqlSync(
        "SELECT DATE(created_at) as date, "
        "COUNT(*) as count, "
        "AVG(" + normalizedScoreExpr + ") as avg_score "
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
    response["days_count"] = days_count;
    callback(ResponseUtil::success(response));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getEmotionHeatmap: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getEmotionHeatmap: " << e.what();
    callback(ResponseUtil::internalError());
  }
}
