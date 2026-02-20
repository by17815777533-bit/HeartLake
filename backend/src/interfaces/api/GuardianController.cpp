/**
 * @file GuardianController.cpp
 * @brief 守望者控制器实现
 */

#include "interfaces/api/GuardianController.h"
#include "infrastructure/services/GuardianIncentiveService.h"
#include "infrastructure/ai/DualMemoryRAG.h"
#include "utils/ResponseUtil.h"
#include "utils/PasetoUtil.h"

using namespace heartlake::controllers;
using namespace heartlake::infrastructure;
using namespace heartlake::utils;

static std::string extractUserId(const HttpRequestPtr& req) {
    try {
        return req->getAttributes()->get<std::string>("user_id");
    } catch (...) {
        return "";
    }
}

void GuardianController::getStats(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    try {
        auto stats = GuardianIncentiveService::getInstance().getGuardianStats(userId);
        Json::Value response;
        response["resonance_points"] = stats.totalResonancePoints;
        response["quality_ripples"] = stats.qualityRipples;
        response["warm_boats"] = stats.warmBoats;
        response["is_guardian"] = stats.isGuardian;
        response["can_transfer_lamp"] = stats.canTransferLamp;

        callback(ResponseUtil::success(response));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get guardian stats: " << e.what();
        callback(ResponseUtil::internalError("获取守望者数据失败"));
    }
}

void GuardianController::transferLamp(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    auto jsonBody = req->getJsonObject();
    if (!jsonBody || !jsonBody->isMember("to_user_id")) {
        callback(ResponseUtil::error(400, "缺少接收者ID"));
        return;
    }

    try {
        std::string toUserId = (*jsonBody)["to_user_id"].asString();
        bool success = GuardianIncentiveService::getInstance().transferLamp(userId, toUserId);

        if (success) {
            callback(ResponseUtil::success("灯火转赠成功"));
        } else {
            callback(ResponseUtil::error(400, "转赠失败，请检查守望者资格"));
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to transfer lamp: " << e.what();
        callback(ResponseUtil::internalError("灯火转赠失败"));
    }
}

void GuardianController::getEmotionInsights(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::string userId = extractUserId(req);
    if (userId.empty()) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }

    try {
        auto& dualMemory = heartlake::ai::DualMemoryRAG::getInstance();
        auto insights = dualMemory.getEmotionInsights(userId);
        callback(ResponseUtil::success(insights));
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to get emotion insights for user " << userId << ": " << e.what();
        callback(ResponseUtil::internalError("获取情绪洞察失败"));
    }
}
