/**
 * 石头应用服务
 *
 * 作为 DDD Application Layer 的核心编排器，协调石头从发布到展示的完整生命周期。
 * 发布流程：校验 -> 领域服务创建实体 -> 持久化 -> 发布 StonePublishedEvent -> 异步触发 AI 分析。
 * 查询流程：优先走 CacheManager 缓存，miss 时回源数据库并回填缓存。
 *
 * @note 本层不包含业务规则（如所有权校验），这些由 StoneService 领域服务负责。
 *       本层只做用例编排、缓存协调、事件发布和 DTO 转换。
 */

#pragma once

#include "domain/stone/services/StoneService.h"
#include "domain/stone/repositories/IStoneRepository.h"
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include <memory>
#include <string>
#include <vector>
#include <json/json.h>

namespace heartlake {
namespace application {

class StoneApplicationService {
private:
    std::shared_ptr<domain::stone::StoneService> stoneService_;
    std::shared_ptr<domain::stone::IStoneRepository> stoneRepository_;
    std::shared_ptr<core::cache::CacheManager> cacheManager_;
    std::shared_ptr<core::events::EventBus> eventBus_;

public:
    StoneApplicationService(
        std::shared_ptr<domain::stone::StoneService> stoneService,
        std::shared_ptr<domain::stone::IStoneRepository> stoneRepository,
        std::shared_ptr<core::cache::CacheManager> cacheManager,
        std::shared_ptr<core::events::EventBus> eventBus
    ) : stoneService_(stoneService),
        stoneRepository_(stoneRepository),
        cacheManager_(cacheManager),
        eventBus_(eventBus) {}

    /**
     * @brief 发布一颗新石头到湖中
     *
     * 完整流程：领域服务创建实体 -> 持久化 -> 清除列表缓存 ->
     * 发布 StonePublishedEvent（触发 AI 情绪分析）-> 返回石头 JSON。
     *
     * @param userId 投放者用户 ID
     * @param content 石头正文内容
     * @param stoneType 外观类型（small / medium / large）
     * @param stoneColor 十六进制色值
     * @param moodType 情绪标签
     * @param isAnonymous 是否匿名投放
     * @return 包含完整石头信息的 JSON（含生成的 stoneId）
     */
    Json::Value publishStone(
        const std::string& userId,
        const std::string& content,
        const std::string& stoneType,
        const std::string& stoneColor,
        const std::string& moodType,
        bool isAnonymous,
        const std::vector<std::string>& tags = {}
    );

    /**
     * @brief 获取石头详情，附带当前用户的涟漪状态
     *
     * 优先从缓存读取，miss 时查库并回填。同时异步递增浏览量。
     *
     * @param stoneId 石头 ID
     * @param currentUserId 当前登录用户 ID，用于判断 has_rippled 字段
     * @return 石头详情 JSON，包含涟漪/纸船计数和 has_rippled 标记
     */
    Json::Value getStoneDetail(const std::string& stoneId, const std::string& currentUserId = "");

    /**
     * @brief 分页获取石头列表
     *
     * 支持按时间、涟漪数、纸船数、浏览量排序，可按情绪类型过滤。
     * 返回结果中每颗石头都会标注当前用户是否已点涟漪。
     *
     * @param page 页码（从 1 开始）
     * @param pageSize 每页数量
     * @param sortBy 排序字段，支持 created_at / ripple_count / boat_count / view_count
     * @param filterMood 按情绪类型过滤，空串表示不过滤
     * @param userId 指定用户 ID 时只返回该用户的石头
     * @param currentUserId 当前登录用户 ID，用于 has_rippled 判断
     * @return 分页 JSON，含 items 数组、total、page、pageSize
     */
    Json::Value getStoneList(
        int page,
        int pageSize,
        const std::string& sortBy = "created_at",
        const std::string& filterMood = "",
        const std::string& userId = "",
        const std::string& currentUserId = ""
    );

    /**
     * @brief 删除石头
     * @details 委托领域服务校验所有权后执行软删除，随后清除相关缓存。
     * @param stoneId 石头 ID
     * @param userId 操作者用户 ID（必须是石头所有者）
     */
    void deleteStone(const std::string& stoneId, const std::string& userId);

    /**
     * @brief 递增石头浏览量
     * @details fire-and-forget 语义，失败仅记日志不抛异常。
     * @param stoneId 石头 ID
     */
    void incrementViewCount(const std::string& stoneId);

    /**
     * @brief 石头发布后的异步 AI 处理管线
     *
     * 在独立线程中依次执行：情感分析 -> 情绪分数回写 -> 推荐索引更新。
     * 任何步骤失败都不影响石头的正常展示，仅降级 AI 相关功能。
     *
     * @param stoneId 石头 ID
     * @param userId 投放者用户 ID
     * @param content 石头正文（供情感分析使用）
     * @param moodType 用户自选的情绪标签（作为分析的先验参考）
     */
    void processStoneAsync(const std::string& stoneId, const std::string& userId,
                           const std::string& content, const std::string& moodType);
};

} // namespace application
} // namespace heartlake
