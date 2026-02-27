/**
 * AdminController 模块实现
 */
#include "interfaces/api/AdminController.h"
#include "utils/ResponseUtil.h"
#include "utils/SecurityAuditScore.h"
#include "utils/PasetoUtil.h"
#include "utils/PasswordUtil.h"
#include "utils/SecurityLogger.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"

using namespace heartlake::controllers;
using namespace heartlake::utils;

/**
 * 时间恒定字符串比较，防止时序攻击
 * 无论字符串是否匹配，比较时间恒定
 */
static bool constantTimeCompare(const std::string& a, const std::string& b) {
    volatile unsigned char result = a.size() ^ b.size();
    size_t len = std::max(a.size(), b.size());
    for (size_t i = 0; i < len; ++i) {
        unsigned char ca = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        unsigned char cb = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        result |= ca ^ cb;
    }
    return result == 0;
}

void AdminController::login(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }

        std::string username = (*json).get("username", "").asString();
        std::string password = (*json).get("password", "").asString();

        if (username.empty() || password.empty()) {
            callback(ResponseUtil::badRequest("用户名和密码不能为空"));
            return;
        }

        // VUL-11 修复：从环境变量读取管理员凭证
        // ADMIN_USERNAME: 管理员用户名
        // ADMIN_PASSWORD_HASH: PBKDF2 哈希值（格式: salt:hash）
        // 兼容旧配置: ADMIN_PASSWORD 明文密码（将在运行时用 PBKDF2 验证）
        const char* env_admin_user = std::getenv("ADMIN_USERNAME");
        const char* env_admin_hash = std::getenv("ADMIN_PASSWORD_HASH");
        const char* env_admin_pass = std::getenv("ADMIN_PASSWORD");

        if (!env_admin_user || strlen(env_admin_user) == 0) {
            LOG_ERROR << "ADMIN_USERNAME not configured";
            callback(ResponseUtil::internalError("服务器配置错误"));
            return;
        }

        // 优先使用哈希值验证，其次使用明文密码的 PBKDF2 验证
        if (!env_admin_hash && (!env_admin_pass || strlen(env_admin_pass) == 0)) {
            LOG_ERROR << "Admin password not configured (need ADMIN_PASSWORD_HASH or ADMIN_PASSWORD)";
            callback(ResponseUtil::internalError("服务器配置错误"));
            return;
        }

        std::string admin_username = env_admin_user;

        // 时间恒定比较用户名，防止时序攻击泄露用户名是否正确
        bool usernameMatch = constantTimeCompare(username, admin_username);

        // VUL-11: 使用 PBKDF2 哈希验证密码，而非明文比对
        bool passwordMatch = false;
        if (env_admin_hash && strlen(env_admin_hash) > 0) {
            // 哈希格式: "salt:hash"
            std::string hashStr = env_admin_hash;
            auto colonPos = hashStr.find(':');
            if (colonPos != std::string::npos) {
                std::string salt = hashStr.substr(0, colonPos);
                std::string storedHash = hashStr.substr(colonPos + 1);
                passwordMatch = PasswordUtil::verifyPassword(password, salt, storedHash);
            }
        } else {
            // 明文密码回退已禁用 — 生产环境必须配置 ADMIN_PASSWORD_HASH
            LOG_ERROR << "ADMIN_PASSWORD_HASH not configured, plaintext fallback disabled";
            callback(ResponseUtil::internalError("服务器配置错误：请配置 ADMIN_PASSWORD_HASH"));
            return;
        }

        if (usernameMatch && passwordMatch) {
            // VUL-22: 记录管理员登录成功
            SecurityLogger::logEventFromRequest(req, "admin_001",
                SecurityEventType::LOGIN_SUCCESS, SecuritySeverity::MEDIUM,
                "管理员登录成功");

            auto adminKey = PasetoUtil::getAdminKey();
            auto token = PasetoUtil::generateAdminToken("admin_001", "admin", adminKey);

            Json::Value data;
            data["token"] = token;
            data["user"]["user_id"] = "admin_001";
            data["user"]["username"] = username;
            data["user"]["role"] = "admin";

            callback(ResponseUtil::success(data, "登录成功"));
        } else {
            // VUL-22: 记录管理员登录失败
            SecurityLogger::logEventFromRequest(req, "",
                SecurityEventType::LOGIN_FAILED, SecuritySeverity::HIGH,
                "管理员登录失败: 用户名或密码错误");

            callback(ResponseUtil::error(401, "用户名或密码错误"));
        }

    } catch (const std::exception &e) {
        LOG_ERROR << "Error in admin login: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void AdminController::logout([[maybe_unused]] const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback) {
    // PASETO是无状态的，客户端删除token即可
    Json::Value data;
    data["message"] = "登出成功";
    callback(ResponseUtil::success(data, "登出成功"));
}

void AdminController::getTrendingTopics([[maybe_unused]] const HttpRequestPtr &req,
                                        std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT UNNEST(tags) as topic, COUNT(*) as count "
            "FROM stones "
            "WHERE status = 'published' AND deleted_at IS NULL AND created_at >= CURRENT_DATE - INTERVAL '7 days' "
            "AND tags IS NOT NULL "
            "GROUP BY topic "
            "ORDER BY count DESC "
            "LIMIT 20"
        );

        Json::Value data(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value item;
            item["topic"] = row["topic"].as<std::string>();
            item["count"] = row["count"].as<int>();
            data.append(item);
        }

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getTrendingTopics: " << e.what();
        Json::Value data(Json::arrayValue);
        callback(ResponseUtil::success(data));
    }
}

void AdminController::getInfo([[maybe_unused]] const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        Json::Value data;
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto adminRole = req->getAttributes()->get<std::string>("admin_role");
        data["user_id"] = adminId;
        data["username"] = adminId;
        data["role"] = adminRole;
        data["permissions"] = Json::arrayValue;
        data["permissions"].append("all");

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getInfo: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void AdminController::getRealtimeStats([[maybe_unused]] const HttpRequestPtr &req,
                                      std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        // 查询各种统计数据
        auto userCount = dbClient->execSqlSync("SELECT COUNT(*) as count FROM users");
        auto stoneCount = dbClient->execSqlSync("SELECT COUNT(*) as count FROM stones WHERE status = 'published' AND deleted_at IS NULL");
        auto todayStones = dbClient->execSqlSync(
            "SELECT COUNT(*) as count FROM stones WHERE DATE(created_at) = CURRENT_DATE"
        );
        auto onlineUsers = dbClient->execSqlSync(
            "SELECT COUNT(DISTINCT user_id) as count FROM user_sessions WHERE created_at > NOW() - INTERVAL '5 minutes'"
        );

        // 验证数据库查询结果
        if (userCount.empty() || stoneCount.empty() || todayStones.empty()) {
            LOG_ERROR << "Database query returned empty results in getRealtimeStats";
            callback(ResponseUtil::internalError("Failed to fetch statistics"));
            return;
        }

        Json::Value data;
        data["total_users"] = userCount[0]["count"].as<int>();
        data["total_stones"] = stoneCount[0]["count"].as<int>();
        data["today_stones"] = todayStones[0]["count"].as<int>();
        data["online_users"] = onlineUsers.empty() ? 0 : onlineUsers[0]["count"].as<int>();

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getRealtimeStats: " << e.what();
        // VUL-16 修复：生产环境不返回 e.what()，仅记录到日志
        callback(ResponseUtil::internalError("获取实时统计失败"));
    }
}

void AdminController::getDashboardStats([[maybe_unused]] const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        Json::Value data;
        
        // 今日新增用户
        auto todayUsers = dbClient->execSqlSync(
            "SELECT COUNT(*) as count FROM users WHERE DATE(created_at) = CURRENT_DATE"
        );
        data["today_new_users"] = todayUsers[0]["count"].as<int>();

        // 今日活跃用户（BUG修复：traveler_id 不存在，应为 user_id）
        auto activeUsers = dbClient->execSqlSync(
            "SELECT COUNT(DISTINCT user_id) as count FROM stones WHERE DATE(created_at) = CURRENT_DATE"
        );
        data["today_active_users"] = activeUsers[0]["count"].as<int>();

        // 今日发布石头数
        auto todayStones = dbClient->execSqlSync(
            "SELECT COUNT(*) as count FROM stones WHERE DATE(created_at) = CURRENT_DATE AND status = 'published' AND deleted_at IS NULL"
        );
        data["today_stones"] = todayStones[0]["count"].as<int>();

        // 今日互动数
        auto todayInteractions = dbClient->execSqlSync(
            "SELECT (SELECT COUNT(*) FROM ripples WHERE DATE(created_at) = CURRENT_DATE) + "
            "(SELECT COUNT(*) FROM paper_boats WHERE DATE(created_at) = CURRENT_DATE) as count"
        );
        data["today_interactions"] = todayInteractions[0]["count"].as<int>();

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getDashboardStats: " << e.what();
        callback(ResponseUtil::internalError("获取仪表盘统计失败"));
    }
}

void AdminController::getUserGrowthStats(const HttpRequestPtr &req,
                                        std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        int days = 7;
        {
            std::string daysParam = req->getParameter("days");
            if (!daysParam.empty()) {
                days = safeInt(daysParam, 7);
            }
        }
        if (days < 1 || days > 365) days = 7;

        auto dbClient = drogon::app().getDbClient("default");

        // 使用参数化查询避免SQL注入
        auto result = dbClient->execSqlSync(
            "SELECT DATE(created_at) as date, COUNT(*) as count "
            "FROM users "
            "WHERE created_at >= CURRENT_DATE - $1::integer * INTERVAL '1 day' "
            "GROUP BY DATE(created_at) "
            "ORDER BY date ASC",
            days
        );

        Json::Value data(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value item;
            item["date"] = row["date"].as<std::string>();
            item["count"] = row["count"].as<int>();
            data.append(item);
        }

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getUserGrowthStats: " << e.what();
        Json::Value data(Json::arrayValue);
        callback(ResponseUtil::success(data));
    }
}

void AdminController::getMoodDistribution([[maybe_unused]] const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");
        
        auto result = dbClient->execSqlSync(
            "SELECT mood_type, COUNT(*) as count "
            "FROM stones "
            "WHERE status = 'published' AND deleted_at IS NULL AND mood_type IS NOT NULL "
            "GROUP BY mood_type "
            "ORDER BY count DESC"
        );

        Json::Value data(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value item;
            item["mood"] = row["mood_type"].as<std::string>();
            item["count"] = row["count"].as<int>();
            data.append(item);
        }

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getMoodDistribution: " << e.what();
        Json::Value data(Json::arrayValue);
        callback(ResponseUtil::success(data));
    }
}

void AdminController::getActiveTimeStats([[maybe_unused]] const HttpRequestPtr &req,
                                        std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");
        
        auto result = dbClient->execSqlSync(
            "SELECT EXTRACT(HOUR FROM created_at) as hour, COUNT(*) as count "
            "FROM stones "
            "WHERE status = 'published' AND deleted_at IS NULL AND created_at >= CURRENT_DATE - INTERVAL '7 days' "
            "GROUP BY EXTRACT(HOUR FROM created_at) "
            "ORDER BY hour"
        );

        Json::Value data(Json::arrayValue);
        for (int i = 0; i < 24; i++) {
            Json::Value item;
            item["hour"] = i;
            item["count"] = 0;
            
            for (const auto& row : result) {
                if (row["hour"].as<int>() == i) {
                    item["count"] = row["count"].as<int>();
                    break;
                }
            }
            
            data.append(item);
        }

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getActiveTimeStats: " << e.what();
        Json::Value data(Json::arrayValue);
        callback(ResponseUtil::success(data));
    }
}

// ==================== 心理风险监控 ====================

void AdminController::getHighRiskUsers([[maybe_unused]] const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        // 获取高风险用户列表（使用视图）
        auto result = dbClient->execSqlSync(
            "SELECT * FROM high_risk_users_monitor "
            "ORDER BY max_risk_score DESC, last_assessment_at DESC "
            "LIMIT 100"
        );

        Json::Value data(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value user;
            user["user_id"] = row["user_id"].as<std::string>();
            user["nickname"] = row["nickname"].as<std::string>();
            user["email"] = row["email"].isNull() ? "" : row["email"].as<std::string>();
            user["total_assessments"] = row["total_assessments"].as<int>();
            user["high_risk_count"] = row["high_risk_count"].as<int>();
            user["max_risk_score"] = row["max_risk_score"].as<float>();
            user["last_assessment_at"] = row["last_assessment_at"].as<std::string>();
            user["high_risk_events"] = row["high_risk_events"].as<int>();
            user["pending_events"] = row["pending_events"].as<int>();

            if (!row["negative_post_frequency"].isNull()) {
                user["negative_post_frequency"] = row["negative_post_frequency"].as<float>();
            }
            if (!row["social_isolation_score"].isNull()) {
                user["social_isolation_score"] = row["social_isolation_score"].as<float>();
            }
            if (!row["consecutive_negative_days"].isNull()) {
                user["consecutive_negative_days"] = row["consecutive_negative_days"].as<int>();
            }

            data.append(user);
        }

        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getHighRiskUsers: " << e.what();
        callback(ResponseUtil::internalError("获取高风险用户列表失败"));
    }
}

void AdminController::getHighRiskEvents(const HttpRequestPtr &req,
                                        std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        // 获取查询参数
        std::string status = req->getParameter("status");
        int limit = 50;
        int offset = 0;
        {
            std::string limitStr = req->getParameter("limit");
            std::string offsetStr = req->getParameter("offset");
            if (!limitStr.empty()) limit = safeInt(limitStr, 50);
            if (!offsetStr.empty()) offset = safeInt(offsetStr, 0);
        }
        if (limit < 1 || limit > 1000) limit = 50;
        if (offset < 0) offset = 0;

        // 使用参数化查询防止SQL注入
        auto result = !status.empty()
            ? dbClient->execSqlSync(
                "SELECT hre.*, u.nickname, u.email "
                "FROM high_risk_events hre "
                "JOIN users u ON hre.user_id = u.user_id "
                "WHERE hre.status = $1 "
                "ORDER BY hre.created_at DESC "
                "LIMIT $2 OFFSET $3", status, std::to_string(limit), std::to_string(offset))
            : dbClient->execSqlSync(
                "SELECT hre.*, u.nickname, u.email "
                "FROM high_risk_events hre "
                "JOIN users u ON hre.user_id = u.user_id "
                "ORDER BY hre.created_at DESC "
                "LIMIT $1 OFFSET $2", std::to_string(limit), std::to_string(offset));

        Json::Value data(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value event;
            event["event_id"] = row["event_id"].as<int>();
            event["user_id"] = row["user_id"].as<std::string>();
            event["nickname"] = row["nickname"].as<std::string>();
            event["email"] = row["email"].isNull() ? "" : row["email"].as<std::string>();
            event["content_id"] = row["content_id"].isNull() ? "" : row["content_id"].as<std::string>();
            event["content_type"] = row["content_type"].isNull() ? "" : row["content_type"].as<std::string>();
            event["risk_level"] = row["risk_level"].as<int>();
            event["risk_score"] = row["risk_score"].as<float>();
            event["intervention_sent"] = row["intervention_sent"].as<bool>();
            event["admin_notified"] = row["admin_notified"].as<bool>();
            event["status"] = row["status"].as<std::string>();
            event["created_at"] = row["created_at"].as<std::string>();

            if (!row["handled_by"].isNull()) {
                event["handled_by"] = row["handled_by"].as<std::string>();
            }
            if (!row["handled_at"].isNull()) {
                event["handled_at"] = row["handled_at"].as<std::string>();
            }
            if (!row["notes"].isNull()) {
                event["notes"] = row["notes"].as<std::string>();
            }

            data.append(event);
        }

        // 获取总数 - 使用参数化查询
        auto countResult = [&]() {
            if (!status.empty()) {
                return dbClient->execSqlSync(
                    "SELECT COUNT(*) as total FROM high_risk_events WHERE status = $1", status);
            } else {
                return dbClient->execSqlSync(
                    "SELECT COUNT(*) as total FROM high_risk_events");
            }
        }();
        int total = safeCount(countResult);

        Json::Value response;
        response["events"] = data;
        response["total"] = total;
        response["limit"] = limit;
        response["offset"] = offset;

        callback(ResponseUtil::success(response));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getHighRiskEvents: " << e.what();
        callback(ResponseUtil::internalError("获取高风险事件失败"));
    }
}

void AdminController::getUserRiskHistory([[maybe_unused]] const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback,
                                         const std::string &user_id) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        // 获取用户的风险评估历史
        auto assessments = dbClient->execSqlSync(
            "SELECT * FROM psychological_assessments "
            "WHERE user_id = $1 "
            "ORDER BY created_at DESC "
            "LIMIT 100",
            user_id
        );

        Json::Value assessmentList(Json::arrayValue);
        for (const auto& row : assessments) {
            Json::Value assessment;
            assessment["assessment_id"] = row["assessment_id"].as<int>();
            assessment["content_id"] = row["content_id"].isNull() ? "" : row["content_id"].as<std::string>();
            assessment["content_type"] = row["content_type"].isNull() ? "" : row["content_type"].as<std::string>();
            assessment["risk_level"] = row["risk_level"].as<int>();
            assessment["risk_score"] = row["risk_score"].as<float>();
            assessment["primary_concern"] = row["primary_concern"].isNull() ? "" : row["primary_concern"].as<std::string>();
            assessment["needs_immediate_attention"] = row["needs_immediate_attention"].as<bool>();
            assessment["created_at"] = row["created_at"].as<std::string>();

            assessmentList.append(assessment);
        }

        // 获取情绪趋势
        auto emotionTrend = dbClient->execSqlSync(
            "SELECT * FROM emotion_trend_analysis "
            "WHERE user_id = $1 "
            "ORDER BY date DESC "
            "LIMIT 30",
            user_id
        );

        Json::Value trendList(Json::arrayValue);
        for (const auto& row : emotionTrend) {
            Json::Value trend;
            trend["date"] = row["date"].as<std::string>();
            trend["avg_sentiment"] = row["avg_sentiment"].as<float>();
            trend["min_sentiment"] = row["min_sentiment"].as<float>();
            trend["max_sentiment"] = row["max_sentiment"].as<float>();
            trend["entry_count"] = row["entry_count"].as<int>();
            trend["negative_count"] = row["negative_count"].as<int>();

            trendList.append(trend);
        }

        // 获取行为模式
        auto behaviorPattern = dbClient->execSqlSync(
            "SELECT * FROM user_behavior_patterns "
            "WHERE user_id = $1 "
            "ORDER BY analysis_date DESC "
            "LIMIT 1",
            user_id
        );

        Json::Value behavior;
        if (!behaviorPattern.empty()) {
            auto row = behaviorPattern[0];
            behavior["analysis_date"] = row["analysis_date"].as<std::string>();
            behavior["negative_post_frequency"] = row["negative_post_frequency"].as<float>();
            behavior["engagement_decline"] = row["engagement_decline"].as<float>();
            behavior["social_isolation_score"] = row["social_isolation_score"].as<float>();
            behavior["consecutive_negative_days"] = row["consecutive_negative_days"].as<int>();
        }

        Json::Value response;
        response["user_id"] = user_id;
        response["assessments"] = assessmentList;
        response["emotion_trend"] = trendList;
        response["behavior_pattern"] = behavior;

        callback(ResponseUtil::success(response));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getUserRiskHistory: " << e.what();
        callback(ResponseUtil::internalError("获取用户风险历史失败"));
    }
}

void AdminController::handleRiskEvent(const HttpRequestPtr &req,
                                      std::function<void(const HttpResponsePtr &)> &&callback,
                                      const std::string &event_id) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }

        std::string action = (*json).get("action", "").asString();
        std::string notes = (*json).get("notes", "").asString();
        // SEC: 从认证属性中获取 admin_id，而非可伪造的请求头
        auto adminIdOpt = Validator::getUserId(req);
        if (!adminIdOpt) {
            callback(ResponseUtil::unauthorized("未授权的管理员操作"));
            return;
        }
        auto& admin_id = *adminIdOpt;

        if (action.empty()) {
            callback(ResponseUtil::badRequest("action 不能为空"));
            return;
        }

        // 安全解析 event_id
        int eventIdInt;
        try {
            eventIdInt = std::stoi(event_id);
        } catch (const std::invalid_argument&) {
            callback(ResponseUtil::badRequest("无效的事件ID格式"));
            return;
        } catch (const std::out_of_range&) {
            callback(ResponseUtil::badRequest("事件ID超出范围"));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");

        // 更新事件状态
        std::string status = "handled";
        if (action == "dismiss") {
            status = "dismissed";
        } else if (action == "escalate") {
            status = "escalated";
        }

        dbClient->execSqlSync(
            "UPDATE high_risk_events "
            "SET status = $1, handled_by = $2, handled_at = NOW(), notes = $3 "
            "WHERE event_id = $4",
            status, admin_id, notes, eventIdInt
        );

        // 记录管理员干预行动
        auto eventResult = dbClient->execSqlSync(
            "SELECT user_id FROM high_risk_events WHERE event_id = $1",
            eventIdInt
        );

        if (!eventResult.empty()) {
            std::string user_id = eventResult[0]["user_id"].as<std::string>();

            dbClient->execSqlSync(
                "INSERT INTO admin_interventions "
                "(admin_id, user_id, event_id, action_type, action_details, outcome, created_at) "
                "VALUES ($1, $2, $3, $4, $5, $6, NOW())",
                admin_id, user_id, eventIdInt, action, notes, status
            );
        }

        Json::Value data;
        data["event_id"] = event_id;
        data["status"] = status;
        data["handled_at"] = static_cast<Json::Int64>(time(nullptr));

        callback(ResponseUtil::success(data, "事件处理成功"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in handleRiskEvent: " << e.what();
        callback(ResponseUtil::internalError("处理事件失败"));
    }
}

void AdminController::getSecurityAudit([[maybe_unused]] const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto& auditor = heartlake::utils::SecurityAuditScore::getInstance();
        Json::Value result = auditor.runAudit();
        callback(ResponseUtil::success(result));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getSecurityAudit: " << e.what();
        callback(ResponseUtil::internalError("安全审计失败"));
    }
}
