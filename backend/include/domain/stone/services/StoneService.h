/**
 * 石头领域服务（Domain Service）
 *
 * 封装石头 Aggregate Root 的核心业务规则：创建、查询、删除、浏览量递增等。
 * 删除时会校验所有权——只有石头的投放者才能删除自己的石头，违反则抛出异常。
 *
 * 本层严格遵循 DDD 分层约束：不涉及缓存、事件发布、DTO 转换等基础设施关注点，
 * 这些横切逻辑由上层 StoneApplicationService 负责协调。
 */

#pragma once

#include "domain/stone/repositories/IStoneRepository.h"
#include <memory>

namespace heartlake::domain::stone {

class StoneService {
public:
    /**
     * @brief 构造函数，注入仓储依赖
     * @param repository 石头仓储接口（通过 DI 容器注入具体实现）
     */
    explicit StoneService(std::shared_ptr<IStoneRepository> repository)
        : repository_(repository) {}

    /**
     * @brief 创建一颗新石头并持久化
     *
     * 组装 StoneEntity 后委托仓储执行 INSERT，返回含生成 stoneId 的完整实体。
     * 不做内容审核——审核由应用层异步触发 AI 引擎完成。
     *
     * @param userId 投放者用户 ID
     * @param content 石头正文
     * @param stoneType 外观类型（small / medium / large）
     * @param stoneColor 十六进制色值（如 #FF6B6B）
     * @param moodType 用户自选的情绪标签
     * @param isAnonymous 是否匿名投放
     * @return 持久化后的完整实体（含生成的 stoneId 和 createdAt）
     */
    StoneEntity createStone(const std::string& userId, const std::string& content,
                           const std::string& stoneType, const std::string& stoneColor,
                           const std::string& moodType, bool isAnonymous);

    /**
     * @brief 按 ID 获取石头详情
     * @param stoneId 石头 ID
     * @return 石头实体，未找到返回 nullopt
     */
    std::optional<StoneEntity> getStone(const std::string& stoneId);

    /**
     * @brief 分页获取某用户投放的石头
     * @param userId 用户 ID
     * @param page 页码（从 1 开始）
     * @param pageSize 每页数量
     * @return 石头列表
     */
    std::vector<StoneEntity> getUserStones(const std::string& userId, int page, int pageSize);

    /**
     * @brief 分页获取所有已发布石头
     * @param page 页码
     * @param pageSize 每页数量
     * @param sortBy 排序字段
     * @param filterMood 情绪过滤条件，空串不过滤
     * @return 石头列表
     */
    std::vector<StoneEntity> listStones(int page, int pageSize, const std::string& sortBy, const std::string& filterMood);

    /// @brief 获取已发布石头总数，可按情绪过滤
    int getTotalCount(const std::string& filterMood = "");

    /// @brief 获取某用户的石头数量
    int getUserStoneCount(const std::string& userId);

    /**
     * @brief 删除石头（带所有权校验）
     * @details 先查询石头实体，比对 userId 是否为所有者。
     *          不匹配时抛出 std::runtime_error，匹配后执行软删除。
     * @param stoneId 石头 ID
     * @param userId 操作者用户 ID
     * @throws std::runtime_error 当 userId 不是石头所有者时
     */
    void deleteStone(const std::string& stoneId, const std::string& userId);

    /**
     * @brief 递增石头浏览量
     * @details 委托仓储的 fire-and-forget 实现，失败不影响主流程。
     * @param stoneId 石头 ID
     */
    void incrementViews(const std::string& stoneId);

private:
    std::shared_ptr<IStoneRepository> repository_;
};

} // namespace heartlake::domain::stone
