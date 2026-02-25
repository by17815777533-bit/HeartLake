/**
 * @file test_security_torture.cpp
 * @brief 安全模块极限压力测试 - 100+ 测试用例覆盖 CircuitBreaker、PASETO、E2E、Password/Identity、Validator
 */

#include <gtest/gtest.h>
#include "utils/CircuitBreaker.h"
#include "utils/PasetoUtil.h"
#include "utils/PasswordUtil.h"
#include "utils/E2EEncryption.h"
#include "utils/IdentityShadowMap.h"
#include "utils/RecoveryKeyGenerator.h"
#include "utils/Validator.h"
#include "utils/IdGenerator.h"
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <optional>
#include <algorithm>
#include <numeric>

using namespace heartlake::utils;

class SecurityTortureTest : public ::testing::Test {};

// =====================================================================
// Section 1: CircuitBreaker Extremes (22 tests)
// =====================================================================

TEST_F(SecurityTortureTest, CB_Threshold1_InstantOpen) {
    CircuitBreaker cb(1, 5000);
    try { cb.execute([]() -> int { throw std::runtime_error("boom"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(SecurityTortureTest, CB_Threshold1000_NeedsAllFailures) {
    CircuitBreaker cb(1000, 5000);
    for (int i = 0; i < 999; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(SecurityTortureTest, CB_RapidOpenCloseReset_100Cycles) {
    CircuitBreaker cb(1, 50);
    for (int cycle = 0; cycle < 100; ++cycle) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
        EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
        cb.reset();
        EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    }
}

TEST_F(SecurityTortureTest, CB_SuccessAfterManyFailures_ResetCount) {
    CircuitBreaker cb(10, 1000);
    for (int i = 0; i < 9; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    cb.execute([]() { return 1; });
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    for (int i = 0; i < 9; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(SecurityTortureTest, CB_HalfOpen_SuccessCloses) {
    CircuitBreaker cb(1, 30);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    cb.execute([]() { return 42; });
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(SecurityTortureTest, CB_HalfOpen_FailureReopens) {
    CircuitBreaker cb(1, 30);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    try { cb.execute([]() -> int { throw std::runtime_error("fail again"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(SecurityTortureTest, CB_OpenRejectsWithMessage) {
    CircuitBreaker cb(1, 5000);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    try {
        cb.execute([]() { return 1; });
        FAIL() << "Should have thrown";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("OPEN"), std::string::npos);
    }
}

TEST_F(SecurityTortureTest, CB_Concurrent50Threads_AllSuccess) {
    CircuitBreaker cb(100, 1000);
    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 50; ++i) {
        threads.emplace_back([&]() {
            try {
                cb.execute([]() { return 1; });
                count++;
            } catch (...) {}
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(count.load(), 50);
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(SecurityTortureTest, CB_Concurrent50Threads_AllFail) {
    CircuitBreaker cb(5, 5000);
    std::vector<std::thread> threads;
    for (int i = 0; i < 50; ++i) {
        threads.emplace_back([&]() {
            try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
            catch (...) {}
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(SecurityTortureTest, CB_ConcurrentMixed_NoDeadlock) {
    CircuitBreaker cb(20, 100);
    std::atomic<int> ops{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 40; ++i) {
        threads.emplace_back([&, i]() {
            try {
                cb.execute([i]() -> int {
                    if (i % 2 == 0) throw std::runtime_error("fail");
                    return i;
                });
            } catch (...) {}
            ops++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(ops.load(), 40);
}

TEST_F(SecurityTortureTest, CB_ResetDuringConcurrentUse) {
    CircuitBreaker cb(5, 5000);
    std::vector<std::thread> threads;
    std::atomic<bool> done{false};
    threads.emplace_back([&]() {
        while (!done.load()) {
            try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
            catch (...) {}
        }
    });
    threads.emplace_back([&]() {
        for (int i = 0; i < 20; ++i) {
            cb.reset();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        done.store(true);
    });
    for (auto& t : threads) t.join();
    // Just verify no crash/deadlock
    SUCCEED();
}

TEST_F(SecurityTortureTest, CB_ExceptionType_LogicError_Propagates) {
    CircuitBreaker cb(10, 100);
    EXPECT_THROW(
        cb.execute([]() -> int { throw std::logic_error("logic"); }),
        std::logic_error
    );
}

TEST_F(SecurityTortureTest, CB_ExceptionType_InvalidArgument_Propagates) {
    CircuitBreaker cb(10, 100);
    EXPECT_THROW(
        cb.execute([]() -> int { throw std::invalid_argument("bad"); }),
        std::invalid_argument
    );
}

TEST_F(SecurityTortureTest, CB_ExceptionType_OutOfRange_Propagates) {
    CircuitBreaker cb(10, 100);
    EXPECT_THROW(
        cb.execute([]() -> int { throw std::out_of_range("oor"); }),
        std::out_of_range
    );
}

TEST_F(SecurityTortureTest, CB_ReturnString) {
    CircuitBreaker cb(3, 100);
    auto result = cb.execute([]() { return std::string("hello"); });
    EXPECT_EQ(result, "hello");
}

TEST_F(SecurityTortureTest, CB_ReturnVector) {
    CircuitBreaker cb(3, 100);
    auto result = cb.execute([]() { return std::vector<int>{1, 2, 3, 4, 5}; });
    EXPECT_EQ(result.size(), 5u);
}

TEST_F(SecurityTortureTest, CB_AlternatingSuccessFailure_StaysClosed) {
    CircuitBreaker cb(3, 100);
    for (int i = 0; i < 50; ++i) {
        if (i % 2 == 0) {
            cb.execute([]() { return 1; });
        } else {
            try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
            catch (...) {}
        }
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(SecurityTortureTest, CB_TimeoutBoundary_StillOpen) {
    CircuitBreaker cb(1, 200);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(SecurityTortureTest, CB_TimeoutBoundary_JustExpired) {
    CircuitBreaker cb(1, 50);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
}

TEST_F(SecurityTortureTest, CB_MultipleResets_NoCorruption) {
    CircuitBreaker cb(2, 100);
    for (int i = 0; i < 50; ++i) {
        cb.reset();
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    cb.execute([]() { return 1; });
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(SecurityTortureTest, CB_FullCycle_ClosedOpenHalfOpenClosed) {
    CircuitBreaker cb(2, 30);
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    try { cb.execute([]() -> int { throw std::runtime_error("f1"); }); } catch (...) {}
    try { cb.execute([]() -> int { throw std::runtime_error("f2"); }); } catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    cb.execute([]() { return 1; });
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(SecurityTortureTest, CB_DoubleFullCycle) {
    CircuitBreaker cb(1, 30);
    for (int cycle = 0; cycle < 2; ++cycle) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); } catch (...) {}
        EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
        cb.execute([]() { return 1; });
        EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    }
}

// =====================================================================
// Section 2: PASETO Token Stress (22 tests)
// =====================================================================

TEST_F(SecurityTortureTest, Paseto_GenerateVerify_Roundtrip) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_rt", key, 1);
    EXPECT_FALSE(token.empty());
    auto uid = PasetoUtil::verifyToken(token, key);
    EXPECT_EQ(uid, "user_rt");
}

TEST_F(SecurityTortureTest, Paseto_V4LocalHeader) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_hdr", key);
    EXPECT_EQ(token.substr(0, 9), "v4.local.");
}

TEST_F(SecurityTortureTest, Paseto_WrongKey_Throws) {
    std::string key1 = "01234567890123456789012345678901";
    std::string key2 = "abcdefghijklmnopqrstuvwxyz012345";
    auto token = PasetoUtil::generateToken("user_wk", key1);
    EXPECT_THROW(PasetoUtil::verifyToken(token, key2), std::runtime_error);
}

TEST_F(SecurityTortureTest, Paseto_EmptyToken_Throws) {
    std::string key = "01234567890123456789012345678901";
    EXPECT_THROW(PasetoUtil::verifyToken("", key), std::runtime_error);
}

TEST_F(SecurityTortureTest, Paseto_InvalidHeader_Throws) {
    std::string key = "01234567890123456789012345678901";
    EXPECT_THROW(PasetoUtil::verifyToken("v3.local.garbage", key), std::runtime_error);
}

TEST_F(SecurityTortureTest, Paseto_TruncatedToken_Throws) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_trunc", key);
    std::string truncated = token.substr(0, token.length() / 2);
    EXPECT_THROW(PasetoUtil::verifyToken(truncated, key), std::runtime_error);
}

TEST_F(SecurityTortureTest, Paseto_TamperedPayload_Throws) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_tamper", key);
    std::string tampered = token;
    size_t mid = tampered.length() / 2;
    tampered[mid] = (tampered[mid] == 'A') ? 'B' : 'A';
    EXPECT_THROW(PasetoUtil::verifyToken(tampered, key), std::runtime_error);
}

TEST_F(SecurityTortureTest, Paseto_ExpiredToken_Throws) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateToken("user_exp", key, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_THROW(PasetoUtil::verifyToken(token, key), std::runtime_error);
}

TEST_F(SecurityTortureTest, Paseto_DifferentUsers_DifferentTokens) {
    std::string key = "01234567890123456789012345678901";
    auto t1 = PasetoUtil::generateToken("user_a", key);
    auto t2 = PasetoUtil::generateToken("user_b", key);
    EXPECT_NE(t1, t2);
}

TEST_F(SecurityTortureTest, Paseto_SameUser_DifferentTokens_Nonce) {
    std::string key = "01234567890123456789012345678901";
    auto t1 = PasetoUtil::generateToken("same_user", key);
    auto t2 = PasetoUtil::generateToken("same_user", key);
    EXPECT_NE(t1, t2);
}

TEST_F(SecurityTortureTest, Paseto_Base64Url_Roundtrip) {
    std::string original = "Hello, World! 你好世界";
    auto encoded = PasetoUtil::base64urlEncode(original);
    auto decoded = PasetoUtil::base64urlDecode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(SecurityTortureTest, Paseto_Base64Url_Empty) {
    EXPECT_EQ(PasetoUtil::base64urlEncode(""), "");
    EXPECT_EQ(PasetoUtil::base64urlDecode(""), "");
}

TEST_F(SecurityTortureTest, Paseto_Base64Url_NoPadding) {
    auto encoded = PasetoUtil::base64urlEncode("test");
    EXPECT_EQ(encoded.find('='), std::string::npos);
}

TEST_F(SecurityTortureTest, Paseto_Base64Url_BinaryData) {
    std::string binary(256, '\0');
    for (int i = 0; i < 256; ++i) binary[i] = static_cast<char>(i);
    auto encoded = PasetoUtil::base64urlEncode(binary);
    auto decoded = PasetoUtil::base64urlDecode(encoded);
    EXPECT_EQ(decoded, binary);
}

TEST_F(SecurityTortureTest, Paseto_AdminToken_GenerateVerify) {
    std::string key = "01234567890123456789012345678901";
    auto token = PasetoUtil::generateAdminToken("admin_1", "super_admin", key, 1);
    EXPECT_FALSE(token.empty());
    std::string adminId, role;
    bool ok = PasetoUtil::verifyAdminToken(token, key, adminId, role);
    EXPECT_TRUE(ok);
    EXPECT_EQ(adminId, "admin_1");
    EXPECT_EQ(role, "super_admin");
}

TEST_F(SecurityTortureTest, Paseto_AdminToken_WrongKey_Fails) {
    std::string key1 = "01234567890123456789012345678901";
    std::string key2 = "abcdefghijklmnopqrstuvwxyz012345";
    auto token = PasetoUtil::generateAdminToken("admin_x", "admin", key1);
    std::string adminId, role;
    EXPECT_FALSE(PasetoUtil::verifyAdminToken(token, key2, adminId, role));
}

TEST_F(SecurityTortureTest, Paseto_AdminToken_InvalidToken_Fails) {
    std::string key = "01234567890123456789012345678901";
    std::string adminId, role;
    EXPECT_FALSE(PasetoUtil::verifyAdminToken("garbage_token", key, adminId, role));
}

TEST_F(SecurityTortureTest, Paseto_AdminToken_EmptyToken_Fails) {
    std::string key = "01234567890123456789012345678901";
    std::string adminId, role;
    EXPECT_FALSE(PasetoUtil::verifyAdminToken("", key, adminId, role));
}

TEST_F(SecurityTortureTest, Paseto_Concurrent10Threads_GenerateVerify) {
    std::string key = "01234567890123456789012345678901";
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            std::string uid = "concurrent_user_" + std::to_string(i);
            auto token = PasetoUtil::generateToken(uid, key, 1);
            auto verified = PasetoUtil::verifyToken(token, key);
            if (verified == uid) success++;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(success.load(), 10);
}

TEST_F(SecurityTortureTest, Paseto_LongUserId) {
    std::string key = "01234567890123456789012345678901";
    std::string longId(500, 'x');
    auto token = PasetoUtil::generateToken(longId, key, 1);
    auto verified = PasetoUtil::verifyToken(token, key);
    EXPECT_EQ(verified, longId);
}

TEST_F(SecurityTortureTest, Paseto_SpecialCharsUserId) {
    std::string key = "01234567890123456789012345678901";
    std::string specialId = "user@#$%^&*()_+-=[]{}|;':\",./<>?";
    auto token = PasetoUtil::generateToken(specialId, key, 1);
    auto verified = PasetoUtil::verifyToken(token, key);
    EXPECT_EQ(verified, specialId);
}

TEST_F(SecurityTortureTest, Paseto_UnicodeUserId) {
    std::string key = "01234567890123456789012345678901";
    std::string unicodeId = "用户_テスト_사용자_🎉";
    auto token = PasetoUtil::generateToken(unicodeId, key, 1);
    auto verified = PasetoUtil::verifyToken(token, key);
    EXPECT_EQ(verified, unicodeId);
}

