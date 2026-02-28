/**
 * 纸船控制器 - 石头评论（纸船）的收发与查看
 *
 * "纸船"是 HeartLake 中对石头的回复载体：用户看到湖面上的石头后，
 * 可以折一只纸船写上回复内容，漂向石头的主人。
 *
 * 本控制器仅保留石头评论纸船链路，提供：
 * - 回复石头（创建纸船）
 * - 查看纸船详情
 * - 查看我发送/收到的纸船列表
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
 *
 * @note 纸船的创建也可通过 InteractionController::createBoat 完成，
 *       本控制器提供更语义化的独立入口。
 */

#pragma once

#include <drogon/HttpController.h>
#include "infrastructure/filters/SecurityAuditFilter.h"

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 纸船 HTTP 控制器
 *
 * @details 路由表：
 * | 方法 | 路径                | 说明             |
 * |------|--------------------|-----------------|
 * | POST | /api/boats/reply   | 回复石头（创建纸船）|
 * | GET  | /api/boats/{id}    | 纸船详情          |
 * | GET  | /api/boats/sent    | 我发送的纸船       |
 * | GET  | /api/boats/received| 我收到的纸船       |
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
     * @brief 回复石头（创建关联纸船）
     * @details POST /api/boats/reply
     *
     * 请求体:
     * @code
     * {
     *   "stone_id": "目标石头ID",
     *   "content": "回复内容",
     *   "mood": "calm"  // 可选，回复时的情绪
     * }
     * @endcode
     *
     * 创建后自动通知石头作者，并广播 boat_update 事件。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void replyToStone(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取纸船详情
     * @details GET /api/boats/{boatId}
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param boatId 纸船ID
     */
    void getBoatDetail(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      const std::string &boatId);

    /**
     * @brief 获取我发送的纸船列表
     * @details GET /api/boats/sent?page=1&page_size=20&status=all
     *
     * @param req HTTP 请求，支持 status 筛选（all/read/unread）
     * @param callback 响应回调
     */
    void getMySentBoats(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取我收到的纸船列表
     * @details GET /api/boats/received?page=1&page_size=20
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getMyReceivedBoats(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace controllers
} // namespace heartlake
