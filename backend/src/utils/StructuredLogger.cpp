/**
 * @file StructuredLogger.cpp
 * @brief StructuredLogger 模块实现
 * Created by 林子怡
 */
#include "utils/StructuredLogger.h"
#include <iostream>
#ifdef _WIN32
#undef ERROR
#endif

using namespace heartlake::utils;
using namespace drogon;

StructuredLogger& StructuredLogger::getInstance() {
    static StructuredLogger instance;
    return instance;
}

void StructuredLogger::initialize(bool jsonFormat, Level minLevel) {
    jsonFormat_ = jsonFormat;
    minLevel_ = minLevel;
    
    LOG_INFO << "Structured logger initialized, JSON format: " 
             << (jsonFormat ? "enabled" : "disabled");
}

void StructuredLogger::debug(const std::string& message, const Json::Value& context) {
    log(Level::DEBUG, message, context);
}

void StructuredLogger::info(const std::string& message, const Json::Value& context) {
    log(Level::INFO, message, context);
}

void StructuredLogger::warn(const std::string& message, const Json::Value& context) {
    log(Level::WARN, message, context);
}

void StructuredLogger::error(const std::string& message, const Json::Value& context) {
    log(Level::ERR, message, context);
}

void StructuredLogger::fatal(const std::string& message, const Json::Value& context) {
    log(Level::FATAL, message, context);
}

void StructuredLogger::log(Level level, const std::string& message, const Json::Value& context) {
    if (level < minLevel_) return;
    
    if (jsonFormat_) {
        Json::Value logEntry;
        logEntry["timestamp"] = getCurrentTimestamp();
        logEntry["level"] = levelToString(level);
        logEntry["message"] = message;
        
        // 添加全局上下文
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [k, v] : globalContext_) {
                logEntry[k] = v;
            }
        }
        
        // 添加附加上下文
        if (!context.isNull()) {
            for (const auto& key : context.getMemberNames()) {
                logEntry[key] = context[key];
            }
        }
        
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string jsonStr = Json::writeString(writer, logEntry);
        
        switch (level) {
            case Level::DEBUG: LOG_DEBUG << jsonStr; break;
            case Level::INFO: LOG_INFO << jsonStr; break;
            case Level::WARN: LOG_WARN << jsonStr; break;
            case Level::ERR: LOG_ERROR << jsonStr; break;
            case Level::FATAL: LOG_FATAL << jsonStr; break;
        }
    } else {
        switch (level) {
            case Level::DEBUG: LOG_DEBUG << message; break;
            case Level::INFO: LOG_INFO << message; break;
            case Level::WARN: LOG_WARN << message; break;
            case Level::ERR: LOG_ERROR << message; break;
            case Level::FATAL: LOG_FATAL << message; break;
        }
    }
}

void StructuredLogger::logRequest(const HttpRequestPtr& req, 
                                  const HttpResponsePtr& resp,
                                  double durationMs) {
    Json::Value context;
    context["method"] = req->methodString();
    context["path"] = req->path();
    context["client_ip"] = req->peerAddr().toIp();
    context["duration_ms"] = durationMs;
    
    if (resp) {
        context["status"] = static_cast<int>(resp->statusCode());
    }
    
    std::string userAgent = req->getHeader("User-Agent");
    if (!userAgent.empty()) {
        context["user_agent"] = userAgent;
    }
    
    std::string requestId = req->getHeader("X-Request-Id");
    if (!requestId.empty()) {
        context["request_id"] = requestId;
    }
    
    log(Level::INFO, "HTTP Request", context);
}

void StructuredLogger::logError(const std::string& errorType,
                               const std::string& message,
                               const std::string& stackTrace,
                               const Json::Value& extra) {
    Json::Value context;
    context["error_type"] = errorType;
    context["error_message"] = message;
    
    if (!stackTrace.empty()) {
        context["stack_trace"] = stackTrace;
    }
    
    if (!extra.isNull()) {
        for (const auto& key : extra.getMemberNames()) {
            context[key] = extra[key];
        }
    }
    
    log(Level::ERR, "Error occurred", context);
}

void StructuredLogger::logBusiness(const std::string& event,
                                  const std::string& userId,
                                  const Json::Value& data) {
    Json::Value context;
    context["event"] = event;
    
    if (!userId.empty()) {
        context["user_id"] = userId;
    }
    
    if (!data.isNull()) {
        context["data"] = data;
    }
    
    log(Level::INFO, "Business event", context);
}

void StructuredLogger::logAI(const std::string& operation,
                            const std::string& input,
                            const std::string& output,
                            double duration,
                            bool success) {
    Json::Value context;
    context["operation"] = operation;
    context["input_length"] = static_cast<int>(input.length());
    context["output_length"] = static_cast<int>(output.length());
    context["duration_ms"] = duration;
    context["success"] = success;
    
    log(success ? Level::INFO : Level::WARN, "AI operation", context);
}

void StructuredLogger::setGlobalContext(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalContext_[key] = value;
}

std::string StructuredLogger::levelToString(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERR: return "ERROR";
        case Level::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string StructuredLogger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count() % 1000;

    std::ostringstream ss;
    struct tm tm_buf;
    gmtime_r(&time, &tm_buf);
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms << "Z";
    return ss.str();
}

std::string StructuredLogger::maskEmail(const std::string& email) {
    size_t at = email.find('@');
    if (at == std::string::npos || at < 2) return "***@***.***";
    return email.substr(0, 2) + "***" + email.substr(at);
}

std::string StructuredLogger::maskPhone(const std::string& phone) {
    if (phone.length() < 7) return "***";
    return phone.substr(0, 3) + "****" + phone.substr(phone.length() - 4);
}

LogBuilder::LogBuilder(StructuredLogger::Level level) : level_(level) {}

LogBuilder& LogBuilder::message(const std::string& msg) {
    message_ = msg;
    return *this;
}

LogBuilder& LogBuilder::field(const std::string& key, const std::string& value) {
    context_[key] = value;
    return *this;
}

LogBuilder& LogBuilder::field(const std::string& key, int value) {
    context_[key] = value;
    return *this;
}

LogBuilder& LogBuilder::field(const std::string& key, double value) {
    context_[key] = value;
    return *this;
}

LogBuilder& LogBuilder::field(const std::string& key, bool value) {
    context_[key] = value;
    return *this;
}

LogBuilder& LogBuilder::userId(const std::string& id) {
    context_["user_id"] = id;
    return *this;
}

LogBuilder& LogBuilder::requestId(const std::string& id) {
    context_["request_id"] = id;
    return *this;
}

LogBuilder& LogBuilder::duration(double ms) {
    context_["duration_ms"] = ms;
    return *this;
}

LogBuilder& LogBuilder::error(const std::string& err) {
    context_["error"] = err;
    return *this;
}

void LogBuilder::emit() {
    StructuredLogger::getInstance().log(level_, message_, context_);
}
