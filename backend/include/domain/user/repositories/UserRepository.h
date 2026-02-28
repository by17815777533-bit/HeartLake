/**
 * 用户仓储 PostgreSQL 实现
 *
 * 通过 Drogon ORM 访问 users 表，实现 IUserRepository 定义的全部持久化契约。
 * 同步方法内部走 execSqlSync，适用于非协程上下文（如 Filter、中间件）；
 * 异步方法基于 C++20 协程（drogon::Task），推荐在协程控制器中使用。
 *
 * @note 所有查询默认过滤 status != 'deleted' 的记录，
 *       被封禁用户（status='banned'）仍可查到，由上层业务决定是否放行。
 */

#pragma once

#include "IUserRepository.h"
#include <drogon/drogon.h>

namespace heartlake::domain::user {

class UserRepository : public IUserRepository {
public:
    // --- 同步接口（IUserRepository 契约） ---

    /// @brief 同步按 ID 查找用户，内部走 execSqlSync
    std::optional<UserEntity> findById(const std::string& userId) override;

    /// @brief 同步按用户名查找用户
    std::optional<UserEntity> findByUsername(const std::string& username) override;

    /// @brief 异步更新 last_active_at 字段，fire-and-forget 语义
    void updateLastActive(const std::string& userId) override;

    // --- 协程异步接口 ---

    /**
     * @brief 协程版按 ID 查找用户
     * @param userId 用户 ID
     * @return 用户实体，未找到返回 nullopt
     */
    drogon::Task<std::optional<UserEntity>> findByIdAsync(const std::string& userId);

    /**
     * @brief 协程版按用户名查找用户
     * @param username 用户名
     * @return 用户实体，未找到返回 nullopt
     */
    drogon::Task<std::optional<UserEntity>> findByUsernameAsync(const std::string& username);
};

} // namespace heartlake::domain::user
