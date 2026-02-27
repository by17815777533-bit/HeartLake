/**
 * 应用服务工厂实现
 */

#include "application/ApplicationServiceFactory.h"
#include "application/StoneApplicationService.h"
#include "application/UserApplicationService.h"
#include "application/InteractionApplicationService.h"
#include "application/FriendApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include <memory>

namespace heartlake {
namespace application {

void ApplicationServiceFactory::initialize() {
    auto& di = core::di::ServiceLocator::instance();

    // 使用空删除器包装单例，避免重复拷贝
    auto cachePtr = std::shared_ptr<core::cache::CacheManager>(
        &core::cache::CacheManager::getInstance(), [](core::cache::CacheManager*){});
    auto eventPtr = std::shared_ptr<core::events::EventBus>(
        &core::events::EventBus::getInstance(), [](core::events::EventBus*){});

    // 注册 StoneApplicationService
    di.registerSingleton<StoneApplicationService>([&di, cachePtr, eventPtr]() {
        return std::make_shared<StoneApplicationService>(
            di.resolve<domain::stone::StoneService>(),
            di.resolve<domain::stone::IStoneRepository>(),
            cachePtr, eventPtr
        );
    });

    // 注册 UserApplicationService
    di.registerSingleton<UserApplicationService>([&di, cachePtr, eventPtr]() {
        return std::make_shared<UserApplicationService>(
            di.resolve<domain::user::IUserRepository>(),
            cachePtr, eventPtr
        );
    });

    // 注册 InteractionApplicationService
    di.registerSingleton<InteractionApplicationService>([cachePtr, eventPtr]() {
        return std::make_shared<InteractionApplicationService>(cachePtr, eventPtr);
    });

    // 注册 FriendApplicationService
    di.registerSingleton<FriendApplicationService>([&di, cachePtr, eventPtr]() {
        return std::make_shared<FriendApplicationService>(
            di.resolve<domain::friend_domain::FriendService>(),
            di.resolve<domain::friend_domain::IFriendRepository>(),
            cachePtr, eventPtr
        );
    });
}

} // namespace application
} // namespace heartlake
