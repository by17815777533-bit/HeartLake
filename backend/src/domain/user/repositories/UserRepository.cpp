/**
 * @file UserRepository.cpp
 * @brief 用户仓储实现 —— 封装 users 表的 CRUD 操作
 *
 * 提供同步和协程两套接口：
 *   - 同步版本（findById / findByUsername）用于非协程上下文
 *   - 异步版本（findByIdAsync / findByUsernameAsync）基于 Drogon 协程，
 *     适配 Controller 层的 co_await 调用链
 *
 * 所有查询只返回 status='active' 的用户，软删除用户自动过滤。
 */

#include "domain/user/repositories/UserRepository.h"
#include "utils/RequestHelper.h"

namespace heartlake::domain::user {
using namespace heartlake::utils;

/// 按 user_id 精确查找活跃用户（同步版本）
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

/// 按 username 精确查找活跃用户（同步版本）
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

/// 异步更新用户最后活跃时间，fire-and-forget 不阻塞调用方
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

// ==================== 协程异步接口 ====================

/// findById 的协程版本，适配 Drogon co_await 调用链
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

/// findByUsername 的协程版本
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
