/**
 * AdminConfigStore 模块实现
 */
#include "utils/AdminConfigStore.h"
#include "utils/EnvUtils.h"
#include <filesystem>
#include <fstream>
#include <unistd.h>

using namespace heartlake::utils;

std::mutex AdminConfigStore::mutex_;

std::string AdminConfigStore::configFilePath() {
    const char* envPath = std::getenv("ADMIN_CONFIG_FILE");
    if (envPath && *envPath) {
        return envPath;
    }

    constexpr const char* kPrimaryPath = "./config/admin_config.json";
    constexpr const char* kFallbackPath = "./logs/admin_config.json";

    const std::filesystem::path primary(kPrimaryPath);
    const std::filesystem::path fallback(kFallbackPath);

    if (std::filesystem::exists(primary)) {
        return primary.string();
    }
    if (std::filesystem::exists(fallback)) {
        return fallback.string();
    }

    std::error_code ec;
    const auto primaryParent = primary.parent_path();
    if (!primaryParent.empty() &&
        std::filesystem::exists(primaryParent, ec) &&
        !ec &&
        access(primaryParent.c_str(), W_OK) == 0) {
        return primary.string();
    }

    return fallback.string();
}

Json::Value AdminConfigStore::defaultConfig() {
    Json::Value config;

    // 系统配置
    Json::Value system;
    system["name"] = "HeartLake";
    system["description"] = "一个温暖的情感交流平台";
    const char* apiUrl = std::getenv("PUBLIC_API_URL");
    const char* wsUrl = std::getenv("PUBLIC_WS_URL");
    system["apiUrl"] = apiUrl ? apiUrl : "";
    system["wsUrl"] = wsUrl ? wsUrl : "";
    system["allowRegister"] = true;
    system["allowAnonymous"] = true;

    // AI配置
    Json::Value ai;
    ai["provider"] = std::getenv("AI_PROVIDER") ? std::getenv("AI_PROVIDER") : "deepseek";
    ai["api_key"] = std::getenv("AI_API_KEY") ? std::getenv("AI_API_KEY") : "";
    ai["base_url"] = std::getenv("AI_BASE_URL") ? std::getenv("AI_BASE_URL") : "https://api.deepseek.com";
    ai["model"] = std::getenv("AI_MODEL") ? std::getenv("AI_MODEL") : "deepseek-chat";
    ai["enableSentiment"] = true;
    ai["enableModeration"] = true;
    ai["enableAutoReply"] = true;
    ai["moderationThreshold"] = 0.7;
    ai["edge_ai_enabled"] = parseBoolEnv(std::getenv("EDGE_AI_ENABLED"), true);
    ai["onnx_enabled"] =
        std::getenv("EDGE_AI_ONNX_ENABLED") ? std::getenv("EDGE_AI_ONNX_ENABLED") : "auto";
    ai["model_path"] =
        std::getenv("EDGE_AI_MODEL_PATH") ? std::getenv("EDGE_AI_MODEL_PATH") : "./models/sentiment_zh.onnx";
    ai["vocab_path"] =
        std::getenv("EDGE_AI_VOCAB_PATH") ? std::getenv("EDGE_AI_VOCAB_PATH") : "";
    ai["onnx_threads"] = parsePositiveIntEnv("EDGE_AI_ONNX_THREADS", 2);
    ai["hnsw_m"] = 16;
    ai["hnsw_ef_construction"] = 200;
    ai["hnsw_ef_search"] = 50;
    ai["dp_epsilon"] = 1.0;
    ai["dp_delta"] = 1e-5;
    ai["pulse_window_seconds"] = 300;
    ai["sentiment_cache_ttl"] = 300;
    ai["sentiment_cache_max"] = 4096;
    ai["cacheStrategy"] = "lru";
    ai["maxBatchSize"] = 32;
    ai["cacheSizeMB"] = 256;
    ai["federatedInterval"] = 300;
    ai["emotionModel"] = "standard";
    ai["vectorSearchEnabled"] = true;
    ai["inferenceTimeout"] = 5000;
    ai["cacheEnabled"] = true;
    ai["federatedEnabled"] = true;
    ai["quantization_bits"] = 8;

    // 限流配置
    Json::Value rate;
    rate["stonePerHour"] = 15;
    rate["boatPerHour"] = 50;
    rate["messagePerMinute"] = 60;
    rate["maxContentLength"] = 2000;

    config["system"] = system;
    config["ai"] = ai;
    config["rate"] = rate;

    return config;
}

Json::Value AdminConfigStore::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string path = configFilePath();

    if (!std::filesystem::exists(path)) {
        // 不要在 load 时自动创建文件，直接返回默认配置
        return defaultConfig();
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        return defaultConfig();
    }

    Json::Value config;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, in, &config, &errs)) {
        return defaultConfig();
    }

    // 兼容空配置
    if (!config.isObject()) {
        return defaultConfig();
    }

    return config;
}

bool AdminConfigStore::save(const Json::Value& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string primaryPath = configFilePath();
    const std::string fallbackPath = "./logs/admin_config.json";

    auto writeTo = [&config](const std::string& path) -> bool {
        std::filesystem::path filePath(path);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        std::ofstream out(path);
        if (!out.is_open()) {
            return false;
        }

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";
        out << Json::writeString(writer, config);
        return static_cast<bool>(out);
    };

    if (writeTo(primaryPath)) {
        return true;
    }
    if (primaryPath != fallbackPath) {
        return writeTo(fallbackPath);
    }
    return false;
}
