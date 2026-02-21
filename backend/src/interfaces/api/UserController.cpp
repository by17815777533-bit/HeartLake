/**
 * @file UserController.cpp
 * @brief UserController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/UserController.h"
#include "infrastructure/cache/RedisCache.h"
#include "infrastructure/services/EmailService.h"
#include "infrastructure/services/VerificationService.h"
#include "utils/IdGenerator.h"
#include "utils/PasetoUtil.h"
#include "utils/RecoveryKeyGenerator.h"
#include "utils/PasswordUtil.h"
#include "utils/ResponseUtil.h"
#include "utils/StructuredLogger.h"
#include <regex>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::services;

// SEC-1: 辅助函数 - 安全地从 attributes 获取 user_id（由认证中间件注入），避免直接读取可伪造的 Header
static std::string extractUserIdSafe(const HttpRequestPtr& req) {
    try {
        return req->getAttributes()->get<std::string>("user_id");
    } catch (...) {
        return "";
    }
}


void UserController::sendVerificationCode(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    // 从请求中获取邮箱
    std::string email = (*json).get("email", "").asString();
    if (email.empty()) {
      callback(ResponseUtil::badRequest("邮箱不能为空"));
      return;
    }

    // 使用VerificationService生成和存储验证码
    std::string code;
    try {
      code = VerificationService::generateAndStoreCode(email, "registration");
    } catch (const std::exception& e) {
      callback(ResponseUtil::badRequest(e.what()));
      return;
    }

    LOG_INFO << "Verification code generated for email: " << heartlake::utils::StructuredLogger::maskEmail(email);

    // 实际项目中这里应该发送短信或邮件
    Json::Value responseData;
    responseData["message"] = "验证码已生成（生产环境会发送到手机/邮箱）";

    // VUL-15 修复：更严格的开发模式检测，同时检查 ENV_MODE 和 DEV_EXPOSE_CODE
    const char* env_mode = std::getenv("ENV_MODE");
    const char* dev_expose = std::getenv("DEV_EXPOSE_CODE");
    bool isDevelopment = env_mode && std::string(env_mode) == "development";
    bool exposeCode = dev_expose && std::string(dev_expose) == "true";
    if (isDevelopment && exposeCode) {
        LOG_WARN << "返回验证码 - 仅限开发模式且 DEV_EXPOSE_CODE=true";
        responseData["dev_code"] = code;
    }

    callback(ResponseUtil::success(responseData, "验证码发送成功"));

  } catch (const std::exception &e) {
    LOG_ERROR << "Error in sendVerificationCode: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::registerUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string email = (*json).get("email", "").asString();
    std::string password = (*json).get("password", "").asString();
    std::string nickname = (*json).get("nickname", "").asString();
    std::string verificationCode = (*json).get("verification_code", "").asString();

    // 邮箱格式验证
    if (email.empty()) {
      callback(ResponseUtil::badRequest("邮箱不能为空"));
      return;
    }
    std::regex email_regex(R"(^[\w\-\.]+@([\w\-]+\.)+[\w\-]{2,4}$)");
    if (!std::regex_match(email, email_regex)) {
      callback(ResponseUtil::badRequest("请输入有效的邮箱地址"));
      return;
    }

    // 密码验证
    if (password.empty() || password.length() < 6) {
      callback(ResponseUtil::badRequest("密码至少6个字符"));
      return;
    }

    if (nickname.empty()) {
      nickname = IdGenerator::generateNickname();
    }

    // 验证码逻辑：开发模式下可跳过
    const char* env_mode = std::getenv("ENV_MODE");
    bool isDev = env_mode && std::string(env_mode) == "development";

    if (!isDev) {
      // 生产模式必须验证验证码
      if (verificationCode.empty()) {
        callback(ResponseUtil::badRequest("验证码不能为空"));
        return;
      }
      const char *test_code = std::getenv("TEST_VERIFICATION_CODE");
      bool isTestCode = test_code && !std::string(test_code).empty() && verificationCode == test_code;
      if (!isTestCode) {
        if (!VerificationService::verifyCode(email, verificationCode, "registration")) {
          callback(ResponseUtil::badRequest("验证码错误或已过期"));
          return;
        }
      }
    } else if (!verificationCode.empty()) {
      // 开发模式下如果提供了验证码也验证
      const char *test_code = std::getenv("TEST_VERIFICATION_CODE");
      bool isTestCode = test_code && !std::string(test_code).empty() && verificationCode == test_code;
      if (!isTestCode) {
        VerificationService::verifyCode(email, verificationCode, "registration");
        // 开发模式下验证码失败不阻止注册
      }
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 检查邮箱是否已注册
    auto checkResult = dbClient->execSqlSync(
        "SELECT user_id FROM users WHERE email = $1", email);
    if (!checkResult.empty()) {
      callback(ResponseUtil::conflict("该邮箱已被注册"));
      return;
    }

    // 生成自动递增的8位数字账号 (10000001, 10000002...)
    auto seqResult =
        dbClient->execSqlSync("SELECT nextval('user_account_seq') as seq");
    int accountNumber = seqResult[0]["seq"].as<int>();

    // 格式化为8位数字，从10000000开始
    int displayNumber = 10000000 + accountNumber;
    std::string username = std::to_string(displayNumber);

    std::string user_id = IdGenerator::generateUserId();

    // 使用安全的PBKDF2密码哈希
    std::string salt, password_hash;
    PasswordUtil::generatePasswordHash(password, salt, password_hash);

    dbClient->execSqlSync(
        "INSERT INTO users (user_id, username, email, password_hash, salt, nickname, "
        "is_anonymous, status, created_at, last_active_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, false, 'active', NOW(), NOW())",
        user_id, username, email, password_hash, salt, nickname);

    // 生成 PASETO Token
    std::string key = PasetoUtil::getKey();
    int expire_hours = 168; // 注册用户7天有效期

    std::string token =
        PasetoUtil::generateToken(user_id, key, expire_hours);

    Json::Value responseData;
    responseData["user_id"] = user_id;
    responseData["username"] = username;
    responseData["email"] = email;
    responseData["nickname"] = nickname;
    responseData["token"] = token;
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + expire_hours * 3600);

    callback(ResponseUtil::created(responseData, "注册成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in registerUser: " << e.base().what();
    // 捕获唯一约束冲突（邮箱重复）
    std::string errMsg = e.base().what();
    if (errMsg.find("users_email_key") != std::string::npos) {
      callback(ResponseUtil::conflict("该邮箱已被注册"));
      return;
    }
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in registerUser: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::loginUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    // 支持邮箱或用户名登录
    std::string email = (*json).get("email", "").asString();
    std::string username = (*json).get("username", "").asString();
    std::string password = (*json).get("password", "").asString();

    // 确定登录标识：优先使用 email，其次 username
    std::string loginIdentifier = !email.empty() ? email : username;
    if (loginIdentifier.empty() || password.empty()) {
      callback(ResponseUtil::badRequest("邮箱/用户名和密码不能为空"));
      return;
    }

    // VUL-19 修复：锁定策略结合 IP 地址，防止攻击者通过暴力尝试锁定他人账号
    auto& redisCache = heartlake::cache::RedisCache::getInstance();
    std::string clientIp = req->getPeerAddr().toIp();
    // 优先使用 X-Forwarded-For（反向代理场景）
    std::string xff = req->getHeader("X-Forwarded-For");
    if (!xff.empty()) {
      auto commaPos = xff.find(',');
      clientIp = (commaPos != std::string::npos) ? xff.substr(0, commaPos) : xff;
    }
    std::string lockKey = "login_lock:" + clientIp + ":" + loginIdentifier;
    std::string lockValue = redisCache.getSync(lockKey);
    if (!lockValue.empty()) {
      callback(ResponseUtil::error(423, "登录尝试过于频繁，请15分钟后重试"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 支持邮箱或用户名查询
    auto userResult = dbClient->execSqlSync(
        "SELECT user_id, username, email, nickname, avatar_url, bio, salt, password_hash FROM users "
        "WHERE (email = $1 OR username = $1) AND status = 'active'",
        loginIdentifier);

    if (userResult.empty()) {
      callback(ResponseUtil::error(401, "用户名或密码错误"));
      return;
    }

    auto row = userResult[0];

    // 匿名用户没有密码，无法通过密码登录
    if (row["salt"].isNull() || row["password_hash"].isNull()) {
      callback(ResponseUtil::error(401, "用户名或密码错误"));
      return;
    }

    std::string salt = row["salt"].as<std::string>();
    std::string stored_hash = row["password_hash"].as<std::string>();

    // 使用PasswordUtil验证密码
    if (!PasswordUtil::verifyPassword(password, salt, stored_hash)) {
      // VUL-19: 失败计数也结合 IP，防止 DoS 锁定攻击
      std::string failKey = "login_fail:" + clientIp + ":" + loginIdentifier;
      std::string failCount = redisCache.getSync(failKey);
      int count = failCount.empty() ? 0 : std::stoi(failCount);
      count++;
      if (count >= 5) {
        redisCache.setexSync(lockKey, "1", 900); // 锁定15分钟
        redisCache.del(failKey);
        callback(ResponseUtil::error(423, "登录失败次数过多，账号已被锁定15分钟"));
        return;
      }
      redisCache.setexSync(failKey, std::to_string(count), 300); // 5分钟内累计
      callback(ResponseUtil::error(401, "用户名或密码错误"));
      return;
    }

    // 登录成功，清除失败计数
    redisCache.del("login_fail:" + clientIp + ":" + loginIdentifier);

    std::string user_id = row["user_id"].as<std::string>();
    std::string nickname = row["nickname"].as<std::string>();
    std::string avatar_url =
        row["avatar_url"].isNull() ? "" : row["avatar_url"].as<std::string>();
    std::string bio = row["bio"].isNull() ? "" : row["bio"].as<std::string>();

    // 更新最后活跃时间
    dbClient->execSqlSync(
        "UPDATE users SET last_active_at = NOW() WHERE user_id = $1", user_id);

    // 生成 PASETO Token
    std::string key = PasetoUtil::getKey();
    int expire_hours = 168;

    std::string token =
        PasetoUtil::generateToken(user_id, key, expire_hours);

    // 从数据库结果中获取 username 和 email
    std::string db_username = row["username"].isNull() ? "" : row["username"].as<std::string>();
    std::string db_email = row["email"].isNull() ? "" : row["email"].as<std::string>();

    Json::Value responseData;
    responseData["user_id"] = user_id;
    responseData["username"] = db_username;
    responseData["email"] = db_email;
    responseData["nickname"] = nickname;
    responseData["token"] = token;
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + expire_hours * 3600);

    callback(ResponseUtil::success(responseData, "登录成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in loginUser: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in loginUser: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

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

    // 检查设备是否已存在
    auto result = dbClient->execSqlSync(
        "SELECT user_id, nickname, is_anonymous FROM users WHERE device_id = "
        "$1 AND status = 'active'",
        device_id);

    Json::Value responseData;
    std::string user_id;

    if (!result.empty()) {
      // 用户已存在，更新最后活跃时间
      auto row = result[0];
      user_id = row["user_id"].as<std::string>();

      dbClient->execSqlSync(
          "UPDATE users SET last_active_at = NOW() WHERE user_id = $1",
          user_id);

      responseData["user_id"] = user_id;
      responseData["nickname"] = row["nickname"].as<std::string>();
      responseData["is_anonymous"] = row["is_anonymous"].as<bool>();
    } else {
      // 创建新用户
      user_id = IdGenerator::generateAnonymousId();
      std::string nickname = IdGenerator::generateNickname();

      dbClient->execSqlSync("INSERT INTO users (user_id, username, nickname, device_id, "
                            "is_anonymous, status, created_at, last_active_at) "
                            "VALUES ($1, $2, $3, $4, true, 'active', NOW(), NOW())",
                            user_id, user_id, nickname, device_id);

      // 生成恢复关键词，哈希后存入数据库，明文仅此次返回
      auto recoveryKey = RecoveryKeyGenerator::generate();
      auto recoveryKeyHash = RecoveryKeyGenerator::hash(recoveryKey);

      // 确保 recovery_key_hash 列存在（首次部署时自动添加）
      try {
          dbClient->execSqlSync(
              "ALTER TABLE users ADD COLUMN IF NOT EXISTS recovery_key_hash VARCHAR(64)");
      } catch (...) {
          // 列已存在或不支持 IF NOT EXISTS，忽略
      }

      dbClient->execSqlSync(
          "UPDATE users SET recovery_key_hash = $1 WHERE user_id = $2",
          recoveryKeyHash, user_id);

      responseData["user_id"] = user_id;
      responseData["nickname"] = nickname;
      responseData["is_anonymous"] = true;
      responseData["recovery_key"] = recoveryKey;
    }

    // 生成 PASETO Token
    std::string key = PasetoUtil::getKey();
    int expire_hours = 24;

    std::string token =
        PasetoUtil::generateToken(user_id, key, expire_hours);

    responseData["token"] = token;
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + expire_hours * 3600);

    callback(ResponseUtil::success(responseData, "登录成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in anonymousLogin: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in anonymousLogin: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::getUserStats(
    const HttpRequestPtr & /*req*/,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &userId) {
  try {
    auto dbClient = drogon::app().getDbClient("default");

    // 获取用户统计数据
    auto result = dbClient->execSqlSync(
        "SELECT "
        "  (SELECT COUNT(*) FROM stones WHERE user_id = $1 AND status = "
        "'published') as stones_count,"
        "  (SELECT COALESCE(SUM(ripple_count), 0) FROM stones WHERE user_id = "
        "$1) as ripples_received,"
        "  (SELECT COALESCE(SUM(boat_count), 0) FROM stones WHERE user_id = "
        "$1) as boats_received,"
        "  (SELECT COUNT(*) FROM ripples WHERE user_id = $1) as ripples_sent,"
        "  (SELECT COUNT(*) FROM paper_boats WHERE sender_id = $1) as boats_sent,"
        "  (SELECT DATE_PART('day', NOW() - created_at) FROM users WHERE "
        "user_id = $1) as join_days",
        userId);

    if (result.empty()) {
      callback(ResponseUtil::notFound("用户不存在"));
      return;
    }

    auto row = result[0];
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

void UserController::getMyBoats(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    std::string user_id = extractUserIdSafe(req);
    if (user_id.empty()) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    int page = 1, page_size = 20;
    if (auto p = req->getParameter("page"); !p.empty()) { try { page = std::stoi(p); } catch (...) {} }
    if (auto p = req->getParameter("page_size"); !p.empty()) { try { page_size = std::stoi(p); } catch (...) {} }

    if (page < 1)
      page = 1;
    if (page_size < 1 || page_size > 100)
      page_size = 20;

    auto dbClient = drogon::app().getDbClient("default");

    // 获取我收到的纸船（通过我发布的石头）
    auto countResult =
        dbClient->execSqlSync("SELECT COUNT(*) as total FROM paper_boats b "
                              "INNER JOIN stones s ON b.stone_id = s.stone_id "
                              "WHERE s.user_id = $1 AND b.status IN ('active', 'drifting', 'sent', 'caught', 'published')",
                              user_id);
    int total = countResult[0]["total"].as<int>();

    int offset = (page - 1) * page_size;
    auto result = dbClient->execSqlSync(
        "SELECT b.boat_id, b.stone_id, b.content, b.boat_color, b.created_at, "
        "s.content as stone_content, s.stone_color, "
        "CASE WHEN b.sender_id = 'ai_ferryman' THEN '摆渡人' "
        "     WHEN b.is_anonymous THEN '匿名旅人' "
        "     ELSE u.nickname END as sender_name "
        "FROM paper_boats b "
        "INNER JOIN stones s ON b.stone_id = s.stone_id "
        "LEFT JOIN users u ON b.sender_id = u.user_id "
        "WHERE s.user_id = $1 AND b.status IN ('active', 'drifting', 'sent', 'caught', 'published') "
        "ORDER BY b.created_at DESC "
        "LIMIT " + std::to_string(page_size) + " OFFSET " + std::to_string(offset),
        user_id);

    Json::Value boats(Json::arrayValue);

    for (const auto &row : result) {
      Json::Value boat;
      boat["boat_id"] = row["boat_id"].isNull() ? "" : row["boat_id"].as<std::string>();
      boat["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
      boat["content"] = row["content"].isNull() ? "" : row["content"].as<std::string>();
      boat["boat_color"] = row["boat_color"].isNull()
                               ? "#F5EFE7"
                               : row["boat_color"].as<std::string>();
      boat["sender_name"] = row["sender_name"].isNull()
                                ? "匿名旅人"
                                : row["sender_name"].as<std::string>();
      boat["stone_content"] = row["stone_content"].isNull() ? "" : row["stone_content"].as<std::string>();
      boat["stone_color"] = row["stone_color"].isNull() ? "#E8F0FE" : row["stone_color"].as<std::string>();

      auto timestamp_str = row["created_at"].as<std::string>();
      boat["created_at"] = timestamp_str;

      boats.append(boat);
    }

    callback(ResponseUtil::paged(boats, total, page, page_size));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getMyBoats: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getMyBoats: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::getEmotionCalendar(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    std::string user_id = extractUserIdSafe(req);
    if (user_id.empty()) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 获取过去30天每天的投石数量和主要颜色
    auto result = dbClient->execSqlSync(
        "SELECT DATE(created_at) as date, "
        "       stone_color, "
        "       COUNT(*) as count "
        "FROM stones "
        "WHERE user_id = $1 "
        "  AND status = 'published' "
        "  AND created_at >= CURRENT_DATE - INTERVAL '30 days' "
        "GROUP BY DATE(created_at), stone_color "
        "ORDER BY date DESC, count DESC",
        user_id);

    Json::Value calendar(Json::arrayValue);
    std::map<std::string, Json::Value> dateMap;

    for (const auto &row : result) {
      std::string date = row["date"].as<std::string>();
      std::string color = row["stone_color"].as<std::string>();
      int count = row["count"].as<int>();

      if (dateMap.find(date) == dateMap.end()) {
        Json::Value dayData;
        dayData["date"] = date;
        dayData["colors"] = Json::Value(Json::arrayValue);
        dayData["total_stones"] = 0;
        dayData["primary_color"] = color; // 第一个就是最多的
        dateMap[date] = dayData;
      }

      Json::Value colorData;
      colorData["color"] = color;
      colorData["count"] = count;
      dateMap[date]["colors"].append(colorData);
      dateMap[date]["total_stones"] =
          dateMap[date]["total_stones"].asInt() + count;
    }

    // 转换为days对象格式（前端期望的格式）
    Json::Value days(Json::objectValue);
    for (auto &pair : dateMap) {
      days[pair.first] = pair.second;
    }

    Json::Value response;
    response["days"] = days;
    response["calendar"] = Json::arrayValue;
    for (auto &pair : dateMap) {
      response["calendar"].append(pair.second);
    }

    callback(ResponseUtil::success(response));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in getEmotionCalendar: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in getUserStats: " << e.what();
    callback(ResponseUtil::internalError());
  }
}
void UserController::updateNickname(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    std::string user_id = extractUserIdSafe(req);
    if (user_id.empty()) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

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

    // 更新昵称
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

void UserController::sendEmailVerificationCode(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string email = (*json)["email"].asString();

    // 邮箱格式验证
    std::regex email_regex(R"(^[\w\-\.]+@([\w\-]+\.)+[\w\-]{2,4}$)");
    if (email.empty() || !std::regex_match(email, email_regex)) {
      callback(ResponseUtil::badRequest("请输入有效的邮箱地址"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 检查邮箱是否已注册
    auto checkResult = dbClient->execSqlSync(
        "SELECT user_id FROM users WHERE email = $1", email);

    if (!checkResult.empty()) {
      callback(ResponseUtil::error(409, "该邮箱已被注册"));
      return;
    }

    // 生成并存储验证码到Redis
    std::string code;
    try {
      code = VerificationService::generateAndStoreCode(email, "registration");
    } catch (const std::exception& e) {
      callback(ResponseUtil::badRequest(e.what()));
      return;
    }

    // 发送邮件
    auto &emailService = EmailService::getInstance();
    bool sent = emailService.sendVerificationCode(email, code, "register");

    Json::Value responseData;
    responseData["email"] = email;

    if (sent) {
      callback(ResponseUtil::success(responseData, "验证码已发送到邮箱"));
    } else {
      // 邮件服务未启用，记录日志但不返回验证码
      LOG_WARN << "Email service not available for: " << heartlake::utils::StructuredLogger::maskEmail(email);
      callback(ResponseUtil::error(503, "邮件服务暂不可用，请稍后重试"));
    }

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in sendEmailVerificationCode: "
              << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in sendEmailVerificationCode: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::registerWithEmail(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    std::string verificationCode = (*json)["verification_code"].asString();
    std::string nickname = (*json).get("nickname", "").asString();

    // 邮箱格式验证
    std::regex email_regex(R"(^[\w\-\.]+@([\w\-]+\.)+[\w\-]{2,4}$)");
    if (email.empty() || !std::regex_match(email, email_regex)) {
      callback(ResponseUtil::badRequest("请输入有效的邮箱地址"));
      return;
    }

    // 密码强度验证
    if (password.empty() || password.length() < 6) {
      callback(ResponseUtil::badRequest("密码至少6个字符"));
      return;
    }

    if (verificationCode.empty()) {
      callback(ResponseUtil::badRequest("验证码不能为空"));
      return;
    }

    // 验证验证码
    if (!VerificationService::verifyCode(email, verificationCode, "registration")) {
      callback(ResponseUtil::badRequest("验证码错误或已过期"));
      return;
    }

    if (nickname.empty()) {
      nickname = IdGenerator::generateNickname();
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 检查邮箱是否已注册
    auto checkResult = dbClient->execSqlSync(
        "SELECT user_id FROM users WHERE email = $1", email);

    if (!checkResult.empty()) {
      callback(ResponseUtil::error(409, "该邮箱已被注册"));
      return;
    }

    // 生成自动递增的8位数字账号
    auto seqResult =
        dbClient->execSqlSync("SELECT nextval('user_account_seq') as seq");
    int accountNumber = seqResult[0]["seq"].as<int>();
    int displayNumber = 10000000 + accountNumber;
    std::string username = std::to_string(displayNumber);

    std::string user_id = IdGenerator::generateUserId();

    // 使用安全的PBKDF2密码哈希
    std::string salt, password_hash;
    PasswordUtil::generatePasswordHash(password, salt, password_hash);

    dbClient->execSqlSync(
        "INSERT INTO users (user_id, username, email, password_hash, salt, nickname, "
        "is_anonymous, status, created_at, last_active_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, false, 'active', NOW(), NOW())",
        user_id, username, email, password_hash, salt, nickname);

    // 生成 PASETO Token
    std::string key = PasetoUtil::getKey();
    int expire_hours = 168;

    std::string token =
        PasetoUtil::generateToken(user_id, key, expire_hours);

    Json::Value responseData;
    responseData["user_id"] = user_id;
    responseData["username"] = username;
    responseData["email"] = email;
    responseData["nickname"] = nickname;
    responseData["token"] = token;
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + expire_hours * 3600);

    callback(ResponseUtil::success(responseData, "注册成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in registerWithEmail: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in registerWithEmail: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::loginWithEmail(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    // 邮箱格式验证
    std::regex email_regex(R"(^[\w\-\.]+@([\w\-]+\.)+[\w\-]{2,4}$)");
    if (email.empty() || !std::regex_match(email, email_regex)) {
      callback(ResponseUtil::badRequest("请输入有效的邮箱地址"));
      return;
    }

    if (password.empty()) {
      callback(ResponseUtil::badRequest("密码不能为空"));
      return;
    }

    // 检查账号是否被锁定
    auto& redisCache = heartlake::cache::RedisCache::getInstance();
    std::string lockKey = "login_lock:email:" + email;
    std::string lockValue = redisCache.getSync(lockKey);
    if (!lockValue.empty()) {
      callback(ResponseUtil::error(423, "账号已被锁定，请15分钟后重试"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 首先获取用户的salt和password_hash
    auto userResult = dbClient->execSqlSync(
        "SELECT user_id, username, nickname, salt, password_hash FROM users WHERE email = $1 AND "
        "status = 'active'",
        email);

    if (userResult.empty()) {
      callback(ResponseUtil::error(401, "邮箱或密码错误"));
      return;
    }

    auto row = userResult[0];
    std::string salt = row["salt"].as<std::string>();
    std::string stored_hash = row["password_hash"].as<std::string>();

    // 使用PasswordUtil验证密码
    if (!PasswordUtil::verifyPassword(password, salt, stored_hash)) {
      // 记录失败次数
      std::string failKey = "login_fail:email:" + email;
      std::string failCount = redisCache.getSync(failKey);
      int count = failCount.empty() ? 0 : std::stoi(failCount);
      count++;
      if (count >= 5) {
        redisCache.setexSync(lockKey, "1", 900); // 锁定15分钟
        redisCache.del(failKey);
        callback(ResponseUtil::error(423, "登录失败次数过多，账号已被锁定15分钟"));
        return;
      }
      redisCache.setexSync(failKey, std::to_string(count), 300); // 5分钟内累计
      callback(ResponseUtil::error(401, "邮箱或密码错误"));
      return;
    }

    // 登录成功，清除失败计数
    redisCache.del("login_fail:email:" + email);

    std::string user_id = row["user_id"].as<std::string>();
    std::string username = row["username"].as<std::string>();
    std::string nickname = row["nickname"].as<std::string>();

    // 更新最后活跃时间
    dbClient->execSqlSync(
        "UPDATE users SET last_active_at = NOW() WHERE user_id = $1", user_id);

    // 生成 PASETO Token
    std::string key = PasetoUtil::getKey();
    int expire_hours = 168;

    std::string token =
        PasetoUtil::generateToken(user_id, key, expire_hours);

    Json::Value responseData;
    responseData["user_id"] = user_id;
    responseData["username"] = username;
    responseData["email"] = email;
    responseData["nickname"] = nickname;
    responseData["token"] = token;
    responseData["expires_at"] =
        static_cast<Json::Int64>(time(nullptr) + expire_hours * 3600);

    callback(ResponseUtil::success(responseData, "登录成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in loginWithEmail: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in loginWithEmail: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::changePassword(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    std::string user_id = extractUserIdSafe(req);
    if (user_id.empty()) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string oldPassword = (*json)["old_password"].asString();
    std::string newPassword = (*json)["new_password"].asString();

    if (oldPassword.empty() || newPassword.empty()) {
      callback(ResponseUtil::badRequest("旧密码和新密码不能为空"));
      return;
    }

    if (newPassword.length() < 6) {
      callback(ResponseUtil::badRequest("新密码至少6个字符"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 首先获取用户的salt和password_hash
    auto userResult = dbClient->execSqlSync(
        "SELECT salt, password_hash FROM users WHERE user_id = $1",
        user_id);

    if (userResult.empty()) {
      callback(ResponseUtil::error(404, "用户不存在"));
      return;
    }

    auto row = userResult[0];

    // 匿名用户没有密码，不能修改密码
    if (row["salt"].isNull() || row["password_hash"].isNull()) {
      callback(ResponseUtil::badRequest("匿名用户请先设置密码"));
      return;
    }

    std::string salt = row["salt"].as<std::string>();
    std::string stored_hash = row["password_hash"].as<std::string>();

    // 验证旧密码
    if (!PasswordUtil::verifyPassword(oldPassword, salt, stored_hash)) {
      callback(ResponseUtil::error(401, "旧密码错误"));
      return;
    }

    // 生成新密码的哈希
    std::string new_salt, new_password_hash;
    PasswordUtil::generatePasswordHash(newPassword, new_salt, new_password_hash);

    // 更新密码
    dbClient->execSqlSync("UPDATE users SET password_hash = $1, salt = $2, updated_at = "
                          "NOW() WHERE user_id = $3",
                          new_password_hash, new_salt, user_id);

    callback(ResponseUtil::success(Json::Value(), "密码修改成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in changePassword: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in changePassword: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::sendResetPasswordCode(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string email = (*json).get("email", "").asString();
    if (email.empty()) {
      callback(ResponseUtil::badRequest("邮箱不能为空"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "SELECT user_id FROM users WHERE email = $1", email);

    // VUL-13 修复：无论邮箱是否注册，都返回相同的成功响应
    // 防止攻击者通过不同响应探测用户注册状态
    Json::Value responseData;
    responseData["message"] = "如果该邮箱已注册，验证码将发送到您的邮箱";

    if (!result.empty()) {
      // 邮箱已注册，实际生成并发送验证码
      std::string code = VerificationService::generateAndStoreCode(email, "reset_password");
      LOG_INFO << "Reset password code generated for: " << heartlake::utils::StructuredLogger::maskEmail(email);

      // VUL-15: 更严格的开发模式检测
      const char* env_mode = std::getenv("ENV_MODE");
      const char* dev_expose = std::getenv("DEV_EXPOSE_CODE");
      bool isDev = env_mode && std::string(env_mode) == "development";
      bool expose = dev_expose && std::string(dev_expose) == "true";
      if (isDev && expose) {
        responseData["dev_code"] = code;
      }
    } else {
      // 邮箱未注册，仅记录日志，不泄露信息
      LOG_INFO << "Password reset requested for unregistered email";
    }

    callback(ResponseUtil::success(responseData));

  } catch (const std::exception &e) {
    LOG_ERROR << "Error in sendResetPasswordCode: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::resetPassword(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string email = (*json)["email"].asString();
    std::string code = (*json).get("verification_code", (*json).get("code", "")).asString();
    std::string newPassword = (*json)["new_password"].asString();

    if (email.empty() || code.empty() || newPassword.empty()) {
      callback(ResponseUtil::badRequest("邮箱、验证码和新密码不能为空"));
      return;
    }

    if (newPassword.length() < 6) {
      callback(ResponseUtil::badRequest("密码至少6个字符"));
      return;
    }

    if (!VerificationService::verifyCode(email, code, "reset_password")) {
      callback(ResponseUtil::error(400, "验证码错误或已过期"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "SELECT user_id FROM users WHERE email = $1", email);
    if (result.empty()) {
      callback(ResponseUtil::error(404, "用户不存在"));
      return;
    }

    std::string user_id = result[0]["user_id"].as<std::string>();
    std::string salt, password_hash;
    PasswordUtil::generatePasswordHash(newPassword, salt, password_hash);

    dbClient->execSqlSync(
        "UPDATE users SET password_hash = $1, salt = $2, updated_at = NOW() WHERE user_id = $3",
        password_hash, salt, user_id);

    VerificationService::deleteCode(email, "reset_password");
    callback(ResponseUtil::success(Json::Value(), "密码重置成功"));

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in resetPassword: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in resetPassword: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::deleteAccount(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    std::string user_id = extractUserIdSafe(req);
    if (user_id.empty()) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    auto json = req->getJsonObject();
    if (!json) {
      callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
      return;
    }

    std::string password = (*json)["password"].asString();
    std::string confirmation = (*json).get("confirmation", "").asString();

    if (password.empty()) {
      callback(ResponseUtil::badRequest("请输入密码"));
      return;
    }

    if (confirmation != "DELETE") {
      callback(ResponseUtil::badRequest("请输入确认文本 'DELETE'"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    // 首先获取用户的salt和password_hash
    auto userResult = dbClient->execSqlSync(
        "SELECT salt, password_hash, is_anonymous FROM users WHERE user_id = $1",
        user_id);

    if (userResult.empty()) {
      callback(ResponseUtil::error(404, "用户不存在"));
      return;
    }

    auto row = userResult[0];

    // 匿名用户可能没有密码，检查 null
    if (row["salt"].isNull() || row["password_hash"].isNull()) {
      callback(ResponseUtil::error(400, "匿名用户请使用其他方式注销账号"));
      return;
    }

    std::string salt = row["salt"].as<std::string>();
    std::string stored_hash = row["password_hash"].as<std::string>();

    // 验证密码
    if (!PasswordUtil::verifyPassword(password, salt, stored_hash)) {
      callback(ResponseUtil::error(401, "密码错误"));
      return;
    }

    // 开始事务，删除所有用户相关数据
    auto transPtr = dbClient->newTransaction();

    try {
      // 删除用户的石头
      transPtr->execSqlSync("DELETE FROM stones WHERE user_id = $1", user_id);

      // 删除用户的纸船
      transPtr->execSqlSync("DELETE FROM paper_boats WHERE sender_id = $1", user_id);

      // 删除用户的涟漪
      transPtr->execSqlSync("DELETE FROM ripples WHERE user_id = $1", user_id);

      // 删除用户的聊天消息
      transPtr->execSqlSync(
          "DELETE FROM chat_messages WHERE user_id = $1 OR receiver_id = $1",
          user_id);

      // 删除用户的好友关系
      transPtr->execSqlSync(
          "DELETE FROM friends WHERE user_id = $1 OR friend_id = $1", user_id);

      // 删除用户的临时好友关系
      transPtr->execSqlSync(
          "DELETE FROM temp_friends WHERE user_id = $1 OR friend_id = $1",
          user_id);

      // 删除用户的举报记录
      transPtr->execSqlSync("DELETE FROM reports WHERE reporter_id = $1",
                            user_id);

      // 删除用户记录
      transPtr->execSqlSync("DELETE FROM users WHERE user_id = $1", user_id);

      // QUALITY-4 修复：显式提交事务，确保所有删除操作原子性完成
      transPtr->execSqlSync("COMMIT");

      LOG_INFO << "User account deleted: " << user_id;
      callback(
          ResponseUtil::success(Json::Value(), "账号已注销，所有数据已清除"));

    } catch (const std::exception &e) {
      // QUALITY-4：异常时显式回滚事务
      try { transPtr->execSqlSync("ROLLBACK"); } catch (...) {}
      LOG_ERROR << "Transaction failed in deleteAccount: " << e.what();
      callback(ResponseUtil::internalError("删除账号失败"));
    }

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error in deleteAccount: " << e.base().what();
    callback(ResponseUtil::internalError("数据库错误"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in deleteAccount: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::searchUsers(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    std::string user_id = extractUserIdSafe(req);
    if (user_id.empty()) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    std::string query = req->getParameter("q");
    if (query.empty()) {
      callback(ResponseUtil::badRequest("搜索关键词不能为空"));
      return;
    }

    // 转义LIKE通配符，防止用户输入的 % _ \ 干扰查询
    auto escapeLike = [](const std::string &input) -> std::string {
      std::string result;
      result.reserve(input.size());
      for (char c : input) {
        if (c == '%' || c == '_' || c == '\\') {
          result += '\\';
        }
        result += c;
      }
      return result;
    };

    auto dbClient = drogon::app().getDbClient("default");

    // 搜索用户（通过用户名或昵称）
    // 只搜索活跃用户，排除自己
    auto result = dbClient->execSqlSync(
        "SELECT user_id, username, nickname, is_anonymous, created_at "
        "FROM users "
        "WHERE (username LIKE $1 ESCAPE '\\' OR nickname LIKE $1 ESCAPE '\\') "
        "AND status = 'active' "
        "AND user_id != $2 "
        "ORDER BY created_at DESC "
        "LIMIT 20",
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

void UserController::getUserInfo(
    [[maybe_unused]] const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &userId) {
  try {
    auto dbClient = drogon::app().getDbClient("default");

    // 获取用户基本信息
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

    auto row = result[0];
    Json::Value user;
    user["user_id"] = row["user_id"].as<std::string>();
    user["username"] = row["username"].as<std::string>();
    user["nickname"] = row["nickname"].as<std::string>();
    user["is_anonymous"] = row["is_anonymous"].as<bool>();
    user["created_at"] = row["created_at"].as<std::string>();
    user["avatar_url"] =
        row["avatar_url"].isNull() ? "" : row["avatar_url"].as<std::string>();
    user["bio"] = row["bio"].isNull() ? "" : row["bio"].as<std::string>();
    // 获取统计信息
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
void UserController::refreshToken(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
    std::string user_id = extractUserIdSafe(req);
    if (user_id.empty()) {
      callback(ResponseUtil::unauthorized("未登录"));
      return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    auto result = dbClient->execSqlSync(
        "SELECT status FROM users WHERE user_id = $1", user_id);

    if (result.empty() || result[0]["status"].as<std::string>() != "active") {
      callback(ResponseUtil::unauthorized("用户不存在或已禁用"));
      return;
    }

    std::string key = PasetoUtil::getKey();
    std::string token = PasetoUtil::generateToken(user_id, key, 168);

    Json::Value responseData;
    responseData["token"] = token;
    responseData["expires_at"] = static_cast<Json::Int64>(time(nullptr) + 168 * 3600);

    callback(ResponseUtil::success(responseData, "Token刷新成功"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Error in refreshToken: " << e.what();
    callback(ResponseUtil::internalError());
  }
}

void UserController::updateProfile(const HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        // SEC-1: 从 attributes 安全获取 user_id（由认证中间件注入）
        std::string user_id = extractUserIdSafe(req);
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }
        
        // QUALITY-3 修复：合并多次独立 UPDATE 为单次 UPDATE，减少数据库往返
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

        // 拼接单条 UPDATE 语句
        std::string sql = "UPDATE users SET ";
        for (size_t i = 0; i < setClauses.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += setClauses[i];
        }
        sql += ", updated_at = NOW() WHERE user_id = $" + std::to_string(paramIdx);

        // 动态执行参数化查询
        if (paramValues.size() == 1) {
            dbClient->execSqlSync(sql, paramValues[0], user_id);
        } else if (paramValues.size() == 2) {
            dbClient->execSqlSync(sql, paramValues[0], paramValues[1], user_id);
        } else if (paramValues.size() == 3) {
            dbClient->execSqlSync(sql, paramValues[0], paramValues[1], paramValues[2], user_id);
        }
        
        // 获取更新后的用户信息
        auto result = dbClient->execSqlSync(
            "SELECT user_id, nickname, avatar_url, bio FROM users WHERE user_id = $1",
            user_id
        );
        
        if (result.empty()) {
             callback(ResponseUtil::internalError("更新失败"));
             return;
        }
        
        auto row = result[0];
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
        auto keyHash = RecoveryKeyGenerator::hash(recoveryKey);

        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT user_id, nickname, is_anonymous FROM users "
            "WHERE recovery_key_hash = $1 AND status = 'active'",
            keyHash);

        if (result.empty()) {
            LOG_WARN << "Recovery attempt with invalid key, hash: " << keyHash.substr(0, 8) << "...";
            callback(ResponseUtil::notFound("关键词无效，请检查后重试"));
            return;
        }

        auto row = result[0];
        auto userId = row["user_id"].as<std::string>();
        auto nickname = row["nickname"].as<std::string>();
        auto isAnonymous = row["is_anonymous"].as<bool>();

        // 更新最后活跃时间
        dbClient->execSqlSync(
            "UPDATE users SET last_active_at = NOW() WHERE user_id = $1",
            userId);

        // 生成 PASETO Token
        std::string key = PasetoUtil::getKey();
        int expire_hours = isAnonymous ? 24 : 168;

        std::string token = PasetoUtil::generateToken(userId, key, expire_hours);

        Json::Value responseData;
        responseData["user_id"] = userId;
        responseData["nickname"] = nickname;
        responseData["is_anonymous"] = isAnonymous;
        responseData["token"] = token;
        responseData["expires_at"] =
            static_cast<Json::Int64>(time(nullptr) + expire_hours * 3600);

        LOG_INFO << "Account recovered successfully for user: " << userId;
        callback(ResponseUtil::success(responseData, "账号恢复成功"));

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in recoverWithKey: " << e.base().what();
        callback(ResponseUtil::internalError("恢复失败，请稍后重试"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in recoverWithKey: " << e.what();
        callback(ResponseUtil::internalError());
    }
}
