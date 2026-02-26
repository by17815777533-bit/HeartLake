/**
 * @file test_e2e_encryption_extended.cpp
 * @brief E2EEncryption 扩展单元测试 - X25519/ECDH/AES-256-GCM/HKDF
 */

#include <gtest/gtest.h>
#include "utils/E2EEncryption.h"
#include <string>
#include <set>

using namespace heartlake::utils;

// =====================================================================
// E2EEncryption 测试
// =====================================================================

class E2EEncryptionExtendedTest : public ::testing::Test {};

// -------------------- generateKey --------------------

TEST_F(E2EEncryptionExtendedTest, GenerateKey_NotEmpty) {
    auto key = E2EEncryption::generateKey();
    EXPECT_FALSE(key.empty());
}

TEST_F(E2EEncryptionExtendedTest, GenerateKey_Unique) {
    std::set<std::string> keys;
    for (int i = 0; i < 50; ++i) {
        keys.insert(E2EEncryption::generateKey());
    }
    EXPECT_EQ(keys.size(), 50u);
}

// -------------------- X25519 密钥对生成 --------------------

TEST_F(E2EEncryptionExtendedTest, GenerateX25519KeyPair_Success) {
    auto kp = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp.has_value());
    EXPECT_FALSE(kp->publicKey.empty());
    EXPECT_FALSE(kp->privateKey.empty());
}

TEST_F(E2EEncryptionExtendedTest, GenerateX25519KeyPair_DifferentEachTime) {
    auto kp1 = E2EEncryption::generateX25519KeyPair();
    auto kp2 = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp1.has_value());
    ASSERT_TRUE(kp2.has_value());
    EXPECT_NE(kp1->publicKey, kp2->publicKey);
    EXPECT_NE(kp1->privateKey, kp2->privateKey);
}

TEST_F(E2EEncryptionExtendedTest, GenerateX25519KeyPair_PublicPrivateDiffer) {
    auto kp = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp.has_value());
    EXPECT_NE(kp->publicKey, kp->privateKey);
}

// -------------------- ECDH 密钥交换 --------------------

TEST_F(E2EEncryptionExtendedTest, ComputeSharedSecret_BothSidesMatch) {
    auto alice = E2EEncryption::generateX25519KeyPair();
    auto bob = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(alice.has_value());
    ASSERT_TRUE(bob.has_value());

    // Alice 用自己的私钥 + Bob 的公钥
    auto secretAlice = E2EEncryption::computeSharedSecret(alice->privateKey, bob->publicKey);
    // Bob 用自己的私钥 + Alice 的公钥
    auto secretBob = E2EEncryption::computeSharedSecret(bob->privateKey, alice->publicKey);

    ASSERT_TRUE(secretAlice.has_value());
    ASSERT_TRUE(secretBob.has_value());
    EXPECT_EQ(*secretAlice, *secretBob);
}

TEST_F(E2EEncryptionExtendedTest, ComputeSharedSecret_DifferentPeers_DifferentSecrets) {
    auto alice = E2EEncryption::generateX25519KeyPair();
    auto bob = E2EEncryption::generateX25519KeyPair();
    auto charlie = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(alice.has_value());
    ASSERT_TRUE(bob.has_value());
    ASSERT_TRUE(charlie.has_value());

    auto secretAB = E2EEncryption::computeSharedSecret(alice->privateKey, bob->publicKey);
    auto secretAC = E2EEncryption::computeSharedSecret(alice->privateKey, charlie->publicKey);

    ASSERT_TRUE(secretAB.has_value());
    ASSERT_TRUE(secretAC.has_value());
    EXPECT_NE(*secretAB, *secretAC);
}

TEST_F(E2EEncryptionExtendedTest, ComputeSharedSecret_InvalidKey_ReturnsNullopt) {
    auto kp = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp.has_value());

    // 用无效的 base64 字符串作为对方公钥
    auto result = E2EEncryption::computeSharedSecret(kp->privateKey, "invalidbase64key");
    EXPECT_FALSE(result.has_value());
}

TEST_F(E2EEncryptionExtendedTest, ComputeSharedSecret_EmptyKey_ReturnsNullopt) {
    auto kp = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp.has_value());

    auto result = E2EEncryption::computeSharedSecret(kp->privateKey, "");
    EXPECT_FALSE(result.has_value());
}

// -------------------- AES-256-GCM 加密/解密 --------------------

TEST_F(E2EEncryptionExtendedTest, EncryptDecrypt_RoundTrip) {
    auto key = E2EEncryption::generateKey();
    std::string plaintext = "Hello, HeartLake!";

    auto encrypted = E2EEncryption::encrypt(plaintext, key);
    ASSERT_TRUE(encrypted.has_value());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, plaintext);
}

TEST_F(E2EEncryptionExtendedTest, EncryptDecrypt_ChineseText) {
    auto key = E2EEncryption::generateKey();
    std::string plaintext = "心湖是一个温暖的情感交流平台，让每个人都能找到共鸣。";

    auto encrypted = E2EEncryption::encrypt(plaintext, key);
    ASSERT_TRUE(encrypted.has_value());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, plaintext);
}

TEST_F(E2EEncryptionExtendedTest, EncryptDecrypt_EmptyPlaintext) {
    auto key = E2EEncryption::generateKey();
    std::string plaintext = "";

    auto encrypted = E2EEncryption::encrypt(plaintext, key);
    ASSERT_TRUE(encrypted.has_value());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, plaintext);
}

TEST_F(E2EEncryptionExtendedTest, EncryptDecrypt_LargeData) {
    auto key = E2EEncryption::generateKey();
    // 构造 >1MB 的数据
    std::string plaintext(1024 * 1024 + 123, 'A');
    for (size_t i = 0; i < plaintext.size(); ++i) {
        plaintext[i] = static_cast<char>('A' + (i % 26));
    }

    auto encrypted = E2EEncryption::encrypt(plaintext, key);
    ASSERT_TRUE(encrypted.has_value());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, plaintext);
}

TEST_F(E2EEncryptionExtendedTest, Encrypt_CiphertextDiffersFromPlaintext) {
    auto key = E2EEncryption::generateKey();
    std::string plaintext = "sensitive data here";

    auto encrypted = E2EEncryption::encrypt(plaintext, key);
    ASSERT_TRUE(encrypted.has_value());
    EXPECT_NE(encrypted->ciphertext, plaintext);
}

// -------------------- 密文篡改检测 --------------------

TEST_F(E2EEncryptionExtendedTest, Decrypt_TamperedCiphertext_Fails) {
    auto key = E2EEncryption::generateKey();
    auto encrypted = E2EEncryption::encrypt("secret message", key);
    ASSERT_TRUE(encrypted.has_value());

    // 篡改密文
    EncryptedMessage tampered = *encrypted;
    if (!tampered.ciphertext.empty()) {
        tampered.ciphertext[0] = (tampered.ciphertext[0] == 'A') ? 'B' : 'A';
    }
    auto result = E2EEncryption::decrypt(tampered, key);
    EXPECT_FALSE(result.has_value());
}

TEST_F(E2EEncryptionExtendedTest, Decrypt_TamperedTag_Fails) {
    auto key = E2EEncryption::generateKey();
    auto encrypted = E2EEncryption::encrypt("secret message", key);
    ASSERT_TRUE(encrypted.has_value());

    EncryptedMessage tampered = *encrypted;
    if (!tampered.tag.empty()) {
        tampered.tag[0] = (tampered.tag[0] == 'A') ? 'B' : 'A';
    }
    auto result = E2EEncryption::decrypt(tampered, key);
    EXPECT_FALSE(result.has_value());
}

TEST_F(E2EEncryptionExtendedTest, Decrypt_TamperedIV_Fails) {
    auto key = E2EEncryption::generateKey();
    auto encrypted = E2EEncryption::encrypt("secret message", key);
    ASSERT_TRUE(encrypted.has_value());

    EncryptedMessage tampered = *encrypted;
    if (!tampered.iv.empty()) {
        tampered.iv[0] = (tampered.iv[0] == 'A') ? 'B' : 'A';
    }
    auto result = E2EEncryption::decrypt(tampered, key);
    EXPECT_FALSE(result.has_value());
}

// -------------------- 不同密钥解密失败 --------------------

TEST_F(E2EEncryptionExtendedTest, Decrypt_WrongKey_Fails) {
    auto key1 = E2EEncryption::generateKey();
    auto key2 = E2EEncryption::generateKey();

    auto encrypted = E2EEncryption::encrypt("secret", key1);
    ASSERT_TRUE(encrypted.has_value());

    auto result = E2EEncryption::decrypt(*encrypted, key2);
    EXPECT_FALSE(result.has_value());
}

TEST_F(E2EEncryptionExtendedTest, Encrypt_InvalidKey_ReturnsNullopt) {
    // 密钥长度不对
    auto result = E2EEncryption::encrypt("test", "shortkey");
    EXPECT_FALSE(result.has_value());
}

// -------------------- IV 唯一性 --------------------

TEST_F(E2EEncryptionExtendedTest, Encrypt_IVUniqueness) {
    auto key = E2EEncryption::generateKey();
    std::set<std::string> ivs;

    for (int i = 0; i < 100; ++i) {
        auto encrypted = E2EEncryption::encrypt("same plaintext", key);
        ASSERT_TRUE(encrypted.has_value());
        ivs.insert(encrypted->iv);
    }
    // 每次加密的 IV 都应不同
    EXPECT_EQ(ivs.size(), 100u);
}

TEST_F(E2EEncryptionExtendedTest, Encrypt_SamePlaintext_DifferentCiphertext) {
    auto key = E2EEncryption::generateKey();
    auto enc1 = E2EEncryption::encrypt("same text", key);
    auto enc2 = E2EEncryption::encrypt("same text", key);
    ASSERT_TRUE(enc1.has_value());
    ASSERT_TRUE(enc2.has_value());
    // 由于 IV 不同，密文也应不同
    EXPECT_NE(enc1->ciphertext, enc2->ciphertext);
}

// -------------------- HKDF 密钥派生 --------------------

TEST_F(E2EEncryptionExtendedTest, DeriveSessionKey_NotEmpty) {
    auto key = E2EEncryption::deriveSessionKey("shared_secret_base64", "salt_value");
    EXPECT_FALSE(key.empty());
}

TEST_F(E2EEncryptionExtendedTest, DeriveSessionKey_Deterministic) {
    auto key1 = E2EEncryption::deriveSessionKey("same_secret", "same_salt");
    auto key2 = E2EEncryption::deriveSessionKey("same_secret", "same_salt");
    EXPECT_EQ(key1, key2);
}

TEST_F(E2EEncryptionExtendedTest, DeriveSessionKey_DifferentSalt_DifferentKey) {
    // deriveSessionKey 内部对参数做 base64Decode，需要传入有效 base64
    // 使用两个不同的共享密钥生成的 base64 值作为不同 salt
    auto kp1 = E2EEncryption::generateX25519KeyPair();
    auto kp2 = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp1.has_value());
    ASSERT_TRUE(kp2.has_value());
    // 用公钥作为 salt（它们是有效的 base64 编码）
    auto key1 = E2EEncryption::deriveSessionKey(kp1->publicKey, kp1->privateKey);
    auto key2 = E2EEncryption::deriveSessionKey(kp1->publicKey, kp2->privateKey);
    EXPECT_NE(key1, key2);
}

TEST_F(E2EEncryptionExtendedTest, DeriveSessionKey_DifferentSecret_DifferentKey) {
    auto kp1 = E2EEncryption::generateX25519KeyPair();
    auto kp2 = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(kp1.has_value());
    ASSERT_TRUE(kp2.has_value());
    auto key1 = E2EEncryption::deriveSessionKey(kp1->publicKey, kp1->privateKey);
    auto key2 = E2EEncryption::deriveSessionKey(kp2->publicKey, kp1->privateKey);
    EXPECT_NE(key1, key2);
}

// -------------------- 完整 E2E 流程 --------------------

TEST_F(E2EEncryptionExtendedTest, FullE2EFlow_KeyExchange_Encrypt_Decrypt) {
    // 模拟 Alice 和 Bob 的完整 E2E 加密流程
    auto alice = E2EEncryption::generateX25519KeyPair();
    auto bob = E2EEncryption::generateX25519KeyPair();
    ASSERT_TRUE(alice.has_value());
    ASSERT_TRUE(bob.has_value());

    // ECDH 密钥交换
    auto sharedSecret = E2EEncryption::computeSharedSecret(alice->privateKey, bob->publicKey);
    ASSERT_TRUE(sharedSecret.has_value());

    // HKDF 派生会话密钥
    auto sessionKey = E2EEncryption::deriveSessionKey(*sharedSecret, "heartlake-session-v1");
    EXPECT_FALSE(sessionKey.empty());

    // Alice 用会话密钥加密消息
    std::string message = "你好，Bob！这是一条加密消息。";
    auto encrypted = E2EEncryption::encrypt(message, sessionKey);
    ASSERT_TRUE(encrypted.has_value());

    // Bob 用相同的会话密钥解密
    auto bobShared = E2EEncryption::computeSharedSecret(bob->privateKey, alice->publicKey);
    ASSERT_TRUE(bobShared.has_value());
    auto bobSessionKey = E2EEncryption::deriveSessionKey(*bobShared, "heartlake-session-v1");

    auto decrypted = E2EEncryption::decrypt(*encrypted, bobSessionKey);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, message);
}

// =====================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
