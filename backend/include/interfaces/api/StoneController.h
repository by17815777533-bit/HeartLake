/**
 * @file StoneController.h
 * @brief StoneController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 石头相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
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
    
    void createStone(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getStones(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);
    
    void getStoneById(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     const std::string &stoneId);
    
    void getMyStones(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
    
    void deleteStone(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &stoneId);
    
    void getLakeWeather(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);
    
    void searchResonance(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        const std::string &stoneId);
};

} // namespace controllers
} // namespace heartlake
