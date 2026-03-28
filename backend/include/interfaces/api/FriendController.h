/**
 * 好友控制器 - 基于亲密分的自动关系与私信通信
 *
 * 当前好友系统不再使用传统“申请 / 同意 / 拒绝”流程，而是：
 * - 通过互动亲密分自动决定是否视为可聊天好友
 * - 仅保留 request 路由用于恢复隐藏关系；accept/reject/pending 已下线
 * - 删除好友仅对当前用户隐藏，不破坏整体互动图
 * - 私信读写统一受亲密分阈值与隐藏状态约束
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 好友系统 HTTP 控制器
 *
 * @details 路由表：
 * | 方法   | 路径                            | 说明               |
 * |--------|--------------------------------|-------------------|
 * | POST   | /api/friends/request            | 兼容入口：恢复隐藏关系 / 返回亲密分状态 |
 * | POST   | /api/friends/accept/{userId}    | 已下线，固定返回 400 |
 * | POST   | /api/friends/reject/{userId}    | 已下线，固定返回 400 |
 * | DELETE | /api/friends/{friendId}         | 对当前用户隐藏好友   |
 * | GET    | /api/friends                    | 获取自动关系好友列表 |
 * | GET    | /api/friends/requests/pending   | 已下线，固定返回 400 |
 * | POST   | /api/friends/{friendId}/messages| 发送私信（需达亲密分阈值） |
 * | GET    | /api/friends/{friendId}/messages| 获取私信记录（需达亲密分阈值） |
 */
class FriendController : public drogon::HttpController<FriendController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(FriendController::sendFriendRequest, "/api/friends/request", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(FriendController::acceptFriendRequest, "/api/friends/accept/{1}", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(FriendController::rejectFriendRequest, "/api/friends/reject/{1}", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(FriendController::removeFriend, "/api/friends/{1}", Delete, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(FriendController::getFriends, "/api/friends", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(FriendController::getPendingRequests, "/api/friends/requests/pending", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(FriendController::sendMessage, "/api/friends/{1}/messages", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(FriendController::getMessages, "/api/friends/{1}/messages", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    /**
     * @brief 发送好友请求
     * @details POST /api/friends/request
     *
     * 请求体: { "target_user_id": "目标用户ID" }
     * 当前实现不会创建待处理请求，而是返回亲密分状态；
     * 若当前用户此前隐藏了该关系，会顺便恢复显示。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void sendFriendRequest(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 接受好友请求（已下线）
     * @details POST /api/friends/accept/{userId}
     *
     * 当前关系模式不支持该操作，接口固定返回 400。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param userId 发起请求的用户ID
     */
    void acceptFriendRequest(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback,
                            const std::string &userId);

    /**
     * @brief 拒绝好友请求（已下线）
     * @details POST /api/friends/reject/{userId}
     *
     * 当前关系模式不支持该操作，接口固定返回 400。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param userId 发起请求的用户ID
     */
    void rejectFriendRequest(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback,
                            const std::string &userId);

    /**
     * @brief 删除好友关系
     * @details DELETE /api/friends/{friendId}
     *
     * 当前实现语义为“仅自己隐藏该好友”。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param friendId 要删除的好友用户ID
     */
    void removeFriend(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &friendId);

    /**
     * @brief 获取好友列表
     * @details GET /api/friends?page=1&page_size=20
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 自动关系好友列表，包含亲密分、等级、评分拆解和聊天资格
     */
    void getFriends(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取待处理的好友请求（已下线）
     * @details GET /api/friends/requests/pending
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 当前关系模式不支持该操作，接口固定返回 400
     */
    void getPendingRequests(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 向好友发送私信
     * @details POST /api/friends/{friendId}/messages
     *
     * 请求体: { "content": "消息内容" }
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param friendId 好友用户ID
     */
    void sendMessage(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &friendId);

    /**
     * @brief 获取与好友的私信记录
     * @details GET /api/friends/{friendId}/messages?page=1&page_size=50
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param friendId 好友用户ID
     * @return 分页消息列表，按时间正序排列
     */
    void getMessages(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &friendId);
};

} // namespace controllers
} // namespace heartlake
