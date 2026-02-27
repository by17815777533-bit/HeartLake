/**
 * 全面集成测试 - 暖心语录、湖神AI、E2EE加密、熔断器
 */

#include <gtest/gtest.h>
#include "utils/E2EEncryption.h"
#include "utils/CircuitBreaker.h"

using namespace heartlake::utils;

// E2EE加密测试
class E2EEncryptionTest : public ::testing::Test {};

TEST_F(E2EEncryptionTest, GenerateKey_ReturnsNonEmpty) {
    auto key = E2EEncryption::generateKey();
    EXPECT_FALSE(key.empty());
    EXPECT_GT(key.length(), 20);  // Base64编码后至少20字符
}

TEST_F(E2EEncryptionTest, EncryptDecrypt_RoundTrip) {
    auto key = E2EEncryption::generateKey();
    std::string plaintext = "测试消息：心湖温暖你我";

    auto encrypted = E2EEncryption::encrypt(plaintext, key);
    ASSERT_TRUE(encrypted.has_value());
    EXPECT_FALSE(encrypted->ciphertext.empty());
    EXPECT_FALSE(encrypted->iv.empty());
    EXPECT_FALSE(encrypted->tag.empty());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, plaintext);
}

TEST_F(E2EEncryptionTest, Decrypt_WrongKey_Fails) {
    auto key1 = E2EEncryption::generateKey();
    auto key2 = E2EEncryption::generateKey();
    std::string plaintext = "秘密消息";

    auto encrypted = E2EEncryption::encrypt(plaintext, key1);
    ASSERT_TRUE(encrypted.has_value());

    auto decrypted = E2EEncryption::decrypt(*encrypted, key2);
    EXPECT_FALSE(decrypted.has_value());
}

TEST_F(E2EEncryptionTest, DeriveSessionKey_Consistent) {
    std::string secret = "shared_secret_123";
    std::string salt = "salt_456";

    auto key1 = E2EEncryption::deriveSessionKey(secret, salt);
    auto key2 = E2EEncryption::deriveSessionKey(secret, salt);

    EXPECT_EQ(key1, key2);
    EXPECT_FALSE(key1.empty());
}

// 熔断器测试
class CircuitBreakerTest : public ::testing::Test {};

TEST_F(CircuitBreakerTest, InitialState_Closed) {
    CircuitBreaker cb(3, 100);
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CircuitBreakerTest, SuccessfulCalls_StayClosed) {
    CircuitBreaker cb(3, 100);
    for (int i = 0; i < 10; i++) {
        EXPECT_NO_THROW(cb.execute([]() { return 1; }));
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CircuitBreakerTest, FailureThreshold_OpensCircuit) {
    CircuitBreaker cb(3, 100);
    for (int i = 0; i < 3; i++) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(CircuitBreakerTest, OpenCircuit_RejectsRequests) {
    CircuitBreaker cb(1, 1000);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}

    EXPECT_THROW(cb.execute([]() { return 1; }), std::runtime_error);
}

TEST_F(CircuitBreakerTest, Reset_ClosesCircuit) {
    CircuitBreaker cb(1, 1000);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}

    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    cb.reset();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
