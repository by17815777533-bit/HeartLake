/**
 * 用户仓储实现
 */

#pragma once

#include "IUserRepository.h"
#include <drogon/drogon.h>

namespace heartlake::domain::user {

class UserRepository : public IUserRepository {
public:
    // Legacy sync methods
    std::optional<UserEntity> findById(const std::string& userId) override;
    std::optional<UserEntity> findByUsername(const std::string& username) override;
    void updateLastActive(const std::string& userId) override;

    // Async methods
    drogon::Task<std::optional<UserEntity>> findByIdAsync(const std::string& userId);
    drogon::Task<std::optional<UserEntity>> findByUsernameAsync(const std::string& username);
};

} // namespace heartlake::domain::user
