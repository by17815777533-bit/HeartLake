/**
 * @file CircuitBreaker.h
 * @brief 熔断器 - 系统自愈机制
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>

namespace heartlake::utils {

class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };

    CircuitBreaker(int failureThreshold = 5, int resetTimeoutMs = 30000)
        : failureThreshold_(failureThreshold), resetTimeoutMs_(resetTimeoutMs) {}

    template<typename Func>
    auto execute(Func&& func) -> decltype(func()) {
        if (!allowRequest()) {
            throw std::runtime_error("Circuit breaker is OPEN");
        }

        try {
            auto result = func();
            onSuccess();
            return result;
        } catch (...) {
            onFailure();
            throw;
        }
    }

    State getState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = State::CLOSED;
        failureCount_ = 0;
    }

private:
    bool allowRequest() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::CLOSED) return true;
        if (state_ == State::OPEN) {
            auto now = std::chrono::steady_clock::now();
            if (now - lastFailureTime_ > std::chrono::milliseconds(resetTimeoutMs_)) {
                state_ = State::HALF_OPEN;
                return true;
            }
            return false;
        }
        return true; // HALF_OPEN
    }

    void onSuccess() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::HALF_OPEN) {
            state_ = State::CLOSED;
            failureCount_ = 0;
        }
    }

    void onFailure() {
        std::lock_guard<std::mutex> lock(mutex_);
        failureCount_++;
        lastFailureTime_ = std::chrono::steady_clock::now();
        if (failureCount_ >= failureThreshold_) {
            state_ = State::OPEN;
        }
    }

    mutable std::mutex mutex_;
    State state_ = State::CLOSED;
    int failureCount_ = 0;
    int failureThreshold_;
    int resetTimeoutMs_;
    std::chrono::steady_clock::time_point lastFailureTime_;
};

} // namespace heartlake::utils
