/**
 * @file ApplicationServiceFactory.cpp
 * @brief 应用服务工厂 —— 统一注册所有 Application Service 到 DI 容器
 *
 * 启动阶段由 ArchitectureBootstrap 调用 initialize()，
 * 将 Stone / User / Interaction 三个应用服务以 Singleton 方式
 * 注册到 ServiceLocator，后续 Controller 层通过 DI 获取实例。
 *
 * 基础设施依赖（CacheManager、EventBus）使用空删除器包装为 shared_ptr，
 * 避免 DI 容器析构时误释放全局单例。
 */

#include "application/ApplicationServiceFactory.h"
#include "application/StoneApplicationService.h"
#include "application/UserApplicationService.h"
#include "application/InteractionApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include <memory>

namespace heartlake {
namespace application {

void ApplicationServiceFactory::initialize() {
    auto& di = core::di::ServiceLocator::instance();

    // 空删除器包装：CacheManager / EventBus 是全局单例，生命周期由自身管理
    auto cachePtr = std::shared_ptr<core::cache::CacheManager>(
        &core::cache::CacheManager::getInstance(), [](core::cache::CacheManager*){});
    auto eventPtr = std::shared_ptr<core::events::EventBus>(
        &core::events::EventBus::getInstance(), [](core::events::EventBus*){});

    // Stone 服务：依赖领域服务 + 仓储 + 缓存 + 事件总线
    di.registerSingleton<StoneApplicationService>([&di, cachePtr, eventPtr]() {
        return std::make_shared<StoneApplicationService>(
            di.resolve<domain::stone::StoneService>(),
            di.resolve<domain::stone::IStoneRepository>(),
            cachePtr, eventPtr
        );
    });

    // User 服务：依赖用户仓储 + 缓存 + 事件总线
    di.registerSingleton<UserApplicationService>([&di, cachePtr, eventPtr]() {
        return std::make_shared<UserApplicationService>(
            di.resolve<domain::user::IUserRepository>(),
            cachePtr, eventPtr
        );
    });

    // Interaction 服务：涟漪/纸船/通知/临时连接，只需缓存和事件总线
    di.registerSingleton<InteractionApplicationService>([cachePtr, eventPtr]() {
        return std::make_shared<InteractionApplicationService>(cachePtr, eventPtr);
    });

}

} // namespace application
} // namespace heartlake
