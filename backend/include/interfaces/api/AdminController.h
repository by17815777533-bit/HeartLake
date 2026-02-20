/**
 * @file AdminController.h
 * @brief AdminController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/AdminAuthFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 管理员相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class AdminController : public drogon::HttpController<AdminController> {
public:
    METHOD_LIST_BEGIN
    
    ADD_METHOD_TO(AdminController::login, "/api/admin/login", Post, Options);

    ADD_METHOD_TO(AdminController::logout, "/api/admin/logout", Post, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminController::getInfo, "/api/admin/info", Get, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminController::getTrendingTopics, "/api/admin/stats/trending-topics", Get, Options, "heartlake::filters::AdminAuthFilter");
    
    ADD_METHOD_TO(AdminController::getRealtimeStats, "/api/admin/stats/realtime", Get, Options, "heartlake::filters::AdminAuthFilter");
    
    ADD_METHOD_TO(AdminController::getDashboardStats, "/api/admin/stats/dashboard", Get, Options, "heartlake::filters::AdminAuthFilter");
    
    ADD_METHOD_TO(AdminController::getUserGrowthStats, "/api/admin/stats/user-growth", Get, Options, "heartlake::filters::AdminAuthFilter");
    
    ADD_METHOD_TO(AdminController::getMoodDistribution, "/api/admin/stats/mood-distribution", Get, Options, "heartlake::filters::AdminAuthFilter");
    
    ADD_METHOD_TO(AdminController::getActiveTimeStats, "/api/admin/stats/active-time", Get, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminController::getHighRiskUsers, "/api/admin/risk/high-risk-users", Get, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminController::getHighRiskEvents, "/api/admin/risk/events", Get, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminController::getUserRiskHistory, "/api/admin/risk/user/{user_id}/history", Get, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminController::handleRiskEvent, "/api/admin/risk/event/{event_id}/handle", Post, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminController::getSecurityAudit, "/api/admin/security/audit", Get, Options, "heartlake::filters::AdminAuthFilter");

    METHOD_LIST_END
    
    void login(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

    void logout(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

    void getInfo(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    void getTrendingTopics(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getRealtimeStats(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getDashboardStats(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getUserGrowthStats(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getMoodDistribution(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getActiveTimeStats(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

    void getHighRiskUsers(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    void getHighRiskEvents(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    void getUserRiskHistory(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback,
                           const std::string &user_id);

    void handleRiskEvent(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        const std::string &event_id);

    void getSecurityAudit(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
