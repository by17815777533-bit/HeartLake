/**
 * @file VectorSearchController.h
 * @brief VectorSearchController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/ai/AIService.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * 向量搜索和推荐控制器
 * 提供基于内容相似度的石头推荐功能
 */
/**
 * @brief 向量搜索相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class VectorSearchController : public drogon::HttpController<VectorSearchController> {
public:
    METHOD_LIST_BEGIN
    
    ADD_METHOD_TO(VectorSearchController::getSimilarStones,
                  "/api/recommendations/similar-stones/{1}", Get);
    
    ADD_METHOD_TO(VectorSearchController::getPersonalizedRecommendations,
                  "/api/recommendations/personalized", Get);
    
    ADD_METHOD_TO(VectorSearchController::updateStoneEmbedding,
                  "/api/admin/stones/{1}/embedding", Post);
    
    METHOD_LIST_END
    
    void getSimilarStones(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback,
                         const std::string &stoneId);
    
    void getPersonalizedRecommendations(const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback);
    
    void updateStoneEmbedding(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback,
                             const std::string &stoneId);
};

} // namespace controllers
} // namespace heartlake
