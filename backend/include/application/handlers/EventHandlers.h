/**
 * 事件处理器
 */

#pragma once

#include "infrastructure/events/EventBus.h"
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/ai/AIService.h"
#include <memory>

namespace heartlake::application::handlers {

// 石头发布事件处理器
class StonePublishedHandler : public core::events::IEventHandler<core::events::StonePublishedEvent> {
public:
    explicit StonePublishedHandler(std::shared_ptr<ai::AIService> aiService)
        : aiService_(aiService) {}

    void handle(const core::events::StonePublishedEvent& event) override {
        // 触发AI情绪分析
        if (aiService_) {
            aiService_->analyzeSentiment(event.content,
                [stoneId = event.stoneId](float /*score*/, const std::string& /*mood*/, const std::string& error) {
                    if (error.empty()) {
                        LOG_DEBUG << "Emotion analyzed for stone: " << stoneId;
                    }
                });
        }
    }

private:
    std::shared_ptr<ai::AIService> aiService_;
};

// 情绪分析完成事件处理器
class EmotionAnalyzedHandler : public core::events::IEventHandler<core::events::EmotionAnalyzedEvent> {
public:
    explicit EmotionAnalyzedHandler(std::shared_ptr<core::cache::CacheManager> cache)
        : cache_(cache) {}

    void handle(const core::events::EmotionAnalyzedEvent& event) override {
        if (cache_) {
            cache_->invalidate("stone:" + event.stoneId);
        }
    }

private:
    std::shared_ptr<core::cache::CacheManager> cache_;
};

// 涟漪创建事件处理器
class RippleCreatedHandler : public core::events::IEventHandler<core::events::RippleCreatedEvent> {
public:
    explicit RippleCreatedHandler(std::shared_ptr<core::cache::CacheManager> cache)
        : cache_(cache) {}

    void handle(const core::events::RippleCreatedEvent& event) override {
        if (cache_) {
            cache_->invalidate("stone:" + event.stoneId);
        }
    }

private:
    std::shared_ptr<core::cache::CacheManager> cache_;
};

// 纸船发送事件处理器
class BoatSentHandler : public core::events::IEventHandler<core::events::BoatSentEvent> {
public:
    void handle(const core::events::BoatSentEvent& event) override {
        LOG_INFO << "Boat sent: " << event.boatId << " for stone: " << event.stoneId;
    }
};

} // namespace heartlake::application::handlers
