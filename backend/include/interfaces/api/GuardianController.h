/**
 * 守望者控制器 - 灯火转赠、情绪洞察与 AI 陪伴对话
 *
 * "守望者"是 HeartLake 的激励与陪伴系统：
 * - 灯火转赠：用户可以将自己的"灯火"（善意积分）赠送给他人，
 *   传递温暖和鼓励
 * - 情绪洞察：基于用户近期行为数据，生成个性化的情绪分析报告
 * - AI 陪伴对话：与 AI 守望者进行心理支持对话，
 *   受 RateLimitFilter 保护防止滥用
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
 * chat 端点额外经 RateLimitFilter 限流保护。
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake::controllers {

/**
 * @brief 守望者系统 HTTP 控制器
 *
 * @details 路由表：
 * | 方法 | 路径                       | 过滤器                          | 说明           |
 * |------|---------------------------|--------------------------------|---------------|
 * | GET  | /api/guardian/stats        | SecurityAuditFilter            | 守望者统计      |
 * | GET  | /api/guardian              | SecurityAuditFilter            | 同上（简写）    |
 * | POST | /api/guardian/transfer-lamp| SecurityAuditFilter            | 灯火转赠        |
 * | GET  | /api/guardian/insights     | SecurityAuditFilter            | 情绪洞察报告    |
 * | POST | /api/guardian/chat         | SecurityAuditFilter+RateLimit  | AI 陪伴对话     |
 */
class GuardianController : public HttpController<GuardianController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(GuardianController::getStats, "/api/guardian/stats", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::getStats, "/api/guardian", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::transferLamp, "/api/guardian/transfer-lamp", Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::getEmotionInsights, "/api/guardian/insights", Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(GuardianController::chat, "/api/guardian/chat", Post, "heartlake::filters::SecurityAuditFilter", "heartlake::filters::RateLimitFilter");

    METHOD_LIST_END

    /**
     * @brief 获取守望者统计数据
     * @details GET /api/guardian/stats 或 GET /api/guardian
     *
     * 返回用户的灯火余额、累计赠送/收到数量、守望等级等。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getStats(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);

    /**
     * @brief 灯火转赠
     * @details POST /api/guardian/transfer-lamp
     *
     * 请求体: { "target_user_id": "接收者ID", "amount": 1, "message": "加油" }
     * 转赠后双方均收到 WebSocket 通知。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @retval 400 余额不足或目标用户不存在
     */
    void transferLamp(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);

    /**
     * @brief 获取个性化情绪洞察报告
     * @details GET /api/guardian/insights?days=7
     *
     * 基于用户近期投石、交互、情绪标签等数据，
     * 生成情绪变化趋势和个性化建议。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getEmotionInsights(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);

    /**
     * @brief AI 守望者陪伴对话
     * @details POST /api/guardian/chat
     *
     * 请求体: { "message": "用户消息", "context": "conversation_id" }
     * 受 RateLimitFilter 限流保护，防止 API 滥用。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void chat(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};

} // namespace heartlake::controllers
