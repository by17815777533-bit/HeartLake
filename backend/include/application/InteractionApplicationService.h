/**
 * 互动应用服务
 *
 * 编排 HeartLake 中所有用户互动用例，涵盖四大子系统：
 *   - 涟漪（Ripple）：类似点赞，每人对同一石头只能点一次（唯一约束保证幂等）
 *   - 纸船（Paper Boat）：评论系统，分为公开评论和带接收者的私密评论
 *   - 通知（Notification）：互动产生的消息通知
 *   - 临时连接（Temp Connection）：通过石头建立的 24h 临时聊天通道，可升级为好友
 *
 * 涟漪和纸船操作会同步更新石头的计数器并清除缓存，
 * 纸船创建时还会调用 EdgeAI 做情绪暖意评分，触发守护者激励积分。
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
     * @brief 创建涟漪（点赞）
     *
     * 使用事务 + 唯一约束防止并发重复。重复点击时做幂等处理，
     * 返回已有涟漪信息而非报错，避免前端误判为权限问题。
     *
     * @param stoneId 目标石头 ID
     * @param userId 点赞用户 ID
     * @return 涟漪记录 JSON（含 ripple_id、created_at）
     */
    Json::Value createRipple(
        const std::string& stoneId,
        const std::string& userId
    );

    /**
     * @brief 分页获取石头的涟漪列表
     * @param stoneId 石头 ID
     * @param page 页码（从 1 开始）
     * @param pageSize 每页数量
     * @return 分页 JSON，含涟漪数组和 total
     */
    Json::Value getRipples(
        const std::string& stoneId,
        int page,
        int pageSize
    );

    /**
     * @brief 删除涟漪（取消点赞）
     * @details 事务内递减石头的 ripple_count，使用 GREATEST(0, count-1) 防止负数。
     * @param rippleId 涟漪记录 ID
     * @param userId 操作者 ID（必须是涟漪创建者）
     */
    void deleteRipple(
        const std::string& rippleId,
        const std::string& userId
    );

    // ==================== 纸船（评论系统）====================
    // 涟漪（Ripple）= 点赞
    // 纸船（Paper Boat）= 评论

    /**
     * @brief 发送私密纸船（带指定接收者）
     * @details 默认匿名发送，接收者可在"收到的纸船"列表中查看。
     * @param stoneId 关联的石头 ID
     * @param senderId 发送者用户 ID
     * @param receiverId 接收者用户 ID
     * @param message 纸船内容
     * @return 纸船记录 JSON
     */
    Json::Value sendBoat(
        const std::string& stoneId,
        const std::string& senderId,
        const std::string& receiverId,
        const std::string& message
    );

    /**
     * @brief 分页获取用户收到的私密纸船
     * @param userId 接收者用户 ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 分页 JSON
     */
    Json::Value getReceivedBoats(
        const std::string& userId,
        int page,
        int pageSize
    );

    /**
     * @brief 分页获取用户发出的私密纸船
     * @param userId 发送者用户 ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 分页 JSON
     */
    Json::Value getSentBoats(
        const std::string& userId,
        int page,
        int pageSize
    );

    /**
     * @brief 创建公开纸船（简化版评论）
     *
     * 不需要指定接收者，创建后会通过 EdgeAI 评估暖意分，
     * 按比例发放守护者激励积分。同时递增石头的 boat_count。
     *
     * @param stoneId 目标石头 ID
     * @param userId 评论者用户 ID
     * @param content 纸船内容
     * @return 纸船记录 JSON（含 warmth_score）
     */
    Json::Value createBoat(
        const std::string& stoneId,
        const std::string& userId,
        const std::string& content
    );

    /**
     * @brief 删除纸船
     * @details 事务内递减 boat_count 并返回最新计数，使用 GREATEST(0, count-1) 防止负数。
     * @param boatId 纸船记录 ID
     * @param userId 操作者 ID（必须是纸船创建者）
     * @return 包含更新后 boat_count 的 JSON
     */
    Json::Value deleteBoat(
        const std::string& boatId,
        const std::string& userId
    );

    // ==================== 通知 ====================

    /**
     * @brief 标记单条通知为已读
     * @param notificationId 通知 ID
     * @param userId 通知所属用户 ID（防止越权标记他人通知）
     */
    void markNotificationRead(
        const std::string& notificationId,
        const std::string& userId
    );

    /**
     * @brief 一键标记该用户所有未读通知为已读
     * @param userId 用户 ID
     */
    void markAllNotificationsRead(
        const std::string& userId
    );

    /**
     * @brief 分页获取用户通知列表
     * @param userId 用户 ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 分页 JSON，含 unread_count 和通知数组
     */
    Json::Value getNotifications(
        const std::string& userId,
        int page,
        int pageSize
    );

    // ==================== 临时连接 ====================

    /**
     * @brief 通过石头发起临时连接
     * @details 与石头投放者建立 24h 有效期的临时聊天通道。
     *          若已存在活跃连接则返回已有记录（幂等）。
     * @param stoneId 石头 ID（用于定位投放者）
     * @param userId 发起者用户 ID
     * @return 连接记录 JSON（含 connection_id、expires_at）
     */
    Json::Value createConnectionForStone(
        const std::string& stoneId,
        const std::string& userId
    );

    /**
     * @brief 直接向目标用户发起临时连接
     * @param targetUserId 目标用户 ID
     * @param userId 发起者用户 ID
     * @return 连接记录 JSON
     */
    Json::Value createConnection(
        const std::string& targetUserId,
        const std::string& userId
    );

    /**
     * @brief 在临时连接中发送消息
     * @details 内部校验 userId 是否为该连接的参与者，防止 IDOR 越权访问。
     * @param connectionId 连接 ID
     * @param userId 发送者用户 ID
     * @param content 消息内容
     * @return 消息记录 JSON
     */
    Json::Value createConnectionMessage(
        const std::string& connectionId,
        const std::string& userId,
        const std::string& content
    );

    /**
     * @brief 获取石头下的公开纸船列表
     * @param stoneId 石头 ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 分页 JSON
     */
    Json::Value getBoats(
        const std::string& stoneId,
        int page,
        int pageSize
    );

    /**
     * @brief 获取临时连接中的消息记录
     * @param connectionId 连接 ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 分页消息 JSON
     */
    Json::Value getConnectionMessages(
        const std::string& connectionId,
        int page,
        int pageSize
    );

    /**
     * @brief 将临时连接升级为正式好友关系
     * @details 双方都需要同意才能升级。升级后连接不会立即关闭，
     *          而是等 TTL 自然过期，好友关系独立于连接存在。
     * @param connectionId 连接 ID
     * @param userId 操作者用户 ID
     * @return 升级结果 JSON（含新建的 friendship_id）
     */
    Json::Value upgradeConnectionToFriend(
        const std::string& connectionId,
        const std::string& userId
    );

    /**
     * @brief 获取当前用户点过的涟漪列表
     * @param userId 用户 ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 分页 JSON
     */
    Json::Value getMyRipples(
        const std::string& userId,
        int page,
        int pageSize
    );

    /**
     * @brief 获取当前用户发出的公开纸船列表
     * @param userId 用户 ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 分页 JSON
     */
    Json::Value getMyBoats(
        const std::string& userId,
        int page,
        int pageSize
    );
};

} // namespace application
} // namespace heartlake
