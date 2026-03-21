/**
 * 领域事件处理器集合
 *
 * 实现事件驱动架构中的订阅端，将领域事件与基础设施副作用解耦。
 * 所有 Handler 在启动阶段通过 EventBus::subscribe() 注册，
 * 事件发布时由 EventBus 自动分发到对应 Handler 的 handle() 方法。
 *
 * 当前已注册的事件链路：
 *   StonePublishedEvent -> StonePublishedHandler -> 触发 AI 情绪分析
 *   BoatSentEvent       -> BoatSentHandler       -> 记录审计日志
 *
 * @note 所有 Handler 的 handle() 方法在 EventBus 的调度线程中同步执行，
 *       耗时操作（如 AI 分析）应在内部异步化，避免阻塞事件分发。
 */

#pragma once

#include "infrastructure/events/EventBus.h"
#include "infrastructure/ai/AIService.h"
#include <memory>

namespace heartlake::application::handlers {

/**
 * @brief 石头发布事件处理器
 *
 * 石头投入湖中后，异步触发 AI 情绪分析。
 * 缓存失效由应用服务写链直接负责，不再通过事件层二次回补。
 */
class StonePublishedHandler : public core::events::IEventHandler<core::events::StonePublishedEvent> {
public:
    /**
     * @brief 构造函数
     * @param aiService AI 服务实例，用于发起情感分析请求
     */
    explicit StonePublishedHandler(std::shared_ptr<ai::AIService> aiService)
        : aiService_(aiService) {}

    /**
     * @brief 处理石头发布事件
     * @details 调用 AIService::analyzeSentiment() 异步分析石头内容的情绪倾向，
     *          分析结果通过回调写入数据库。aiService_ 为空时静默跳过（降级模式）。
     * @param event 石头发布事件，包含 stoneId 和 content
     */
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

/**
 * @brief 纸船发送事件处理器
 * @details 记录纸船发送的审计日志，为后续扩展通知推送预留入口。
 */
class BoatSentHandler : public core::events::IEventHandler<core::events::BoatSentEvent> {
public:
    /// @brief 记录纸船发送日志
    void handle(const core::events::BoatSentEvent& event) override {
        LOG_INFO << "Boat sent: " << event.boatId << " for stone: " << event.stoneId;
    }
};

} // namespace heartlake::application::handlers
