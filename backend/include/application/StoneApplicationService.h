/**
 * @file StoneApplicationService.h
 * @brief StoneApplicationService 模块接口定义
 * Created by 白洋
 */

#pragma once

#include "domain/stone/services/StoneService.h"
#include "domain/stone/repositories/IStoneRepository.h"
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include <memory>
#include <string>
#include <json/json.h>

namespace heartlake {
namespace application {

/**
 * @brief 石头应用服务
 *
 * 职责:
 * - 用例编排 (协调多个领域服务)
 * - 事务管理
 * - 事件发布
 * - DTO 转换
 * - 缓存协调
 */
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
     * @brief 发布石头
     */
    Json::Value publishStone(
        const std::string& userId,
        const std::string& content,
        const std::string& stoneType,
        const std::string& stoneColor,
        const std::string& moodType,
        bool isAnonymous
    );

    /**
     * @brief 获取石头详情
     */
    Json::Value getStoneDetail(const std::string& stoneId);

    /**
     * @brief 获取石头列表
     * @param page 页码
     * @param pageSize 每页数量
     * @param sortBy 排序字段
     * @param filterMood 心情过滤
     * @param userId 用户ID（可选，用于过滤特定用户的石头）
     */
    Json::Value getStoneList(
        int page,
        int pageSize,
        const std::string& sortBy = "created_at",
        const std::string& filterMood = "",
        const std::string& userId = ""
    );

    /**
     * @brief 删除石头
     */
    void deleteStone(const std::string& stoneId, const std::string& userId);

    /**
     * @brief 增加石头浏览量
     */
    void incrementViewCount(const std::string& stoneId);

    /**
     * @brief 异步处理石头发布后的AI任务
     */
    void processStoneAsync(const std::string& stoneId, const std::string& userId,
                           const std::string& content, const std::string& moodType);
};

} // namespace application
} // namespace heartlake
