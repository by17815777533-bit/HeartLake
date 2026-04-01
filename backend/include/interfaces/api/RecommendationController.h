/**
 * 推荐控制器 - 多算法融合的内容推荐与情绪发现
 *
 * 实现 HeartLake 的智能推荐系统，融合多种推荐策略：
 * - 协同过滤（基于用户行为相似度）
 * - 内容过滤（基于石头文本语义相似度）
 * - 随机探索（保证推荐多样性，避免信息茧房）
 * - 高级算法：UCB 探索-利用平衡、Thompson Sampling、MMR 多样性重排
 *
 * 同时提供情绪发现、趋势追踪、交互记录等辅助功能，
 * 构成完整的推荐闭环：推荐 → 交互 → 学习 → 优化推荐。
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
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
 * @brief 智能推荐 HTTP 控制器
 *
 * @details 路由表：
 * | 方法 | 路径                                  | 说明                          |
 * |------|--------------------------------------|------------------------------|
 * | GET  | /api/recommendations/stones          | 混合推荐石头                   |
 * | GET  | /api/recommendations/discover/{mood} | 按情绪发现内容                 |
 * | GET  | /api/discover/{mood}                 | 同上（兼容路由）               |
 * | POST | /api/recommendations/track           | 记录单次交互                   |
 * | POST | /api/recommendations/track-batch     | 批量记录交互                   |
 * | GET  | /api/recommendations/emotion-trends  | 个人情绪趋势                   |
 * | GET  | /api/recommendations/trending        | 热门内容                       |
 * | POST | /api/recommendations/search          | 语义搜索推荐                   |
 * | GET  | /api/recommendations/advanced        | 高级多算法推荐                 |
 * | GET  | /api/admin/recommendations/advanced  | 管理端查看指定用户高级推荐      |
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

    ADD_METHOD_TO(RecommendationController::getAdminAdvancedRecommendations,
                  "/api/admin/recommendations/advanced", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    METHOD_LIST_END

    /**
     * @brief 获取混合推荐石头
     * @details GET /api/recommendations/stones?limit=20
     *
     * 算法权重：协同过滤 40% + 内容过滤 40% + 随机探索 20%。
     * 对新用户自动降级为热门推荐 + 随机探索。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getRecommendedStones(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 按情绪发现内容
     * @details GET /api/recommendations/discover/{mood}?limit=20
     *
     * 发现当前处于指定情绪的石头和用户，
     * 帮助用户找到情绪共鸣的内容。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param mood 情绪类型（happy/calm/sad/anxious/angry/surprised/confused/neutral）
     */
    void discoverByMood(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       const std::string &mood);

    /**
     * @brief 记录用户交互行为
     * @details POST /api/recommendations/track
     *
     * 请求体: { "stone_id": "...", "action": "view|ripple|boat|share", "duration_ms": 3000 }
     * 交互数据用于更新用户偏好模型。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void trackInteraction(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取个人情绪趋势
     * @details GET /api/recommendations/emotion-trends?days=30
     *
     * 基于用户投石和交互历史，展示情绪变化曲线。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getEmotionTrends(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取热门内容
     * @details GET /api/recommendations/trending?limit=20
     *
     * 基于近期涟漪数、纸船数、浏览量综合排序。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getTrendingContent(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 语义搜索推荐
     * @details POST /api/recommendations/search
     *
     * 请求体: { "query": "搜索关键词", "limit": 20 }
     * 全文搜索 + 向量语义匹配，结果按相关性智能排序。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void searchRecommendations(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 批量记录交互行为
     * @details POST /api/recommendations/track-batch
     *
     * 请求体: { "interactions": [{ "stone_id": "...", "action": "..." }, ...] }
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void trackBatchInteractions(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 高级多算法融合推荐
     * @details GET /api/recommendations/advanced?limit=20
     *
     * 使用 UCB（探索-利用平衡）、Thompson Sampling（贝叶斯采样）、
     * MMR（最大边际相关性）等高级算法，在推荐质量和多样性之间取得平衡。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getAdvancedRecommendations(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 管理端查看指定用户的高级多算法推荐
     * @details GET /api/admin/recommendations/advanced?user_id=xxx&limit=20
     *
     * 用于运营/排障侧直接核对高级算法对指定用户的真实产出，
     * 返回字段与移动端高级推荐保持一致，并额外标记 inspected_user_id。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getAdminAdvancedRecommendations(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback);

private:
    /**
     * @brief 混合推荐算法核心实现
     * @param userId 当前用户ID
     * @param dbClient 数据库连接
     * @param callback 响应回调
     */
    void calculateStoneRecommendations(const std::string &userId,
                                      int limit,
                                      const orm::DbClientPtr &dbClient,
                                      std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 基于最近交互自动更新用户偏好向量
     * @param userId 用户ID
     * @param dbClient 数据库连接
     */
    void updateUserPreferences(const std::string &userId,
                              const orm::DbClientPtr &dbClient);

    /**
     * @brief 生成温馨的推荐理由文案
     * @param mood1 用户当前情绪
     * @param mood2 推荐石头的情绪
     * @param relationshipType 推荐关系类型（similar/complementary/trending）
     * @return 推荐理由文案，如"你们都在寻找内心的平静"
     */
    std::string generateRecommendationReason(const std::string &mood1,
                                            const std::string &mood2,
                                            const std::string &relationshipType);

    /**
     * @brief 计算热门内容排行（内部方法）
     * @param callback 响应回调
     */
    void calculateTrendingContent(int limit,
                                  std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
