/**
 * @file UserController.h
 * @brief UserController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 用户相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class UserController : public drogon::HttpController<UserController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(UserController::anonymousLogin, "/api/auth/anonymous", Post);

  ADD_METHOD_TO(UserController::registerUser, "/api/auth/register", Post);
  ADD_METHOD_TO(UserController::registerUser, "/api/v1/auth/register", Post);

  ADD_METHOD_TO(UserController::loginUser, "/api/auth/login", Post);
  ADD_METHOD_TO(UserController::loginUser, "/api/v1/auth/login", Post);

  ADD_METHOD_TO(UserController::getUserStats, "/api/users/{1}/stats", Get);

  ADD_METHOD_TO(UserController::getMyBoats, "/api/users/my/boats", Get);

  ADD_METHOD_TO(UserController::getEmotionCalendar,
                "/api/users/my/emotion-calendar", Get);

  ADD_METHOD_TO(UserController::getEmotionHeatmap,
                "/api/users/my/emotion-heatmap", Get);

  ADD_METHOD_TO(UserController::updateNickname, "/api/users/my/nickname",
                Put);

  ADD_METHOD_TO(UserController::updateProfile, "/api/users/my/profile", Put);

  ADD_METHOD_TO(UserController::sendVerificationCode,
                "/api/auth/verification-code", Post);

  ADD_METHOD_TO(UserController::sendEmailVerificationCode,
                "/api/auth/email/verification-code", Post);

  ADD_METHOD_TO(UserController::registerWithEmail,
                "/api/auth/register/email", Post);

  ADD_METHOD_TO(UserController::loginWithEmail, "/api/auth/login/email",
                Post);

  ADD_METHOD_TO(UserController::changePassword, "/api/auth/change-password",
                Post);

  ADD_METHOD_TO(UserController::sendResetPasswordCode,
                "/api/auth/reset-password/code", Post);

  ADD_METHOD_TO(UserController::resetPassword, "/api/auth/reset-password",
                Post);

  ADD_METHOD_TO(UserController::deleteAccount, "/api/auth/delete-account",
                Post);

  ADD_METHOD_TO(UserController::searchUsers, "/api/users/search", Get);

  ADD_METHOD_TO(UserController::getUserInfo, "/api/users/{1}", Get);

  ADD_METHOD_TO(UserController::refreshToken, "/api/auth/refresh", Post);

  ADD_METHOD_TO(UserController::recoverWithKey, "/api/auth/recover", Post);

  METHOD_LIST_END

  void anonymousLogin(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void registerUser(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void loginUser(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);

  void getUserStats(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &userId);

  void getMyBoats(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

  void
  getEmotionCalendar(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void
  getEmotionHeatmap(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void updateNickname(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void updateProfile(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void
  sendVerificationCode(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

  void sendEmailVerificationCode(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback);

  void
  registerWithEmail(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void loginWithEmail(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void changePassword(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void sendResetPasswordCode(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback);

  void resetPassword(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void deleteAccount(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void searchUsers(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  void getUserInfo(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   const std::string &userId);

  void refreshToken(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void recoverWithKey(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
