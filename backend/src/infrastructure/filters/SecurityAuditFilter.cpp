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
    bool allowAll{false};
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
            if (token == "*") {
                c.allowAll = true;
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
    if (config.allowAll) return true;
    // 仅允许 CORS_ALLOWED_ORIGIN 中显式配置的来源，不再硬编码放行 localhost
    return config.allowedOrigins.count(origin) > 0;
}

static void addCorsHeaders(const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
    const std::string origin = req->getHeader("Origin");
    const auto& config = getCorsConfig();
    if (!isOriginAllowed(origin, config)) {
        return;
    }

    const std::string allowOrigin = config.allowAll ? "*" : origin;
    resp->addHeader("Access-Control-Allow-Origin", allowOrigin);
    resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type,Authorization,X-User-Id,X-Request-Id");
    resp->addHeader("Access-Control-Allow-Credentials", "true");
    if (allowOrigin != "*") {
        resp->addHeader("Vary", "Origin");
    }
}
} // namespace

void SecurityAuditFilter::doFilter(const drogon::HttpRequestPtr& req,
                                   drogon::FilterCallback&& fcb,
                                   drogon::FilterChainCallback&& fccb) {
    static const std::vector<std::string> whitelist = {
        "/api/auth/login",
        "/api/auth/register",
        "/api/auth/anonymous",
        "/api/lake/stones",
        "/api/lake/weather",
        "/api/health",
        "/ws/broadcast"
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

    for (const auto& prefix : whitelist) {
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
        fccb();
    } catch (const std::exception& e) {
        auto resp = utils::ResponseUtil::unauthorized("Token无效或已过期");
        addCorsHeaders(req, resp);
        fcb(resp);
    }
}

} // namespace filters
} // namespace heartlake
