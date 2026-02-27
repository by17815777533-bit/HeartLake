/**
 * RBACManager 模块接口定义
 */

#pragma once

#ifdef _WIN32
#undef DELETE
#endif

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <json/json.h>

namespace heartlake {
namespace utils {

/**
 * RBAC权限管理器 - 基于角色的访问控制
 * 
 * 角色层级:
 * - super_admin: 超级管理员（所有权限）
 * - admin: 管理员（用户管理、内容审核）
 * - moderator: 审核员（内容审核）
 * - analyst: 分析师（只读数据）
 */
/**
 * 基于角色的访问控制管理器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class RBACManager {
public:
    static RBACManager& getInstance();
    
    /**
     * initialize方法
     */
    void initialize();
    
    /**
     * hasPermission方法
     *
     * @param role 参数说明
     * @param resource 参数说明
     * @param action 参数说明
     * @return 返回值说明
     */
    bool hasPermission(const std::string& role, const std::string& resource, const std::string& action);
    
    std::vector<std::string> getRolePermissions(const std::string& role);
    
    /**
     * checkPathPermission方法
     *
     * @param role 参数说明
     * @param path 参数说明
     * @param method 参数说明
     * @return 返回值说明
     */
    bool checkPathPermission(const std::string& role, const std::string& path, const std::string& method);
    
    std::vector<std::string> getAllRoles();
    /**
     * isValidRole方法
     *
     * @param role 参数说明
     * @return 返回值说明
     */
    bool isValidRole(const std::string& role);
    /**
     * getRoleLevel方法
     *
     * @param role 参数说明
     * @return 返回值说明
     */
    int getRoleLevel(const std::string& role);
    
private:
    RBACManager() = default;
    
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_set<std::string>>> permissions_;
    
    std::unordered_map<std::string, int> roleLevels_;
    
    std::string pathToResource(const std::string& path);
    std::string methodToAction(const std::string& method);
};

namespace Permissions {
    constexpr const char* USERS = "users";
    constexpr const char* CONTENT = "content";
    constexpr const char* REPORTS = "reports";
    constexpr const char* SETTINGS = "settings";
    constexpr const char* STATISTICS = "statistics";
    constexpr const char* AI = "ai";
    constexpr const char* SYSTEM = "system";
    
    constexpr const char* READ = "read";
    constexpr const char* CREATE = "create";
    constexpr const char* UPDATE = "update";
    constexpr const char* DELETE = "delete";
    constexpr const char* MANAGE = "manage";
}

namespace Roles {
    constexpr const char* SUPER_ADMIN = "super_admin";
    constexpr const char* ADMIN = "admin";
    constexpr const char* MODERATOR = "moderator";
    constexpr const char* ANALYST = "analyst";
}

} // namespace utils
} // namespace heartlake
