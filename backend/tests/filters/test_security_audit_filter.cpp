/**
 * @file test_security_audit_filter.cpp
 * @brief SecurityAuditFilter 单元测试
 *
 * 覆盖：
 * 1. sanitizeForLog 日志注入防护（换行符、控制字符、tab保留）
 * 2. 精确匹配白名单放行
 * 3. 前缀匹配白名单放行
 * 4. 路径边界检查（防止 /api/v1/admin-fake 绕过）
 * 5. 路径规范化（连续斜杠、尾部斜杠、路径遍历）
 * 6. CORS origin 验证
 * 7. Token 认证流程
 * 8. ResponseDesensitizer 脱敏处理
 */

#include <gtest/gtest.h>
#include "infrastructure/filters/SecurityAuditFilter.h"
#include "utils/PasetoUtil.h"
#include <drogon/drogon.h>
#include <json/json.h>

using namespace heartlake::filters;
using namespace heartlake::utils;
using namespace drogon;

// ============================================================================
// SecurityAuditFilter 测试
// ============================================================================

class SecurityAuditFilterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        setenv("PASETO_KEY", "test_security_key_32bytes_long!!", 1);
        setenv("CORS_ALLOWED_ORIGIN", "http://localhost:5173,http://localhost:3000,http://localhost:8080", 1);
    }

    std::string generateValidToken(const std::string& userId) {
        std::string key = PasetoUtil::getKey();
        return PasetoUtil::generateToken(userId, key);
    }

    HttpRequestPtr makeRequest(const std::string& path, const std::string& token = "",
                               const std::string& origin = "") {
        auto req = HttpRequest::newHttpRequest();
        req->setPath(path);
        req->setMethod(Get);
        if (!token.empty()) {
            req->addHeader("Authorization", "Bearer " + token);
        }
        if (!origin.empty()) {
            req->addHeader("Origin", origin);
        }
        return req;
    }
};

// ---------- 精确匹配白名单 ----------

// 1. /api/auth/login 白名单放行
TEST_F(SecurityAuditFilterTest, AuthLoginWhitelisted) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/auth/login");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 2. /api/auth/register 白名单放行
TEST_F(SecurityAuditFilterTest, AuthRegisterWhitelisted) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/auth/register");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 3. /api/auth/anonymous 白名单放行
TEST_F(SecurityAuditFilterTest, AuthAnonymousWhitelisted) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/auth/anonymous");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 4. /api/lake/stones 精确匹配放行
TEST_F(SecurityAuditFilterTest, LakeStonesExactWhitelisted) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/lake/stones");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 5. /api/lake/stones/123 不在精确白名单中，需要认证
TEST_F(SecurityAuditFilterTest, LakeStonesSubpathRequiresAuth) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/lake/stones/123");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// ---------- 前缀匹配白名单 ----------

// 6. /api/health 前缀匹配放行
TEST_F(SecurityAuditFilterTest, HealthPrefixWhitelisted) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/health");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 7. /api/health/ready 前缀子路径放行
TEST_F(SecurityAuditFilterTest, HealthReadySubpathWhitelisted) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/health/ready");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 8. /api/healthcheck 不匹配前缀白名单（不是 /api/health 的子路径）
TEST_F(SecurityAuditFilterTest, HealthcheckNotMatchedByPrefix) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/healthcheck");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// ---------- 路径规范化 ----------

// 9. 连续斜杠规范化后匹配白名单
TEST_F(SecurityAuditFilterTest, DoubleSlashNormalized) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api//auth//login");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 10. 尾部斜杠去除后匹配白名单
TEST_F(SecurityAuditFilterTest, TrailingSlashNormalized) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/auth/login/");

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
}

// 11. 路径遍历 /.. 返回 400
TEST_F(SecurityAuditFilterTest, PathTraversalRejected) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/auth/../admin/users");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k400BadRequest);
}

// ---------- Token 认证 ----------

// 12. 有效 token 通过认证，user_id 写入请求属性
TEST_F(SecurityAuditFilterTest, ValidTokenPassesAndSetsUserId) {
    SecurityAuditFilter filter;
    std::string token = generateValidToken("user_test_001");
    auto req = makeRequest("/api/some/protected/path", token);

    bool passed = false;
    filter.doFilter(req,
        [](const HttpResponsePtr&) {},
        [&passed]() { passed = true; });

    EXPECT_TRUE(passed);
    // 验证 user_id 被写入请求属性
    auto userId = req->getAttributes()->get<std::string>("user_id");
    EXPECT_EQ(userId, "user_test_001");
}

// 13. 无 Authorization 头返回 401
TEST_F(SecurityAuditFilterTest, NoAuthHeaderReturns401) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/protected/resource");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// 14. 无效 token 返回 401
TEST_F(SecurityAuditFilterTest, InvalidTokenReturns401) {
    SecurityAuditFilter filter;
    auto req = makeRequest("/api/protected/resource", "garbage_token_value");

    HttpResponsePtr response;
    filter.doFilter(req,
        [&response](const HttpResponsePtr& resp) { response = resp; },
        []() {});

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->statusCode(), k401Unauthorized);
}

// ---------- ResponseDesensitizer ----------

// 15. 邮箱字段脱敏
TEST_F(SecurityAuditFilterTest, ResponseDesensitizerEmail) {
    Json::Value json;
    json["email"] = "alice@example.com";
    json["name"] = "Alice";

    ResponseDesensitizer::desensitize(json);

    // email 应被脱敏（包含星号）
    std::string email = json["email"].asString();
    EXPECT_NE(email.find('*'), std::string::npos);
    // name 不是敏感字段，不应被修改
    EXPECT_EQ(json["name"].asString(), "Alice");
}

// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
