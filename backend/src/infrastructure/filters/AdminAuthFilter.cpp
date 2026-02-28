/**
 * 管理后台认证过滤器实现
 *
 * 拦截所有 /api/admin/* 请求（登录接口除外），完成以下校验链：
 * 1. 从 Authorization 头提取 PASETO v4 token
 * 2. 验证 token 签名与有效期，解析出 admin_id 和 role
 * 3. 通过 RBACManager 检查角色对当前 path+method 的访问权限
 * 4. 校验通过后将 admin_id / admin_role 注入 request attributes
 *
 * CORS 策略：仅允许 CORS_ALLOWED_ORIGIN 环境变量中显式列出的域名。
 */
#include "infrastructure/filters/AdminAuthFilter.h"
#include "utils/ResponseUtil.h"
#include "utils/RBACManager.h"
#include "utils/PasetoUtil.h"
#include <drogon/drogon.h>
#include <set>
#include <sstream>
#include <cstdlib>

using namespace drogon;
using namespace heartlake::filters;
using namespace heartlake::utils;

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

static void addCorsHeaders(const HttpRequestPtr& req, const HttpResponsePtr& resp) {
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

void AdminAuthFilter::doFilter(const HttpRequestPtr& req,
                               FilterCallback&& fcb,
                               FilterChainCallback&& fccb) {
    // 跳过登录接口
    if (req->path() == "/api/admin/login") {
        fccb();
        return;
    }
    
    std::string token;
    try {
        token = PasetoUtil::extractToken(req);
    } catch (const std::exception&) {
        auto resp = ResponseUtil::unauthorized("请先登录管理后台");
        addCorsHeaders(req, resp);
        fcb(resp);
        return;
    }
    
    std::string adminId, role;
    if (!verifyAdminToken(token, adminId, role)) {
        auto resp = ResponseUtil::unauthorized("Token无效或已过期");
        addCorsHeaders(req, resp);
        fcb(resp);
        return;
    }
    
    // RBAC权限检查
    auto& rbac = RBACManager::getInstance();
    std::string path = req->path();
    std::string method = req->methodString();
    
    if (!rbac.checkPathPermission(role, path, method)) {
        LOG_WARN << "Permission denied: admin=" << adminId << " role=" << role 
                 << " path=" << path << " method=" << method;
        auto resp = ResponseUtil::forbidden("没有权限执行此操作");
        addCorsHeaders(req, resp);
        fcb(resp);
        return;
    }
    
    // 将管理员信息添加到请求属性中
    req->getAttributes()->insert("admin_id", adminId);
    req->getAttributes()->insert("admin_role", role);
    
    LOG_DEBUG << "Admin auth passed: " << adminId << " (" << role << ")";
    fccb();
}

bool AdminAuthFilter::verifyAdminToken(const std::string& token, std::string& adminId, std::string& role) {
    try {
        auto key = PasetoUtil::getAdminKey();
        return PasetoUtil::verifyAdminToken(token, key, adminId, role);
    } catch (const std::exception& e) {
        LOG_WARN << "Admin token verification failed: " << e.what();
        return false;
    }
}
