/**
 * 用户仓储实现
 */

#include "domain/user/repositories/UserRepository.h"
#include "utils/RequestHelper.h"

namespace heartlake::domain::user {
using namespace heartlake::utils;

std::optional<UserEntity> UserRepository::findById(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT user_id, username, nickname, email, is_anonymous, status, created_at "
        "FROM users WHERE user_id = $1 AND status = 'active'", userId
    );
    auto row = safeRow(result);
    if (!row) return std::nullopt;

    UserEntity entity;
    entity.userId = (*row)["user_id"].as<std::string>();
    entity.username = (*row)["username"].as<std::string>();
    entity.nickname = (*row)["nickname"].as<std::string>();
    entity.isAnonymous = (*row)["is_anonymous"].as<bool>();
    entity.status = (*row)["status"].as<std::string>();
    return entity;
}

std::optional<UserEntity> UserRepository::findByUsername(const std::string& username) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT user_id, username, nickname, email, is_anonymous, status, created_at "
        "FROM users WHERE username = $1 AND status = 'active'", username
    );
    auto row = safeRow(result);
    if (!row) return std::nullopt;

    UserEntity entity;
    entity.userId = (*row)["user_id"].as<std::string>();
    entity.username = (*row)["username"].as<std::string>();
    entity.nickname = (*row)["nickname"].as<std::string>();
    entity.isAnonymous = (*row)["is_anonymous"].as<bool>();
    entity.status = (*row)["status"].as<std::string>();
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
    auto row = safeRow(result);
    if (!row) co_return std::nullopt;

    UserEntity entity;
    entity.userId = (*row)["user_id"].as<std::string>();
    entity.username = (*row)["username"].as<std::string>();
    entity.nickname = (*row)["nickname"].as<std::string>();
    entity.isAnonymous = (*row)["is_anonymous"].as<bool>();
    entity.status = (*row)["status"].as<std::string>();
    co_return entity;
}

drogon::Task<std::optional<UserEntity>> UserRepository::findByUsernameAsync(const std::string& username) {
    auto db = drogon::app().getDbClient("default");
    auto result = co_await db->execSqlCoro(
        "SELECT user_id, username, nickname, email, is_anonymous, status, created_at "
        "FROM users WHERE username = $1 AND status = 'active'", username
    );
    auto row = safeRow(result);
    if (!row) co_return std::nullopt;

    UserEntity entity;
    entity.userId = (*row)["user_id"].as<std::string>();
    entity.username = (*row)["username"].as<std::string>();
    entity.nickname = (*row)["nickname"].as<std::string>();
    entity.isAnonymous = (*row)["is_anonymous"].as<bool>();
    entity.status = (*row)["status"].as<std::string>();
    co_return entity;
}

} // namespace heartlake::domain::user
