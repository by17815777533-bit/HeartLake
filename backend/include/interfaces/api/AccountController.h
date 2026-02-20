/**
 * @file AccountController.h
 * @brief 账号管理控制器 - 企业级账号功能
 * @author 白洋
 * @date 2025-02-08
 * @copyright Copyright (c) 2025 HeartLake. All rights reserved.
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 账号管理HTTP控制器
 *
 * 提供完整的账号管理功能：
 * - 个人信息管理（头像、昵称、简介）
 * - 账号安全（密码、登录设备、安全日志）
 * - 隐私设置（可见性、黑名单）
 * - 数据管理（导出、删除）
 * - 统计信息（活跃度、内容统计）
 */
class AccountController : public drogon::HttpController<AccountController> {
public:
  METHOD_LIST_BEGIN

  // ==================== 个人信息管理 ====================

  /// 获取完整账号信息
  ADD_METHOD_TO(AccountController::getAccountInfo, "/api/account/info", Get);

  /// 更新头像
  ADD_METHOD_TO(AccountController::updateAvatar, "/api/account/avatar", Post);

  /// 更新个人资料
  ADD_METHOD_TO(AccountController::updateProfile, "/api/account/profile", Put);

  /// 获取账号统计数据
  ADD_METHOD_TO(AccountController::getAccountStats, "/api/account/stats", Get);

  // ==================== 账号安全 ====================

  /// 获取登录设备列表
  ADD_METHOD_TO(AccountController::getLoginDevices, "/api/account/devices", Get);

  /// 移除登录设备
  ADD_METHOD_TO(AccountController::removeDevice, "/api/account/devices/{1}", Delete);

  /// 获取登录日志
  ADD_METHOD_TO(AccountController::getLoginLogs, "/api/account/login-logs", Get);

  /// 获取安全事件
  ADD_METHOD_TO(AccountController::getSecurityEvents, "/api/account/security-events", Get);

  /// 修改密码（需要旧密码）
  ADD_METHOD_TO(AccountController::changePasswordSecure, "/api/account/change-password", Post);

  /// 绑定邮箱
  ADD_METHOD_TO(AccountController::bindEmail, "/api/account/bind-email", Post);

  /// 解绑邮箱
  ADD_METHOD_TO(AccountController::unbindEmail, "/api/account/unbind-email", Post);

  // ==================== 隐私设置 ====================

  /// 获取隐私设置
  ADD_METHOD_TO(AccountController::getPrivacySettings, "/api/account/privacy", Get);

  /// 更新隐私设置
  ADD_METHOD_TO(AccountController::updatePrivacySettings, "/api/account/privacy", Put);

  /// 获取黑名单
  ADD_METHOD_TO(AccountController::getBlockedUsers, "/api/account/blocked-users", Get);

  /// 拉黑用户
  ADD_METHOD_TO(AccountController::blockUser, "/api/account/block/{1}", Post);

  /// 取消拉黑
  ADD_METHOD_TO(AccountController::unblockUser, "/api/account/unblock/{1}", Delete);

  // ==================== 数据管理 ====================

  /// 导出个人数据
  ADD_METHOD_TO(AccountController::exportData, "/api/account/export", Post);

  /// 获取导出任务状态
  ADD_METHOD_TO(AccountController::getExportStatus, "/api/account/export/{1}", Get);

  /// 注销账号（软删除）
  ADD_METHOD_TO(AccountController::deactivateAccount, "/api/account/deactivate", Post);

  /// 永久删除账号
  ADD_METHOD_TO(AccountController::deleteAccountPermanently, "/api/account/delete-permanent", Post);

  METHOD_LIST_END

  // ==================== 个人信息管理方法 ====================

  void getAccountInfo(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void updateAvatar(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void updateProfile(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void getAccountStats(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  // ==================== 账号安全方法 ====================

  void getLoginDevices(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  void removeDevice(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &sessionId);

  void getLoginLogs(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void getSecurityEvents(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

  void changePasswordSecure(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);

  void bindEmail(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);

  void unbindEmail(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  // ==================== 隐私设置方法 ====================

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

  // ==================== 数据管理方法 ====================

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
