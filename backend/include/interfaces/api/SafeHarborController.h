/**
 * @file SafeHarborController.h
 * @brief 安全港湾危机干预控制器
 */

#pragma once

#include <drogon/HttpController.h>

namespace heartlake::controllers {

class SafeHarborController : public drogon::HttpController<SafeHarborController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SafeHarborController::getHotlines, "/api/safe-harbor/hotlines", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getSelfHelpTools, "/api/safe-harbor/tools", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getWarmPrompt, "/api/safe-harbor/prompt", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::addResource, "/api/safe-harbor/resources", drogon::Post);
    ADD_METHOD_TO(SafeHarborController::updateResource, "/api/safe-harbor/resources/{1}", drogon::Put);
    ADD_METHOD_TO(SafeHarborController::deleteResource, "/api/safe-harbor/resources/{1}", drogon::Delete);
    ADD_METHOD_TO(SafeHarborController::getResources, "/api/safe-harbor/resources", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::recordAccess, "/api/safe-harbor/access", drogon::Post);
    ADD_METHOD_TO(SafeHarborController::getAccessHistory, "/api/safe-harbor/access/history", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::recommendResources, "/api/safe-harbor/recommend", drogon::Get);
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
