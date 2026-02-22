/**
 * @file RecommendationController.h
 * @brief RecommendationController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include "utils/ResponseUtil.h"
#include "utils/IdGenerator.h"
#include "infrastructure/filters/SecurityAuditFilter.h"
#include <json/json.h>
#include <cmath>
#include <algorithm>
#include <unordered_map>

using namespace drogon;
using namespace heartlake::utils;

namespace heartlake {
namespace controllers {

/**
 * @brief 推荐相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class RecommendationController : public HttpController<RecommendationController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RecommendationController::getRecommendedStones,
                  "/api/recommendations/stones", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(RecommendationController::discoverByMood,
                  "/api/recommendations/discover/{mood}", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(RecommendationController::discoverByMood,
                  "/api/discover/{mood}", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(RecommendationController::trackInteraction,
                  "/api/recommendations/track", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(RecommendationController::getEmotionTrends,
                  "/api/recommendations/emotion-trends", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(RecommendationController::getTrendingContent,
                  "/api/recommendations/trending", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(RecommendationController::searchRecommendations,
                  "/api/recommendations/search", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(RecommendationController::trackBatchInteractions,
                  "/api/recommendations/track-batch", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(RecommendationController::getAdvancedRecommendations,
                  "/api/recommendations/advanced", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    /**
     * 获取推荐的石头
     * 算法：混合推荐（协同过滤 40% + 内容过滤 40% + 随机探索 20%）
     */
    void getRecommendedStones(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * 基于情绪的发现
     * 发现当前处于特定情绪的用户和内容
     */
    void discoverByMood(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       const std::string &mood);

    /**
     * 记录用户交互
     * 用于学习用户偏好
     */
    void trackInteraction(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * 获取个人情绪趋势
     * 展示用户的情绪变化曲线
     */
    void getEmotionTrends(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * 获取热门内容
     * 发现当前热门的石头、标签、情绪
     */
    void getTrendingContent(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * 搜索推荐内容
     * 全文搜索 + 智能排序
     */
    void searchRecommendations(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * 批量追踪交互
     * 批量上传用户交互数据
     */
    void trackBatchInteractions(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * 高级推荐（多算法融合）
     * GET /api/recommendations/advanced?limit=20
     * 使用UCB、Thompson Sampling、MMR等高级算法
     */
    void getAdvancedRecommendations(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback);

private:
    /**
     * 计算石头推荐
     * 混合推荐算法的核心实现
     */
    void calculateStoneRecommendations(const std::string &userId,
                                      const orm::DbClientPtr &dbClient,
                                      std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * 计算两个用户的相似度
     * 基于共同的交互模式
     */
    double calculateUserSimilarity(const std::string &userId1,
                                   const std::string &userId2,
                        const orm::DbClientPtr &dbClient);

    /**
     * 获取用户的情绪兼容性分数
     * 基于情绪兼容性矩阵
     */
    double getEmotionCompatibility(const std::string &mood1,
                                   const std::string &mood2,
                                   const orm::DbClientPtr &dbClient);

    /**
     * 计算内容推荐分数
     * 综合考虑：向量相似度、情绪匹配、时间衰减
     */
    double calculateContentScore(const Json::Value &stone,
                                 const Json::Value &userProfile,
                                 const orm::DbClientPtr &dbClient);

    /**
     * 更新用户偏好
     * 基于最近的交互自动学习
     */
    void updateUserPreferences(const std::string &userId,
                              const orm::DbClientPtr &dbClient);

    /**
     * 生成温馨的推荐理由
     * 例如："你们都在寻找内心的平静"
     */
    std::string generateRecommendationReason(const std::string &mood1,
                                            const std::string &mood2,
                                            const std::string &relationshipType);

    /**
     * 计算热门内容（内部方法）
     */
    void calculateTrendingContent(std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
