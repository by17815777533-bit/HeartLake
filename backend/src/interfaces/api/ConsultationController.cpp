/**
 * @file ConsultationController.cpp
 * @brief 心理咨询室控制器 — E2EE 端到端加密会话管理
 *
 * 实现用户与咨询师之间的加密通信：createSession 生成会话并分配服务端密钥，
 * exchangeKey 完成 Diffie-Hellman 风格的密钥交换，sendMessage 存储
 * AES-GCM 密文（ciphertext + iv + tag），getMessages 按时序返回加密消息。
 * 所有操作均验证用户为会话参与方，使用 IdentityShadowMap 隐藏真实身份。
 * 会话 ID 由 OpenSSL RAND_bytes 生成，保证不可预测。
 */

#include "interfaces/api/ConsultationController.h"
#include "utils/E2EEncryption.h"
#include "utils/IdentityShadowMap.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <algorithm>
#include <cstdint>
#include <drogon/drogon.h>
#include <iomanip>
#include <openssl/rand.h>
#include <sstream>

using namespace drogon;
using namespace heartlake::api;
using namespace heartlake::utils;

namespace {
std::string generateSessionId() {
  unsigned char bytes[16];
  RAND_bytes(bytes, 16);
  std::ostringstream oss;
  oss << "sess_";
  for (int i = 0; i < 16; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(bytes[i]);
  }
  return oss.str();
}

bool isValidEncryptedPayload(const Json::Value &payload, std::string &ciphertext,
                             std::string &iv, std::string &tag) {
  if (!payload.isObject() || !payload.isMember("ciphertext") ||
      !payload.isMember("iv") || !payload.isMember("tag") ||
      !payload["ciphertext"].isString() || !payload["iv"].isString() ||
      !payload["tag"].isString()) {
    return false;
  }

  ciphertext = payload["ciphertext"].asString();
  iv = payload["iv"].asString();
  tag = payload["tag"].asString();
  return !ciphertext.empty() && !iv.empty() && !tag.empty() &&
         ciphertext.size() <= 65536 && iv.size() <= 32 && tag.size() <= 64;
}

int extractWindowTotal(const orm::Result &result) {
  if (result.empty() || result[0]["total_count"].isNull()) {
    return 0;
  }
  return result[0]["total_count"].as<int>();
}

Json::Value buildMessageCollectionPayload(const orm::Result &result,
                                          const std::string &currentUserShadowId,
                                          int total, int page,
                                          int pageSize) {
  Json::Value messages(Json::arrayValue);
  for (const auto &row : result) {
    if (row["sender_shadow_id"].isNull()) {
      continue;
    }

    Json::Value message;
    const auto senderShadowId = row["sender_shadow_id"].as<std::string>();
    message["sender"] = senderShadowId;
    message["sender_type"] =
        senderShadowId == currentUserShadowId ? "user" : "counselor";
    message["encrypted"]["ciphertext"] = row["ciphertext"].as<std::string>();
    message["encrypted"]["iv"] = row["iv"].as<std::string>();
    message["encrypted"]["tag"] = row["tag"].as<std::string>();
    message["time"] = row["created_at"].as<std::string>();
    messages.append(message);
  }

  return ResponseUtil::buildCollectionPayload("messages", messages, total, page,
                                              pageSize);
}
} // namespace

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
    if (counselorId.empty() || counselorId.size() > 64 || counselorId == *userId) {
        callback(ResponseUtil::badRequest("counselor_id无效"));
        return;
    }
    std::string sessionId = generateSessionId();
    std::string serverKey = E2EEncryption::generateKey();

    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "INSERT INTO consultation_sessions (id, user_id, counselor_id, server_key, status, created_at) "
        "SELECT $1, $2, u.user_id, $3, 'pending', NOW() "
        "FROM users u "
        "WHERE u.user_id = $4 AND u.user_id <> $2",
        [callback, sessionId, serverKey](const orm::Result& result) {
            if (result.affectedRows() == 0) {
                callback(ResponseUtil::badRequest("咨询师不存在或无效"));
                return;
            }
            Json::Value resp;
            resp["session_id"] = sessionId;
            resp["server_public_key"] = serverKey;
            callback(ResponseUtil::success(resp));
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "创建会话失败"));
        },
        sessionId, *userId, serverKey, counselorId
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
    if (sessionId.empty() || sessionId.size() > 64 || clientKey.empty() ||
        clientKey.size() > 4096) {
        callback(ResponseUtil::badRequest("session_id或client_public_key无效"));
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
    const auto& enc = (*json)["encrypted"];
    std::string ciphertext;
    std::string iv;
    std::string tag;
    if (sessionId.empty() || sessionId.size() > 64 ||
        !isValidEncryptedPayload(enc, ciphertext, iv, tag)) {
        callback(ResponseUtil::badRequest("加密字段无效"));
        return;
    }

    auto shadowId = IdentityShadowMap::getInstance().getOrCreateShadowId(*userId);

    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "INSERT INTO consultation_messages (session_id, sender_shadow_id, ciphertext, iv, tag, created_at) "
        "SELECT $1, $2, $3, $4, $5, NOW() "
        "FROM consultation_sessions "
        "WHERE id = $1 AND status = 'active' AND (user_id = $6 OR counselor_id = $6)",
        [callback](const orm::Result& result) {
            if (result.affectedRows() == 0) {
                callback(ResponseUtil::forbidden("无权在此会话中发送消息"));
                return;
            }
            callback(ResponseUtil::success("消息已发送"));
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "发送失败"));
        },
        sessionId, shadowId, ciphertext, iv, tag, *userId
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
    if (sessionId.empty() || sessionId.size() > 64) {
        callback(ResponseUtil::badRequest("session_id无效"));
        return;
    }

    const auto [page, pageSize] = safePagination(req);
    const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;
    const std::string currentUserShadowId =
        IdentityShadowMap::getInstance().getOrCreateShadowId(*userId);

    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "WITH authorized_session AS ("
        "  SELECT 1 AS allowed "
        "  FROM consultation_sessions "
        "  WHERE id = $1 AND (user_id = $2 OR counselor_id = $2)"
        "), paged_messages AS ("
        "  SELECT m.sender_shadow_id, m.ciphertext, m.iv, m.tag, m.created_at, "
        "         COUNT(*) OVER() AS total_count "
        "  FROM consultation_messages m "
        "  JOIN authorized_session auth ON TRUE "
        "  WHERE m.session_id = $1 "
        "  ORDER BY m.created_at ASC "
        "  LIMIT $3 OFFSET $4"
        ") "
        "SELECT pm.sender_shadow_id, pm.ciphertext, pm.iv, pm.tag, "
        "       pm.created_at, pm.total_count "
        "FROM authorized_session auth "
        "LEFT JOIN paged_messages pm ON TRUE",
        [callback, currentUserShadowId, page, pageSize, db, sessionId](
            const orm::Result& result) {
            if (result.empty()) {
                callback(ResponseUtil::forbidden("无权访问此会话消息"));
                return;
            }

            const bool hasMessages = !result[0]["sender_shadow_id"].isNull();
            if (hasMessages || page == 1) {
                const int total = hasMessages ? extractWindowTotal(result) : 0;
                callback(ResponseUtil::success(buildMessageCollectionPayload(
                    result, currentUserShadowId, total, page, pageSize)));
                return;
            }

            db->execSqlAsync(
                "SELECT COUNT(*) AS total FROM consultation_messages WHERE session_id = $1",
                [callback, currentUserShadowId, page, pageSize, result](
                    const orm::Result& countResult) {
                    callback(ResponseUtil::success(buildMessageCollectionPayload(
                        result, currentUserShadowId, safeCount(countResult), page,
                        pageSize)));
                },
                [callback](const orm::DrogonDbException&) {
                    callback(ResponseUtil::error(500, "获取消息总数失败"));
                },
                sessionId);
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "获取消息失败"));
        },
        sessionId, *userId, static_cast<int64_t>(pageSize), offset
    );
}

void ConsultationController::getSessions(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = Validator::getUserId(req);
    if (!userId) {
        callback(ResponseUtil::unauthorized("未授权"));
        return;
    }

    const auto [page, pageSize] = safePagination(req);
    const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "SELECT id, counselor_id, status, created_at, COUNT(*) OVER() AS total_count "
        "FROM consultation_sessions "
        "WHERE user_id = $1 OR counselor_id = $1 "
        "ORDER BY created_at DESC LIMIT $2 OFFSET $3",
        [callback, page, pageSize, userId, db](const orm::Result& r) {
            Json::Value sessions(Json::arrayValue);
            for (const auto& row : r) {
                Json::Value s;
                s["session_id"] = row["id"].as<std::string>();
                s["counselor_id"] = row["counselor_id"].as<std::string>();
                s["status"] = row["status"].as<std::string>();
                s["created_at"] = row["created_at"].as<std::string>();
                sessions.append(s);
            }

            if (!r.empty() || page == 1) {
                const int total = r.empty() ? 0 : extractWindowTotal(r);
                Json::Value data = ResponseUtil::buildCollectionPayload(
                    "sessions", sessions, total, page, pageSize);
                callback(ResponseUtil::success(data));
                return;
            }

            db->execSqlAsync(
                "SELECT COUNT(*) AS total "
                "FROM consultation_sessions "
                "WHERE user_id = $1 OR counselor_id = $1",
                [callback, sessions, page, pageSize](const orm::Result& countResult) {
                    Json::Value data = ResponseUtil::buildCollectionPayload(
                        "sessions", sessions, safeCount(countResult), page, pageSize);
                    callback(ResponseUtil::success(data));
                },
                [callback](const orm::DrogonDbException&) {
                    callback(ResponseUtil::error(500, "获取会话总数失败"));
                },
                *userId);
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "获取会话列表失败"));
        },
        *userId, static_cast<int64_t>(pageSize), offset
    );
}
