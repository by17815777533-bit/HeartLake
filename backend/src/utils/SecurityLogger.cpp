/**
 * @file SecurityLogger.cpp
 * @brief SecurityLogger 模块实现
 * Created by 白洋
 */

#include "utils/SecurityLogger.h"
#include <drogon/drogon.h>
#include <json/json.h>

using namespace heartlake::utils;

std::string SecurityLogger::eventTypeToString(SecurityEventType type) {
    switch (type) {
        case SecurityEventType::LOGIN_SUCCESS: return "login_success";
        case SecurityEventType::LOGIN_FAILED: return "login_failed";
        case SecurityEventType::LOGIN_SUSPICIOUS: return "login_suspicious";
        case SecurityEventType::PASSWORD_CHANGED: return "password_changed";
        case SecurityEventType::PASSWORD_RESET_REQUEST: return "password_reset_request";
        case SecurityEventType::PASSWORD_RESET_SUCCESS: return "password_reset_success";
        case SecurityEventType::ACCOUNT_LOCKED: return "account_locked";
        case SecurityEventType::ACCOUNT_UNLOCKED: return "account_unlocked";
        case SecurityEventType::ACCOUNT_CREATED: return "account_created";
        case SecurityEventType::ACCOUNT_DELETED: return "account_deleted";
        case SecurityEventType::EMAIL_VERIFIED: return "email_verified";
        case SecurityEventType::EMAIL_CHANGED: return "email_changed";
        case SecurityEventType::VERIFICATION_CODE_SENT: return "verification_code_sent";
        case SecurityEventType::VERIFICATION_CODE_FAILED: return "verification_code_failed";
        case SecurityEventType::RATE_LIMIT_EXCEEDED: return "rate_limit_exceeded";
        case SecurityEventType::UNAUTHORIZED_ACCESS: return "unauthorized_access";
        case SecurityEventType::PERMISSION_DENIED: return "permission_denied";
        case SecurityEventType::TOKEN_EXPIRED: return "token_expired";
        case SecurityEventType::TOKEN_INVALID: return "token_invalid";
        case SecurityEventType::SESSION_CREATED: return "session_created";
        case SecurityEventType::SESSION_TERMINATED: return "session_terminated";
        case SecurityEventType::SUSPICIOUS_ACTIVITY: return "suspicious_activity";
        case SecurityEventType::DATA_EXPORT_REQUEST: return "data_export_request";
        case SecurityEventType::PRIVACY_SETTINGS_CHANGED: return "privacy_settings_changed";
        case SecurityEventType::TWO_FACTOR_ENABLED: return "two_factor_enabled";
        case SecurityEventType::TWO_FACTOR_DISABLED: return "two_factor_disabled";
        default: return "unknown";
    }
}

std::string SecurityLogger::severityToString(SecuritySeverity severity) {
    switch (severity) {
        case SecuritySeverity::LOW: return "low";
        case SecuritySeverity::MEDIUM: return "medium";
        case SecuritySeverity::HIGH: return "high";
        case SecuritySeverity::CRITICAL: return "critical";
        default: return "low";
    }
}

std::string SecurityLogger::extractIpAddress(const drogon::HttpRequestPtr& req) {
    // 优先检查X-Forwarded-For（代理/负载均衡器）
    std::string xForwardedFor = req->getHeader("X-Forwarded-For");
    if (!xForwardedFor.empty()) {
        // X-Forwarded-For可能包含多个IP，取第一个
        size_t commaPos = xForwardedFor.find(',');
        if (commaPos != std::string::npos) {
            return xForwardedFor.substr(0, commaPos);
        }
        return xForwardedFor;
    }

    // 检查X-Real-IP
    std::string xRealIp = req->getHeader("X-Real-IP");
    if (!xRealIp.empty()) {
        return xRealIp;
    }

    // 使用对端地址
    return req->getPeerAddr().toIp();
}

std::string SecurityLogger::extractUserAgent(const drogon::HttpRequestPtr& req) {
    return req->getHeader("User-Agent");
}

void SecurityLogger::logEvent(const std::string& userId,
                             SecurityEventType eventType,
                             SecuritySeverity severity,
                             const std::string& description,
                             const std::string& ipAddress,
                             const std::string& userAgent,
                             const std::string& metadata) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        std::string eventTypeStr = eventTypeToString(eventType);
        std::string severityStr = severityToString(severity);

        // 如果userId为空，使用NULL
        if (userId.empty()) {
            dbClient->execSqlAsync(
                "INSERT INTO security_events (user_id, event_type, severity, description, "
                "ip_address, user_agent, metadata, created_at) "
                "VALUES (NULL, $1, $2, $3, $4, $5, $6::jsonb, NOW())",
                [](const drogon::orm::Result&) {
                    // 成功
                },
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "Failed to log security event: " << e.base().what();
                },
                eventTypeStr, severityStr, description, ipAddress, userAgent, metadata
            );
        } else {
            dbClient->execSqlAsync(
                "INSERT INTO security_events (user_id, event_type, severity, description, "
                "ip_address, user_agent, metadata, created_at) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7::jsonb, NOW())",
                [](const drogon::orm::Result&) {
                    // 成功
                },
                [](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "Failed to log security event: " << e.base().what();
                },
                userId, eventTypeStr, severityStr, description, ipAddress, userAgent, metadata
            );
        }

        // 同时记录到应用日志
        LOG_INFO << "[SECURITY] " << severityStr << " - " << eventTypeStr
                 << " - User: " << (userId.empty() ? "N/A" : userId)
                 << " - IP: " << ipAddress << " - " << description;

    } catch (const std::exception& e) {
        LOG_ERROR << "Exception in SecurityLogger::logEvent: " << e.what();
    }
}

void SecurityLogger::logEventFromRequest(const drogon::HttpRequestPtr& req,
                                        const std::string& userId,
                                        SecurityEventType eventType,
                                        SecuritySeverity severity,
                                        const std::string& description,
                                        const std::string& metadata) {
    std::string ipAddress = extractIpAddress(req);
    std::string userAgent = extractUserAgent(req);
    logEvent(userId, eventType, severity, description, ipAddress, userAgent, metadata);
}

void SecurityLogger::logLoginSuccess(const std::string& userId,
                                    const std::string& ipAddress,
                                    const std::string& userAgent) {
    Json::Value meta;
    meta["login_method"] = "password";
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string metadata = Json::writeString(writer, meta);

    logEvent(userId, SecurityEventType::LOGIN_SUCCESS, SecuritySeverity::LOW,
            "用户登录成功", ipAddress, userAgent, metadata);
}

void SecurityLogger::logLoginFailed(const std::string& username,
                                   const std::string& reason,
                                   const std::string& ipAddress,
                                   const std::string& userAgent) {
    Json::Value meta;
    meta["username"] = username;
    meta["failure_reason"] = reason;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string metadata = Json::writeString(writer, meta);

    logEvent("", SecurityEventType::LOGIN_FAILED, SecuritySeverity::MEDIUM,
            "登录失败: " + reason, ipAddress, userAgent, metadata);
}

void SecurityLogger::logSuspiciousLogin(const std::string& userId,
                                       const std::string& reason,
                                       const std::string& ipAddress,
                                       const std::string& userAgent) {
    Json::Value meta;
    meta["reason"] = reason;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string metadata = Json::writeString(writer, meta);

    logEvent(userId, SecurityEventType::LOGIN_SUSPICIOUS, SecuritySeverity::HIGH,
            "检测到可疑登录: " + reason, ipAddress, userAgent, metadata);
}

void SecurityLogger::logPasswordChanged(const std::string& userId,
                                       const std::string& ipAddress) {
    logEvent(userId, SecurityEventType::PASSWORD_CHANGED, SecuritySeverity::MEDIUM,
            "用户修改密码", ipAddress, "", "{}");
}

void SecurityLogger::logAccountLocked(const std::string& userId,
                                     const std::string& reason,
                                     const std::string& ipAddress) {
    Json::Value meta;
    meta["reason"] = reason;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string metadata = Json::writeString(writer, meta);

    logEvent(userId, SecurityEventType::ACCOUNT_LOCKED, SecuritySeverity::HIGH,
            "账号被锁定: " + reason, ipAddress, "", metadata);
}

void SecurityLogger::logRateLimitExceeded(const std::string& endpoint,
                                         const std::string& ipAddress,
                                         const std::string& userId) {
    Json::Value meta;
    meta["endpoint"] = endpoint;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string metadata = Json::writeString(writer, meta);

    logEvent(userId, SecurityEventType::RATE_LIMIT_EXCEEDED, SecuritySeverity::MEDIUM,
            "速率限制触发: " + endpoint, ipAddress, "", metadata);
}

void SecurityLogger::logUnauthorizedAccess(const std::string& endpoint,
                                          const std::string& ipAddress,
                                          const std::string& reason) {
    Json::Value meta;
    meta["endpoint"] = endpoint;
    meta["reason"] = reason;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string metadata = Json::writeString(writer, meta);

    logEvent("", SecurityEventType::UNAUTHORIZED_ACCESS, SecuritySeverity::HIGH,
            "未授权访问: " + endpoint, ipAddress, "", metadata);
}

std::string SecurityLogger::getUserSecurityEvents(const std::string& userId, int limit) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT event_type, severity, description, ip_address, created_at "
            "FROM security_events WHERE user_id = $1 "
            "ORDER BY created_at DESC LIMIT $2",
            userId, limit
        );

        Json::Value events(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value event;
            event["event_type"] = row["event_type"].as<std::string>();
            event["severity"] = row["severity"].as<std::string>();
            event["description"] = row["description"].as<std::string>();
            event["ip_address"] = row["ip_address"].as<std::string>();
            event["created_at"] = row["created_at"].as<std::string>();
            events.append(event);
        }

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        return Json::writeString(writer, events);

    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get user security events: " << e.what();
        return "[]";
    }
}

bool SecurityLogger::hasSuspiciousActivity(const std::string& userId, int timeWindowMinutes) {
    try {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT COUNT(*) as count FROM security_events "
            "WHERE user_id = $1 AND severity IN ('high', 'critical') "
            "AND created_at > NOW() - make_interval(mins => $2)",
            userId, timeWindowMinutes
        );

        if (!result.empty()) {
            int count = result[0]["count"].as<int>();
            return count > 0;
        }

        return false;

    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to check suspicious activity: " << e.what();
        return false;
    }
}