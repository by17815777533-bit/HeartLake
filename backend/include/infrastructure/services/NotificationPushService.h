/**
 * @file NotificationPushService.h
 * @brief 实时通知推送服务 - WebSocket推送
 * @author 白洋
 * @date 2025-02-08
 */

#pragma once

#include <drogon/drogon.h>
#include <json/json.h>
#include "infrastructure/realtime/WebSocketHub.h"

namespace heartlake {
namespace services {

/**
 * @brief 通知类型
 */
enum class NotificationType {
    RIPPLE_RECEIVED,      // 收到涟漪
    BOAT_RECEIVED,        // 收到纸船
    FRIEND_REQUEST,       // 好友请求
    FRIEND_ACCEPTED,      // 好友接受
    TEMP_FRIEND_EXPIRING, // 临时好友即将过期
    NEW_MESSAGE,          // 新消息
    SYSTEM_NOTICE,        // 系统通知
    AI_REPLY              // AI回复
};

/**
 * @brief 通知消息结构
 */
struct NotificationMessage {
    std::string notificationId;
    std::string userId;
    NotificationType type;
    std::string title;
    std::string content;
    std::string relatedId;
    int64_t timestamp;
    Json::Value extraData;

    Json::Value toJson() const {
        Json::Value json;
        json["notification_id"] = notificationId;
        json["user_id"] = userId;
        json["type"] = static_cast<int>(type);
        json["title"] = title;
        json["content"] = content;
        json["related_id"] = relatedId;
        json["timestamp"] = timestamp;
        json["extra_data"] = extraData;
        return json;
    }
};

/**
 * @brief 通知推送服务
 */
class NotificationPushService {
public:
    static NotificationPushService& getInstance() {
        static NotificationPushService instance;
        return instance;
    }

    /**
     * @brief 推送通知给指定用户
     */
    bool pushToUser(const std::string& userId, const NotificationMessage& notification) {
        // 使用 WebSocketHub（实际的连接管理器）而非废弃的 WebSocketConnectionManager
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

    /**
     * @brief 推送涟漪通知
     */
    void pushRippleNotification(const std::string& stoneOwnerId,
                               const std::string& rippleUserId,
                               const std::string& stoneId) {
        NotificationMessage notification;
        notification.notificationId = "notif_" + drogon::utils::getUuid();
        notification.userId = stoneOwnerId;
        notification.type = NotificationType::RIPPLE_RECEIVED;
        notification.title = "收到新的涟漪";
        notification.content = "有人喜欢了你的石头";
        notification.relatedId = stoneId;
        notification.timestamp = std::time(nullptr);
        notification.extraData["ripple_user_id"] = rippleUserId;

        pushToUser(stoneOwnerId, notification);
    }

    /**
     * @brief 推送纸船通知
     */
    void pushBoatNotification(const std::string& receiverId,
                             const std::string& senderId,
                             const std::string& boatId,
                             const std::string& content) {
        NotificationMessage notification;
        notification.notificationId = "notif_" + drogon::utils::getUuid();
        notification.userId = receiverId;
        notification.type = NotificationType::BOAT_RECEIVED;
        notification.title = "收到新纸船";
        notification.content = content.substr(0, 50); // 截取前50字符
        notification.relatedId = boatId;
        notification.timestamp = std::time(nullptr);
        notification.extraData["sender_id"] = senderId;

        pushToUser(receiverId, notification);
    }

    /**
     * @brief 推送好友请求通知
     */
    void pushFriendRequestNotification(const std::string& receiverId,
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

    /**
     * @brief 推送临时好友即将过期通知
     */
    void pushTempFriendExpiringNotification(const std::string& userId,
                                            const std::string& friendId,
                                            const std::string& friendNickname,
                                            int minutesLeft) {
        NotificationMessage notification;
        notification.notificationId = "notif_" + drogon::utils::getUuid();
        notification.userId = userId;
        notification.type = NotificationType::TEMP_FRIEND_EXPIRING;
        notification.title = "临时好友即将过期";
        notification.content = "与 " + friendNickname + " 的临时好友关系还有 " +
                              std::to_string(minutesLeft) + " 分钟过期";
        notification.relatedId = friendId;
        notification.timestamp = std::time(nullptr);
        notification.extraData["minutes_left"] = minutesLeft;

        pushToUser(userId, notification);
    }

    /**
     * @brief 推送新消息通知
     */
    void pushNewMessageNotification(const std::string& receiverId,
                                    const std::string& senderId,
                                    const std::string& senderNickname,
                                    const std::string& messagePreview) {
        NotificationMessage notification;
        notification.notificationId = "notif_" + drogon::utils::getUuid();
        notification.userId = receiverId;
        notification.type = NotificationType::NEW_MESSAGE;
        notification.title = "新消息";
        notification.content = senderNickname + ": " + messagePreview.substr(0, 30);
        notification.relatedId = senderId;
        notification.timestamp = std::time(nullptr);

        pushToUser(receiverId, notification);
    }

    /**
     * @brief 推送系统通知
     */
    void pushSystemNotice(const std::string& userId,
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

    /**
     * @brief 批量推送通知
     */
    void pushToMultipleUsers(const std::vector<std::string>& userIds,
                            const NotificationMessage& notification) {
        for (const auto& userId : userIds) {
            auto userNotification = notification;
            userNotification.userId = userId;
            pushToUser(userId, userNotification);
        }
    }

private:
    NotificationPushService() = default;
    ~NotificationPushService() = default;
    NotificationPushService(const NotificationPushService&) = delete;
    NotificationPushService& operator=(const NotificationPushService&) = delete;
};

} // namespace services
} // namespace heartlake
