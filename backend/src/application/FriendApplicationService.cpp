/**
 * FriendApplicationService 模块实现 - 异步化改造
 */

#include "application/FriendApplicationService.h"
#include "infrastructure/services/FriendshipTTLEngine.h"
#include <drogon/orm/DbClient.h>

namespace heartlake::application {

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

drogon::Task<Json::Value> FriendApplicationService::acceptFriendRequestAsync(
    [[maybe_unused]] const std::string& userId,
    const std::string& friendshipId
) {
    if (!friendService_ || !repository_) {
        Json::Value err;
        err["error"] = "Service not initialized";
        co_return err;
    }
    // Get friendship data before accepting
    auto friendships = co_await repository_->findAllByUserIdAsync(userId);
    std::string friendId;
    std::string actualFriendshipId = friendshipId;

    // 先按 friendshipId 精确匹配
    for (const auto& f : friendships) {
        if (f.friendshipId == friendshipId) {
            friendId = (f.userId == userId) ? f.friendId : f.userId;
            break;
        }
    }

    // 如果没匹配到，尝试按 from_user_id 查找 pending 请求
    if (friendId.empty()) {
        for (const auto& f : friendships) {
            if (f.userId == friendshipId && f.friendId == userId && f.status == "pending") {
                friendId = f.userId;
                actualFriendshipId = f.friendshipId;
                break;
            }
        }
    }

    if (friendId.empty()) {
        Json::Value err;
        err["error"] = "好友请求不存在";
        co_return err;
    }

    co_await friendService_->acceptFriendRequestAsync(actualFriendshipId);

    // Create friendship with 24h TTL
    auto& ttlEngine = infrastructure::FriendshipTTLEngine::getInstance();
    co_await ttlEngine.createFriendshipWithTTL(actualFriendshipId, userId, friendId, 86400);

    Json::Value result;
    result["success"] = true;
    result["friendship_id"] = actualFriendshipId;
    result["friend_id"] = friendId;

    co_return result;
}

drogon::Task<Json::Value> FriendApplicationService::rejectFriendRequestAsync(
    [[maybe_unused]] const std::string& userId,
    const std::string& friendshipId
) {
    if (!friendService_ || !repository_) {
        Json::Value err;
        err["error"] = "Service not initialized";
        co_return err;
    }

    std::string actualFriendshipId = friendshipId;

    // 先按 friendshipId 精确匹配，如果没匹配到则按 from_user_id 查找
    auto friendships = co_await repository_->findAllByUserIdAsync(userId);
    bool found = false;
    for (const auto& f : friendships) {
        if (f.friendshipId == friendshipId) {
            found = true;
            break;
        }
    }
    if (!found) {
        for (const auto& f : friendships) {
            if (f.userId == friendshipId && f.friendId == userId && f.status == "pending") {
                actualFriendshipId = f.friendshipId;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        Json::Value err;
        err["error"] = "好友请求不存在";
        co_return err;
    }

    co_await friendService_->rejectFriendRequestAsync(actualFriendshipId);

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

    Json::Value result;
    result["success"] = true;

    co_return result;
}

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
    std::vector<std::string> friendshipIds;

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

    // 批量查询发起者的用户信息
    std::map<std::string, std::pair<std::string, std::string>> userInfoMap; // userId -> {nickname, username}
    if (!pendingItems.empty()) {
        try {
            auto dbClient = drogon::app().getDbClient();
            for (const auto& [fid, fromId] : pendingItems) {
                auto rows = co_await dbClient->execSqlCoro(
                    "SELECT nickname, username FROM users WHERE user_id = $1 LIMIT 1", fromId);
                if (!rows.empty()) {
                    userInfoMap[fromId] = {
                        rows[0]["nickname"].as<std::string>(),
                        rows[0]["username"].isNull() ? "" : rows[0]["username"].as<std::string>()
                    };
                }
            }
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
            item["nickname"] = it->second.first;
            item["username"] = it->second.second;
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
