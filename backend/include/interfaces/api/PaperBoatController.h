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
 * @brief 纸船相关的HTTP控制器
 *
 * 仅保留石头评论纸船链路：
 * - 回复石头
 * - 查看纸船详情
 * - 查看我发送/收到的纸船
 */
class PaperBoatController : public drogon::HttpController<PaperBoatController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(PaperBoatController::replyToStone, "/api/boats/reply", Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(PaperBoatController::getBoatDetail, "/api/boats/{1}", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(PaperBoatController::getMySentBoats, "/api/boats/sent", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(PaperBoatController::getMyReceivedBoats, "/api/boats/received", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END
    
    /**
     * 回复石头（创建关联纸船）
     * POST /api/boats/reply
     * Body: { "stone_id": "...", "content": "...", "mood": "..." }
     */
    void replyToStone(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
    
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
};

} // namespace controllers
} // namespace heartlake
