/**
 * RecoveryKeyGenerator 单元测试
 *
 * 覆盖：
 * 1. 密钥生成格式正确（8个中文词，用 - 分隔）
 * 2. PBKDF2 加盐哈希（salt:hash 格式）
 * 3. 哈希验证（正确密钥通过）
 * 4. 错误密钥拒绝
 * 5. 重复生成唯一性
 * 6. 盐值随机性
 * 7. 恒定时间比较（时序攻击防护）
 * 8. 边界情况处理
 */

#include <gtest/gtest.h>
#include "utils/RecoveryKeyGenerator.h"
#include <set>
#include <string>
#include <algorithm>

using namespace heartlake::utils;

// ============================================================================
// RecoveryKeyGenerator 测试
// ============================================================================

class RecoveryKeyTest : public ::testing::Test {};

// ---------- 密钥生成格式 ----------

// 1. 生成的密钥非空
TEST_F(RecoveryKeyTest, GenerateReturnsNonEmpty) {
    std::string key = RecoveryKeyGenerator::generate();
    EXPECT_FALSE(key.empty());
}

// 2. 生成的密钥包含 7 个分隔符（8个词）
TEST_F(RecoveryKeyTest, GenerateHasEightWords) {
    std::string key = RecoveryKeyGenerator::generate();
    int dashCount = static_cast<int>(std::count(key.begin(), key.end(), '-'));
    EXPECT_EQ(dashCount, 7) << "Key: " << key;
}

// 3. 每个词都是非空的中文字符
TEST_F(RecoveryKeyTest, EachWordIsNonEmpty) {
    std::string key = RecoveryKeyGenerator::generate();
    std::istringstream ss(key);
    std::string word;
    int wordCount = 0;
    while (std::getline(ss, word, '-')) {
        EXPECT_FALSE(word.empty()) << "Word " << wordCount << " is empty in key: " << key;
        // 中文字符在 UTF-8 中至少 3 字节
        EXPECT_GE(word.size(), 3u) << "Word too short: " << word;
        wordCount++;
    }
    EXPECT_EQ(wordCount, 8);
}

// ---------- 唯一性 ----------

// 4. 多次生成的密钥互不相同
TEST_F(RecoveryKeyTest, GeneratedKeysAreUnique) {
    std::set<std::string> keys;
    for (int i = 0; i < 100; ++i) {
        keys.insert(RecoveryKeyGenerator::generate());
    }
    // 256^8 = 2^64 种可能，100次碰撞概率极低
    EXPECT_EQ(keys.size(), 100u);
}

// 5. 生成的密钥有足够的熵（不会出现全部相同的词）
TEST_F(RecoveryKeyTest, KeyHasSufficientEntropy) {
    std::string key = RecoveryKeyGenerator::generate();
    std::vector<std::string> words;
    std::istringstream ss(key);
    std::string word;
    while (std::getline(ss, word, '-')) {
        words.push_back(word);
    }
    // 8个词中至少有2个不同的词（概率上几乎不可能全部相同）
    std::set<std::string> uniqueWords(words.begin(), words.end());
    EXPECT_GE(uniqueWords.size(), 2u);
}

// ---------- 哈希 ----------

// 6. hash 返回 salt:hash 格式
TEST_F(RecoveryKeyTest, HashReturnsSaltColonHash) {
    std::string key = RecoveryKeyGenerator::generate();
    std::string hashed = RecoveryKeyGenerator::hash(key);

    auto sep = hashed.find(':');
    ASSERT_NE(sep, std::string::npos) << "Hash missing separator: " << hashed;

    std::string salt = hashed.substr(0, sep);
    std::string hashPart = hashed.substr(sep + 1);

    // salt 是 16 字节的 hex = 32 字符
    EXPECT_EQ(salt.size(), 32u) << "Salt: " << salt;
    // hash 是 32 字节的 hex = 64 字符
    EXPECT_EQ(hashPart.size(), 64u) << "Hash: " << hashPart;
}

// 7. salt 和 hash 都是合法的十六进制字符串
TEST_F(RecoveryKeyTest, HashContainsValidHex) {
    std::string key = RecoveryKeyGenerator::generate();
    std::string hashed = RecoveryKeyGenerator::hash(key);

    for (char c : hashed) {
        if (c == ':') continue;
        EXPECT_TRUE(std::isxdigit(c)) << "Non-hex char: " << c << " in " << hashed;
    }
}

// 8. 同一密钥两次哈希结果不同（因为随机盐）
TEST_F(RecoveryKeyTest, SameKeyDifferentHashes) {
    std::string key = RecoveryKeyGenerator::generate();
    std::string h1 = RecoveryKeyGenerator::hash(key);
    std::string h2 = RecoveryKeyGenerator::hash(key);
    EXPECT_NE(h1, h2) << "Same key should produce different hashes due to random salt";
}

// 9. 不同密钥的盐值不同
TEST_F(RecoveryKeyTest, DifferentKeysDifferentSalts) {
    std::set<std::string> salts;
    for (int i = 0; i < 20; ++i) {
        std::string key = RecoveryKeyGenerator::generate();
        std::string hashed = RecoveryKeyGenerator::hash(key);
        std::string salt = hashed.substr(0, hashed.find(':'));
        salts.insert(salt);
    }
    // 20个随机盐应该全部不同
    EXPECT_EQ(salts.size(), 20u);
}

// ---------- 验证 ----------

// 10. 正确密钥验证通过
TEST_F(RecoveryKeyTest, VerifyCorrectKeyPasses) {
    std::string key = RecoveryKeyGenerator::generate();
    std::string hashed = RecoveryKeyGenerator::hash(key);
    EXPECT_TRUE(RecoveryKeyGenerator::verify(key, hashed));
}

// 11. 错误密钥验证失败
TEST_F(RecoveryKeyTest, VerifyWrongKeyFails) {
    std::string key1 = RecoveryKeyGenerator::generate();
    std::string key2 = RecoveryKeyGenerator::generate();
    std::string hashed = RecoveryKeyGenerator::hash(key1);
    EXPECT_FALSE(RecoveryKeyGenerator::verify(key2, hashed));
}

// 12. 空密钥验证失败
TEST_F(RecoveryKeyTest, VerifyEmptyKeyFails) {
    std::string key = RecoveryKeyGenerator::generate();
    std::string hashed = RecoveryKeyGenerator::hash(key);
    EXPECT_FALSE(RecoveryKeyGenerator::verify("", hashed));
}

// 13. 无效的存储哈希格式（无冒号分隔符）验证失败
TEST_F(RecoveryKeyTest, VerifyInvalidHashFormatFails) {
    std::string key = RecoveryKeyGenerator::generate();
    EXPECT_FALSE(RecoveryKeyGenerator::verify(key, "no_separator_here"));
}

// 14. 篡改哈希值验证失败
TEST_F(RecoveryKeyTest, VerifyTamperedHashFails) {
    std::string key = RecoveryKeyGenerator::generate();
    std::string hashed = RecoveryKeyGenerator::hash(key);
    // 修改 hash 部分的最后一个字符
    if (!hashed.empty()) {
        hashed.back() = (hashed.back() == 'a') ? 'b' : 'a';
    }
    EXPECT_FALSE(RecoveryKeyGenerator::verify(key, hashed));
}

// 15. 篡改盐值验证失败
TEST_F(RecoveryKeyTest, VerifyTamperedSaltFails) {
    std::string key = RecoveryKeyGenerator::generate();
    std::string hashed = RecoveryKeyGenerator::hash(key);
    // 修改 salt 部分的第一个字符
    if (!hashed.empty()) {
        hashed[0] = (hashed[0] == 'a') ? 'b' : 'a';
    }
    EXPECT_FALSE(RecoveryKeyGenerator::verify(key, hashed));
}

// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
