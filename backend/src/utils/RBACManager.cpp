/**
 * @file RBACManager.cpp
 * @brief RBACManager 模块实现
 * Created by 林子怡
 */
#include "utils/RBACManager.h"
#include <drogon/drogon.h>

#ifdef _WIN32
#undef DELETE
#endif

using namespace heartlake::utils;

RBACManager& RBACManager::getInstance() {
    static RBACManager instance;
    return instance;
}

void RBACManager::initialize() {
    // 角色层级定义
    roleLevels_ = {
        {Roles::SUPER_ADMIN, 100},
        {Roles::ADMIN, 80},
        {Roles::MODERATOR, 60},
        {Roles::ANALYST, 40}
    };
    
    // Super Admin - 所有权限
    permissions_[Roles::SUPER_ADMIN] = {
        {Permissions::USERS, {Permissions::READ, Permissions::CREATE, Permissions::UPDATE, Permissions::DELETE, Permissions::MANAGE}},
        {Permissions::CONTENT, {Permissions::READ, Permissions::CREATE, Permissions::UPDATE, Permissions::DELETE, Permissions::MANAGE}},
        {Permissions::REPORTS, {Permissions::READ, Permissions::CREATE, Permissions::UPDATE, Permissions::DELETE, Permissions::MANAGE}},
        {Permissions::SETTINGS, {Permissions::READ, Permissions::UPDATE, Permissions::MANAGE}},
        {Permissions::STATISTICS, {Permissions::READ, Permissions::MANAGE}},
        {Permissions::AI, {Permissions::READ, Permissions::CREATE, Permissions::UPDATE, Permissions::MANAGE}},
        {Permissions::SYSTEM, {Permissions::READ, Permissions::MANAGE}}
    };
    
    // Admin - 用户管理、内容审核、举报处理、系统只读
    permissions_[Roles::ADMIN] = {
        {Permissions::USERS, {Permissions::READ, Permissions::CREATE, Permissions::UPDATE, Permissions::DELETE}},
        {Permissions::CONTENT, {Permissions::READ, Permissions::CREATE, Permissions::UPDATE, Permissions::DELETE}},
        {Permissions::REPORTS, {Permissions::READ, Permissions::UPDATE}},
        {Permissions::SETTINGS, {Permissions::READ}},
        {Permissions::STATISTICS, {Permissions::READ}},
        {Permissions::AI, {Permissions::READ, Permissions::CREATE}},
        {Permissions::SYSTEM, {Permissions::READ}}
    };
    
    // Moderator - 内容审核、举报处理
    permissions_[Roles::MODERATOR] = {
        {Permissions::USERS, {Permissions::READ}},
        {Permissions::CONTENT, {Permissions::READ, Permissions::UPDATE}},
        {Permissions::REPORTS, {Permissions::READ, Permissions::UPDATE}},
        {Permissions::STATISTICS, {Permissions::READ}},
        {Permissions::AI, {Permissions::READ}}
    };
    
    // Analyst - 只读数据分析
    permissions_[Roles::ANALYST] = {
        {Permissions::USERS, {Permissions::READ}},
        {Permissions::CONTENT, {Permissions::READ}},
        {Permissions::STATISTICS, {Permissions::READ}},
        {Permissions::AI, {Permissions::READ}}
    };
    
    LOG_INFO << "RBAC Manager initialized with " << permissions_.size() << " roles";
}

bool RBACManager::hasPermission(const std::string& role, const std::string& resource, const std::string& action) {
    auto roleIt = permissions_.find(role);
    if (roleIt == permissions_.end()) {
        return false;
    }
    
    auto resourceIt = roleIt->second.find(resource);
    if (resourceIt == roleIt->second.end()) {
        return false;
    }
    
    return resourceIt->second.count(action) > 0;
}

std::vector<std::string> RBACManager::getRolePermissions(const std::string& role) {
    std::vector<std::string> result;
    
    auto roleIt = permissions_.find(role);
    if (roleIt == permissions_.end()) {
        return result;
    }
    
    for (const auto& [resource, actions] : roleIt->second) {
        for (const auto& action : actions) {
            result.push_back(resource + ":" + action);
        }
    }
    
    return result;
}

bool RBACManager::checkPathPermission(const std::string& role, const std::string& path, const std::string& method) {
    std::string resource = pathToResource(path);
    std::string action = methodToAction(method);
    
    // 如果是MANAGE权限，允许所有操作
    if (hasPermission(role, resource, Permissions::MANAGE)) {
        return true;
    }
    
    return hasPermission(role, resource, action);
}

std::vector<std::string> RBACManager::getAllRoles() {
    return {Roles::SUPER_ADMIN, Roles::ADMIN, Roles::MODERATOR, Roles::ANALYST};
}

bool RBACManager::isValidRole(const std::string& role) {
    return roleLevels_.count(role) > 0;
}

int RBACManager::getRoleLevel(const std::string& role) {
    auto it = roleLevels_.find(role);
    return it != roleLevels_.end() ? it->second : 0;
}

std::string RBACManager::pathToResource(const std::string& path) {
    // 路径到资源映射（支持 /api/admin/ 和 /api/v1/admin/ 两种前缀）
    if (path.find("/admin/users") != std::string::npos) {
        return Permissions::USERS;
    }
    if (path.find("/admin/content") != std::string::npos ||
        path.find("/admin/stones") != std::string::npos ||
        path.find("/admin/boats") != std::string::npos ||
        path.find("/admin/moderation") != std::string::npos) {
        return Permissions::CONTENT;
    }
    if (path.find("/admin/reports") != std::string::npos) {
        return Permissions::REPORTS;
    }
    if (path.find("/admin/config") != std::string::npos ||
        path.find("/admin/settings") != std::string::npos) {
        return Permissions::SETTINGS;
    }
    if (path.find("/admin/stats") != std::string::npos ||
        path.find("/admin/analytics") != std::string::npos) {
        return Permissions::STATISTICS;
    }
    if (path.find("/admin/sensitive-words") != std::string::npos) {
        return Permissions::CONTENT;
    }
    if (path.find("/admin/logs") != std::string::npos) {
        return Permissions::STATISTICS;
    }
    if (path.find("/admin/broadcast") != std::string::npos) {
        return Permissions::SETTINGS;
    }
    if (path.find("/admin/info") != std::string::npos) {
        return Permissions::USERS;
    }
    if (path.find("/admin/risk") != std::string::npos ||
        path.find("/admin/security") != std::string::npos) {
        return Permissions::SYSTEM;
    }
    if (path.find("/ai/") != std::string::npos) {
        return Permissions::AI;
    }
    if (path.find("/admin/system") != std::string::npos) {
        return Permissions::SYSTEM;
    }
    
    // 默认返回最受限的资源
    return Permissions::SYSTEM;
}

std::string RBACManager::methodToAction(const std::string& method) {
    if (method == "GET" || method == "HEAD") {
        return Permissions::READ;
    }
    if (method == "POST") {
        return Permissions::CREATE;
    }
    if (method == "PUT" || method == "PATCH") {
        return Permissions::UPDATE;
    }
    if (method == "DELETE") {
        return Permissions::DELETE;
    }
    return Permissions::READ;
}
