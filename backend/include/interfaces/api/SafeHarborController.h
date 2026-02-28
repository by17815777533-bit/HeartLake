/**
 * 安全港湾控制器 - 心理危机干预资源管理
 *
 * 为处于心理困境的用户提供即时可用的危机干预资源：
 * - 心理援助热线（全国/地区）
 * - 自助工具（呼吸练习、正念引导等）
 * - 温暖提示语
 * - 专业机构资源推荐
 *
 * 读取端点无需认证（危机资源应可匿名访问），
 * 写入端点需要管理员 PASETO 令牌鉴权，
 * 访问记录端点经 SecurityAuditFilter 校验。
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"
#include "infrastructure/filters/AdminAuthFilter.h"

namespace heartlake::controllers {

/**
 * @brief 安全港湾 HTTP 控制器
 *
 * @details 端点权限分层设计：
 * - 公开（无需登录）：热线查询、自助工具、温暖提示、资源列表、智能推荐
 * - 管理员：资源的增删改
 * - 已登录用户：访问记录与历史查询
 *
 * @note 公开端点不设认证门槛，确保用户在任何状态下都能获取帮助。
 */
class SafeHarborController : public drogon::HttpController<SafeHarborController> {
public:
    METHOD_LIST_BEGIN
    // ---- 公开读取端点（危机资源应可匿名访问） ----
    ADD_METHOD_TO(SafeHarborController::getHotlines, "/api/safe-harbor/hotlines", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getSelfHelpTools, "/api/safe-harbor/tools", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getWarmPrompt, "/api/safe-harbor/prompt", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::getResources, "/api/safe-harbor/resources", drogon::Get);
    ADD_METHOD_TO(SafeHarborController::recommendResources, "/api/safe-harbor/recommend", drogon::Get);
    // ---- 写入端点（需要管理员权限） ----
    ADD_METHOD_TO(SafeHarborController::addResource, "/api/safe-harbor/resources", drogon::Post, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(SafeHarborController::updateResource, "/api/safe-harbor/resources/{1}", drogon::Put, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(SafeHarborController::deleteResource, "/api/safe-harbor/resources/{1}", drogon::Delete, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(SafeHarborController::recordAccess, "/api/safe-harbor/access", drogon::Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(SafeHarborController::getAccessHistory, "/api/safe-harbor/access/history", drogon::Get, "heartlake::filters::SecurityAuditFilter");
    METHOD_LIST_END

    /// GET /api/safe-harbor/hotlines — 获取心理援助热线列表（按地区分组）
    void getHotlines(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /// GET /api/safe-harbor/tools — 获取自助工具列表（呼吸练习、正念引导等）
    void getSelfHelpTools(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /// GET /api/safe-harbor/prompt — 获取随机温暖提示语
    void getWarmPrompt(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief 添加危机干预资源（管理员）
     * @details POST /api/safe-harbor/resources
     *
     * 请求体: { "title": "...", "type": "hotline|tool|article", "content": "...", "region": "..." }
     */
    void addResource(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief 更新危机干预资源（管理员）
     * @details PUT /api/safe-harbor/resources/{resourceId}
     */
    void updateResource(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& resourceId);

    /**
     * @brief 删除危机干预资源（管理员）
     * @details DELETE /api/safe-harbor/resources/{resourceId}
     */
    void deleteResource(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& resourceId);

    /// GET /api/safe-harbor/resources — 获取全部危机干预资源列表
    void getResources(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief 记录用户访问安全港湾（用于统计和风险评估）
     * @details POST /api/safe-harbor/access
     *
     * 频繁访问安全港湾可能表明用户处于心理困境，
     * 系统会据此触发 PsychologicalRiskAssessment 评估。
     */
    void recordAccess(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /// GET /api/safe-harbor/access/history — 获取当前用户的访问历史
    void getAccessHistory(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief 智能推荐危机干预资源
     * @details GET /api/safe-harbor/recommend?mood=sad&severity=high
     *
     * 根据用户当前情绪状态和严重程度，推荐最合适的资源。
     */
    void recommendResources(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace heartlake::controllers
