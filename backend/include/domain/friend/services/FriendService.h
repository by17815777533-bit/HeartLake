/**
 * @file FriendService.h
 * @brief 好友领域服务 - 异步化改造
 */

#pragma once

#include "domain/friend/repositories/IFriendRepository.h"
#include <memory>
#include <drogon/drogon.h>

namespace heartlake::domain::friend_domain {

class FriendService {
public:
    explicit FriendService(std::shared_ptr<IFriendRepository> repository)
        : repository_(repository) {}

    // Async methods
    drogon::Task<void> sendFriendRequestAsync(const std::string& userId, const std::string& friendId);
    drogon::Task<void> acceptFriendRequestAsync(const std::string& friendshipId);
    drogon::Task<void> rejectFriendRequestAsync(const std::string& friendshipId);
    drogon::Task<void> removeFriendAsync(const std::string& userId, const std::string& friendId);
    drogon::Task<std::vector<FriendEntity>> getFriendsAsync(const std::string& userId);
    drogon::Task<bool> areFriendsAsync(const std::string& userId, const std::string& friendId);

    // Legacy sync methods
    void sendFriendRequest(const std::string& userId, const std::string& friendId);
    void acceptFriendRequest(const std::string& friendshipId);
    void rejectFriendRequest(const std::string& friendshipId);
    void removeFriend(const std::string& userId, const std::string& friendId);
    std::vector<FriendEntity> getFriends(const std::string& userId);
    bool areFriends(const std::string& userId, const std::string& friendId);

private:
    std::shared_ptr<IFriendRepository> repository_;
};

} // namespace heartlake::domain::friend_domain
