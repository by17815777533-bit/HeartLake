/**
 * StructuredLogger 模块接口定义
 */

#pragma once

#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <unordered_map>
#include <mutex>
#include <json/json.h>
#include <drogon/drogon.h>

namespace heartlake {
namespace utils {

/**
 * 结构化日志工具
 * 支持JSON格式日志，便于日志收集和分析
 */
/**
 * 结构化日志记录器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class StructuredLogger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARN,
        ERR,  // Renamed from ERROR to avoid Windows macro conflict
        FATAL
    };
    
    static StructuredLogger& getInstance();
    
    /**
     * initialize方法
     *
     * @param jsonFormat 参数说明
     * @param minLevel 参数说明
     */
    void initialize(bool jsonFormat = true, Level minLevel = Level::INFO);
    
    /**
     * debug方法
     *
     * @param message 参数说明
     * @param context 参数说明
     */
    void debug(const std::string& message, const Json::Value& context = Json::Value());
    /**
     * info方法
     *
     * @param message 参数说明
     * @param context 参数说明
     */
    void info(const std::string& message, const Json::Value& context = Json::Value());
    /**
     * warn方法
     *
     * @param message 参数说明
     * @param context 参数说明
     */
    void warn(const std::string& message, const Json::Value& context = Json::Value());
    /**
     * error方法
     *
     * @param message 参数说明
     * @param context 参数说明
     */
    void error(const std::string& message, const Json::Value& context = Json::Value());
    /**
     * fatal方法
     *
     * @param message 参数说明
     * @param context 参数说明
     */
    void fatal(const std::string& message, const Json::Value& context = Json::Value());
    
    void logRequest(const drogon::HttpRequestPtr& req, 
                   const drogon::HttpResponsePtr& resp = nullptr,
                   double durationMs = 0.0);
    
    void logError(const std::string& errorType, 
                 const std::string& message,
                 const std::string& stackTrace = "",
                 const Json::Value& extra = Json::Value());
    
    void logBusiness(const std::string& event,
                    const std::string& userId = "",
                    const Json::Value& data = Json::Value());
    
    void logAI(const std::string& operation,
              const std::string& input,
              const std::string& output,
              double duration,
              bool success);
    
    /**
     * setGlobalContext方法
     *
     * @param key 参数说明
     * @param value 参数说明
     */
    void setGlobalContext(const std::string& key, const std::string& value);
    
    /**
     * log方法
     *
     * @param level 参数说明
     * @param message 参数说明
     * @param context 参数说明
     */
    void log(Level level, const std::string& message, const Json::Value& context);

    // 敏感信息脱敏
    static std::string maskEmail(const std::string& email);
    static std::string maskPhone(const std::string& phone);

private:
    StructuredLogger() = default;
    
    std::string levelToString(Level level);
    std::string getCurrentTimestamp();
    
    bool jsonFormat_ = true;
    Level minLevel_ = Level::INFO;
    std::unordered_map<std::string, std::string> globalContext_;
    std::mutex mutex_;
};

/**
 * LogBuilder类
 *
 * 详细说明
 *
 * @note 注意事项
 */
class LogBuilder {
public:
    LogBuilder(StructuredLogger::Level level);
    
    LogBuilder& message(const std::string& msg);
    LogBuilder& field(const std::string& key, const std::string& value);
    LogBuilder& field(const std::string& key, int value);
    LogBuilder& field(const std::string& key, double value);
    LogBuilder& field(const std::string& key, bool value);
    LogBuilder& userId(const std::string& id);
    LogBuilder& requestId(const std::string& id);
    LogBuilder& duration(double ms);
    LogBuilder& error(const std::string& err);
    
    /**
     * emit方法
     */
    void emit();
    
private:
    StructuredLogger::Level level_;
    std::string message_;
    Json::Value context_;
};

#define LOG_STRUCT_DEBUG(msg) heartlake::utils::StructuredLogger::getInstance().debug(msg)
#define LOG_STRUCT_INFO(msg) heartlake::utils::StructuredLogger::getInstance().info(msg)
#define LOG_STRUCT_WARN(msg) heartlake::utils::StructuredLogger::getInstance().warn(msg)
#define LOG_STRUCT_ERROR(msg) heartlake::utils::StructuredLogger::getInstance().error(msg)

#define LOG_BUILD_DEBUG() heartlake::utils::LogBuilder(heartlake::utils::StructuredLogger::Level::DEBUG)
#define LOG_BUILD_INFO() heartlake::utils::LogBuilder(heartlake::utils::StructuredLogger::Level::INFO)
#define LOG_BUILD_WARN() heartlake::utils::LogBuilder(heartlake::utils::StructuredLogger::Level::WARN)
#define LOG_BUILD_ERROR() heartlake::utils::LogBuilder(heartlake::utils::StructuredLogger::Level::ERR)

} // namespace utils
} // namespace heartlake
