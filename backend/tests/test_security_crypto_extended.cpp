/**
 * @file test_security_crypto_extended.cpp
 * @brief 安全加密模块扩展测试 - PASETO、PBKDF2、影子映射、恢复密钥
 */

#include <gtest/gtest.h>
#include "utils/PasetoUtil.h"
#include "utils/PasswordUtil.h"
#include "utils/E2EEncryption.h"
#include "utils/IdentityShadowMap.h"
#include "utils/RecoveryKeyGenerator.h"
#include <set>
#include <string>
#include <thread>
#include <chrono>

using namespace heartlake::utils;

class SecurityCryptoExtTest : public ::testing::Test {};

// =====================================================================
// PASETO Token
// =====================================================================

TEST_F(SecurityCryptoExtTest, Paseto_GenerateAndVerify) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_123", key, 1);
    EXPECT_FALSE(token.empty());
    auto userId = PasetoUtil::verifyToken(token, key);
    EXPECT_EQ(userId, "user_123");
}

TEST_F(SecurityCryptoExtTest, Paseto_HasV4LocalHeader) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_abc", key);
    EXPECT_EQ(token.substr(0, 9), "v4.local.");
}

TEST_F(SecurityCryptoExtTest, Paseto_WrongKey_Fails) {
    std::string key1 = "01234567890123456789012345678901";
    std::string key2 = "abcdefghijklmnopqrstuvwxyz012345";
    auto token = PasetoUtil::generateToken("user_x", key1);
    EXPECT_THROW(PasetoUtil::verifyToken(token, key2), std::runtime_error);
}

TEST_F(SecurityCryptoExtTest, Paseto_EmptyToken_Fails) {
    std::string key = "01234567890123456789012345678901";
    EXPECT_THROW(PasetoUtil::verifyToken("", key), std::runtime_error);
}

TEST_F(SecurityCryptoExtTest, Paseto_InvalidHeader_Fails) {
    std::string key = "01234567890123456789012345678901";
    EXPECT_THROW(PasetoUtil::verifyToken("v3.local.garbage", key), std::runtime_error);
}

TEST_F(SecurityCryptoExtTest, Paseto_TruncatedToken_Fails) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_trunc", key);
    std::string truncated = token.substr(0, token.length() / 2);
    EXPECT_THROW(PasetoUtil::verifyToken(truncated, key), std::runtime_error);
}

TEST_F(SecurityCryptoExtTest, Paseto_TamperedPayload_Fails) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_tamper", key);
    // Flip a character in the middle
    std::string tampered = token;
    size_t mid = tampered.length() / 2;
    tampered[mid] = (tampered[mid] == 'A') ? 'B' : 'A';
    EXPECT_THROW(PasetoUtil::verifyToken(tampered, key), std::runtime_error);
}

TEST_F(SecurityCryptoExtTest, Paseto_ExpiredToken_Fails) {
    std::string key = "01234567890123456789012345678901";
    // Generate token that expires in 0 hours (already expired or about to)
    auto token = PasetoUtil::generateToken("user_exp", key, 0);
    // Sleep briefly to ensure expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_THROW(PasetoUtil::verifyToken(token, key), std::runtime_error);
}

TEST_F(SecurityCryptoExtTest, Paseto_DifferentUsersGetDifferentTokens) {
    std::string key = "01234567890123456789012345678901";
    auto t1 = PasetoUtil::generateToken("user_a", key);
    auto t2 = PasetoUtil::generateToken("user_b", key);
    EXPECT_NE(t1, t2);
}

TEST_F(SecurityCryptoExtTest, Paseto_Base64UrlEncodeDecode) {
    std::string original = "Hello, World! 你好世界";
    auto encoded = PasetoUtil::base64urlEncode(original);
    auto decoded = PasetoUtil::base64urlDecode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(SecurityCryptoExtTest, Paseto_Base64UrlEmpty) {
    EXPECT_EQ(PasetoUtil::base64urlEncode(""), "");
    EXPECT_EQ(PasetoUtil::base64urlDecode(""), "");
}

TEST_F(SecurityCryptoExtTest, Paseto_Base64UrlNoPadding) {
    auto encoded = PasetoUtil::base64urlEncode("test");
    EXPECT_EQ(encoded.find('='), std::string::npos);
}

TEST_F(SecurityCryptoExtTest, Paseto_AdminToken_GenerateAndVerify) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateAdminToken("admin_1", "super_admin", key, 1);
    EXPECT_FALSE(token.empty());
    std::string adminId, role;
    bool ok = PasetoUtil::verifyAdminToken(token, key, adminId, role);
    EXPECT_TRUE(ok);
    EXPECT_EQ(adminId, "admin_1");
    EXPECT_EQ(role, "super_admin");
}

TEST_F(SecurityCryptoExtTest, Paseto_AdminToken_WrongKey_Fails) {
    std::string key1 = "01234567890123456789012345678901";
    std::string key2 = "abcdefghijklmnopqrstuvwxyz012345";
    auto token = PasetoUtil::generateAdminToken("admin_x", "admin", key1);
    std::string adminId, role;
    EXPECT_FALSE(PasetoUtil::verifyAdminToken(token, key2, adminId, role));
}

TEST_F(SecurityCryptoExtTest, Paseto_AdminToken_InvalidToken_Fails) {
    std::string key = "01234567890123456789012345678901";
    std::string adminId, role;
    EXPECT_FALSE(PasetoUtil::verifyAdminToken("garbage", key, adminId, role));
}

// =====================================================================
// PBKDF2 / PasswordUtil
// =====================================================================

TEST_F(SecurityCryptoExtTest, Password_HashAndVerify) {
    std::string salt, hash;
    PasswordUtil::generatePasswordHash("MyP@ssw0rd", salt, hash);
    EXPECT_FALSE(salt.empty());
    EXPECT_FALSE(hash.empty());
    EXPECT_TRUE(PasswordUtil::verifyPassword("MyP@ssw0rd", salt, hash));
}

TEST_F(SecurityCryptoExtTest, Password_WrongPassword_Fails) {
    std::string salt, hash;
    PasswordUtil::generatePasswordHash("correct", salt, hash);
    EXPECT_FALSE(PasswordUtil::verifyPassword("wrong", salt, hash));
}

TEST_F(SecurityCryptoExtTest, Password_EmptyPassword) {
    std::string salt, hash;
    PasswordUtil::generatePasswordHash("", salt, hash);
    EXPECT_TRUE(PasswordUtil::verifyPassword("", salt, hash));
    EXPECT_FALSE(PasswordUtil::verifyPassword("notempty", salt, hash));
}

TEST_F(SecurityCryptoExtTest, Password_LongPassword) {
    std::string longPwd(200, 'x');
    std::string salt, hash;
    PasswordUtil::generatePasswordHash(longPwd, salt, hash);
    EXPECT_TRUE(PasswordUtil::verifyPassword(longPwd, salt, hash));
}

TEST_F(SecurityCryptoExtTest, Password_UnicodePassword) {
    std::string salt, hash;
    PasswordUtil::generatePasswordHash("密码测试🔑", salt, hash);
    EXPECT_TRUE(PasswordUtil::verifyPassword("密码测试🔑", salt, hash));
    EXPECT_FALSE(PasswordUtil::verifyPassword("密码测试", salt, hash));
}

TEST_F(SecurityCryptoExtTest, Password_DifferentSalts) {
    auto salt1 = PasswordUtil::generateSalt();
    auto salt2 = PasswordUtil::generateSalt();
    EXPECT_NE(salt1, salt2);
}

TEST_F(SecurityCryptoExtTest, Password_SamePwdDifferentSalt_DifferentHash) {
    auto salt1 = PasswordUtil::generateSalt();
    auto salt2 = PasswordUtil::generateSalt();
    auto hash1 = PasswordUtil::hashPassword("same", salt1);
    auto hash2 = PasswordUtil::hashPassword("same", salt2);
    EXPECT_NE(hash1, hash2);
}

TEST_F(SecurityCryptoExtTest, Password_SamePwdSameSalt_SameHash) {
    auto salt = PasswordUtil::generateSalt();
    auto hash1 = PasswordUtil::hashPassword("same", salt);
    auto hash2 = PasswordUtil::hashPassword("same", salt);
    EXPECT_EQ(hash1, hash2);
}

TEST_F(SecurityCryptoExtTest, Password_SaltIsHex) {
    auto salt = PasswordUtil::generateSalt();
    for (char c : salt) {
        EXPECT_TRUE(std::isxdigit(c)) << "Non-hex in salt: " << c;
    }
}

TEST_F(SecurityCryptoExtTest, Password_HashIsHex) {
    auto salt = PasswordUtil::generateSalt();
    auto hash = PasswordUtil::hashPassword("test", salt);
    for (char c : hash) {
        EXPECT_TRUE(std::isxdigit(c)) << "Non-hex in hash: " << c;
    }
}

// =====================================================================
// E2EEncryption 扩展
// =====================================================================

TEST_F(SecurityCryptoExtTest, E2E_EncryptEmptyString) {
    auto key = E2EEncryption::generateKey();
    auto enc = E2EEncryption::encrypt("", key);
    ASSERT_TRUE(enc.has_value());
    auto dec = E2EEncryption::decrypt(*enc, key);
    ASSERT_TRUE(dec.has_value());
    EXPECT_EQ(*dec, "");
}

TEST_F(SecurityCryptoExtTest, E2E_EncryptLongMessage) {
    auto key = E2EEncryption::generateKey();
    std::string longMsg(10000, 'A');
    auto enc = E2EEncryption::encrypt(longMsg, key);
    ASSERT_TRUE(enc.has_value());
    auto dec = E2EEncryption::decrypt(*enc, key);
    ASSERT_TRUE(dec.has_value());
    EXPECT_EQ(*dec, longMsg);
}

TEST_F(SecurityCryptoExtTest, E2E_EncryptUnicode) {
    auto key = E2EEncryption::generateKey();
    std::string msg = "你好世界🌍端到端加密";
    auto enc = E2EEncryption::encrypt(msg, key);
    ASSERT_TRUE(enc.has_value());
    auto dec = E2EEncryption::decrypt(*enc, key);
    ASSERT_TRUE(dec.has_value());
    EXPECT_EQ(*dec, msg);
}

TEST_F(SecurityCryptoExtTest, E2E_TamperedCiphertext_Fails) {
    auto key = E2EEncryption::generateKey();
    auto enc = E2EEncryption::encrypt("secret", key);
    ASSERT_TRUE(enc.has_value());
    // Tamper with ciphertext
    std::string tampered = enc->ciphertext;
    if (!tampered.empty()) {
        tampered[0] = (tampered[0] == 'A') ? 'B' : 'A';
    }
    EncryptedMessage bad = {tampered, enc->iv, enc->tag};
    auto dec = E2EEncryption::decrypt(bad, key);
    EXPECT_FALSE(dec.has_value());
}

TEST_F(SecurityCryptoExtTest, E2E_TamperedTag_Fails) {
    auto key = E2EEncryption::generateKey();
    auto enc = E2EEncryption::encrypt("secret", key);
    ASSERT_TRUE(enc.has_value());
    std::string badTag = enc->tag;
    if (!badTag.empty()) {
        badTag[0] = (badTag[0] == 'A') ? 'B' : 'A';
    }
    EncryptedMessage bad = {enc->ciphertext, enc->iv, badTag};
    auto dec = E2EEncryption::decrypt(bad, key);
    EXPECT_FALSE(dec.has_value());
}

TEST_F(SecurityCryptoExtTest, E2E_DeriveSessionKey_DifferentSalts) {
    // deriveSessionKey expects base64-encoded inputs
    auto key1 = E2EEncryption::generateKey();
    auto key2 = E2EEncryption::generateKey();
    auto k1 = E2EEncryption::deriveSessionKey(key1, key1);
    auto k2 = E2EEncryption::deriveSessionKey(key1, key2);
    EXPECT_NE(k1, k2);
}

TEST_F(SecurityCryptoExtTest, E2E_DeriveSessionKey_DifferentSecrets) {
    auto key1 = E2EEncryption::generateKey();
    auto key2 = E2EEncryption::generateKey();
    auto salt = E2EEncryption::generateKey();
    auto k1 = E2EEncryption::deriveSessionKey(key1, salt);
    auto k2 = E2EEncryption::deriveSessionKey(key2, salt);
    EXPECT_NE(k1, k2);
}

// =====================================================================
// IdentityShadowMap 扩展
// =====================================================================

TEST_F(SecurityCryptoExtTest, Shadow_SameUserSameId) {
    auto& sm = IdentityShadowMap::getInstance();
    auto id1 = sm.getOrCreateShadowId("shadow_test_user");
    auto id2 = sm.getOrCreateShadowId("shadow_test_user");
    EXPECT_EQ(id1, id2);
}

TEST_F(SecurityCryptoExtTest, Shadow_DifferentUsersDifferentIds) {
    auto& sm = IdentityShadowMap::getInstance();
    auto id1 = sm.getOrCreateShadowId("shadow_user_a");
    auto id2 = sm.getOrCreateShadowId("shadow_user_b");
    EXPECT_NE(id1, id2);
}

TEST_F(SecurityCryptoExtTest, Shadow_EmptyUserId) {
    auto& sm = IdentityShadowMap::getInstance();
    EXPECT_NO_THROW(sm.getOrCreateShadowId(""));
}

TEST_F(SecurityCryptoExtTest, Shadow_RotateChangesId) {
    auto& sm = IdentityShadowMap::getInstance();
    auto old = sm.getOrCreateShadowId("shadow_rotate_ext");
    auto rotated = sm.rotateShadowId("shadow_rotate_ext");
    EXPECT_NE(old, rotated);
}

TEST_F(SecurityCryptoExtTest, Shadow_AnonymizeIp_NotEmpty) {
    auto anon = IdentityShadowMap::anonymizeIp("192.168.1.100");
    EXPECT_FALSE(anon.empty());
}

TEST_F(SecurityCryptoExtTest, Shadow_AnonymizeIp_Deterministic) {
    auto a1 = IdentityShadowMap::anonymizeIp("10.0.0.1");
    auto a2 = IdentityShadowMap::anonymizeIp("10.0.0.1");
    EXPECT_EQ(a1, a2);
}

TEST_F(SecurityCryptoExtTest, Shadow_AnonymizeIp_DifferentIps) {
    auto a1 = IdentityShadowMap::anonymizeIp("10.0.0.1");
    auto a2 = IdentityShadowMap::anonymizeIp("10.0.0.2");
    EXPECT_NE(a1, a2);
}

TEST_F(SecurityCryptoExtTest, Shadow_AnonymizeFingerprint_NotEmpty) {
    auto anon = IdentityShadowMap::anonymizeFingerprint("device_fp_abc123");
    EXPECT_FALSE(anon.empty());
}

TEST_F(SecurityCryptoExtTest, Shadow_AnonymizeFingerprint_Deterministic) {
    auto a1 = IdentityShadowMap::anonymizeFingerprint("fp_xyz");
    auto a2 = IdentityShadowMap::anonymizeFingerprint("fp_xyz");
    EXPECT_EQ(a1, a2);
}

TEST_F(SecurityCryptoExtTest, Shadow_DesensitizeEmail_Short) {
    auto r = IdentityShadowMap::desensitize("a@b.com", "email");
    EXPECT_NE(r.find('@'), std::string::npos);
}

TEST_F(SecurityCryptoExtTest, Shadow_DesensitizePhone_11Digits) {
    auto r = IdentityShadowMap::desensitize("18888888888", "phone");
    EXPECT_EQ(r.substr(0, 3), "188");
    EXPECT_EQ(r.substr(r.length() - 4), "8888");
}

TEST_F(SecurityCryptoExtTest, Shadow_DesensitizeName_Chinese) {
    auto r = IdentityShadowMap::desensitize("张三", "name");
    EXPECT_FALSE(r.empty());
}

TEST_F(SecurityCryptoExtTest, Shadow_DesensitizeUnknownType) {
    auto r = IdentityShadowMap::desensitize("something", "unknown");
    EXPECT_FALSE(r.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
