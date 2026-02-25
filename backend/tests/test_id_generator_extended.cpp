/**
 * @file test_id_generator_extended.cpp
 * @brief IdGenerator 扩展单元测试 - 格式、长度、唯一性、并发
 */

#include <gtest/gtest.h>
#include "utils/IdGenerator.h"
#include "utils/RecoveryKeyGenerator.h"
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>

using namespace heartlake::utils;

class IdGeneratorExtendedTest : public ::testing::Test {};

// =====================================================================
// generateUserId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, UserId_HasPrefix) {
    auto id = IdGenerator::generateUserId();
    EXPECT_EQ(id.substr(0, 5), "user_");
}

TEST_F(IdGeneratorExtendedTest, UserId_CorrectLength) {
    auto id = IdGenerator::generateUserId();
    EXPECT_EQ(id.length(), 5 + 16);  // "user_" + 16 hex chars
}

TEST_F(IdGeneratorExtendedTest, UserId_HexSuffix) {
    auto id = IdGenerator::generateUserId();
    for (size_t i = 5; i < id.length(); ++i) {
        EXPECT_TRUE(std::isxdigit(id[i])) << "Non-hex at pos " << i;
    }
}

TEST_F(IdGeneratorExtendedTest, UserId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) {
        ids.insert(IdGenerator::generateUserId());
    }
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateAnonymousId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, AnonymousId_HasPrefix) {
    auto id = IdGenerator::generateAnonymousId();
    EXPECT_EQ(id.substr(0, 10), "anonymous_");
}

TEST_F(IdGeneratorExtendedTest, AnonymousId_CorrectLength) {
    auto id = IdGenerator::generateAnonymousId();
    EXPECT_EQ(id.length(), 10 + 12);
}

TEST_F(IdGeneratorExtendedTest, AnonymousId_HexSuffix) {
    auto id = IdGenerator::generateAnonymousId();
    for (size_t i = 10; i < id.length(); ++i) {
        EXPECT_TRUE(std::isxdigit(id[i]));
    }
}

TEST_F(IdGeneratorExtendedTest, AnonymousId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) {
        ids.insert(IdGenerator::generateAnonymousId());
    }
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateStoneId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, StoneId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateStoneId().substr(0, 6), "stone_");
}

TEST_F(IdGeneratorExtendedTest, StoneId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateStoneId().length(), 6 + 16);
}

TEST_F(IdGeneratorExtendedTest, StoneId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateStoneId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateRippleId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, RippleId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateRippleId().substr(0, 7), "ripple_");
}

TEST_F(IdGeneratorExtendedTest, RippleId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateRippleId().length(), 7 + 16);
}

TEST_F(IdGeneratorExtendedTest, RippleId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateRippleId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateBoatId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, BoatId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateBoatId().substr(0, 5), "boat_");
}

TEST_F(IdGeneratorExtendedTest, BoatId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateBoatId().length(), 5 + 16);
}

TEST_F(IdGeneratorExtendedTest, BoatId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateBoatId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateNotificationId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, NotificationId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateNotificationId().substr(0, 6), "notif_");
}

TEST_F(IdGeneratorExtendedTest, NotificationId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateNotificationId().length(), 6 + 16);
}

TEST_F(IdGeneratorExtendedTest, NotificationId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateNotificationId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateConnectionId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, ConnectionId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateConnectionId().substr(0, 5), "conn_");
}

TEST_F(IdGeneratorExtendedTest, ConnectionId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateConnectionId().length(), 5 + 16);
}

TEST_F(IdGeneratorExtendedTest, ConnectionId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateConnectionId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateMessageId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, MessageId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateMessageId().substr(0, 4), "msg_");
}

TEST_F(IdGeneratorExtendedTest, MessageId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateMessageId().length(), 4 + 16);
}

TEST_F(IdGeneratorExtendedTest, MessageId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateMessageId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateReportId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, ReportId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateReportId().substr(0, 7), "report_");
}

TEST_F(IdGeneratorExtendedTest, ReportId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateReportId().length(), 7 + 16);
}

TEST_F(IdGeneratorExtendedTest, ReportId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateReportId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateSessionId
// =====================================================================

TEST_F(IdGeneratorExtendedTest, SessionId_HasPrefix) {
    EXPECT_EQ(IdGenerator::generateSessionId().substr(0, 8), "session_");
}

TEST_F(IdGeneratorExtendedTest, SessionId_CorrectLength) {
    EXPECT_EQ(IdGenerator::generateSessionId().length(), 8 + 16);
}

TEST_F(IdGeneratorExtendedTest, SessionId_HexSuffix) {
    auto id = IdGenerator::generateSessionId();
    for (size_t i = 8; i < id.length(); ++i) {
        EXPECT_TRUE(std::isxdigit(id[i]));
    }
}

TEST_F(IdGeneratorExtendedTest, SessionId_Unique1000) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) ids.insert(IdGenerator::generateSessionId());
    EXPECT_EQ(ids.size(), 1000u);
}

// =====================================================================
// generateUUID
// =====================================================================

TEST_F(IdGeneratorExtendedTest, UUID_CorrectLength) {
    auto uuid = IdGenerator::generateUUID();
    EXPECT_EQ(uuid.length(), 32u);
}

TEST_F(IdGeneratorExtendedTest, UUID_AllHex) {
    auto uuid = IdGenerator::generateUUID();
    for (char c : uuid) {
        EXPECT_TRUE(std::isxdigit(c)) << "Non-hex char: " << c;
    }
}

TEST_F(IdGeneratorExtendedTest, UUID_Unique1000) {
    std::set<std::string> uuids;
    for (int i = 0; i < 1000; ++i) uuids.insert(IdGenerator::generateUUID());
    EXPECT_EQ(uuids.size(), 1000u);
}

// =====================================================================
// generateNickname
// =====================================================================

TEST_F(IdGeneratorExtendedTest, Nickname_HasPrefix) {
    auto nick = IdGenerator::generateNickname();
    EXPECT_NE(nick.find("#"), std::string::npos);
}

TEST_F(IdGeneratorExtendedTest, Nickname_NumberRange) {
    auto nick = IdGenerator::generateNickname();
    size_t hashPos = nick.find('#');
    ASSERT_NE(hashPos, std::string::npos);
    int num = std::stoi(nick.substr(hashPos + 1));
    EXPECT_GE(num, 1000);
    EXPECT_LE(num, 9999);
}

TEST_F(IdGeneratorExtendedTest, Nickname_MultipleGeneration) {
    std::set<std::string> nicks;
    for (int i = 0; i < 100; ++i) {
        nicks.insert(IdGenerator::generateNickname());
    }
    // 有9000种可能，100次应该有很多不同的
    EXPECT_GT(nicks.size(), 50u);
}

// =====================================================================
// 并发生成测试
// =====================================================================

TEST_F(IdGeneratorExtendedTest, ConcurrentUserId_NoCollision) {
    const int numThreads = 8;
    const int idsPerThread = 125;
    std::set<std::string> allIds;
    std::mutex mtx;
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&]() {
            std::vector<std::string> localIds;
            for (int i = 0; i < idsPerThread; ++i) {
                localIds.push_back(IdGenerator::generateUserId());
            }
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& id : localIds) allIds.insert(std::move(id));
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(allIds.size(), static_cast<size_t>(numThreads * idsPerThread));
}

TEST_F(IdGeneratorExtendedTest, ConcurrentUUID_NoCollision) {
    const int numThreads = 8;
    const int idsPerThread = 125;
    std::set<std::string> allIds;
    std::mutex mtx;
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&]() {
            std::vector<std::string> localIds;
            for (int i = 0; i < idsPerThread; ++i) {
                localIds.push_back(IdGenerator::generateUUID());
            }
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& id : localIds) allIds.insert(std::move(id));
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(allIds.size(), static_cast<size_t>(numThreads * idsPerThread));
}

TEST_F(IdGeneratorExtendedTest, ConcurrentSessionId_NoCollision) {
    const int numThreads = 4;
    const int idsPerThread = 250;
    std::set<std::string> allIds;
    std::mutex mtx;
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&]() {
            std::vector<std::string> localIds;
            for (int i = 0; i < idsPerThread; ++i) {
                localIds.push_back(IdGenerator::generateSessionId());
            }
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& id : localIds) allIds.insert(std::move(id));
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(allIds.size(), static_cast<size_t>(numThreads * idsPerThread));
}

// =====================================================================
// RecoveryKeyGenerator
// =====================================================================

TEST_F(IdGeneratorExtendedTest, RecoveryKey_NotEmpty) {
    auto key = RecoveryKeyGenerator::generate();
    EXPECT_FALSE(key.empty());
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_Has7Dashes) {
    auto key = RecoveryKeyGenerator::generate();
    int dashes = std::count(key.begin(), key.end(), '-');
    EXPECT_EQ(dashes, 7);  // 8 words separated by 7 dashes
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_Unique100) {
    std::set<std::string> keys;
    for (int i = 0; i < 100; ++i) {
        keys.insert(RecoveryKeyGenerator::generate());
    }
    EXPECT_EQ(keys.size(), 100u);
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_HashNotEmpty) {
    auto key = RecoveryKeyGenerator::generate();
    auto hashed = RecoveryKeyGenerator::hash(key);
    EXPECT_FALSE(hashed.empty());
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_HashContainsColon) {
    auto key = RecoveryKeyGenerator::generate();
    auto hashed = RecoveryKeyGenerator::hash(key);
    EXPECT_NE(hashed.find(':'), std::string::npos);
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_VerifyCorrectKey) {
    auto key = RecoveryKeyGenerator::generate();
    auto hashed = RecoveryKeyGenerator::hash(key);
    EXPECT_TRUE(RecoveryKeyGenerator::verify(key, hashed));
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_VerifyWrongKey) {
    auto key1 = RecoveryKeyGenerator::generate();
    auto key2 = RecoveryKeyGenerator::generate();
    auto hashed = RecoveryKeyGenerator::hash(key1);
    EXPECT_FALSE(RecoveryKeyGenerator::verify(key2, hashed));
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_VerifyInvalidFormat) {
    EXPECT_FALSE(RecoveryKeyGenerator::verify("test", "nocolon"));
}

TEST_F(IdGeneratorExtendedTest, RecoveryKey_DifferentHashesForSameKey) {
    auto key = RecoveryKeyGenerator::generate();
    auto hash1 = RecoveryKeyGenerator::hash(key);
    auto hash2 = RecoveryKeyGenerator::hash(key);
    // Different salts should produce different hashes
    EXPECT_NE(hash1, hash2);
    // But both should verify
    EXPECT_TRUE(RecoveryKeyGenerator::verify(key, hash1));
    EXPECT_TRUE(RecoveryKeyGenerator::verify(key, hash2));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
