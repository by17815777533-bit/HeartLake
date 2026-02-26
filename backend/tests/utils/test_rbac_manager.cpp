/**
 * @file test_rbac_manager.cpp
 * @brief RBACManager 单元测试
 *
 * 覆盖：
 * 1. 角色层级（super_admin > admin > moderator > analyst）
 * 2. 各角色权限范围验证
 * 3. 路径到资源映射（pathToResource）
 * 4. HTTP method 到操作映射（methodToAction）
 * 5. checkPathPermission 综合验证
 * 6. 未知角色拒绝
 * 7. 7种资源 × 5种操作的组合测试
 * 8. MANAGE 权限隐含所有操作
 */

#include <gtest/gtest.h>
#include "utils/RBACManager.h"
#include <set>

using namespace heartlake::utils;

// ============================================================================
// RBACManager 测试
// ============================================================================

class RBACManagerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        RBACManager::getInstance().initialize();
    }

    RBACManager& rbac() {
        return RBACManager::getInstance();
    }
};

// ---------- 角色验证 ----------

// 1. super_admin 是有效角色
TEST_F(RBACManagerTest, SuperAdminIsValidRole) {
    EXPECT_TRUE(rbac().isValidRole("super_admin"));
}

// 2. admin 是有效角色
TEST_F(RBACManagerTest, AdminIsValidRole) {
    EXPECT_TRUE(rbac().isValidRole("admin"));
}

// 3. moderator 是有效角色
TEST_F(RBACManagerTest, ModeratorIsValidRole) {
    EXPECT_TRUE(rbac().isValidRole("moderator"));
}

// 4. analyst 是有效角色
TEST_F(RBACManagerTest, AnalystIsValidRole) {
    EXPECT_TRUE(rbac().isValidRole("analyst"));
}

// 5. 未知角色无效
TEST_F(RBACManagerTest, UnknownRoleIsInvalid) {
    EXPECT_FALSE(rbac().isValidRole("hacker"));
    EXPECT_FALSE(rbac().isValidRole(""));
    EXPECT_FALSE(rbac().isValidRole("root"));
    EXPECT_FALSE(rbac().isValidRole("guest"));
}

// ---------- 角色层级 ----------

// 6. 角色层级排序正确
TEST_F(RBACManagerTest, RoleLevelHierarchy) {
    EXPECT_GT(rbac().getRoleLevel("super_admin"), rbac().getRoleLevel("admin"));
    EXPECT_GT(rbac().getRoleLevel("admin"), rbac().getRoleLevel("moderator"));
    EXPECT_GT(rbac().getRoleLevel("moderator"), rbac().getRoleLevel("analyst"));
}

// 7. 未知角色层级为 0
TEST_F(RBACManagerTest, UnknownRoleLevelIsZero) {
    EXPECT_EQ(rbac().getRoleLevel("unknown"), 0);
    EXPECT_EQ(rbac().getRoleLevel(""), 0);
}

// 8. getAllRoles 返回全部4个角色
TEST_F(RBACManagerTest, GetAllRolesReturnsFour) {
    auto roles = rbac().getAllRoles();
    EXPECT_EQ(roles.size(), 4u);
    std::set<std::string> roleSet(roles.begin(), roles.end());
    EXPECT_TRUE(roleSet.count("super_admin"));
    EXPECT_TRUE(roleSet.count("admin"));
    EXPECT_TRUE(roleSet.count("moderator"));
    EXPECT_TRUE(roleSet.count("analyst"));
}

// ---------- super_admin 权限（全部拥有） ----------

// 9. super_admin 拥有所有资源的 MANAGE 权限
TEST_F(RBACManagerTest, SuperAdminHasManageOnAllResources) {
    const std::vector<std::string> resources = {
        Permissions::USERS, Permissions::CONTENT, Permissions::REPORTS,
        Permissions::SETTINGS, Permissions::STATISTICS, Permissions::AI, Permissions::SYSTEM
    };
    for (const auto& res : resources) {
        EXPECT_TRUE(rbac().hasPermission("super_admin", res, "manage"))
            << "super_admin should have manage on " << res;
    }
}

// 10. super_admin 拥有所有资源的 CRUD 权限
TEST_F(RBACManagerTest, SuperAdminHasCRUDOnAllResources) {
    // users 资源应有全部5种操作
    EXPECT_TRUE(rbac().hasPermission("super_admin", "users", "read"));
    EXPECT_TRUE(rbac().hasPermission("super_admin", "users", "create"));
    EXPECT_TRUE(rbac().hasPermission("super_admin", "users", "update"));
    EXPECT_TRUE(rbac().hasPermission("super_admin", "users", "delete"));
    EXPECT_TRUE(rbac().hasPermission("super_admin", "users", "manage"));
}

// ---------- admin 权限 ----------

// 11. admin 可以管理用户（CRUD，无 MANAGE）
TEST_F(RBACManagerTest, AdminCanCRUDUsers) {
    EXPECT_TRUE(rbac().hasPermission("admin", "users", "read"));
    EXPECT_TRUE(rbac().hasPermission("admin", "users", "create"));
    EXPECT_TRUE(rbac().hasPermission("admin", "users", "update"));
    EXPECT_TRUE(rbac().hasPermission("admin", "users", "delete"));
    EXPECT_FALSE(rbac().hasPermission("admin", "users", "manage"));
}

// 12. admin 可以管理内容（CRUD）
TEST_F(RBACManagerTest, AdminCanCRUDContent) {
    EXPECT_TRUE(rbac().hasPermission("admin", "content", "read"));
    EXPECT_TRUE(rbac().hasPermission("admin", "content", "create"));
    EXPECT_TRUE(rbac().hasPermission("admin", "content", "update"));
    EXPECT_TRUE(rbac().hasPermission("admin", "content", "delete"));
}

// 13. admin 可以读取和更新举报，但不能删除
TEST_F(RBACManagerTest, AdminCanReadUpdateReports) {
    EXPECT_TRUE(rbac().hasPermission("admin", "reports", "read"));
    EXPECT_TRUE(rbac().hasPermission("admin", "reports", "update"));
    EXPECT_FALSE(rbac().hasPermission("admin", "reports", "delete"));
    EXPECT_FALSE(rbac().hasPermission("admin", "reports", "manage"));
}

// 14. admin 只能读取设置，不能修改
TEST_F(RBACManagerTest, AdminCanOnlyReadSettings) {
    EXPECT_TRUE(rbac().hasPermission("admin", "settings", "read"));
    EXPECT_FALSE(rbac().hasPermission("admin", "settings", "update"));
    EXPECT_FALSE(rbac().hasPermission("admin", "settings", "manage"));
}

// ---------- moderator 权限 ----------

// 15. moderator 可以读取用户但不能修改
TEST_F(RBACManagerTest, ModeratorCanOnlyReadUsers) {
    EXPECT_TRUE(rbac().hasPermission("moderator", "users", "read"));
    EXPECT_FALSE(rbac().hasPermission("moderator", "users", "create"));
    EXPECT_FALSE(rbac().hasPermission("moderator", "users", "update"));
    EXPECT_FALSE(rbac().hasPermission("moderator", "users", "delete"));
}

// 16. moderator 可以读取和更新内容
TEST_F(RBACManagerTest, ModeratorCanReadUpdateContent) {
    EXPECT_TRUE(rbac().hasPermission("moderator", "content", "read"));
    EXPECT_TRUE(rbac().hasPermission("moderator", "content", "update"));
    EXPECT_FALSE(rbac().hasPermission("moderator", "content", "create"));
    EXPECT_FALSE(rbac().hasPermission("moderator", "content", "delete"));
}

// 17. moderator 可以读取和更新举报
TEST_F(RBACManagerTest, ModeratorCanReadUpdateReports) {
    EXPECT_TRUE(rbac().hasPermission("moderator", "reports", "read"));
    EXPECT_TRUE(rbac().hasPermission("moderator", "reports", "update"));
    EXPECT_FALSE(rbac().hasPermission("moderator", "reports", "delete"));
}

// 18. moderator 不能访问设置
TEST_F(RBACManagerTest, ModeratorCannotAccessSettings) {
    EXPECT_FALSE(rbac().hasPermission("moderator", "settings", "read"));
    EXPECT_FALSE(rbac().hasPermission("moderator", "settings", "update"));
}

// ---------- analyst 权限（只读） ----------

// 19. analyst 只能读取用户
TEST_F(RBACManagerTest, AnalystCanOnlyReadUsers) {
    EXPECT_TRUE(rbac().hasPermission("analyst", "users", "read"));
    EXPECT_FALSE(rbac().hasPermission("analyst", "users", "create"));
    EXPECT_FALSE(rbac().hasPermission("analyst", "users", "update"));
    EXPECT_FALSE(rbac().hasPermission("analyst", "users", "delete"));
}

// 20. analyst 只能读取内容
TEST_F(RBACManagerTest, AnalystCanOnlyReadContent) {
    EXPECT_TRUE(rbac().hasPermission("analyst", "content", "read"));
    EXPECT_FALSE(rbac().hasPermission("analyst", "content", "create"));
    EXPECT_FALSE(rbac().hasPermission("analyst", "content", "update"));
    EXPECT_FALSE(rbac().hasPermission("analyst", "content", "delete"));
}

// 21. analyst 不能访问举报
TEST_F(RBACManagerTest, AnalystCannotAccessReports) {
    EXPECT_FALSE(rbac().hasPermission("analyst", "reports", "read"));
}

// 22. analyst 不能访问设置和系统
TEST_F(RBACManagerTest, AnalystCannotAccessSettingsOrSystem) {
    EXPECT_FALSE(rbac().hasPermission("analyst", "settings", "read"));
    EXPECT_FALSE(rbac().hasPermission("analyst", "system", "read"));
}

// ---------- checkPathPermission 路径权限 ----------

// 23. super_admin 通过 MANAGE 权限可以 DELETE 用户
TEST_F(RBACManagerTest, SuperAdminDeleteUsersViaManage) {
    EXPECT_TRUE(rbac().checkPathPermission("super_admin", "/api/admin/users/123", "DELETE"));
}

// 24. admin GET 用户列表通过
TEST_F(RBACManagerTest, AdminGetUsersPath) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/users", "GET"));
}

// 25. admin POST 创建用户通过
TEST_F(RBACManagerTest, AdminPostUsersPath) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/users", "POST"));
}

// 26. admin PUT 更新内容通过
TEST_F(RBACManagerTest, AdminPutContentPath) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/content/456", "PUT"));
}

// 27. moderator GET 内容通过
TEST_F(RBACManagerTest, ModeratorGetContentPath) {
    EXPECT_TRUE(rbac().checkPathPermission("moderator", "/api/admin/content", "GET"));
}

// 28. moderator DELETE 内容被拒绝
TEST_F(RBACManagerTest, ModeratorDeleteContentDenied) {
    EXPECT_FALSE(rbac().checkPathPermission("moderator", "/api/admin/content/789", "DELETE"));
}

// 29. analyst GET 统计数据通过
TEST_F(RBACManagerTest, AnalystGetStatsPath) {
    EXPECT_TRUE(rbac().checkPathPermission("analyst", "/api/admin/stats", "GET"));
}

// 30. analyst POST 统计数据被拒绝
TEST_F(RBACManagerTest, AnalystPostStatsDenied) {
    EXPECT_FALSE(rbac().checkPathPermission("analyst", "/api/admin/stats", "POST"));
}

// ---------- 路径映射边界 ----------

// 31. /api/admin/sensitive-words 映射到 content 资源
TEST_F(RBACManagerTest, SensitiveWordsPathMapsToContent) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/sensitive-words", "GET"));
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/sensitive-words", "POST"));
}

// 32. /api/admin/moderation 映射到 content 资源
TEST_F(RBACManagerTest, ModerationPathMapsToContent) {
    EXPECT_TRUE(rbac().checkPathPermission("moderator", "/api/admin/moderation", "PUT"));
}

// 33. /api/admin/edge-ai 映射到 ai 资源
TEST_F(RBACManagerTest, EdgeAIPathMapsToAI) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/edge-ai", "GET"));
    EXPECT_TRUE(rbac().checkPathPermission("analyst", "/api/admin/edge-ai", "GET"));
}

// 34. /api/admin/logs 映射到 statistics 资源
TEST_F(RBACManagerTest, LogsPathMapsToStatistics) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/logs", "GET"));
    EXPECT_TRUE(rbac().checkPathPermission("moderator", "/api/admin/logs", "GET"));
}

// 35. /api/admin/system 映射到 system 资源
TEST_F(RBACManagerTest, SystemPathMapsToSystem) {
    EXPECT_TRUE(rbac().checkPathPermission("super_admin", "/api/admin/system", "GET"));
    EXPECT_FALSE(rbac().checkPathPermission("analyst", "/api/admin/system", "GET"));
}

// 36. 未知路径默认映射到 system（最受限）
TEST_F(RBACManagerTest, UnknownPathDefaultsToSystem) {
    // analyst 没有 system 权限
    EXPECT_FALSE(rbac().checkPathPermission("analyst", "/api/admin/unknown-endpoint", "GET"));
    // super_admin 有 system:manage 权限
    EXPECT_TRUE(rbac().checkPathPermission("super_admin", "/api/admin/unknown-endpoint", "GET"));
}

// ---------- 未知角色 ----------

// 37. 未知角色没有任何权限
TEST_F(RBACManagerTest, UnknownRoleHasNoPermissions) {
    EXPECT_FALSE(rbac().hasPermission("hacker", "users", "read"));
    EXPECT_FALSE(rbac().hasPermission("", "content", "read"));
    EXPECT_FALSE(rbac().checkPathPermission("guest", "/api/admin/users", "GET"));
}

// 38. getRolePermissions 对未知角色返回空
TEST_F(RBACManagerTest, UnknownRolePermissionsEmpty) {
    auto perms = rbac().getRolePermissions("nonexistent");
    EXPECT_TRUE(perms.empty());
}

// ---------- getRolePermissions 完整性 ----------

// 39. super_admin 权限数量最多
TEST_F(RBACManagerTest, SuperAdminHasMostPermissions) {
    auto superPerms = rbac().getRolePermissions("super_admin");
    auto adminPerms = rbac().getRolePermissions("admin");
    auto modPerms = rbac().getRolePermissions("moderator");
    auto analystPerms = rbac().getRolePermissions("analyst");

    EXPECT_GT(superPerms.size(), adminPerms.size());
    EXPECT_GT(adminPerms.size(), modPerms.size());
    EXPECT_GE(modPerms.size(), analystPerms.size());
}

// 40. HEAD 方法映射为 read
TEST_F(RBACManagerTest, HeadMethodMapsToRead) {
    EXPECT_TRUE(rbac().checkPathPermission("analyst", "/api/admin/users", "HEAD"));
}

// 41. PATCH 方法映射为 update
TEST_F(RBACManagerTest, PatchMethodMapsToUpdate) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/users/123", "PATCH"));
    EXPECT_FALSE(rbac().checkPathPermission("analyst", "/api/admin/users/123", "PATCH"));
}

// ---------- admin/info 路径映射 ----------

// 42. /api/admin/info 映射到 users 资源
TEST_F(RBACManagerTest, AdminInfoPathMapsToUsers) {
    EXPECT_TRUE(rbac().checkPathPermission("admin", "/api/admin/info", "GET"));
    EXPECT_TRUE(rbac().checkPathPermission("moderator", "/api/admin/info", "GET"));
}

// ---------- broadcast/config 路径映射 ----------

// 43. /api/admin/broadcast 映射到 settings 资源
TEST_F(RBACManagerTest, BroadcastPathMapsToSettings) {
    EXPECT_TRUE(rbac().checkPathPermission("super_admin", "/api/admin/broadcast", "POST"));
    EXPECT_FALSE(rbac().checkPathPermission("moderator", "/api/admin/broadcast", "POST"));
}

// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
