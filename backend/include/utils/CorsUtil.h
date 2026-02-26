/**
 * @file CorsUtil.h
 * @brief CORS 响应头工具 — 统一管理跨域配置，避免各 Filter 重复实现
 * Created by 白洋
 */
#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <set>
#include <string>
#include <sstream>
#include <cstdlib>
#include <drogon/drogon.h>

namespace heartlake {
namespace utils {

class CorsUtil {
public:
    /// 从环境变量 CORS_ALLOWED_ORIGIN 读取白名单（逗号分隔），不支持通配符
    static const std::set<std::string>& getAllowedOrigins() {
        static const std::set<std::string> origins = [] {
            std::set<std::string> result;
            const char* env = std::getenv("CORS_ALLOWED_ORIGIN");
            std::string raw = env ? env : "http://localhost:5173,http://localhost:3000,http://localhost:8080";
            std::istringstream ss(raw);
            std::string token;
            while (std::getline(ss, token, ',')) {
                token.erase(0, token.find_first_not_of(" "));
                if (!token.empty()) {
                    token.erase(token.find_last_not_of(" ") + 1);
                }
                if (token == "*") {
                    LOG_WARN << "[CORS] 忽略通配符 '*'，请显式列出允许的域名";
                    continue;
                }
                if (!token.empty()) {
                    result.insert(token);
                }
            }
            return result;
        }();
        return origins;
    }

    /// 判断请求 Origin 是否在白名单中
    static bool isOriginAllowed(const std::string& origin) {
        if (origin.empty()) return false;
        return getAllowedOrigins().count(origin) > 0;
    }

    /// 为响应添加 CORS 头，仅当 Origin 在白名单中时生效
    static void addCorsHeaders(const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
        const std::string origin = req->getHeader("Origin");
        if (!isOriginAllowed(origin)) return;

        resp->addHeader("Access-Control-Allow-Origin", origin);
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type,Authorization,X-User-Id,X-Request-Id");
        resp->addHeader("Access-Control-Allow-Credentials", "true");
        resp->addHeader("Vary", "Origin");
    }
};

} // namespace utils
} // namespace heartlake
