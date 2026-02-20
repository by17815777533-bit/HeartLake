/**
 * @file Telemetry.h
 * @brief OpenTelemetry可观测性模块 - 支持Trace/Metrics/Logs三大支柱
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <functional>
#include <random>
#include <sstream>
#include <iomanip>
#include <vector>
#include <mutex>
#include <atomic>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

namespace heartlake {
namespace utils {

// Span状态
enum class SpanStatus { OK, ERROR };

// Metrics数据
struct MetricsData {
    std::atomic<uint64_t> requestCount{0};
    std::atomic<uint64_t> errorCount{0};
    std::map<std::string, std::atomic<uint64_t>> latencyBuckets; // ms buckets: 10,50,100,500,1000,5000
    std::mutex bucketMutex;
};

// Span数据结构
struct SpanData {
    std::string traceId;
    std::string spanId;
    std::string parentSpanId;
    std::string name;
    std::string serviceName;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    SpanStatus status{SpanStatus::OK};
    std::map<std::string, std::string> attributes;
    std::vector<std::pair<std::chrono::system_clock::time_point, std::string>> events;
};

// Span RAII包装器
class Span {
public:
    Span(const std::string& name, const std::string& parentTraceId = "",
         const std::string& parentSpanId = "");
    ~Span();

    void setAttribute(const std::string& key, const std::string& value);
    void addEvent(const std::string& name);
    void setStatus(SpanStatus status);
    void setError(const std::string& message);

    const std::string& traceId() const { return data_.traceId; }
    const std::string& spanId() const { return data_.spanId; }

private:
    SpanData data_;
    bool ended_{false};
};

// Telemetry主类
class Telemetry {
public:
    static Telemetry& getInstance();

    void initialize(const std::string& serviceName, const std::string& otlpEndpoint = "");
    void shutdown();

    // 创建Span
    std::unique_ptr<Span> startSpan(const std::string& name,
                                     const std::string& parentTraceId = "",
                                     const std::string& parentSpanId = "");

    // HTTP请求追踪
    std::unique_ptr<Span> startHttpSpan(const drogon::HttpRequestPtr& req);
    void endHttpSpan(Span* span, const drogon::HttpResponsePtr& resp);

    // 从请求头提取trace context
    static std::pair<std::string, std::string> extractTraceContext(const drogon::HttpRequestPtr& req);

    // 注入trace context到响应头
    static void injectTraceContext(const drogon::HttpResponsePtr& resp,
                                   const std::string& traceId, const std::string& spanId);

    // 记录span（内部使用）
    void recordSpan(const SpanData& span);

    // 导出OTLP JSON格式
    std::string exportOTLP();
    std::string exportMetricsOTLP();

    // Metrics
    void recordRequest(bool isError, uint64_t latencyMs);
    uint64_t getRequestCount() const { return metrics_.requestCount.load(); }
    uint64_t getErrorCount() const { return metrics_.errorCount.load(); }

    const std::string& serviceName() const { return serviceName_; }

private:
    Telemetry();

    friend class Span;
    static std::string generateId(int bytes);

    std::string serviceName_{"heartlake"};
    std::string otlpEndpoint_;
    std::vector<SpanData> spans_;
    std::mutex mutex_;
    bool initialized_{false};
    MetricsData metrics_;
};

// 便捷宏
#define OTEL_SPAN(name) \
    auto _otel_span_ = heartlake::utils::Telemetry::getInstance().startSpan(name)

#define OTEL_SPAN_WITH_PARENT(name, traceId, spanId) \
    auto _otel_span_ = heartlake::utils::Telemetry::getInstance().startSpan(name, traceId, spanId)

} // namespace utils
} // namespace heartlake
