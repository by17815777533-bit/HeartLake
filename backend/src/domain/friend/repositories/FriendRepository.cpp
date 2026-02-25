/**
 * @file FriendRepository.cpp
 * @brief 好友仓储实现 - 异步化改造
 */

#include "domain/friend/repositories/FriendRepository.h"
#include "utils/RequestHelper.h"

namespace heartlake::domain::friend_domain {
using namespace heartlake::utils;

FriendEntity FriendRepository::rowToEntity(const drogon::orm::Row& row) {
    FriendEntity entity;
    entity.friendshipId = row["friendship_id"].as<std::string>();
    entity.userId = row["user_id"].as<std::string>();
    entity.friendId = row["friend_id"].as<std::string>();
    entity.status = row["status"].as<std::string>();
    if (!row["created_at"].isNull()) {
        entity.createdAt = row["created_at"].as<std::string>();
    }
    return entity;
}

// ==================== Async Methods ====================

drogon::Task<void> FriendRepository::saveAsync(const FriendEntity& friendship) {
    auto db = drogon::app().getDbClient("default");
    co_await db->execSqlCoro(
        "INSERT INTO friends (friendship_id, user_id, friend_id, status, created_at) "
        "VALUES ($1, $2, $3, $4, NOW())",
        friendship.friendshipId, friendship.userId, friendship.friendId, friendship.status
    );
}

drogon::Task<std::optional<FriendEntity>> FriendRepository::findByUserAndFriendAsync(
    const std::string& userId, const std::string& friendId) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        "SELECT * FROM friends WHERE (user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)",
        userId, friendId
    );
    auto row = safeRow(result);
    if (!row) co_return std::nullopt;
    co_return rowToEntity(*row);
}

drogon::Task<std::vector<FriendEntity>> FriendRepository::findByUserIdAsync(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        "SELECT * FROM friends WHERE (user_id = $1 OR friend_id = $1) AND status = 'accepted'", userId
    );
    std::vector<FriendEntity> friends;
    for (const auto& row : result) {
        friends.push_back(rowToEntity(row));
    }
    co_return friends;
}

drogon::Task<std::vector<FriendEntity>> FriendRepository::findAllByUserIdAsync(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        "SELECT * FROM friends WHERE user_id = $1 OR friend_id = $1", userId
    );
    std::vector<FriendEntity> friends;
    for (const auto& row : result) {
        friends.push_back(rowToEntity(row));
    }
    co_return friends;
}

drogon::Task<void> FriendRepository::updateStatusAsync(const std::string& friendshipId, const std::string& status) {
    auto db = drogon::app().getDbClient("default");
    co_await db->execSqlCoro("UPDATE friends SET status = $1 WHERE friendship_id = $2", status, friendshipId);
}

drogon::Task<void> FriendRepository::deleteByIdAsync(const std::string& friendshipId) {
    auto db = drogon::app().getDbClient("default");
    co_await db->execSqlCoro("DELETE FROM friends WHERE friendship_id = $1", friendshipId);
}

drogon::Task<void> FriendRepository::deleteBidirectionalAsync(const std::string& userId, const std::string& friendId) {
    auto db = drogon::app().getDbClient("default");
    co_await db->execSqlCoro(
        "DELETE FROM friends WHERE (user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)",
        userId, friendId
    );
}

// ==================== Legacy Sync Methods ====================

void FriendRepository::save(const FriendEntity& friendship) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlSync(
        "INSERT INTO friends (friendship_id, user_id, friend_id, status, created_at) "
        "VALUES ($1, $2, $3, $4, NOW())",
        friendship.friendshipId, friendship.userId, friendship.friendId, friendship.status
    );
}

std::optional<FriendEntity> FriendRepository::findByUserAndFriend(const std::string& userId, const std::string& friendId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT * FROM friends WHERE (user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)",
        userId, friendId
    );
    auto row = safeRow(result);
    if (!row) return std::nullopt;
    return rowToEntity(*row);
}

std::vector<FriendEntity> FriendRepository::findByUserId(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT * FROM friends WHERE (user_id = $1 OR friend_id = $1) AND status = 'accepted'", userId
    );
    std::vector<FriendEntity> friends;
    for (const auto& row : result) {
        friends.push_back(rowToEntity(row));
    }
    return friends;
}

void FriendRepository::updateStatus(const std::string& friendshipId, const std::string& status) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlSync("UPDATE friends SET status = $1 WHERE friendship_id = $2", status, friendshipId);
}

void FriendRepository::deleteById(const std::string& friendshipId) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlSync("DELETE FROM friends WHERE friendship_id = $1", friendshipId);
}

} // namespace heartlake::domain::friend_domain
