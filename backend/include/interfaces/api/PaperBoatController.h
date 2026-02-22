/**
 * @file PaperBoatController.h
 * @brief PaperBoatController 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * 漂流纸船控制器 v2.0
 * 支持: 随机漂流、定向漂流、慢递机制、AI自动回复
 */
/**
 * @brief 纸船相关的HTTP控制器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class PaperBoatController : public drogon::HttpController<PaperBoatController> {
public:
    METHOD_LIST_BEGIN


    ADD_METHOD_TO(PaperBoatController::sendBoat, "/api/boats/drift", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(PaperBoatController::replyToStone, "/api/boats/reply", Post, "heartlake::filters::SecurityAuditFilter");


    ADD_METHOD_TO(PaperBoatController::catchBoat, "/api/boats/catch", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(PaperBoatController::respondToBoat, "/api/boats/{1}/respond", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(PaperBoatController::releaseBoat, "/api/boats/{1}/release", Post, "heartlake::filters::SecurityAuditFilter");


    ADD_METHOD_TO(PaperBoatController::getBoatDetail, "/api/boats/{1}", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(PaperBoatController::getMySentBoats, "/api/boats/sent", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(PaperBoatController::getMyReceivedBoats, "/api/boats/received", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(PaperBoatController::getDriftingCount, "/api/boats/drifting/count", Get, "heartlake::filters::SecurityAuditFilter");


    ADD_METHOD_TO(PaperBoatController::getBoatStatus, "/api/boats/{1}/status", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END
    
    /**
     * 放纸船
     * POST /api/boats/drift
     * Body: {
     *   "content": "想对世界说的话...",
     *   "mood": "hopeful",
     *   "drift_mode": "random",  // random | directed | wish
     *   "receiver_id": null,     // 定向漂流时指定
     *   "boat_style": "paper"    // paper | origami | lotus
     * }
     */
    void sendBoat(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);
    
    /**
     * 回复石头（创建关联纸船）
     * POST /api/boats/reply
     * Body: { "stone_id": "...", "content": "...", "mood": "..." }
     */
    void replyToStone(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
    
    /**
     * 捞纸船
     * POST /api/boats/catch
     * Response: { "boat": { ... }, "is_ai_reply": false }
     */
    void catchBoat(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);
    
    /**
     * 回应纸船
     * POST /api/boats/{boat_id}/respond
     * Body: { "content": "回复内容..." }
     */
    void respondToBoat(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      const std::string &boatId);
    
    /**
     * 扔回水中
     * POST /api/boats/{boat_id}/release
     */
    void releaseBoat(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    const std::string &boatId);
    
    /**
     * 获取纸船详情
     * GET /api/boats/{boat_id}
     */
    void getBoatDetail(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      const std::string &boatId);
    
    /**
     * 获取我发送的纸船
     * GET /api/boats/sent?page=1&page_size=20&status=all
     */
    void getMySentBoats(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);
    
    /**
     * 获取我收到的纸船
     * GET /api/boats/received?page=1&page_size=20
     */
    void getMyReceivedBoats(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);
    
    /**
     * 获取漂流中纸船数量
     * GET /api/boats/drifting/count
     */
    void getDriftingCount(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
    
    /**
     * 获取纸船状态
     * GET /api/boats/{boat_id}/status
     * Response: {
     *   "status": "drifting",
     *   "scheduled_delivery_at": "...",
     *   "caught_by": null,
     *   "ai_replied": false
     * }
     */
    void getBoatStatus(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      const std::string &boatId);

private:
    int calculateDriftDelay();
    int calculateSoulDistanceDelay(const std::string& content, const std::string& targetStoneId);
};

} // namespace controllers
} // namespace heartlake
