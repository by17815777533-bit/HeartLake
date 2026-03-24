/**
 * 用户控制器 - 匿名认证、账号恢复与用户信息查询
 *
 * HeartLake 采用匿名优先的认证模型：
 * - 匿名登录：无需注册，自动生成匿名身份和恢复密钥
 * - 密钥恢复：通过恢复密钥在新设备上找回账号
 * - Token 刷新：会话 refresh_token 与短期访问令牌协同续期
 *
 * 同时提供用户信息查询、搜索、个人资料编辑，
 * 以及情绪日历/热力图等数据可视化接口。
 *
 * 认证端点无需登录；用户信息端点部分公开、部分需登录。
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 用户认证与信息 HTTP 控制器
 *
 * @details 路由分组：
 * - 认证：anonymous / recover / refresh / delete-account
 * - 用户信息：getUserInfo / getUserStats / searchUsers / getMyBoats / updateProfile
 * - 情绪数据：emotion-calendar / emotion-heatmap
 */
class UserController : public drogon::HttpController<UserController> {
public:
  METHOD_LIST_BEGIN

  // ---- 认证 ----
  ADD_METHOD_TO(UserController::anonymousLogin, "/api/auth/anonymous", Post);
  ADD_METHOD_TO(UserController::recoverWithKey, "/api/auth/recover", Post);
  ADD_METHOD_TO(UserController::refreshToken, "/api/auth/refresh", Post);
  ADD_METHOD_TO(UserController::deleteAccount, "/api/auth/delete-account", Post);

  // ---- 用户信息 ----
  ADD_METHOD_TO(UserController::getUserInfo, "/api/users/{1}", Get);
  ADD_METHOD_TO(UserController::getUserStats, "/api/users/{1}/stats", Get);
  ADD_METHOD_TO(UserController::searchUsers, "/api/users/search", Get);
  ADD_METHOD_TO(UserController::getMyBoats, "/api/users/my/boats", Get);
  ADD_METHOD_TO(UserController::updateProfile, "/api/users/my/profile", Put);

  // ---- 情绪数据 ----
  ADD_METHOD_TO(UserController::getEmotionCalendar, "/api/users/my/emotion-calendar", Get);
  ADD_METHOD_TO(UserController::getEmotionHeatmap, "/api/users/my/emotion-heatmap", Get);

  METHOD_LIST_END

  /**
   * @brief 匿名登录
   * @details POST /api/auth/anonymous
   *
   * 无需任何凭证，服务端自动生成匿名用户身份。
   * 返回 PASETO 令牌、用户ID、恢复密钥（仅首次返回，需用户妥善保存）。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void anonymousLogin(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 通过恢复密钥找回账号
   * @details POST /api/auth/recover
   *
   * 请求体: { "recovery_key": "用户保存的恢复密钥" }
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @retval 400 恢复密钥无效或已过期
   */
  void recoverWithKey(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 刷新 PASETO 令牌
   * @details POST /api/auth/refresh
   *
   * 请求体: { "refresh_token": "..." }，可选。
   * 兼容旧客户端：若仍携带有效 Authorization 访问令牌，也可回补会话并下发 refresh_token。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void refreshToken(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 停用账号（保留 delete-account 路由名，语义与停用一致）
   * @details POST /api/auth/delete-account
   *
   * 与 `/api/account/deactivate` 保持一致；
   * 接受 `confirmation=DELETE/DEACTIVATE`，返回统一停用结果。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void deleteAccount(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 获取用户公开信息
   * @details GET /api/users/{userId}
   *
   * 公开端点，返回昵称、头像、签名等非敏感信息。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @param userId 用户ID
   */
  void getUserInfo(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   const std::string &userId);

  /**
   * @brief 获取用户统计数据
   * @details GET /api/users/{userId}/stats
   *
   * 返回石头数、好友数、涟漪数、纸船数等。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @param userId 用户ID
   */
  void getUserStats(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &userId);

  /**
   * @brief 搜索用户
   * @details GET /api/users/search?keyword=xxx&page=1&page_size=20
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void searchUsers(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 获取我收到的纸船列表
   * @details GET /api/users/my/boats?page=1&page_size=20
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void getMyBoats(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 更新个人资料
   * @details PUT /api/users/my/profile
   *
   * 请求体: { "nickname": "...", "bio": "...", "avatar_url": "..." }
   * 所有字段均可选。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void updateProfile(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 获取情绪日历
   * @details GET /api/users/my/emotion-calendar?year=2025&month=6
   *
   * 返回指定月份每天的主导情绪，用于日历视图展示。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void getEmotionCalendar(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 获取情绪热力图
   * @details GET /api/users/my/emotion-heatmap?days=90
   *
   * 返回近 N 天的情绪强度数据，用于热力图可视化。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void getEmotionHeatmap(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
