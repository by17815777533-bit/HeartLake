/**
 * @file SafeHarborController.cpp
 * @brief 安全港湾危机干预控制器实现 - 心湖最后一片社交净土的守护者
 */

#include "interfaces/api/SafeHarborController.h"
#include "infrastructure/services/SafeHarborService.h"
#include "utils/ResponseUtil.h"
#include "utils/PasetoUtil.h"

using namespace heartlake::controllers;
using namespace heartlake::utils;

static std::string extractUserId(const drogon::HttpRequestPtr& req) {
    try {
        return req->getAttributes()->get<std::string>("user_id");
    } catch (...) {
        LOG_WARN << "Failed to extract user_id from request attributes";
        return "";
    }
}

void SafeHarborController::getHotlines(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    try {
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        callback(ResponseUtil::success(service.getHotlines()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get hotlines: " << e.what();
        callback(ResponseUtil::internalError("获取热线信息失败"));
    }
}

void SafeHarborController::getSelfHelpTools(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    try {
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        callback(ResponseUtil::success(service.getSelfHelpTools()));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get self-help tools: " << e.what();
        callback(ResponseUtil::internalError("获取自助工具失败"));
    }
}

void SafeHarborController::getWarmPrompt(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    try {
        std::string riskLevel = req->getParameter("risk_level");
        if (riskLevel.empty()) riskLevel = "low";
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        callback(ResponseUtil::success(service.getWarmPrompt(riskLevel)));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get warm prompt: " << e.what();
        callback(ResponseUtil::internalError("获取温馨提示失败"));
    }
}

// 援助资源CRUD
void SafeHarborController::addResource(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    // SEC-05: 认证检查 — 防止未登录用户篡改危机干预资源
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("name") || !json->isMember("type")) {
        callback(ResponseUtil::badRequest("缺少必要字段: name, type"));
        return;
    }
    try {
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        auto result = service.addResource(*json);
        callback(result.isNull() ? ResponseUtil::error(500, "添加资源失败") : ResponseUtil::success(result, "资源添加成功"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to add resource: " << e.what();
        callback(ResponseUtil::internalError("添加资源失败"));
    }
}

void SafeHarborController::updateResource(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& resourceId
) {
    // SEC-05: 认证检查 — 防止未登录用户篡改危机干预资源
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    if (resourceId.empty()) {
        callback(ResponseUtil::badRequest("资源ID不能为空"));
        return;
    }
    auto json = req->getJsonObject();
    if (!json) {
        callback(ResponseUtil::badRequest("请求体不能为空"));
        return;
    }
    try {
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        bool success = service.updateResource(resourceId, *json);
        callback(success ? ResponseUtil::success("资源更新成功") : ResponseUtil::notFound("资源不存在"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to update resource: " << e.what();
        callback(ResponseUtil::internalError("更新资源失败"));
    }
}

void SafeHarborController::deleteResource(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& resourceId
) {
    // SEC-05: 认证检查 — 防止未登录用户删除危机干预资源
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    if (resourceId.empty()) {
        callback(ResponseUtil::badRequest("资源ID不能为空"));
        return;
    }
    try {
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        bool success = service.deleteResource(resourceId);
        callback(success ? ResponseUtil::noContent() : ResponseUtil::notFound("资源不存在"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to delete resource: " << e.what();
        callback(ResponseUtil::internalError("删除资源失败"));
    }
}

void SafeHarborController::getResources(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    try {
        std::string type = req->getParameter("type");
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        callback(ResponseUtil::success(service.getResources(type)));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get resources: " << e.what();
        callback(ResponseUtil::internalError("获取资源列表失败"));
    }
}

// 用户求助记录
void SafeHarborController::recordAccess(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto json = req->getJsonObject();
    if (!json || !json->isMember("resource_id")) {
        callback(ResponseUtil::badRequest("缺少resource_id"));
        return;
    }
    try {
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        service.recordUserAccess(userId, (*json)["resource_id"].asString());
        callback(ResponseUtil::success("访问已记录"));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to record access: " << e.what();
        callback(ResponseUtil::internalError("记录访问失败"));
    }
}

void SafeHarborController::getAccessHistory(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    try {
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        callback(ResponseUtil::success(service.getUserAccessHistory(userId)));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get access history: " << e.what();
        callback(ResponseUtil::internalError("获取访问记录失败"));
    }
}

// 情绪推荐
void SafeHarborController::recommendResources(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    try {
        std::string userId = extractUserId(req);
        std::string emotion = req->getParameter("emotion");
        if (emotion.empty()) emotion = "neutral";
        auto& service = heartlake::infrastructure::SafeHarborService::getInstance();
        callback(ResponseUtil::success(service.recommendByEmotion(userId, emotion)));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to recommend resources: " << e.what();
        callback(ResponseUtil::internalError("获取推荐资源失败"));
    }
}
