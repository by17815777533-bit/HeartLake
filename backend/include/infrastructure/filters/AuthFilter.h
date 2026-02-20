/**
 * @file AuthFilter.h
 * @brief AuthFilter 模块接口定义
 * Created by 白洋
 */

#pragma once

#include <drogon/drogon.h>
#include <drogon/HttpFilter.h>
#include "utils/PasetoUtil.h"
#include "utils/ResponseUtil.h"

namespace heartlake {
namespace filters {

/**
 * @brief 认证过滤器，用于验证用户身份
 * @deprecated 请使用 SecurityAuditFilter 替代，它提供更完整的安全功能
 */
class [[deprecated("Use SecurityAuditFilter instead")]] AuthFilter : public drogon::HttpFilter<AuthFilter> {
public:
    static const char* classTypeName() { return "heartlake::filters::AuthFilter"; }
    void doFilter(const drogon::HttpRequestPtr &req,
                 drogon::FilterCallback &&fcb,
                 drogon::FilterChainCallback &&fccb) override {
        try {
            std::string token = utils::PasetoUtil::extractToken(req);
            std::string key = utils::PasetoUtil::getKey();
            std::string user_id = utils::PasetoUtil::verifyToken(token, key);
            
            req->addHeader("X-User-Id", user_id);
            
            fccb();
            
        } catch (const std::exception &e) {
            LOG_WARN << "Auth failed: " << e.what();
            auto resp = utils::ResponseUtil::unauthorized("认证失败，请重新登录");
            fcb(resp);
        }
    }
};

} // namespace filters
} // namespace heartlake
