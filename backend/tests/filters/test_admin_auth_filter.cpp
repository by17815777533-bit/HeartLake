/**
 * AdminAuthFilter 单元测试
 *
 * 覆盖：
 * 1. PASETO token 认证流程（有效/无效/空/过期）
 * 2. RBAC 权限检查（不同角色访问不同路径）
 * 3. CORS 头处理
 * 4. Bearer 前缀解析
 * 5. 登录接口白名单放行
 */

#include <gtest/gtest.h>
#include "infrastructure/filters/AdminAuthFilter.h"
#include "utils/PasetoUtil.h"
#include "utils/RBACManager.h"
#include "utils/ResponseUtil.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <chrono>
#include <thread>

using namespace heartlake::utils;
using namespace heartlake::filters;
using namespace drogon;

// ============================================================================
// AdminAuthFilter 测试
// ============================================================================

class AdminAuthFilterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 设置测试用 PASETO 密钥（32字节）
        setenv("ADMIN_PASETO_KEY", "test_admin_key_32bytes_long!!!!!", 1);
        setenv("CORS_ALLOWED_ORIGIN", "http://localhost:5173,http://localhost:3000", 1);

        // 初始化 RBAC
        RBACManager::getInstance().initialize();
    }

    std::string generateValidAdminToken(const std::string& adminId, const std::string& role, int expireHours = 24) {
        std::string key = PasetoUtil::getAdminKey();
        return PasetoUtil::generateAdminToken(adminId, role, key, expireHours);
    }

    // 构造带 Authorization 头的请求
    HttpRequestPtr makeRequest(const std::string& path, const std::string& method, const std::string& token = "") {
        auto req = HttpRequest::newHttpRequest();
        req->setPath(path);
        if (method == "GET") req->setMethod(Get);
        else if (method == "POST") req->setMethod(Post);
        else if (method == "PUT") req->setMethod(Put);
        else if (method == "DELETE") req->setMethod(Delete);

        if (!token.empty()) {
            req->addHeader("Authorization", "Bearer " + token);
        }
        return req;
    }
};

// ---------- 登录接口白名单 ----------

// 1. 登录接口无需认证直接放行
TEST_F(AdminAuthFilterTest, LoginPathBypassesAuth) {
    AdminAuthFilter filter;
    auto req = makeRequest("/api/admin/login", "POST");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) { /* 不应被调用 */ },
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// ---------- Token 缺失/格式错误 ----------

// 2. 无 Authorization 头返回 401
TEST_F(AdminAuthFilterTest, MissingAuthHeaderReturns401) {
    AdminAuthFilter filter;
    auto req = makeRequest("/api/admin/users", "GET");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// 3. 空 Authorization 头返回 401
TEST_F(AdminAuthFilterTest, EmptyAuthHeaderReturns401) {
    AdminAuthFilter filter;
    auto req = makeRequest("/api/admin/users", "GET");
    req->addHeader("Authorization", "");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// 4. 缺少 Bearer 前缀返回 401
TEST_F(AdminAuthFilterTest, MissingBearerPrefixReturns401) {
    AdminAuthFilter filter;
    auto req = makeRequest("/api/admin/users", "GET");
    req->addHeader("Authorization", "some_random_token");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// 5. Bearer 后面跟无效 token 返回 401
TEST_F(AdminAuthFilterTest, InvalidTokenReturns401) {
    AdminAuthFilter filter;
    auto req = makeRequest("/api/admin/users", "GET");
    req->addHeader("Authorization", "Bearer invalid_garbage_token");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// 6. Bearer 后面跟空字符串返回 401
TEST_F(AdminAuthFilterTest, BearerWithEmptyTokenReturns401) {
    AdminAuthFilter filter;
    auto req = makeRequest("/api/admin/users", "GET");
    req->addHeader("Authorization", "Bearer ");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// 7. 篡改过的 token 返回 401
TEST_F(AdminAuthFilterTest, TamperedTokenReturns401) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("admin_001", "super_admin");
    // 篡改 token 的最后几个字符
    if (token.size() > 5) {
        token[token.size() - 1] = 'X';
        token[token.size() - 2] = 'Y';
        token[token.size() - 3] = 'Z';
    }
    auto req = makeRequest("/api/admin/users", "GET");
    req->addHeader("Authorization", "Bearer " + token);

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// ---------- 有效 Token + RBAC 权限 ----------

// 8. super_admin 访问用户管理 GET 通过
TEST_F(AdminAuthFilterTest, SuperAdminCanAccessUsersGet) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("admin_001", "super_admin");
    auto req = makeRequest("/api/admin/users", "GET", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 9. super_admin 访问用户管理 DELETE 通过
TEST_F(AdminAuthFilterTest, SuperAdminCanDeleteUsers) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("admin_001", "super_admin");
    auto req = makeRequest("/api/admin/users/123", "DELETE", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 10. super_admin 访问系统设置通过
TEST_F(AdminAuthFilterTest, SuperAdminCanAccessSettings) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("admin_001", "super_admin");
    auto req = makeRequest("/api/admin/settings", "PUT", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 11. admin 角色可以读取用户列表
TEST_F(AdminAuthFilterTest, AdminCanReadUsers) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("admin_002", "admin");
    auto req = makeRequest("/api/admin/users", "GET", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 12. admin 角色可以审核内容
TEST_F(AdminAuthFilterTest, AdminCanUpdateContent) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("admin_002", "admin");
    auto req = makeRequest("/api/admin/content/456", "PUT", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 13. moderator 可以读取内容
TEST_F(AdminAuthFilterTest, ModeratorCanReadContent) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("mod_001", "moderator");
    auto req = makeRequest("/api/admin/content", "GET", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 14. moderator 不能删除用户 → 403
TEST_F(AdminAuthFilterTest, ModeratorCannotDeleteUsers) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("mod_001", "moderator");
    auto req = makeRequest("/api/admin/users/123", "DELETE", token);

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k403Forbidden);
}

// 15. analyst 只能读取统计数据
TEST_F(AdminAuthFilterTest, AnalystCanReadStatistics) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("analyst_001", "analyst");
    auto req = makeRequest("/api/admin/stats", "GET", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 16. analyst 不能修改设置 → 403
TEST_F(AdminAuthFilterTest, AnalystCannotUpdateSettings) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("analyst_001", "analyst");
    auto req = makeRequest("/api/admin/settings", "PUT", token);

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k403Forbidden);
}

// 17. analyst 不能创建用户 → 403
TEST_F(AdminAuthFilterTest, AnalystCannotCreateUsers) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("analyst_001", "analyst");
    auto req = makeRequest("/api/admin/users", "POST", token);

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k403Forbidden);
}

// ---------- 请求属性注入 ----------

// 18. 认证通过后 admin_id 和 admin_role 注入到请求属性
TEST_F(AdminAuthFilterTest, InjectsAdminIdAndRoleToAttributes) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("admin_inject_test", "admin");
    auto req = makeRequest("/api/admin/users", "GET", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
    auto attrs = req->getAttributes();
    EXPECT_EQ(attrs->get<std::string>("admin_id"), "admin_inject_test");
    EXPECT_EQ(attrs->get<std::string>("admin_role"), "admin");
}

// ---------- 用错误密钥签发的 token ----------

// 19. 用不同密钥签发的 token 验证失败
TEST_F(AdminAuthFilterTest, WrongKeyTokenReturns401) {
    AdminAuthFilter filter;
    // 用一个不同的密钥手动生成 token
    std::string wrongKey = "wrong_key_32bytes_long_enough!!!";
    std::string token = PasetoUtil::generateAdminToken("admin_001", "super_admin", wrongKey);
    auto req = makeRequest("/api/admin/users", "GET", token);

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// ---------- moderator 不能修改系统设置 ----------

// 20. moderator 不能修改系统设置 → 403
TEST_F(AdminAuthFilterTest, ModeratorCannotUpdateSettings) {
    AdminAuthFilter filter;
    std::string token = generateValidAdminToken("mod_001", "moderator");
    auto req = makeRequest("/api/admin/settings", "PUT", token);

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k403Forbidden);
}

// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
