/**
 * VIP控制器实现
 */

#include "interfaces/api/VIPController.h"
#include "infrastructure/services/VIPService.h"
#include "utils/ResponseUtil.h"
#include "utils/PasetoUtil.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"
#include <drogon/drogon.h>

using namespace heartlake::controllers;
using namespace heartlake::services;
using namespace heartlake::utils;

void VIPController::getVIPStatus(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto userId = Validator::getUserId(req);
        if (!userId) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 获取VIP状态
        Json::Value status = VIPService::getVIPStatus(*userId);

        callback(ResponseUtil::success(status));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error getting VIP status: " << e.what();
        callback(ResponseUtil::error(500, "获取VIP状态失败"));
    }
}

void VIPController::getPrivileges(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto userId = Validator::getUserId(req);
        if (!userId) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 获取用户权益列表
        auto privileges = VIPService::getUserPrivileges(*userId);

        // 直接返回权益列表
        Json::Value privilegeList(Json::arrayValue);
        for (const auto& priv : privileges) {
            privilegeList.append(priv);
        }

        Json::Value response;
        response["privileges"] = privilegeList;
        response["total"] = static_cast<int>(privilegeList.size());

        callback(ResponseUtil::success(response));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error getting privileges: " << e.what();
        callback(ResponseUtil::error(500, "获取权益列表失败"));
    }
}

void VIPController::checkFreeCounseling(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto userId = Validator::getUserId(req);
        if (!userId) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 检查免费咨询额度
        bool hasQuota = VIPService::hasFreeCounselingQuota(*userId);

        Json::Value response;
        response["has_quota"] = hasQuota;

        if (hasQuota) {
            response["message"] = "您有1次免费心理咨询机会";
        } else {
            response["message"] = "您已使用过免费心理咨询额度";
        }

        callback(ResponseUtil::success(response));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error checking free counseling: " << e.what();
        callback(ResponseUtil::error(500, "检查咨询额度失败"));
    }
}

void VIPController::bookCounseling(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto userId = Validator::getUserId(req);
        if (!userId) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 解析请求体
        auto jsonBody = req->getJsonObject();
        if (!jsonBody) {
            callback(ResponseUtil::error(400, "请求体格式错误"));
            return;
        }

        // 获取预约时间
        if (!jsonBody->isMember("appointment_time")) {
            callback(ResponseUtil::error(400, "缺少预约时间"));
            return;
        }

        std::string appointmentTime = (*jsonBody)["appointment_time"].asString();
        bool isFreeVIP = (*jsonBody).get("is_free_vip", false).asBool();

        // 预约咨询
        std::string appointmentId = VIPService::bookCounseling(*userId, appointmentTime, isFreeVIP);

        if (appointmentId.empty()) {
            callback(ResponseUtil::error(400, "预约失败，请检查您的VIP权益"));
            return;
        }

        Json::Value response;
        response["appointment_id"] = appointmentId;
        response["appointment_time"] = appointmentTime;
        response["is_free_vip"] = isFreeVIP;
        response["message"] = "预约成功";

        callback(ResponseUtil::success(response));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error booking counseling: " << e.what();
        callback(ResponseUtil::error(500, "预约失败"));
    }
}

void VIPController::getAICommentFrequency(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto userId = Validator::getUserId(req);
        if (!userId) {
            callback(ResponseUtil::unauthorized("未登录"));
            return;
        }

        // 获取AI评论频率
        float frequency = VIPService::getAICommentFrequency(*userId);

        Json::Value response;
        response["frequency_hours"] = frequency;
        response["frequency_minutes"] = static_cast<int>(frequency * 60);

        if (frequency <= 0.5f) {
            response["message"] = "VIP用户享受高频AI评论，每30分钟一次";
        } else {
            response["message"] = "普通用户AI评论频率为每2小时一次";
        }

        callback(ResponseUtil::success(response));
    } catch (const std::exception& e) {
        LOG_ERROR << "Error getting AI comment frequency: " << e.what();
        callback(ResponseUtil::error(500, "获取AI评论频率失败"));
    }
}
