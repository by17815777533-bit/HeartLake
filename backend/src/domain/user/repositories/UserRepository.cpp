/**
 * @file UserRepository.cpp
 * @brief 用户仓储实现
 */

#include "domain/user/repositories/UserRepository.h"

namespace heartlake::domain::user {

std::optional<UserEntity> UserRepository::findById(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT user_id, username, nickname, email, is_anonymous, status, created_at "
        "FROM users WHERE user_id = $1 AND status = 'active'", userId
    );
    if (result.empty()) return std::nullopt;

    UserEntity entity;
    entity.userId = result[0]["user_id"].as<std::string>();
    entity.username = result[0]["username"].as<std::string>();
    entity.nickname = result[0]["nickname"].as<std::string>();
    entity.isAnonymous = result[0]["is_anonymous"].as<bool>();
    entity.status = result[0]["status"].as<std::string>();
    return entity;
}

std::optional<UserEntity> UserRepository::findByUsername(const std::string& username) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT user_id, username, nickname, email, is_anonymous, status, created_at "
        "FROM users WHERE username = $1 AND status = 'active'", username
    );
    if (result.empty()) return std::nullopt;

    UserEntity entity;
    entity.userId = result[0]["user_id"].as<std::string>();
    entity.username = result[0]["username"].as<std::string>();
    entity.nickname = result[0]["nickname"].as<std::string>();
    entity.isAnonymous = result[0]["is_anonymous"].as<bool>();
    entity.status = result[0]["status"].as<std::string>();
    return entity;
}

void UserRepository::updateLastActive(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "UPDATE users SET last_active_at = NOW() WHERE user_id = $1",
        [](const drogon::orm::Result&) {},
        [userId](const drogon::orm::DrogonDbException& e) {
            LOG_WARN << "Failed to update last_active for user " << userId << ": " << e.base().what();
        },
        userId
    );
}

// ==================== Async Methods ====================

drogon::Task<std::optional<UserEntity>> UserRepository::findByIdAsync(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        "SELECT user_id, username, nickname, email, is_anonymous, status, created_at "
        "FROM users WHERE user_id = $1 AND status = 'active'", userId
    );
    if (result.empty()) co_return std::nullopt;

    UserEntity entity;
    entity.userId = result[0]["user_id"].as<std::string>();
    entity.username = result[0]["username"].as<std::string>();
    entity.nickname = result[0]["nickname"].as<std::string>();
    entity.isAnonymous = result[0]["is_anonymous"].as<bool>();
    entity.status = result[0]["status"].as<std::string>();
    co_return entity;
}

drogon::Task<std::optional<UserEntity>> UserRepository::findByUsernameAsync(const std::string& username) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        "SELECT user_id, username, nickname, email, is_anonymous, status, created_at "
        "FROM users WHERE username = $1 AND status = 'active'", username
    );
    if (result.empty()) co_return std::nullopt;

    UserEntity entity;
    entity.userId = result[0]["user_id"].as<std::string>();
    entity.username = result[0]["username"].as<std::string>();
    entity.nickname = result[0]["nickname"].as<std::string>();
    entity.isAnonymous = result[0]["is_anonymous"].as<bool>();
    entity.status = result[0]["status"].as<std::string>();
    co_return entity;
}

} // namespace heartlake::domain::user
