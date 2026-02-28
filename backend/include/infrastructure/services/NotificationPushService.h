/**
 * @brief 实时通知推送服务 -- 通过 WebSocket 向客户端推送各类通知
 *
 * @details
 * 统一的通知出口，将业务层产生的各类事件（涟漪、纸船、好友请求等）
 * 封装为 NotificationMessage 后，通过 WebSocketHub 推送到客户端。
 *
 * 支持单用户推送和批量推送，提供好友请求、系统通知等常用场景的
 * 便捷方法，业务层无需关心 WebSocket 协议细节。
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <json/json.h>

namespace heartlake {
namespace services {

/// 通知类型枚举
enum class NotificationType {
    RIPPLE_RECEIVED,      ///< 收到涟漪
    BOAT_RECEIVED,        ///< 收到纸船
    FRIEND_REQUEST,       ///< 好友请求
    FRIEND_ACCEPTED,      ///< 好友接受
    TEMP_FRIEND_EXPIRING, ///< 临时好友即将过期
    NEW_MESSAGE,          ///< 新消息
    SYSTEM_NOTICE,        ///< 系统通知
    AI_REPLY              ///< AI 回复
};

/// 通知消息载体
struct NotificationMessage {
    std::string notificationId;   ///< 通知唯一标识
    std::string userId;           ///< 目标用户
    NotificationType type;        ///< 通知类型
    std::string title;            ///< 通知标题
    std::string content;          ///< 通知正文
    std::string relatedId;        ///< 关联的业务对象 ID（石头/纸船/好友等）
    int64_t timestamp;            ///< 通知产生的 Unix 时间戳
    Json::Value extraData;        ///< 附加数据（类型相关的扩展字段）

    /// 序列化为 JSON，用于 WebSocket 传输
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
 * @brief 通知推送服务，全局单例
 */
class NotificationPushService {
public:
    static NotificationPushService& getInstance() {
        static NotificationPushService instance;
        return instance;
    }

    /**
     * @brief 推送通知给指定用户
     * @param userId 目标用户 ID
     * @param notification 通知消息
     * @return 推送是否成功（用户不在线时返回 false）
     */
    bool pushToUser(const std::string& userId, const NotificationMessage& notification);

    /**
     * @brief 推送好友请求通知（便捷方法）
     * @param receiverId 接收方用户 ID
     * @param requesterId 请求方用户 ID
     * @param requesterNickname 请求方昵称，用于展示
     */
    void pushFriendRequestNotification(const std::string& receiverId,
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
