/**
 * InteractionController 模块接口定义
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * 互动相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class InteractionController : public drogon::HttpController<InteractionController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(InteractionController::createRipple, "/api/stones/{1}/ripples", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::createBoat, "/api/stones/{1}/boats", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::getBoats, "/api/stones/{1}/boats", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::getNotifications, "/api/notifications", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::markNotificationRead, "/api/notifications/{1}/read", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::markAllNotificationsRead, "/api/notifications/read-all", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::getUnreadCount, "/api/notifications/unread-count", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::createConnectionForStone, "/api/stones/{1}/connections", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::createConnection, "/api/connections", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::upgradeConnectionToFriend, "/api/connections/{1}/friend", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::getConnectionMessages, "/api/connections/{1}/messages", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::createConnectionMessage, "/api/connections/{1}/messages", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::getMyRipples, "/api/interactions/my/ripples", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::getMyBoats, "/api/interactions/my/boats", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::deleteBoat, "/api/boats/{1}", Delete, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(InteractionController::deleteRipple, "/api/ripples/{1}", Delete, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END
    
    void createRipple(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &stoneId);
    
    void createBoat(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   const std::string &stoneId);
    
    void getBoats(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback,
                 const std::string &stoneId);
    
    void getNotifications(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
    
    void markNotificationRead(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback,
                             const std::string &notificationId);
    
    void markAllNotificationsRead(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback);

    void getUnreadCount(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

    void createConnectionForStone(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback,
                                 const std::string &stoneId);

    void createConnection(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    void upgradeConnectionToFriend(const HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&callback,
                                  const std::string &connectionId);

    void getConnectionMessages(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback,
                              const std::string &connectionId);

    void createConnectionMessage(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback,
                                const std::string &connectionId);

    void getMyRipples(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

    void getMyBoats(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);
    
    void deleteBoat(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   const std::string &boatId);
    
    void deleteRipple(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &rippleId);
};

} // namespace controllers
} // namespace heartlake
