/**
 * VIP 控制器 - 灯火计划权益查询与心理咨询预约
 *
 * 前端展示层称其为“灯火计划”，接口层仍保留 `/api/vip/` 路径前缀：
 * - 灯火状态与等级查询
 * - 权益列表（专属标识、优先推荐、免费咨询等）
 * - 心理咨询额度检查与预约
 * - AI 智能评论频率配置（灯火用户享有更高频率）
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief VIP 会员 HTTP 控制器
 *
 * @details 路由表：
 * | 方法 | 路径                          | 说明                    |
 * |------|------------------------------|------------------------|
 * | GET  | /api/vip/status              | 查询 VIP 状态与等级      |
 * | GET  | /api/vip/privileges          | 获取 VIP 权益列表        |
 * | GET  | /api/vip/counseling/check    | 检查免费咨询额度         |
 * | POST | /api/vip/counseling/book     | 预约心理咨询             |
 * | GET  | /api/vip/ai-comment-frequency| 获取 AI 评论频率配置     |
 */
class VIPController : public HttpController<VIPController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(VIPController::getVIPStatus, "/api/vip/status", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(VIPController::getPrivileges, "/api/vip/privileges", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(VIPController::checkFreeCounseling, "/api/vip/counseling/check", Get, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(VIPController::bookCounseling, "/api/vip/counseling/book", Post, "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(VIPController::getAICommentFrequency, "/api/vip/ai-comment-frequency", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    /**
     * @brief 查询当前用户的 VIP 状态
     * @details GET /api/vip/status
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return VIP 等级、到期时间、累计天数等
     */
    void getVIPStatus(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * @brief 获取 VIP 权益列表
     * @details GET /api/vip/privileges
     *
     * 返回当前等级可享受的全部权益及使用状态。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void getPrivileges(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * @brief 检查是否有免费心理咨询额度
     * @details GET /api/vip/counseling/check
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 剩余免费次数、本月已用次数、下次重置时间
     */
    void checkFreeCounseling(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * @brief 预约心理咨询
     * @details POST /api/vip/counseling/book
     *
     * 请求体: { "counselor_id": "...", "time_slot": "2025-06-15T14:00:00Z" }
     * 预约成功后扣减免费额度，并创建 E2EE 咨询会话。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @retval 400 额度不足或时间冲突
     */
    void bookCounseling(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * @brief 获取 AI 智能评论频率配置
     * @details GET /api/vip/ai-comment-frequency
     *
     * VIP 用户享有更高的 AI 评论触发频率。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 当前频率配置、VIP 加成倍率
     */
    void getAICommentFrequency(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );
};

} // namespace controllers
} // namespace heartlake
