/**
 * 好友仓储 PostgreSQL 实现
 *
 * 通过 Drogon ORM 操作 friends 表，实现 IFriendRepository 全部契约。
 * 双向查询（A->B 或 B->A）在 SQL 层用 OR 条件处理，无需应用层拼接。
 *
 * @note deleteBidirectionalAsync 使用单条 DELETE ... WHERE (A,B) OR (B,A) 语句，
 *       一次往返完成双向清理，比两次单向删除更高效。
 */

#pragma once

#include "IFriendRepository.h"
#include <drogon/drogon.h>

namespace heartlake::domain::friend_domain {

class FriendRepository : public IFriendRepository {
public:
    // --- 协程异步接口 ---

    /// @brief 插入新的好友关系记录
    drogon::Task<void> saveAsync(const FriendEntity& friendship) override;

    /// @brief 双向查找好友关系（SQL OR 条件匹配双方向）
    drogon::Task<std::optional<FriendEntity>> findByUserAndFriendAsync(const std::string& userId, const std::string& friendId) override;

    /// @brief 查询某用户已接受的好友列表
    drogon::Task<std::vector<FriendEntity>> findByUserIdAsync(const std::string& userId) override;

    /// @brief 查询某用户全部好友记录（含所有状态）
    drogon::Task<std::vector<FriendEntity>> findAllByUserIdAsync(const std::string& userId) override;

    /// @brief 更新好友关系状态（pending -> accepted / rejected）
    drogon::Task<void> updateStatusAsync(const std::string& friendshipId, const std::string& status) override;

    /// @brief 按 ID 删除单条记录
    drogon::Task<void> deleteByIdAsync(const std::string& friendshipId) override;

    /// @brief 双向删除：一条 SQL 同时清理 A->B 和 B->A
    drogon::Task<void> deleteBidirectionalAsync(const std::string& userId, const std::string& friendId) override;

    // --- 同步接口（兼容旧代码路径） ---

    /// @brief 同步版插入
    void save(const FriendEntity& friendship) override;

    /// @brief 同步版双向查找
    std::optional<FriendEntity> findByUserAndFriend(const std::string& userId, const std::string& friendId) override;

    /// @brief 同步版查询已接受的好友列表
    std::vector<FriendEntity> findByUserId(const std::string& userId) override;

    /// @brief 同步版更新状态
    void updateStatus(const std::string& friendshipId, const std::string& status) override;

    /// @brief 同步版按 ID 删除
    void deleteById(const std::string& friendshipId) override;

private:
    /**
     * @brief 将数据库行映射为 FriendEntity
     * @param row Drogon ORM 返回的数据库行
     * @return 填充完毕的好友关系实体
     */
    static FriendEntity rowToEntity(const drogon::orm::Row& row);
};

} // namespace heartlake::domain::friend_domain
