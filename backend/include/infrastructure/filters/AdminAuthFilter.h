/**
 * 管理员认证过滤器 - PASETO v4 令牌校验与角色鉴权
 *
 * 拦截所有管理后台请求，从 Authorization 头中提取 PASETO v4 令牌，
 * 验证签名有效性和过期时间，解析出管理员ID和角色信息，
 * 注入到 request attributes 供下游 Controller 使用。
 *
 * 鉴权流程：
 * 1. 提取 Authorization: Bearer <token>
 * 2. 调用 PasetoUtil 验证令牌签名和有效期
 * 3. 解析 payload 中的 admin_id 和 role
 * 4. 将 admin_id / role 写入 request attributes
 * 5. 验证失败返回 401 Unauthorized
 *
 * @note OPTIONS 预检请求直接放行（CORS 支持）。
 */

#pragma once

#include <drogon/HttpFilter.h>

using namespace drogon;

namespace heartlake {
namespace filters {

/**
 * @brief 管理员 PASETO 认证过滤器
 *
 * @details 在 Drogon 请求管道中作为前置过滤器运行，
 * 挂载在所有 /api/admin/* 路由上。通过 classTypeName()
 * 注册为 "filters::AdminAuthFilter"，供路由宏引用。
 */
class AdminAuthFilter : public HttpFilter<AdminAuthFilter> {
public:
    static const char* classTypeName() { return "filters::AdminAuthFilter"; }
    AdminAuthFilter() = default;

    /**
     * @brief 执行认证过滤逻辑
     *
     * @param req 入站 HTTP 请求
     * @param fcb 过滤失败回调 — 调用此回调直接返回错误响应，跳过后续处理
     * @param fccb 过滤通过回调 — 调用此回调将请求传递给下一个过滤器或 Controller
     */
    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;

private:
    /**
     * @brief 验证管理员 PASETO 令牌
     *
     * @param token PASETO v4 令牌字符串
     * @param[out] adminId 解析出的管理员ID
     * @param[out] role 解析出的角色（super_admin / admin / operator）
     * @return true 令牌有效，adminId 和 role 已填充
     * @return false 令牌无效、过期或签名校验失败
     */
    bool verifyAdminToken(const std::string& token, std::string& adminId, std::string& role);
};

} // namespace filters
} // namespace heartlake
