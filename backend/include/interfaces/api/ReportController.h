/**
 * ReportController 模块接口定义
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * 举报相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class ReportController : public drogon::HttpController<ReportController> {
public:
    METHOD_LIST_BEGIN
    
    ADD_METHOD_TO(ReportController::createReport, "/api/reports", Post, "heartlake::filters::SecurityAuditFilter");
    
    ADD_METHOD_TO(ReportController::getMyReports, "/api/reports/my", Get, "heartlake::filters::SecurityAuditFilter");
    
    METHOD_LIST_END
    
    void createReport(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getMyReports(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
