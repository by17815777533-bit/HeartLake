/**
 * 石头控制器 - 心湖核心内容载体的 CRUD 与湖面数据
 *
 * "石头"是 HeartLake 的核心概念：用户将心事写在石头上投入心湖，
 * 其他用户可以在湖面看到漂浮的石头，产生共鸣后发送涟漪或纸船。
 *
 * 本控制器负责：
 * - 投石（创建石头，附带情绪标签和内容）
 * - 湖面石头流（公开浏览，支持分页和情绪筛选）
 * - 个人石头管理（查看/删除自己的石头）
 * - 湖面天气（基于全站情绪聚合的氛围指标）
 * - 情绪共鸣搜索（基于向量相似度查找共鸣石头）
 *
 * 公开端点无需认证；写操作和个人数据端点经 SecurityAuditFilter 校验。
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 石头 HTTP 控制器
 *
 * @details 路由表：
 * | 方法   | 路径                         | 权限   | 说明              |
 * |--------|------------------------------|-------|------------------|
 * | POST   | /api/stones                  | 登录   | 投石              |
 * | GET    | /api/lake/stones             | 公开   | 湖面石头流         |
 * | GET    | /api/stones/my               | 登录   | 我的石头列表       |
 * | GET    | /api/stones/{id}             | 公开   | 石头详情           |
 * | DELETE | /api/stones/{id}             | 登录   | 删除自己的石头      |
 * | GET    | /api/lake/weather            | 公开   | 湖面天气           |
 * | GET    | /api/stones/{id}/resonance   | 登录   | 情绪共鸣搜索       |
 */
class StoneController : public drogon::HttpController<StoneController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(StoneController::createStone, "/api/stones", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(StoneController::getStones, "/api/lake/stones", Get);

    ADD_METHOD_TO(StoneController::getMyStones, "/api/stones/my", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(StoneController::getStoneById, "/api/stones/{1}", Get);

    ADD_METHOD_TO(StoneController::deleteStone, "/api/stones/{1}", Delete, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(StoneController::getLakeWeather, "/api/lake/weather", Get);

    // 健康检查已迁移到 HealthController（/api/health + /api/health/detailed）

    ADD_METHOD_TO(StoneController::searchResonance, "/api/stones/{1}/resonance", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    /**
     * @brief 投石（创建石头）
     * @details POST /api/stones
     *
     * 请求体:
     * @code
     * {
     *   "content": "心事内容",
     *   "stone_type": "small|medium|large|light|heavy",
     *   "stone_color": "#7A92A3",
     *   "mood_type": "calm|happy|sad|anxious|angry|surprised|confused|neutral",
     *   "tags": ["标签1", "标签2"],
     *   "is_anonymous": true
     * }
     * @endcode
     *
     * `stone_type / stone_color / mood_type` 未显式传入时分别默认回落到
     * `medium / #7A92A3 / calm`。创建后自动触发情感分析、内容审核和向量索引更新。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void createStone(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取湖面石头流
     * @details GET /api/lake/stones?page=1&page_size=20&mood=calm
     *
     * 公开端点，支持按情绪类型筛选和分页。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getStones(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取石头详情
     * @details GET /api/stones/{stoneId}
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param stoneId 石头ID
     */
    void getStoneById(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &stoneId);

    /**
     * @brief 获取我的石头列表
     * @details GET /api/stones/my?page=1&page_size=20
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getMyStones(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 删除自己的石头
     * @details DELETE /api/stones/{stoneId}
     *
     * 仅允许删除自己投出的石头，同时清理关联的涟漪和纸船。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param stoneId 石头ID
     */
    void deleteStone(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &stoneId);

    /**
     * @brief 获取湖面天气
     * @details GET /api/lake/weather
     *
     * 基于近期全站情绪分布计算的氛围指标，
     * 包含主导情绪、情绪温度、活跃度等。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getLakeWeather(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 情绪共鸣搜索
     * @details GET /api/stones/{stoneId}/resonance?limit=10
     *
     * 基于 HNSW 向量索引查找与指定石头情绪最相似的内容。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param stoneId 基准石头ID
     */
    void searchResonance(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        const std::string &stoneId);
};

} // namespace controllers
} // namespace heartlake
