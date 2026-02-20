/**
 * @file ConfigManager.h
 * @brief ConfigManager 模块接口定义
 * Created by 白洋
 */

#pragma once
#include <json/json.h>
#include <string>
#include <optional>
#include <mutex>
#include <atomic>

namespace heartlake {
namespace config {

/**
 * @brief 配置管理器，用于管理应用配置
 *
 * 详细说明
 *
 * @note 注意事项
 */
class ConfigManager {
public:
    static ConfigManager& getInstance();

    /**
     * 加载配置（按优先级）
     */
    void load(const std::string& configFile = "config.json");

    /**
     * 热重载配置，失败时自动回滚
     */
    bool reload();
    
    /**
     * 获取字符串配置
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "");
    
    /**
     * 获取整数配置
     */
    int getInt(const std::string& key, int defaultValue = 0);
    
    /**
     * 获取布尔配置
     */
    bool getBool(const std::string& key, bool defaultValue = false);
    
    /**
     * 获取浮点数配置
     */
    double getDouble(const std::string& key, double defaultValue = 0.0);
    
    /**
     * 获取JSON对象配置
     */
    Json::Value getJson(const std::string& key);
    
    /**
     * 数据库配置
     */
    struct DatabaseConfig {
        std::string host;
        int port;
        std::string name;
        std::string user;
        std::string password;
        int pool_size;
    };
    /**
     * @brief getDatabaseConfig方法
     * @return 返回值说明
     */
    DatabaseConfig getDatabaseConfig();
    
    /**
     * Redis配置
     */
    struct RedisConfig {
        std::string host;
        int port;
        std::string password;
        int db;
        int pool_size;
    };
    /**
     * @brief getRedisConfig方法
     * @return 返回值说明
     */
    RedisConfig getRedisConfig();
    
    /**
     * AI配置
     */
    struct AIConfig {
        std::string provider;
        std::string api_key;
        std::string base_url;
        std::string model;
        int timeout;
        double temperature;
        int max_tokens;
    };
    /**
     * @brief getAIConfig方法
     * @return 返回值说明
     */
    AIConfig getAIConfig();
    
    /**
     * 安全配置
     */
    struct SecurityConfig {
        std::string paseto_key;
        int paseto_expire_hours;
        int password_salt_rounds;
    };
    /**
     * @brief getSecurityConfig方法
     * @return 返回值说明
     */
    SecurityConfig getSecurityConfig();
    
    /**
     * 邮件配置
     */
    struct EmailConfig {
        std::string smtp_host;
        int smtp_port;
        std::string smtp_user;
        std::string smtp_password;
        std::string from_address;
        std::string from_name;
    };
    /**
     * @brief getEmailConfig方法
     * @return 返回值说明
     */
    EmailConfig getEmailConfig();
    
    /**
     * 限流配置
     */
    struct RateLimitConfig {
        bool enabled;
        int stone_per_hour;
        int boat_per_hour;
        int message_per_minute;
        int ai_per_hour;
    };
    /**
     * @brief getRateLimitConfig方法
     * @return 返回值说明
     */
    RateLimitConfig getRateLimitConfig();
    
    /**
     * 服务器配置
     */
    struct ServerConfig {
        std::string listen_address;
        int listen_port;
        int thread_num;
        std::string log_level;
        std::string document_root;
        std::string upload_path;
    };
    /**
     * @brief getServerConfig方法
     * @return 返回值说明
     */
    ServerConfig getServerConfig();

private:
    ConfigManager() = default;

    std::string getFromEnvOrConfig(
        const std::string& envKey,
        const std::string& configKey,
        const std::string& defaultValue = ""
    );

    int getIntFromEnvOrConfig(
        const std::string& envKey,
        const std::string& configKey,
        int defaultValue = 0
    );

    bool getBoolFromEnvOrConfig(
        const std::string& envKey,
        const std::string& configKey,
        bool defaultValue = false
    );

    mutable std::mutex mutex_;
    Json::Value config_;
    std::string configFile_;
};

} // namespace config
} // namespace heartlake
