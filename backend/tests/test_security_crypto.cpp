/**
 * 安全加密模块单元测试
 *
 * 覆盖：
 * 1. E2EEncryption: X25519密钥交换、AES-256-GCM加解密、HKDF会话密钥派生
 * 2. IdentityShadowMap: HMAC-SHA256匿名化、影子ID、数据脱敏
 */

#include <gtest/gtest.h>
#include "utils/E2EEncryption.h"
#include "utils/IdentityShadowMap.h"
#include <set>

using namespace heartlake::utils;

// ============================================================================
// E2EEncryption 测试
// ============================================================================

class E2EEncryptionTest : public ::testing::Test {};

// 1. X25519 密钥对生成
TEST_F(E2EEncryptionTest, GenerateX25519KeyPair) {
    auto kp = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp.has_value());
    EXPECT_FALSE(kp->publicKey.empty());
    EXPECT_FALSE(kp->privateKey.empty());
    // 公钥和私钥不应相同
    EXPECT_NE(kp->publicKey, kp->privateKey);
}

// 2. 两次生成的密钥对不同
TEST_F(E2EEncryptionTest, KeyPairsAreUnique) {
    auto kp1 = E2EEncryption::generateX25519KeyPair();
    auto kp2 = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp1.has_value());
    ASSERT_TRUE(kp2.has_value());
    EXPECT_NE(kp1->publicKey, kp2->publicKey);
    EXPECT_NE(kp1->privateKey, kp2->privateKey);
}

// 3. ECDH 共享密钥：双方计算结果一致
TEST_F(E2EEncryptionTest, SharedSecretAgreement) {
    auto alice = E2EEncryption::generateX25519KeyPair();
    auto bob = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(alice.has_value());
    ASSERT_TRUE(bob.has_value());

    auto secretA = E2EEncryption::computeSharedSecret(alice->privateKey, bob->publicKey);
    auto secretB = E2EEncryption::computeSharedSecret(bob->privateKey, alice->publicKey);
    ASSERT_TRUE(secretA.has_value());
    ASSERT_TRUE(secretB.has_value());
    EXPECT_EQ(*secretA, *secretB);
}

// 4. 无效密钥计算共享密钥应失败
TEST_F(E2EEncryptionTest, SharedSecretInvalidKey) {
    auto result = E2EEncryption::computeSharedSecret("invalid_base64", "also_invalid");
    EXPECT_FALSE(result.has_value());
}

// 5. AES-256-GCM 加密解密往返
TEST_F(E2EEncryptionTest, EncryptDecryptRoundTrip) {
    auto key = E2EEncryption::generateKey();
    std::string plaintext = "HeartLake端到端加密测试消息";

    auto encrypted = E2EEncryption::encrypt(plaintext, key);
    ASSERT_TRUE(encrypted.has_value());
    EXPECT_FALSE(encrypted->ciphertext.empty());
    EXPECT_FALSE(encrypted->iv.empty());
    EXPECT_FALSE(encrypted->tag.empty());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, plaintext);
}

// 6. 错误密钥解密应失败
TEST_F(E2EEncryptionTest, DecryptWithWrongKey) {
    auto key1 = E2EEncryption::generateKey();
    auto key2 = E2EEncryption::generateKey();

    auto encrypted = E2EEncryption::encrypt("secret message", key1);
    ASSERT_TRUE(encrypted.has_value());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key2);
    EXPECT_FALSE(decrypted.has_value());
}

// 7. 篡改密文解密应失败（认证加密完整性）
TEST_F(E2EEncryptionTest, TamperedCiphertextFails) {
    auto key = E2EEncryption::generateKey();
    auto encrypted = E2EEncryption::encrypt("integrity test", key);
    ASSERT_TRUE(encrypted.has_value());

    // 篡改密文
    EncryptedMessage tampered = *encrypted;
    if (!tampered.ciphertext.empty()) {
        tampered.ciphertext[0] = (tampered.ciphertext[0] == 'A') ? 'B' : 'A';
    }
    auto decrypted = E2EEncryption::decrypt(tampered, key);
    EXPECT_FALSE(decrypted.has_value());
}

// 8. HKDF 会话密钥派生：相同输入产生相同输出
TEST_F(E2EEncryptionTest, DeriveSessionKeyDeterministic) {
    auto alice = E2EEncryption::generateX25519KeyPair();
    auto bob = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(alice.has_value());
    ASSERT_TRUE(bob.has_value());

    auto shared = E2EEncryption::computeSharedSecret(alice->privateKey, bob->publicKey);
    ASSERT_TRUE(shared.has_value());

    auto salt = E2EEncryption::generateKey(); // 用随机key当salt
    auto sessionKey1 = E2EEncryption::deriveSessionKey(*shared, salt);
    auto sessionKey2 = E2EEncryption::deriveSessionKey(*shared, salt);
    EXPECT_EQ(sessionKey1, sessionKey2);
    EXPECT_FALSE(sessionKey1.empty());
}

// 9. 完整E2E流程：密钥交换 -> 派生 -> 加密 -> 解密
TEST_F(E2EEncryptionTest, FullE2EFlow) {
    auto alice = E2EEncryption::generateX25519KeyPair();
    auto bob = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(alice.has_value());
    ASSERT_TRUE(bob.has_value());

    auto shared = E2EEncryption::computeSharedSecret(alice->privateKey, bob->publicKey);
    ASSERT_TRUE(shared.has_value());

    auto salt = E2EEncryption::generateKey();
    auto sessionKey = E2EEncryption::deriveSessionKey(*shared, salt);

    std::string message = "Hello from Alice to Bob!";
    auto encrypted = E2EEncryption::encrypt(message, sessionKey);
    ASSERT_TRUE(encrypted.has_value());

    // Bob 用相同的共享密钥派生会话密钥并解密
    auto sharedBob = E2EEncryption::computeSharedSecret(bob->privateKey, alice->publicKey);
    auto sessionKeyBob = E2EEncryption::deriveSessionKey(*sharedBob, salt);
    EXPECT_EQ(sessionKey, sessionKeyBob);

    auto decrypted = E2EEncryption::decrypt(*encrypted, sessionKeyBob);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, message);
}

// ============================================================================
// IdentityShadowMap 测试
// ============================================================================

class IdentityShadowMapTest : public ::testing::Test {
protected:
    IdentityShadowMap& shadowMap = IdentityShadowMap::getInstance();
};

// 10. IP匿名化：格式正确且确定性
TEST_F(IdentityShadowMapTest, AnonymizeIpFormat) {
    auto anon = IdentityShadowMap::anonymizeIp("192.168.1.100");
    // 格式: "ip_" + 16个十六进制字符
    EXPECT_EQ(anon.substr(0, 3), "ip_");
    EXPECT_EQ(anon.length(), 3 + 16);

    // 确定性：同一IP始终映射到同一匿名ID
    auto anon2 = IdentityShadowMap::anonymizeIp("192.168.1.100");
    EXPECT_EQ(anon, anon2);
}

// 11. 不同IP产生不同匿名ID
TEST_F(IdentityShadowMapTest, DifferentIpsDifferentAnon) {
    auto anon1 = IdentityShadowMap::anonymizeIp("10.0.0.1");
    auto anon2 = IdentityShadowMap::anonymizeIp("10.0.0.2");
    EXPECT_NE(anon1, anon2);
}

// 12. 指纹匿名化
TEST_F(IdentityShadowMapTest, AnonymizeFingerprint) {
    auto fp1 = IdentityShadowMap::anonymizeFingerprint("device-abc-123");
    EXPECT_EQ(fp1.substr(0, 3), "fp_");
    EXPECT_EQ(fp1.length(), 3 + 16);

    // 确定性
    auto fp2 = IdentityShadowMap::anonymizeFingerprint("device-abc-123");
    EXPECT_EQ(fp1, fp2);

    // 不同指纹不同结果
    auto fp3 = IdentityShadowMap::anonymizeFingerprint("device-xyz-456");
    EXPECT_NE(fp1, fp3);
}

// 13. 影子ID生成与获取
TEST_F(IdentityShadowMapTest, GetOrCreateShadowId) {
    auto id1 = shadowMap.getOrCreateShadowId("user_test_001");
    EXPECT_FALSE(id1.empty());
    EXPECT_EQ(id1.substr(0, 4), "shd_");

    // 同一用户返回相同影子ID
    auto id2 = shadowMap.getOrCreateShadowId("user_test_001");
    EXPECT_EQ(id1, id2);

    // 不同用户返回不同影子ID
    auto id3 = shadowMap.getOrCreateShadowId("user_test_002");
    EXPECT_NE(id1, id3);
}

// 14. 影子ID轮换
TEST_F(IdentityShadowMapTest, RotateShadowId) {
    auto original = shadowMap.getOrCreateShadowId("user_rotate_test");
    auto rotated = shadowMap.rotateShadowId("user_rotate_test");
    EXPECT_NE(original, rotated);
    EXPECT_EQ(rotated.substr(0, 4), "shd_");

    // 轮换后获取应返回新ID
    auto current = shadowMap.getOrCreateShadowId("user_rotate_test");
    EXPECT_EQ(current, rotated);
}

// 15. 邮箱脱敏
TEST_F(IdentityShadowMapTest, DesensitizeEmail) {
    auto result = IdentityShadowMap::desensitize("alice@example.com", "email");
    // "alice" -> "a****@example.com"
    EXPECT_EQ(result[0], 'a');
    EXPECT_NE(result.find('@'), std::string::npos);
    EXPECT_NE(result.find("example.com"), std::string::npos);
    // 中间应有星号
    EXPECT_NE(result.find('*'), std::string::npos);
}

// 16. 手机号脱敏
TEST_F(IdentityShadowMapTest, DesensitizePhone) {
    auto result = IdentityShadowMap::desensitize("13812345678", "phone");
    // 前3位 + 星号 + 后4位
    EXPECT_EQ(result.substr(0, 3), "138");
    EXPECT_EQ(result.substr(result.length() - 4), "5678");
    EXPECT_NE(result.find('*'), std::string::npos);
}

// 17. 姓名脱敏
TEST_F(IdentityShadowMapTest, DesensitizeName) {
    auto result = IdentityShadowMap::desensitize("Alice", "name");
    EXPECT_EQ(result[0], 'A');
    EXPECT_EQ(result.length(), 5u);
    // 后面应全是星号
    EXPECT_EQ(result.substr(1), "****");
}

// 18. 空值脱敏返回空
TEST_F(IdentityShadowMapTest, DesensitizeEmpty) {
    EXPECT_EQ(IdentityShadowMap::desensitize("", "email"), "");
    EXPECT_EQ(IdentityShadowMap::desensitize("", "phone"), "");
    EXPECT_EQ(IdentityShadowMap::desensitize("", "name"), "");
}
