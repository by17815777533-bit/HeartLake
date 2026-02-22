/**
 * @file PrivacyController.h
 * @brief 差分隐私统计控制器
 *
 * 提供隐私保护的情绪聚合统计接口
 * 基于 FedMultiEmo / FED-PsyAU 论文的差分隐私机制
 */
#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

class PrivacyController : public drogon::HttpController<PrivacyController> {
public:
    METHOD_LIST_BEGIN

    /// 差分隐私保护的湖面情绪统计
    ADD_METHOD_TO(PrivacyController::getPrivacyStats, "/api/lake/privacy-stats", Get, "heartlake::filters::SecurityAuditFilter");

    /// 隐私预算消耗报告
    ADD_METHOD_TO(PrivacyController::getPrivacyReport, "/api/lake/privacy-report", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    void getPrivacyStats(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    void getPrivacyReport(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
