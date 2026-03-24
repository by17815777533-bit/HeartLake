/**
 * 统一通知服务实现
 *
 * 先写 notifications 表，再通过统一的 `new_notification` 事件实时推送。
 * 这样通知列表、未读数和实时角标共享同一份事实来源。
 */

#include "infrastructure/services/NotificationPushService.h"

#include <ctime>
#include <drogon/drogon.h>

#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/IdGenerator.h"

namespace heartlake::services {

namespace {

std::string notificationTypeToStorage(NotificationType type) {
    switch (type) {
        case NotificationType::RIPPLE_RECEIVED:
            return "ripple";
        case NotificationType::BOAT_RECEIVED:
            return "boat";
        case NotificationType::CONNECTION_REQUEST:
            return "connection";
        case NotificationType::FRIEND_ACCEPTED:
            return "friend_accepted";
        case NotificationType::TEMP_FRIEND_EXPIRING:
            return "temp_friend_expired";
        case NotificationType::NEW_MESSAGE:
            return "message";
        case NotificationType::SYSTEM_NOTICE:
            return "system_notice";
        case NotificationType::AI_REPLY:
            return "ai_reply";
    }
    return "system_notice";
}

void mergeRealtimeFields(Json::Value &payload, const Json::Value &extraData) {
    if (!extraData.isObject()) {
        return;
    }

    for (const auto &memberName : extraData.getMemberNames()) {
        payload[memberName] = extraData[memberName];
    }
}

} // namespace

Json::Value NotificationMessage::toRealtimePayload() const {
    Json::Value payload(Json::objectValue);
    const auto notificationType = notificationTypeToStorage(type);

    payload["notification_id"] = notificationId;
    payload["id"] = notificationId;
    payload["notificationId"] = notificationId;
    payload["user_id"] = userId;
    payload["notification_type"] = notificationType;
    payload["title"] = title;
    payload["content"] = content;
    payload["related_id"] = relatedId;
    payload["target_id"] = relatedId;
    payload["related_type"] = relatedType;
    payload["is_read"] = isRead;
    payload["timestamp"] = static_cast<Json::Int64>(timestamp);
    payload["extra_data"] = extraData;
    mergeRealtimeFields(payload, extraData);
    return payload;
}

bool NotificationPushService::push(NotificationMessage notification) {
    if (notification.userId.empty()) {
        LOG_WARN << "Skip notification with empty user id";
        return false;
    }

    if (notification.notificationId.empty()) {
        notification.notificationId = heartlake::utils::IdGenerator::generateNotificationId();
    }
    if (notification.timestamp <= 0) {
        notification.timestamp = static_cast<int64_t>(std::time(nullptr));
    }

    const auto notificationType = notificationTypeToStorage(notification.type);
    auto db = drogon::app().getDbClient("default");

    try {
        db->execSqlSync(
            "INSERT INTO notifications "
            "(notification_id, user_id, type, title, content, related_id, related_type, is_read, created_at) "
            "VALUES ($1, $2, $3, NULLIF($4, ''), $5, NULLIF($6, ''), NULLIF($7, ''), $8, TO_TIMESTAMP($9))",
            notification.notificationId,
            notification.userId,
            notificationType,
            notification.title,
            notification.content,
            notification.relatedId,
            notification.relatedType,
            notification.isRead,
            notification.timestamp);

        auto unreadResult = db->execSqlSync(
            "SELECT COUNT(*)::INTEGER AS unread_count "
            "FROM notifications WHERE user_id = $1 AND is_read = false",
            notification.userId);

        auto payload = notification.toRealtimePayload();
        payload["unread_count"] =
            unreadResult.empty() ? 0 : unreadResult[0]["unread_count"].as<int>();
        payload["type"] = "new_notification";
        heartlake::controllers::BroadcastWebSocketController::sendToUser(
            notification.userId, payload);
        return true;
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Failed to persist notification for user "
                  << notification.userId << ": " << e.base().what();
        return false;
    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to dispatch notification for user "
                  << notification.userId << ": " << e.what();
        return false;
    }
}

void NotificationPushService::pushConnectionNotice(
    const std::string &receiverId,
    const std::string &requesterId,
    const std::string &requesterNickname) {
    NotificationMessage notification;
    notification.userId = receiverId;
    notification.type = NotificationType::CONNECTION_REQUEST;
    notification.title = "新的连接";
    notification.content = requesterNickname.empty()
        ? "有人想和你建立新的连接"
        : requesterNickname + " 想和你建立新的连接";
    notification.relatedId = requesterId;
    notification.relatedType = "user";
    notification.timestamp = static_cast<int64_t>(std::time(nullptr));
    notification.extraData["from_user_id"] = requesterId;
    notification.extraData["target_user_id"] = requesterId;

    push(notification);
}

void NotificationPushService::pushSystemNotice(const std::string &userId,
                                               const std::string &title,
                                               const std::string &content) {
    NotificationMessage notification;
    notification.userId = userId;
    notification.type = NotificationType::SYSTEM_NOTICE;
    notification.title = title;
    notification.content = content;
    notification.relatedType = "system";
    notification.timestamp = static_cast<int64_t>(std::time(nullptr));

    push(notification);
}

void NotificationPushService::pushToMultipleUsers(
    const std::vector<std::string> &userIds,
    const NotificationMessage &notification) {
    for (const auto &userId : userIds) {
        auto userNotification = notification;
        userNotification.userId = userId;
        push(userNotification);
    }
}

} // namespace heartlake::services
