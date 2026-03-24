/**
 * 好友领域服务（Domain Service）
 *
 * 封装好友关系的核心业务规则：发送请求、接受/拒绝、删除好友、关系判定等。
 * 发送请求前会通过仓储检查是否已存在关系记录（任意状态），防止重复请求。
 * 删除好友采用双向删除策略，确保双方视角一致——不会出现"我删了你但你还看得到我"的情况。
 *
 * @note 本层只处理业务规则，不涉及 TTL 管理、缓存失效等应用层关注点；
 *       这些能力现在由控制器和独立基础设施服务直接编排。
 */

#pragma once

#include "domain/friend/repositories/IFriendRepository.h"
#include <memory>
#include <drogon/drogon.h>

namespace heartlake::domain::friend_domain {

class FriendService {
public:
    /**
     * @brief 构造函数，注入仓储依赖
     * @param repository 好友仓储接口（通过 DI 容器注入具体实现）
     */
    explicit FriendService(std::shared_ptr<IFriendRepository> repository)
        : repository_(repository) {}

    // --- 协程异步接口 ---

    /**
     * @brief 发送好友请求
     * @details 先查询是否已存在任意状态的关系记录，存在则抛出异常。
     *          不存在时创建 status='pending' 的新记录。
     * @param userId 发起方用户 ID
     * @param friendId 接收方用户 ID
     * @throws std::runtime_error 当已存在关系记录时
     */
    drogon::Task<void> sendFriendRequestAsync(const std::string& userId, const std::string& friendId);

    /**
     * @brief 接受好友请求
     * @details 将 status 从 pending 更新为 accepted。
     * @param friendshipId 好友关系 ID
     */
    drogon::Task<void> acceptFriendRequestAsync(const std::string& friendshipId);

    /**
     * @brief 拒绝好友请求
     * @details 将 status 从 pending 更新为 rejected。
     * @param friendshipId 好友关系 ID
     */
    drogon::Task<void> rejectFriendRequestAsync(const std::string& friendshipId);

    /**
     * @brief 双向删除好友关系
     * @details 委托仓储的 deleteBidirectionalAsync 一次性清理双方向记录。
     * @param userId 用户 A
     * @param friendId 用户 B
     */
    drogon::Task<void> removeFriendAsync(const std::string& userId, const std::string& friendId);

    /**
     * @brief 获取某用户的已接受好友列表
     * @param userId 用户 ID
     * @return status='accepted' 的好友关系列表
     */
    drogon::Task<std::vector<FriendEntity>> getFriendsAsync(const std::string& userId);

    /**
     * @brief 判断两人是否为已接受的好友
     * @param userId 用户 A
     * @param friendId 用户 B
     * @return true 表示双方为好友（status='accepted'）
     */
    drogon::Task<bool> areFriendsAsync(const std::string& userId, const std::string& friendId);

private:
    std::shared_ptr<IFriendRepository> repository_;
};

} // namespace heartlake::domain::friend_domain
