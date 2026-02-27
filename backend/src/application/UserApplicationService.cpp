/**
 * UserApplicationService 模块实现
 */

#include "application/UserApplicationService.h"
#include "utils/RequestHelper.h"
#include <drogon/drogon.h>

using namespace heartlake::utils;

namespace heartlake {
namespace application {

Json::Value UserApplicationService::getUserProfile(const std::string& userId) {
    // 尝试从缓存获取
    std::string cacheKey = "user:" + userId;
    if (cacheManager_) {
        auto cached = cacheManager_->getJson(cacheKey);
        if (cached) {
            return *cached;
        }
    }

    // 从数据库获取
    auto dbClient = drogon::app().getDbClient("default");

    try {
        auto result = dbClient->execSqlSync(
            "SELECT user_id, username, nickname, avatar_url, bio, gender, "
            "birthday, location, is_anonymous, created_at "
            "FROM users WHERE user_id = $1",
            userId
        );

        if (result.empty()) {
            throw std::runtime_error("用户不存在");
        }

        auto row = *safeRow(result);
        Json::Value user;
        user["user_id"] = row["user_id"].as<std::string>();
        user["username"] = row["username"].as<std::string>();
        user["nickname"] = row["nickname"].as<std::string>();

        if (!row["avatar_url"].isNull()) {
            user["avatar_url"] = row["avatar_url"].as<std::string>();
        }
        if (!row["bio"].isNull()) {
            user["bio"] = row["bio"].as<std::string>();
        }
        if (!row["gender"].isNull()) {
            user["gender"] = row["gender"].as<std::string>();
        }
        if (!row["birthday"].isNull()) {
            user["birthday"] = row["birthday"].as<std::string>();
        }
        if (!row["location"].isNull()) {
            user["location"] = row["location"].as<std::string>();
        }

        user["is_anonymous"] = row["is_anonymous"].as<bool>();
        user["created_at"] = row["created_at"].as<std::string>();

        // 缓存结果
        if (cacheManager_) {
            cacheManager_->setJson(cacheKey, user, 300);
        }

        return user;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get user profile: " << e.base().what();
        throw std::runtime_error("获取用户资料失败");
    }
}

void UserApplicationService::updateUserProfile(
    const std::string& userId,
    const Json::Value& updates
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 构建更新语句
        std::vector<std::string> setClauses;
        std::vector<std::string> params;
        int paramIndex = 1;

        if (updates.isMember("nickname")) {
            setClauses.push_back("nickname = $" + std::to_string(paramIndex++));
            params.push_back(updates["nickname"].asString());
        }
        if (updates.isMember("avatar_url")) {
            setClauses.push_back("avatar_url = $" + std::to_string(paramIndex++));
            params.push_back(updates["avatar_url"].asString());
        }
        if (updates.isMember("bio")) {
            setClauses.push_back("bio = $" + std::to_string(paramIndex++));
            params.push_back(updates["bio"].asString());
        }
        if (updates.isMember("gender")) {
            setClauses.push_back("gender = $" + std::to_string(paramIndex++));
            params.push_back(updates["gender"].asString());
        }
        if (updates.isMember("birthday")) {
            setClauses.push_back("birthday = $" + std::to_string(paramIndex++));
            params.push_back(updates["birthday"].asString());
        }
        if (updates.isMember("location")) {
            setClauses.push_back("location = $" + std::to_string(paramIndex++));
            params.push_back(updates["location"].asString());
        }

        if (setClauses.empty()) {
            return;
        }

        setClauses.push_back("updated_at = NOW()");

        std::string sql = "UPDATE users SET ";
        for (size_t i = 0; i < setClauses.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += setClauses[i];
        }
        sql += " WHERE user_id = $" + std::to_string(paramIndex);
        params.push_back(userId);

        // 执行更新
        if (params.size() == 2) {
            dbClient->execSqlSync(sql, params[0], params[1]);
        } else if (params.size() == 3) {
            dbClient->execSqlSync(sql, params[0], params[1], params[2]);
        } else if (params.size() == 4) {
            dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3]);
        } else if (params.size() == 5) {
            dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3], params[4]);
        } else if (params.size() == 6) {
            dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3], params[4], params[5]);
        } else if (params.size() == 7) {
            dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
        }

        // 使缓存失效
        if (cacheManager_) {
            cacheManager_->invalidate("user:" + userId);
        }

        LOG_INFO << "User profile updated: " << userId;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to update user profile: " << e.base().what();
        throw std::runtime_error("更新用户资料失败");
    }
}

Json::Value UserApplicationService::searchUsers(
    const std::string& keyword,
    int page,
    int pageSize
) {
    auto dbClient = drogon::app().getDbClient("default");

    try {
        int offset = (page - 1) * pageSize;
        // 转义 LIKE 特殊字符，防止通配符注入
        auto escapeLike = [](const std::string& s) {
            std::string result;
            result.reserve(s.size());
            for (char c : s) {
                if (c == '%' || c == '_' || c == '\\') {
                    result += '\\';
                }
                result += c;
            }
            return result;
        };
        std::string searchPattern = "%" + escapeLike(keyword) + "%";

        auto result = dbClient->execSqlSync(
            "SELECT user_id, username, nickname, avatar_url, bio "
            "FROM users "
            "WHERE (username LIKE $1 ESCAPE '\\' OR nickname LIKE $1 ESCAPE '\\') "
            "AND is_anonymous = false "
            "ORDER BY created_at DESC "
            "LIMIT $2 OFFSET $3",
            searchPattern, std::to_string(pageSize), std::to_string(offset)
        );

        Json::Value users(Json::arrayValue);
        for (const auto& row : result) {
            Json::Value user;
            user["user_id"] = row["user_id"].as<std::string>();
            user["username"] = row["username"].as<std::string>();
            user["nickname"] = row["nickname"].as<std::string>();

            if (!row["avatar_url"].isNull()) {
                user["avatar_url"] = row["avatar_url"].as<std::string>();
            }
            if (!row["bio"].isNull()) {
                user["bio"] = row["bio"].as<std::string>();
            }

            users.append(user);
        }

        return users;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to search users: " << e.base().what();
        throw std::runtime_error("搜索用户失败");
    }
}

Json::Value UserApplicationService::getUsersBatch(const std::vector<std::string>& userIds) {
    if (userIds.empty()) {
        return Json::Value(Json::arrayValue);
    }

    auto dbClient = drogon::app().getDbClient("default");

    try {
        // 使用参数化查询防止SQL注入
        std::string placeholders;
        for (size_t i = 0; i < userIds.size(); ++i) {
            if (i > 0) placeholders += ", ";
            placeholders += "$" + std::to_string(i + 1);
        }
        std::string sql =
            "SELECT user_id, username, nickname, avatar_url "
            "FROM users WHERE user_id IN (" + placeholders + ")";

        // 超过5个时分批查询
        if (userIds.size() > 5) {
            Json::Value allUsers(Json::arrayValue);
            for (size_t i = 0; i < userIds.size(); i += 5) {
                std::vector<std::string> batch(userIds.begin() + i,
                    userIds.begin() + std::min(i + 5, userIds.size()));
                auto batchResult = getUsersBatch(batch);
                for (const auto& u : batchResult) allUsers.append(u);
            }
            return allUsers;
        }

        // 动态执行参数化查询
        auto dbResult = [&]() -> drogon::orm::Result {
            switch (userIds.size()) {
                case 1: return dbClient->execSqlSync(sql, userIds[0]);
                case 2: return dbClient->execSqlSync(sql, userIds[0], userIds[1]);
                case 3: return dbClient->execSqlSync(sql, userIds[0], userIds[1], userIds[2]);
                case 4: return dbClient->execSqlSync(sql, userIds[0], userIds[1], userIds[2], userIds[3]);
                default: return dbClient->execSqlSync(sql, userIds[0], userIds[1], userIds[2], userIds[3], userIds[4]);
            }
        }();

        Json::Value users(Json::arrayValue);
        for (const auto& row : dbResult) {
            Json::Value user;
            user["user_id"] = row["user_id"].as<std::string>();
            user["username"] = row["username"].as<std::string>();
            user["nickname"] = row["nickname"].as<std::string>();

            if (!row["avatar_url"].isNull()) {
                user["avatar_url"] = row["avatar_url"].as<std::string>();
            }

            users.append(user);
        }

        return users;

    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Failed to get users batch: " << e.base().what();
        throw std::runtime_error("批量获取用户失败");
    }
}

} // namespace application
} // namespace heartlake
