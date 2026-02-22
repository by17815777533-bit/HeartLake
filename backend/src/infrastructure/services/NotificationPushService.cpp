/**
 * @file NotificationPushService.cpp
 * @brief 实时通知推送服务实现
 */

#include "infrastructure/services/NotificationPushService.h"
#include <drogon/drogon.h>
#include <ctime>
#include "infrastructure/realtime/WebSocketHub.h"

namespace heartlake {
namespace services {

bool NotificationPushService::pushToUser(const std::string& userId, const NotificationMessage& notification) {
    auto& hub = heartlake::realtime::WebSocketHub::getInstance();

    try {
        Json::Value message;
        message["type"] = "notification";
        message["data"] = notification.toJson();

        Json::FastWriter writer;
        hub.sendToUser(userId, writer.write(message));
        LOG_INFO << "Notification pushed to user: " << userId;
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to push notification: " << e.what();
        return false;
    }
}

void NotificationPushService::pushFriendRequestNotification(const std::string& receiverId,
                                                             const std::string& requesterId,
                                                             const std::string& requesterNickname) {
    NotificationMessage notification;
    notification.notificationId = "notif_" + drogon::utils::getUuid();
    notification.userId = receiverId;
    notification.type = NotificationType::FRIEND_REQUEST;
    notification.title = "新的好友请求";
    notification.content = requesterNickname + " 想要添加你为好友";
    notification.relatedId = requesterId;
    notification.timestamp = std::time(nullptr);

    pushToUser(receiverId, notification);
}

void NotificationPushService::pushSystemNotice(const std::string& userId,
                                                const std::string& title,
                                                const std::string& content) {
    NotificationMessage notification;
    notification.notificationId = "notif_" + drogon::utils::getUuid();
    notification.userId = userId;
    notification.type = NotificationType::SYSTEM_NOTICE;
    notification.title = title;
    notification.content = content;
    notification.timestamp = std::time(nullptr);

    pushToUser(userId, notification);
}

void NotificationPushService::pushToMultipleUsers(const std::vector<std::string>& userIds,
                                                   const NotificationMessage& notification) {
    for (const auto& userId : userIds) {
        auto userNotification = notification;
        userNotification.userId = userId;
        pushToUser(userId, userNotification);
    }
}

} // namespace services
} // namespace heartlake
