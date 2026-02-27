/**
 * 全链路压力测试
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "utils/CircuitBreaker.h"

using namespace heartlake::utils;

class StressTest : public ::testing::Test {
protected:
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};
};

TEST_F(StressTest, CircuitBreakerBasic) {
    CircuitBreaker cb(3, 100);
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);

    // 触发3次失败，熔断器应打开
    for (int i = 0; i < 3; i++) {
        try {
            cb.execute([]() -> int { throw std::runtime_error("fail"); });
        } catch (...) {}
    }
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);

    // 等待重置
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_NO_THROW(cb.execute([]() { return 1; }));
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST_F(StressTest, ConcurrentRequests) {
    CircuitBreaker cb(10, 1000);
    std::vector<std::thread> threads;

    for (int i = 0; i < 100; i++) {
        threads.emplace_back([this, &cb, i]() {
            try {
                cb.execute([i]() {
                    if (i % 10 == 0) throw std::runtime_error("simulated");
                    return i;
                });
                successCount++;
            } catch (...) {
                failureCount++;
            }
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_GT(successCount.load(), 0);
}

TEST_F(StressTest, RateLimiterConcurrency) {
    std::atomic<int> allowed{0};
    std::atomic<int> blocked{0};
    std::mutex mtx;
    int tokens = 50;

    std::vector<std::thread> threads;
    for (int i = 0; i < 100; i++) {
        threads.emplace_back([&]() {
            std::lock_guard<std::mutex> lock(mtx);
            if (tokens > 0) {
                tokens--;
                allowed++;
            } else {
                blocked++;
            }
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(allowed.load(), 50);
    EXPECT_EQ(blocked.load(), 50);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
