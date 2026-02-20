/**
 * @file PrivacyController.cpp
 * @brief 差分隐私统计控制器实现
 */

#include "interfaces/api/PrivacyController.h"
#include "infrastructure/privacy/DifferentialPrivacyEngine.h"
#include "utils/ResponseUtil.h"

using namespace heartlake::controllers;
using namespace heartlake::utils;

void PrivacyController::getPrivacyStats(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        // 可选参数：epsilon（隐私预算），默认2.0
        double epsilon = 2.0;
        if (auto e = req->getParameter("epsilon"); !e.empty()) {
            try { epsilon = std::stod(e); } catch (...) {}
        }
        // 限制 epsilon 范围 [0.1, 10.0]
        epsilon = std::clamp(epsilon, 0.1, 10.0);

        auto& engine = heartlake::privacy::DifferentialPrivacyEngine::getInstance();
        auto weather = engine.getPrivateLakeWeather(epsilon);

        callback(ResponseUtil::success(weather, "差分隐私保护统计"));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getPrivacyStats: " << e.what();
        callback(ResponseUtil::internalError("隐私统计查询失败"));
    }
}

void PrivacyController::getPrivacyReport(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
) {
    try {
        auto& engine = heartlake::privacy::DifferentialPrivacyEngine::getInstance();
        auto report = engine.getPrivacyReport();

        callback(ResponseUtil::success(report, "隐私预算报告"));

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getPrivacyReport: " << e.what();
        callback(ResponseUtil::internalError("隐私报告查询失败"));
    }
}
