/**
 * 互动控制器 - 涟漪、纸船、通知、连接等社交互动接口
 *
 * 管理用户之间围绕"石头"展开的各类互动行为，包括：
 * - 涟漪（类似点赞）的创建与删除
 * - 纸船（评论）的发送、查看与删除
 * - 站内通知的读取与已读标记
 * - 用户连接的建立、消息收发及好友升级
 *
 * 所有写操作会通过 WebSocket 广播实时事件，
 * 并在必要时向石头作者推送定向通知。
 */

#pragma once

#include "infrastructure/filters/SecurityAuditFilter.h"
#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * 互动控制器
 *
 * 统一处理涟漪、纸船、通知、连接四大互动模块的 HTTP 请求。
 * 所有端点均经过 SecurityAuditFilter 安全审计。
 */
class InteractionController
    : public drogon::HttpController<InteractionController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(InteractionController::createRipple, "/api/stones/{1}/ripples",
                Post, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::createBoat, "/api/stones/{1}/boats",
                Post, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::getBoats, "/api/stones/{1}/boats", Get,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::getNotifications, "/api/notifications",
                Get, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::markNotificationRead,
                "/api/notifications/{1}/read", Post,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::markAllNotificationsRead,
                "/api/notifications/read-all", Post,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::getUnreadCount,
                "/api/notifications/unread-count", Get,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::createConnectionForStone,
                "/api/stones/{1}/connections", Post,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::createConnection, "/api/connections",
                Post, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::upgradeConnectionToFriend,
                "/api/connections/{1}/friend", Post,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::getConnectionMessages,
                "/api/connections/{1}/messages", Get,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::createConnectionMessage,
                "/api/connections/{1}/messages", Post,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::getMyRipples,
                "/api/interactions/my/ripples", Get,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::getMyBoats, "/api/interactions/my/boats",
                Get, "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::deleteBoat, "/api/boats/{1}", Delete,
                "heartlake::filters::SecurityAuditFilter");

  ADD_METHOD_TO(InteractionController::deleteRipple, "/api/ripples/{1}", Delete,
                "heartlake::filters::SecurityAuditFilter");

  METHOD_LIST_END

  /**
   * 为石头创建涟漪（点赞）
   * POST /api/stones/{stoneId}/ripples
   *
   * 幂等操作：重复点赞不会增加计数，返回 already_rippled 标记。
   * 首次涟漪会广播 ripple_update 事件并通知石头作者。
   *
   * @param stoneId 目标石头ID
   * @return 涟漪计数及是否重复操作
   * @retval 401 未登录
   * @retval 400 石头不存在等业务错误
   */
  void createRipple(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &stoneId);

  /**
   * 为石头发送纸船（评论）
   * POST /api/stones/{stoneId}/boats
   *
   * 请求体: { "content": "评论内容" }
   * 成功后广播 boat_update 事件并通知石头作者。
   *
   * @param stoneId 目标石头ID
   * @return 纸船ID、纸船计数
   * @retval 400 content 为空或缺失
   */
  void createBoat(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  const std::string &stoneId);

  /**
   * 获取石头的纸船列表
   * GET /api/stones/{stoneId}/boats?page=1&page_size=20
   *
   * @param stoneId 目标石头ID
   * @return 分页纸船列表，包含 boats、total、page、page_size
   */
  void getBoats(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                const std::string &stoneId);

  /**
   * 获取当前用户的通知列表
   * GET /api/notifications?page=1&page_size=20
   *
   * @return 分页通知列表
   */
  void
  getNotifications(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * 标记单条通知为已读
   * POST /api/notifications/{notificationId}/read
   *
   * @param notificationId 通知ID
   * @retval 400 通知不存在或无权操作
   */
  void
  markNotificationRead(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       const std::string &notificationId);

  /**
   * 一键标记所有通知为已读
   * POST /api/notifications/read-all
   */
  void markAllNotificationsRead(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * 获取未读通知数量
   * GET /api/notifications/unread-count
   *
   * @return { "unread_count": N }
   */
  void getUnreadCount(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * 通过石头建立用户连接
   * POST /api/stones/{stoneId}/connections
   *
   * 在当前用户与石头作者之间创建连接关系。
   *
   * @param stoneId 目标石头ID
   */
  void createConnectionForStone(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &stoneId);

  /**
   * 直接创建用户连接
   * POST /api/connections
   *
   * 请求体: { "target_user_id": "..." }
   *
   * @retval 400 目标用户ID为空
   */
  void
  createConnection(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * 历史连接升级好友入口（已下线）
   * POST /api/connections/{connectionId}/friend
   *
   * 当前关系模式不支持手动升级好友。该路由仅为兼容保留，
   * 固定返回 400，不再推送 friend_accepted 事件。
   *
   * @param connectionId 连接ID
   */
  void upgradeConnectionToFriend(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &connectionId);

  /**
   * 获取连接内的消息列表
   * GET /api/connections/{connectionId}/messages?page=1&page_size=20
   *
   * 单页最多50条消息。
   *
   * @param connectionId 连接ID
   */
  void
  getConnectionMessages(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        const std::string &connectionId);

  /**
   * 在连接中发送消息
   * POST /api/connections/{connectionId}/messages
   *
   * 请求体: { "content": "消息内容" }
   *
   * @param connectionId 连接ID
   * @retval 400 content 为空或缺失
   */
  void createConnectionMessage(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &connectionId);

  /**
   * 获取我发出的涟漪列表
   * GET /api/interactions/my/ripples?page=1&page_size=20
   */
  void getMyRipples(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * 获取我发出的纸船列表
   * GET /api/interactions/my/boats?page=1&page_size=20
   */
  void getMyBoats(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * 删除纸船
   * DELETE /api/boats/{boatId}
   *
   * 仅允许删除自己发送的纸船，成功后广播 boat_deleted 事件。
   *
   * @param boatId 纸船ID
   */
  void deleteBoat(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  const std::string &boatId);

  /**
   * 删除涟漪（取消点赞）
   * DELETE /api/ripples/{rippleId}
   *
   * 仅允许删除自己的涟漪，成功后广播 ripple_deleted 事件。
   *
   * @param rippleId 涟漪ID
   */
  void deleteRipple(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &rippleId);
};

} // namespace controllers
} // namespace heartlake
