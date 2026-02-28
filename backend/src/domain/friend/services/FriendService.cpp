/**
 * @file FriendService.cpp
 * @brief 好友领域服务 —— 封装好友关系的核心业务规则
 *
 * 提供协程异步和同步两套接口：
 *   - 异步版本供 Controller 层 co_await 调用
 *   - 同步版本兼容旧代码路径
 *
 * 业务规则：
 *   - 发送请求前检查是否已存在关系（防重复）
 *   - 删除好友时同步清除双向记录
 *   - areFriends 只认 status='accepted' 的关系
 */

#include "domain/friend/services/FriendService.h"
#include "utils/IdGenerator.h"

namespace heartlake::domain::friend_domain {

// ==================== 协程异步接口 ====================

/// 发送好友请求：先查重，再创建 pending 状态的关系记录
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

/// 删除好友：双向清除关系记录
drogon::Task<void> FriendService::removeFriendAsync(const std::string& userId, const std::string& friendId) {
    co_await repository_->deleteBidirectionalAsync(userId, friendId);
}

drogon::Task<std::vector<FriendEntity>> FriendService::getFriendsAsync(const std::string& userId) {
    co_return co_await repository_->findByUserIdAsync(userId);
}

/// 判断两人是否为已接受的好友关系
drogon::Task<bool> FriendService::areFriendsAsync(const std::string& userId, const std::string& friendId) {
    auto friendship = co_await repository_->findByUserAndFriendAsync(userId, friendId);
    co_return friendship && friendship->status == "accepted";
}

// ==================== 同步兼容接口（旧代码路径） ====================

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
