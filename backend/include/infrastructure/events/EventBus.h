/**
 * 事件总线 - 领域事件发布订阅
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

// 基础事件类
struct DomainEvent {
    virtual ~DomainEvent() = default;
    int64_t timestamp = std::time(nullptr);
};

// 石头发布事件
struct StonePublishedEvent : DomainEvent {
    std::string stoneId;
    std::string userId;
    std::string content;
    std::string moodType;
};

// 情绪分析完成事件
struct EmotionAnalyzedEvent : DomainEvent {
    std::string stoneId;
    float emotionScore;
    std::string detectedMood;
};

// 涟漪创建事件
struct RippleCreatedEvent : DomainEvent {
    std::string rippleId;
    std::string stoneId;
    std::string userId;
};

// 纸船发送事件
struct BoatSentEvent : DomainEvent {
    std::string boatId;
    std::string stoneId;
    std::string senderId;
    std::string content;
};

// 事件处理器接口
template<typename E>
class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void handle(const E& event) = 0;
};

// 事件总线
class EventBus {
public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    template<typename E>
    void subscribe(std::shared_ptr<IEventHandler<E>> handler) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto typeIdx = std::type_index(typeid(E));
        handlers_[typeIdx].push_back([handler](const std::any& event) {
            handler->handle(std::any_cast<const E&>(event));
        });
    }

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

    void clear() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        handlers_.clear();
    }

private:
    EventBus() = default;
    std::unordered_map<std::type_index, std::vector<std::function<void(const std::any&)>>> handlers_;
    std::recursive_mutex mutex_;
};

} // namespace heartlake::core::events
