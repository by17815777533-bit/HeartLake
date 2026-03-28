/**
 * @file AdminManagementController.cpp
 * @brief 管理后台业务操作控制器 — 用户/内容/举报/敏感词/系统配置的 CRUD
 *
 * 提供管理员日常运营所需的全部管理接口：用户列表与封禁、石头与纸船管理、
 * 举报审核流转、敏感词维护、系统配置读写、广播消息下发、操作日志查询等。
 * 所有查询均使用参数化 SQL 防注入，状态值通过白名单校验。
 */
#include "interfaces/api/AdminManagementController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/BusinessRules.h"
#include "utils/AdminRealtimeNotifier.h"
#include "utils/ResponseUtil.h"
#include "utils/IdGenerator.h"
#include "utils/AdminConfigStore.h"
#include "utils/RealtimeEvent.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include "infrastructure/ai/AIService.h"
#include <drogon/orm/Exception.h>
#include <json/json.h>
#include <ctime>
#include <stdexcept>

using namespace drogon;
using namespace heartlake::utils;
using namespace heartlake::controllers;

namespace {
bool isValidUserStatus(const std::string &s) {
    return s == "active" || s == "banned" || s == "deleted";
}

bool isValidReportStatus(const std::string &s) {
    return s == "pending" || s == "handled" || s == "ignored";
}

bool isValidStoneStatus(const std::string& status) {
    return status == "published" || status == "pending" || status == "deleted";
}

bool isValidBoatStatus(const std::string& status) {
    return status == "active" || status == "pending" || status == "deleted" || status == "published";
}

double moderationRiskScoreForReason(const std::string& reason) {
    if (reason == "violence") return 0.92;
    if (reason == "harassment") return 0.84;
    if (reason == "inappropriate") return 0.76;
    if (reason == "spam") return 0.63;
    return 0.56;
}

std::string moderationRiskLevel(double score) {
    if (score >= 0.85) return "high";
    if (score >= 0.7) return "medium";
    return "low";
}

void writeOperationLog(const HttpRequestPtr& req,
                       const std::string& adminId,
                       const std::string& action,
                       const std::string& targetType = "",
                       const std::string& targetId = "",
                       const std::string& detail = "") {
    try {
        auto dbClient = app().getDbClient("default");
        const auto ipAddress = req ? req->peerAddr().toIp() : "";
        dbClient->execSqlSync(
            "INSERT INTO operation_logs (admin_id, action, target_type, target_id, detail, ip_address, created_at) "
            "VALUES ($1, $2, NULLIF($3, ''), NULLIF($4, ''), $5, NULLIF($6, ''), NOW())",
            adminId, action, targetType, targetId, detail, ipAddress
        );
    } catch (const std::exception& e) {
        LOG_WARN << "writeOperationLog failed: " << e.what();
    }
}

int toSensitiveLevel(const std::string &level) {
    if (level == "high" || level == "critical") return 3;
    if (level == "medium") return 2;
    return 1;
}

HttpResponsePtr collectionResponse(const Json::Value& items,
                                   const char* semanticKey,
                                   int total,
                                   int page,
                                   int pageSize) {
    const std::string primaryKey =
        (semanticKey != nullptr && *semanticKey != '\0') ? semanticKey : "items";
    return ResponseUtil::success(
        ResponseUtil::buildCollectionPayload(primaryKey, items, total, page, pageSize));
}

int extractWindowTotal(const drogon::orm::Result& result,
                       const std::string& column = "total_count") {
    return result.empty() || result[0][column].isNull()
        ? 0
        : result[0][column].as<int>();
}

drogon::orm::Result execSqlWithStringParams(
    const drogon::orm::DbClientPtr &dbClient,
    const std::string &sql,
    const std::vector<std::string> &params) {
    switch (params.size()) {
        case 0: return dbClient->execSqlSync(sql);
        case 1: return dbClient->execSqlSync(sql, params[0]);
        case 2: return dbClient->execSqlSync(sql, params[0], params[1]);
        case 3: return dbClient->execSqlSync(sql, params[0], params[1], params[2]);
        case 4: return dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3]);
        default: throw std::invalid_argument("SQL 参数数量超出支持范围");
    }
}

drogon::orm::Result execSqlWithStringParamsAndPagination(
    const drogon::orm::DbClientPtr &dbClient,
    const std::string &sql,
    const std::vector<std::string> &params,
    int64_t limit,
    int64_t offset) {
    switch (params.size()) {
        case 0: return dbClient->execSqlSync(sql, limit, offset);
        case 1: return dbClient->execSqlSync(sql, params[0], limit, offset);
        case 2: return dbClient->execSqlSync(sql, params[0], params[1], limit, offset);
        case 3: return dbClient->execSqlSync(sql, params[0], params[1], params[2], limit, offset);
        case 4: return dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3], limit, offset);
        default: throw std::invalid_argument("SQL 参数数量超出支持范围");
    }
}

int resolveWindowTotalOrFallbackCount(const drogon::orm::Result &result,
                                      int64_t offset) {
    const int total = extractWindowTotal(result);
    if (!result.empty() || offset <= 0) {
        return total;
    }
    throw std::out_of_range("页码超出范围");
}
}

void AdminManagementController::getUsers(const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

        const auto userId = !req->getParameter("userId").empty()
            ? req->getParameter("userId")
            : req->getParameter("user_id");
        const auto nickname = req->getParameter("nickname");
        const auto status = req->getParameter("status");
        const auto search = req->getParameter("search");

        auto dbClient = app().getDbClient("default");

        // 使用参数化查询防止SQL注入
        std::string countSql = "SELECT COUNT(*) as total FROM users u WHERE 1=1";
        std::string filteredUsersSql =
            "WITH filtered_users AS ("
            "  SELECT u.user_id, u.username, u.nickname, u.status, u.created_at, u.last_active_at, "
            "  COUNT(*) OVER() AS total_count "
            "  FROM users u "
            "  WHERE 1=1";

        std::vector<std::string> params;
        int paramIdx = 1;

        if (!userId.empty()) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            filteredUsersSql += " AND u.user_id ILIKE " + placeholder + " ESCAPE '\\'";
            countSql += " AND u.user_id ILIKE " + placeholder + " ESCAPE '\\'";
            params.push_back("%" + escapeLike(userId) + "%");
            paramIdx++;
        }
        if (!nickname.empty()) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            filteredUsersSql += " AND u.nickname ILIKE " + placeholder + " ESCAPE '\\'";
            countSql += " AND u.nickname ILIKE " + placeholder + " ESCAPE '\\'";
            params.push_back("%" + escapeLike(nickname) + "%");
            paramIdx++;
        }
        if (!status.empty() && isValidUserStatus(status)) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            filteredUsersSql += " AND u.status = " + placeholder;
            countSql += " AND u.status = " + placeholder;
            params.push_back(status);
            paramIdx++;
        }
        if (!search.empty()) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            const std::string searchCondition =
                " AND (u.username ILIKE " + placeholder +
                " ESCAPE '\\' OR u.nickname ILIKE " + placeholder +
                " ESCAPE '\\' OR u.user_id ILIKE " + placeholder +
                " ESCAPE '\\')";
            filteredUsersSql += searchCondition;
            countSql += searchCondition;
            params.push_back("%" + escapeLike(search) + "%");
            paramIdx++;
        }

        const int limitParam = paramIdx++;
        const int offsetParam = paramIdx;
        filteredUsersSql +=
            " ORDER BY u.created_at DESC LIMIT $" + std::to_string(limitParam) +
            " OFFSET $" + std::to_string(offsetParam) +
            "), filtered_user_ids AS ("
            "  SELECT fu.user_id "
            "  FROM filtered_users fu"
            "), stone_counts AS ("
            "  SELECT s.user_id, COUNT(*) AS stones_count "
            "  FROM stones s "
            "  JOIN filtered_user_ids fui ON fui.user_id = s.user_id "
            "  WHERE s.deleted_at IS NULL "
            "    AND COALESCE(s.status, 'published') <> 'deleted' "
            "  GROUP BY s.user_id"
            "), boat_counts AS ("
            "  SELECT pb.sender_id AS user_id, COUNT(*) AS boat_count "
            "  FROM paper_boats pb "
            "  JOIN filtered_user_ids fui ON fui.user_id = pb.sender_id "
            "  WHERE COALESCE(pb.status, 'active') <> 'deleted' "
            "  GROUP BY pb.sender_id"
            ") "
            "SELECT fu.user_id, fu.username, fu.nickname, fu.status, fu.created_at, fu.last_active_at, "
            "COALESCE(sc.stones_count, 0) as stones_count, "
            "COALESCE(bc.boat_count, 0) as boat_count, "
            "fu.total_count "
            "FROM filtered_users fu "
            "LEFT JOIN stone_counts sc ON sc.user_id = fu.user_id "
            "LEFT JOIN boat_counts bc ON bc.user_id = fu.user_id "
            "ORDER BY fu.created_at DESC";

        auto execWithParams = [&]() {
            switch (params.size()) {
                case 0: return dbClient->execSqlSync(filteredUsersSql, static_cast<int64_t>(pageSize), offset);
                case 1: return dbClient->execSqlSync(filteredUsersSql, params[0], static_cast<int64_t>(pageSize), offset);
                case 2: return dbClient->execSqlSync(filteredUsersSql, params[0], params[1], static_cast<int64_t>(pageSize), offset);
                case 3: return dbClient->execSqlSync(filteredUsersSql, params[0], params[1], params[2], static_cast<int64_t>(pageSize), offset);
                case 4: return dbClient->execSqlSync(filteredUsersSql, params[0], params[1], params[2], params[3], static_cast<int64_t>(pageSize), offset);
                default: throw std::invalid_argument("SQL 参数数量超出支持范围");
            }
        };
        auto result = execWithParams();
        const int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value users(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value user;
            user["user_id"] = row["user_id"].as<std::string>();
            user["username"] = row["username"].isNull() ? "" : row["username"].as<std::string>();
            user["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
            user["status"] = row["status"].isNull() ? "active" : row["status"].as<std::string>();
            user["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            user["last_active_at"] = row["last_active_at"].isNull() ? "" : row["last_active_at"].as<std::string>();
            user["stones_count"] = row["stones_count"].as<int>();
            user["boat_count"] = row["boat_count"].as<int>();
            users.append(user);
        }

        callback(collectionResponse(users, "users", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getUsers error: " << e.what();
        callback(ResponseUtil::internalError("获取用户列表失败"));
    }
}

void AdminManagementController::getUserDetail(const HttpRequestPtr &,
                                              std::function<void(const HttpResponsePtr &)> &&callback,
                                              const std::string &userId) {
    try {
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "SELECT u.user_id, u.username, u.nickname, u.status, u.created_at, u.last_active_at, "
            "COALESCE(st.stones_count, 0) as stones_count, "
            "COALESCE(pb.boat_count, 0) as boat_count "
            "FROM users u "
            "LEFT JOIN ("
            "  SELECT user_id, COUNT(*) as stones_count "
            "  FROM stones "
            "  WHERE deleted_at IS NULL AND COALESCE(status, 'published') <> 'deleted' "
            "  GROUP BY user_id"
            ") st ON st.user_id = u.user_id "
            "LEFT JOIN ("
            "  SELECT sender_id as user_id, COUNT(*) as boat_count "
            "  FROM paper_boats "
            "  WHERE COALESCE(status, 'active') <> 'deleted' "
            "  GROUP BY sender_id"
            ") pb ON pb.user_id = u.user_id "
            "WHERE u.user_id = $1",
            userId
        );
        if (result.empty()) {
            callback(ResponseUtil::notFound("用户不存在"));
            return;
        }

        auto row = *safeRow(result);
        Json::Value user;
        user["user_id"] = row["user_id"].as<std::string>();
        user["username"] = row["username"].as<std::string>();
        user["nickname"] = row["nickname"].as<std::string>();
        user["status"] = row["status"].as<std::string>();
        user["created_at"] = row["created_at"].as<std::string>();
        user["last_active_at"] = row["last_active_at"].as<std::string>();
        user["stones_count"] = row["stones_count"].as<int>();
        user["boat_count"] = row["boat_count"].as<int>();

        callback(ResponseUtil::success(user));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getUserDetail error: " << e.what();
        callback(ResponseUtil::internalError("获取用户详情失败"));
    }
}

void AdminManagementController::updateUserStatus(const HttpRequestPtr &req,
                                                 std::function<void(const HttpResponsePtr &)> &&callback,
                                                 const std::string &userId) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("status")) {
        callback(ResponseUtil::badRequest("缺少status"));
        return;
    }

    try {
        std::string status = (*json)["status"].asString();
        if (!isValidUserStatus(status)) {
            callback(ResponseUtil::badRequest("无效的状态值，允许: active, banned, deleted"));
            return;
        }
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "UPDATE users SET status = $1, updated_at = NOW() "
            "WHERE user_id = $2 RETURNING user_id",
            status, userId);
        if (result.empty()) {
            callback(ResponseUtil::notFound("用户不存在"));
            return;
        }
        writeOperationLog(req, adminId, "user_status", "user", userId, "更新状态为 " + status);
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin updateUserStatus error: " << e.what();
        callback(ResponseUtil::internalError("更新失败"));
    }
}

void AdminManagementController::banUser(const HttpRequestPtr &req,
                                        std::function<void(const HttpResponsePtr &)> &&callback,
                                        const std::string &userId) {
    try {
        auto json = req->getJsonObject();
        const auto reason = json && json->isMember("reason") ? (*json)["reason"].asString() : "";
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "UPDATE users SET status = 'banned', updated_at = NOW() "
            "WHERE user_id = $1 RETURNING user_id",
            userId);
        if (result.empty()) {
            callback(ResponseUtil::notFound("用户不存在"));
            return;
        }
        writeOperationLog(req, adminId, "ban_user", "user", userId, reason.empty() ? "封禁用户" : reason);
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin banUser error: " << e.what();
        callback(ResponseUtil::internalError("封禁失败"));
    }
}

void AdminManagementController::unbanUser(const HttpRequestPtr &req,
                                          std::function<void(const HttpResponsePtr &)> &&callback,
                                          const std::string &userId) {
    try {
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "UPDATE users SET status = 'active', updated_at = NOW() "
            "WHERE user_id = $1 RETURNING user_id",
            userId);
        if (result.empty()) {
            callback(ResponseUtil::notFound("用户不存在"));
            return;
        }
        writeOperationLog(req, adminId, "unban_user", "user", userId, "解除封禁");
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin unbanUser error: " << e.what();
        callback(ResponseUtil::internalError("解封失败"));
    }
}

void AdminManagementController::getContents(const HttpRequestPtr &req,
                                            std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;
        const auto status = req->getParameter("status");
        const auto keyword = req->getParameter("keyword");

        auto dbClient = app().getDbClient("default");
        const std::string baseFeedSql =
            "FROM ("
            "  SELECT s.stone_id AS content_id, 'stone' AS content_type, "
            "         s.content, s.mood_type, s.is_anonymous, "
            "         COALESCE(s.status, 'published') AS status, "
            "         s.created_at, COALESCE(u.nickname, '匿名') AS author_nickname, "
            "         COALESCE(s.ripple_count, 0) AS ripple_count, "
            "         COALESCE(s.boat_count, 0) AS boat_count "
            "  FROM stones s "
            "  LEFT JOIN users u ON s.user_id = u.user_id "
            "  UNION ALL "
            "  SELECT b.boat_id AS content_id, 'boat' AS content_type, "
            "         b.content, '' AS mood_type, b.is_anonymous, "
            "         CASE WHEN COALESCE(b.status, 'active') = 'active' "
            "              THEN 'published' ELSE COALESCE(b.status, 'active') END AS status, "
            "         b.created_at, COALESCE(u.nickname, '匿名') AS author_nickname, "
            "         0 AS ripple_count, 0 AS boat_count "
            "  FROM paper_boats b "
            "  LEFT JOIN users u ON b.sender_id = u.user_id"
            ") content_feed "
            "WHERE 1=1";
        std::string querySql =
            "SELECT content_id, content_type, content, mood_type, is_anonymous, "
            "status, created_at, author_nickname, ripple_count, boat_count, "
            "COUNT(*) OVER() AS total_count " + baseFeedSql;
        std::string countSql = "SELECT COUNT(*) as total " + baseFeedSql;
        std::vector<std::string> params;
        int paramIdx = 1;

        if (!status.empty() && isValidStoneStatus(status)) {
            querySql += " AND status = $" + std::to_string(paramIdx);
            countSql += " AND status = $" + std::to_string(paramIdx);
            params.push_back(status);
            ++paramIdx;
        }

        if (!keyword.empty()) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            querySql += " AND (content ILIKE " + placeholder +
                " ESCAPE '\\' OR author_nickname ILIKE " + placeholder +
                " ESCAPE '\\')";
            countSql += " AND (content ILIKE " + placeholder +
                " ESCAPE '\\' OR author_nickname ILIKE " + placeholder +
                " ESCAPE '\\')";
            params.push_back("%" + escapeLike(keyword) + "%");
            ++paramIdx;
        }

        const int limitParam = paramIdx++;
        const int offsetParam = paramIdx;
        querySql += " ORDER BY created_at DESC LIMIT $" + std::to_string(limitParam) +
            " OFFSET $" + std::to_string(offsetParam);

        auto result = execSqlWithStringParamsAndPagination(
            dbClient, querySql, params, static_cast<int64_t>(pageSize), offset);
        const int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            const auto contentType = row["content_type"].as<std::string>();
            const auto contentId = row["content_id"].isNull()
                ? ""
                : row["content_id"].as<std::string>();
            item["id"] = contentId;
            item["content_type"] = contentType;
            item["type"] = contentType;
            if (contentType == "boat") {
                item["boat_id"] = contentId;
            } else {
                item["stone_id"] = contentId;
            }
            item["content"] = row["content"].isNull() ? "" : row["content"].as<std::string>();
            item["mood_type"] = row["mood_type"].isNull() ? "" : row["mood_type"].as<std::string>();
            item["is_anonymous"] = row["is_anonymous"].isNull() ? false : row["is_anonymous"].as<bool>();
            item["status"] = row["status"].isNull() ? "published" : row["status"].as<std::string>();
            item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            item["author_nickname"] = row["author_nickname"].isNull() ? "匿名" : row["author_nickname"].as<std::string>();
            item["ripple_count"] = row["ripple_count"].isNull() ? 0 : row["ripple_count"].as<int>();
            item["boat_count"] = row["boat_count"].isNull() ? 0 : row["boat_count"].as<int>();
            list.append(item);
        }

        callback(collectionResponse(list, "contents", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getContents error: " << e.what();
        callback(ResponseUtil::internalError("获取内容列表失败"));
    }
}

void AdminManagementController::getStones(const HttpRequestPtr &req,
                                          std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;
        const auto status = req->getParameter("status");
        const auto keyword = req->getParameter("keyword");

        auto dbClient = app().getDbClient("default");
        std::string countSql = "SELECT COUNT(*) as total FROM stones s LEFT JOIN users u ON s.user_id = u.user_id WHERE 1=1";
        std::string querySql =
            "SELECT s.stone_id, s.content, s.mood_type, s.is_anonymous, "
            "s.ripple_count, s.boat_count, s.status, s.created_at, u.nickname, "
            "COUNT(*) OVER() AS total_count "
            "FROM stones s LEFT JOIN users u ON s.user_id = u.user_id WHERE 1=1";
        std::vector<std::string> params;
        int paramIdx = 1;

        if (!status.empty() && isValidStoneStatus(status)) {
            if (status == "deleted") {
                querySql += " AND (COALESCE(s.status, 'published') = 'deleted' OR s.deleted_at IS NOT NULL)";
                countSql += " AND (COALESCE(s.status, 'published') = 'deleted' OR s.deleted_at IS NOT NULL)";
            } else {
                querySql += " AND COALESCE(s.status, 'published') = $" + std::to_string(paramIdx);
                countSql += " AND COALESCE(s.status, 'published') = $" + std::to_string(paramIdx);
                params.push_back(status);
                paramIdx++;
            }
        }

        if (!keyword.empty()) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            querySql += " AND (s.content ILIKE " + placeholder + " ESCAPE '\\' OR COALESCE(u.nickname, '') ILIKE " + placeholder + " ESCAPE '\\')";
            countSql += " AND (s.content ILIKE " + placeholder + " ESCAPE '\\' OR COALESCE(u.nickname, '') ILIKE " + placeholder + " ESCAPE '\\')";
            params.push_back("%" + escapeLike(keyword) + "%");
            paramIdx++;
        }

        const int limitParam = paramIdx++;
        const int offsetParam = paramIdx;
        querySql += " ORDER BY s.created_at DESC LIMIT $" + std::to_string(limitParam) +
            " OFFSET $" + std::to_string(offsetParam);

        auto execWithParams = [&]() {
            switch (params.size()) {
                case 0: return dbClient->execSqlSync(querySql, static_cast<int64_t>(pageSize), offset);
                case 1: return dbClient->execSqlSync(querySql, params[0], static_cast<int64_t>(pageSize), offset);
                case 2: return dbClient->execSqlSync(querySql, params[0], params[1], static_cast<int64_t>(pageSize), offset);
                default: throw std::invalid_argument("SQL 参数数量超出支持范围");
            }
        };

        auto result = execWithParams();
        int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            // BUG-FIX: stone_id 列可能为 NULL（旧数据），添加空值保护
            item["stone_id"] = row["stone_id"].isNull() ? "" : row["stone_id"].as<std::string>();
            item["content"] = row["content"].as<std::string>();
            item["mood_type"] = row["mood_type"].isNull() ? "" : row["mood_type"].as<std::string>();
            item["is_anonymous"] = row["is_anonymous"].isNull() ? false : row["is_anonymous"].as<bool>();
            item["status"] = row["status"].isNull() ? "published" : row["status"].as<std::string>();
            item["ripple_count"] = row["ripple_count"].isNull() ? 0 : row["ripple_count"].as<int>();
            item["boat_count"] = row["boat_count"].isNull() ? 0 : row["boat_count"].as<int>();
            item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            item["author_nickname"] = row["nickname"].isNull() ? "匿名" : row["nickname"].as<std::string>();
            list.append(item);
        }

        callback(collectionResponse(list, "stones", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getStones error: " << e.what();
        callback(ResponseUtil::internalError("获取石头列表失败"));
    }
}

void AdminManagementController::getStoneDetail(const HttpRequestPtr &,
                                               std::function<void(const HttpResponsePtr &)> &&callback,
                                               const std::string &stoneId) {
    try {
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, s.mood_type, s.is_anonymous, "
            "s.ripple_count, s.boat_count, s.status, s.created_at, u.nickname "
            "FROM stones s LEFT JOIN users u ON s.user_id = u.user_id WHERE s.stone_id = $1",
            stoneId
        );
        if (result.empty()) {
            callback(ResponseUtil::notFound("石头不存在"));
            return;
        }
        auto row = *safeRow(result);
        Json::Value item;
        item["stone_id"] = row["stone_id"].as<std::string>();
        item["content"] = row["content"].isNull() ? "" : row["content"].as<std::string>();
        item["mood_type"] = row["mood_type"].isNull() ? "" : row["mood_type"].as<std::string>();
        item["is_anonymous"] = row["is_anonymous"].isNull() ? false : row["is_anonymous"].as<bool>();
        item["status"] = row["status"].isNull() ? "published" : row["status"].as<std::string>();
        item["ripple_count"] = row["ripple_count"].isNull() ? 0 : row["ripple_count"].as<int>();
        item["boat_count"] = row["boat_count"].isNull() ? 0 : row["boat_count"].as<int>();
        item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
        item["author_nickname"] = row["nickname"].isNull() ? "匿名" : row["nickname"].as<std::string>();
        callback(ResponseUtil::success(item));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getStoneDetail error: " << e.what();
        callback(ResponseUtil::internalError("获取详情失败"));
    }
}

void AdminManagementController::deleteStone(const HttpRequestPtr &req,
                                            std::function<void(const HttpResponsePtr &)> &&callback,
                                            const std::string &stoneId) {
    try {
        auto json = req->getJsonObject();
        const auto reason = json && json->isMember("reason") ? (*json)["reason"].asString() : "后台删除石头";
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "UPDATE stones SET status = 'deleted', deleted_at = NOW(), updated_at = NOW() "
            "WHERE stone_id = $1 RETURNING stone_id",
            stoneId);
        if (result.empty()) {
            callback(ResponseUtil::notFound("石头不存在"));
            return;
        }
        writeOperationLog(req, adminId, "delete_content", "stone", stoneId, reason);
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin deleteStone error: " << e.what();
        callback(ResponseUtil::internalError("删除失败"));
    }
}

void AdminManagementController::getBoats(const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;
        const auto status = req->getParameter("status");
        const auto keyword = req->getParameter("keyword");

        auto dbClient = app().getDbClient("default");
        std::string countSql =
            "SELECT COUNT(*) as total "
            "FROM paper_boats b LEFT JOIN users u ON b.sender_id = u.user_id WHERE 1=1";
        std::string querySql =
            "SELECT b.boat_id, b.content, b.is_anonymous, b.status, b.created_at, u.nickname, "
            "COUNT(*) OVER() AS total_count "
            "FROM paper_boats b LEFT JOIN users u ON b.sender_id = u.user_id WHERE 1=1";
        std::vector<std::string> params;
        int paramIdx = 1;

        if (!status.empty() && isValidBoatStatus(status)) {
            if (status == "published") {
                querySql += " AND COALESCE(b.status, 'active') = 'active'";
                countSql += " AND COALESCE(b.status, 'active') = 'active'";
            } else {
                querySql += " AND COALESCE(b.status, 'active') = $" + std::to_string(paramIdx);
                countSql += " AND COALESCE(b.status, 'active') = $" + std::to_string(paramIdx);
                params.push_back(status);
                paramIdx++;
            }
        }

        if (!keyword.empty()) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            querySql += " AND (b.content ILIKE " + placeholder + " ESCAPE '\\' OR COALESCE(u.nickname, '') ILIKE " + placeholder + " ESCAPE '\\')";
            countSql += " AND (b.content ILIKE " + placeholder + " ESCAPE '\\' OR COALESCE(u.nickname, '') ILIKE " + placeholder + " ESCAPE '\\')";
            params.push_back("%" + escapeLike(keyword) + "%");
            paramIdx++;
        }

        const int limitParam = paramIdx++;
        const int offsetParam = paramIdx;
        querySql += " ORDER BY b.created_at DESC LIMIT $" + std::to_string(limitParam) +
            " OFFSET $" + std::to_string(offsetParam);

        auto execWithParams = [&]() {
            switch (params.size()) {
                case 0: return dbClient->execSqlSync(querySql, static_cast<int64_t>(pageSize), offset);
                case 1: return dbClient->execSqlSync(querySql, params[0], static_cast<int64_t>(pageSize), offset);
                case 2: return dbClient->execSqlSync(querySql, params[0], params[1], static_cast<int64_t>(pageSize), offset);
                default: throw std::invalid_argument("SQL 参数数量超出支持范围");
            }
        };

        auto result = execWithParams();
        int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["boat_id"] = row["boat_id"].as<std::string>();
            item["content"] = row["content"].isNull() ? "" : row["content"].as<std::string>();
            item["is_anonymous"] = row["is_anonymous"].isNull() ? false : row["is_anonymous"].as<bool>();
            const auto rawStatus = row["status"].isNull() ? "active" : row["status"].as<std::string>();
            item["status"] = rawStatus == "active" ? "published" : rawStatus;
            item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            item["author_nickname"] = row["nickname"].isNull() ? "匿名" : row["nickname"].as<std::string>();
            list.append(item);
        }

        callback(collectionResponse(list, "boats", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getBoats error: " << e.what();
        // QUALITY-2 修复：错误时返回 500 而非伪装成功
        callback(ResponseUtil::internalError("获取纸船列表失败"));
    }
}

void AdminManagementController::deleteBoat(const HttpRequestPtr &req,
                                           std::function<void(const HttpResponsePtr &)> &&callback,
                                           const std::string &boatId) {
    try {
        auto json = req->getJsonObject();
        const auto reason = json && json->isMember("reason") ? (*json)["reason"].asString() : "后台删除纸船";
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "WITH updated_boat AS ("
            "  UPDATE paper_boats "
            "  SET status = 'deleted', deleted_at = NOW() "
            "  WHERE boat_id = $1 AND COALESCE(status, 'active') <> 'deleted' "
            "  RETURNING boat_id, stone_id"
            "), updated_stone AS ("
            "  UPDATE stones "
            "  SET boat_count = GREATEST(boat_count - 1, 0), updated_at = NOW() "
            "  WHERE stone_id IN (SELECT stone_id FROM updated_boat WHERE stone_id IS NOT NULL) "
            "  RETURNING stone_id, boat_count"
            ") "
            "SELECT ub.boat_id, ub.stone_id, COALESCE(us.boat_count, 0) AS boat_count "
            "FROM updated_boat ub "
            "LEFT JOIN updated_stone us ON us.stone_id = ub.stone_id",
            boatId);
        if (result.empty()) {
            callback(ResponseUtil::notFound("纸船不存在或已删除"));
            return;
        }
        writeOperationLog(req, adminId, "delete_content", "boat", boatId, reason);
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin deleteBoat error: " << e.what();
        callback(ResponseUtil::internalError("删除失败"));
    }
}

void AdminManagementController::getPendingModeration(const HttpRequestPtr &req,
                                                     std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        int offset = (page - 1) * pageSize;

        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "SELECT r.report_id, r.target_type, r.target_id, r.reason, r.created_at, "
            "s.content as stone_content, b.content as boat_content, "
            "COUNT(*) OVER() AS total_count "
            "FROM reports r "
            "LEFT JOIN stones s ON r.target_type = 'stone' AND r.target_id = s.stone_id "
            "LEFT JOIN paper_boats b ON r.target_type = 'boat' AND r.target_id = b.boat_id "
            "WHERE r.status = 'pending' "
            "ORDER BY r.created_at DESC LIMIT $1 OFFSET $2",
            static_cast<int64_t>(pageSize), static_cast<int64_t>(offset)
        );
        const int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["moderation_id"] = row["report_id"].as<std::string>();
            item["content_id"] = row["target_id"].as<std::string>();
            item["content_type"] = row["target_type"].as<std::string>();
            const auto stoneContent = row["stone_content"].isNull() ? "" : row["stone_content"].as<std::string>();
            const auto boatContent = row["boat_content"].isNull() ? "" : row["boat_content"].as<std::string>();
            item["content"] = !stoneContent.empty() ? stoneContent : boatContent;
            item["ai_reason"] = row["reason"].as<std::string>();
            const auto score = moderationRiskScoreForReason(row["reason"].as<std::string>());
            item["ai_score"] = score;
            item["risk_level"] = moderationRiskLevel(score);
            item["confidence"] = score;
            item["created_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        callback(collectionResponse(list, "pending", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        // QUALITY-2 修复：错误时返回 500 而非伪装成功
        LOG_ERROR << "Admin getPendingModeration error: " << e.what();
        callback(ResponseUtil::internalError("获取待审核列表失败"));
    }
}

void AdminManagementController::approveContent(const HttpRequestPtr &req,
                                               std::function<void(const HttpResponsePtr &)> &&callback,
                                               const std::string &moderationId) {
    try {
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto trans = dbClient->newTransaction();
        auto reportResult = trans->execSqlSync(
            "SELECT report_id, target_type, target_id, reason, status "
            "FROM reports WHERE report_id = $1 FOR UPDATE",
            moderationId);
        if (reportResult.empty()) {
            callback(ResponseUtil::notFound("举报不存在"));
            return;
        }

        const auto report = *safeRow(reportResult);
        const auto reportStatus = report["status"].as<std::string>();
        if (reportStatus != "pending") {
            callback(ResponseUtil::conflict("举报已处理"));
            return;
        }

        trans->execSqlSync(
            "UPDATE reports SET status = 'handled', handled_by = $1, handled_at = NOW() "
            "WHERE report_id = $2",
            adminId, moderationId);

        const std::string logId = "mod_" + IdGenerator::generateMessageId();
        trans->execSqlSync(
            "INSERT INTO moderation_logs (log_id, target_type, target_id, action, reason, operator_id, created_at) "
            "VALUES ($1, $2, $3, 'approved', $4, $5, NOW())",
            logId,
            report["target_type"].as<std::string>(),
            report["target_id"].as<std::string>(),
            report["reason"].isNull() ? "" : report["reason"].as<std::string>(),
            adminId);

        writeOperationLog(req, adminId, "approve", "report", moderationId, "审核通过");
        Json::Value wsMsg;
        wsMsg["report_id"] = moderationId;
        wsMsg["status"] = "handled";
        wsMsg["action"] = "approved";
        BroadcastWebSocketController::broadcast(
            buildRealtimeEvent("new_moderation", std::move(wsMsg)));
        heartlake::utils::broadcastAdminRealtimeStatsUpdate("report_handled");
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin approveContent error: " << e.what();
        callback(ResponseUtil::internalError("操作失败"));
    }
}

void AdminManagementController::rejectContent(const HttpRequestPtr &req,
                                              std::function<void(const HttpResponsePtr &)> &&callback,
                                              const std::string &moderationId) {
    auto json = req->getJsonObject();
    std::string reason = json && json->isMember("reason") ? (*json)["reason"].asString() : "";

    try {
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto trans = dbClient->newTransaction();
        auto reportResult = trans->execSqlSync(
            "SELECT report_id, target_type, target_id, status "
            "FROM reports WHERE report_id = $1 FOR UPDATE",
            moderationId);
        if (reportResult.empty()) {
            callback(ResponseUtil::notFound("举报不存在"));
            return;
        }

        const auto report = *safeRow(reportResult);
        const auto reportStatus = report["status"].as<std::string>();
        if (reportStatus != "pending") {
            callback(ResponseUtil::conflict("举报已处理"));
            return;
        }

        trans->execSqlSync(
            "UPDATE reports SET status = 'ignored', handled_by = $1, handled_at = NOW() "
            "WHERE report_id = $2",
            adminId, moderationId);

        const std::string logId = "mod_" + IdGenerator::generateMessageId();
        const auto moderationReason = reason.empty() ? "审核拒绝" : reason;
        trans->execSqlSync(
            "INSERT INTO moderation_logs (log_id, target_type, target_id, action, reason, operator_id, created_at) "
            "VALUES ($1, $2, $3, 'rejected', $4, $5, NOW())",
            logId,
            report["target_type"].as<std::string>(),
            report["target_id"].as<std::string>(),
            moderationReason,
            adminId);

        writeOperationLog(req, adminId, "reject", "report", moderationId, moderationReason);
        Json::Value wsMsg;
        wsMsg["report_id"] = moderationId;
        wsMsg["status"] = "ignored";
        wsMsg["action"] = "rejected";
        BroadcastWebSocketController::broadcast(
            buildRealtimeEvent("new_moderation", std::move(wsMsg)));
        heartlake::utils::broadcastAdminRealtimeStatsUpdate("report_ignored");
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin rejectContent error: " << e.what();
        callback(ResponseUtil::internalError("操作失败"));
    }
}

void AdminManagementController::getModerationHistory(const HttpRequestPtr &req,
                                                     std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;
        const auto resultFilter = req->getParameter("result");

        auto dbClient = app().getDbClient("default");
        std::string countSql = "SELECT COUNT(*) as total FROM moderation_logs ml WHERE 1=1";
        std::string querySql =
            "SELECT ml.log_id, ml.action, ml.reason, ml.operator_id, ml.created_at, ml.target_type, ml.target_id, "
            "COALESCE(s.content, b.content, u.nickname, ml.target_id) as target_preview, "
            "COUNT(*) OVER() AS total_count "
            "FROM moderation_logs ml "
            "LEFT JOIN stones s ON ml.target_type = 'stone' AND ml.target_id = s.stone_id "
            "LEFT JOIN paper_boats b ON ml.target_type = 'boat' AND ml.target_id = b.boat_id "
            "LEFT JOIN users u ON ml.target_type = 'user' AND ml.target_id = u.user_id "
            "WHERE 1=1";
        std::vector<std::string> params;

        if ((resultFilter == "approved" || resultFilter == "rejected")) {
            countSql += " AND ml.action = $1";
            querySql += " AND ml.action = $1";
            params.push_back(resultFilter);
        }

        querySql += " ORDER BY ml.created_at DESC LIMIT $" +
            std::to_string(params.size() + 1) +
            " OFFSET $" + std::to_string(params.size() + 2);

        auto result = execSqlWithStringParamsAndPagination(
            dbClient, querySql, params, static_cast<int64_t>(pageSize), offset);
        const int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["moderation_id"] = row["log_id"].as<std::string>();
            item["content_type"] = row["target_type"].as<std::string>();
            item["content"] = row["target_preview"].isNull() ? row["target_id"].as<std::string>() : row["target_preview"].as<std::string>();
            item["result"] = row["action"].as<std::string>();
            item["reason"] = row["reason"].isNull() ? "" : row["reason"].as<std::string>();
            item["moderator"] = row["operator_id"].as<std::string>();
            item["moderated_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        callback(collectionResponse(list, "history", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        // QUALITY-2 修复：错误时返回 500 而非伪装成功
        LOG_ERROR << "Admin getModerationHistory error: " << e.what();
        callback(ResponseUtil::internalError("获取审核历史失败"));
    }
}

void AdminManagementController::getReports(const HttpRequestPtr &req,
                                           std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

        const auto status = req->getParameter("status");
        const auto reason = req->getParameter("type");

        auto dbClient = app().getDbClient("default");
        std::string countSql = "SELECT COUNT(*) as total FROM reports r WHERE 1=1";
        std::string querySql =
            "SELECT r.report_id, r.reporter_id, r.target_type, r.target_id, r.reason, r.description, r.status, r.created_at, "
            "COALESCE(s.content, '') as stone_content, COALESCE(b.content, '') as boat_content, COALESCE(ut.nickname, '') as user_target_nickname, "
            "COUNT(*) OVER() AS total_count "
            "FROM reports r "
            "LEFT JOIN stones s ON r.target_type = 'stone' AND r.target_id = s.stone_id "
            "LEFT JOIN paper_boats b ON r.target_type = 'boat' AND r.target_id = b.boat_id "
            "LEFT JOIN users ut ON r.target_type = 'user' AND r.target_id = ut.user_id "
            "WHERE 1=1";
        std::vector<std::string> params;
        int paramIdx = 1;

        if (!status.empty() && isValidReportStatus(status)) {
            countSql += " AND r.status = $" + std::to_string(paramIdx);
            querySql += " AND r.status = $" + std::to_string(paramIdx);
            params.push_back(status);
            paramIdx++;
        }
        if (!reason.empty()) {
            countSql += " AND r.reason = $" + std::to_string(paramIdx);
            querySql += " AND r.reason = $" + std::to_string(paramIdx);
            params.push_back(reason);
            paramIdx++;
        }

        const int limitParam = paramIdx++;
        const int offsetParam = paramIdx;
        querySql += " ORDER BY r.created_at DESC LIMIT $" + std::to_string(limitParam) +
                    " OFFSET $" + std::to_string(offsetParam);

        auto result = execSqlWithStringParamsAndPagination(
            dbClient, querySql, params, static_cast<int64_t>(pageSize), offset);
        const int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["id"] = row["report_id"].as<std::string>();
            item["reporter_id"] = row["reporter_id"].isNull() ? "" : row["reporter_id"].as<std::string>();
            item["target_type"] = row["target_type"].as<std::string>();
            item["target_id"] = row["target_id"].as<std::string>();
            item["type"] = row["reason"].as<std::string>();
            item["reason"] = row["description"].isNull() ? row["reason"].as<std::string>() : row["description"].as<std::string>();
            const auto stoneContent = row["stone_content"].isNull() ? "" : row["stone_content"].as<std::string>();
            const auto boatContent = row["boat_content"].isNull() ? "" : row["boat_content"].as<std::string>();
            const auto userTargetNickname = row["user_target_nickname"].isNull() ? "" : row["user_target_nickname"].as<std::string>();
            item["target_content"] = !stoneContent.empty() ? stoneContent : (!boatContent.empty() ? boatContent : userTargetNickname);
            item["status"] = row["status"].as<std::string>();
            item["created_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        callback(collectionResponse(list, "reports", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        // QUALITY-2 修复：错误时返回 500 而非伪装成功
        LOG_ERROR << "Admin getReports error: " << e.what();
        callback(ResponseUtil::internalError("获取举报列表失败"));
    }
}

void AdminManagementController::getReportDetail(const HttpRequestPtr &,
                                                std::function<void(const HttpResponsePtr &)> &&callback,
                                                const std::string &reportId) {
    try {
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "SELECT report_id, reporter_id, target_type, target_id, reason, description, status, created_at FROM reports WHERE report_id = $1",
            reportId
        );
        if (result.empty()) {
            callback(ResponseUtil::notFound("举报不存在"));
            return;
        }
        auto row = *safeRow(result);
        Json::Value item;
        item["report_id"] = row["report_id"].as<std::string>();
        item["reporter_id"] = row["reporter_id"].as<std::string>();
        item["target_type"] = row["target_type"].as<std::string>();
        item["target_id"] = row["target_id"].as<std::string>();
        item["reason"] = row["reason"].as<std::string>();
        item["description"] = row["description"].as<std::string>();
        item["status"] = row["status"].as<std::string>();
        item["created_at"] = row["created_at"].as<std::string>();
        callback(ResponseUtil::success(item));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getReportDetail error: " << e.what();
        callback(ResponseUtil::internalError("获取详情失败"));
    }
}

void AdminManagementController::handleReport(const HttpRequestPtr &req,
                                             std::function<void(const HttpResponsePtr &)> &&callback,
                                             const std::string &reportId) {
    auto json = req->getJsonObject();
    std::string action = json && json->isMember("action") ? (*json)["action"].asString() : "handled";

    // 验证action值，防止任意状态写入
    if (!isValidReportStatus(action)) {
        callback(ResponseUtil::badRequest("无效的操作，允许: pending, handled, ignored"));
        return;
    }

    try {
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "UPDATE reports SET status = $1, handled_by = $2, handled_at = NOW() "
            "WHERE report_id = $3 RETURNING report_id",
            action, adminId, reportId);
        if (result.empty()) {
            callback(ResponseUtil::notFound("举报不存在"));
            return;
        }
        const auto note = json && json->isMember("note") ? (*json)["note"].asString() : "";
        writeOperationLog(req, adminId, "handle_report", "report", reportId, note.empty() ? action : note);
        Json::Value wsMsg;
        wsMsg["report_id"] = reportId;
        wsMsg["status"] = action;
        wsMsg["action"] = action;
        BroadcastWebSocketController::broadcast(
            buildRealtimeEvent("new_moderation", std::move(wsMsg)));
        heartlake::utils::broadcastAdminRealtimeStatsUpdate("report_status_changed");
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin handleReport error: " << e.what();
        callback(ResponseUtil::internalError("处理失败"));
    }
}

void AdminManagementController::getSensitiveWords(const HttpRequestPtr &req,
                                                  std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;
        const auto keyword = req->getParameter("keyword");
        const auto level = req->getParameter("level");

        auto dbClient = app().getDbClient("default");
        std::string whereClause = " WHERE 1=1";
        std::vector<std::string> params;

        if (!keyword.empty()) {
            whereClause += " AND word ILIKE $1 ESCAPE '\\'";
            params.push_back("%" + escapeLike(keyword) + "%");
        }

        if (!level.empty()) {
            const auto index = std::to_string(params.size() + 1);
            if (level == "low") {
                whereClause += " AND level <= $" + index;
                params.push_back("1");
            } else if (level == "medium") {
                whereClause += " AND level = $" + index;
                params.push_back("2");
            } else if (level == "high" || level == "critical") {
                whereClause += " AND level >= $" + index;
                params.push_back("3");
            }
        }

        const std::string querySql =
            "SELECT id, word, category, "
            "CASE "
            "  WHEN level >= 3 THEN 'high' "
            "  WHEN level = 2 THEN 'medium' "
            "  ELSE 'low' "
            "END as level, "
            "CASE WHEN is_active THEN 'block' ELSE 'allow' END as action, "
            "created_at, "
            "COUNT(*) OVER() AS total_count "
            "FROM sensitive_words" + whereClause +
            " ORDER BY created_at DESC LIMIT $" + std::to_string(params.size() + 1) +
            " OFFSET $" + std::to_string(params.size() + 2);
        const std::string countSql =
            "SELECT COUNT(*) as total FROM sensitive_words" + whereClause;

        auto result = execSqlWithStringParamsAndPagination(
            dbClient, querySql, params, static_cast<int64_t>(pageSize), offset);
        const int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value words(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["id"] = row["id"].as<int>();
            item["word"] = row["word"].as<std::string>();
            item["level"] = row["level"].as<std::string>();
            item["category"] = row["category"].as<std::string>();
            item["action"] = row["action"].as<std::string>();
            item["created_at"] = row["created_at"].as<std::string>();
            words.append(item);
        }

        callback(collectionResponse(words, "words", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        // QUALITY-2 修复：错误时返回 500 而非伪装成功
        LOG_ERROR << "Admin getSensitiveWords error: " << e.what();
        callback(ResponseUtil::internalError("获取敏感词列表失败"));
    }
}

void AdminManagementController::addSensitiveWord(const HttpRequestPtr &req,
                                                 std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("word")) {
        callback(ResponseUtil::badRequest("缺少敏感词"));
        return;
    }

    try {
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync(
            "INSERT INTO sensitive_words (word, level, category, is_active, created_at) VALUES ($1, $2, $3, $4, NOW())",
            (*json)["word"].asString(),
            toSensitiveLevel((*json).get("level", "medium").asString()),
            (*json).get("category", "general").asString(),
            (*json).get("action", "block").asString() != "allow"
        );
        writeOperationLog(req, adminId, "sensitive_add", "sensitive_word", "", (*json)["word"].asString());
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin addSensitiveWord error: " << e.what();
        callback(ResponseUtil::internalError("添加失败"));
    }
}

void AdminManagementController::updateSensitiveWord(const HttpRequestPtr &req,
                                                    std::function<void(const HttpResponsePtr &)> &&callback,
                                                    const std::string &id) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("word")) {
        callback(ResponseUtil::badRequest("缺少敏感词"));
        return;
    }

    try {
        int wordId = std::stoi(id);
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "UPDATE sensitive_words SET word = $1, level = $2, category = $3, is_active = $4 "
            "WHERE id = $5 RETURNING id",
            (*json)["word"].asString(),
            toSensitiveLevel((*json).get("level", "medium").asString()),
            (*json).get("category", "general").asString(),
            (*json).get("action", "block").asString() != "allow",
            wordId
        );
        if (result.empty()) {
            callback(ResponseUtil::notFound("敏感词不存在"));
            return;
        }
        writeOperationLog(req, adminId, "sensitive_update", "sensitive_word", id, (*json)["word"].asString());
        callback(ResponseUtil::success());
    } catch (const std::invalid_argument &) {
        callback(ResponseUtil::badRequest("无效的ID格式"));
    } catch (const std::out_of_range &) {
        callback(ResponseUtil::badRequest("ID超出范围"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin updateSensitiveWord error: " << e.what();
        callback(ResponseUtil::internalError("更新失败"));
    }
}

void AdminManagementController::deleteSensitiveWord(const HttpRequestPtr &req,
                                                    std::function<void(const HttpResponsePtr &)> &&callback,
                                                    const std::string &id) {
    try {
        int wordId = std::stoi(id);
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        auto result = dbClient->execSqlSync(
            "DELETE FROM sensitive_words WHERE id = $1 RETURNING id", wordId);
        if (result.empty()) {
            callback(ResponseUtil::notFound("敏感词不存在"));
            return;
        }
        writeOperationLog(req, adminId, "sensitive_delete", "sensitive_word", id, "删除敏感词");
        callback(ResponseUtil::success());
    } catch (const std::invalid_argument &) {
        callback(ResponseUtil::badRequest("无效的ID格式"));
    } catch (const std::out_of_range &) {
        callback(ResponseUtil::badRequest("ID超出范围"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin deleteSensitiveWord error: " << e.what();
        callback(ResponseUtil::internalError("删除失败"));
    }
}

void AdminManagementController::getSystemConfig(const HttpRequestPtr &,
                                                std::function<void(const HttpResponsePtr &)> &&callback) {
    auto config = AdminConfigStore::load();
    callback(ResponseUtil::success(config));
}

void AdminManagementController::updateSystemConfig(const HttpRequestPtr &req,
                                                   std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::badRequest("无效配置"));
        return;
    }

    auto config = AdminConfigStore::load();
    for (const auto &key : json->getMemberNames()) {
        config[key] = (*json)[key];
    }

    if (!AdminConfigStore::save(config)) {
        callback(ResponseUtil::internalError("保存失败"));
        return;
    }

    // 更新AI配置
    if (config.isMember("ai")) {
        ai::AIService::getInstance().initialize(config["ai"]);
    }

    auto adminId = req->getAttributes()->get<std::string>("admin_id");
    writeOperationLog(req, adminId, "config", "config", "", "更新系统配置");
    callback(ResponseUtil::success());
}

void AdminManagementController::broadcastMessage(const HttpRequestPtr &req,
                                                 std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("message")) {
        callback(ResponseUtil::badRequest("message不能为空"));
        return;
    }

    Json::Value payload;
    payload["message"] = (*json)["message"].asString();
    payload["level"] = (*json).get("level", "info").asString();

    try {
        auto adminId = req->getAttributes()->get<std::string>("admin_id");
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync(
            "INSERT INTO broadcast_messages (title, content, level, created_at) VALUES ($1, $2, $3, NOW())",
            "", (*json)["message"].asString(), (*json).get("level", "info").asString()
        );
        writeOperationLog(req, adminId, "broadcast", "broadcast", "", (*json)["message"].asString());
    } catch (const std::exception& e) {
        LOG_WARN << "persist broadcast message failed: " << e.what();
    }

    BroadcastWebSocketController::broadcast(
        buildRealtimeEvent("broadcast", std::move(payload)));
    callback(ResponseUtil::success());
}

void AdminManagementController::getBroadcastHistory(const HttpRequestPtr &req,
                                                    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = app().getDbClient("default");
        auto [page, pageSize] = safePagination(req);

        auto result = dbClient->execSqlSync(
            "SELECT *, COUNT(*) OVER() AS total_count "
            "FROM broadcast_messages ORDER BY created_at DESC LIMIT $1 OFFSET $2",
            static_cast<int64_t>(pageSize), static_cast<int64_t>((page - 1) * pageSize)
        );
        const int total = resolveWindowTotalOrFallbackCount(
            result, static_cast<int64_t>(page - 1) * pageSize);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["id"] = row["id"].as<int>();
            item["title"] = row["title"].as<std::string>();
            item["content"] = row["content"].as<std::string>();
            item["level"] = row["level"].as<std::string>();
            item["created_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        callback(collectionResponse(list, "broadcasts", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        LOG_ERROR << "getBroadcastHistory error: " << e.what();
        callback(ResponseUtil::internalError("获取广播历史失败"));
    }
}

void AdminManagementController::getOperationLogs(const HttpRequestPtr &req,
                                                 std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = app().getDbClient("default");
        auto [page, pageSize] = safePagination(req);
        const int64_t offset = static_cast<int64_t>(page - 1) * pageSize;

        std::string countSql = "SELECT COUNT(*) as total FROM operation_logs WHERE 1=1";
        std::string querySql =
            "SELECT id, admin_id, action, target_type, target_id, detail as details, created_at, "
            "COUNT(*) OVER() AS total_count "
            "FROM operation_logs WHERE 1=1";
        std::vector<std::string> params;
        int paramIdx = 1;

        const auto adminId = !req->getParameter("operator").empty()
            ? req->getParameter("operator")
            : req->getParameter("admin_id");
        const auto action = req->getParameter("action");
        const auto startDate = req->getParameter("start_date");
        const auto endDate = req->getParameter("end_date");

        if (!adminId.empty()) {
            const auto placeholder = "$" + std::to_string(paramIdx);
            countSql += " AND admin_id ILIKE " + placeholder + " ESCAPE '\\'";
            querySql += " AND admin_id ILIKE " + placeholder + " ESCAPE '\\'";
            params.push_back("%" + escapeLike(adminId) + "%");
            paramIdx++;
        }

        if (!action.empty()) {
            countSql += " AND action = $" + std::to_string(paramIdx);
            querySql += " AND action = $" + std::to_string(paramIdx);
            params.push_back(action);
            paramIdx++;
        }

        if (!startDate.empty()) {
            countSql += " AND created_at >= $" + std::to_string(paramIdx) + "::date";
            querySql += " AND created_at >= $" + std::to_string(paramIdx) + "::date";
            params.push_back(startDate);
            paramIdx++;
        }

        if (!endDate.empty()) {
            countSql += " AND created_at < ($" + std::to_string(paramIdx) + "::date + INTERVAL '1 day')";
            querySql += " AND created_at < ($" + std::to_string(paramIdx) + "::date + INTERVAL '1 day')";
            params.push_back(endDate);
            paramIdx++;
        }

        const int limitParam = paramIdx++;
        const int offsetParam = paramIdx;
        querySql += " ORDER BY created_at DESC LIMIT $" + std::to_string(limitParam) +
                    " OFFSET $" + std::to_string(offsetParam);

        auto result = execSqlWithStringParamsAndPagination(
            dbClient, querySql, params, static_cast<int64_t>(pageSize), offset);
        const int total = resolveWindowTotalOrFallbackCount(result, offset);

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["id"] = row["id"].as<int>();
            item["admin_id"] = row["admin_id"].as<std::string>();
            item["action"] = row["action"].as<std::string>();
            item["target_type"] = row["target_type"].isNull() ? "" : row["target_type"].as<std::string>();
            item["target_id"] = row["target_id"].isNull() ? "" : row["target_id"].as<std::string>();
            item["details"] = row["details"].isNull() ? "" : row["details"].as<std::string>();
            item["created_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        callback(collectionResponse(list, "logs", total, page, pageSize));
    } catch (const std::out_of_range &e) {
        callback(ResponseUtil::badRequest(e.what()));
    } catch (const std::exception &e) {
        LOG_ERROR << "getOperationLogs error: " << e.what();
        callback(ResponseUtil::internalError("获取操作日志失败"));
    }
}
