/**
 * 举报控制器 - 用户内容举报与查询
 *
 * 提供用户对违规内容（石头、纸船、用户）的举报入口，
 * 以及个人举报记录的查询功能。举报提交后进入管理后台
 * 的审核队列，由 AdminManagementController 处理。
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
 * @brief 用户举报 HTTP 控制器
 *
 * @details 提供两个端点：
 * - POST /api/reports — 提交举报（需登录）
 * - GET  /api/reports/my — 查询自己提交的举报记录（需登录）
 *
 * 举报类型支持：违规内容、骚扰行为、不当言论等，
 * 具体类型由请求体中的 reason 字段指定。
 */
class ReportController : public drogon::HttpController<ReportController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(ReportController::createReport, "/api/reports", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(ReportController::getMyReports, "/api/reports/my", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    /**
     * @brief 提交举报
     * @details POST /api/reports
     *
     * 请求体:
     * @code
     * {
     *   "target_id": "被举报对象ID",
     *   "target_type": "stone|boat|user",
     *   "reason": "举报原因描述"
     * }
     * @endcode
     *
     * @param req HTTP 请求（需携带 PASETO 令牌）
     * @param callback 响应回调
     * @retval 200 举报提交成功
     * @retval 400 缺少必要字段或参数非法
     */
    void createReport(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 查询我提交的举报记录
     * @details GET /api/reports/my?page=1&page_size=20
     *
     * @param req HTTP 请求（需携带 PASETO 令牌）
     * @param callback 响应回调
     * @return 分页举报列表，包含举报状态（pending/handled/dismissed）
     */
    void getMyReports(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
