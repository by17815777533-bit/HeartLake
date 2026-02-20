/**
 * @file FriendRepository.h
 * @brief 好友仓储实现
 */

#pragma once

#include "IFriendRepository.h"
#include <drogon/drogon.h>

namespace heartlake::domain::friend_domain {

class FriendRepository : public IFriendRepository {
public:
    // Async methods
    drogon::Task<void> saveAsync(const FriendEntity& friendship) override;
    drogon::Task<std::optional<FriendEntity>> findByUserAndFriendAsync(const std::string& userId, const std::string& friendId) override;
    drogon::Task<std::vector<FriendEntity>> findByUserIdAsync(const std::string& userId) override;
    drogon::Task<std::vector<FriendEntity>> findAllByUserIdAsync(const std::string& userId) override;
    drogon::Task<void> updateStatusAsync(const std::string& friendshipId, const std::string& status) override;
    drogon::Task<void> deleteByIdAsync(const std::string& friendshipId) override;
    drogon::Task<void> deleteBidirectionalAsync(const std::string& userId, const std::string& friendId) override;

    // Legacy sync methods
    void save(const FriendEntity& friendship) override;
    std::optional<FriendEntity> findByUserAndFriend(const std::string& userId, const std::string& friendId) override;
    std::vector<FriendEntity> findByUserId(const std::string& userId) override;
    void updateStatus(const std::string& friendshipId, const std::string& status) override;
    void deleteById(const std::string& friendshipId) override;

private:
    static FriendEntity rowToEntity(const drogon::orm::Row& row);
};

} // namespace heartlake::domain::friend_domain
