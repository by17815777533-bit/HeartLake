/**
 * 差分隐私统计控制器 - 隐私保护的情绪聚合查询
 *
 * 基于 DifferentialPrivacyEngine 实现的隐私保护统计接口。
 * 所有返回的聚合数据均经过 Laplace/Gaussian 噪声注入，
 * 满足 (ε, δ)-差分隐私保证，防止个体用户情绪被反推。
 *
 * 理论基础参考 FedMultiEmo (ACL 2024) 和 FED-PsyAU (EMNLP 2024)
 * 中的联邦差分隐私聚合机制。
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
 */
#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 差分隐私统计 HTTP 控制器
 *
 * @details 路由表：
 * - GET /api/lake/privacy-stats  — 经 DP 保护的湖面情绪分布统计
 * - GET /api/lake/privacy-report — 隐私预算消耗报告
 */
class PrivacyController : public drogon::HttpController<PrivacyController> {
public:
    METHOD_LIST_BEGIN

    /// 差分隐私保护的湖面情绪统计
    ADD_METHOD_TO(PrivacyController::getPrivacyStats, "/api/lake/privacy-stats", Get, "heartlake::filters::SecurityAuditFilter");

    /// 隐私预算消耗报告
    ADD_METHOD_TO(PrivacyController::getPrivacyReport, "/api/lake/privacy-report", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    /**
     * @brief 获取经差分隐私保护的情绪聚合统计
     * @details GET /api/lake/privacy-stats?epsilon=1.0
     *
     * 返回各情绪类别的用户数分布，数值经 Laplace 噪声注入。
     * epsilon 越小隐私保护越强，但统计精度越低。
     *
     * @param req HTTP 请求，可选 query 参数 epsilon（默认 1.0）
     * @param callback 响应回调
     * @return 噪声化的情绪分布 + 置信区间
     */
    void getPrivacyStats(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取隐私预算消耗报告
     * @details GET /api/lake/privacy-report
     *
     * 返回当前已消耗的 ε 总量、剩余预算、查询次数、
     * 预算消耗速率以及预计耗尽时间。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getPrivacyReport(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
