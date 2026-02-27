/**
 * VIP控制器接口定义
 */

#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * VIP控制器
 *
 * 处理VIP相关的HTTP请求，包括：
 * - 查询VIP状态
 * - 获取VIP权益列表
 * - 预约心理咨询
 * - 检查权益使用情况
 */
class VIPController : public HttpController<VIPController> {
public:
    METHOD_LIST_BEGIN

    // 获取用户VIP状态
    ADD_METHOD_TO(VIPController::getVIPStatus, "/api/vip/status", Get, "heartlake::filters::SecurityAuditFilter");

    // 获取用户VIP权益列表
    ADD_METHOD_TO(VIPController::getPrivileges, "/api/vip/privileges", Get, "heartlake::filters::SecurityAuditFilter");

    // 检查是否有免费心理咨询额度
    ADD_METHOD_TO(VIPController::checkFreeCounseling, "/api/vip/counseling/check", Get, "heartlake::filters::SecurityAuditFilter");

    // 预约心理咨询
    ADD_METHOD_TO(VIPController::bookCounseling, "/api/vip/counseling/book", Post, "heartlake::filters::SecurityAuditFilter");

    // 获取AI评论频率
    ADD_METHOD_TO(VIPController::getAICommentFrequency, "/api/vip/ai-comment-frequency", Get, "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    /**
     * 获取用户VIP状态
     */
    void getVIPStatus(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * 获取用户VIP权益列表
     */
    void getPrivileges(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * 检查是否有免费心理咨询额度
     */
    void checkFreeCounseling(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * 预约心理咨询
     */
    void bookCounseling(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    /**
     * 获取AI评论频率
     */
    void getAICommentFrequency(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );
};

} // namespace controllers
} // namespace heartlake
