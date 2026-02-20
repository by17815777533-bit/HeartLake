/**
 * @file AdminManagementController.h
 * @brief AdminManagementController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/AdminAuthFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 管理员管理相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class AdminManagementController : public drogon::HttpController<AdminManagementController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminManagementController::getUsers, "/api/admin/users", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getUserDetail, "/api/admin/users/{1}", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::updateUserStatus, "/api/admin/users/{1}/status", Put, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::banUser, "/api/admin/users/{1}/ban", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::unbanUser, "/api/admin/users/{1}/unban", Post, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getStones, "/api/admin/stones", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getStoneDetail, "/api/admin/stones/{1}", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::deleteStone, "/api/admin/stones/{1}", Delete, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getBoats, "/api/admin/boats", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::deleteBoat, "/api/admin/boats/{1}", Delete, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getPendingModeration, "/api/admin/moderation/pending", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::approveContent, "/api/admin/moderation/{1}/approve", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::rejectContent, "/api/admin/moderation/{1}/reject", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getModerationHistory, "/api/admin/moderation/history", Get, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getReports, "/api/admin/reports", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getReportDetail, "/api/admin/reports/{1}", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::handleReport, "/api/admin/reports/{1}/handle", Post, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getSensitiveWords, "/api/admin/sensitive-words", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::addSensitiveWord, "/api/admin/sensitive-words", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::updateSensitiveWord, "/api/admin/sensitive-words/{1}", Put, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::deleteSensitiveWord, "/api/admin/sensitive-words/{1}", Delete, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getSystemConfig, "/api/admin/config", Get, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::updateSystemConfig, "/api/admin/config", Put, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::broadcastMessage, "/api/admin/broadcast", Post, Options, "heartlake::filters::AdminAuthFilter");
    ADD_METHOD_TO(AdminManagementController::getBroadcastHistory, "/api/admin/broadcast/history", Get, Options, "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(AdminManagementController::getOperationLogs, "/api/admin/logs", Get, Options, "heartlake::filters::AdminAuthFilter");
    METHOD_LIST_END

    void getUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getUserDetail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);
    void updateUserStatus(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);
    void banUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);
    void unbanUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &userId);

    void getStones(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getStoneDetail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &stoneId);
    void deleteStone(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &stoneId);

    void getBoats(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteBoat(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &boatId);

    void getPendingModeration(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void approveContent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &moderationId);
    void rejectContent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &moderationId);
    void getModerationHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    void getReports(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getReportDetail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &reportId);
    void handleReport(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &reportId);

    void getSensitiveWords(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void addSensitiveWord(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void updateSensitiveWord(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &id);
    void deleteSensitiveWord(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &id);

    void getSystemConfig(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void updateSystemConfig(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    void broadcastMessage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getBroadcastHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    void getOperationLogs(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
