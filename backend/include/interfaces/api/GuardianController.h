/**
 * @file GuardianController.h
 * @brief 守望者控制器 - 灯火转赠与激励系统API
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake::controllers {

class GuardianController : public HttpController<GuardianController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(GuardianController::getStats, "/api/guardian/stats", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::getStats, "/api/guardian", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::transferLamp, "/api/guardian/transfer-lamp", Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::getEmotionInsights, "/api/guardian/insights", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::chat, "/api/guardian/chat", Post, "heartlake::filters::SecurityAuditFilter", "heartlake::filters::RateLimitFilter");

    METHOD_LIST_END

    void getStats(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void transferLamp(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void getEmotionInsights(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void chat(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};

} // namespace heartlake::controllers
