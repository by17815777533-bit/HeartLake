/**
 * @file ReportController.cpp
 * @brief ReportController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/ReportController.h"
#include "utils/ResponseUtil.h"
#include "utils/IdGenerator.h"

using namespace heartlake::controllers;
using namespace heartlake::utils;

void ReportController::createReport(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        std::string user_id;
        try { user_id = req->getAttributes()->get<std::string>("user_id"); } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        auto json = req->getJsonObject();
        if (!json) {
            callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
            return;
        }
        
        std::string target_type = (*json)["target_type"].asString(); // "stone", "boat", "user"
        std::string target_id = (*json)["target_id"].asString();
        std::string reason = (*json)["reason"].asString(); // "spam", "inappropriate", "harassment", etc.
        std::string description = (*json)["description"].asString();
        
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
        try { user_id = req->getAttributes()->get<std::string>("user_id"); } catch (...) {}
        if (user_id.empty()) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }
        
        int page = 1, page_size = 20;
        if (auto p = req->getParameter("page"); !p.empty()) { try { page = std::stoi(p); } catch (...) {} }
        if (auto p = req->getParameter("page_size"); !p.empty()) { try { page_size = std::stoi(p); } catch (...) {} }
        
        if (page < 1) page = 1;
        if (page_size < 1 || page_size > 100) page_size = 20;
        
        auto dbClient = drogon::app().getDbClient("default");
        
        // 获取总数
        auto countResult = dbClient->execSqlSync(
            "SELECT COUNT(*) as total FROM reports WHERE reporter_id = $1",
            user_id
        );
        int total = countResult.empty() ? 0 : countResult[0]["total"].as<int>();

        // 获取列表
        int offset = (page - 1) * page_size;
        auto result = dbClient->execSqlSync(
            "SELECT report_id, target_type, target_id, reason, description, status, created_at "
            "FROM reports WHERE reporter_id = $1 "
            "ORDER BY created_at DESC LIMIT $2 OFFSET $3",
            user_id, std::to_string(page_size), std::to_string(offset)
        );
        
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
        
        Json::Value responseData;
        responseData["reports"] = reports;
        responseData["total"] = total;
        responseData["page"] = page;
        responseData["page_size"] = page_size;
        
        callback(ResponseUtil::success(responseData));
        
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in getMyReports: " << e.base().what();
        callback(ResponseUtil::internalError("数据库错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in getMyReports: " << e.what();
        callback(ResponseUtil::internalError());
    }
}
