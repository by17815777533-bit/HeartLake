/**
 * 咨询室控制器 - 端到端加密心理咨询通信
 *
 * 基于 E2EE（端到端加密）实现用户与咨询师之间的安全通信。
 * 消息在客户端加密后传输，服务端仅存储密文，无法读取明文内容。
 * 密钥交换采用 X25519 ECDH 协议，确保前向安全性。
 *
 * 所有端点经 SecurityAuditFilter 进行 PASETO 令牌校验。
 */

#pragma once

#include <drogon/HttpController.h>

namespace heartlake {
namespace api {

/**
 * @brief E2EE 加密咨询室 HTTP 控制器
 *
 * @details 提供五个端点覆盖咨询会话的完整生命周期：
 * - 创建会话 → 密钥交换 → 发送加密消息 → 查看消息 → 查看会话列表
 *
 * @note 消息内容在客户端加密，服务端只做密文中转和持久化，
 *       不具备解密能力，符合零知识隐私设计。
 */
class ConsultationController : public drogon::HttpController<ConsultationController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ConsultationController::createSession, "/api/consultation/session", drogon::Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(ConsultationController::exchangeKey, "/api/consultation/key-exchange", drogon::Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(ConsultationController::sendMessage, "/api/consultation/message", drogon::Post, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(ConsultationController::getMessages, "/api/consultation/messages/{sessionId}", drogon::Get, "heartlake::filters::SecurityAuditFilter");
    ADD_METHOD_TO(ConsultationController::getSessions, "/api/consultation/sessions", drogon::Get, "heartlake::filters::SecurityAuditFilter");
    METHOD_LIST_END

    /**
     * @brief 创建咨询会话
     * @details POST /api/consultation/session
     *
     * 请求体: { "counselor_id": "咨询师用户ID" }
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 会话ID，用于后续密钥交换和消息收发
     */
    void createSession(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief X25519 密钥交换
     * @details POST /api/consultation/key-exchange
     *
     * 双方各自提交公钥，服务端中转后双方可独立计算共享密钥。
     * 请求体: { "session_id": "...", "public_key": "base64编码公钥" }
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void exchangeKey(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief 发送加密消息
     * @details POST /api/consultation/message
     *
     * 请求体: { "session_id": "...", "ciphertext": "base64密文", "nonce": "..." }
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     */
    void sendMessage(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief 获取会话消息列表
     * @details GET /api/consultation/messages/{sessionId}?page=1&page_size=50
     *
     * 返回密文消息列表，客户端自行解密。
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @param sessionId 会话ID（路径参数）
     */
    void getMessages(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& sessionId);

    /**
     * @brief 获取当前用户的咨询会话列表
     * @details GET /api/consultation/sessions
     *
     * @param req HTTP 请求
     * @param callback 响应回调
     * @return 会话列表，包含会话状态、对方信息、最后消息时间
     */
    void getSessions(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace api
} // namespace heartlake
