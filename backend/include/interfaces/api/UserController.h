/**
 * 用户控制器 - 匿名登录 + 关键词恢复
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

class UserController : public drogon::HttpController<UserController> {
public:
  METHOD_LIST_BEGIN

  // 认证
  ADD_METHOD_TO(UserController::anonymousLogin, "/api/auth/anonymous", Post);
  ADD_METHOD_TO(UserController::recoverWithKey, "/api/auth/recover", Post);
  ADD_METHOD_TO(UserController::refreshToken, "/api/auth/refresh", Post);
  ADD_METHOD_TO(UserController::deleteAccount, "/api/auth/delete-account", Post);

  // 用户信息
  ADD_METHOD_TO(UserController::getUserInfo, "/api/users/{1}", Get);
  ADD_METHOD_TO(UserController::getUserStats, "/api/users/{1}/stats", Get);
  ADD_METHOD_TO(UserController::searchUsers, "/api/users/search", Get);
  ADD_METHOD_TO(UserController::getMyBoats, "/api/users/my/boats", Get);
  ADD_METHOD_TO(UserController::updateNickname, "/api/users/my/nickname", Put);
  ADD_METHOD_TO(UserController::updateProfile, "/api/users/my/profile", Put);

  // 情绪数据
  ADD_METHOD_TO(UserController::getEmotionCalendar, "/api/users/my/emotion-calendar", Get);
  ADD_METHOD_TO(UserController::getEmotionHeatmap, "/api/users/my/emotion-heatmap", Get);

  METHOD_LIST_END

  void anonymousLogin(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void recoverWithKey(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void refreshToken(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  void deleteAccount(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void getUserInfo(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   const std::string &userId);

  void getUserStats(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &userId);

  void searchUsers(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  void getMyBoats(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

  void updateNickname(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  void updateProfile(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void getEmotionCalendar(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

  void getEmotionHeatmap(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
