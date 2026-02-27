/**
 * FriendApplicationService 模块接口定义
 */

#pragma once

#include <drogon/drogon.h>
#include "domain/friend/services/FriendService.h"
#include "domain/friend/repositories/IFriendRepository.h"
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include <memory>
#include <json/json.h>

namespace heartlake::application {

class FriendApplicationService {
private:
    std::shared_ptr<domain::friend_domain::FriendService> friendService_;
    std::shared_ptr<domain::friend_domain::IFriendRepository> repository_;
    std::shared_ptr<core::cache::CacheManager> cacheManager_;
    std::shared_ptr<core::events::EventBus> eventBus_;

public:
    FriendApplicationService() = default;

    FriendApplicationService(
        std::shared_ptr<domain::friend_domain::FriendService> friendService,
        std::shared_ptr<domain::friend_domain::IFriendRepository> repository,
        std::shared_ptr<core::cache::CacheManager> cacheManager,
        std::shared_ptr<core::events::EventBus> eventBus
    ) : friendService_(friendService),
        repository_(repository),
        cacheManager_(cacheManager),
        eventBus_(eventBus) {}

    drogon::Task<Json::Value> sendFriendRequestAsync(
        const std::string& fromUserId,
        const std::string& toUserId,
        const std::string& message
    );

    drogon::Task<Json::Value> acceptFriendRequestAsync(
        const std::string& userId,
        const std::string& friendshipId
    );

    drogon::Task<Json::Value> rejectFriendRequestAsync(
        const std::string& userId,
        const std::string& friendshipId
    );

    drogon::Task<Json::Value> removeFriendAsync(
        const std::string& userId,
        const std::string& friendId
    );

    drogon::Task<Json::Value> getFriendsListAsync(const std::string& userId);
    drogon::Task<Json::Value> getReceivedRequestsAsync(const std::string& userId);
    drogon::Task<Json::Value> getSentRequestsAsync(const std::string& userId);
};

} // namespace heartlake::application
