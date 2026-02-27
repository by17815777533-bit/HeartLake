/**
 * UserApplicationService 模块接口定义
 */

#pragma once

#include "domain/user/repositories/IUserRepository.h"
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include <memory>
#include <string>
#include <vector>
#include <json/json.h>

namespace heartlake {
namespace application {

class UserApplicationService {
private:
    std::shared_ptr<domain::user::IUserRepository> userRepository_;
    std::shared_ptr<core::cache::CacheManager> cacheManager_;
    std::shared_ptr<core::events::EventBus> eventBus_;

public:
    UserApplicationService(
        std::shared_ptr<domain::user::IUserRepository> userRepository,
        std::shared_ptr<core::cache::CacheManager> cacheManager,
        std::shared_ptr<core::events::EventBus> eventBus
    ) : userRepository_(userRepository),
        cacheManager_(cacheManager),
        eventBus_(eventBus) {}

    /**
     * 获取用户资料
     */
    Json::Value getUserProfile(const std::string& userId);

    /**
     * 更新用户资料
     */
    void updateUserProfile(
        const std::string& userId,
        const Json::Value& updates
    );

    /**
     * 搜索用户
     */
    Json::Value searchUsers(
        const std::string& keyword,
        int page,
        int pageSize
    );

    /**
     * 批量获取用户信息
     */
    Json::Value getUsersBatch(const std::vector<std::string>& userIds);
};

} // namespace application
} // namespace heartlake
