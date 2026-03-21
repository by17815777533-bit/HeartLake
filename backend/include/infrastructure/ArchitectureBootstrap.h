/**
 * @brief DDD 架构引导器
 *
 * @details
 * 按照 Infrastructure -> Domain -> Application -> EventHandler 的顺序
 * 完成整个应用的依赖注入和服务初始化。遵循 DDD 分层架构约束：
 * 上层可依赖下层，下层不可反向依赖上层。
 */

#pragma once

#include "infrastructure/di/ServiceLocator.h"
#include "infrastructure/events/EventBus.h"
#include "infrastructure/cache/CacheManager.h"
#include "application/ApplicationServiceFactory.h"
#include "application/handlers/EventHandlers.h"
#include "domain/stone/repositories/IStoneRepository.h"
#include "domain/stone/repositories/StoneRepository.h"
#include "domain/stone/services/StoneService.h"
#include "domain/user/repositories/IUserRepository.h"
#include "domain/user/repositories/UserRepository.h"
#include "domain/friend/repositories/IFriendRepository.h"
#include "domain/friend/repositories/FriendRepository.h"
#include "domain/friend/services/FriendService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/SummaryService.h"
#include "infrastructure/services/LakeGodGuardianService.h"
#include "infrastructure/services/ResonanceSearchService.h"
#include "infrastructure/vector/MilvusClient.h"
#include "infrastructure/services/EmotionTrackingService.h"
#include "infrastructure/services/GuardianIncentiveService.h"

// 前向声明实现类
namespace heartlake::domain::stone {
    class StoneRepository;
}
namespace heartlake::domain::user {
    class UserRepository;
}
namespace heartlake::domain::friend_domain {
    class FriendRepository;
}

namespace heartlake {

/**
 * @brief 架构引导器 -- 应用启动时的"总装配线"
 *
 * @details
 * 严格按照 DDD 分层顺序完成初始化：
 *   1. 基础设施层 -- 缓存、Milvus 向量库、AI 摘要等
 *   2. 领域层     -- 仓储接口绑定、领域服务注册
 *   3. 应用层     -- 应用服务工厂
 *   4. 事件处理器 -- 领域事件 -> 处理器的订阅关系
 *
 * 单例通过 ServiceLocator 注册时使用空删除器包装，
 * 避免 DI 容器析构时 double-free 静态单例。
 */
class ArchitectureBootstrap {
public:
    /**
     * @brief 初始化整个架构
     * @details 按 基础设施 -> 领域 -> 应用 -> 事件处理器 的顺序依次启动，
     *          任何一步失败都会抛出异常终止启动流程
     */
    static void initialize() {
        LOG_INFO << "=== Initializing HeartLake Architecture ===";

        // 1. 初始化基础设施
        initializeInfrastructure();

        // 2. 初始化领域层
        initializeDomain();

        // 3. 初始化应用层
        initializeApplication();

        // 4. 注册事件处理器
        registerEventHandlers();

        LOG_INFO << "=== Architecture Initialization Complete ===";
    }

private:
    /// 初始化基础设施层：缓存、事件总线、Milvus、AI 摘要服务
    static void initializeInfrastructure() {
        LOG_INFO << "Initializing Infrastructure Layer...";

        auto& di = core::di::ServiceLocator::instance();

        // 注册缓存管理器 (单例，使用空删除器避免double-free)
        di.registerInstance<core::cache::CacheManager>(
            std::shared_ptr<core::cache::CacheManager>(
                &core::cache::CacheManager::getInstance(), [](core::cache::CacheManager*){}
            )
        );

        // 注册事件总线 (单例，使用空删除器避免double-free)
        di.registerInstance<core::events::EventBus>(
            std::shared_ptr<core::events::EventBus>(
                &core::events::EventBus::getInstance(), [](core::events::EventBus*){}
            )
        );

        // 初始化Milvus向量数据库
        infrastructure::MilvusClient::getInstance().initialize();

        // 初始化AI摘要服务
        ai::SummaryService::getInstance().initialize(Json::Value());

        // 注意: ResonanceSearchService、LakeGodGuardianService、EmotionTrackingService
        // 在 main.cpp 启动初始化阶段按正确顺序启动（Redis初始化之后）

        // EdgeAIEngine 在 main.cpp 中集中初始化，避免重复初始化导致冷启动抖动。

        LOG_INFO << "Infrastructure Layer initialized";
    }

    /// 初始化领域层：绑定仓储接口、注册领域服务
    static void initializeDomain() {
        LOG_INFO << "Initializing Domain Layer...";

        auto& di = core::di::ServiceLocator::instance();

        // 注册仓储
        di.registerSingleton<domain::stone::IStoneRepository, domain::stone::StoneRepository>();
        di.registerSingleton<domain::user::IUserRepository, domain::user::UserRepository>();
        di.registerSingleton<domain::friend_domain::IFriendRepository, domain::friend_domain::FriendRepository>();

        // 注册领域服务
        di.registerSingleton<domain::stone::StoneService>([&di]() {
            return std::make_shared<domain::stone::StoneService>(
                di.resolve<domain::stone::IStoneRepository>()
            );
        });

        di.registerSingleton<domain::friend_domain::FriendService>([&di]() {
            return std::make_shared<domain::friend_domain::FriendService>(
                di.resolve<domain::friend_domain::IFriendRepository>()
            );
        });


        LOG_INFO << "Domain Layer initialized";
    }

    /// 初始化应用层：通过 ApplicationServiceFactory 装配应用服务
    static void initializeApplication() {
        LOG_INFO << "Initializing Application Layer...";

        application::ApplicationServiceFactory::initialize();

        LOG_INFO << "Application Layer initialized";
    }

    /// 注册领域事件处理器：建立事件 -> handler 的订阅关系
    static void registerEventHandlers() {
        LOG_INFO << "Registering Event Handlers...";

        try {
            auto& eventBus = core::events::EventBus::getInstance();

            // 注册石头发布事件处理器 (触发 AI 情绪分析)
            auto aiService = std::shared_ptr<ai::AIService>(&ai::AIService::getInstance(), [](ai::AIService*){});
            auto stonePublishedHandler = std::make_shared<application::handlers::StonePublishedHandler>(aiService);
            eventBus.subscribe<core::events::StonePublishedEvent>(stonePublishedHandler);

            // 注册纸船发送事件处理器
            auto boatSentHandler = std::make_shared<application::handlers::BoatSentHandler>();
            eventBus.subscribe<core::events::BoatSentEvent>(boatSentHandler);

            LOG_INFO << "Event Handlers registered successfully";
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to register event handlers: " << e.what();
            throw;
        }
    }
};

} // namespace heartlake
