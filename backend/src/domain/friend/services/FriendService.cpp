/**
 * 好友领域服务实现 - 异步化改造
 */

#include "domain/friend/services/FriendService.h"
#include "utils/IdGenerator.h"

namespace heartlake::domain::friend_domain {

// ==================== Async Methods ====================

drogon::Task<void> FriendService::sendFriendRequestAsync(const std::string& userId, const std::string& friendId) {
    auto existing = co_await repository_->findByUserAndFriendAsync(userId, friendId);
    if (existing) throw std::runtime_error("好友请求已存在");

    FriendEntity friendship;
    friendship.friendshipId = utils::IdGenerator::generateUUID();
    friendship.userId = userId;
    friendship.friendId = friendId;
    friendship.status = "pending";
    co_await repository_->saveAsync(friendship);
}

drogon::Task<void> FriendService::acceptFriendRequestAsync(const std::string& friendshipId) {
    co_await repository_->updateStatusAsync(friendshipId, "accepted");
}

drogon::Task<void> FriendService::rejectFriendRequestAsync(const std::string& friendshipId) {
    co_await repository_->updateStatusAsync(friendshipId, "rejected");
}

drogon::Task<void> FriendService::removeFriendAsync(const std::string& userId, const std::string& friendId) {
    co_await repository_->deleteBidirectionalAsync(userId, friendId);
}

drogon::Task<std::vector<FriendEntity>> FriendService::getFriendsAsync(const std::string& userId) {
    co_return co_await repository_->findByUserIdAsync(userId);
}

drogon::Task<bool> FriendService::areFriendsAsync(const std::string& userId, const std::string& friendId) {
    auto friendship = co_await repository_->findByUserAndFriendAsync(userId, friendId);
    co_return friendship && friendship->status == "accepted";
}

// ==================== Legacy Sync Methods ====================

void FriendService::sendFriendRequest(const std::string& userId, const std::string& friendId) {
    auto existing = repository_->findByUserAndFriend(userId, friendId);
    if (existing) throw std::runtime_error("好友请求已存在");

    FriendEntity friendship;
    friendship.friendshipId = utils::IdGenerator::generateUUID();
    friendship.userId = userId;
    friendship.friendId = friendId;
    friendship.status = "pending";
    repository_->save(friendship);
}

void FriendService::acceptFriendRequest(const std::string& friendshipId) {
    repository_->updateStatus(friendshipId, "accepted");
}

void FriendService::rejectFriendRequest(const std::string& friendshipId) {
    repository_->updateStatus(friendshipId, "rejected");
}

void FriendService::removeFriend(const std::string& userId, const std::string& friendId) {
    auto friendship = repository_->findByUserAndFriend(userId, friendId);
    if (friendship) {
        repository_->deleteById(friendship->friendshipId);
    }
}

std::vector<FriendEntity> FriendService::getFriends(const std::string& userId) {
    return repository_->findByUserId(userId);
}

bool FriendService::areFriends(const std::string& userId, const std::string& friendId) {
    auto friendship = repository_->findByUserAndFriend(userId, friendId);
    return friendship && friendship->status == "accepted";
}

} // namespace heartlake::domain::friend_domain
