/**
 * TempFriendController 模块实现
 */
#include "interfaces/api/TempFriendController.h"
#include "infrastructure/services/NotificationPushService.h"
#include "utils/BusinessRules.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>
#include <trantor/utils/Logger.h>

using namespace heartlake::controllers;
using namespace heartlake::utils;
using namespace drogon;

namespace {
constexpr const char kTempFriendBaseColumns[] =
    "temp_friend_id, user1_id, user2_id, source, source_id, status, "
    "upgraded_to_friend, created_at, expires_at";
constexpr const char kTempFriendAliasedColumns[] =
    "tf.temp_friend_id, tf.user1_id, tf.user2_id, tf.source, tf.source_id, "
    "tf.status, tf.upgraded_to_friend, tf.created_at, tf.expires_at";
}

void TempFriendController::createTempFriend(const HttpRequestPtr &req,
                                           std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "无效的请求数据";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }
        
        // SEC-03: 安全提取用户ID — 防止未认证请求导致异常
        auto userIdOpt = Validator::getUserId(req);
        if (!userIdOpt) {
            Json::Value ret;
            ret["code"] = 401;
            ret["message"] = "未登录";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        auto& currentUserId = *userIdOpt;

        if (!jsonPtr->isMember("target_user_id") || (*jsonPtr)["target_user_id"].asString().empty()) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "缺少target_user_id参数";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }
        auto targetUserId = (*jsonPtr)["target_user_id"].asString();
        auto source = jsonPtr->isMember("source") ? (*jsonPtr)["source"].asString() : std::string("");
        auto sourceId = (*jsonPtr).get("source_id", "").asString();
        
        if (targetUserId.empty()) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "目标用户ID不能为空";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }

        if (targetUserId == currentUserId) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "不能添加自己为临时好友";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }
        
        // 检查是否已经是好友
        auto dbClient = app().getDbClient("default");
        auto targetResult = dbClient->execSqlSync(
            "SELECT user_id FROM users WHERE user_id = $1 LIMIT 1", targetUserId);
        if (targetResult.empty()) {
            Json::Value ret;
            ret["code"] = 404;
            ret["message"] = "目标用户不存在";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }

        auto friendCheck = dbClient->execSqlSync(
            "SELECT friendship_id FROM friends "
            "WHERE ((user_id = $1 AND friend_id = $2) "
            "OR (user_id = $2 AND friend_id = $1)) "
            "AND status = 'accepted' LIMIT 1",
            currentUserId, targetUserId);
        if (!friendCheck.empty()) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "已经是永久好友";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }
        
        // 检查是否已存在临时好友或永久好友关系
        auto checkSql = "SELECT COUNT(*) as cnt FROM temp_friends "
                       "WHERE ((user1_id = $1 AND user2_id = $2) "
                       "OR (user1_id = $2 AND user2_id = $1)) "
                       "AND status = 'active'";
        
        auto result = dbClient->execSqlSync(checkSql, currentUserId, targetUserId);
        if (safeCount(result, "cnt") > 0) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "已经是临时好友";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }
        
        // 创建临时好友关系
        auto tempFriendId = drogon::utils::getUuid();
        auto expiresAt = trantor::Date::date().after(24 * 3600); // 24小时后过期
        
        auto insertSql = "INSERT INTO temp_friends "
                        "(temp_friend_id, user1_id, user2_id, source, source_id, expires_at) "
                        "VALUES ($1, $2, $3, $4, $5, $6) "
                        "RETURNING temp_friend_id, expires_at";
        
        auto insertResult = dbClient->execSqlSync(insertSql, 
            tempFriendId, currentUserId, targetUserId, 
            source, sourceId, expiresAt.toDbStringLocal());
        
        if (insertResult.size() > 0) {
            Json::Value ret;
            ret["code"] = 0;
            ret["message"] = "临时好友创建成功";
            
            Json::Value data;
            data["temp_friend_id"] = insertResult[0]["temp_friend_id"].as<std::string>();
            data["target_user_id"] = targetUserId;
            data["expires_at"] = insertResult[0]["expires_at"].as<std::string>();
            data["source"] = source;
            ret["data"] = data;
            
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);

            // 发送临时好友通知
            auto& notificationService = heartlake::services::NotificationPushService::getInstance();
            notificationService.pushFriendRequestNotification(targetUserId, currentUserId, "");
        } else {
            Json::Value ret;
            ret["code"] = 500;
            ret["message"] = "创建临时好友失败";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        }
        
    } catch (const std::exception &e) {
        LOG_ERROR << "创建临时好友异常: " << e.what();
        Json::Value ret;
        ret["code"] = 500;
        ret["message"] = "服务器内部错误";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}

void TempFriendController::getMyTempFriends(const HttpRequestPtr &req,
                                           std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto userIdOpt2 = Validator::getUserId(req);
        if (!userIdOpt2) {
            Json::Value ret;
            ret["code"] = 401;
            ret["message"] = "未登录";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        auto& currentUserId = *userIdOpt2;
        auto dbClient = app().getDbClient("default");

        // 首先清理过期的临时好友
        auto cleanupSql = "UPDATE temp_friends SET status = 'expired' "
                         "WHERE status = 'active' AND expires_at < NOW()";
        dbClient->execSqlSync(cleanupSql);
        
        // 查询临时好友列表
        auto querySql = std::string("SELECT ") + kTempFriendAliasedColumns + ", "
                       "CASE "
                       "  WHEN tf.user1_id = $1 THEN u2.nickname "
                       "  ELSE u1.nickname "
                       "END as friend_nickname, "
                       "CASE "
                       "  WHEN tf.user1_id = $1 THEN u2.user_id "
                       "  ELSE u1.user_id "
                       "END as friend_user_id, "
                       "CASE "
                       "  WHEN tf.user1_id = $1 THEN u2.avatar_url "
                       "  ELSE u1.avatar_url "
                       "END as friend_avatar, "
                       "EXTRACT(EPOCH FROM (tf.expires_at - NOW())) as seconds_remaining "
                       "FROM temp_friends tf "
                       "LEFT JOIN users u1 ON tf.user1_id = u1.user_id "
                       "LEFT JOIN users u2 ON tf.user2_id = u2.user_id "
                       "WHERE (tf.user1_id = $1 OR tf.user2_id = $1) "
                       "AND tf.status = 'active' "
                       "ORDER BY tf.created_at DESC";
        
        auto result = dbClient->execSqlSync(querySql, currentUserId);
        
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "获取成功";
        
        Json::Value friends(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value friend_;
            friend_["temp_friend_id"] = row["temp_friend_id"].as<std::string>();
            friend_["friend_user_id"] = row["friend_user_id"].as<std::string>();
            friend_["friend_id"] = row["friend_user_id"].as<std::string>();
            friend_["friend_nickname"] = row["friend_nickname"].isNull() ? "匿名用户" : row["friend_nickname"].as<std::string>();
            friend_["friend_avatar"] = row["friend_avatar"].isNull() ? "" : row["friend_avatar"].as<std::string>();
            friend_["source"] = row["source"].as<std::string>();
            friend_["created_at"] = row["created_at"].as<std::string>();
            friend_["expires_at"] = row["expires_at"].as<std::string>();
            int secRemaining = row["seconds_remaining"].isNull()
                                   ? 0
                                   : clampRemainingSeconds(row["seconds_remaining"].as<int>());
            friend_["seconds_remaining"] = secRemaining;
            friend_["hours_remaining"] = remainingHoursFromSeconds(secRemaining);
            
            friends.append(friend_);
        }
        
        ret["data"]["items"] = friends;
        ret["data"]["friends"] = friends;
        ret["data"]["temp_friends"] = friends;
        ret["data"]["total"] = static_cast<int>(result.size());
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        
    } catch (const std::exception &e) {
        LOG_ERROR << "获取临时好友列表异常: " << e.what();
        Json::Value ret;
        ret["code"] = 500;
        ret["message"] = "服务器内部错误";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}

void TempFriendController::getTempFriendDetail(const HttpRequestPtr &req,
                                              std::function<void(const HttpResponsePtr &)> &&callback,
                                              const std::string &tempFriendId) {
    try {
        auto userIdOpt4 = Validator::getUserId(req);
        if (!userIdOpt4) {
            Json::Value ret;
            ret["code"] = 401;
            ret["message"] = "未登录";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        auto& currentUserId = *userIdOpt4;
        auto dbClient = app().getDbClient("default");

        auto querySql = std::string("SELECT ") + kTempFriendAliasedColumns + ", "
                       "u1.nickname as user1_nickname, u1.avatar_url as user1_avatar, "
                       "u2.nickname as user2_nickname, u2.avatar_url as user2_avatar, "
                       "EXTRACT(EPOCH FROM (tf.expires_at - NOW())) as seconds_remaining "
                       "FROM temp_friends tf "
                       "LEFT JOIN users u1 ON tf.user1_id = u1.user_id "
                       "LEFT JOIN users u2 ON tf.user2_id = u2.user_id "
                       "WHERE tf.temp_friend_id = $1 "
                       "AND (tf.user1_id = $2 OR tf.user2_id = $2)";
        
        auto result = dbClient->execSqlSync(querySql, tempFriendId, currentUserId);
        
        if (result.size() == 0) {
            Json::Value ret;
            ret["code"] = 404;
            ret["message"] = "临时好友不存在";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }
        
        auto row = *safeRow(result);
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "获取成功";
        
        Json::Value data;
        data["temp_friend_id"] = row["temp_friend_id"].as<std::string>();
        data["status"] = row["status"].as<std::string>();
        data["source"] = row["source"].as<std::string>();
        data["created_at"] = row["created_at"].as<std::string>();
        data["expires_at"] = row["expires_at"].as<std::string>();
        int secRemaining = row["seconds_remaining"].isNull()
                               ? 0
                               : clampRemainingSeconds(row["seconds_remaining"].as<int>());
        data["seconds_remaining"] = secRemaining;
        data["hours_remaining"] = remainingHoursFromSeconds(secRemaining);
        data["upgraded_to_friend"] = row["upgraded_to_friend"].as<bool>();
        
        // 判断对方是谁
        std::string friendUserId, friendNickname, friendAvatar;
        if (row["user1_id"].as<std::string>() == currentUserId) {
            friendUserId = row["user2_id"].as<std::string>();
            friendNickname = row["user2_nickname"].isNull() ? "匿名用户" : row["user2_nickname"].as<std::string>();
            friendAvatar = row["user2_avatar"].isNull() ? "" : row["user2_avatar"].as<std::string>();
        } else {
            friendUserId = row["user1_id"].as<std::string>();
            friendNickname = row["user1_nickname"].isNull() ? "匿名用户" : row["user1_nickname"].as<std::string>();
            friendAvatar = row["user1_avatar"].isNull() ? "" : row["user1_avatar"].as<std::string>();
        }
        
        data["friend_user_id"] = friendUserId;
        data["friend_id"] = friendUserId;
        data["friend_nickname"] = friendNickname;
        data["friend_avatar"] = friendAvatar;
        
        ret["data"] = data;
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        
    } catch (const std::exception &e) {
        LOG_ERROR << "获取临时好友详情异常: " << e.what();
        Json::Value ret;
        ret["code"] = 500;
        ret["message"] = "服务器内部错误";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}

void TempFriendController::upgradeToPermanent(const HttpRequestPtr &req,
                                             std::function<void(const HttpResponsePtr &)> &&callback,
                                             const std::string &tempFriendId) {
    try {
        auto userIdOpt3 = Validator::getUserId(req);
        if (!userIdOpt3) {
            Json::Value ret;
            ret["code"] = 401;
            ret["message"] = "未登录";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        auto& currentUserId = *userIdOpt3;
        auto dbClient = app().getDbClient("default");
        auto querySql = std::string("SELECT ") + kTempFriendBaseColumns + " FROM temp_friends "
                       "WHERE temp_friend_id = $1 "
                       "AND (user1_id = $2 OR user2_id = $2) "
                       "AND status = 'active'";
        
        auto result = dbClient->execSqlSync(querySql, tempFriendId, currentUserId);
        
        if (result.size() == 0) {
            Json::Value ret;
            ret["code"] = 404;
            ret["message"] = "临时好友不存在或已过期";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
            return;
        }
        
        auto row = *safeRow(result);
        auto user1 = row["user1_id"].as<std::string>();
        auto user2 = row["user2_id"].as<std::string>();
        
        // 开启事务
        auto trans = dbClient->newTransaction();
        
        try {
            // 创建永久好友关系
            auto friendshipId = drogon::utils::getUuid();
            auto insertSql = "INSERT INTO friends "
                           "(friendship_id, user_id, friend_id, status, created_at) "
                           "VALUES ($1, $2, $3, 'accepted', NOW())";
            
            trans->execSqlSync(insertSql, friendshipId, user1, user2);
            
            // 更新临时好友状态
            auto updateSql = "UPDATE temp_friends "
                           "SET status = 'upgraded', upgraded_to_friend = true "
                           "WHERE temp_friend_id = $1";
            
            trans->execSqlSync(updateSql, tempFriendId);
            
            Json::Value ret;
            ret["code"] = 0;
            ret["message"] = "已升级为永久好友";
            
            Json::Value data;
            data["friendship_id"] = friendshipId;
            data["temp_friend_id"] = tempFriendId;
            ret["data"] = data;
            
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);

            // 通知双方好友升级成功
            auto& notificationService = heartlake::services::NotificationPushService::getInstance();
            std::string otherUserId = (user1 == currentUserId) ? user2 : user1;
            notificationService.pushSystemNotice(otherUserId, "好友升级", "你们已成为永久好友");

        } catch (const std::exception &e) {
            LOG_ERROR << "升级好友事务失败: " << e.what();
            throw;
        }
        
    } catch (const std::exception &e) {
        LOG_ERROR << "升级为永久好友异常: " << e.what();
        Json::Value ret;
        ret["code"] = 500;
        ret["message"] = "服务器内部错误";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}

void TempFriendController::deleteTempFriend(const HttpRequestPtr &req,
                                           std::function<void(const HttpResponsePtr &)> &&callback,
                                           const std::string &tempFriendId) {
    try {
        auto userIdOpt5 = Validator::getUserId(req);
        if (!userIdOpt5) {
            Json::Value ret;
            ret["code"] = 401;
            ret["message"] = "未登录";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        auto& currentUserId = *userIdOpt5;
        auto dbClient = app().getDbClient("default");

        // 更新状态为expired而不是删除
        auto updateSql = "UPDATE temp_friends "
                        "SET status = 'expired' "
                        "WHERE temp_friend_id = $1 "
                        "AND (user1_id = $2 OR user2_id = $2) "
                        "AND status = 'active'";
        
        auto result = dbClient->execSqlSync(updateSql, tempFriendId, currentUserId);
        
        if (result.affectedRows() > 0) {
            Json::Value ret;
            ret["code"] = 0;
            ret["message"] = "临时好友已删除";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        } else {
            Json::Value ret;
            ret["code"] = 404;
            ret["message"] = "临时好友不存在";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        }
        
    } catch (const std::exception &e) {
        LOG_ERROR << "删除临时好友异常: " << e.what();
        Json::Value ret;
        ret["code"] = 500;
        ret["message"] = "服务器内部错误";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}

void TempFriendController::checkTempFriendStatus(const HttpRequestPtr &req,
                                                std::function<void(const HttpResponsePtr &)> &&callback,
                                                const std::string &targetUserId) {
    try {
        auto userIdOpt6 = Validator::getUserId(req);
        if (!userIdOpt6) {
            Json::Value ret;
            ret["code"] = 401;
            ret["message"] = "未登录";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        auto& currentUserId = *userIdOpt6;
        auto dbClient = app().getDbClient("default");

        // 检查临时好友关系
        auto querySql = std::string("SELECT ") + kTempFriendAliasedColumns + ", "
                       "EXTRACT(EPOCH FROM (tf.expires_at - NOW())) as seconds_remaining "
                       "FROM temp_friends tf "
                       "WHERE ((tf.user1_id = $1 AND tf.user2_id = $2) "
                       "OR (tf.user1_id = $2 AND tf.user2_id = $1)) "
                       "AND tf.status = 'active'";
        
        auto result = dbClient->execSqlSync(querySql, currentUserId, targetUserId);
        
        Json::Value ret;
        ret["code"] = 0;
        
        if (auto rowOpt = safeRow(result)) {
            auto row = *rowOpt;
            Json::Value data;
            data["is_temp_friend"] = true;
            data["temp_friend_id"] = row["temp_friend_id"].as<std::string>();
            data["expires_at"] = row["expires_at"].as<std::string>();
            int secRemaining = row["seconds_remaining"].isNull()
                                   ? 0
                                   : clampRemainingSeconds(row["seconds_remaining"].as<int>());
            data["seconds_remaining"] = secRemaining;
            data["hours_remaining"] = remainingHoursFromSeconds(secRemaining);
            ret["data"] = data;
        } else {
            Json::Value data;
            data["is_temp_friend"] = false;
            ret["data"] = data;
        }
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        
    } catch (const std::exception &e) {
        LOG_ERROR << "检查临时好友状态异常: " << e.what();
        Json::Value ret;
        ret["code"] = 500;
        ret["message"] = "服务器内部错误";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}
