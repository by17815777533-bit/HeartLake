/**
 * RBACManager 模块实现
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
        {Permissions::AI, {Permissions::READ, Permissions::CREATE, Permissions::UPDATE}},
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

// 辅助函数：检查路径是否包含精确的路径段（按 / 分割后匹配）
static bool hasPathSegment(const std::string& path, const std::string& segment) {
    std::string needle = "/" + segment;
    auto pos = path.find(needle);
    if (pos == std::string::npos) return false;
    // 确保匹配的是完整路径段：下一个字符必须是 '/' 或字符串结尾
    auto endPos = pos + needle.size();
    return endPos == path.size() || path[endPos] == '/';
}

std::string RBACManager::pathToResource(const std::string& path) {
    // 路径到资源映射（使用精确路径段匹配，防止子串误匹配）
    if (hasPathSegment(path, "admin/users") || hasPathSegment(path, "admin/info")) {
        return Permissions::USERS;
    }
    if (hasPathSegment(path, "admin/content") ||
        hasPathSegment(path, "admin/stones") ||
        hasPathSegment(path, "admin/boats") ||
        hasPathSegment(path, "admin/moderation") ||
        hasPathSegment(path, "admin/sensitive-words")) {
        return Permissions::CONTENT;
    }
    if (hasPathSegment(path, "admin/reports")) {
        return Permissions::REPORTS;
    }
    if (hasPathSegment(path, "admin/config") ||
        hasPathSegment(path, "admin/settings") ||
        hasPathSegment(path, "admin/broadcast")) {
        return Permissions::SETTINGS;
    }
    if (hasPathSegment(path, "admin/stats") ||
        hasPathSegment(path, "admin/analytics") ||
        hasPathSegment(path, "admin/logs")) {
        return Permissions::STATISTICS;
    }
    if (hasPathSegment(path, "admin/risk") ||
        hasPathSegment(path, "admin/security") ||
        hasPathSegment(path, "admin/system")) {
        return Permissions::SYSTEM;
    }
    if (hasPathSegment(path, "admin/edge-ai") ||
        hasPathSegment(path, "admin/recommendations") ||
        hasPathSegment(path, "edge-ai") ||
        hasPathSegment(path, "recommendations") ||
        hasPathSegment(path, "ai")) {
        return Permissions::AI;
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
