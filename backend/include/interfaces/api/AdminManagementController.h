/**
 * 管理后台运营控制器 - 用户/内容/审核/敏感词/系统配置全量管理
 *
 * 提供管理后台的日常运营功能，覆盖六大管理域：
 * - 用户管理：列表查询、详情、状态变更、封禁/解封
 * - 内容管理：石头和纸船的查看与删除
 * - 内容审核：待审核队列、通过/拒绝、审核历史
 * - 举报处理：举报列表、详情、处置操作
 * - 敏感词管理：CRUD 操作
 * - 系统配置：全局参数读取与修改
 * - 广播消息：全站公告推送与历史
 * - 操作日志：管理员操作审计
 *
 * 所有端点均需 AdminAuthFilter（PASETO 令牌）鉴权，
 * 并支持 OPTIONS 预检请求（CORS）。
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/AdminAuthFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 管理后台运营 HTTP 控制器
 *
 * @details 本控制器是管理后台最大的控制器，包含 20+ 个端点，
 * 按功能域分组：users / stones / boats / moderation / reports /
 * sensitive-words / config / broadcast / logs。
 */
class AdminManagementController : public drogon::HttpController<AdminManagementController> {
public:
    METHOD_LIST_BEGIN
    // ---- 用户管理 ----
    ADD_METHOD_TO(AdminManagementController::getUsers, "/api/admin/users", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getUserDetail, "/api/admin/users/{1}", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::updateUserStatus, "/api/admin/users/{1}/status", Put, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::banUser, "/api/admin/users/{1}/ban", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::unbanUser, "/api/admin/users/{1}/unban", Post, Options, "heartlake::filters::AdminAuthFilter");

    // ---- 内容管理（石头 + 纸船） ----
    ADD_METHOD_TO(AdminManagementController::getStones, "/api/admin/stones", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getStoneDetail, "/api/admin/stones/{1}", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::deleteStone, "/api/admin/stones/{1}", Delete, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getBoats, "/api/admin/boats", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::deleteBoat, "/api/admin/boats/{1}", Delete, Options, "heartlake::filters::AdminAuthFilter");

    // ---- 内容审核 ----
    ADD_METHOD_TO(AdminManagementController::getPendingModeration, "/api/admin/moderation/pending", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::approveContent, "/api/admin/moderation/{1}/approve", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::rejectContent, "/api/admin/moderation/{1}/reject", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getModerationHistory, "/api/admin/moderation/history", Get, Options, "heartlake::filters::AdminAuthFilter");

    // ---- 举报处理 ----
    ADD_METHOD_TO(AdminManagementController::getReports, "/api/admin/reports", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getReportDetail, "/api/admin/reports/{1}", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::handleReport, "/api/admin/reports/{1}/handle", Post, Options, "heartlake::filters::AdminAuthFilter");

    // ---- 敏感词管理 ----
    ADD_METHOD_TO(AdminManagementController::getSensitiveWords, "/api/admin/sensitive-words", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::addSensitiveWord, "/api/admin/sensitive-words", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::updateSensitiveWord, "/api/admin/sensitive-words/{1}", Put, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::deleteSensitiveWord, "/api/admin/sensitive-words/{1}", Delete, Options, "heartlake::filters::AdminAuthFilter");

    // ---- 系统配置 ----
    ADD_METHOD_TO(AdminManagementController::getSystemConfig, "/api/admin/config", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::updateSystemConfig, "/api/admin/config", Put, Options, "heartlake::filters::AdminAuthFilter");

    // ---- 广播消息 ----
    ADD_METHOD_TO(AdminManagementController::broadcastMessage, "/api/admin/broadcast", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getBroadcastHistory, "/api/admin/broadcast/history", Get, Options, "heartlake::filters::AdminAuthFilter");

    // ---- 操作日志 ----
    ADD_METHOD_TO(AdminManagementController::getOperationLogs, "/api/admin/logs", Get, Options, "heartlake::filters::AdminAuthFilter");
    METHOD_LIST_END

    // ==================== 用户管理 ====================

    /// GET /api/admin/users?page=1&page_size=20&keyword=xxx — 用户列表（支持搜索）
    void getUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/users/{userId} — 用户详情（含统计数据）
    void getUserDetail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);

    /// PUT /api/admin/users/{userId}/status — 更新用户状态
    void updateUserStatus(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);

    /**
     * @brief 封禁用户
     * @details POST /api/admin/users/{userId}/ban
     *
     * 请求体: { "reason": "封禁原因", "duration_hours": 24 }
     * duration_hours 为 0 表示永久封禁。
     */
    void banUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);

    /// POST /api/admin/users/{userId}/unban — 解封用户
    void unbanUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);

    // ==================== 内容管理 ====================

    /// GET /api/admin/stones?page=1&page_size=20 — 石头列表
    void getStones(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/stones/{stoneId} — 石头详情
    void getStoneDetail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &stoneId);

    /// DELETE /api/admin/stones/{stoneId} — 删除石头（级联清理涟漪和纸船）
    void deleteStone(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &stoneId);

    /// GET /api/admin/boats?page=1&page_size=20 — 纸船列表
    void getBoats(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// DELETE /api/admin/boats/{boatId} — 删除纸船
    void deleteBoat(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &boatId);

    // ==================== 内容审核 ====================

    /// GET /api/admin/moderation/pending — 待审核内容队列
    void getPendingModeration(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// POST /api/admin/moderation/{moderationId}/approve — 审核通过
    void approveContent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &moderationId);

    /// POST /api/admin/moderation/{moderationId}/reject — 审核拒绝
    void rejectContent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &moderationId);

    /// GET /api/admin/moderation/history — 审核历史记录
    void getModerationHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    // ==================== 举报处理 ====================

    /// GET /api/admin/reports?page=1&page_size=20&status=pending — 举报列表
    void getReports(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/reports/{reportId} — 举报详情
    void getReportDetail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &reportId);

    /**
     * @brief 处置举报
     * @details POST /api/admin/reports/{reportId}/handle
     *
     * 请求体: { "action": "warn|delete|ban|dismiss", "note": "处置说明" }
     */
    void handleReport(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &reportId);

    // ==================== 敏感词管理 ====================

    /// GET /api/admin/sensitive-words?page=1&page_size=50 — 敏感词列表
    void getSensitiveWords(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// POST /api/admin/sensitive-words — 添加敏感词
    void addSensitiveWord(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// PUT /api/admin/sensitive-words/{id} — 更新敏感词
    void updateSensitiveWord(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &id);

    /// DELETE /api/admin/sensitive-words/{id} — 删除敏感词
    void deleteSensitiveWord(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &id);

    // ==================== 系统配置 ====================

    /// GET /api/admin/config — 获取系统配置
    void getSystemConfig(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// PUT /api/admin/config — 更新系统配置
    void updateSystemConfig(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    // ==================== 广播消息 ====================

    /**
     * @brief 发送全站广播消息
     * @details POST /api/admin/broadcast
     *
     * 请求体: { "title": "标题", "content": "内容", "type": "notice|alert" }
     * 通过 WebSocket 实时推送给所有在线用户。
     */
    void broadcastMessage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    /// GET /api/admin/broadcast/history — 广播历史记录
    void getBroadcastHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    // ==================== 操作日志 ====================

    /// GET /api/admin/logs?page=1&page_size=50&action=ban — 管理员操作日志
    void getOperationLogs(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
