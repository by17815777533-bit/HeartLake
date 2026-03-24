/**
 * 守望者控制器 - 灯火转赠、情绪洞察与 AI 陪伴对话
 *
 * "守望者"是 HeartLake 的激励与陪伴系统：
 * - 守望统计：展示共鸣点、高质量涟漪、温暖纸船和灯火转赠资格
 * - 灯火转赠：达到条件的守望者可向其他用户传递灯火
 * - 情绪洞察：基于用户近期行为生成个性化情绪分析报告
 * - AI 陪伴对话：与“湖神”进行心理支持对话，受 RateLimitFilter 保护
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
     * 返回 `resonance_points / quality_ripples / warm_boats / is_guardian /
     * can_transfer_lamp / role` 等稳定字段。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getStats(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);

    /**
     * @brief 灯火转赠
     * @details POST /api/guardian/transfer-lamp
     *
     * 请求体: { "to_user_id": "接收者ID" }
     * 当前版本固定转赠一盏灯火，由服务端校验守望者资格。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @retval 400 余额不足或目标用户不存在
     */
    void transferLamp(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);

    /**
     * @brief 获取个性化情绪洞察报告
     * @details GET /api/guardian/insights
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
     * 请求体: { "content": "用户消息", "emotion": "sad", "emotion_score": 0.3 }
     * 受 RateLimitFilter 限流保护，防止 API 滥用。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void chat(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};

} // namespace heartlake::controllers
