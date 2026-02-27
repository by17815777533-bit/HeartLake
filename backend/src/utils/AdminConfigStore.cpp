/**
 * AdminConfigStore 模块实现
 */
#include "utils/AdminConfigStore.h"
#include <fstream>
#include <filesystem>

using namespace heartlake::utils;

std::mutex AdminConfigStore::mutex_;

std::string AdminConfigStore::configFilePath() {
    const char* envPath = std::getenv("ADMIN_CONFIG_FILE");
    return envPath ? envPath : "./config/admin_config.json";
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
    const std::string path = configFilePath();

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
    return true;
}
