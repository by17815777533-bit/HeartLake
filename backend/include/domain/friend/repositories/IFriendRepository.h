/**
 * @file IFriendRepository.h
 * @brief 好友仓储接口
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <drogon/drogon.h>

namespace heartlake::domain::friend_domain {

struct FriendEntity {
    std::string friendshipId;
    std::string userId;
    std::string friendId;
    std::string status;
    std::string createdAt;
    int64_t ttlSeconds = 86400; // 24h TTL
};

class IFriendRepository {
public:
    virtual ~IFriendRepository() = default;

    // Async methods
    virtual drogon::Task<void> saveAsync(const FriendEntity& friendship) = 0;
    virtual drogon::Task<std::optional<FriendEntity>> findByUserAndFriendAsync(const std::string& userId, const std::string& friendId) = 0;
    virtual drogon::Task<std::vector<FriendEntity>> findByUserIdAsync(const std::string& userId) = 0;
    virtual drogon::Task<std::vector<FriendEntity>> findAllByUserIdAsync(const std::string& userId) = 0;
    virtual drogon::Task<void> updateStatusAsync(const std::string& friendshipId, const std::string& status) = 0;
    virtual drogon::Task<void> deleteByIdAsync(const std::string& friendshipId) = 0;
    virtual drogon::Task<void> deleteBidirectionalAsync(const std::string& oderId, const std::string& friendId) = 0;

    // Legacy sync methods (deprecated)
    virtual void save(const FriendEntity& friendship) = 0;
    virtual std::optional<FriendEntity> findByUserAndFriend(const std::string& userId, const std::string& friendId) = 0;
    virtual std::vector<FriendEntity> findByUserId(const std::string& userId) = 0;
    virtual void updateStatus(const std::string& friendshipId, const std::string& status) = 0;
    virtual void deleteById(const std::string& friendshipId) = 0;
};

} // namespace heartlake::domain::friend_domain
