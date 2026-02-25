/**
 * @file FriendController.cpp
 * @brief FriendController 模块实现
 * Created by 白洋
 */

#include "interfaces/api/FriendController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "application/FriendApplicationService.h"
#include "infrastructure/di/ServiceLocator.h"
#include "infrastructure/services/IntimacyService.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <memory>
#include <functional>
#include <algorithm>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace heartlake::application;

static std::shared_ptr<FriendApplicationService> getFriendService() {
    return heartlake::core::di::ServiceLocator::instance().resolve<FriendApplicationService>();
}

static std::string intimacyLevelZh(const std::string& level) {
    if (level == "soulmate") return "灵魂同频";
    if (level == "close") return "深度共鸣";
    if (level == "warm") return "温暖连接";
    return "初识";
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

    if (!json->isMember("user_id")) {
        callback(ResponseUtil::badRequest("缺少user_id字段"));
        return;
    }

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("用户未认证"));
        return;
    }
    auto userId = *userIdOpt;
    const std::string targetUserId = (*json)["user_id"].asString();
    if (targetUserId.empty()) {
        callback(ResponseUtil::badRequest("target user_id 不能为空"));
        return;
    }
    if (targetUserId == userId) {
        callback(ResponseUtil::badRequest("不能对自己操作"));
        return;
    }

    try {
        auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
        const double score = intimacy.getIntimacyScore(userId, targetUserId);

        Json::Value data;
        data["mode"] = "intimacy_auto";
        data["from_user_id"] = userId;
        data["to_user_id"] = targetUserId;
        data["intimacy_score"] = score;
        data["intimacy_level"] = heartlake::infrastructure::IntimacyService::levelFromScore(score);
        data["intimacy_label"] = intimacyLevelZh(data["intimacy_level"].asString());
        data["can_chat"] = score >= 20.0;

        callback(ResponseUtil::success(data, "已切换为亲密分自动关系，无需发送好友请求"));
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
        data["intimacy_score"] = score;
        data["intimacy_level"] = heartlake::infrastructure::IntimacyService::levelFromScore(score);
        data["intimacy_label"] = intimacyLevelZh(data["intimacy_level"].asString());
        data["can_chat"] = score >= 20.0;
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

    auto service = getFriendService();
    if (!service) {
        callback(ResponseUtil::internalError("服务未初始化"));
        return;
    }

    drogon::async_run([service, userId, friendId, callback]() -> drogon::Task<void> {
        try {
            auto result = co_await service->removeFriendAsync(userId, friendId);
            callback(ResponseUtil::success(result, "已删除好友"));
        } catch (const std::runtime_error& e) {
            LOG_ERROR << "Error in removeFriend: " << e.what();
            callback(ResponseUtil::error(400, "删除好友失败"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Unexpected error in removeFriend: " << e.what();
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
            try {
                limit = std::stoi(p);
            } catch (...) {}
        }
        limit = std::clamp(limit, 1, 200);

        auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
        auto peers = intimacy.getTopIntimacyPeers(userId, limit, 20.0);

        Json::Value friends(Json::arrayValue);
        for (const auto& peer : peers) {
            Json::Value item;
            item["friendship_id"] = "intimacy_auto:" + peer.userId;
            item["friend_id"] = peer.userId;
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
        data["total"] = static_cast<int>(peers.size());

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
            auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
            const double score = intimacy.getIntimacyScore(userId, friendId);
            if (score < 20.0) {
                callback(ResponseUtil::forbidden("亲密分不足，暂不可私聊（>=20可开启）"));
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
            auto& intimacy = heartlake::infrastructure::IntimacyService::getInstance();
            const double score = intimacy.getIntimacyScore(userId, friendId);
            if (score < 20.0) {
                callback(ResponseUtil::forbidden("亲密分不足，暂不可查看私聊（>=20可开启）"));
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
