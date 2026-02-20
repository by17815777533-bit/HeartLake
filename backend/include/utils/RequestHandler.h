/**
 * @file RequestHandler.h
 * @brief 统一请求处理工具 - 消除 Controller 中重复的 try-catch 和 JSON 校验模式
 * Created by Claude Code
 */

#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/Exception.h>
#include "utils/ResponseUtil.h"
#include <functional>
#include <string>

namespace heartlake {
namespace utils {

/**
 * @brief 统一请求处理工具类
 *
 * 封装了 Controller 中常见的 try-catch、JSON 校验、user_id 提取等重复逻辑。
 * 使用方式：
 *   RequestHandler::handle(req, callback, [](auto& req, auto& cb) {
 *       // 业务逻辑
 *   });
 */
class RequestHandler {
public:
    using Callback = std::function<void(const drogon::HttpResponsePtr&)>;
    using Handler = std::function<void(const drogon::HttpRequestPtr&, Callback&)>;
    using JsonHandler = std::function<void(const Json::Value&, const drogon::HttpRequestPtr&, Callback&)>;

    /**
     * @brief 通用请求处理（自动 try-catch + 日志）
     */
    static void handle(
        const drogon::HttpRequestPtr& req,
        Callback&& callback,
        const std::string& operationName,
        Handler&& handler
    ) {
        try {
            handler(req, callback);
        } catch (const std::runtime_error& e) {
            LOG_ERROR << "Error in " << operationName << ": " << e.what();
            callback(ResponseUtil::error(400, e.what()));
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Database error in " << operationName << ": " << e.base().what();
            callback(ResponseUtil::internalError("数据库错误"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Unexpected error in " << operationName << ": " << e.what();
            callback(ResponseUtil::internalError());
        }
    }

    /**
     * @brief 需要 JSON body 的请求处理（自动校验 JSON + try-catch）
     */
    static void handleJson(
        const drogon::HttpRequestPtr& req,
        Callback&& callback,
        const std::string& operationName,
        JsonHandler&& handler
    ) {
        try {
            auto json = req->getJsonObject();
            if (!json) {
                callback(ResponseUtil::badRequest("请求体必须是 JSON 格式"));
                return;
            }
            handler(*json, req, callback);
        } catch (const std::runtime_error& e) {
            LOG_ERROR << "Error in " << operationName << ": " << e.what();
            callback(ResponseUtil::error(400, e.what()));
        } catch (const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Database error in " << operationName << ": " << e.base().what();
            callback(ResponseUtil::internalError("数据库错误"));
        } catch (const std::exception& e) {
            LOG_ERROR << "Unexpected error in " << operationName << ": " << e.what();
            callback(ResponseUtil::internalError());
        }
    }

    /**
     * @brief 安全地从 request attributes 提取 user_id（由认证中间件注入）
     */
    static std::string extractUserId(const drogon::HttpRequestPtr& req) {
        try {
            return req->getAttributes()->get<std::string>("user_id");
        } catch (...) {
            return "";
        }
    }

    /**
     * @brief 提取 user_id，为空时抛异常
     */
    static std::string extractUserIdOrThrow(const drogon::HttpRequestPtr& req) {
        std::string userId = extractUserId(req);
        if (userId.empty()) {
            throw std::runtime_error("未登录");
        }
        return userId;
    }

    /**
     * @brief 安全解析分页参数
     */
    static std::pair<int, int> extractPagination(const drogon::HttpRequestPtr& req, int defaultPage = 1, int defaultSize = 20, int maxSize = 100) {
        int page = defaultPage, pageSize = defaultSize;
        if (auto p = req->getParameter("page"); !p.empty()) { try { page = std::stoi(p); } catch (...) {} }
        if (auto p = req->getParameter("page_size"); !p.empty()) { try { pageSize = std::stoi(p); } catch (...) {} }
        if (page < 1) page = 1;
        if (pageSize < 1 || pageSize > maxSize) pageSize = defaultSize;
        return {page, pageSize};
    }

    /**
     * @brief 校验 JSON 必填字段
     */
    static bool validateRequired(const Json::Value& json, const std::vector<std::string>& fields, Callback& callback) {
        for (const auto& field : fields) {
            if (!json.isMember(field) || json[field].asString().empty()) {
                callback(ResponseUtil::badRequest(field + " 不能为空"));
                return false;
            }
        }
        return true;
    }
};

} // namespace utils
} // namespace heartlake
