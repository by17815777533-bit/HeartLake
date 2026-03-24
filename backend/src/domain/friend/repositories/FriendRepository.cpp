/**
 * @file FriendRepository.cpp
 * @brief 好友仓储实现 —— 封装 friends 表的持久化操作
 *
 * 好友关系是双向的：查询时同时匹配 user_id 和 friend_id 两个方向，
 * 删除时使用 deleteBidirectionalAsync 一次清除双向记录。
 */

#include "domain/friend/repositories/FriendRepository.h"
#include "utils/RequestHelper.h"

namespace {
constexpr const char kFriendSelectColumns[] =
    "friendship_id, user_id, friend_id, status, created_at";
}

namespace heartlake::domain::friend_domain {
using namespace heartlake::utils;

/// 将数据库行映射为 FriendEntity，created_at 可能为 NULL 需做空值保护
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

// ==================== 协程异步接口 ====================

/// 异步插入好友关系记录
drogon::Task<void> FriendRepository::saveAsync(const FriendEntity& friendship) {
    auto db = drogon::app().getDbClient("default");
    co_await db->execSqlCoro(
        "INSERT INTO friends (friendship_id, user_id, friend_id, status, created_at) "
        "VALUES ($1, $2, $3, $4, NOW())",
        friendship.friendshipId, friendship.userId, friendship.friendId, friendship.status
    );
}

/// 双向查找好友关系（A→B 或 B→A 均匹配）
drogon::Task<std::optional<FriendEntity>> FriendRepository::findByUserAndFriendAsync(
    const std::string& userId, const std::string& friendId) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        std::string("SELECT ") + kFriendSelectColumns +
            " FROM friends WHERE (user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)",
        userId, friendId
    );
    auto row = safeRow(result);
    if (!row) co_return std::nullopt;
    co_return rowToEntity(*row);
}

/// 查询用户的已接受好友列表（双向匹配，只返回 status='accepted'）
drogon::Task<std::vector<FriendEntity>> FriendRepository::findByUserIdAsync(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        std::string("SELECT ") + kFriendSelectColumns +
            " FROM friends WHERE (user_id = $1 OR friend_id = $1) AND status = 'accepted'",
        userId
    );
    std::vector<FriendEntity> friends;
    for (const auto& row : result) {
        friends.push_back(rowToEntity(row));
    }
    co_return friends;
}

/// 查询用户的全部好友关系（含 pending/rejected 等所有状态）
drogon::Task<std::vector<FriendEntity>> FriendRepository::findAllByUserIdAsync(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        std::string("SELECT ") + kFriendSelectColumns +
            " FROM friends WHERE user_id = $1 OR friend_id = $1",
        userId
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

/// 双向物理删除好友关系（A→B 和 B→A 同时清除）
drogon::Task<void> FriendRepository::deleteBidirectionalAsync(const std::string& userId, const std::string& friendId) {
    auto db = drogon::app().getDbClient("default");
    co_await db->execSqlCoro(
        "DELETE FROM friends WHERE (user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)",
        userId, friendId
    );
}

} // namespace heartlake::domain::friend_domain
