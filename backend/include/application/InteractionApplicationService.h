/**
 * InteractionApplicationService 模块接口定义
 */

#pragma once

#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include <memory>
#include <string>
#include <json/json.h>

namespace heartlake {
namespace application {

class InteractionApplicationService {
private:
    std::shared_ptr<core::cache::CacheManager> cacheManager_;
    std::shared_ptr<core::events::EventBus> eventBus_;

public:
    InteractionApplicationService(
        std::shared_ptr<core::cache::CacheManager> cacheManager,
        std::shared_ptr<core::events::EventBus> eventBus
    ) : cacheManager_(cacheManager),
        eventBus_(eventBus) {}

    // ==================== 涟漪 ====================

    /**
     * 创建涟漪
     */
    Json::Value createRipple(
        const std::string& stoneId,
        const std::string& userId
    );

    /**
     * 获取石头的涟漪列表
     */
    Json::Value getRipples(
        const std::string& stoneId,
        int page,
        int pageSize
    );

    /**
     * 删除涟漪
     */
    void deleteRipple(
        const std::string& rippleId,
        const std::string& userId
    );

    // ==================== 纸船（评论系统）====================
    // 注意：纸船（Paper Boat）就是评论系统
    // - 涟漪（Ripple）= 点赞
    // - 纸船（Paper Boat）= 评论

    /**
     * 发送纸船（带接收者的私密评论）
     */
    Json::Value sendBoat(
        const std::string& stoneId,
        const std::string& senderId,
        const std::string& receiverId,
        const std::string& message
    );

    /**
     * 获取收到的纸船
     */
    Json::Value getReceivedBoats(
        const std::string& userId,
        int page,
        int pageSize
    );

    /**
     * 获取发送的纸船
     */
    Json::Value getSentBoats(
        const std::string& userId,
        int page,
        int pageSize
    );

    /**
     * 创建纸船（公开评论，简化版本）
     */
    Json::Value createBoat(
        const std::string& stoneId,
        const std::string& userId,
        const std::string& content
    );

    /**
     * 删除纸船
     */
    Json::Value deleteBoat(
        const std::string& boatId,
        const std::string& userId
    );

    // ==================== 通知 ====================

    /**
     * 标记通知为已读
     */
    void markNotificationRead(
        const std::string& notificationId,
        const std::string& userId
    );

    /**
     * 标记所有通知为已读
     */
    void markAllNotificationsRead(
        const std::string& userId
    );

    /**
     * 获取通知列表
     */
    Json::Value getNotifications(
        const std::string& userId,
        int page,
        int pageSize
    );

    // ==================== 临时连接 ====================

    /**
     * 为石头创建临时连接
     */
    Json::Value createConnectionForStone(
        const std::string& stoneId,
        const std::string& userId
    );

    /**
     * 直接创建临时连接
     */
    Json::Value createConnection(
        const std::string& targetUserId,
        const std::string& userId
    );

    /**
     * 发送连接消息
     */
    Json::Value createConnectionMessage(
        const std::string& connectionId,
        const std::string& userId,
        const std::string& content
    );

    /**
     * 获取石头的纸船列表
     */
    Json::Value getBoats(
        const std::string& stoneId,
        int page,
        int pageSize
    );

    /**
     * 获取连接消息列表
     */
    Json::Value getConnectionMessages(
        const std::string& connectionId,
        int page,
        int pageSize
    );

    /**
     * 升级临时连接为好友
     */
    Json::Value upgradeConnectionToFriend(
        const std::string& connectionId,
        const std::string& userId
    );

    /**
     * 获取我的涟漪
     */
    Json::Value getMyRipples(
        const std::string& userId,
        int page,
        int pageSize
    );

    /**
     * 获取我的纸船
     */
    Json::Value getMyBoats(
        const std::string& userId,
        int page,
        int pageSize
    );
};

} // namespace application
} // namespace heartlake
