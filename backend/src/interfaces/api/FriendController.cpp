/**
 * @file FriendController.cpp
 * @brief 好友系统控制器 — 基于亲密分的自动关系管理
 *
 * 心湖采用"亲密分自动好友"模式取代传统的手动申请/同意流程：
 * IntimacyService 根据互动频率、涟漪共振、情绪共鸣等多维度指标
 * 实时计算双方亲密分，达到阈值（kIntimacyThreshold = 12）自动开启
 * 私聊权限。getFriends 返回按亲密分排序的好友列表（含14维评分明细），
 * removeFriend 仅标记为 hidden 而非真正删除关系。私聊消息通过
 * WebSocket 实时推送，使用协程避免回调嵌套导致的连接池竞争。
 */

#include "interfaces/api/FriendController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "infrastructure/di/ServiceLocator.h"
#include "infrastructure/services/IntimacyService.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <memory>
#include <functional>
#include <algorithm>
#include <unordered_set>

using namespace heartlake::controllers;
using namespace heartlake::utils;
static std::string intimacyLevelZh(const std::string& level) {
    if (level == "soulmate") return "灵魂同频";
    if (level == "close") return "深度共鸣";
    if (level == "warm") return "温暖连接";
    return "初识";
}

constexpr double kIntimacyThreshold = 12.0;

static bool isRemovedByCurrentUser(
    const std::string& userId,
    const std::string& friendId
) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT 1 FROM friends WHERE user_id = $1 AND friend_id = $2 AND status = 'removed' LIMIT 1",
        userId, friendId
    );
    return !result.empty();
}


// ==================== 好友请求相关 ====================

void FriendController::sendFriendRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::badRequest("请求体必须是JSON格式"));
        return;
    }

    if (!json->isMember("user_id") && !json->isMember("target_user_id")) {
        callback(ResponseUtil::badRequest("缺少 target_user_id 字段"));
        return;
    }

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    auto userId = *userIdOpt;
    const std::string targetUserId = json->get("target_user_id", json->get("user_id", "")).asString();
    if (targetUserId.empty()) {
        callback(ResponseUtil::badRequest("target_user_id 不能为空"));
        return;
    }
    if (targetUserId == userId) {
        callback(ResponseUtil::badRequest("不能对自己操作"));
        return;
    }

    try {
        auto db = drogon::app().getDbClient("default");
        auto hiddenResult = db->execSqlSync(
            "SELECT 1 FROM friends WHERE user_id = $1 AND friend_id = $2 AND status = 'removed' LIMIT 1",
            userId, targetUserId
        );
        const bool restoredHiddenFriend = !hiddenResult.empty();
        if (restoredHiddenFriend) {
            db->execSqlSync(
                "DELETE FROM friends WHERE user_id = $1 AND friend_id = $2 AND status = 'removed'",
                userId, targetUserId
            );
        }

        auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
        const double score = intimacy.getIntimacyScore(userId, targetUserId);

        Json::Value data;
        data["mode"] = "intimacy_auto";
        data["from_user_id"] = userId;
        data["to_user_id"] = targetUserId;
        data["peer_id"] = targetUserId;
        data["user_id"] = targetUserId;
        data["friend_id"] = targetUserId;
        data["friend_user_id"] = targetUserId;
        data["intimacy_score"] = score;
        data["intimacy_level"] = heartlake::infrastructure::IntimacyService::levelFromScore(score);
        data["intimacy_label"] = intimacyLevelZh(data["intimacy_level"].asString());
        data["can_chat"] = score >= kIntimacyThreshold;
        data["restored_hidden_friend"] = restoredHiddenFriend;

        callback(ResponseUtil::success(
            data,
            restoredHiddenFriend
                ? "已恢复该好友显示，关系由亲密分自动判定"
                : "已切换为亲密分自动关系，无需发送好友请求"
        ));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error in sendFriendRequest(auto mode): " << e.what();
        callback(ResponseUtil::internalError("获取亲密分失败"));
    }
}

void FriendController::acceptFriendRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& userId
) {
    auto currentUserIdOpt = Validator::getUserId(req);
    if (!currentUserIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    auto currentUserId = *currentUserIdOpt;

    try {
        auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
        const double score = intimacy.getIntimacyScore(currentUserId, userId);
        Json::Value data;
        data["mode"] = "intimacy_auto";
        data["peer_id"] = userId;
        data["user_id"] = userId;
        data["friend_id"] = userId;
        data["friend_user_id"] = userId;
        data["intimacy_score"] = score;
        data["intimacy_level"] = heartlake::infrastructure::IntimacyService::levelFromScore(score);
        data["intimacy_label"] = intimacyLevelZh(data["intimacy_level"].asString());
        data["can_chat"] = score >= kIntimacyThreshold;
        callback(ResponseUtil::success(data, "无需接受：关系由互动亲密分自动判定"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error in acceptFriendRequest(auto mode): " << e.what();
        callback(ResponseUtil::internalError("获取亲密分失败"));
    }
}

void FriendController::rejectFriendRequest(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& userId
) {
    auto currentUserIdOpt = Validator::getUserId(req);
    if (!currentUserIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    auto currentUserId = *currentUserIdOpt;

    try {
        Json::Value data;
        data["mode"] = "intimacy_auto";
        data["peer_id"] = userId;
        data["user_id"] = userId;
        data["friend_id"] = userId;
        data["friend_user_id"] = userId;
        data["note"] = "系统不再使用手动同意/拒绝流程";
        callback(ResponseUtil::success(data, "无需拒绝：系统按互动亲密分自动判断关系"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error in rejectFriendRequest(auto mode): " << e.what();
        callback(ResponseUtil::internalError("自动关系处理失败"));
    }
}

void FriendController::removeFriend(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& friendId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    auto userId = *userIdOpt;

    drogon::async_run([userId, friendId, callback]() -> drogon::Task<void> {
        try {
            auto db = drogon::app().getDbClient("default");
            // 兼容历史手动好友关系：先清理双向记录
            co_await db->execSqlCoro(
                "DELETE FROM friends WHERE (user_id = $1 AND friend_id = $2) OR (user_id = $2 AND friend_id = $1)",
                userId, friendId
            );
            // 亲密分自动好友模式下，删除操作语义为“仅自己隐藏该好友”
            co_await db->execSqlCoro(
                "INSERT INTO friends (friendship_id, user_id, friend_id, status, created_at) "
                "VALUES ($1, $2, $3, 'removed', NOW()) "
                "ON CONFLICT (user_id, friend_id) DO UPDATE SET status = 'removed', created_at = NOW()",
                utils::IdGenerator::generateUUID(), userId, friendId
            );

            Json::Value result;
            result["success"] = true;
            result["mode"] = "intimacy_auto_hidden";
            result["friend_id"] = friendId;
            result["friend_user_id"] = friendId;
            result["user_id"] = friendId;
            result["peer_id"] = friendId;
            callback(ResponseUtil::success(result, "已从好友列表移除"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Error in removeFriend: " << e.what();
            callback(ResponseUtil::internalError("删除好友失败"));
        }
    });
}

// ==================== 好友列表相关 ====================

void FriendController::getFriends(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    const auto userId = *userIdOpt;

    try {
        int limit = 80;
        if (auto p = req->getParameter("limit"); !p.empty()) {
            limit = safeInt(p, 80);
        }
        limit = std::clamp(limit, 1, 200);

        auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
        auto peers = intimacy.getTopIntimacyPeers(userId, limit, kIntimacyThreshold);
        std::unordered_set<std::string> removedPeers;
        try {
            auto db = drogon::app().getDbClient("default");
            auto removedResult = db->execSqlSync(
                "SELECT friend_id FROM friends WHERE user_id = $1 AND status = 'removed'",
                userId
            );
            for (const auto& row : removedResult) {
                removedPeers.insert(row["friend_id"].as<std::string>());
            }
        } catch (const std::exception& e) {
            LOG_WARN << "Load removed friends failed for user " << userId << ": " << e.what();
        }

        Json::Value friends(Json::arrayValue);
        for (const auto& peer : peers) {
            if (removedPeers.find(peer.userId) != removedPeers.end()) {
                continue;
            }
            Json::Value item;
            item["friendship_id"] = "intimacy_auto:" + peer.userId;
            item["friend_id"] = peer.userId;
            item["friend_user_id"] = peer.userId;
            item["user_id"] = peer.userId;
            item["username"] = peer.username;
            item["nickname"] = peer.nickname;
            item["status"] = "intimacy_auto";
            item["intimacy_score"] = peer.intimacyScore;
            item["intimacy_level"] = peer.intimacyLevel;
            item["intimacy_label"] = intimacyLevelZh(peer.intimacyLevel);
            item["interaction_count"] = peer.interactionCount;
            item["boat_comments"] = peer.boatComments;
            item["ripple_count"] = peer.rippleCount;
            item["last_interacted_at"] = peer.lastInteractedAt;
            item["ai_compatibility"] = peer.aiCompatibility;
            Json::Value breakdown;
            breakdown["interaction_strength"] = peer.interactionStrength;
            breakdown["reciprocity_score"] = peer.reciprocityScore;
            breakdown["co_ripple_score"] = peer.coRippleScore;
            breakdown["mood_resonance"] = peer.moodResonance;
            breakdown["semantic_similarity"] = peer.semanticSimilarity;
            breakdown["emotion_trend_alignment"] = peer.emotionTrendAlignment;
            breakdown["dialogue_cohesion"] = peer.dialogueCohesion;
            breakdown["response_agility"] = peer.responseAgility;
            breakdown["graph_affinity"] = peer.graphAffinity;
            breakdown["emotion_synchrony"] = peer.emotionSynchrony;
            breakdown["freshness_score"] = peer.freshnessScore;
            breakdown["temporal_diversity"] = peer.temporalDiversity;
            breakdown["anti_gaming_penalty"] = peer.antiGamingPenalty;
            breakdown["behavior_health"] = peer.behaviorHealth;
            item["score_breakdown"] = breakdown;
            item["can_chat"] = peer.canChat;
            item["ttl_seconds"] = Json::Int64(-1);
            friends.append(item);
        }

        Json::Value data;
        data["mode"] = "intimacy_auto";
        data["friends"] = friends;
        data["items"] = friends;
        data["list"] = friends;
        data["total"] = static_cast<int>(friends.size());

        callback(ResponseUtil::success(data, "好友关系已由亲密分自动生成"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getFriends(intimacy mode): " << e.what();
        callback(ResponseUtil::internalError("获取亲密关系失败"));
    }
}

void FriendController::getPendingRequests(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    const auto userId = *userIdOpt;

    Json::Value data;
    data["mode"] = "intimacy_auto";
    data["requests"] = Json::arrayValue;
    data["items"] = Json::arrayValue;
    data["list"] = Json::arrayValue;
    data["total"] = 0;
    callback(ResponseUtil::success(data, "手动好友申请已下线，关系由互动亲密分自动判定"));
}

// ==================== 好友消息相关 ====================

void FriendController::sendMessage(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& friendId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    auto userId = *userIdOpt;

    auto json = req->getJsonObject();
    if (!json || !json->isMember("content")) {
        callback(ResponseUtil::badRequest("缺少content字段"));
        return;
    }

    std::string content = (*json)["content"].asString();
    if (content.empty() || content.size() > 5000) {
        callback(ResponseUtil::badRequest("content长度必须在1-5000之间"));
        return;
    }

    // BUG-7 修复：将嵌套回调改为协程模式，避免回调嵌套导致的连接池竞争和潜在死锁
    drogon::async_run([userId, friendId, content, callback]() -> drogon::Task<void> {
        try {
            if (isRemovedByCurrentUser(userId, friendId)) {
                callback(ResponseUtil::forbidden("你已移除此好友，暂不可私聊"));
                co_return;
            }
            auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
            const double score = intimacy.getIntimacyScore(userId, friendId);
            if (score < kIntimacyThreshold) {
                callback(ResponseUtil::forbidden("亲密分不足，暂不可私聊（>=12可开启）"));
                co_return;
            }

            auto db = drogon::app().getDbClient("default");
            // 插入消息
            co_await db->execSqlCoro(
                "INSERT INTO friend_messages (sender_id, receiver_id, content, created_at) VALUES ($1, $2, $3, NOW())",
                userId, friendId, content
            );

            // 推送实时消息给接收方
            Json::Value wsMsg;
            wsMsg["type"] = "new_friend_message";
            wsMsg["sender_id"] = userId;
            wsMsg["receiver_id"] = friendId;
            wsMsg["content"] = content;
            wsMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));
            BroadcastWebSocketController::sendToUser(friendId, wsMsg);

            callback(ResponseUtil::success("消息已发送"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Error in sendMessage: " << e.what();
            callback(ResponseUtil::internalError("发送消息失败"));
        }
    });
}

void FriendController::getMessages(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    const std::string& friendId
) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    auto userId = *userIdOpt;

    // BUG-7 修复：将嵌套回调改为协程模式，避免回调嵌套导致的连接池竞争和潜在死锁
    drogon::async_run([userId, friendId, callback]() -> drogon::Task<void> {
        try {
            if (isRemovedByCurrentUser(userId, friendId)) {
                callback(ResponseUtil::forbidden("你已移除此好友，无法查看私聊"));
                co_return;
            }
            auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
            const double score = intimacy.getIntimacyScore(userId, friendId);
            if (score < kIntimacyThreshold) {
                callback(ResponseUtil::forbidden("亲密分不足，暂不可查看私聊（>=12可开启）"));
                co_return;
            }

            auto db = drogon::app().getDbClient("default");
            // SEC-04: 查询消息记录 — 添加 LIMIT 防止海量数据拖垮服务
            auto r = co_await db->execSqlCoro(
                "SELECT id, sender_id, receiver_id, content, created_at FROM friend_messages "
                "WHERE (sender_id = $1 AND receiver_id = $2) OR (sender_id = $2 AND receiver_id = $1) "
                "ORDER BY created_at ASC LIMIT 200",
                userId, friendId
            );
            Json::Value messages(Json::arrayValue);
            for (const auto& row : r) {
                Json::Value msg;
                msg["id"] = row["id"].as<int64_t>();
                msg["sender_id"] = row["sender_id"].as<std::string>();
                msg["receiver_id"] = row["receiver_id"].as<std::string>();
                msg["content"] = row["content"].as<std::string>();
                msg["created_at"] = row["created_at"].as<std::string>();
                messages.append(msg);
            }
            callback(ResponseUtil::success(messages));
        } catch (const std::exception& e) {
            LOG_ERROR << "Error in getMessages: " << e.what();
            callback(ResponseUtil::internalError("获取消息失败"));
        }
    });
}
