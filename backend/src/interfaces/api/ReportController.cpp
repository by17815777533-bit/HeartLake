/**
 * ReportController 模块实现
 */
#include "interfaces/api/ReportController.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/AdminRealtimeNotifier.h"
#include "utils/ResponseUtil.h"
#include "utils/IdGenerator.h"
#include "utils/RealtimeEvent.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include <ctime>

using namespace heartlake::controllers;
using namespace heartlake::utils;

namespace {
int extractWindowTotal(const drogon::orm::Result &result,
                       const std::string &column = "total_count") {
    return result.empty() || result[0][column].isNull()
               ? 0
               : result[0][column].as<int>();
}

int64_t paginationOffset(int page, int pageSize) {
    return static_cast<int64_t>(page - 1) * static_cast<int64_t>(pageSize);
}
} // namespace

void ReportController::createReport(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        std::string user_id;
        auto userIdOpt = Validator::getUserId(req);
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }
        user_id = *userIdOpt;

        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }
        
        std::string target_type = (*json)["target_type"].asString(); // "stone", "boat", "user"
        std::string target_id = (*json)["target_id"].asString();
        std::string reason = (*json)["reason"].asString(); // "spam", "inappropriate", "harassment", etc.
        std::string description = (*json)["description"].asString();
        if (description.size() > 2000) {
            callback(ResponseUtil::badRequest("description长度不能超过2000"));
            return;
        }
        
        if (target_type.empty() || target_id.empty() || reason.empty()) {
            callback(ResponseUtil::badRequest("target_type, target_id 和 reason 不能为空"));
            return;
        }
        
        // 验证target_type
        if (target_type != "stone" && target_type != "boat" && target_type != "user") {
            callback(ResponseUtil::badRequest("target_type 必须是 stone, boat 或 user"));
            return;
        }
        
        auto dbClient = drogon::app().getDbClient("default");
        
        // 检查是否已经举报过
        auto checkResult = dbClient->execSqlSync(
            "SELECT report_id FROM reports WHERE reporter_id = $1 AND target_type = $2 AND target_id = $3 AND status = 'pending'",
            user_id, target_type, target_id
        );

        if (!checkResult.empty()) {
            callback(ResponseUtil::error(409, "您已经举报过该内容"));
            return;
        }

        std::string report_id = IdGenerator::generateReportId();

        dbClient->execSqlSync(
            "INSERT INTO reports (report_id, reporter_id, target_type, target_id, reason, description, status, created_at) "
            "VALUES ($1, $2, $3, $4, $5, $6, 'pending', NOW())",
            report_id, user_id, target_type, target_id, reason, description
        );
        
        Json::Value responseData;
        responseData["report_id"] = report_id;

        Json::Value wsMsg;
        wsMsg["report_id"] = report_id;
        wsMsg["target_type"] = target_type;
        wsMsg["target_id"] = target_id;
        wsMsg["reason"] = reason;
        wsMsg["reporter_id"] = user_id;
        wsMsg["status"] = "pending";
        heartlake::controllers::BroadcastWebSocketController::broadcast(
            buildRealtimeEvent("new_report", std::move(wsMsg)));
        heartlake::utils::broadcastAdminRealtimeStatsUpdate("new_report");
        
        callback(ResponseUtil::success(responseData, "举报已提交，我们会尽快处理"));
        
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in createReport: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in createReport: " << e.what();
        callback(ResponseUtil::internalError());
    }
}

void ReportController::getMyReports(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        std::string user_id;
        auto userIdOpt = Validator::getUserId(req);
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }
        user_id = *userIdOpt;

        auto [page, page_size] = safePagination(req);

        auto dbClient = drogon::app().getDbClient("default");
        const int64_t offset = paginationOffset(page, page_size);
        auto result = dbClient->execSqlSync(
            "SELECT report_id, target_type, target_id, reason, description, status, created_at, "
            "COUNT(*) OVER() AS total_count "
            "FROM reports WHERE reporter_id = $1 "
            "ORDER BY created_at DESC LIMIT $2 OFFSET $3",
            user_id, static_cast<int64_t>(page_size), offset
        );
        if (result.empty() && page > 1) {
            callback(ResponseUtil::badRequest("页码超出范围，请返回上一页重试"));
            return;
        }
        const int total = extractWindowTotal(result);
        
        Json::Value reports(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value report;
            report["report_id"] = row["report_id"].as<std::string>();
            report["target_type"] = row["target_type"].as<std::string>();
            report["target_id"] = row["target_id"].as<std::string>();
            report["reason"] = row["reason"].as<std::string>();
            report["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
            report["status"] = row["status"].as<std::string>();
            report["created_at"] = row["created_at"].as<std::string>();
            reports.append(report);
        }
        
        callback(ResponseUtil::success(ResponseUtil::buildCollectionPayload(
            "reports", reports, total, page, page_size)));
        
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in getMyReports: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getMyReports: " << e.what();
        callback(ResponseUtil::internalError());
    }
}
