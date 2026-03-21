/**
 * @file main.cpp
 * @brief Heart Lake 后端服务入口 —— 启动引导、中间件注册、子系统初始化
 *
 * 启动流程概览：
 *   1. 加载 .env 环境变量（支持多候选路径 + 显式 HEARTLAKE_ENV_PATH）
 *   2. 校验关键安全配置（PASETO_KEY、DB_PASSWORD）
 *   3. 根据 CPU 核数 / ONNX 推理需求自适应设置 IO 线程数
 *   4. 注册 Drogon 中间件链：CORS → 安全响应头 → PASETO 认证 → 链路追踪
 *   5. 按依赖顺序初始化子系统：
 *      Architecture → RBAC → ContentFilter → AIService → EmbeddingEngine
 *      → EdgeAI → EmotionResonance → Recommendation → DualMemoryRAG
 *      → RateLimiter → Redis → FriendshipTTL → ResonanceSearch
 *   6. 启动后台守护任务（LakeGod / EmotionTracking / UserFollowUp / WS心跳）
 *   7. 调用 app.run() 进入事件循环
 */
#include <drogon/drogon.h>
#include "utils/PasetoUtil.h"
#include "utils/RBACManager.h"
#include <iostream>
#include <ctime>
#include <set>
#include <sstream>
#include <algorithm>
#include <thread>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <vector>
#include <iomanip>

#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/ai/EmotionResonanceEngine.h"
#include "infrastructure/ai/RecommendationEngine.h"
#include "infrastructure/ai/DualMemoryRAG.h"
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
#include "utils/EnvUtils.h"
#include "utils/AdminConfigStore.h"

using namespace drogon;

/// 构造统一格式的 JSON 错误响应，自动映射业务 code 到 HTTP 状态码
static HttpResponsePtr createErrorResponse(int code, const std::string& message) {
    Json::Value json;
    json["code"] = code;
    json["message"] = message;
    json["data"] = Json::nullValue;
    json["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));
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

/// 根据硬件并发度计算默认 IO 线程数，上限 8、下限 2，避免低配机器过度抢占
static int getDefaultServerThreads() {
    const unsigned int hw = std::thread::hardware_concurrency();
    if (hw == 0) {
        return 4;
    }
    // 低配环境优先：默认线程数随CPU核数变化，避免固定16线程造成过度抢占
    const unsigned int capped = std::min(8u, hw);
    return static_cast<int>(std::max(2u, capped));
}

/// 解析显式布尔环境变量；未设置或格式非法时返回 std::nullopt
static std::optional<bool> parseExplicitBoolEnv(const char* rawValue) {
    if (!rawValue || *rawValue == '\0') {
        return std::nullopt;
    }

    std::string normalized = rawValue;
    normalized.erase(normalized.begin(), std::find_if(normalized.begin(), normalized.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    normalized.erase(std::find_if(normalized.rbegin(), normalized.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), normalized.end());
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return std::nullopt;
}

/// 去除字符串首尾空白字符
static std::string trimWhitespace(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
    return value;
}

/// 剥离值两端的可选引号（单引号或双引号），用于 .env 解析
static std::string stripOptionalQuotes(const std::string& value) {
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

/// 仅在环境变量缺失时注入默认值，避免低配机器误用高占用参数。
static void setDefaultEnvIfMissing(const char* key, const std::string& value) {
    const char* existing = std::getenv(key);
    if (existing && *existing != '\0') {
        return;
    }
    setenv(key, value.c_str(), 0);
}

/**
 * 解析 .env 文件并注入环境变量。
 * 支持 # 注释、export 前缀、内联注释裁剪、引号剥离。
 * @param overrideExisting 为 true 时覆盖已有同名环境变量
 * @return 文件存在且成功加载返回 true
 */
static bool loadEnvFileIfExists(const std::filesystem::path& path, bool overrideExisting = false) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    size_t loadedCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        std::string item = trimWhitespace(line);
        if (item.empty() || item[0] == '#') {
            continue;
        }

        if (item.rfind("export ", 0) == 0) {
            item = trimWhitespace(item.substr(7));
        }

        const size_t eqPos = item.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }

        std::string key = trimWhitespace(item.substr(0, eqPos));
        std::string value = trimWhitespace(item.substr(eqPos + 1));
        if (key.empty()) {
            continue;
        }

        // 仅在非引号值中裁剪内联注释（例如 KEY=abc #comment）
        if (!value.empty() && value.front() != '"' && value.front() != '\'') {
            const size_t inlineComment = value.find(" #");
            if (inlineComment != std::string::npos) {
                value = trimWhitespace(value.substr(0, inlineComment));
            }
        }

        value = stripOptionalQuotes(value);

        if (!overrideExisting && std::getenv(key.c_str()) != nullptr) {
            continue;
        }

        setenv(key.c_str(), value.c_str(), overrideExisting ? 1 : 0);
        ++loadedCount;
    }

    LOG_INFO << "Loaded " << loadedCount << " env vars from " << path.string();
    return true;
}

/// 按优先级搜索 .env 文件：HEARTLAKE_ENV_PATH > .env > ../.env > ./backend/.env
static void bootstrapEnvFromDotEnv() {
    const char* explicitEnvPath = std::getenv("HEARTLAKE_ENV_PATH");
    if (explicitEnvPath && *explicitEnvPath != '\0') {
        if (!loadEnvFileIfExists(explicitEnvPath, false)) {
            LOG_WARN << "HEARTLAKE_ENV_PATH is set but file not found: " << explicitEnvPath;
        }
        return;
    }

    static const std::vector<std::filesystem::path> candidates = {
        ".env",
        "../.env",
        "./backend/.env"
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
            loadEnvFileIfExists(candidate, false);
            return;
        }
    }
}

/// 解析 config.json 路径：优先使用指定路径，不存在时依次尝试 fallback 候选
static std::string resolveConfigPath(std::string requestedPath) {
    if (!requestedPath.empty()) {
        std::error_code ec;
        if (std::filesystem::exists(requestedPath, ec) &&
            std::filesystem::is_regular_file(requestedPath, ec)) {
            return requestedPath;
        }
    }

    static const std::vector<std::string> fallbackCandidates = {
        "../config.json",
        "./config.json",
        "./backend/config.json"
    };
    for (const auto& candidate : fallbackCandidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) &&
            std::filesystem::is_regular_file(candidate, ec)) {
            LOG_WARN << "Config path fallback: " << requestedPath << " -> " << candidate;
            return candidate;
        }
    }
    return requestedPath;
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
        // 允许直接运行二进制时自动加载 .env，避免本地模型/数据库配置丢失。
        bootstrapEnvFromDotEnv();

        // 优先使用环境变量配置路径，支持绝对路径
        const char* config_path = std::getenv("HEARTLAKE_CONFIG_PATH");
        std::string configFile = config_path ? config_path : "../config.json";
        if (argc > 1) {
            configFile = argv[1];
        }
        configFile = resolveConfigPath(configFile);

        auto& app = drogon::app();

        // 从环境变量读取配置
        const char* db_host = std::getenv("DB_HOST");
        const char* db_port_str = std::getenv("DB_PORT");
        const char* db_name = std::getenv("DB_NAME");
        const char* db_user = std::getenv("DB_USER");
        const char* db_password = std::getenv("DB_PASSWORD");
        const char* db_pool_size_str = std::getenv("DB_CONNECTION_POOL_SIZE");
        if (!db_pool_size_str || *db_pool_size_str == '\0') {
            db_pool_size_str = std::getenv("DB_POOL_SIZE");
        }
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
        if (!db_password) {
            LOG_ERROR << "DB_PASSWORD environment variable is required";
            return 1;
        }
        [[maybe_unused]] std::string dbPassword = db_password;
        [[maybe_unused]] int dbPoolSize = db_pool_size_str ? std::atoi(db_pool_size_str) : 50;
        [[maybe_unused]] double dbTimeout = db_timeout_str ? std::atof(db_timeout_str) : 30.0;

        std::string serverHost = server_host ? server_host : "0.0.0.0";
        int serverPort = server_port_str ? std::atoi(server_port_str) : 8080;
        const int defaultServerThreads = getDefaultServerThreads();
        int serverThreads = defaultServerThreads;
        if (server_threads_str) {
            int configuredThreads = std::atoi(server_threads_str);
            if (configuredThreads > 0) {
                serverThreads = configuredThreads;
            } else {
                LOG_WARN << "Invalid SERVER_THREADS value, fallback to default: " << defaultServerThreads;
            }
        }
        // 只有在显式启用 ONNX 时才额外预留线程，避免 2C2G 环境被“自动放大”到 8 线程。
        const auto explicitOnnxSwitch = parseExplicitBoolEnv(std::getenv("EDGE_AI_ONNX_ENABLED"));
        if (explicitOnnxSwitch.has_value() && explicitOnnxSwitch.value()) {
            const unsigned int hw = std::thread::hardware_concurrency();
            const unsigned int effectiveHw = (hw == 0) ? 8u : hw;
            const int minThreadsForOnnx = static_cast<int>(std::clamp(effectiveHw, 4u, 8u));
            if (serverThreads < minThreadsForOnnx) {
                LOG_WARN << "SERVER_THREADS=" << serverThreads
                         << " is too low for ONNX path, auto-upgrade to "
                         << minThreadsForOnnx;
                serverThreads = minThreadsForOnnx;
            }
        } else if (!explicitOnnxSwitch.has_value() && std::getenv("EDGE_AI_ONNX_ENABLED")) {
            LOG_WARN << "Invalid EDGE_AI_ONNX_ENABLED value, skip ONNX thread auto-upgrade";
        }
        LOG_INFO << "Server threads set to " << serverThreads;

        const bool lowResourceProfile = (serverThreads <= 2);
        if (lowResourceProfile) {
            setDefaultEnvIfMissing("EMBEDDING_CACHE_SIZE", "6000");
            setDefaultEnvIfMissing("AI_SEMANTIC_CACHE_MAX_SIZE", "2000");
            setDefaultEnvIfMissing("AI_SENTIMENT_CACHE_MAX_SIZE", "8000");
            setDefaultEnvIfMissing("EDGE_AI_SENTIMENT_CACHE_MAX_SIZE", "4096");
            setDefaultEnvIfMissing("REDIS_POOL_SIZE", "4");
            setDefaultEnvIfMissing("REDIS_MAX_POOL_SIZE", "8");
            setDefaultEnvIfMissing("AI_OLLAMA_NUM_THREAD", "2");
            setDefaultEnvIfMissing("AI_OLLAMA_NUM_CTX", "2048");
            setDefaultEnvIfMissing("ENABLE_LAKE_GOD_GUARDIAN", "false");
            setDefaultEnvIfMissing("ENABLE_EMOTION_TRACKING", "false");
            setDefaultEnvIfMissing("ENABLE_USER_FOLLOWUP", "false");
            LOG_INFO << "Low-resource profile active: 2C2G-friendly defaults applied";
        }

        // 配置服务器
        app.addListener(serverHost, static_cast<uint16_t>(serverPort));

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
        // loadConfigFile() 会回填默认配置，若配置文件未显式声明线程数，可能把前面的 setThreadNum 覆盖回默认值。
        // 在配置加载后再次强制设置线程数，避免高并发下退化为单线程。
        app.setThreadNum(static_cast<size_t>(serverThreads));
        LOG_INFO << "Config loaded, database client will be initialized by framework";
        LOG_INFO << "Effective server thread count (post-config): " << serverThreads;

        // 全局异常处理器 - 捕获未处理的异常
        app.setExceptionHandler([](const std::exception& e,
                                   const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& callback) {
            LOG_ERROR << "Unhandled exception at " << req->path() << ": " << e.what();
            Json::Value json;
            json["code"] = 500;
            json["message"] = "服务器内部错误";
            json["data"] = Json::nullValue;
            json["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));
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

        auto applyCorsHeaders = [matchOrigin](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
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
        };
        auto applySecurityHeaders = [](const HttpResponsePtr &resp) {
            // 安全响应头：防止 MIME 类型嗅探、点击劫持、强制 HTTPS
            resp->addHeader("X-Content-Type-Options", "nosniff");
            resp->addHeader("X-Frame-Options", "DENY");
            resp->addHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
            resp->addHeader("X-XSS-Protection", "1; mode=block");
            resp->addHeader("Referrer-Policy", "strict-origin-when-cross-origin");
        };

        // 安全响应头 + CORS 后处理中间件
        app.registerPostHandlingAdvice([applyCorsHeaders, applySecurityHeaders](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
            applyCorsHeaders(req, resp);
            applySecurityHeaders(resp);
        });

        // OPTIONS - 使用 PreRoutingAdvice 在路由匹配前处理 CORS 预检请求
        app.registerPreRoutingAdvice([applyCorsHeaders, applySecurityHeaders](const HttpRequestPtr &req, AdviceCallback &&acb, AdviceChainCallback &&accb) {
            if (req->method() == Options) {
                auto resp = HttpResponse::newHttpResponse();
                applyCorsHeaders(req, resp);
                applySecurityHeaders(resp);
                resp->setStatusCode(k204NoContent);
                acb(resp);
                return;
            }
            accb();
        });

        // PASETO认证中间件
        app.registerPreHandlingAdvice([applyCorsHeaders, applySecurityHeaders](const HttpRequestPtr &req, AdviceCallback &&acb, AdviceChainCallback &&accb) {

            std::string path = req->path();
            bool needsAuth = true;

            // 认证白名单：使用显式路径列表，避免前缀匹配导致的安全漏洞
            // 只有明确列出的路径才跳过认证
            // VUL-17 修复：从白名单中移除 /metrics，需要认证才能访问
            static const std::vector<std::string> exactWhitelist = {
                "/api/auth/anonymous",
                "/api/auth/register",
                "/api/auth/login",
                "/api/auth/verification-code",
                "/api/auth/email/verification-code",
                "/api/auth/recover",
                "/api/auth/register/email",
                "/api/auth/login/email",
                "/api/auth/reset-password/code",
                "/api/auth/reset-password",
                "/api/v1/auth/register",
                "/api/v1/auth/login",
                "/api/v1/auth/reset-password",
                "/api/health",
                "/api/health/detailed",
                // VUL-17: /metrics 已移除，需要认证访问
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
                        auto resp = createErrorResponse(401, "未登录");
                        applyCorsHeaders(req, resp);
                        applySecurityHeaders(resp);
                        acb(resp);
                        return;
                    }

                    std::string token;
                    if (authHeader.find("Bearer ") == 0) {
                        token = authHeader.substr(7);
                    } else {
                        auto resp = createErrorResponse(401, "认证格式错误");
                        applyCorsHeaders(req, resp);
                        applySecurityHeaders(resp);
                        acb(resp);
                        return;
                    }

                    std::string user_id = heartlake::utils::PasetoUtil::verifyToken(token, heartlake::utils::PasetoUtil::getKey());
                    if (user_id.empty()) {
                        auto resp = createErrorResponse(401, "认证失败，请重新登录");
                        applyCorsHeaders(req, resp);
                        applySecurityHeaders(resp);
                        acb(resp);
                        return;
                    }
                    req->getAttributes()->insert("user_id", user_id);

                } catch (const std::exception& e) {
                    LOG_WARN << "Auth failed: " << e.what();
                    auto resp = createErrorResponse(401, "认证失败，请重新登录");
                    applyCorsHeaders(req, resp);
                    applySecurityHeaders(resp);
                    acb(resp);
                    return;
                }
            }

            accb();
        });

        // 链路追踪：在请求处理完成后注入响应头并记录耗时日志
        app.registerPostHandlingAdvice(
            [](const drogon::HttpRequestPtr& req,
               const drogon::HttpResponsePtr& resp) {
                auto traceId = req->getAttributes()->get<std::string>("trace_id");
                auto spanId = req->getAttributes()->get<std::string>("span_id");

                if (!traceId.empty()) {
                    resp->addHeader("X-Trace-Id", traceId);
                    resp->addHeader("X-Span-Id", spanId);

                    // 计算请求耗时
                    auto startTimeStr = req->getAttributes()->get<std::string>("trace_start_time");
                    if (!startTimeStr.empty()) {
                        long long startUs = 0;
                        auto [ptr, ec] = std::from_chars(
                            startTimeStr.data(),
                            startTimeStr.data() + startTimeStr.size(),
                            startUs);
                        if (ec == std::errc{}) {
                            auto nowUs = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::steady_clock::now().time_since_epoch()).count();
                            double durationMs = static_cast<double>(nowUs - startUs) / 1000.0;

                            std::ostringstream durationOss;
                            durationOss << std::fixed << std::setprecision(2) << durationMs;

                            const int statusCode = static_cast<int>(resp->getStatusCode());
                            const bool serverError = (statusCode >= 500);
                            const bool suspiciousSlow = (durationMs >= 3000.0);
                            const bool hotEdgePath = (req->getPath() == "/api/edge-ai/analyze");
                            // 高频接口默认不做逐请求 INFO 打印，仅在异常慢请求或服务端错误时记录。
                            if (serverError || (suspiciousSlow && !hotEdgePath)) {
                                LOG_WARN << "[TRACE] trace_id=" << traceId
                                         << " span_id=" << spanId
                                         << " method=" << req->getMethodString()
                                         << " path=" << req->getPath()
                                         << " status=" << statusCode
                                         << " duration_ms=" << durationOss.str();
                            }
                        }
                    }
                }
            });

        // 明确执行启动初始化，避免 registerBeginningAdvice 在不同运行模式下不触发。
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
        Json::Value adminConfig = heartlake::utils::AdminConfigStore::load();
        Json::Value aiConfig = adminConfig["ai"];
        if (!aiConfig.isObject()) {
            const char* ai_provider = std::getenv("AI_PROVIDER");
            const char* ai_key = std::getenv("AI_API_KEY");
            const char* ai_url = std::getenv("AI_BASE_URL");
            const char* ai_model = std::getenv("AI_MODEL");
            const char* ai_timeout = std::getenv("AI_TIMEOUT");
            aiConfig["provider"] = ai_provider ? ai_provider : "ollama";
            aiConfig["api_key"] = ai_key ? ai_key : "";
            aiConfig["base_url"] = ai_url ? ai_url : "http://127.0.0.1:11434";
            aiConfig["model"] = ai_model ? ai_model : "heartlake-qwen";
            aiConfig["timeout"] = ai_timeout ? std::atoi(ai_timeout) : 10;
        }
        heartlake::ai::AIService::getInstance().initialize(aiConfig);

        // 初始化嵌入向量引擎（统一配置，避免多服务维度漂移）
        const int embeddingDim = std::clamp(
            heartlake::utils::parsePositiveIntEnv("EMBEDDING_DIM", 256), 64, 1536);
        const int embeddingCacheSize = std::clamp(
            heartlake::utils::parsePositiveIntEnv("EMBEDDING_CACHE_SIZE", 20000), 1000, 200000);
        LOG_INFO << "Initializing Embedding Engine... dim=" << embeddingDim
                 << ", cache=" << embeddingCacheSize;
        heartlake::ai::AdvancedEmbeddingEngine::getInstance().initialize(
            static_cast<size_t>(embeddingDim), static_cast<size_t>(embeddingCacheSize));

        // 初始化边缘AI引擎（情感分析、内容审核、HNSW、联邦学习、差分隐私）
        LOG_INFO << "Initializing Edge AI Engine...";
        Json::Value edgeAIConfig;
        edgeAIConfig["enabled"] = aiConfig.get(
            "edge_ai_enabled",
            heartlake::utils::parseBoolEnv(std::getenv("EDGE_AI_ENABLED"), true));
        edgeAIConfig["hnsw_m"] = aiConfig.get("hnsw_m", 16);
        edgeAIConfig["hnsw_ef_construction"] =
            aiConfig.get("hnsw_ef_construction", 200);
        edgeAIConfig["hnsw_ef_search"] = aiConfig.get("hnsw_ef_search", 50);
        edgeAIConfig["dp_epsilon"] = aiConfig.get("dp_epsilon", 1.0);
        edgeAIConfig["dp_delta"] = aiConfig.get("dp_delta", 1e-5);
        edgeAIConfig["dp_max_budget"] = aiConfig.get("dp_max_budget", 10.0);
        edgeAIConfig["pulse_window_seconds"] =
            aiConfig.get("pulse_window_seconds", 300);
        edgeAIConfig["sentiment_cache_ttl"] =
            aiConfig.get("sentiment_cache_ttl",
                         heartlake::utils::parsePositiveIntEnv(
                             "SENTIMENT_CACHE_TTL", 300));
        edgeAIConfig["sentiment_cache_max"] =
            aiConfig.get("sentiment_cache_max",
                         heartlake::utils::parsePositiveIntEnv(
                             "SENTIMENT_CACHE_MAX", 4096));
        edgeAIConfig["onnx_threads"] = aiConfig.get(
            "onnx_threads",
            heartlake::utils::parsePositiveIntEnv("EDGE_AI_ONNX_THREADS", 2));
        if (aiConfig.isMember("model_path") && aiConfig["model_path"].isString() &&
            !aiConfig["model_path"].asString().empty()) {
            edgeAIConfig["model_path"] = aiConfig["model_path"].asString();
        }
        if (aiConfig.isMember("vocab_path") && aiConfig["vocab_path"].isString() &&
            !aiConfig["vocab_path"].asString().empty()) {
            edgeAIConfig["vocab_path"] = aiConfig["vocab_path"].asString();
        }
        if (const char* edgeModelPath = std::getenv("EDGE_AI_MODEL_PATH"); edgeModelPath && *edgeModelPath) {
            edgeAIConfig["model_path"] = edgeModelPath;
        }
        if (const char* edgeVocabPath = std::getenv("EDGE_AI_VOCAB_PATH"); edgeVocabPath && *edgeVocabPath) {
            edgeAIConfig["vocab_path"] = edgeVocabPath;
        }
        if (aiConfig.isMember("onnx_enabled")) {
            edgeAIConfig["onnx_enabled"] = aiConfig["onnx_enabled"];
        }
        LOG_INFO << "Edge AI enabled=" << edgeAIConfig["enabled"].asBool();
        heartlake::ai::EdgeAIEngine::getInstance().initialize(edgeAIConfig);

        // 预热情感共鸣引擎（单例懒加载）
        LOG_INFO << "Initializing Emotion Resonance Engine...";
        heartlake::ai::EmotionResonanceEngine::getInstance();

        // 初始化推荐引擎
        LOG_INFO << "Initializing Recommendation Engine...";
        heartlake::ai::RecommendationEngine::getInstance().initialize(32);
        heartlake::ai::RecommendationEngine::getInstance().setDbClientProvider(
            [](){ return drogon::app().getDbClient("default"); });

        // 预热双记忆RAG（单例懒加载）
        LOG_INFO << "Initializing Dual Memory RAG...";
        heartlake::ai::DualMemoryRAG::getInstance();

        // 初始化限流器
        LOG_INFO << "Initializing Rate Limiter...";
        heartlake::middleware::RateLimiter::getInstance().initialize();

        // 初始化Redis连接池（低配默认12连接，可通过环境变量调整）
        LOG_INFO << "Initializing Redis with connection pool...";
        heartlake::cache::RedisPoolConfig redisConfig;
        const int redisPoolInitial = heartlake::utils::parsePositiveIntEnv("REDIS_POOL_SIZE", 12);
        const int redisPoolMax = heartlake::utils::parsePositiveIntEnv(
            "REDIS_MAX_POOL_SIZE", std::max(redisPoolInitial, std::min(redisPoolInitial * 2, 12)));
        redisConfig.initialSize = redisPoolInitial;
        redisConfig.maxSize = redisPoolMax;
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
        heartlake::infrastructure::ResonanceSearchService::getInstance().initialize(
            static_cast<size_t>(embeddingDim));

        // 后台任务默认全部启动，可通过环境变量关闭
        if (heartlake::utils::parseBoolEnv(std::getenv("ENABLE_LAKE_GOD_GUARDIAN"), true)) {
            LOG_INFO << "Starting LakeGod Guardian Service...";
            heartlake::infrastructure::LakeGodGuardianService::getInstance().start();
        } else {
            LOG_INFO << "LakeGod Guardian Service disabled (set ENABLE_LAKE_GOD_GUARDIAN=true to enable)";
        }

        if (heartlake::utils::parseBoolEnv(std::getenv("ENABLE_EMOTION_TRACKING"), true)) {
            LOG_INFO << "Starting Emotion Tracking Service...";
            heartlake::infrastructure::EmotionTrackingService::getInstance().start();
        } else {
            LOG_INFO << "Emotion Tracking Service disabled (set ENABLE_EMOTION_TRACKING=true to enable)";
        }

        if (heartlake::utils::parseBoolEnv(std::getenv("ENABLE_USER_FOLLOWUP"), true)) {
            LOG_INFO << "Starting User FollowUp Service...";
            heartlake::infrastructure::UserFollowUpService::getInstance().start();
        } else {
            LOG_INFO << "User FollowUp Service disabled (set ENABLE_USER_FOLLOWUP=true to enable)";
        }

        if (heartlake::utils::parseBoolEnv(std::getenv("ENABLE_WS_HEARTBEAT"), true)) {
            LOG_INFO << "Starting WebSocket heartbeat timer...";
            heartlake::controllers::BroadcastWebSocketController::startHeartbeatTimer();
        } else {
            LOG_INFO << "WebSocket heartbeat timer disabled (set ENABLE_WS_HEARTBEAT=true to enable)";
        }

        LOG_INFO << "All services initialized successfully";

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
