/**
 * 账号管理控制器 - 个人信息、安全设备、隐私设置与数据管理
 *
 * 提供用户账号全生命周期管理：
 * - 个人资料查看与编辑（昵称、头像、签名等）
 * - 登录设备管理与安全事件审计
 * - 隐私偏好设置与用户拉黑
 * - 数据导出（GDPR 合规）与账号注销/永久删除
 *
 * 所有端点均需 PASETO 令牌认证（SecurityAuditFilter）。
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 账号管理 HTTP 控制器
 *
 * @details 覆盖四大功能域：个人信息、设备安全、隐私设置、数据管理。
 * 永久删除操作在事务中按依赖顺序级联清理所有关联数据。
 */
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

  /// GET /api/account/info - 获取当前用户的完整个人资料
  void getAccountInfo(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  /// POST /api/account/avatar - 更新头像，请求体: { "avatar_url": "..." }
  void updateAvatar(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * PUT /api/account/profile - 更新个人资料
   *
   * 支持字段: nickname, bio, avatar_url/avatar, gender, birthday, location,
   * email（均可选）。昵称会经过 HTML 转义和长度校验。
   */
  void updateProfile(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  /// GET /api/account/stats - 获取账号统计（石头数、好友数、收到的涟漪数）
  void getAccountStats(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  /// GET /api/account/devices - 获取当前活跃的登录设备列表
  void getLoginDevices(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  /// DELETE /api/account/devices/{sessionId} - 移除指定登录设备
  void removeDevice(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &sessionId);

  /// GET /api/account/login-logs - 获取最近50条登录日志
  void getLoginLogs(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  /// GET /api/account/security-events - 获取最近50条安全事件
  void getSecurityEvents(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

  /// GET /api/account/privacy - 获取隐私设置（可见性、在线状态、好友请求等）
  void getPrivacySettings(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * PUT /api/account/privacy - 更新隐私设置
   *
   * 支持多种类型兼容（bool/int/string），使用 UPSERT 语义。
   */
  void updatePrivacySettings(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback);

  /// GET /api/account/blocked-users - 获取拉黑用户列表
  void getBlockedUsers(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  /// POST /api/account/block/{targetUserId} - 拉黑用户（幂等，不能拉黑自己）
  void blockUser(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback,
                 const std::string &targetUserId);

  /// DELETE /api/account/unblock/{targetUserId} - 取消拉黑
  void unblockUser(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   const std::string &targetUserId);

  /// POST /api/account/export - 创建数据导出任务（异步处理）
  void exportData(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

  /// GET /api/account/export/{taskId} - 查询数据导出任务状态
  void getExportStatus(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       const std::string &taskId);

  /**
   * POST /api/account/deactivate - 注销账号（30天内可恢复）
   *
   * 请求体可省略；如提供则兼容 `confirmation=DEACTIVATE/DELETE`。
   */
  void deactivateAccount(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * POST /api/account/delete-permanent - 永久删除账号及所有关联数据
   *
   * 请求体可省略；如提供则兼容 `confirmation=DELETE`。
   * 在事务中按依赖顺序级联删除主链路关联数据，不可恢复。
   */
  void deleteAccountPermanently(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
