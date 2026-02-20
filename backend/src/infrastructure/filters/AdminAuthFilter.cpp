/**
 * @file AdminAuthFilter.cpp
 * @brief AdminAuthFilter 模块实现
 * Created by 白洋
 */
#include "infrastructure/filters/AdminAuthFilter.h"
#include "utils/ResponseUtil.h"
#include "utils/RBACManager.h"
#include "utils/PasetoUtil.h"
#include <drogon/drogon.h>

using namespace drogon;
using namespace heartlake::filters;
using namespace heartlake::utils;

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
        fcb(ResponseUtil::unauthorized("请先登录管理后台"));
        return;
    }
    
    std::string adminId, role;
    if (!verifyAdminToken(token, adminId, role)) {
        fcb(ResponseUtil::unauthorized("Token无效或已过期"));
        return;
    }
    
    // RBAC权限检查
    auto& rbac = RBACManager::getInstance();
    std::string path = req->path();
    std::string method = req->methodString();
    
    if (!rbac.checkPathPermission(role, path, method)) {
        LOG_WARN << "Permission denied: admin=" << adminId << " role=" << role 
                 << " path=" << path << " method=" << method;
        fcb(ResponseUtil::forbidden("没有权限执行此操作"));
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
