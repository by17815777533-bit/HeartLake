/**
 * 账号管理控制器
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

class AccountController : public drogon::HttpController<AccountController> {
public:
  METHOD_LIST_BEGIN

  // 个人信息
  ADD_METHOD_TO(AccountController::getAccountInfo, "/api/account/info", Get, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::updateAvatar, "/api/account/avatar", Post, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::updateProfile, "/api/account/profile", Put, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::getAccountStats, "/api/account/stats", Get, "heartlake::filters::SecurityAuditFilter");

  // 设备管理
  ADD_METHOD_TO(AccountController::getLoginDevices, "/api/account/devices", Get, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::removeDevice, "/api/account/devices/{1}", Delete, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::getLoginLogs, "/api/account/login-logs", Get, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::getSecurityEvents, "/api/account/security-events", Get, "heartlake::filters::SecurityAuditFilter");

  // 隐私设置
  ADD_METHOD_TO(AccountController::getPrivacySettings, "/api/account/privacy", Get, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::updatePrivacySettings, "/api/account/privacy", Put, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::getBlockedUsers, "/api/account/blocked-users", Get, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::blockUser, "/api/account/block/{1}", Post, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::unblockUser, "/api/account/unblock/{1}", Delete, "heartlake::filters::SecurityAuditFilter");

  // 数据管理
  ADD_METHOD_TO(AccountController::exportData, "/api/account/export", Post, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::getExportStatus, "/api/account/export/{1}", Get, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::deactivateAccount, "/api/account/deactivate", Post, "heartlake::filters::SecurityAuditFilter");
  ADD_METHOD_TO(AccountController::deleteAccountPermanently, "/api/account/delete-permanent", Post, "heartlake::filters::SecurityAuditFilter");

  METHOD_LIST_END

  void getAccountInfo(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void updateAvatar(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void updateProfile(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void getAccountStats(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  void getLoginDevices(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  void removeDevice(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &sessionId);

  void getLoginLogs(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void getSecurityEvents(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

  void getPrivacySettings(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

  void updatePrivacySettings(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback);

  void getBlockedUsers(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  void blockUser(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback,
                 const std::string &targetUserId);

  void unblockUser(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   const std::string &targetUserId);

  void exportData(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

  void getExportStatus(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       const std::string &taskId);

  void deactivateAccount(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

  void deleteAccountPermanently(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
