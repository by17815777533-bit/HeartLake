/**
 * @file CircuitBreaker.h
 * @brief 熔断器模式实现
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <stdexcept>

namespace heartlake {
namespace utils {

class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };

    /**
     * @param failureThreshold 连续失败次数阈值，达到后熔断
     * @param resetTimeoutMs 熔断后冷却时间（毫秒），超时后进入半开状态
     */
    CircuitBreaker(int failureThreshold, int resetTimeoutMs)
        : failureThreshold_(failureThreshold)
        , resetTimeout_(std::chrono::milliseconds(resetTimeoutMs))
        , state_(State::CLOSED)
        , failureCount_(0) {}

    State getState() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::OPEN) {
            auto now = std::chrono::steady_clock::now();
            if (now - openedAt_ >= resetTimeout_) {
                state_ = State::HALF_OPEN;
            }
        }
        return state_;
    }

    template<typename F>
    auto execute(F&& func) -> decltype(func()) {
        // Phase 1: 持锁检查状态，决定是否允许执行
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (state_ == State::OPEN) {
                auto now = std::chrono::steady_clock::now();
                if (now - openedAt_ >= resetTimeout_) {
                    state_ = State::HALF_OPEN;
                } else {
                    throw std::runtime_error("Circuit breaker is OPEN");
                }
            }
        }

        // Phase 2: 锁外执行 func()，避免持锁阻塞其他调用者
        try {
            auto result = func();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                onSuccess();
            }
            return result;
        } catch (...) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                onFailure();
            }
            throw;
        }
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = State::CLOSED;
        failureCount_ = 0;
    }

private:
    void onSuccess() {
        failureCount_ = 0;
        state_ = State::CLOSED;
    }

    void onFailure() {
        failureCount_++;
        if (failureCount_ >= failureThreshold_) {
            state_ = State::OPEN;
            openedAt_ = std::chrono::steady_clock::now();
        }
    }

    int failureThreshold_;
    std::chrono::milliseconds resetTimeout_;
    State state_;
    int failureCount_;
    std::chrono::steady_clock::time_point openedAt_;
    std::mutex mutex_;
};

} // namespace utils
} // namespace heartlake
