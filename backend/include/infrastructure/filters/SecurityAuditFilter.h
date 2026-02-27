/**
 * 安全审计过滤器 - 全接口PASETO校验与敏感字段脱敏
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
 * 安全审计过滤器
 *
 * 功能：
 * 1. 全接口PASETO校验（白名单除外）
 * 2. 请求日志记录（匿名化IP和设备指纹）
 * 3. 响应敏感字段自动脱敏
 */
class SecurityAuditFilter : public drogon::HttpFilter<SecurityAuditFilter> {
public:
    static const char* classTypeName() { return "heartlake::filters::SecurityAuditFilter"; }

    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;

private:
    void logRequest(const drogon::HttpRequestPtr& req, const std::string& userId);
};

/**
 * 响应脱敏处理器
 * 用于在响应返回前自动脱敏敏感字段
 */
class ResponseDesensitizer {
public:
    /**
     * 脱敏JSON响应中的敏感字段
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
    static bool isSensitiveField(const std::string& field) {
        static const std::vector<std::string> sensitive = {
            "email", "phone", "mobile", "real_name", "id_card", "address"
        };
        for (const auto& s : sensitive) {
            if (field.find(s) != std::string::npos) return true;
        }
        return false;
    }

    static std::string getFieldType(const std::string& field) {
        if (field.find("email") != std::string::npos) return "email";
        if (field.find("phone") != std::string::npos || field.find("mobile") != std::string::npos) return "phone";
        return "name";
    }
};

} // namespace filters
} // namespace heartlake
