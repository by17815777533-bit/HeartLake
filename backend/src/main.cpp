/**
 * @file main.cpp
 * @brief main 模块实现
 * Created by 白洋
 */
#include <drogon/drogon.h>
#include "utils/PasetoUtil.h"
#include "utils/RBACManager.h"
#include <iostream>
#include <ctime>
#include <set>
#include <sstream>

#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/ai/ImageModerationEngine.h"
#include "middleware/RateLimiter.h"
#include "utils/ContentFilter.h"
#include "infrastructure/services/FriendshipTTLEngine.h"
#include "infrastructure/services/LakeGodGuardianService.h"
#include "infrastructure/services/ResonanceSearchService.h"
#include "infrastructure/services/EmotionTrackingService.h"
#include "infrastructure/services/UserFollowUpService.h"
#include "infrastructure/cache/RedisCache.h"
#include "infrastructure/ArchitectureBootstrap.h"
#include "interfaces/api/BroadcastWebSocketController.h"

using namespace drogon;

static HttpResponsePtr createErrorResponse(int code, const std::string& message) {
    Json::Value json;
    json["code"] = code;
    json["message"] = message;
    json["data"] = Json::nullValue;
    json["timestamp"] = (Json::Int64)std::time(nullptr);
    auto resp = HttpResponse::newHttpJsonResponse(json);
    // Map error code to appropriate HTTP status
    HttpStatusCode httpStatus = k500InternalServerError;
    if (code == 400) httpStatus = k400BadRequest;
    else if (code == 401) httpStatus = k401Unauthorized;
    else if (code == 403) httpStatus = k403Forbidden;
    else if (code == 404) httpStatus = k404NotFound;
    else if (code == 429) httpStatus = k429TooManyRequests;
    resp->setStatusCode(httpStatus);
    return resp;
}

int main(int argc, char *argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    std::cout << R"(
    ╔═══════════════════════════════════════╗
    ║      Heart Lake API Server v3.0       ║
    ║   一个温暖治愈的情感交流空间          ║
    ║   [DDD + Event-Driven Architecture]   ║
    ╚═══════════════════════════════════════╝
    )" << std::endl;

    LOG_INFO << "Starting Heart Lake Server...";

    try {
        // 优先使用环境变量配置路径，支持绝对路径
        const char* config_path = std::getenv("HEARTLAKE_CONFIG_PATH");
        std::string configFile = config_path ? config_path : "../config.json";
        if (argc > 1) {
            configFile = argv[1];
        }

        auto& app = drogon::app();

        // 从环境变量读取配置
        const char* db_host = std::getenv("DB_HOST");
        const char* db_port_str = std::getenv("DB_PORT");
        const char* db_name = std::getenv("DB_NAME");
        const char* db_user = std::getenv("DB_USER");
        const char* db_password = std::getenv("DB_PASSWORD");
        const char* db_pool_size_str = std::getenv("DB_CONNECTION_POOL_SIZE");
        const char* db_timeout_str = std::getenv("DB_TIMEOUT");

        const char* server_host = std::getenv("SERVER_HOST");
        const char* server_port_str = std::getenv("SERVER_PORT");
        const char* server_threads_str = std::getenv("SERVER_THREADS");
        const char* log_level_str = std::getenv("LOG_LEVEL");

        // 验证PASETO密钥配置 - 安全检查
        const char* paseto_key = std::getenv("PASETO_KEY");
        if (!paseto_key || std::strlen(paseto_key) < 32) {
            LOG_FATAL << "FATAL: PASETO_KEY environment variable is not set or too short (need >= 32 bytes)!";
            LOG_FATAL << "Example: export PASETO_KEY=\"your-32-byte-or-longer-secret-key\"";
            return 1;
        }
        LOG_INFO << "PASETO_KEY validation passed";

        // 使用默认值或环境变量
        [[maybe_unused]] std::string dbHost = db_host ? db_host : "127.0.0.1";
        [[maybe_unused]] int dbPort = db_port_str ? std::atoi(db_port_str) : 5432;
        [[maybe_unused]] std::string dbName = db_name ? db_name : "heartlake";
        [[maybe_unused]] std::string dbUser = db_user ? db_user : "postgres";
        [[maybe_unused]] std::string dbPassword = db_password ? db_password : "postgres";
        [[maybe_unused]] int dbPoolSize = db_pool_size_str ? std::atoi(db_pool_size_str) : 50;
        [[maybe_unused]] double dbTimeout = db_timeout_str ? std::atof(db_timeout_str) : 30.0;

        std::string serverHost = server_host ? server_host : "0.0.0.0";
        int serverPort = server_port_str ? std::atoi(server_port_str) : 8080;
        int serverThreads = server_threads_str ? std::atoi(server_threads_str) : 16;

        // 配置服务器
        app.addListener(serverHost, serverPort);
        app.setThreadNum(serverThreads);

        // 配置日志级别
        if (log_level_str) {
            std::string logLevel = log_level_str;
            if (logLevel == "TRACE") app.setLogLevel(trantor::Logger::kTrace);
            else if (logLevel == "DEBUG") app.setLogLevel(trantor::Logger::kDebug);
            else if (logLevel == "INFO") app.setLogLevel(trantor::Logger::kInfo);
            else if (logLevel == "WARN") app.setLogLevel(trantor::Logger::kWarn);
            else if (logLevel == "ERROR") app.setLogLevel(trantor::Logger::kError);
            else if (logLevel == "FATAL") app.setLogLevel(trantor::Logger::kFatal);
            else app.setLogLevel(trantor::Logger::kInfo);
        } else {
            app.setLogLevel(trantor::Logger::kInfo);
        }

        // 配置静态文件和上传路径（支持环境变量配置绝对路径）
        const char* doc_root = std::getenv("HEARTLAKE_DOCUMENT_ROOT");
        const char* upload_dir = std::getenv("HEARTLAKE_UPLOAD_PATH");
        app.setDocumentRoot(doc_root ? doc_root : "./public");
        app.setUploadPath(upload_dir ? upload_dir : "./uploads");

        // 使用配置文件加载数据库客户端
        LOG_INFO << "Loading config file...";
        app.loadConfigFile(configFile);
        LOG_INFO << "Config loaded, database client will be initialized by framework";

        // 全局异常处理器 - 捕获未处理的异常
        app.setExceptionHandler([](const std::exception& e,
                                   const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& callback) {
            LOG_ERROR << "Unhandled exception at " << req->path() << ": " << e.what();
            Json::Value json;
            json["code"] = 500;
            json["message"] = "服务器内部错误";
            json["data"] = Json::nullValue;
            json["timestamp"] = (Json::Int64)std::time(nullptr);
            auto resp = HttpResponse::newHttpJsonResponse(json);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        });

        // VUL-14 修复：CORS 支持多 Origin（逗号分隔）
        // 开发环境默认允许 admin(5173) + Flutter Web(端口范围)
        const char* cors_origin = std::getenv("CORS_ALLOWED_ORIGIN");
        std::string corsConfig = cors_origin ? cors_origin : "http://localhost:5173,http://localhost:3000,http://localhost:8080";

        // 解析允许的 Origin 列表
        std::set<std::string> allowedOrigins;
        bool allowAll = false;
        {
            std::istringstream ss(corsConfig);
            std::string token;
            while (std::getline(ss, token, ',')) {
                // trim
                token.erase(0, token.find_first_not_of(" "));
                token.erase(token.find_last_not_of(" ") + 1);
                if (token == "*") {
                    allowAll = true;
                    LOG_WARN << "CORS_ALLOWED_ORIGIN 包含通配符 *，生产环境请设置具体域名！";
                }
                if (!token.empty()) allowedOrigins.insert(token);
            }
        }
        LOG_INFO << "CORS allowed origins: " << corsConfig;

        // 匹配请求 Origin 的 lambda
        auto matchOrigin = [allowedOrigins, allowAll](const HttpRequestPtr &req) -> std::string {
            if (allowAll) return "*";
            std::string origin = req->getHeader("Origin");
            if (origin.empty()) {
                // 非浏览器请求（如 Flutter 桌面端/curl），直接放行
                return !allowedOrigins.empty() ? *allowedOrigins.begin() : "";
            }
            if (allowedOrigins.count(origin)) return origin;
            // 开发环境：允许所有 localhost 端口
            if (origin.find("http://localhost:") == 0 || origin.find("http://127.0.0.1:") == 0) {
                return origin;
            }
            return "";
        };

        // 安全响应头 + CORS 后处理中间件
        app.registerPostHandlingAdvice([matchOrigin](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
            std::string origin = matchOrigin(req);
            if (!origin.empty()) {
                resp->addHeader("Access-Control-Allow-Origin", origin);
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type,Authorization,X-User-Id,X-Request-Id");
                resp->addHeader("Access-Control-Allow-Credentials", "true");
                if (origin != "*") {
                    resp->addHeader("Vary", "Origin");
                }
            }
            // 安全响应头：防止 MIME 类型嗅探、点击劫持、强制 HTTPS
            resp->addHeader("X-Content-Type-Options", "nosniff");
            resp->addHeader("X-Frame-Options", "DENY");
            resp->addHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
            resp->addHeader("X-XSS-Protection", "1; mode=block");
            resp->addHeader("Referrer-Policy", "strict-origin-when-cross-origin");
        });

        // OPTIONS - 使用 PreRoutingAdvice 在路由匹配前处理 CORS 预检请求
        app.registerPreRoutingAdvice([matchOrigin](const HttpRequestPtr &req, AdviceCallback &&acb, AdviceChainCallback &&accb) {
            if (req->method() == Options) {
                auto resp = HttpResponse::newHttpResponse();
                std::string origin = matchOrigin(req);
                if (!origin.empty()) {
                    resp->addHeader("Access-Control-Allow-Origin", origin);
                    resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                    resp->addHeader("Access-Control-Allow-Headers", "Content-Type,Authorization,X-User-Id,X-Request-Id");
                    resp->addHeader("Access-Control-Allow-Credentials", "true");
                    if (origin != "*") {
                        resp->addHeader("Vary", "Origin");
                    }
                }
                resp->setStatusCode(k204NoContent);
                acb(resp);
                return;
            }
            accb();
        });

        // PASETO认证中间件
        app.registerPreHandlingAdvice([matchOrigin](const HttpRequestPtr &req, AdviceCallback &&acb, AdviceChainCallback &&accb) {

            std::string path = req->path();
            bool needsAuth = true;

            // 认证白名单：使用显式路径列表，避免前缀匹配导致的安全漏洞
            // 只有明确列出的路径才跳过认证
            // VUL-17 修复：从白名单中移除 /metrics，需要认证才能访问
            static const std::vector<std::string> exactWhitelist = {
                "/api/auth/register",
                "/api/auth/login",
                "/api/auth/send-code",
                "/api/auth/verify-code",
                "/api/auth/reset-password",
                "/api/v1/auth/register",
                "/api/v1/auth/login",
                "/api/v1/auth/send-code",
                "/api/v1/auth/verify-code",
                "/api/v1/auth/reset-password",
                "/api/health",
                "/api/health/detailed",
                // VUL-17: /metrics 已移除，需要认证访问
                "/api/boats/drifting/count",
            };
            // 前缀白名单：这些路径下的所有子路径都不需要认证
            static const std::vector<std::string> prefixWhitelist = {
                "/api/admin/",    // 管理后台有独立的 AdminAuthFilter
                "/api/lake/",     // 公开湖面数据
                "/api/safe-harbor/", // 安全港公开接口
                "/ws/",           // WebSocket 连接
            };

            // 精确匹配白名单
            for (const auto& wp : exactWhitelist) {
                if (path == wp) {
                    needsAuth = false;
                    break;
                }
            }
            // 前缀匹配白名单（仅限安全的前缀）
            if (needsAuth) {
                for (const auto& prefix : prefixWhitelist) {
                    if (path.find(prefix) == 0) {
                        needsAuth = false;
                        break;
                    }
                }
            }

            // GET 查看石头不需要认证（除了 /my 开头的路径）
            if (needsAuth && req->method() == Get && path.find("/api/stones/") == 0) {
                if (path.find("/api/stones/my") != 0) {
                    needsAuth = false;
                }
            }

            if (needsAuth) {
                try {
                    std::string authHeader = req->getHeader("Authorization");
                    if (authHeader.empty()) {
                        acb(createErrorResponse(401, "未登录"));
                        return;
                    }

                    std::string token;
                    if (authHeader.find("Bearer ") == 0) {
                        token = authHeader.substr(7);
                    } else {
                        acb(createErrorResponse(401, "认证格式错误"));
                        return;
                    }

                    std::string user_id = heartlake::utils::PasetoUtil::verifyToken(token, heartlake::utils::PasetoUtil::getKey());
                    if (user_id.empty()) {
                        acb(createErrorResponse(401, "认证失败，请重新登录"));
                        return;
                    }
                    req->addHeader("X-User-Id", user_id);
                    req->getAttributes()->insert("user_id", user_id);

                } catch (const std::exception& e) {
                    LOG_WARN << "Auth failed: " << e.what();
                    acb(createErrorResponse(401, "认证失败，请重新登录"));
                    return;
                }
            }

            accb();
        });

        // 注册服务启动后的初始化回调
        app.registerBeginningAdvice([]() {
            // 初始化整个架构（基础设施→领域→应用→事件处理器）
            LOG_INFO << "Initializing Architecture...";
            heartlake::ArchitectureBootstrap::initialize();
            LOG_INFO << "Architecture initialized";

            // 初始化RBAC权限管理
            LOG_INFO << "Initializing RBAC Manager...";
            heartlake::utils::RBACManager::getInstance().initialize();
            LOG_INFO << "RBAC Manager initialized";

            // 初始化内容过滤器
            LOG_INFO << "Initializing ContentFilter...";
            heartlake::ContentFilter::getInstance().initialize();
            LOG_INFO << "ContentFilter initialized";

            // 初始化AI服务
            LOG_INFO << "Initializing AI Service...";
            Json::Value aiConfig;
            const char* ai_provider = std::getenv("AI_PROVIDER");
            const char* ai_key = std::getenv("AI_API_KEY");
            const char* ai_url = std::getenv("AI_BASE_URL");
            const char* ai_model = std::getenv("AI_MODEL");
            const char* ai_timeout = std::getenv("AI_TIMEOUT");
            aiConfig["provider"] = ai_provider ? ai_provider : "deepseek";
            aiConfig["api_key"] = ai_key ? ai_key : "";
            aiConfig["base_url"] = ai_url ? ai_url : "https://api.deepseek.com";
            aiConfig["model"] = ai_model ? ai_model : "deepseek-chat";
            aiConfig["timeout"] = ai_timeout ? std::atoi(ai_timeout) : 10;
            heartlake::ai::AIService::getInstance().initialize(aiConfig);

            // 初始化嵌入向量引擎
            LOG_INFO << "Initializing Embedding Engine...";
            heartlake::ai::AdvancedEmbeddingEngine::getInstance().initialize(128, 10000);

            // 初始化图片审核引擎
            LOG_INFO << "Initializing Image Moderation Engine...";
            heartlake::ai::ImageModerationEngine::getInstance().initialize();

            // 初始化限流器
            LOG_INFO << "Initializing Rate Limiter...";
            heartlake::middleware::RateLimiter::getInstance().initialize();

            // 初始化Redis连接池（初始30连接，动态扩容）
            LOG_INFO << "Initializing Redis with connection pool...";
            heartlake::cache::RedisPoolConfig redisConfig;
            redisConfig.initialSize = 30;
            redisConfig.maxSize = 100;
            redisConfig.idleTimeoutMs = 30000;
            const char* redis_host = std::getenv("REDIS_HOST");
            const char* redis_port_str = std::getenv("REDIS_PORT");
            const char* redis_password = std::getenv("REDIS_PASSWORD");
            heartlake::cache::RedisCache::getInstance().initialize(
                redis_host ? redis_host : "127.0.0.1",
                redis_port_str ? std::atoi(redis_port_str) : 6379,
                redis_password ? redis_password : "",
                0,
                redisConfig
            );

            // 初始化石友关系TTL引擎
            LOG_INFO << "Initializing Friendship TTL Engine...";
            heartlake::infrastructure::FriendshipTTLEngine::getInstance().initialize();

            // 初始化同频共鸣搜索服务
            LOG_INFO << "Initializing Resonance Search Service...";
            heartlake::infrastructure::ResonanceSearchService::getInstance().initialize(128);

            // 启动湖神守护服务（自动为零互动石头派送暖心纸船）
            LOG_INFO << "Starting LakeGod Guardian Service...";
            heartlake::infrastructure::LakeGodGuardianService::getInstance().start();

            // 启动情绪追踪服务（72h负重者监测与关怀）
            LOG_INFO << "Starting Emotion Tracking Service...";
            heartlake::infrastructure::EmotionTrackingService::getInstance().start();

            // 启动用户回访服务（定期关怀高风险用户）
            LOG_INFO << "Starting User FollowUp Service...";
            heartlake::infrastructure::UserFollowUpService::getInstance().start();

            // 启动WebSocket心跳定时器，清理僵尸连接
            LOG_INFO << "Starting WebSocket heartbeat timer...";
            heartlake::controllers::BroadcastWebSocketController::startHeartbeatTimer();

            LOG_INFO << "All services initialized successfully";
        });

        LOG_INFO << "Heart Lake Server starting...";
        LOG_INFO << "Listening on 0.0.0.0:8080";

        // 启动服务器
        app.run();

    } catch (const std::exception& e) {
        LOG_ERROR << "Fatal error: " << e.what();
        return 1;
    }

    return 0;
}
