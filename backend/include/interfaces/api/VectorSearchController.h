/**
 * 向量搜索控制器 - 基于 HNSW 的语义相似度推荐
 *
 * 利用 AdvancedEmbeddingEngine 生成的文本向量，
 * 通过 HNSW 索引实现石头内容的语义相似度检索。
 * 支持两种推荐模式：
 * - 基于单个石头的相似内容推荐
 * - 基于用户历史偏好的个性化推荐
 *
 * 公开端点经 SecurityAuditFilter 校验；
 * 管理端点额外经 AdminAuthFilter 鉴权。
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/ai/AIService.h"
#include "infrastructure/filters/SecurityAuditFilter.h"
#include "infrastructure/filters/AdminAuthFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 向量搜索与推荐 HTTP 控制器
 *
 * @details 路由表：
 * | 方法 | 路径                                      | 权限     | 说明                 |
 * |------|------------------------------------------|---------|---------------------|
 * | GET  | /api/recommendations/similar-stones/{id} | 用户     | 查找相似石头          |
 * | GET  | /api/recommendations/personalized        | 用户     | 个性化推荐            |
 * | POST | /api/admin/stones/{id}/embedding         | 管理员   | 手动更新石头向量       |
 */
class VectorSearchController : public drogon::HttpController<VectorSearchController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(VectorSearchController::getSimilarStones,
                  "/api/recommendations/similar-stones/{1}", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(VectorSearchController::getPersonalizedRecommendations,
                  "/api/recommendations/personalized", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(VectorSearchController::updateStoneEmbedding,
                  "/api/admin/stones/{1}/embedding", Post, "heartlake::filters::AdminAuthFilter");

    METHOD_LIST_END

    /**
     * @brief 获取与指定石头语义相似的石头列表
     * @details GET /api/recommendations/similar-stones/{stoneId}?limit=10
     *
     * 从 HNSW 索引中检索 topK 近邻，按余弦相似度降序排列。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param stoneId 基准石头ID
     * @return 相似石头列表，包含相似度分数
     */
    void getSimilarStones(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback,
                         const std::string &stoneId);

    /**
     * @brief 获取个性化推荐石头
     * @details GET /api/recommendations/personalized?limit=20
     *
     * 综合用户历史交互（涟漪、纸船、浏览）生成偏好向量，
     * 在 HNSW 索引中检索最匹配的内容。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 推荐石头列表，包含推荐理由
     */
    void getPersonalizedRecommendations(const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 手动更新石头的嵌入向量（管理员）
     * @details POST /api/admin/stones/{stoneId}/embedding
     *
     * 重新计算指定石头的文本嵌入并更新 HNSW 索引。
     * 适用于内容被编辑后需要刷新向量的场景。
     *
     * @param req HTTP 请求（需携带管理员 PASETO 令牌）
     * @param callback 响应回调
     * @param stoneId 目标石头ID
     */
    void updateStoneEmbedding(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback,
                             const std::string &stoneId);
};

} // namespace controllers
} // namespace heartlake
