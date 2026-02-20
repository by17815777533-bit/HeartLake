/**
 * @file IUserRepository.h
 * @brief 用户仓储接口
 */

#pragma once

#include <string>
#include <optional>

namespace heartlake::domain::user {

struct UserEntity {
    std::string userId;
    std::string username;
    std::string nickname;
    std::string email;
    bool isAnonymous;
    std::string status;
    std::string createdAt;
};

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual std::optional<UserEntity> findById(const std::string& userId) = 0;
    virtual std::optional<UserEntity> findByUsername(const std::string& username) = 0;
    virtual void updateLastActive(const std::string& userId) = 0;
};

} // namespace heartlake::domain::user
