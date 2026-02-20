/**
 * @file SecurityAuditFilter.cpp
 * @brief SecurityAuditFilter 实现
 */

#include "infrastructure/filters/SecurityAuditFilter.h"
#include "utils/PasetoUtil.h"
#include "utils/ResponseUtil.h"
#include <vector>
#include <string>

namespace heartlake {
namespace filters {

void SecurityAuditFilter::doFilter(const drogon::HttpRequestPtr& req,
                                   drogon::FilterCallback&& fcb,
                                   drogon::FilterChainCallback&& fccb) {
    static const std::vector<std::string> whitelist = {
        "/api/auth/login",
        "/api/auth/register",
        "/api/auth/anonymous",
        "/api/lake/stones",
        "/api/lake/weather",
        "/api/health",
        "/ws/broadcast"
    };

    const std::string path = req->path();

    for (const auto& prefix : whitelist) {
        if (path.find(prefix) == 0) {
            fccb();
            return;
        }
    }

    const std::string authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
        fcb(utils::ResponseUtil::unauthorized("未授权访问"));
        return;
    }

    const std::string token = authHeader.substr(7);

    try {
        std::string key = utils::PasetoUtil::getKey();
        std::string userId = utils::PasetoUtil::verifyToken(token, key);

        req->addHeader("X-User-Id", userId);
        req->getAttributes()->insert("user_id", userId);
        fccb();
    } catch (const std::exception& e) {
        fcb(utils::ResponseUtil::unauthorized("Token无效或已过期"));
    }
}

} // namespace filters
} // namespace heartlake
