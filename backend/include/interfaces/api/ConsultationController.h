/**
 * @file ConsultationController.h
 * @brief 咨询室控制器 - E2EE端到端加密通信
 * Created by engineer-4
 */

#pragma once

#include <drogon/HttpController.h>
#include "utils/E2EEncryption.h"

namespace heartlake {
namespace api {

class ConsultationController : public drogon::HttpController<ConsultationController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ConsultationController::createSession, "/api/consultation/session", drogon::Post);
    ADD_METHOD_TO(ConsultationController::exchangeKey, "/api/consultation/key-exchange", drogon::Post);
    ADD_METHOD_TO(ConsultationController::sendMessage, "/api/consultation/message", drogon::Post);
    ADD_METHOD_TO(ConsultationController::getMessages, "/api/consultation/messages/{sessionId}", drogon::Get);
    METHOD_LIST_END

    void createSession(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void exchangeKey(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void sendMessage(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getMessages(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& sessionId);
};

} // namespace api
} // namespace heartlake
