/**
 * 管理后台控制器 - 管理员认证与运营数据统计
 *
 * 提供管理后台的核心功能：
 * - 管理员登录/登出/信息查询
 * - Dashboard 统计面板（用户增长、情绪分布、活跃时段、热门话题）
 * - 实时运营数据（在线人数、今日投石量等）
 * - 心理风险预警（高风险用户、风险事件、处置操作）
 * - 安全审计日志查询
 *
 * 登录接口无需认证；其余端点均需 AdminAuthFilter（PASETO 令牌）鉴权。
 * 所有端点支持 OPTIONS 预检请求（CORS）。
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/AdminAuthFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 管理后台 HTTP 控制器
 *
 * @details 路由分组：
 * - 认证：login / logout / getInfo
 * - 统计面板：dashboard / user-growth / mood-distribution / active-time / trending-topics / realtime
 * - 风险管理：high-risk-users / events / user history / handle event
 * - 安全审计：security audit log
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

    // ---- 认证 ----

    /**
     * @brief 管理员登录
     * @details POST /api/admin/login
     *
     * 请求体: { "username": "admin", "password": "..." }
     * 成功返回 PASETO 令牌和管理员信息。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void login(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

    /// POST /api/admin/logout — 管理员登出，使当前令牌失效
    void logout(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/info — 获取当前管理员信息（角色、权限等）
    void getInfo(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    // ---- 统计面板 ----

    /// GET /api/admin/stats/trending-topics — 热门话题/标签排行
    void getTrendingTopics(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/stats/realtime — 实时运营数据（在线人数、今日投石量等）
    void getRealtimeStats(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/stats/dashboard — Dashboard 综合统计（总用户、总石头、总互动等）
    void getDashboardStats(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/stats/user-growth?days=30 — 用户增长趋势
    void getUserGrowthStats(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/stats/mood-distribution — 全站情绪分布饼图数据
    void getMoodDistribution(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/stats/active-time — 24小时活跃时段分布
    void getActiveTimeStats(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

    // ---- 风险管理 ----

    /// GET /api/admin/risk/high-risk-users — 获取高风险用户列表
    void getHighRiskUsers(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/risk/events — 获取风险事件列表
    void getHighRiskEvents(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取指定用户的风险历史
     * @details GET /api/admin/risk/user/{user_id}/history
     * @param user_id 用户ID
     */
    void getUserRiskHistory(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback,
                           const std::string &user_id);

    /**
     * @brief 处置风险事件
     * @details POST /api/admin/risk/event/{event_id}/handle
     *
     * 请求体: { "action": "warn|ban|dismiss", "note": "处置备注" }
     *
     * @param event_id 风险事件ID
     */
    void handleRiskEvent(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        const std::string &event_id);

    // ---- 安全审计 ----

    /// GET /api/admin/security/audit?page=1&page_size=50 — 安全审计日志
    void getSecurityAudit(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
