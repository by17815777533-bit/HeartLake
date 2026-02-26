/**
 * @file test_paseto_extended.cpp
 * @brief PasetoUtil 扩展单元测试
 *
 * 覆盖：
 * 1. admin token 生成和验证
 * 2. token 过期检测
 * 3. token 篡改检测
 * 4. 空 payload 处理
 * 5. 不同密钥验证失败
 * 6. extractToken 从请求头提取
 * 7. userId/role 字段验证
 * 8. base64url 编解码
 * 9. 普通用户 token 生成验证
 */

#include <gtest/gtest.h>
#include "utils/PasetoUtil.h"
#include <drogon/drogon.h>
#include <set>
#include <thread>
#include <chrono>

using namespace heartlake::utils;
using namespace drogon;

// ============================================================================
// PasetoUtil 扩展测试
// ============================================================================

class PasetoExtendedTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        setenv("PASETO_KEY", "test_paseto_key_32bytes_long!!!!!", 1);
        setenv("ADMIN_PASETO_KEY", "test_admin_key_32bytes_long!!!!!", 1);
    }

    std::string userKey() { return PasetoUtil::getKey(); }
    std::string adminKey() { return PasetoUtil::getAdminKey(); }
};

// ---------- base64url 编解码 ----------

// 1. base64url 编解码往返
TEST_F(PasetoExtendedTest, Base64urlRoundTrip) {
    std::string original = "HeartLake PASETO v4 测试数据";
    std::string encoded = PasetoUtil::base64urlEncode(original);
    std::string decoded = PasetoUtil::base64urlDecode(encoded);
    EXPECT_EQ(decoded, original);
}

// 2. base64url 编码不含 +/= 字符
TEST_F(PasetoExtendedTest, Base64urlNoStandardChars) {
    std::string data = "binary\x00\x01\x02\xff\xfe data with special chars";
    std::string encoded = PasetoUtil::base64urlEncode(data);
    EXPECT_EQ(encoded.find('+'), std::string::npos);
    EXPECT_EQ(encoded.find('/'), std::string::npos);
    EXPECT_EQ(encoded.find('='), std::string::npos);
}

// 3. 空字符串编解码
TEST_F(PasetoExtendedTest, Base64urlEmptyString) {
    std::string encoded = PasetoUtil::base64urlEncode("");
    EXPECT_TRUE(encoded.empty());
    std::string decoded = PasetoUtil::base64urlDecode("");
    EXPECT_TRUE(decoded.empty());
}

// ---------- 普通用户 token ----------

// 4. 生成的 token 以 v4.local. 开头
TEST_F(PasetoExtendedTest, TokenHasCorrectHeader) {
    std::string token = PasetoUtil::generateToken("user_001", userKey());
    EXPECT_EQ(token.find("v4.local."), 0u);
}

// 5. 生成并验证普通用户 token
TEST_F(PasetoExtendedTest, GenerateAndVerifyUserToken) {
    std::string token = PasetoUtil::generateToken("user_test_123", userKey());
    std::string userId = PasetoUtil::verifyToken(token, userKey());
    EXPECT_EQ(userId, "user_test_123");
}

// 6. 不同用户生成不同 token
TEST_F(PasetoExtendedTest, DifferentUsersGetDifferentTokens) {
    std::string t1 = PasetoUtil::generateToken("user_a", userKey());
    std::string t2 = PasetoUtil::generateToken("user_b", userKey());
    EXPECT_NE(t1, t2);
}

// 7. 同一用户多次生成的 token 不同（因为时间戳和 nonce）
TEST_F(PasetoExtendedTest, SameUserGetsDifferentTokensEachTime) {
    std::set<std::string> tokens;
    for (int i = 0; i < 10; ++i) {
        tokens.insert(PasetoUtil::generateToken("user_same", userKey()));
    }
    EXPECT_EQ(tokens.size(), 10u);
}

// ---------- Admin token ----------

// 8. admin token 生成和验证（三参数版本）
TEST_F(PasetoExtendedTest, AdminTokenGenerateAndVerify) {
    std::string token = PasetoUtil::generateAdminToken("admin_001", "super_admin", adminKey());
    std::string result = PasetoUtil::verifyAdminToken(token, adminKey());
    EXPECT_FALSE(result.empty());
}

// 9. admin token 验证提取 adminId 和 role（四参数版本）
TEST_F(PasetoExtendedTest, AdminTokenExtractsIdAndRole) {
    std::string token = PasetoUtil::generateAdminToken("admin_002", "moderator", adminKey());
    std::string adminId, role;
    bool ok = PasetoUtil::verifyAdminToken(token, adminKey(), adminId, role);
    EXPECT_TRUE(ok);
    EXPECT_EQ(adminId, "admin_002");
    EXPECT_EQ(role, "moderator");
}

// 10. admin token 不同角色正确保存
TEST_F(PasetoExtendedTest, AdminTokenPreservesRole) {
    std::vector<std::string> roles = {"super_admin", "admin", "moderator", "analyst"};
    for (const auto& r : roles) {
        std::string token = PasetoUtil::generateAdminToken("admin_x", r, adminKey());
        std::string adminId, role;
        bool ok = PasetoUtil::verifyAdminToken(token, adminKey(), adminId, role);
        EXPECT_TRUE(ok) << "Failed for role: " << r;
        EXPECT_EQ(role, r);
    }
}

// ---------- Token 过期 ----------

// 11. 过期 token 验证失败（用户 token）
TEST_F(PasetoExtendedTest, ExpiredUserTokenRejected) {
    // 生成一个立即过期的 token（0小时有效期，实际上 exp = now）
    // 由于 generateToken 最小单位是小时，我们用一个 trick：
    // 生成 token 后手动等待不现实，所以测试 verifyToken 对过期的处理
    // 这里用 admin token 的方式测试过期逻辑
    std::string token = PasetoUtil::generateToken("user_exp", userKey(), 0);
    // expireHours=0 意味着 exp = now，验证时 now >= exp 应该失败
    // 等一小段时间确保过期
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_THROW(PasetoUtil::verifyToken(token, userKey()), std::runtime_error);
}

// 12. 过期 admin token 验证失败
TEST_F(PasetoExtendedTest, ExpiredAdminTokenRejected) {
    std::string token = PasetoUtil::generateAdminToken("admin_exp", "admin", adminKey(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string adminId, role;
    bool ok = PasetoUtil::verifyAdminToken(token, adminKey(), adminId, role);
    EXPECT_FALSE(ok);
}

// ---------- Token 篡改 ----------

// 13. 篡改 token payload 验证失败
TEST_F(PasetoExtendedTest, TamperedTokenRejected) {
    std::string token = PasetoUtil::generateToken("user_tamper", userKey());
    // 修改 token 中间部分
    size_t headerLen = std::string("v4.local.").size();
    if (token.size() > headerLen + 10) {
        token[headerLen + 5] ^= 0xFF;
    }
    EXPECT_THROW(PasetoUtil::verifyToken(token, userKey()), std::runtime_error);
}

// 14. 截断 token 验证失败
TEST_F(PasetoExtendedTest, TruncatedTokenRejected) {
    std::string token = PasetoUtil::generateToken("user_trunc", userKey());
    std::string truncated = token.substr(0, token.size() / 2);
    EXPECT_THROW(PasetoUtil::verifyToken(truncated, userKey()), std::runtime_error);
}

// ---------- 错误密钥 ----------

// 15. 用错误密钥验证用户 token 失败
TEST_F(PasetoExtendedTest, WrongKeyRejectsUserToken) {
    std::string token = PasetoUtil::generateToken("user_wk", userKey());
    std::string wrongKey = "wrong_key_32bytes_long_enough!!!";
    EXPECT_THROW(PasetoUtil::verifyToken(token, wrongKey), std::runtime_error);
}

// 16. 用用户密钥验证 admin token 失败
TEST_F(PasetoExtendedTest, UserKeyRejectsAdminToken) {
    std::string token = PasetoUtil::generateAdminToken("admin_cross", "admin", adminKey());
    std::string adminId, role;
    bool ok = PasetoUtil::verifyAdminToken(token, userKey(), adminId, role);
    EXPECT_FALSE(ok);
}

// ---------- 无效 token 格式 ----------

// 17. 完全无效的字符串
TEST_F(PasetoExtendedTest, GarbageTokenRejected) {
    EXPECT_THROW(PasetoUtil::verifyToken("not_a_token", userKey()), std::runtime_error);
}

// 18. 只有 header 没有 payload
TEST_F(PasetoExtendedTest, HeaderOnlyTokenRejected) {
    EXPECT_THROW(PasetoUtil::verifyToken("v4.local.", userKey()), std::runtime_error);
}

// ---------- extractToken ----------

// 19. extractToken 从 Bearer 头提取 token
TEST_F(PasetoExtendedTest, ExtractTokenFromBearerHeader) {
    auto req = HttpRequest::newHttpRequest();
    std::string expectedToken = PasetoUtil::generateToken("user_ext", userKey());
    req->addHeader("Authorization", "Bearer " + expectedToken);

    std::string extracted = PasetoUtil::extractToken(req);
    EXPECT_EQ(extracted, expectedToken);
}

// 20. extractToken 无 Authorization 头抛异常
TEST_F(PasetoExtendedTest, ExtractTokenMissingHeaderThrows) {
    auto req = HttpRequest::newHttpRequest();
    EXPECT_THROW(PasetoUtil::extractToken(req), std::exception);
}

// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
