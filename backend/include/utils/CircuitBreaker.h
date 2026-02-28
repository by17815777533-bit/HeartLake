/**
 * 熔断器模式实现（Circuit Breaker）
 *
 * 保护下游服务免受级联故障影响。状态机：
 * - CLOSED: 正常放行，连续失败达到阈值后切换到 OPEN
 * - OPEN: 拒绝所有请求并抛出异常，冷却超时后进入 HALF_OPEN
 * - HALF_OPEN: 放行一次试探请求，成功则恢复 CLOSED，失败则重回 OPEN
 *
 * execute() 采用两阶段锁策略：持锁检查状态，释放锁后执行业务逻辑，
 * 避免长时间持锁阻塞其他调用者。
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <stdexcept>

namespace heartlake {
namespace utils {

/**
 * @brief 熔断器 — 三态状态机保护下游服务
 *
 * 保护下游服务免受级联故障影响。状态机：
 * - CLOSED: 正常放行，连续失败达到阈值后切换到 OPEN
 * - OPEN: 拒绝所有请求并抛出异常，冷却超时后进入 HALF_OPEN
 * - HALF_OPEN: 放行一次试探请求，成功则恢复 CLOSED，失败则重回 OPEN
 *
 * execute() 采用两阶段锁策略：持锁检查状态，释放锁后执行业务逻辑，
 * 避免长时间持锁阻塞其他调用者。
 */
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

    /// 获取当前状态，OPEN 超时后自动转为 HALF_OPEN
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

    /**
     * @brief 在熔断器保护下执行业务逻辑
     * @tparam F 可调用对象类型
     * @param func 业务逻辑函数
     * @return func 的返回值
     * @throws std::runtime_error 熔断器处于 OPEN 状态时抛出
     */
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

    /// 手动重置为 CLOSED 状态，清零失败计数
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

    int failureThreshold_;                              ///< 触发熔断的连续失败次数阈值
    std::chrono::milliseconds resetTimeout_;             ///< 熔断后冷却时间
    State state_;                                        ///< 当前状态
    int failureCount_;                                   ///< 当前连续失败计数
    std::chrono::steady_clock::time_point openedAt_;     ///< 进入 OPEN 状态的时间点
    std::mutex mutex_;                                   ///< 保护状态转换的互斥锁
};

} // namespace utils
} // namespace heartlake
