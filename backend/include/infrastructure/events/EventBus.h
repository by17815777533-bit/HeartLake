/**
 * @brief 领域事件总线 -- 发布-订阅模式解耦业务模块
 *
 * @details
 * 基于 std::type_index 做事件类型路由，支持同一事件类型注册多个处理器。
 * 所有领域事件继承自 DomainEvent 基类，携带产生时间戳。
 *
 * 线程安全策略：
 * - 使用 recursive_mutex 保护 handlers_ 的读写
 * - publish 时先在锁内拷贝 handler 列表快照，再在锁外逐个调用，
 *   避免回调中再次操作 EventBus 导致死锁
 * - 单个 handler 抛异常不影响后续 handler 的执行
 *
 * 典型事件流：Stone 发布 -> AI 情绪分析 -> 缓存更新 -> 推荐刷新
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <mutex>
#include <functional>
#include <string>
#include <cassert>
#include <any>
#include <drogon/drogon.h>

namespace heartlake::core::events {

/// 领域事件基类，所有具体事件必须继承此类
struct DomainEvent {
    virtual ~DomainEvent() = default;
    int64_t timestamp = std::time(nullptr);  ///< 事件产生的 Unix 时间戳
};

/// 石头发布事件：用户投出一颗新石头时触发
struct StonePublishedEvent : DomainEvent {
    std::string stoneId;
    std::string userId;
    std::string content;
    std::string moodType;
};

/// 纸船发送事件：用户向石头发送纸船回复时触发
struct BoatSentEvent : DomainEvent {
    std::string boatId;
    std::string stoneId;
    std::string senderId;
    std::string content;
};

/**
 * @brief 事件处理器接口
 * @tparam E 事件类型，必须继承自 DomainEvent
 */
template<typename E>
class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    /// 处理事件，实现类需保证线程安全
    virtual void handle(const E& event) = 0;
};

/**
 * @brief 事件总线，全局单例
 *
 * @details
 * 使用 std::any 做类型擦除存储 handler，publish 时通过 any_cast 还原。
 * subscribe 和 publish 均为 O(n)，n 为该事件类型的 handler 数量。
 */
class EventBus {
public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    /**
     * @brief 订阅指定类型的事件
     * @tparam E 事件类型
     * @param handler 事件处理器，通过 shared_ptr 持有以延长生命周期
     */
    template<typename E>
    void subscribe(std::shared_ptr<IEventHandler<E>> handler) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto typeIdx = std::type_index(typeid(E));
        handlers_[typeIdx].push_back([handler](const std::any& event) {
            handler->handle(std::any_cast<const E&>(event));
        });
    }

    /**
     * @brief 发布事件，同步通知所有已订阅的处理器
     * @tparam E 事件类型
     * @param event 事件对象
     * @note 先拷贝 handler 快照再逐个调用，单个 handler 异常不影响其余
     */
    template<typename E>
    void publish(const E& event) {
        // 拷贝handler列表后释放锁，避免回调中再次操作EventBus导致死锁
        std::vector<std::function<void(const std::any&)>> snapshot;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            auto typeIdx = std::type_index(typeid(E));
            auto it = handlers_.find(typeIdx);
            if (it != handlers_.end()) {
                snapshot = it->second;
            }
        }
        std::any wrapped = event;
        for (auto& handler : snapshot) {
            try {
                handler(wrapped);
            } catch (const std::exception& e) {
                LOG_ERROR << "EventBus handler error: " << e.what();
            } catch (...) {
                LOG_ERROR << "EventBus handler unknown error";
            }
        }
    }

    /// 清空所有订阅关系（主要用于测试）
    void clear() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        handlers_.clear();
    }

private:
    EventBus() = default;
    std::unordered_map<std::type_index, std::vector<std::function<void(const std::any&)>>> handlers_;  ///< 事件类型 -> handler 列表
    std::recursive_mutex mutex_;  ///< 递归锁，允许 handler 内部再次 subscribe
};

} // namespace heartlake::core::events
