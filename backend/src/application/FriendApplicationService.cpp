/**
 * @file FriendApplicationService.cpp
 * @brief 好友应用服务 —— 协调好友领域服务与基础设施完成好友业务流程
 *
 * 全部接口基于 Drogon 协程（co_await），核心流程：
 *   - 发送/接受/拒绝好友请求
 *   - 接受时自动创建 24h TTL 的临时好友关系（FriendshipTTLEngine）
 *   - 好友列表批量查询 TTL（单次网络往返，避免 N+1）
 *   - 收到的请求附带发起者昵称信息
 */

#include "application/FriendApplicationService.h"
#include "infrastructure/services/FriendshipTTLEngine.h"
#include "utils/RequestHelper.h"
#include <drogon/orm/DbClient.h>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

struct ResolvedFriendshipRequest {
    std::string friendshipId;
    std::string friendId;
};

struct FriendUserSummary {
    std::string nickname;
    std::string username;
    std::string avatarUrl;
};

std::optional<ResolvedFriendshipRequest> resolveFriendshipRequest(
    const std::vector<heartlake::domain::friend_domain::FriendEntity>& friendships,
    const std::string& userId,
    const std::string& friendshipRef
) {
    std::optional<ResolvedFriendshipRequest> fallback;

    for (const auto& friendship : friendships) {
        if (friendship.friendshipId == friendshipRef) {
            return ResolvedFriendshipRequest{
                friendship.friendshipId,
                friendship.userId == userId ? friendship.friendId : friendship.userId
            };
        }

        if (!fallback && friendship.userId == friendshipRef &&
            friendship.friendId == userId && friendship.status == "pending") {
            fallback = ResolvedFriendshipRequest{friendship.friendshipId, friendship.userId};
        }
    }

    return fallback;
}

drogon::Task<std::unordered_map<std::string, FriendUserSummary>> loadUserSummaries(
    const std::vector<std::string>& userIds
) {
    std::unordered_map<std::string, FriendUserSummary> summaries;
    if (userIds.empty()) {
        co_return summaries;
    }

    std::vector<std::string> uniqueUserIds;
    uniqueUserIds.reserve(userIds.size());
    std::unordered_set<std::string> seen;
    for (const auto& userId : userIds) {
        if (!userId.empty() && seen.insert(userId).second) {
            uniqueUserIds.push_back(userId);
        }
    }

    if (uniqueUserIds.empty()) {
        co_return summaries;
    }

    auto dbClient = drogon::app().getDbClient("default");
    auto rows = co_await dbClient->execSqlCoro(
        "SELECT user_id, nickname, username, avatar_url "
        "FROM users WHERE user_id = ANY($1::text[])",
        heartlake::utils::toPgTextArrayLiteral(uniqueUserIds));

    summaries.reserve(rows.size());
    for (const auto& row : rows) {
        const std::string rowUserId =
            row["user_id"].isNull() ? "" : row["user_id"].as<std::string>();
        if (rowUserId.empty()) {
            continue;
        }

        FriendUserSummary summary;
        summary.username =
            row["username"].isNull() ? "" : row["username"].as<std::string>();
        summary.nickname =
            row["nickname"].isNull() ? summary.username : row["nickname"].as<std::string>();
        summary.avatarUrl =
            row["avatar_url"].isNull() ? "" : row["avatar_url"].as<std::string>();
        summaries.emplace(rowUserId, std::move(summary));
    }

    co_return summaries;
}

} // namespace

namespace heartlake::application {

/// 发送好友请求，委托领域服务处理查重和持久化
drogon::Task<Json::Value> FriendApplicationService::sendFriendRequestAsync(
    const std::string& fromUserId,
    const std::string& toUserId,
    [[maybe_unused]] const std::string& message
) {
    if (!friendService_) {
        Json::Value err;
        err["error"] = "FriendService not initialized";
        co_return err;
    }
    co_await friendService_->sendFriendRequestAsync(fromUserId, toUserId);

    Json::Value result;
    result["status"] = "pending";
    result["from_user_id"] = fromUserId;
    result["to_user_id"] = toUserId;

    co_return result;
}

/**
 * 接受好友请求：
 *   1. 先按 friendshipId 精确匹配
 *   2. 匹配不到则按 from_user_id 查找 pending 请求（兼容前端传 userId 的场景）
 *   3. 调用领域服务更新状态为 accepted
 *   4. 通过 FriendshipTTLEngine 创建 24h TTL 的临时好友关系
 */
drogon::Task<Json::Value> FriendApplicationService::acceptFriendRequestAsync(
    [[maybe_unused]] const std::string& userId,
    const std::string& friendshipId
) {
    if (!friendService_ || !repository_) {
        Json::Value err;
        err["error"] = "Service not initialized";
        co_return err;
    }
    auto friendships = co_await repository_->findAllByUserIdAsync(userId);
    const auto resolved = resolveFriendshipRequest(friendships, userId, friendshipId);
    if (!resolved) {
        Json::Value err;
        err["error"] = "好友请求不存在";
        co_return err;
    }

    co_await friendService_->acceptFriendRequestAsync(resolved->friendshipId);

    // Create friendship with 24h TTL
    auto& ttlEngine = infrastructure::FriendshipTTLEngine::getInstance();
    co_await ttlEngine.createFriendshipWithTTL(
        resolved->friendshipId,
        userId,
        resolved->friendId,
        86400);

    if (cacheManager_) {
        cacheManager_->invalidate("user:" + userId);
        cacheManager_->invalidate("user:" + resolved->friendId);
    }

    Json::Value result;
    result["success"] = true;
    result["friendship_id"] = resolved->friendshipId;
    result["friend_id"] = resolved->friendId;

    co_return result;
}

/// 拒绝好友请求，查找逻辑同 accept（支持 friendshipId 和 from_user_id 两种匹配）
drogon::Task<Json::Value> FriendApplicationService::rejectFriendRequestAsync(
    [[maybe_unused]] const std::string& userId,
    const std::string& friendshipId
) {
    if (!friendService_ || !repository_) {
        Json::Value err;
        err["error"] = "Service not initialized";
        co_return err;
    }

    auto friendships = co_await repository_->findAllByUserIdAsync(userId);
    const auto resolved = resolveFriendshipRequest(friendships, userId, friendshipId);
    if (!resolved) {
        Json::Value err;
        err["error"] = "好友请求不存在";
        co_return err;
    }

    co_await friendService_->rejectFriendRequestAsync(resolved->friendshipId);

    Json::Value result;
    result["success"] = true;

    co_return result;
}

drogon::Task<Json::Value> FriendApplicationService::removeFriendAsync(
    const std::string& userId,
    const std::string& friendId
) {
    if (!friendService_) {
        Json::Value err;
        err["error"] = "FriendService not initialized";
        co_return err;
    }
    co_await friendService_->removeFriendAsync(userId, friendId);

    if (cacheManager_) {
        cacheManager_->invalidate("user:" + userId);
        cacheManager_->invalidate("user:" + friendId);
    }

    Json::Value result;
    result["success"] = true;

    co_return result;
}

/**
 * 获取好友列表，附带每个好友关系的 TTL 剩余秒数。
 * 使用 getBatchFriendshipTTL 批量查询，单次 Redis 往返避免 N+1 问题。
 */
drogon::Task<Json::Value> FriendApplicationService::getFriendsListAsync(
    const std::string& userId
) {
    if (!friendService_) {
        Json::Value err;
        err["error"] = "FriendService not initialized";
        co_return err;
    }
    auto friendships = co_await friendService_->getFriendsAsync(userId);

    Json::Value result;
    result["friends"] = Json::Value(Json::arrayValue);

    // BUG-6 修复：使用批量 TTL 查询替代 N+1 逐个查询，避免好友列表超时
    auto& ttlEngine = infrastructure::FriendshipTTLEngine::getInstance();
    std::vector<Json::Value> items;
    items.reserve(friendships.size());
    std::vector<std::string> friendshipIds;
    friendshipIds.reserve(friendships.size());

    for (const auto& f : friendships) {
        Json::Value item;
        item["friendship_id"] = f.friendshipId;
        item["friend_id"] = (f.userId == userId) ? f.friendId : f.userId;
        item["status"] = f.status;
        item["created_at"] = f.createdAt;
        items.push_back(item);
        friendshipIds.push_back(f.friendshipId);
    }

    // 批量并行查询所有好友关系的 TTL，单次网络往返
    if (!friendshipIds.empty()) {
        try {
            auto ttls = co_await ttlEngine.getBatchFriendshipTTL(friendshipIds);
            for (size_t i = 0; i < items.size() && i < ttls.size(); ++i) {
                items[i]["ttl_seconds"] = static_cast<Json::Int64>(ttls[i]);
            }
        } catch (const std::exception& e) {
            LOG_WARN << "批量获取好友 TTL 失败: " << e.what();
            // TTL 查询失败不影响好友列表返回，使用默认值 -1
            for (auto& item : items) {
                item["ttl_seconds"] = static_cast<Json::Int64>(-1);
            }
        }
    }

    for (auto& item : items) {
        result["friends"].append(item);
    }

    co_return result;
}

/// 获取收到的好友请求列表，附带发起者的昵称和用户名
drogon::Task<Json::Value> FriendApplicationService::getReceivedRequestsAsync(
    const std::string& userId
) {
    if (!repository_) {
        Json::Value err;
        err["error"] = "Repository not initialized";
        co_return err;
    }
    auto friendships = co_await repository_->findAllByUserIdAsync(userId);

    Json::Value result;
    result["requests"] = Json::Value(Json::arrayValue);

    // 收集所有待处理请求的发起者ID
    std::vector<std::pair<std::string, std::string>> pendingItems; // {friendshipId, fromUserId}
    for (const auto& f : friendships) {
        if (f.friendId == userId && f.status == "pending") {
            pendingItems.emplace_back(f.friendshipId, f.userId);
        }
    }

    std::vector<std::string> fromUserIds;
    fromUserIds.reserve(pendingItems.size());
    for (const auto& [_, fromId] : pendingItems) {
        fromUserIds.push_back(fromId);
    }

    // 批量查询发起者的用户信息
    std::unordered_map<std::string, FriendUserSummary> userInfoMap;
    if (!pendingItems.empty()) {
        try {
            userInfoMap = co_await loadUserSummaries(fromUserIds);
        } catch (const std::exception& e) {
            LOG_WARN << "查询好友请求用户信息失败: " << e.what();
        }
    }

    for (const auto& [fid, fromId] : pendingItems) {
        Json::Value item;
        item["friendship_id"] = fid;
        item["from_user_id"] = fromId;
        auto it = userInfoMap.find(fromId);
        if (it != userInfoMap.end()) {
            item["nickname"] = it->second.nickname;
            item["username"] = it->second.username;
            if (!it->second.avatarUrl.empty()) {
                item["avatar_url"] = it->second.avatarUrl;
                item["avatarUrl"] = it->second.avatarUrl;
            }
        } else {
            item["nickname"] = "未知用户";
            item["username"] = "";
        }
        item["created_at"] = ""; // friendships 中没有单独的 created_at，留空
        result["requests"].append(item);
    }

    co_return result;
}

drogon::Task<Json::Value> FriendApplicationService::getSentRequestsAsync(
    const std::string& userId
) {
    if (!repository_) {
        Json::Value err;
        err["error"] = "Repository not initialized";
        co_return err;
    }
    auto friendships = co_await repository_->findAllByUserIdAsync(userId);

    Json::Value result;
    result["requests"] = Json::Value(Json::arrayValue);

    for (const auto& f : friendships) {
        if (f.userId == userId && f.status == "pending") {
            Json::Value item;
            item["friendship_id"] = f.friendshipId;
            item["to_user_id"] = f.friendId;
            item["created_at"] = f.createdAt;
            result["requests"].append(item);
        }
    }

    co_return result;
}

} // namespace heartlake::application
