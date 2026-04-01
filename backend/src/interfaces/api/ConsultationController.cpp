/**
 * @file ConsultationController.cpp
 * @brief 心理咨询室控制器 — E2EE 端到端加密会话管理
 *
 * 实现用户与咨询师之间的加密通信：createSession 生成会话并分配服务端会话种子，
 * exchangeKey 稳定返回同一组协商参数，sendMessage 存储
 * AES-GCM 密文（ciphertext + iv + tag），getMessages 按时序返回分页加密消息。
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

int extractWindowTotal(const orm::Result &result,
                       const std::string &column = "total_count") {
  if (result.empty() || result[0][column].isNull()) {
    return 0;
  }
  return result[0][column].as<int>();
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

    try {
        auto db = app().getDbClient("default");
        auto result = db->execSqlSync(
            "INSERT INTO consultation_sessions (id, user_id, counselor_id, server_key, status, created_at) "
            "SELECT $1::varchar, $2::varchar, u.user_id, $3::text, 'pending', NOW() "
            "FROM users u "
            "WHERE u.user_id = $4::varchar AND u.user_id <> $2::varchar",
            sessionId, *userId, serverKey, counselorId
        );

        if (result.affectedRows() == 0) {
            callback(ResponseUtil::badRequest("咨询师不存在或无效"));
            return;
        }

        Json::Value resp;
        resp["session_id"] = sessionId;
        resp["server_public_key"] = serverKey;
        callback(ResponseUtil::success(resp));
    } catch (const orm::DrogonDbException& e) {
        LOG_ERROR << "Consultation createSession DB error: " << e.base().what();
        callback(ResponseUtil::error(500, "创建会话失败"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Consultation createSession error: " << e.what();
        callback(ResponseUtil::error(500, "创建会话失败"));
    }
}

void ConsultationController::exchangeKey(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = Validator::getUserId(req);
    if (!userId) {
        callback(ResponseUtil::unauthorized("未授权"));
        return;
    }

    auto json = req->getJsonObject();
    const std::string clientKey =
        json && json->isMember("client_public_key")
            ? (*json)["client_public_key"].asString()
            : (json && json->isMember("public_key")
                   ? (*json)["public_key"].asString()
                   : std::string{});
    if (!json || !json->isMember("session_id") || clientKey.empty()) {
        callback(ResponseUtil::badRequest("参数不完整"));
        return;
    }

    std::string sessionId = (*json)["session_id"].asString();
    if (sessionId.empty() || sessionId.size() > 64 || clientKey.empty() ||
        clientKey.size() > 4096) {
        callback(ResponseUtil::badRequest("session_id或client_public_key无效"));
        return;
    }
    std::string salt = E2EEncryption::generateKey();

    // 验证用户是会话参与者后才允许密钥交换
    auto db = app().getDbClient("default");
    db->execSqlAsync(
        "UPDATE consultation_sessions "
        "SET client_key = $1, "
        "    key_salt = COALESCE(key_salt, $2), "
        "    status = 'active' "
        "WHERE id = $3 AND (user_id = $4 OR counselor_id = $4) "
        "RETURNING server_key, key_salt",
        [callback](const orm::Result& r) {
            if (r.empty()) {
                callback(ResponseUtil::forbidden("无权操作此会话"));
                return;
            }
            Json::Value resp;
            resp["server_public_key"] = r[0]["server_key"].as<std::string>();
            resp["salt"] = r[0]["key_salt"].as<std::string>();
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
        "SELECT $1::varchar, $2::varchar, $3::text, $4::varchar, $5::varchar, NOW() "
        "FROM consultation_sessions "
        "WHERE id = $1::varchar AND status = 'active' "
        "  AND (user_id = $6::varchar OR counselor_id = $6::varchar) "
        "RETURNING id, created_at::text AS created_at",
        [callback, sessionId](const orm::Result& result) {
            if (result.affectedRows() == 0) {
                callback(ResponseUtil::forbidden("无权在此会话中发送消息"));
                return;
            }
            Json::Value data;
            if (!result.empty()) {
                data["message_id"] = result[0]["id"].as<int>();
                data["id"] = result[0]["id"].as<int>();
                data["created_at"] = result[0]["created_at"].as<std::string>();
            }
            data["session_id"] = sessionId;
            data["encrypted"] = true;
            data["status"] = "sent";
            callback(ResponseUtil::success(data, "消息已发送"));
        },
        [callback](const orm::DrogonDbException& e) {
            LOG_ERROR << "Consultation sendMessage DB error: " << e.base().what();
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
        "), total_messages AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM consultation_messages m "
        "  JOIN authorized_session auth ON TRUE "
        "  WHERE m.session_id = $1"
        ") "
        "SELECT pm.sender_shadow_id, pm.ciphertext, pm.iv, pm.tag, "
        "       pm.created_at, tm.total_count "
        "FROM authorized_session auth "
        "CROSS JOIN total_messages tm "
        "LEFT JOIN LATERAL ("
        "  SELECT m.sender_shadow_id, m.ciphertext, m.iv, m.tag, m.created_at "
        "  FROM consultation_messages m "
        "  WHERE m.session_id = $1 "
        "  ORDER BY m.created_at ASC "
        "  LIMIT $3 OFFSET $4"
        ") pm ON TRUE",
        [callback, currentUserShadowId, page, pageSize](const orm::Result& result) {
            if (result.empty()) {
                callback(ResponseUtil::forbidden("无权访问此会话消息"));
                return;
            }

            callback(ResponseUtil::success(buildMessageCollectionPayload(
                result, currentUserShadowId, extractWindowTotal(result), page,
                pageSize)));
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
        "WITH scoped_sessions AS ("
        "  SELECT cs.id, cs.user_id, cs.counselor_id, cs.status, cs.created_at, "
        "         CASE "
        "           WHEN cs.user_id = $1 THEN cs.counselor_id "
        "           ELSE cs.user_id "
        "         END AS counterpart_id, "
        "         CASE "
        "           WHEN cs.user_id = $1 "
        "             THEN COALESCE(c.nickname, c.username, cs.counselor_id) "
        "           ELSE COALESCE(u.nickname, u.username, cs.user_id) "
        "         END AS counselor_name, "
        "         CASE "
        "           WHEN cs.user_id = $1 THEN c.avatar_url "
        "           ELSE u.avatar_url "
        "         END AS counselor_avatar_url, "
        "         CASE "
        "           WHEN last_message.created_at IS NULL THEN '暂无消息' "
        "           ELSE '加密消息' "
        "         END AS last_message, "
        "         COALESCE(last_message.created_at, cs.created_at) AS updated_at "
        "  FROM consultation_sessions cs "
        "  LEFT JOIN users u ON u.user_id = cs.user_id "
        "  LEFT JOIN users c ON c.user_id = cs.counselor_id "
        "  LEFT JOIN LATERAL ("
        "    SELECT created_at "
        "    FROM consultation_messages "
        "    WHERE session_id = cs.id "
        "    ORDER BY created_at DESC "
        "    LIMIT 1"
        "  ) AS last_message ON TRUE "
        "  WHERE cs.user_id = $1 OR cs.counselor_id = $1"
        "), total_sessions AS ("
        "  SELECT COUNT(*)::INTEGER AS total_count "
        "  FROM scoped_sessions"
        ") "
        "SELECT ss.id, ss.user_id, ss.counselor_id, ss.status, ss.created_at, "
        "       ss.counterpart_id, ss.counselor_name, ss.counselor_avatar_url, "
        "       ss.last_message, ss.updated_at, "
        "       ts.total_count "
        "FROM total_sessions ts "
        "LEFT JOIN LATERAL ("
        "  SELECT id, user_id, counselor_id, status, created_at, "
        "         counterpart_id, counselor_name, counselor_avatar_url, "
        "         last_message, updated_at "
        "  FROM scoped_sessions "
        "  ORDER BY updated_at DESC LIMIT $2 OFFSET $3"
        ") ss ON TRUE",
        [callback, page, pageSize](const orm::Result& r) {
            Json::Value sessions(Json::arrayValue);
            for (const auto& row : r) {
                if (row["id"].isNull()) {
                    continue;
                }
                Json::Value s;
                s["session_id"] = row["id"].as<std::string>();
                s["user_id"] = row["user_id"].as<std::string>();
                s["counselor_id"] = row["counselor_id"].as<std::string>();
                s["counterpart_id"] = row["counterpart_id"].as<std::string>();
                s["counterpartId"] = s["counterpart_id"];
                s["participant_id"] = s["counterpart_id"];
                s["participantId"] = s["participant_id"];
                s["status"] = row["status"].as<std::string>();
                s["created_at"] = row["created_at"].as<std::string>();
                s["createdAt"] = s["created_at"];
                s["updated_at"] = row["updated_at"].as<std::string>();
                s["updatedAt"] = s["updated_at"];
                s["counselor_name"] = row["counselor_name"].as<std::string>();
                s["counselorName"] = s["counselor_name"];
                if (!row["counselor_avatar_url"].isNull()) {
                    s["counselor_avatar_url"] =
                        row["counselor_avatar_url"].as<std::string>();
                    s["counselorAvatarUrl"] = s["counselor_avatar_url"];
                }
                s["last_message"] = row["last_message"].as<std::string>();
                s["lastMessage"] = s["last_message"];
                sessions.append(s);
            }

            Json::Value data = ResponseUtil::buildCollectionPayload(
                "sessions", sessions, extractWindowTotal(r), page, pageSize);
            callback(ResponseUtil::success(data));
        },
        [callback](const orm::DrogonDbException&) {
            callback(ResponseUtil::error(500, "获取会话列表失败"));
        },
        *userId, static_cast<int64_t>(pageSize), offset
    );
}
