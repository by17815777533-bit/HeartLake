/**
 * @file ConfigManager.cpp
 * @brief ConfigManager 模块实现
 * Created by 白洋
 */
#include "config/ConfigManager.h"
#include <drogon/drogon.h>
#include <fstream>
#include <cstdlib>

namespace heartlake {
namespace config {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::load(const std::string& configFile) {
    std::lock_guard<std::mutex> lock(mutex_);
    configFile_ = configFile;

    std::ifstream ifs(configFile);
    if (ifs.is_open()) {
        Json::CharReaderBuilder reader;
        std::string errs;
        if (Json::parseFromStream(reader, ifs, &config_, &errs)) {
            LOG_INFO << "Configuration loaded from: " << configFile;
        } else {
            LOG_WARN << "Failed to parse config file: " << errs;
        }
    } else {
        LOG_WARN << "Config file not found: " << configFile << ", using defaults";
    }
}

bool ConfigManager::reload() {
    std::lock_guard<std::mutex> lock(mutex_);
    Json::Value backup = config_;

    std::ifstream ifs(configFile_);
    if (!ifs.is_open()) {
        LOG_WARN << "Reload failed: cannot open " << configFile_;
        return false;
    }

    Json::CharReaderBuilder reader;
    std::string errs;
    Json::Value newConfig;
    if (!Json::parseFromStream(reader, ifs, &newConfig, &errs)) {
        LOG_WARN << "Reload failed, rollback: " << errs;
        return false;
    }

    config_ = newConfig;
    LOG_INFO << "Configuration reloaded from: " << configFile_;
    return true;
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) {
    return getFromEnvOrConfig(key, key, defaultValue);
}

int ConfigManager::getInt(const std::string& key, int defaultValue) {
    return getIntFromEnvOrConfig(key, key, defaultValue);
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) {
    return getBoolFromEnvOrConfig(key, key, defaultValue);
}

double ConfigManager::getDouble(const std::string& key, double defaultValue) {
    const char* env = std::getenv(key.c_str());
    if (env) {
        return std::stod(env);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (config_.isMember(key) && config_[key].isDouble()) {
        return config_[key].asDouble();
    }
    return defaultValue;
}

Json::Value ConfigManager::getJson(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (config_.isMember(key)) {
        return config_[key];
    }
    return Json::Value();
}

ConfigManager::DatabaseConfig ConfigManager::getDatabaseConfig() {
    DatabaseConfig dbConfig;
    dbConfig.host = getFromEnvOrConfig("DB_HOST", "db.host", "localhost");
    dbConfig.port = getIntFromEnvOrConfig("DB_PORT", "db.port", 5432);
    dbConfig.name = getFromEnvOrConfig("DB_NAME", "db.name", "heartlake");
    dbConfig.user = getFromEnvOrConfig("DB_USER", "db.user", "postgres");
    dbConfig.password = getFromEnvOrConfig("DB_PASSWORD", "db.password", "postgres");
    dbConfig.pool_size = getIntFromEnvOrConfig("DB_POOL_SIZE", "db.pool_size", 20);
    return dbConfig;
}

ConfigManager::RedisConfig ConfigManager::getRedisConfig() {
    RedisConfig redisConfig;
    redisConfig.host = getFromEnvOrConfig("REDIS_HOST", "redis.host", "localhost");
    redisConfig.port = getIntFromEnvOrConfig("REDIS_PORT", "redis.port", 6379);
    redisConfig.password = getFromEnvOrConfig("REDIS_PASSWORD", "redis.password", "");
    redisConfig.db = getIntFromEnvOrConfig("REDIS_DB", "redis.db", 0);
    redisConfig.pool_size = getIntFromEnvOrConfig("REDIS_POOL_SIZE", "redis.pool_size", 30);
    return redisConfig;
}

ConfigManager::AIConfig ConfigManager::getAIConfig() {
    AIConfig aiConfig;
    aiConfig.provider = getFromEnvOrConfig("AI_PROVIDER", "ai.provider", "deepseek");
    aiConfig.api_key = getFromEnvOrConfig("AI_API_KEY", "ai.api_key", "");
    aiConfig.base_url = getFromEnvOrConfig("AI_BASE_URL", "ai.base_url", "https://api.deepseek.com");
    aiConfig.model = getFromEnvOrConfig("AI_MODEL", "ai.model", "deepseek-chat");
    aiConfig.timeout = getIntFromEnvOrConfig("AI_TIMEOUT", "ai.timeout", 30);
    aiConfig.temperature = getDouble("AI_TEMPERATURE", 0.7);
    aiConfig.max_tokens = getIntFromEnvOrConfig("AI_MAX_TOKENS", "ai.max_tokens", 800);
    return aiConfig;
}

ConfigManager::SecurityConfig ConfigManager::getSecurityConfig() {
    SecurityConfig secConfig;
    secConfig.paseto_key = getFromEnvOrConfig("PASETO_KEY", "security.paseto_key", "");
    secConfig.paseto_expire_hours = getIntFromEnvOrConfig("PASETO_EXPIRE_HOURS", "security.paseto_expire_hours", 168);
    secConfig.password_salt_rounds = getIntFromEnvOrConfig("PASSWORD_SALT_ROUNDS", "security.password_salt_rounds", 12);
    return secConfig;
}

ConfigManager::EmailConfig ConfigManager::getEmailConfig() {
    EmailConfig emailConfig;
    emailConfig.smtp_host = getFromEnvOrConfig("EMAIL_HOST", "email.smtp_host", "smtp.qq.com");
    emailConfig.smtp_port = getIntFromEnvOrConfig("EMAIL_PORT", "email.smtp_port", 587);
    emailConfig.smtp_user = getFromEnvOrConfig("EMAIL_USER", "email.smtp_user", "");
    emailConfig.smtp_password = getFromEnvOrConfig("EMAIL_PASSWORD", "email.smtp_password", "");
    emailConfig.from_address = getFromEnvOrConfig("EMAIL_FROM", "email.from_address", emailConfig.smtp_user);
    emailConfig.from_name = getFromEnvOrConfig("EMAIL_FROM_NAME", "email.from_name", "HeartLake");
    return emailConfig;
}

ConfigManager::RateLimitConfig ConfigManager::getRateLimitConfig() {
    RateLimitConfig rateConfig;
    rateConfig.enabled = getBoolFromEnvOrConfig("RATE_LIMIT_ENABLED", "rate_limit.enabled", true);
    rateConfig.stone_per_hour = getIntFromEnvOrConfig("RATE_STONE_PER_HOUR", "rate_limit.stone_per_hour", 15);
    rateConfig.boat_per_hour = getIntFromEnvOrConfig("RATE_BOAT_PER_HOUR", "rate_limit.boat_per_hour", 50);
    rateConfig.message_per_minute = getIntFromEnvOrConfig("RATE_MESSAGE_PER_MINUTE", "rate_limit.message_per_minute", 60);
    rateConfig.ai_per_hour = getIntFromEnvOrConfig("RATE_AI_PER_HOUR", "rate_limit.ai_per_hour", 30);
    return rateConfig;
}

ConfigManager::ServerConfig ConfigManager::getServerConfig() {
    ServerConfig serverConfig;
    serverConfig.listen_address = getFromEnvOrConfig("BIND_ADDRESS", "server.listen_address", "0.0.0.0");
    serverConfig.listen_port = getIntFromEnvOrConfig("BACKEND_PORT", "server.listen_port", 8080);
    serverConfig.thread_num = getIntFromEnvOrConfig("BACKEND_THREADS", "server.thread_num", 8);
    serverConfig.log_level = getFromEnvOrConfig("LOG_LEVEL", "server.log_level", "INFO");
    serverConfig.document_root = getFromEnvOrConfig("DOCUMENT_ROOT", "server.document_root", "./public");
    serverConfig.upload_path = getFromEnvOrConfig("UPLOAD_PATH", "server.upload_path", "./uploads");
    return serverConfig;
}

std::string ConfigManager::getFromEnvOrConfig(
    const std::string& envKey,
    const std::string& configKey,
    const std::string& defaultValue
) {
    // 1. 优先从环境变量读取
    const char* env = std::getenv(envKey.c_str());
    if (env && env[0] != '\0') {
        return env;
    }

    // 2. 从配置文件读取（支持嵌套）
    std::vector<std::string> keys;
    std::string key;
    for (char c : configKey) {
        if (c == '.') {
            keys.push_back(key);
            key.clear();
        } else {
            key += c;
        }
    }
    if (!key.empty()) {
        keys.push_back(key);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    Json::Value current = config_;
    for (const auto& k : keys) {
        if (current.isMember(k)) {
            current = current[k];
        } else {
            return defaultValue;
        }
    }

    if (current.isString()) {
        return current.asString();
    }

    return defaultValue;
}

int ConfigManager::getIntFromEnvOrConfig(
    const std::string& envKey,
    const std::string& configKey,
    int defaultValue
) {
    const char* env = std::getenv(envKey.c_str());
    if (env && env[0] != '\0') {
        return std::stoi(env);
    }
    
    std::string strValue = getFromEnvOrConfig("", configKey, "");
    if (!strValue.empty()) {
        return std::stoi(strValue);
    }
    
    return defaultValue;
}

bool ConfigManager::getBoolFromEnvOrConfig(
    const std::string& envKey,
    const std::string& configKey,
    bool defaultValue
) {
    const char* env = std::getenv(envKey.c_str());
    if (env && env[0] != '\0') {
        std::string value = env;
        return (value == "true" || value == "1" || value == "yes");
    }
    
    std::string strValue = getFromEnvOrConfig("", configKey, "");
    if (!strValue.empty()) {
        return (strValue == "true" || strValue == "1" || strValue == "yes");
    }
    
    return defaultValue;
}

} // namespace config
} // namespace heartlake
