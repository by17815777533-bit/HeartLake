/**
 * @file SafeHarborController.h
 * @brief 安全港湾危机干预控制器
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

namespace heartlake::controllers {

class SafeHarborController : public drogon::HttpController<SafeHarborController> {
public:
    METHOD_LIST_BEGIN
    // 公开读取端点（危机资源应可匿名访问）
    ADD_METHOD_TO(SafeHarborController::getHotlines, "/api/safe-harbor/hotlines", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getSelfHelpTools, "/api/safe-harbor/tools", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getWarmPrompt, "/api/safe-harbor/prompt", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getResources, "/api/safe-harbor/resources", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::recommendResources, "/api/safe-harbor/recommend", drogon::Get);
    // 写入端点需要认证
    ADD_METHOD_TO(SafeHarborController::addResource, "/api/safe-harbor/resources", drogon::Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(SafeHarborController::updateResource, "/api/safe-harbor/resources/{1}", drogon::Put, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(SafeHarborController::deleteResource, "/api/safe-harbor/resources/{1}", drogon::Delete, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(SafeHarborController::recordAccess, "/api/safe-harbor/access", drogon::Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(SafeHarborController::getAccessHistory, "/api/safe-harbor/access/history", drogon::Get, "heartlake::filters::SecurityAuditFilter");
    METHOD_LIST_END

    void getHotlines(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getSelfHelpTools(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getWarmPrompt(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void addResource(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void updateResource(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& resourceId);
    void deleteResource(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& resourceId);
    void getResources(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void recordAccess(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getAccessHistory(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void recommendResources(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace heartlake::controllers
