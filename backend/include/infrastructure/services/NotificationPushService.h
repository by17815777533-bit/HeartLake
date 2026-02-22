/**
 * @file NotificationPushService.h
 * @brief 实时通知推送服务 - WebSocket推送
 * @author 白洋
 * @date 2025-02-08
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <json/json.h>

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
    bool pushToUser(const std::string& userId, const NotificationMessage& notification);

    /**
     * @brief 推送好友请求通知
     */
    void pushFriendRequestNotification(const std::string& receiverId,
                                       const std::string& requesterId,
                                       const std::string& requesterNickname);

    /**
     * @brief 推送系统通知
     */
    void pushSystemNotice(const std::string& userId,
                         const std::string& title,
                         const std::string& content);

    /**
     * @brief 批量推送通知
     */
    void pushToMultipleUsers(const std::vector<std::string>& userIds,
                            const NotificationMessage& notification);

private:
    NotificationPushService() = default;
    ~NotificationPushService() = default;
    NotificationPushService(const NotificationPushService&) = delete;
    NotificationPushService& operator=(const NotificationPushService&) = delete;
};

} // namespace services
} // namespace heartlake
