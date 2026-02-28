/**
 * @brief 基于角色的访问控制（RBAC）管理器
 *
 * 管理后台的权限控制核心，采用 资源-动作 二维权限模型。
 * 角色按层级从高到低排列：
 * - super_admin: 超级管理员，拥有全部权限
 * - admin: 管理员，可管理用户和内容审核
 * - moderator: 审核员，仅限内容审核操作
 * - analyst: 分析师，只读访问统计数据
 *
 * @details 权限检查支持两种粒度：
 *   1. 资源+动作 精确匹配（hasPermission）
 *   2. HTTP 路径+方法 自动映射（checkPathPermission），
 *      内部将 REST 路径转换为资源名、HTTP 方法转换为动作名
 *
 * @note 单例模式，initialize() 需在服务启动时调用一次
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

class RBACManager {
public:
    static RBACManager& getInstance();

    /// 加载角色-权限映射表和角色层级配置，服务启动时调用
    void initialize();

    /**
     * @brief 检查指定角色是否拥有对某资源执行某动作的权限
     * @param role 角色标识（如 "admin"、"moderator"）
     * @param resource 资源名称（如 "users"、"content"）
     * @param action 动作名称（如 "read"、"delete"）
     * @return 拥有权限返回 true
     */
    bool hasPermission(const std::string& role, const std::string& resource, const std::string& action);

    /// 获取指定角色拥有的所有权限描述列表（格式 "resource:action"）
    std::vector<std::string> getRolePermissions(const std::string& role);

    /**
     * @brief 根据 HTTP 路径和方法检查权限（供 Filter 层直接调用）
     * @param role 角色标识
     * @param path 请求路径（如 "/api/admin/users"）
     * @param method HTTP 方法（如 "GET"、"POST"）
     * @return 允许访问返回 true
     */
    bool checkPathPermission(const std::string& role, const std::string& path, const std::string& method);

    /// 返回系统中定义的全部角色标识
    std::vector<std::string> getAllRoles();

    /**
     * @brief 判断角色标识是否合法
     * @param role 待验证的角色字符串
     * @return 存在于角色表中返回 true
     */
    bool isValidRole(const std::string& role);

    /**
     * @brief 获取角色的数值层级，用于比较权限高低
     * @param role 角色标识
     * @return 层级数值，super_admin 最高；未知角色返回 0
     */
    int getRoleLevel(const std::string& role);

private:
    RBACManager() = default;

    /// 三层嵌套: role -> resource -> {action set}
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_set<std::string>>> permissions_;

    /// 角色 -> 数值层级映射
    std::unordered_map<std::string, int> roleLevels_;

    /// 将 REST 路径映射为资源名（如 "/api/admin/users/123" -> "users"）
    std::string pathToResource(const std::string& path);
    /// 将 HTTP 方法映射为动作名（如 "DELETE" -> "delete"）
    std::string methodToAction(const std::string& method);
};

/// 资源名称和动作名称常量，与权限表中的 key 一一对应
namespace Permissions {
    // ---- 资源 ----
    constexpr const char* USERS = "users";
    constexpr const char* CONTENT = "content";
    constexpr const char* REPORTS = "reports";
    constexpr const char* SETTINGS = "settings";
    constexpr const char* STATISTICS = "statistics";
    constexpr const char* AI = "ai";
    constexpr const char* SYSTEM = "system";

    // ---- 动作 ----
    constexpr const char* READ = "read";
    constexpr const char* CREATE = "create";
    constexpr const char* UPDATE = "update";
    constexpr const char* DELETE = "delete";
    constexpr const char* MANAGE = "manage";   ///< 包含 CRUD 全部操作的超集
}

/// 角色标识常量
namespace Roles {
    constexpr const char* SUPER_ADMIN = "super_admin";
    constexpr const char* ADMIN = "admin";
    constexpr const char* MODERATOR = "moderator";
    constexpr const char* ANALYST = "analyst";
}

} // namespace utils
} // namespace heartlake
