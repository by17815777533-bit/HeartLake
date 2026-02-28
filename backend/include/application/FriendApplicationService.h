/**
 * 好友应用服务
 *
 * 在 DDD 应用层编排好友相关用例：发送/接受/拒绝请求、删除好友、查询列表等。
 * 接受好友请求时会同步创建 24h TTL 的好友关系（通过 FriendshipTTLEngine），
 * 好友列表查询使用批量 TTL 查询优化，避免 N+1 问题导致超时。
 *
 * 全部方法基于 C++20 协程（drogon::Task），适配 Drogon 异步控制器。
 *
 * @note 好友关系的核心业务规则（如重复请求检测、双向删除）由领域层 FriendService 负责，
 *       本层只做用例编排、TTL 管理和 DTO 转换。
 */

#pragma once

#include <drogon/drogon.h>
#include "domain/friend/services/FriendService.h"
#include "domain/friend/repositories/IFriendRepository.h"
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include <memory>
#include <json/json.h>

namespace heartlake::application {

class FriendApplicationService {
private:
    std::shared_ptr<domain::friend_domain::FriendService> friendService_;
    std::shared_ptr<domain::friend_domain::IFriendRepository> repository_;
    std::shared_ptr<core::cache::CacheManager> cacheManager_;
    std::shared_ptr<core::events::EventBus> eventBus_;

public:
    FriendApplicationService() = default;

    /**
     * @brief 构造函数，注入全部依赖
     * @param friendService 好友领域服务（业务规则）
     * @param repository 好友仓储（持久化）
     * @param cacheManager 缓存管理器（好友列表缓存）
     * @param eventBus 事件总线（发布好友关系变更事件）
     */
    FriendApplicationService(
        std::shared_ptr<domain::friend_domain::FriendService> friendService,
        std::shared_ptr<domain::friend_domain::IFriendRepository> repository,
        std::shared_ptr<core::cache::CacheManager> cacheManager,
        std::shared_ptr<core::events::EventBus> eventBus
    ) : friendService_(friendService),
        repository_(repository),
        cacheManager_(cacheManager),
        eventBus_(eventBus) {}

    /**
     * @brief 发送好友请求
     * @param fromUserId 发起方用户 ID
     * @param toUserId 接收方用户 ID
     * @param message 附言（可选，展示在请求列表中）
     * @return 包含 status / from_user_id / to_user_id 的 JSON
     */
    drogon::Task<Json::Value> sendFriendRequestAsync(
        const std::string& fromUserId,
        const std::string& toUserId,
        const std::string& message
    );

    /**
     * @brief 接受好友请求
     *
     * 支持两种匹配方式：先按 friendshipId 精确匹配，
     * 未命中则按 from_user_id 查找 pending 请求（兼容前端传参差异）。
     * 接受后通过 FriendshipTTLEngine 创建带 24h TTL 的好友关系。
     *
     * @param userId 当前用户 ID（接收方）
     * @param friendshipId 好友请求 ID 或发起方用户 ID
     * @return 包含好友关系详情和 TTL 信息的 JSON
     */
    drogon::Task<Json::Value> acceptFriendRequestAsync(
        const std::string& userId,
        const std::string& friendshipId
    );

    /**
     * @brief 拒绝好友请求
     * @details 匹配逻辑同 acceptFriendRequestAsync，支持 ID 和用户名两种方式。
     * @param userId 当前用户 ID
     * @param friendshipId 好友请求 ID 或发起方用户 ID
     * @return 操作结果 JSON
     */
    drogon::Task<Json::Value> rejectFriendRequestAsync(
        const std::string& userId,
        const std::string& friendshipId
    );

    /**
     * @brief 删除好友关系
     * @details 委托领域服务执行双向删除，随后清除双方的好友列表缓存。
     * @param userId 当前用户 ID
     * @param friendId 要删除的好友用户 ID
     * @return 操作结果 JSON
     */
    drogon::Task<Json::Value> removeFriendAsync(
        const std::string& userId,
        const std::string& friendId
    );

    /**
     * @brief 获取好友列表
     * @details 返回所有已接受的好友关系，每条附带 TTL 剩余秒数。
     *          使用批量 TTL 查询优化，单次 SQL 获取全部 TTL，避免 N+1。
     * @param userId 用户 ID
     * @return 好友数组 JSON，每个元素含 friendship_id / friend_info / ttl_remaining
     */
    drogon::Task<Json::Value> getFriendsListAsync(const std::string& userId);

    /**
     * @brief 获取收到的待处理好友请求
     * @details 只返回 status=pending 的请求，附带发起者的 nickname 和 avatar。
     * @param userId 当前用户 ID
     * @return 请求数组 JSON
     */
    drogon::Task<Json::Value> getReceivedRequestsAsync(const std::string& userId);

    /**
     * @brief 获取已发出的好友请求
     * @param userId 当前用户 ID
     * @return 请求数组 JSON，含每条请求的状态
     */
    drogon::Task<Json::Value> getSentRequestsAsync(const std::string& userId);
};

} // namespace heartlake::application
