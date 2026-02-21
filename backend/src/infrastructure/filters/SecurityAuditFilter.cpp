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

    std::string path = req->path();

    // 路径规范化：去除连续斜杠
    while (path.find("//") != std::string::npos) {
        path.replace(path.find("//"), 2, "/");
    }
    // 拒绝包含路径遍历的请求
    if (path.find("/..") != std::string::npos) {
        fcb(utils::ResponseUtil::error(400, "非法路径"));
        return;
    }
    // 去除尾部斜杠
    if (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }

    for (const auto& prefix : whitelist) {
        if (path == prefix || (path.size() > prefix.size() && path.find(prefix) == 0 && path[prefix.size()] == '/')) {
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

        req->getAttributes()->insert("user_id", userId);
        fccb();
    } catch (const std::exception& e) {
        fcb(utils::ResponseUtil::unauthorized("Token无效或已过期"));
    }
}

} // namespace filters
} // namespace heartlake
