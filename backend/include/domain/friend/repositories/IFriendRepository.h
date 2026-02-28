/**
 * 好友仓储抽象接口
 *
 * 定义好友关系 Aggregate Root 的持久化契约，是领域层与基础设施层的依赖倒置边界。
 * HeartLake 的好友关系带有 24 小时 TTL 机制——
 * 两人通过石头结缘后建立临时好友，到期自动解除，
 * 需要双方主动续期才能维持长期关系。
 *
 * @note 好友关系是双向的：A 向 B 发送请求，B 接受后双方互为好友。
 *       但数据库中只存一条记录（userId=A, friendId=B），查询时需要双向匹配。
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <drogon/drogon.h>

namespace heartlake::domain::friend_domain {

/**
 * @brief 好友关系领域实体
 * @details 映射 friends 表，记录一条从 userId 到 friendId 的好友关系。
 *          status 字段驱动状态机：pending -> accepted / rejected。
 */
struct FriendEntity {
    std::string friendshipId;
    std::string userId;        ///< 发起方（发送好友请求的用户）
    std::string friendId;      ///< 接收方（收到好友请求的用户）
    std::string status;        ///< 状态机：pending / accepted / rejected
    std::string createdAt;
    int64_t ttlSeconds = 86400; ///< 好友关系存活时间，默认 24 小时（86400 秒）
};

/**
 * @brief 好友仓储接口（纯虚基类）
 *
 * 同时提供协程异步和传统同步两套方法。
 * 异步方法基于 drogon::Task，推荐在协程控制器中使用；
 * 同步方法仅供旧代码路径兼容，新功能不应再使用。
 */
class IFriendRepository {
public:
    virtual ~IFriendRepository() = default;

    // --- 协程异步接口 ---

    /**
     * @brief 持久化好友关系实体
     * @param friendship 待保存的好友关系（friendshipId 为空时自动生成）
     */
    virtual drogon::Task<void> saveAsync(const FriendEntity& friendship) = 0;

    /**
     * @brief 双向查找好友关系
     * @details 无论谁是发起方都能匹配到，SQL 层用 OR 条件实现。
     * @param userId 用户 A
     * @param friendId 用户 B
     * @return 好友关系实体，不存在时返回 nullopt
     */
    virtual drogon::Task<std::optional<FriendEntity>> findByUserAndFriendAsync(const std::string& userId, const std::string& friendId) = 0;

    /**
     * @brief 查询某用户所有已接受的好友关系
     * @param userId 用户 ID
     * @return status='accepted' 的好友关系列表
     */
    virtual drogon::Task<std::vector<FriendEntity>> findByUserIdAsync(const std::string& userId) = 0;

    /**
     * @brief 查询某用户关联的全部好友记录
     * @details 包含 pending / accepted / rejected 所有状态，用于好友请求列表展示。
     * @param userId 用户 ID
     * @return 全部好友关系列表
     */
    virtual drogon::Task<std::vector<FriendEntity>> findAllByUserIdAsync(const std::string& userId) = 0;

    /**
     * @brief 更新好友关系状态
     * @param friendshipId 好友关系 ID
     * @param status 新状态（accepted / rejected）
     */
    virtual drogon::Task<void> updateStatusAsync(const std::string& friendshipId, const std::string& status) = 0;

    /**
     * @brief 按 ID 删除单条好友关系记录
     * @param friendshipId 好友关系 ID
     */
    virtual drogon::Task<void> deleteByIdAsync(const std::string& friendshipId) = 0;

    /**
     * @brief 双向删除好友关系
     * @details 同时移除 A->B 和 B->A 两条记录（如果存在），确保双方视角一致。
     * @param userId 用户 A
     * @param friendId 用户 B
     */
    virtual drogon::Task<void> deleteBidirectionalAsync(const std::string& userId, const std::string& friendId) = 0;

    // --- 同步接口（deprecated，保留兼容） ---

    /// @brief 同步版持久化
    virtual void save(const FriendEntity& friendship) = 0;

    /// @brief 同步版双向查找
    virtual std::optional<FriendEntity> findByUserAndFriend(const std::string& userId, const std::string& friendId) = 0;

    /// @brief 同步版查询已接受的好友列表
    virtual std::vector<FriendEntity> findByUserId(const std::string& userId) = 0;

    /// @brief 同步版更新状态
    virtual void updateStatus(const std::string& friendshipId, const std::string& status) = 0;

    /// @brief 同步版按 ID 删除
    virtual void deleteById(const std::string& friendshipId) = 0;
};

} // namespace heartlake::domain::friend_domain
