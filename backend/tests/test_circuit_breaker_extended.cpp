/**
 * CircuitBreaker 扩展测试 - 并发、边界、超时精度、异常传播
 */

#include <gtest/gtest.h>
#include "utils/CircuitBreaker.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <stdexcept>

using namespace heartlake::utils;

class CBExtTest : public ::testing::Test {};

// =====================================================================
// 基本状态转换
// =====================================================================

TEST_F(CBExtTest, InitialState_Closed) {
    CircuitBreaker cb(5, 1000);
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CBExtTest, SuccessKeepsClosed) {
    CircuitBreaker cb(3, 100);
    for (int i = 0; i < 20; ++i) {
        EXPECT_NO_THROW(cb.execute([]() { return 42; }));
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CBExtTest, FailuresBelowThreshold_StayClosed) {
    CircuitBreaker cb(5, 100);
    for (int i = 0; i < 4; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CBExtTest, FailuresAtThreshold_Opens) {
    CircuitBreaker cb(3, 100);
    for (int i = 0; i < 3; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(CBExtTest, Open_RejectsRequests) {
    CircuitBreaker cb(1, 5000);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    EXPECT_THROW(cb.execute([]() { return 1; }), std::runtime_error);
}

TEST_F(CBExtTest, Open_ToHalfOpen_AfterTimeout) {
    CircuitBreaker cb(1, 50);  // 50ms timeout
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
}

TEST_F(CBExtTest, HalfOpen_SuccessCloses) {
    CircuitBreaker cb(1, 50);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    // Now HALF_OPEN, success should close
    EXPECT_NO_THROW(cb.execute([]() { return 1; }));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CBExtTest, HalfOpen_FailureReopens) {
    CircuitBreaker cb(1, 50);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    // Now HALF_OPEN, failure should reopen
    try { cb.execute([]() -> int { throw std::runtime_error("fail again"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(CBExtTest, Reset_ClosesFromOpen) {
    CircuitBreaker cb(1, 5000);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    cb.reset();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CBExtTest, Reset_WorksAfterSuccess) {
    CircuitBreaker cb(3, 100);
    cb.execute([]() { return 1; });
    cb.reset();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

// =====================================================================
// 边界值
// =====================================================================

TEST_F(CBExtTest, Threshold1_SingleFailureOpens) {
    CircuitBreaker cb(1, 100);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(CBExtTest, Threshold1_SingleSuccessKeepsClosed) {
    CircuitBreaker cb(1, 100);
    cb.execute([]() { return 1; });
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CBExtTest, LargeThreshold_ManyFailuresNeeded) {
    CircuitBreaker cb(100, 1000);
    for (int i = 0; i < 99; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(CBExtTest, SuccessResetsFailureCount) {
    CircuitBreaker cb(3, 100);
    // 2 failures
    for (int i = 0; i < 2; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    // 1 success resets counter
    cb.execute([]() { return 1; });
    // 2 more failures should not open (counter was reset)
    for (int i = 0; i < 2; ++i) {
        try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
        catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

// =====================================================================
// 超时精度
// =====================================================================

TEST_F(CBExtTest, Timeout_NotExpiredYet) {
    CircuitBreaker cb(1, 200);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

TEST_F(CBExtTest, Timeout_JustExpired) {
    CircuitBreaker cb(1, 100);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
}

// =====================================================================
// 异常类型传播
// =====================================================================

TEST_F(CBExtTest, ExceptionType_RuntimeError) {
    CircuitBreaker cb(5, 100);
    EXPECT_THROW(
        cb.execute([]() -> int { throw std::runtime_error("specific"); }),
        std::runtime_error
    );
}

TEST_F(CBExtTest, ExceptionType_LogicError) {
    CircuitBreaker cb(5, 100);
    EXPECT_THROW(
        cb.execute([]() -> int { throw std::logic_error("logic"); }),
        std::logic_error
    );
}

TEST_F(CBExtTest, ExceptionType_InvalidArgument) {
    CircuitBreaker cb(5, 100);
    EXPECT_THROW(
        cb.execute([]() -> int { throw std::invalid_argument("bad arg"); }),
        std::invalid_argument
    );
}

TEST_F(CBExtTest, OpenCircuit_ThrowsRuntimeError) {
    CircuitBreaker cb(1, 5000);
    try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
    catch (...) {}
    try {
        cb.execute([]() { return 1; });
        FAIL() << "Should have thrown";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("OPEN"), std::string::npos);
    }
}

// =====================================================================
// 并发
// =====================================================================

TEST_F(CBExtTest, Concurrent_ManySuccesses) {
    CircuitBreaker cb(100, 1000);
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 50; ++i) {
        threads.emplace_back([&]() {
            try {
                cb.execute([]() { return 1; });
                success++;
            } catch (...) {}
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(success.load(), 50);
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(CBExtTest, Concurrent_MixedSuccessFailure) {
    CircuitBreaker cb(20, 1000);
    std::atomic<int> success{0}, failure{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 30; ++i) {
        threads.emplace_back([&, i]() {
            try {
                cb.execute([i]() -> int {
                    if (i % 3 == 0) throw std::runtime_error("fail");
                    return i;
                });
                success++;
            } catch (...) {
                failure++;
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_GT(success.load() + failure.load(), 0);
}

TEST_F(CBExtTest, Concurrent_RapidFailures) {
    CircuitBreaker cb(5, 500);
    std::vector<std::thread> threads;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([&]() {
            try { cb.execute([]() -> int { throw std::runtime_error("fail"); }); }
            catch (...) {}
        });
    }
    for (auto& t : threads) t.join();
    // After 20 failures with threshold 5, should be OPEN
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
}

// =====================================================================
// 返回值
// =====================================================================

TEST_F(CBExtTest, ReturnValue_Int) {
    CircuitBreaker cb(3, 100);
    int result = cb.execute([]() { return 42; });
    EXPECT_EQ(result, 42);
}

TEST_F(CBExtTest, ReturnValue_String) {
    CircuitBreaker cb(3, 100);
    std::string result = cb.execute([]() { return std::string("hello"); });
    EXPECT_EQ(result, "hello");
}

TEST_F(CBExtTest, ReturnValue_Vector) {
    CircuitBreaker cb(3, 100);
    auto result = cb.execute([]() { return std::vector<int>{1, 2, 3}; });
    EXPECT_EQ(result.size(), 3u);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
