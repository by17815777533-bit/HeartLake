/**
 * @file SecurityAuditFilter.cpp
 * @brief SecurityAuditFilter 实现
 */

#include "infrastructure/filters/SecurityAuditFilter.h"
#include "utils/PasetoUtil.h"
#include "utils/ResponseUtil.h"
#include <vector>
#include <string>
#include <set>
#include <sstream>
#include <cstdlib>

namespace heartlake {
namespace filters {

namespace {
struct CorsConfig {
    std::set<std::string> allowedOrigins;
};

static const CorsConfig& getCorsConfig() {
    static const CorsConfig config = [] {
        CorsConfig c;
        const char* cors_origin = std::getenv("CORS_ALLOWED_ORIGIN");
        std::string corsConfig = cors_origin ? cors_origin : "http://localhost:5173,http://localhost:3000,http://localhost:8080";
        std::istringstream ss(corsConfig);
        std::string token;
        while (std::getline(ss, token, ',')) {
            token.erase(0, token.find_first_not_of(" "));
            if (!token.empty()) {
                token.erase(token.find_last_not_of(" ") + 1);
            }
            // 忽略通配符 *，生产环境必须显式列出允许的 origin
            if (token == "*") {
                LOG_WARN << "[CORS] 忽略通配符 '*'，请在 CORS_ALLOWED_ORIGIN 中显式列出允许的域名";
                continue;
            }
            if (!token.empty()) {
                c.allowedOrigins.insert(token);
            }
        }
        return c;
    }();
    return config;
}

static bool isOriginAllowed(const std::string& origin, const CorsConfig& config) {
    if (origin.empty()) return false;
    return config.allowedOrigins.count(origin) > 0;
}

static void addCorsHeaders(const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
    const std::string origin = req->getHeader("Origin");
    const auto& config = getCorsConfig();
    if (!isOriginAllowed(origin, config)) {
        return;
    }

    const std::string allowOrigin = origin;
    resp->addHeader("Access-Control-Allow-Origin", allowOrigin);
    resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type,Authorization,X-User-Id,X-Request-Id");
    resp->addHeader("Access-Control-Allow-Credentials", "true");
    resp->addHeader("Vary", "Origin");
}
// 日志注入防护：移除换行符和控制字符，防止日志伪造攻击
static std::string sanitizeForLog(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (char c : input) {
        if (c == '\n' || c == '\r') {
            result += ' ';
        } else if (static_cast<unsigned char>(c) < 0x20 && c != '\t') {
            // 除 tab 外的控制字符替换为空格
            result += ' ';
        } else {
            result += c;
        }
    }
    return result;
}
} // namespace

void SecurityAuditFilter::logRequest(const drogon::HttpRequestPtr& req,
                                      const std::string& userId) {
    const std::string method = req->methodString();
    const std::string path = sanitizeForLog(req->path());
    const std::string clientIp = sanitizeForLog(req->peerAddr().toIp());
    const std::string safeUserId = sanitizeForLog(userId);
    const std::string userAgent = sanitizeForLog(req->getHeader("User-Agent"));

    LOG_INFO << "[SecurityAudit] " << method << " " << path
             << " user=" << safeUserId
             << " ip=" << clientIp
             << " ua=" << userAgent;
}

void SecurityAuditFilter::doFilter(const drogon::HttpRequestPtr& req,
                                   drogon::FilterCallback&& fcb,
                                   drogon::FilterChainCallback&& fccb) {
    // 精确匹配白名单：仅允许完全匹配，防止 /api/lake/stones/xxx 绕过认证
    static const std::set<std::string> exactWhitelist = {
        "/api/auth/login",
        "/api/auth/register",
        "/api/auth/anonymous",
        "/api/lake/stones",
        "/api/lake/weather",
    };
    // 前缀匹配白名单：允许子路径（如 /api/health/ready、/ws/broadcast/xxx）
    static const std::vector<std::string> prefixWhitelist = {
        "/api/health",
        "/ws/broadcast",
    };

    std::string path = req->path();

    // 路径规范化：去除连续斜杠
    while (path.find("//") != std::string::npos) {
        path.replace(path.find("//"), 2, "/");
    }
    // 拒绝包含路径遍历的请求
    if (path.find("/..") != std::string::npos) {
        auto resp = utils::ResponseUtil::error(400, "非法路径");
        addCorsHeaders(req, resp);
        fcb(resp);
        return;
    }
    // 去除尾部斜杠
    if (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }

    // 精确匹配：O(log n) 查找
    if (exactWhitelist.count(path) > 0) {
        fccb();
        return;
    }
    // 前缀匹配：仅对需要子路径的端点生效
    for (const auto& prefix : prefixWhitelist) {
        if (path == prefix || (path.size() > prefix.size() && path.find(prefix) == 0 && path[prefix.size()] == '/')) {
            fccb();
            return;
        }
    }

    const std::string authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
        auto resp = utils::ResponseUtil::unauthorized("未授权访问");
        addCorsHeaders(req, resp);
        fcb(resp);
        return;
    }

    const std::string token = authHeader.substr(7);

    try {
        std::string key = utils::PasetoUtil::getKey();
        std::string userId = utils::PasetoUtil::verifyToken(token, key);

        req->getAttributes()->insert("user_id", userId);
        logRequest(req, userId);
        fccb();
    } catch (const std::exception& e) {
        auto resp = utils::ResponseUtil::unauthorized("Token无效或已过期");
        addCorsHeaders(req, resp);
        fcb(resp);
    }
}

} // namespace filters
} // namespace heartlake
