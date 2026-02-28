/**
 * @brief 安全审计过滤器 - 全接口 PASETO 校验、请求审计与响应脱敏
 *
 * @details
 * 作为 HeartLake 的核心安全组件，挂载在绝大多数 API 端点上，
 * 承担三重职责：
 * 1. PASETO v4 令牌校验 -- 验证用户身份，提取 user_id 注入 request attributes
 * 2. 请求审计日志 -- 记录请求路径、方法、用户ID（IP 和设备指纹经匿名化处理）
 * 3. 响应脱敏 -- 自动检测并脱敏响应 JSON 中的敏感字段（邮箱、手机号等）
 *
 * 白名单路径（如 /api/health、/api/auth/anonymous）跳过令牌校验。
 */

#pragma once

#include <drogon/drogon.h>
#include <drogon/HttpFilter.h>
#include "utils/PasetoUtil.h"
#include "utils/ResponseUtil.h"
#include "utils/IdentityShadowMap.h"
#include "utils/SecurityLogger.h"

namespace heartlake {
namespace filters {

/**
 * @brief 安全审计过滤器
 *
 * @details 在 Drogon 请求管道中作为前置过滤器运行。
 * 通过 classTypeName() 注册为 "heartlake::filters::SecurityAuditFilter"。
 *
 * 处理流程：
 * 1. 检查请求路径是否在白名单中
 * 2. 从 Authorization 头提取 PASETO 令牌
 * 3. 调用 PasetoUtil 验证令牌，提取 user_id
 * 4. 将 user_id 写入 request attributes
 * 5. 调用 logRequest 记录审计日志
 * 6. 放行请求到下游 Controller
 */
class SecurityAuditFilter : public drogon::HttpFilter<SecurityAuditFilter> {
public:
    static const char* classTypeName() { return "heartlake::filters::SecurityAuditFilter"; }

    /**
     * @brief 执行安全审计过滤逻辑
     *
     * @param req 入站 HTTP 请求
     * @param fcb 过滤失败回调 — 令牌无效时返回 401
     * @param fccb 过滤通过回调 — 令牌有效时放行
     */
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;

private:
    /**
     * @brief 记录请求审计日志
     * @details IP 地址经 IdentityShadowMap 匿名化处理后记录，
     *          不存储原始 IP，符合隐私保护要求。
     * @param req HTTP 请求
     * @param userId 从令牌中解析出的用户ID
     */
    void logRequest(const drogon::HttpRequestPtr& req, const std::string& userId);
};

/**
 * @brief 响应敏感字段脱敏处理器
 *
 * @details 递归遍历 JSON 响应体，对匹配敏感字段名的值进行脱敏。
 * 支持的敏感字段：email、phone、mobile、real_name、id_card、address。
 * 脱敏策略由 IdentityShadowMap::desensitize 根据字段类型决定：
 * - email: u***@domain.com
 * - phone: 138****1234
 * - name:  张*
 */
class ResponseDesensitizer {
public:
    /**
     * @brief 递归脱敏 JSON 中的敏感字段
     * @param json 待脱敏的 JSON 值（会被原地修改）
     */
    static void desensitize(Json::Value& json) {
        if (json.isObject()) {
            for (const auto& key : json.getMemberNames()) {
                if (isSensitiveField(key)) {
                    json[key] = utils::IdentityShadowMap::desensitize(
                        json[key].asString(), getFieldType(key));
                } else {
                    desensitize(json[key]);
                }
            }
        } else if (json.isArray()) {
            for (auto& item : json) {
                desensitize(item);
            }
        }
    }

private:
    /**
     * @brief 判断字段名是否为敏感字段
     * @param field JSON 字段名
     * @return true 字段名包含敏感关键词
     */
    static bool isSensitiveField(const std::string& field) {
        static const std::vector<std::string> sensitive = {
            "email", "phone", "mobile", "real_name", "id_card", "address"
        };
        for (const auto& s : sensitive) {
            if (field.find(s) != std::string::npos) return true;
        }
        return false;
    }

    /**
     * @brief 根据字段名推断脱敏类型
     * @param field JSON 字段名
     * @return 脱敏类型标识（"email" / "phone" / "name"）
     */
    static std::string getFieldType(const std::string& field) {
        if (field.find("email") != std::string::npos) return "email";
        if (field.find("phone") != std::string::npos || field.find("mobile") != std::string::npos) return "phone";
        return "name";
    }
};

} // namespace filters
} // namespace heartlake
