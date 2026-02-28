/**
 * @brief 结构化日志记录器，支持JSON格式输出
 *
 * 单例模式，提供分级日志（DEBUG/INFO/WARN/ERR/FATAL）和场景化日志
 * （请求日志、错误日志、业务事件、智能引擎调用），便于ELK等日志平台采集分析。
 * 内置敏感信息脱敏（邮箱、手机号），线程安全。
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

class StructuredLogger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARN,
        ERR,  // 避免与 Windows ERROR 宏冲突
        FATAL
    };

    static StructuredLogger& getInstance();

    /**
     * @brief 初始化日志配置
     * @param jsonFormat 是否输出JSON格式（false则输出纯文本）
     * @param minLevel 最低输出级别，低于此级别的日志会被丢弃
     */
    void initialize(bool jsonFormat = true, Level minLevel = Level::INFO);

    /// 输出 DEBUG 级别日志
    void debug(const std::string& message, const Json::Value& context = Json::Value());
    /// 输出 INFO 级别日志
    void info(const std::string& message, const Json::Value& context = Json::Value());
    /// 输出 WARN 级别日志
    void warn(const std::string& message, const Json::Value& context = Json::Value());
    /// 输出 ERR 级别日志
    void error(const std::string& message, const Json::Value& context = Json::Value());
    /// 输出 FATAL 级别日志，通常伴随进程退出
    void fatal(const std::string& message, const Json::Value& context = Json::Value());

    /** @brief 记录HTTP请求日志，包含耗时和响应状态 */
    void logRequest(const drogon::HttpRequestPtr& req,
                   const drogon::HttpResponsePtr& resp = nullptr,
                   double durationMs = 0.0);

    /** @brief 记录异常/错误，附带堆栈和扩展信息 */
    void logError(const std::string& errorType,
                 const std::string& message,
                 const std::string& stackTrace = "",
                 const Json::Value& extra = Json::Value());

    /** @brief 记录业务事件（登录、发帖、点赞等） */
    void logBusiness(const std::string& event,
                    const std::string& userId = "",
                    const Json::Value& data = Json::Value());

    /** @brief 记录智能引擎调用（情感分析、推荐等），含输入输出和耗时 */
    void logAI(const std::string& operation,
              const std::string& input,
              const std::string& output,
              double duration,
              bool success);

    /** @brief 设置全局上下文字段，会附加到每条日志中（如服务名、版本号） */
    void setGlobalContext(const std::string& key, const std::string& value);

    /// 通用日志输出，由各级别快捷方法内部调用
    void log(Level level, const std::string& message, const Json::Value& context);

    /** @brief 邮箱脱敏，保留首尾字符和域名 */
    static std::string maskEmail(const std::string& email);
    /** @brief 手机号脱敏，保留前3后4 */
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
 * @brief 链式日志构建器，用于灵活组装结构化日志字段
 *
 * 用法示例: LogBuilder(Level::INFO).message("下单成功").userId("u123").duration(42.5).emit();
 */
class LogBuilder {
public:
    LogBuilder(StructuredLogger::Level level);

    /// 设置日志消息正文
    LogBuilder& message(const std::string& msg);
    /// 添加字符串类型的自定义字段
    LogBuilder& field(const std::string& key, const std::string& value);
    /// 添加整型自定义字段
    LogBuilder& field(const std::string& key, int value);
    /// 添加浮点型自定义字段
    LogBuilder& field(const std::string& key, double value);
    /// 添加布尔型自定义字段
    LogBuilder& field(const std::string& key, bool value);
    /// 设置关联的用户ID
    LogBuilder& userId(const std::string& id);
    /// 设置关联的请求ID，用于链路追踪
    LogBuilder& requestId(const std::string& id);
    /// 设置操作耗时（毫秒）
    LogBuilder& duration(double ms);
    /// 设置错误信息
    LogBuilder& error(const std::string& err);

    /** @brief 提交日志到 StructuredLogger */
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
