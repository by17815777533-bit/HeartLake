/**
 * 好友控制器 - 好友关系管理与私信通信
 *
 * 覆盖好友系统的完整生命周期：
 * - 发送/接受/拒绝好友请求
 * - 删除好友关系
 * - 好友列表与待处理请求查询
 * - 好友间私信收发
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
 * | POST   | /api/friends/request           | 发送好友请求        |
 * | POST   | /api/friends/accept/{userId}   | 接受好友请求        |
 * | POST   | /api/friends/reject/{userId}   | 拒绝好友请求        |
 * | DELETE | /api/friends/{friendId}         | 删除好友            |
 * | GET    | /api/friends                    | 获取好友列表        |
 * | GET    | /api/friends/requests/pending   | 获取待处理请求      |
 * | POST   | /api/friends/{friendId}/messages| 发送私信           |
 * | GET    | /api/friends/{friendId}/messages| 获取私信记录        |
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
     * 不能向自己发送请求，重复请求返回幂等响应。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void sendFriendRequest(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 接受好友请求
     * @details POST /api/friends/accept/{userId}
     *
     * 接受后双方互为好友，通过 WebSocket 通知对方。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param userId 发起请求的用户ID
     */
    void acceptFriendRequest(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback,
                            const std::string &userId);

    /**
     * @brief 拒绝好友请求
     * @details POST /api/friends/reject/{userId}
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
     * 双向解除好友关系。
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
     * @return 分页好友列表，包含昵称、头像、在线状态
     */
    void getFriends(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取待处理的好友请求
     * @details GET /api/friends/requests/pending
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 待处理请求列表，包含请求者信息和请求时间
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
