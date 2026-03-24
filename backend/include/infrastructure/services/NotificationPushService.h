/**
 * @brief 通知服务 -- 统一负责持久化与实时分发
 *
 * @details
 * 所有站内通知必须经过该服务写入 notifications 表，然后再通过统一的
 * WebSocket 事件 `new_notification` 分发给对应用户，避免出现
 * “实时收到了但刷新列表消失”或“不同模块自行拼接不同通知格式”的漂移。
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <json/json.h>

namespace heartlake {
namespace services {

enum class NotificationType {
    RIPPLE_RECEIVED,
    BOAT_RECEIVED,
    CONNECTION_REQUEST,
    FRIEND_ACCEPTED,
    TEMP_FRIEND_EXPIRING,
    NEW_MESSAGE,
    SYSTEM_NOTICE,
    AI_REPLY
};

struct NotificationMessage {
    std::string notificationId;
    std::string userId;
    NotificationType type{NotificationType::SYSTEM_NOTICE};
    std::string title;
    std::string content;
    std::string relatedId;
    std::string relatedType;
    bool isRead{false};
    int64_t timestamp{0};
    Json::Value extraData{Json::objectValue};

    Json::Value toRealtimePayload() const;
};

/**
 * @brief 通知推送服务，全局单例
 */
class NotificationPushService {
public:
    static NotificationPushService& getInstance() {
        static NotificationPushService instance;
        return instance;
    }

    /// 写库并向目标用户分发实时通知。
    bool push(NotificationMessage notification);

    /**
     * @brief 推送连接/临时好友通知
     * @param receiverId 接收方用户 ID
     * @param requesterId 发起方用户 ID
     * @param requesterNickname 发起方昵称
     */
    void pushConnectionNotice(const std::string& receiverId,
                              const std::string& requesterId,
                              const std::string& requesterNickname);

    /**
     * @brief 推送系统通知（便捷方法）
     * @param userId 目标用户 ID
     * @param title 通知标题
     * @param content 通知正文
     */
    void pushSystemNotice(const std::string& userId,
                         const std::string& title,
                         const std::string& content);

    /**
     * @brief 批量推送同一通知给多个用户
     * @param userIds 目标用户 ID 列表
     * @param notification 通知消息
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
