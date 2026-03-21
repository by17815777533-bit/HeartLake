/**
 * @file UserApplicationService.cpp
 * @brief 用户应用服务 —— 用户资料的查询、更新、搜索、批量获取
 *
 * 缓存策略：
 *   - getUserProfile 采用 cache-aside，命中缓存直接返回，未命中查库后回写（TTL
 * 300s）
 *   - updateUserProfile 写入后主动失效缓存
 *
 * 安全措施：
 *   - searchUsers 对 ILIKE 通配符做转义，防止通配符注入
 *   - getUsersBatch 使用 PostgreSQL ANY(text[]) 批量查询，减少大批量场景的往返次数
 */

#include "application/UserApplicationService.h"
#include "utils/BusinessRules.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include <algorithm>
#include <drogon/drogon.h>
#include <unordered_map>
#include <unordered_set>

using namespace heartlake::utils;

namespace heartlake {
namespace application {

namespace {

constexpr int kUserProfileCacheTtlSeconds = 300;
constexpr int kUserSummaryCacheTtlSeconds = 300;

std::string buildUserProfileCacheKey(const std::string &userId) {
  return "user:" + userId;
}

std::string buildUserSummaryCacheKey(const std::string &userId) {
  return "user:summary:" + userId;
}

drogon::orm::Result
execSqlSyncWithStringParams(const drogon::orm::DbClientPtr &dbClient,
                            const std::string &sql,
                            const std::vector<std::string> &params) {
  switch (params.size()) {
  case 0:
    return dbClient->execSqlSync(sql);
  case 1:
    return dbClient->execSqlSync(sql, params[0]);
  case 2:
    return dbClient->execSqlSync(sql, params[0], params[1]);
  case 3:
    return dbClient->execSqlSync(sql, params[0], params[1], params[2]);
  case 4:
    return dbClient->execSqlSync(sql, params[0], params[1], params[2],
                                 params[3]);
  case 5:
    return dbClient->execSqlSync(sql, params[0], params[1], params[2],
                                 params[3], params[4]);
  case 6:
    return dbClient->execSqlSync(sql, params[0], params[1], params[2],
                                 params[3], params[4], params[5]);
  case 7:
    return dbClient->execSqlSync(sql, params[0], params[1], params[2],
                                 params[3], params[4], params[5], params[6]);
  case 8:
    return dbClient->execSqlSync(sql, params[0], params[1], params[2],
                                 params[3], params[4], params[5], params[6],
                                 params[7]);
  default:
    throw std::invalid_argument("SQL 参数数量超出支持范围");
  }
}

int extractWindowTotal(const drogon::orm::Result &result,
                       const std::string &column = "total_count") {
  return result.empty() || result[0][column].isNull()
             ? 0
             : result[0][column].as<int>();
}

template <typename F>
int resolveWindowTotalOrFallback(const drogon::orm::Result &result, int page,
                                 F &&fallbackQuery) {
  if (!result.empty() || page <= 1) {
    return extractWindowTotal(result);
  }
  return fallbackQuery();
}

void addUserAliases(Json::Value &user) {
  if (user.isMember("user_id")) {
    user["id"] = user["user_id"];
    user["userId"] = user["user_id"];
  }
  if (user.isMember("avatar_url")) {
    user["avatarUrl"] = user["avatar_url"];
  }
}

Json::Value buildBasicUserJson(const drogon::orm::Row &row, bool includeBio) {
  Json::Value user;
  user["user_id"] =
      row["user_id"].isNull() ? "" : row["user_id"].as<std::string>();
  user["username"] =
      row["username"].isNull() ? "" : row["username"].as<std::string>();

  const std::string username = user["username"].asString();
  const std::string nickname =
      row["nickname"].isNull() ? username : row["nickname"].as<std::string>();
  user["nickname"] = nickname;

  if (!row["avatar_url"].isNull()) {
    user["avatar_url"] = row["avatar_url"].as<std::string>();
  }
  if (includeBio && !row["bio"].isNull()) {
    user["bio"] = row["bio"].as<std::string>();
  }

  addUserAliases(user);
  return user;
}

Json::Value buildProfileUserJson(const drogon::orm::Row &row) {
  Json::Value user = buildBasicUserJson(row, true);

  if (!row["gender"].isNull()) {
    user["gender"] = row["gender"].as<std::string>();
  }
  if (!row["birthday"].isNull()) {
    user["birthday"] = row["birthday"].as<std::string>();
  }
  if (!row["location"].isNull()) {
    user["location"] = row["location"].as<std::string>();
  }

  user["is_anonymous"] =
      row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
  user["created_at"] =
      row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
  user["stone_count"] =
      row["stone_count"].isNull() ? 0 : row["stone_count"].as<int>();
  user["friend_count"] =
      row["friend_count"].isNull() ? 0 : row["friend_count"].as<int>();
  user["ripples_received"] =
      row["ripples_received"].isNull() ? 0 : row["ripples_received"].as<int>();
  user["boats_received"] =
      row["boats_received"].isNull() ? 0 : row["boats_received"].as<int>();
  return user;
}

Json::Value buildPaginatedUsersResponse(const Json::Value &users, int total,
                                        int page, int pageSize) {
  return ResponseUtil::buildCollectionPayload("users", users, total, page,
                                              pageSize);
}

void cacheUserSummary(const std::shared_ptr<heartlake::core::cache::CacheManager>
                          &cacheManager,
                      const Json::Value &userSummary) {
  if (!cacheManager || !userSummary.isMember("user_id")) {
    return;
  }

  const auto userId = userSummary["user_id"].asString();
  if (userId.empty()) {
    return;
  }

  cacheManager->setJson(buildUserSummaryCacheKey(userId), userSummary,
                        kUserSummaryCacheTtlSeconds);
}

} // namespace

/// 获取用户资料，优先走缓存
Json::Value UserApplicationService::getUserProfile(const std::string &userId) {
  // 尝试从缓存获取
  const std::string cacheKey = buildUserProfileCacheKey(userId);
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
        "SELECT u.user_id, u.username, u.nickname, u.avatar_url, u.bio, "
        "u.gender, "
        "       u.birthday, u.location, u.is_anonymous, u.created_at, "
        "       COALESCE(stone_stats.stone_count, 0) AS stone_count, "
        "       COALESCE(stone_stats.ripples_received, 0) AS "
        "ripples_received, "
        "       COALESCE(stone_stats.boats_received, 0) AS boats_received, "
        "       COALESCE(friend_stats.friend_count, 0) AS friend_count "
        "FROM users u "
        "LEFT JOIN LATERAL ("
        "  SELECT COUNT(*) FILTER ("
        "           WHERE s.status = 'published' AND s.deleted_at IS NULL"
        "         ) AS stone_count, "
        "         COALESCE(SUM(CASE "
        "             WHEN s.status = 'published' AND s.deleted_at IS NULL "
        "             THEN s.ripple_count ELSE 0 END), 0) AS "
        "ripples_received, "
        "         COALESCE(SUM(CASE "
        "             WHEN s.status = 'published' AND s.deleted_at IS NULL "
        "             THEN s.boat_count ELSE 0 END), 0) AS boats_received "
        "  FROM stones s "
        "  WHERE s.user_id = u.user_id "
        ") AS stone_stats ON TRUE "
        "LEFT JOIN LATERAL ("
        "  SELECT COUNT(*) AS friend_count FROM friends f "
        "  WHERE (f.user_id = u.user_id OR f.friend_id = u.user_id) "
        "    AND f.status = 'accepted'"
        ") AS friend_stats ON TRUE "
        "WHERE u.user_id = $1 AND u.status = 'active'",
        userId);

    if (result.empty()) {
      throw std::runtime_error("用户不存在");
    }

    auto row = *safeRow(result);
    Json::Value user = buildProfileUserJson(row);

    // 缓存结果
    if (cacheManager_) {
      cacheManager_->setJson(cacheKey, user, kUserProfileCacheTtlSeconds);
      cacheUserSummary(cacheManager_, buildBasicUserJson(row, false));
    }

    return user;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get user profile: " << e.base().what();
    throw std::runtime_error("获取用户资料失败");
  }
}

/// 更新用户资料：动态构建 SET 子句，只更新传入的字段
Json::Value UserApplicationService::updateUserProfile(const std::string &userId,
                                                      const Json::Value &updates) {
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
      throw std::runtime_error("没有要更新的字段");
    }

    setClauses.push_back("updated_at = NOW()");

    std::string sql = "UPDATE users SET ";
    for (size_t i = 0; i < setClauses.size(); ++i) {
      if (i > 0)
        sql += ", ";
      sql += setClauses[i];
    }
    sql += " WHERE user_id = $" + std::to_string(paramIndex);
    sql += " RETURNING user_id, username, nickname, avatar_url, bio";
    params.push_back(userId);

    auto result = execSqlSyncWithStringParams(dbClient, sql, params);
    if (result.empty()) {
      throw std::runtime_error("用户不存在");
    }

    const auto &row = result[0];
    Json::Value updatedUser = buildBasicUserJson(row, true);

    if (cacheManager_) {
      cacheManager_->invalidate(buildUserProfileCacheKey(userId));
      cacheUserSummary(cacheManager_, updatedUser);
    }

    LOG_INFO << "User profile updated: " << userId;
    return updatedUser;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to update user profile: " << e.base().what();
    throw std::runtime_error("更新用户资料失败");
  }
}

/// 搜索用户：按 username/nickname 模糊匹配，ILIKE 通配符已转义
Json::Value UserApplicationService::searchUsers(const std::string &keyword,
                                                int page, int pageSize,
                                                const std::string &excludeUserId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;
    const std::string searchPattern = "%" + escapeLike(keyword) + "%";

    auto result = [&]() -> drogon::orm::Result {
      if (!excludeUserId.empty()) {
        return dbClient->execSqlSync(
            "SELECT user_id, username, nickname, avatar_url, bio, "
            "is_anonymous, COUNT(*) OVER() AS total_count "
            "FROM users "
            "WHERE (username ILIKE $1 ESCAPE '\\' OR nickname ILIKE $1 ESCAPE "
            "'\\') "
            "AND status = 'active' "
            "AND user_id != $2 "
            "ORDER BY created_at DESC "
            "LIMIT $3 OFFSET $4",
            searchPattern, excludeUserId, static_cast<int64_t>(pageSize),
            offset);
      }

      return dbClient->execSqlSync(
          "SELECT user_id, username, nickname, avatar_url, bio, "
          "is_anonymous, COUNT(*) OVER() AS total_count "
          "FROM users "
          "WHERE (username ILIKE $1 ESCAPE '\\' OR nickname ILIKE $1 ESCAPE "
          "'\\') "
          "AND status = 'active' "
          "ORDER BY created_at DESC "
          "LIMIT $2 OFFSET $3",
          searchPattern, static_cast<int64_t>(pageSize), offset);
    }();

    Json::Value users(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value user = buildBasicUserJson(row, true);
      user["is_anonymous"] =
          row["is_anonymous"].isNull() ? true : row["is_anonymous"].as<bool>();
      users.append(user);
    }

    const int total = resolveWindowTotalOrFallback(result, page, [&]() {
      if (!excludeUserId.empty()) {
        auto countResult = dbClient->execSqlSync(
            "SELECT COUNT(*)::INTEGER AS total_count "
            "FROM users "
            "WHERE (username ILIKE $1 ESCAPE '\\' OR nickname ILIKE $1 ESCAPE "
            "'\\') "
            "AND status = 'active' "
            "AND user_id != $2",
            searchPattern, excludeUserId);
        return extractWindowTotal(countResult);
      }

      auto countResult = dbClient->execSqlSync(
          "SELECT COUNT(*)::INTEGER AS total_count "
          "FROM users "
          "WHERE (username ILIKE $1 ESCAPE '\\' OR nickname ILIKE $1 ESCAPE "
          "'\\') "
          "AND status = 'active'",
          searchPattern);
      return extractWindowTotal(countResult);
    });

    return buildPaginatedUsersResponse(users, total, page, pageSize);

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to search users: " << e.base().what();
    throw std::runtime_error("搜索用户失败");
  }
}

/// 批量获取用户基本信息，使用 ANY(text[]) 降低大批量场景的查询往返
Json::Value
UserApplicationService::getUsersBatch(const std::vector<std::string> &userIds) {
  if (userIds.empty()) {
    return Json::Value(Json::arrayValue);
  }

  auto dbClient = drogon::app().getDbClient("default");

  try {
    std::vector<std::string> uniqueUserIds;
    uniqueUserIds.reserve(userIds.size());
    std::unordered_set<std::string> seen;
    for (const auto &requestedId : userIds) {
      if (!requestedId.empty() && seen.insert(requestedId).second) {
        uniqueUserIds.push_back(requestedId);
      }
    }

    constexpr size_t kUserBatchSize = 100;
    std::unordered_map<std::string, Json::Value> userById;
    userById.reserve(uniqueUserIds.size());
    std::vector<std::string> missedUserIds;
    missedUserIds.reserve(uniqueUserIds.size());

    if (cacheManager_) {
      for (const auto &uniqueUserId : uniqueUserIds) {
        auto cached =
            cacheManager_->getJson(buildUserSummaryCacheKey(uniqueUserId));
        if (cached) {
          userById.emplace(uniqueUserId, *cached);
        } else {
          missedUserIds.push_back(uniqueUserId);
        }
      }
    } else {
      missedUserIds = uniqueUserIds;
    }

    for (size_t i = 0; i < missedUserIds.size(); i += kUserBatchSize) {
      std::vector<std::string> batch(missedUserIds.begin() + i,
                                     missedUserIds.begin() +
                                         std::min(i + kUserBatchSize,
                                                  missedUserIds.size()));

      auto dbResult = dbClient->execSqlSync(
          "SELECT user_id, username, nickname, avatar_url "
          "FROM users WHERE user_id = ANY($1::text[]) AND status = 'active'",
          toPgTextArrayLiteral(batch));
      for (const auto &row : dbResult) {
        const std::string rowUserId =
            row["user_id"].isNull() ? "" : row["user_id"].as<std::string>();
        if (!rowUserId.empty()) {
          Json::Value userSummary = buildBasicUserJson(row, false);
          userById[rowUserId] = userSummary;
          cacheUserSummary(cacheManager_, userSummary);
        }
      }
    }

    Json::Value users(Json::arrayValue);
    for (const auto &requestedId : userIds) {
      auto it = userById.find(requestedId);
      if (it != userById.end()) {
        users.append(it->second);
      }
    }

    return users;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get users batch: " << e.base().what();
    throw std::runtime_error("批量获取用户失败");
  }
}

} // namespace application
} // namespace heartlake
