/**
 * 咨询室控制器实现 - E2EE端到端加密
 */

#include "interfaces/api/ConsultationController.h"
#include "utils/E2EEncryption.h"
#include "utils/IdentityShadowMap.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <drogon/drogon.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

using namespace drogon;
using namespace heartlake::api;
using namespace heartlake::utils;

namespace {
    std::string generateSessionId() {
        unsigned char bytes[16];
        RAND_bytes(bytes, 16);
        std::ostringstream oss;
        oss << "sess_";
        for (int i = 0; i < 16; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
        return oss.str();
    }
}

void ConsultationController::createSession(const HttpRequestPtr& req,
                                           std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = Validator::getUserId(req);
    if (!userId) {
        callback(ResponseUtil::unauthorized("未授权"));
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("counselor_id")) {
        callback(ResponseUtil::badRequest("缺少咨询师ID"));
        return;
    }

    std::string counselorId = (*json)["counselor_id"].asString();
    if (counselorId.empty() || counselorId.size() > 64) {
        callback(ResponseUtil::badRequest("counselor_id无效"));
        return;
    }
    std::string sessionId = generateSessionId();
    std::string serverKey = E2EEncryption::generateKey();

    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "INSERT INTO consultation_sessions (id, user_id, counselor_id, server_key, status, created_at) "
        "VALUES ($1, $2, $3, $4, 'pending', NOW())",
        [callback, sessionId, serverKey](const orm::Result&) {
            Json::Value resp;
            resp["session_id"] = sessionId;
            resp["server_public_key"] = serverKey;
            callback(ResponseUtil::success(resp));
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "创建会话失败"));
        },
        sessionId, *userId, counselorId, serverKey
    );
}

void ConsultationController::exchangeKey(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = Validator::getUserId(req);
    if (!userId) {
        callback(ResponseUtil::unauthorized("未授权"));
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("session_id") || !json->isMember("client_public_key")) {
        callback(ResponseUtil::badRequest("参数不完整"));
        return;
    }

    std::string sessionId = (*json)["session_id"].asString();
    std::string clientKey = (*json)["client_public_key"].asString();
    if (clientKey.empty() || clientKey.size() > 4096) {
        callback(ResponseUtil::badRequest("client_public_key无效"));
        return;
    }
    std::string salt = E2EEncryption::generateKey();

    // 验证用户是会话参与者后才允许密钥交换
    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "UPDATE consultation_sessions SET client_key = $1, key_salt = $2, status = 'active' "
        "WHERE id = $3 AND (user_id = $4 OR counselor_id = $4)",
        [callback, salt](const orm::Result& r) {
            if (r.affectedRows() == 0) {
                callback(ResponseUtil::forbidden("无权操作此会话"));
                return;
            }
            Json::Value resp;
            resp["salt"] = salt;
            resp["status"] = "key_exchanged";
            callback(ResponseUtil::success(resp));
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "密钥交换失败"));
        },
        clientKey, salt, sessionId, *userId
    );
}

void ConsultationController::sendMessage(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = Validator::getUserId(req);
    if (!userId) {
        callback(ResponseUtil::unauthorized("未授权"));
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("session_id") || !json->isMember("encrypted")) {
        callback(ResponseUtil::badRequest("参数不完整"));
        return;
    }

    std::string sessionId = (*json)["session_id"].asString();
    auto& enc = (*json)["encrypted"];
    std::string ciphertext = enc["ciphertext"].asString();
    std::string iv = enc["iv"].asString();
    std::string tag = enc["tag"].asString();
    if (ciphertext.size() > 65536 || iv.size() > 32 || tag.size() > 64) {
        callback(ResponseUtil::badRequest("加密字段长度超限"));
        return;
    }

    auto shadowId = IdentityShadowMap::getInstance().getOrCreateShadowId(*userId);

    // 验证用户是会话参与者后才允许发送消息
    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "SELECT id FROM consultation_sessions WHERE id = $1 AND (user_id = $2 OR counselor_id = $2) AND status = 'active'",
        [callback, sessionId, shadowId, ciphertext, iv, tag, db](const orm::Result& r) {
            if (r.empty()) {
                callback(ResponseUtil::forbidden("无权在此会话中发送消息"));
                return;
            }
            db->execSqlAsync(
                "INSERT INTO consultation_messages (session_id, sender_shadow_id, ciphertext, iv, tag, created_at) "
                "VALUES ($1, $2, $3, $4, $5, NOW())",
                [callback](const orm::Result&) {
                    callback(ResponseUtil::success("消息已发送"));
                },
                [callback](const orm::DrogonDbException&) {
                    callback(ResponseUtil::error(500, "发送失败"));
                },
                sessionId, shadowId, ciphertext, iv, tag
            );
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "验证会话权限失败"));
        },
        sessionId, *userId
    );
}

void ConsultationController::getMessages(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& callback,
                                         const std::string& sessionId) {
    auto userId = Validator::getUserId(req);
    if (!userId) {
        callback(ResponseUtil::unauthorized("未授权"));
        return;
    }

    const std::string currentUserShadowId =
        IdentityShadowMap::getInstance().getOrCreateShadowId(*userId);

    // 先验证用户是会话参与者
    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "SELECT id FROM consultation_sessions WHERE id = $1 AND (user_id = $2 OR counselor_id = $2)",
        [callback, sessionId, db, currentUserShadowId](const orm::Result& r) {
            if (r.empty()) {
                callback(ResponseUtil::forbidden("无权访问此会话消息"));
                return;
            }
            db->execSqlAsync(
                "SELECT sender_shadow_id, ciphertext, iv, tag, created_at FROM consultation_messages "
                "WHERE session_id = $1 ORDER BY created_at ASC LIMIT 500",
                [callback, currentUserShadowId](const orm::Result& dbResult) {
                    Json::Value messages(Json::arrayValue);
                    for (const auto& row : dbResult) {
                        Json::Value msg;
                        const auto senderShadowId = row["sender_shadow_id"].as<std::string>();
                        msg["sender"] = senderShadowId;
                        msg["sender_type"] = senderShadowId == currentUserShadowId ? "user" : "counselor";
                        msg["encrypted"]["ciphertext"] = row["ciphertext"].as<std::string>();
                        msg["encrypted"]["iv"] = row["iv"].as<std::string>();
                        msg["encrypted"]["tag"] = row["tag"].as<std::string>();
                        msg["time"] = row["created_at"].as<std::string>();
                        messages.append(msg);
                    }
                    callback(ResponseUtil::success(messages));
                },
                [callback](const orm::DrogonDbException&) {
                    callback(ResponseUtil::error(500, "获取消息失败"));
                },
                sessionId
            );
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "验证会话权限失败"));
        },
        sessionId, *userId
    );
}

void ConsultationController::getSessions(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = Validator::getUserId(req);
    if (!userId) {
        callback(ResponseUtil::unauthorized("未授权"));
        return;
    }

    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "SELECT id, counselor_id, status, created_at "
        "FROM consultation_sessions WHERE user_id = $1 OR counselor_id = $1 "
        "ORDER BY created_at DESC LIMIT 50",
        [callback](const orm::Result& r) {
            Json::Value sessions(Json::arrayValue);
            for (const auto& row : r) {
                Json::Value s;
                s["session_id"] = row["id"].as<std::string>();
                s["counselor_id"] = row["counselor_id"].as<std::string>();
                s["status"] = row["status"].as<std::string>();
                s["created_at"] = row["created_at"].as<std::string>();
                sessions.append(s);
            }
            Json::Value data;
            data["sessions"] = sessions;
            data["total"] = static_cast<int>(r.size());
            callback(ResponseUtil::success(data));
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "获取会话列表失败"));
        },
        *userId
    );
}
