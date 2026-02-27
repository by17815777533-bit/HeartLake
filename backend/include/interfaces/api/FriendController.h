/**
 * FriendController 模块接口定义
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * 好友相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
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
    
    void sendFriendRequest(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
    
    void acceptFriendRequest(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback,
                            const std::string &userId);
    
    void rejectFriendRequest(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback,
                            const std::string &userId);
    
    void removeFriend(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &friendId);
    
    void getFriends(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getPendingRequests(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

    void sendMessage(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &friendId);

    void getMessages(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &friendId);
};

} // namespace controllers
} // namespace heartlake
