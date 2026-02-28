/**
 * 石头仓储抽象接口
 *
 * 定义石头 Aggregate Root 的持久化契约。
 * "石头"是 HeartLake 的核心内容载体——用户将心情写在石头上投入湖中，
 * 其他人可以捡起来阅读、点涟漪（点赞）或放纸船（评论）回应。
 *
 * 领域层和应用层只依赖此接口，具体的 PostgreSQL 实现通过 DI 容器注入。
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <json/json.h>

namespace heartlake::domain::stone {

/**
 * @brief 石头领域实体
 * @details 映射 stones 表，承载用户投放的心情内容及其互动统计。
 *          涟漪/纸船/浏览计数由数据库事务维护，实体中为只读快照。
 */
struct StoneEntity {
    std::string stoneId;
    std::string userId;
    std::string content;       ///< 石头正文（用户写下的心情文字）
    std::string stoneType;     ///< 石头外观类型：small / medium / large
    std::string stoneColor;    ///< 石头颜色（十六进制色值）
    std::string moodType;      ///< 情绪标签：happy / sad / calm / anxious 等
    bool isAnonymous;          ///< 是否匿名投放
    int ripplesCount;          ///< 涟漪数（类似点赞）
    int boatsCount;            ///< 纸船数（类似评论）
    int viewsCount;            ///< 浏览量
    std::string status;        ///< published / deleted
    std::string createdAt;
};

/**
 * @brief 石头仓储接口（纯虚基类）
 *
 * 提供石头的 CRUD、分页查询、计数以及情绪分数更新等操作。
 * 删除采用软删除策略（设置 deleted_at + status='deleted'），不会物理移除数据。
 *
 * @note 所有查询方法默认过滤已删除的石头（status='deleted'），
 *       只返回 status='published' 的记录。
 */
class IStoneRepository {
public:
    virtual ~IStoneRepository() = default;

    /**
     * @brief 持久化石头实体
     * @details 若 stoneId 为空则自动生成 UUID，INSERT 后返回含完整字段的实体。
     * @param stone 待持久化的石头实体
     * @return 持久化后的完整实体（含生成的 stoneId 和 createdAt）
     */
    virtual StoneEntity save(const StoneEntity& stone) = 0;

    /**
     * @brief 按 ID 查找已发布且未删除的石头
     * @param stoneId 石头唯一标识
     * @return 石头实体，未找到或已删除时返回 nullopt
     */
    virtual std::optional<StoneEntity> findById(const std::string& stoneId) = 0;

    /**
     * @brief 分页查询某用户投放的石头
     * @param userId 用户 ID
     * @param page 页码（从 1 开始）
     * @param pageSize 每页数量
     * @return 石头列表，按创建时间降序
     */
    virtual std::vector<StoneEntity> findByUserId(const std::string& userId, int page, int pageSize) = 0;

    /**
     * @brief 分页查询所有已发布石头
     * @param page 页码
     * @param pageSize 每页数量
     * @param sortBy 排序字段，支持 created_at / ripple_count / boat_count / view_count
     * @param filterMood 按情绪类型过滤，空串表示不过滤
     * @return 石头列表
     */
    virtual std::vector<StoneEntity> findAll(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) = 0;

    /// @brief 统计已发布石头总数，可按情绪类型过滤
    virtual int countAll(const std::string& filterMood = "") = 0;

    /// @brief 统计某用户投放的石头数量
    virtual int countByUserId(const std::string& userId) = 0;

    /**
     * @brief 软删除石头
     * @details 设置 deleted_at 时间戳并将 status 改为 'deleted'，不物理移除行。
     * @param stoneId 石头 ID
     */
    virtual void deleteById(const std::string& stoneId) = 0;

    /**
     * @brief 异步递增浏览量
     * @details fire-and-forget 语义，失败仅记日志不抛异常，不影响主流程。
     * @param stoneId 石头 ID
     */
    virtual void incrementViewCount(const std::string& stoneId) = 0;

    /**
     * @brief 更新情感分析结果
     * @details 由 AI 引擎异步回调写入，更新 emotion_score 和 analyzed_mood 字段。
     * @param stoneId 石头 ID
     * @param score 情感分数（-1.0 ~ 1.0，负值为消极，正值为积极）
     * @param mood AI 分析得出的情绪标签
     */
    virtual void updateEmotionScore(const std::string& stoneId, float score, const std::string& mood) = 0;
};

} // namespace heartlake::domain::stone
