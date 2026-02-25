/**
 * @file AdminManagementController.cpp
 * @brief AdminManagementController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/AdminManagementController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/ResponseUtil.h"
#include "utils/IdGenerator.h"
#include "utils/AdminConfigStore.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include "infrastructure/ai/AIService.h"
#include <drogon/orm/Exception.h>
#include <json/json.h>

using namespace drogon;
using namespace heartlake::utils;
using namespace heartlake::controllers;

namespace {
std::string escapeLike(const std::string &value) {
    std::string result;
    result.reserve(value.size() * 2);
    for (char c : value) {
        if (c == '%' || c == '_' || c == '\\') {
            result += '\\';
            result += c;
        } else {
            result += c;
        }
    }
    return result;
}

bool isValidUserStatus(const std::string &s) {
    return s == "active" || s == "banned" || s == "deleted";
}

bool isValidReportStatus(const std::string &s) {
    return s == "pending" || s == "handled" || s == "ignored";
}
}

void AdminManagementController::getUsers(const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        int offset = (page - 1) * pageSize;

        const auto userId = req->getParameter("userId");
        const auto nickname = req->getParameter("nickname");
        const auto status = req->getParameter("status");
        const auto search = req->getParameter("search");

        auto dbClient = app().getDbClient("default");

        // 使用参数化查询防止SQL注入
        std::string countSql = "SELECT COUNT(*) as total FROM users u WHERE 1=1";
        std::string querySql =
            "SELECT u.user_id, u.username, u.nickname, u.status, u.created_at, u.last_active_at, "
            "0 as stones_count, "
            "0 as boat_count "
            "FROM users u WHERE 1=1";

        std::vector<std::string> params;
        int paramIdx = 1;

        if (!userId.empty()) {
            countSql += " AND u.user_id LIKE $" + std::to_string(paramIdx) + " ESCAPE '\\'";
            querySql += " AND u.user_id LIKE $" + std::to_string(paramIdx) + " ESCAPE '\\'";
            params.push_back("%" + escapeLike(userId) + "%");
            paramIdx++;
        }
        if (!nickname.empty()) {
            countSql += " AND u.nickname LIKE $" + std::to_string(paramIdx) + " ESCAPE '\\'";
            querySql += " AND u.nickname LIKE $" + std::to_string(paramIdx) + " ESCAPE '\\'";
            params.push_back("%" + escapeLike(nickname) + "%");
            paramIdx++;
        }
        if (!status.empty() && isValidUserStatus(status)) {
            countSql += " AND u.status = $" + std::to_string(paramIdx);
            querySql += " AND u.status = $" + std::to_string(paramIdx);
            params.push_back(status);
            paramIdx++;
        }
        // BUG-2 修复：search 参数加入 SQL WHERE 条件，支持模糊搜索 username/nickname/user_id
        if (!search.empty()) {
            std::string searchCondition = " AND (u.username ILIKE $" + std::to_string(paramIdx) +
                " OR u.nickname ILIKE $" + std::to_string(paramIdx) +
                " OR u.user_id ILIKE $" + std::to_string(paramIdx) + ")";
            countSql += searchCondition;
            querySql += searchCondition;
            params.push_back("%" + escapeLike(search) + "%");
            paramIdx++;
        }

        // LIMIT/OFFSET 内联为安全整数（parsePage/parsePageSize 保证为 int）
        querySql += " ORDER BY u.created_at DESC LIMIT " + std::to_string(pageSize) +
                    " OFFSET " + std::to_string(offset);

        // 执行查询：countSql 和 querySql 使用相同的 filter params
        auto execWithParams = [&](const std::string& sql) {
            switch (params.size()) {
                case 0: return dbClient->execSqlSync(sql);
                case 1: return dbClient->execSqlSync(sql, params[0]);
                case 2: return dbClient->execSqlSync(sql, params[0], params[1]);
                case 3: return dbClient->execSqlSync(sql, params[0], params[1], params[2]);
                default: return dbClient->execSqlSync(sql, params[0], params[1], params[2], params[3]);
            }
        };
        auto countResult = execWithParams(countSql);
        auto result = execWithParams(querySql);

        int total = safeCount(countResult);

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

        Json::Value data;
        data["users"] = users;
        data["total"] = total;
        callback(ResponseUtil::success(data));
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
            "0 as stones_count, "
            "0 as boat_count "
            "FROM users u WHERE u.user_id = $1",
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
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync("UPDATE users SET status = $1, updated_at = NOW() WHERE user_id = $2", status, userId);
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin updateUserStatus error: " << e.what();
        callback(ResponseUtil::internalError("更新失败"));
    }
}

void AdminManagementController::banUser(const HttpRequestPtr &,
                                        std::function<void(const HttpResponsePtr &)> &&callback,
                                        const std::string &userId) {
    try {
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync("UPDATE users SET status = 'banned', updated_at = NOW() WHERE user_id = $1", userId);
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin banUser error: " << e.what();
        callback(ResponseUtil::internalError("封禁失败"));
    }
}

void AdminManagementController::unbanUser(const HttpRequestPtr &,
                                          std::function<void(const HttpResponsePtr &)> &&callback,
                                          const std::string &userId) {
    try {
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync("UPDATE users SET status = 'active', updated_at = NOW() WHERE user_id = $1", userId);
        callback(ResponseUtil::success());
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin unbanUser error: " << e.what();
        callback(ResponseUtil::internalError("解封失败"));
    }
}

void AdminManagementController::getStones(const HttpRequestPtr &req,
                                          std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto [page, pageSize] = safePagination(req);
        int offset = (page - 1) * pageSize;

        auto dbClient = app().getDbClient("default");
        auto countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM stones");
        int total = safeCount(countResult);

        auto result = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, s.mood_type, s.is_anonymous, "
            "s.ripple_count, s.boat_count, s.status, s.created_at, u.nickname "
            "FROM stones s LEFT JOIN users u ON s.user_id = u.user_id "
            "ORDER BY s.created_at DESC LIMIT " + std::to_string(pageSize) +
            " OFFSET " + std::to_string(offset)
        );

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

        Json::Value data;
        data["list"] = list;
        data["total"] = total;
        callback(ResponseUtil::success(data));
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

void AdminManagementController::deleteStone(const HttpRequestPtr &,
                                            std::function<void(const HttpResponsePtr &)> &&callback,
                                            const std::string &stoneId) {
    try {
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync("UPDATE stones SET status = 'deleted', deleted_at = NOW(), updated_at = NOW() WHERE stone_id = $1", stoneId);
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
        int offset = (page - 1) * pageSize;

        auto dbClient = app().getDbClient("default");
        auto countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM paper_boats");
        int total = safeCount(countResult);

        auto result = dbClient->execSqlSync(
            "SELECT b.boat_id, b.content, b.is_anonymous, b.status, b.created_at, u.nickname "
            "FROM paper_boats b LEFT JOIN users u ON b.sender_id = u.user_id "
            "ORDER BY b.created_at DESC LIMIT $1 OFFSET $2",
            std::to_string(pageSize), std::to_string(offset)
        );

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["boat_id"] = row["boat_id"].as<std::string>();
            item["content"] = row["content"].isNull() ? "" : row["content"].as<std::string>();
            item["is_anonymous"] = row["is_anonymous"].isNull() ? false : row["is_anonymous"].as<bool>();
            item["status"] = row["status"].isNull() ? "active" : row["status"].as<std::string>();
            item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
            item["author_nickname"] = row["nickname"].isNull() ? "匿名" : row["nickname"].as<std::string>();
            list.append(item);
        }

        Json::Value data;
        data["list"] = list;
        data["total"] = total;
        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "Admin getBoats error: " << e.what();
        // QUALITY-2 修复：错误时返回 500 而非伪装成功
        callback(ResponseUtil::internalError("获取纸船列表失败"));
    }
}

void AdminManagementController::deleteBoat(const HttpRequestPtr &,
                                           std::function<void(const HttpResponsePtr &)> &&callback,
                                           const std::string &boatId) {
    try {
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync("UPDATE paper_boats SET status = 'deleted', deleted_at = NOW() WHERE boat_id = $1", boatId);
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
        auto countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM reports WHERE status = 'pending'");
        int total = safeCount(countResult);

        auto result = dbClient->execSqlSync(
            "SELECT r.report_id, r.target_type, r.target_id, r.reason, r.created_at, "
            "s.content as stone_content, b.content as boat_content "
            "FROM reports r "
            "LEFT JOIN stones s ON r.target_type = 'stone' AND r.target_id = s.stone_id "
            "LEFT JOIN paper_boats b ON r.target_type = 'boat' AND r.target_id = b.boat_id "
            "WHERE r.status = 'pending' "
            "ORDER BY r.created_at DESC LIMIT $1 OFFSET $2",
            std::to_string(pageSize), std::to_string(offset)
        );

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
            item["risk_level"] = "medium";
            item["confidence"] = 0.8;
            item["created_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        Json::Value data;
        data["list"] = list;
        data["total"] = total;
        callback(ResponseUtil::success(data));
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
        dbClient->execSqlSync("UPDATE reports SET status = 'handled', handled_by = $1, handled_at = NOW() WHERE report_id = $2", adminId, moderationId);

        const std::string logId = "mod_" + IdGenerator::generateMessageId();
        dbClient->execSqlSync(
            "INSERT INTO moderation_logs (log_id, target_type, target_id, action, reason, operator_id, created_at) "
            "SELECT $1, target_type, target_id, 'handled', reason, $2, NOW() FROM reports WHERE report_id = $3",
            logId, adminId, moderationId
        );

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
        dbClient->execSqlSync("UPDATE reports SET status = 'ignored', handled_by = $1, handled_at = NOW() WHERE report_id = $2", adminId, moderationId);

        const std::string logId = "mod_" + IdGenerator::generateMessageId();
        dbClient->execSqlSync(
            "INSERT INTO moderation_logs (log_id, target_type, target_id, action, reason, operator_id, created_at) "
            "SELECT $1, target_type, target_id, 'ignored', $2, $3, NOW() FROM reports WHERE report_id = $4",
            logId, reason, adminId, moderationId
        );

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
        int offset = (page - 1) * pageSize;

        auto dbClient = app().getDbClient("default");
        auto countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM moderation_logs");
        int total = safeCount(countResult);

        auto result = dbClient->execSqlSync(
            "SELECT action, reason, operator_id, created_at, target_type, target_id FROM moderation_logs "
            "ORDER BY created_at DESC LIMIT $1 OFFSET $2",
            std::to_string(pageSize), std::to_string(offset)
        );

        Json::Value list(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value item;
            item["content_type"] = row["target_type"].as<std::string>();
            item["content"] = row["target_id"].as<std::string>();
            item["result"] = row["action"].as<std::string>();
            item["reason"] = row["reason"].as<std::string>();
            item["moderator"] = row["operator_id"].as<std::string>();
            item["moderated_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        Json::Value data;
        data["list"] = list;
        data["total"] = total;
        callback(ResponseUtil::success(data));
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
        int offset = (page - 1) * pageSize;

        const auto status = req->getParameter("status");
        const auto reason = req->getParameter("type");

        auto dbClient = app().getDbClient("default");
        std::optional<orm::Result> countResult, result;

        // 使用参数化查询防止SQL注入
        if (!status.empty() && isValidReportStatus(status) && !reason.empty()) {
            countResult = dbClient->execSqlSync(
                "SELECT COUNT(*) as total FROM reports r WHERE r.status = $1 AND r.reason = $2",
                status, reason);
            result = dbClient->execSqlSync(
                "SELECT r.report_id, r.target_type, r.target_id, r.reason, r.description, r.status, r.created_at, "
                "COALESCE(s.content, '') as stone_content, COALESCE(b.content, '') as boat_content "
                "FROM reports r "
                "LEFT JOIN stones s ON r.target_type = 'stone' AND r.target_id = s.stone_id "
                "LEFT JOIN paper_boats b ON r.target_type = 'boat' AND r.target_id = b.boat_id "
                "WHERE r.status = $1 AND r.reason = $2 ORDER BY r.created_at DESC LIMIT $3 OFFSET $4",
                status, reason, std::to_string(pageSize), std::to_string(offset));
        } else if (!status.empty() && isValidReportStatus(status)) {
            countResult = dbClient->execSqlSync(
                "SELECT COUNT(*) as total FROM reports r WHERE r.status = $1", status);
            result = dbClient->execSqlSync(
                "SELECT r.report_id, r.target_type, r.target_id, r.reason, r.description, r.status, r.created_at, "
                "COALESCE(s.content, '') as stone_content, COALESCE(b.content, '') as boat_content "
                "FROM reports r "
                "LEFT JOIN stones s ON r.target_type = 'stone' AND r.target_id = s.stone_id "
                "LEFT JOIN paper_boats b ON r.target_type = 'boat' AND r.target_id = b.boat_id "
                "WHERE r.status = $1 ORDER BY r.created_at DESC LIMIT $2 OFFSET $3",
                status, std::to_string(pageSize), std::to_string(offset));
        } else if (!reason.empty()) {
            countResult = dbClient->execSqlSync(
                "SELECT COUNT(*) as total FROM reports r WHERE r.reason = $1", reason);
            result = dbClient->execSqlSync(
                "SELECT r.report_id, r.target_type, r.target_id, r.reason, r.description, r.status, r.created_at, "
                "COALESCE(s.content, '') as stone_content, COALESCE(b.content, '') as boat_content "
                "FROM reports r "
                "LEFT JOIN stones s ON r.target_type = 'stone' AND r.target_id = s.stone_id "
                "LEFT JOIN paper_boats b ON r.target_type = 'boat' AND r.target_id = b.boat_id "
                "WHERE r.reason = $1 ORDER BY r.created_at DESC LIMIT $2 OFFSET $3",
                reason, std::to_string(pageSize), std::to_string(offset));
        } else {
            countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM reports r");
            result = dbClient->execSqlSync(
                "SELECT r.report_id, r.target_type, r.target_id, r.reason, r.description, r.status, r.created_at, "
                "COALESCE(s.content, '') as stone_content, COALESCE(b.content, '') as boat_content "
                "FROM reports r "
                "LEFT JOIN stones s ON r.target_type = 'stone' AND r.target_id = s.stone_id "
                "LEFT JOIN paper_boats b ON r.target_type = 'boat' AND r.target_id = b.boat_id "
                "ORDER BY r.created_at DESC LIMIT $1 OFFSET $2",
                std::to_string(pageSize), std::to_string(offset));
        }
        int total = (!countResult || countResult->empty()) ? 0 : safeCount(*countResult);

        Json::Value list(Json::arrayValue);
        for (const auto &row : *result) {
            Json::Value item;
            item["id"] = row["report_id"].as<std::string>();
            item["type"] = row["reason"].as<std::string>();
            item["reason"] = row["description"].as<std::string>();
            const auto stoneContent = row["stone_content"].isNull() ? "" : row["stone_content"].as<std::string>();
            const auto boatContent = row["boat_content"].isNull() ? "" : row["boat_content"].as<std::string>();
            item["target_content"] = !stoneContent.empty() ? stoneContent : boatContent;
            item["status"] = row["status"].as<std::string>();
            item["created_at"] = row["created_at"].as<std::string>();
            list.append(item);
        }

        Json::Value data;
        data["list"] = list;
        data["total"] = total;
        callback(ResponseUtil::success(data));
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
        dbClient->execSqlSync(
            "UPDATE reports SET status = $1, handled_by = $2, handled_at = NOW() WHERE report_id = $3",
            action, adminId, reportId
        );
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
        int offset = (page - 1) * pageSize;

        auto dbClient = app().getDbClient("default");
        auto countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM sensitive_words");
        int total = safeCount(countResult);

        auto result = dbClient->execSqlSync(
            "SELECT id, word, level, category, action, created_at FROM sensitive_words "
            "ORDER BY created_at DESC LIMIT $1 OFFSET $2",
            std::to_string(pageSize), std::to_string(offset)
        );

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

        Json::Value data;
        data["words"] = words;
        data["total"] = total;
        callback(ResponseUtil::success(data));
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
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync(
            "INSERT INTO sensitive_words (word, level, action, category, created_at) VALUES ($1, $2, $3, $4, NOW())",
            (*json)["word"].asString(),
            (*json).get("level", "medium").asString(),
            (*json).get("action", "block").asString(),
            (*json).get("category", "other").asString()
        );
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
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync(
            "UPDATE sensitive_words SET word = $1, level = $2, action = $3, category = $4 WHERE id = $5",
            (*json)["word"].asString(),
            (*json).get("level", "medium").asString(),
            (*json).get("action", "block").asString(),
            (*json).get("category", "other").asString(),
            wordId
        );
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

void AdminManagementController::deleteSensitiveWord(const HttpRequestPtr &,
                                                    std::function<void(const HttpResponsePtr &)> &&callback,
                                                    const std::string &id) {
    try {
        int wordId = std::stoi(id);
        auto dbClient = app().getDbClient("default");
        dbClient->execSqlSync("DELETE FROM sensitive_words WHERE id = $1", wordId);
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
    payload["type"] = "broadcast";
    payload["message"] = (*json)["message"].asString();
    payload["level"] = (*json).get("level", "info").asString();
    payload["timestamp"] = static_cast<Json::Int64>(time(nullptr));

    BroadcastWebSocketController::broadcast(payload);
    callback(ResponseUtil::success());
}

void AdminManagementController::getBroadcastHistory(const HttpRequestPtr &req,
                                                    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = app().getDbClient("default");
        auto [page, pageSize] = safePagination(req);

        auto countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM broadcast_messages");
        int total = safeCount(countResult);

        auto result = dbClient->execSqlSync(
            "SELECT * FROM broadcast_messages ORDER BY created_at DESC LIMIT $1 OFFSET $2",
            pageSize, (page - 1) * pageSize
        );

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

        Json::Value data;
        data["list"] = list;
        data["total"] = total;
        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "getBroadcastHistory error: " << e.what();
        Json::Value data;
        data["list"] = Json::arrayValue;
        data["total"] = 0;
        callback(ResponseUtil::success(data));
    }
}

void AdminManagementController::getOperationLogs(const HttpRequestPtr &req,
                                                 std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto dbClient = app().getDbClient("default");
        auto [page, pageSize] = safePagination(req);

        auto countResult = dbClient->execSqlSync("SELECT COUNT(*) as total FROM admin_operation_logs");
        int total = safeCount(countResult);

        auto result = dbClient->execSqlSync(
            "SELECT * FROM admin_operation_logs ORDER BY created_at DESC LIMIT $1 OFFSET $2",
            pageSize, (page - 1) * pageSize
        );

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

        Json::Value data;
        data["list"] = list;
        data["total"] = total;
        callback(ResponseUtil::success(data));
    } catch (const std::exception &e) {
        LOG_ERROR << "getOperationLogs error: " << e.what();
        Json::Value data;
        data["list"] = Json::arrayValue;
        data["total"] = 0;
        callback(ResponseUtil::success(data));
    }
}
