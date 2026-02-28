/**
 * 石头仓储 PostgreSQL 实现
 *
 * 通过 Drogon ORM 操作 stones 表，实现 IStoneRepository 定义的全部持久化契约。
 * 排序字段使用白名单校验（created_at / ripple_count / boat_count / view_count），
 * 拒绝非法字段以防止 SQL 注入。删除走软删除（设置 deleted_at + status='deleted'）。
 *
 * @note save() 内部使用 INSERT ... RETURNING * 一次往返完成插入和回读，
 *       避免额外的 SELECT 查询。
 */

#pragma once

#include "IStoneRepository.h"
#include <drogon/drogon.h>

namespace heartlake::domain::stone {

class StoneRepository : public IStoneRepository {
public:
    /// @brief 插入新石头，自动生成 UUID 作为 stoneId
    StoneEntity save(const StoneEntity& stone) override;

    /// @brief 按 ID 查找，只返回 status='published' 的记录
    std::optional<StoneEntity> findById(const std::string& stoneId) override;

    /// @brief 分页查询用户的石头，按 created_at DESC 排序
    std::vector<StoneEntity> findByUserId(const std::string& userId, int page, int pageSize) override;

    /**
     * @brief 分页查询全部已发布石头
     * @details sortBy 经白名单校验后拼入 ORDER BY，非法值回退到 created_at。
     */
    std::vector<StoneEntity> findAll(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) override;

    /// @brief 统计已发布石头总数
    int countAll(const std::string& filterMood = "") override;

    /// @brief 统计某用户的石头数量
    int countByUserId(const std::string& userId) override;

    /// @brief 软删除：UPDATE status='deleted', deleted_at=NOW()
    void deleteById(const std::string& stoneId) override;

    /// @brief 原子递增 view_count，fire-and-forget
    void incrementViewCount(const std::string& stoneId) override;

    /// @brief 回写 AI 情感分析结果到 emotion_score 和 analyzed_mood 字段
    void updateEmotionScore(const std::string& stoneId, float score, const std::string& mood) override;

private:
    /**
     * @brief 将数据库行映射为 StoneEntity
     * @param row Drogon ORM 返回的数据库行
     * @return 填充完毕的石头实体
     */
    StoneEntity rowToEntity(const drogon::orm::Row& row);
};

} // namespace heartlake::domain::stone
