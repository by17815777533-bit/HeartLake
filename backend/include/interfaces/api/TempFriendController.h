/**
 * 临时好友控制器 - 24小时限时社交关系
 *
 * 实现"心湖"特色的临时好友机制：用户因石头或纸船互动自动建立的
 * 临时关系，24小时后自动失效。在有效期内双方可以聊天。
 * 历史上的手动直连和手动升级永久好友入口已经下线。
 *
 * 设计理念：降低社交压力，让用户在安全的时间窗口内
 * 自然地建立连接，避免永久关系带来的心理负担。
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 临时好友 HTTP 控制器
 *
 * @details 路由表：
 * | 方法   | 路径                              | 说明                |
 * |--------|----------------------------------|-------------------|
 * | POST   | /api/temp-friends                | 历史手动直连入口（已下线） |
 * | GET    | /api/temp-friends                | 获取临时好友列表     |
 * | GET    | /api/temp-friends/{id}           | 获取临时好友详情     |
 * | POST   | /api/temp-friends/{id}/upgrade   | 历史升级入口（已下线） |
 * | DELETE | /api/temp-friends/{id}           | 删除临时好友关系     |
 * | GET    | /api/temp-friends/check/{userId} | 检查临时好友状态     |
 */
class TempFriendController
    : public drogon::HttpController<TempFriendController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(TempFriendController::createTempFriend, "/api/temp-friends",
                Post, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(TempFriendController::getMyTempFriends, "/api/temp-friends",
                Get, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(TempFriendController::getTempFriendDetail,
                "/api/temp-friends/{1}", Get,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(TempFriendController::upgradeToPermanent,
                "/api/temp-friends/{1}/upgrade", Post,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(TempFriendController::deleteTempFriend, "/api/temp-friends/{1}",
                Delete, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(TempFriendController::checkTempFriendStatus,
                "/api/temp-friends/check/{1}", Get,
                "heartlake::filters::SecurityAuditFilter");

  METHOD_LIST_END

  /**
   * @brief 历史手动创建临时好友入口（已下线）
   * @details POST /api/temp-friends
   *
   * 当前关系模式下，临时好友只允许由石头/纸船互动自动建立。
   * 该接口保留路由兼容性，但固定返回 400。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   */
  void
  createTempFriend(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 获取当前用户的临时好友列表
   * @details GET /api/temp-friends
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @return 临时好友列表，包含剩余有效时间
   */
  void
  getMyTempFriends(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 获取临时好友关系详情
   * @details GET /api/temp-friends/{tempFriendId}
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @param tempFriendId 临时好友关系ID
   */
  void
  getTempFriendDetail(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      const std::string &tempFriendId);

  /**
   * @brief 历史手动升级永久好友入口（已下线）
   * @details POST /api/temp-friends/{tempFriendId}/upgrade
   *
   * 当前好友关系由亲密分自动判定，该接口保留路由兼容性，
   * 但固定返回 400。
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @param tempFriendId 临时好友关系ID
   */
  void
  upgradeToPermanent(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &tempFriendId);

  /**
   * @brief 主动删除临时好友关系
   * @details DELETE /api/temp-friends/{tempFriendId}
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @param tempFriendId 临时好友关系ID
   */
  void deleteTempFriend(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        const std::string &tempFriendId);

  /**
   * @brief 检查与目标用户的临时好友状态
   * @details GET /api/temp-friends/check/{targetUserId}
   *
   * @param req HTTP 请求
   * @param callback 响应回调
   * @param targetUserId 目标用户ID
   * @return 是否存在临时好友关系、剩余时间
   */
  void
  checkTempFriendStatus(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        const std::string &targetUserId);
};

} // namespace controllers
} // namespace heartlake
