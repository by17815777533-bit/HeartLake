/**
 * @brief OpenTelemetry 兼容的可观测性模块
 *
 * 实现分布式追踪（Trace）和指标采集（Metrics）两大支柱：
 * - Trace: 通过 Span 记录请求在各服务/函数间的调用链路和耗时
 * - Metrics: 原子计数器统计请求总量、错误数和延迟分布直方图
 *
 * 支持 W3C Trace Context 标准的 traceparent 头传播，
 * 可导出 OTLP JSON 格式供 Jaeger/Zipkin 等后端采集。
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

/// Span 执行状态
enum class SpanStatus { OK, ERROR };

/// 请求指标聚合数据（原子操作，线程安全）
struct MetricsData {
    std::atomic<uint64_t> requestCount{0};   ///< 总请求数
    std::atomic<uint64_t> errorCount{0};     ///< 错误请求数
    std::map<std::string, std::atomic<uint64_t>> latencyBuckets; ///< 延迟直方图桶（ms: 10/50/100/500/1000/5000）
    std::mutex bucketMutex;                  ///< 保护 latencyBuckets 的写入
};

/// 单个 Span 的完整数据
struct SpanData {
    std::string traceId;       ///< 16 字节 hex 编码的 trace 标识
    std::string spanId;        ///< 8 字节 hex 编码的 span 标识
    std::string parentSpanId;  ///< 父 span 标识，根 span 为空
    std::string name;          ///< span 名称（如 "HTTP GET /api/stones"）
    std::string serviceName;                ///< 所属服务名称
    std::chrono::system_clock::time_point startTime;   ///< span 开始时间
    std::chrono::system_clock::time_point endTime;     ///< span 结束时间
    SpanStatus status{SpanStatus::OK};
    std::map<std::string, std::string> attributes;  ///< 键值对属性
    std::vector<std::pair<std::chrono::system_clock::time_point, std::string>> events; ///< 时间线事件
};

/**
 * @brief RAII 风格的 Span 包装器
 *
 * 构造时记录开始时间，析构时自动结束并提交到 Telemetry。
 * 生命周期内可附加属性、事件和错误信息。
 */
class Span {
public:
    /**
     * @param name span 名称
     * @param parentTraceId 父 trace ID（跨服务传播时使用）
     * @param parentSpanId 父 span ID
     */
    Span(const std::string& name, const std::string& parentTraceId = "",
         const std::string& parentSpanId = "");
    ~Span();

    /// 附加键值对属性（如 http.method、http.status_code）
    void setAttribute(const std::string& key, const std::string& value);
    /// 记录时间线事件（如 "cache_miss"、"db_query_start"）
    void addEvent(const std::string& name);
    /// 设置 span 执行状态（OK 或 ERROR）
    void setStatus(SpanStatus status);
    /// 标记错误并设置错误消息
    void setError(const std::string& message);

    const std::string& traceId() const { return data_.traceId; }
    const std::string& spanId() const { return data_.spanId; }

private:
    SpanData data_;
    bool ended_{false};
};

/**
 * @brief 可观测性管理器（单例）
 *
 * 管理 Span 的创建、收集和导出，以及请求指标的聚合。
 * initialize() 需在服务启动时调用，设置服务名和可选的 OTLP 端点。
 */
class Telemetry {
public:
    static Telemetry& getInstance();

    /**
     * @brief 初始化可观测性模块
     * @param serviceName 服务名称，写入每个 span 的 service.name 属性
     * @param otlpEndpoint OTLP 采集端点 URL（为空则仅本地存储）
     */
    void initialize(const std::string& serviceName, const std::string& otlpEndpoint = "");
    /// 关闭模块，刷新未导出的 span
    void shutdown();

    /// 创建一个新的 Span，可选指定父 trace/span 实现链路串联
    std::unique_ptr<Span> startSpan(const std::string& name,
                                     const std::string& parentTraceId = "",
                                     const std::string& parentSpanId = "");

    /// 为 HTTP 请求创建 Span，自动提取 traceparent 头作为父上下文
    std::unique_ptr<Span> startHttpSpan(const drogon::HttpRequestPtr& req);
    /// 结束 HTTP Span，记录响应状态码和耗时
    void endHttpSpan(Span* span, const drogon::HttpResponsePtr& resp);

    /// 从请求的 traceparent 头解析 {traceId, spanId}
    static std::pair<std::string, std::string> extractTraceContext(const drogon::HttpRequestPtr& req);
    /// 将 trace context 注入响应头，供下游服务传播
    static void injectTraceContext(const drogon::HttpResponsePtr& resp,
                                   const std::string& traceId, const std::string& spanId);

    /// 内部使用：将已完成的 span 数据存入缓冲区
    void recordSpan(const SpanData& span);

    /// 导出所有缓冲的 span 为 OTLP JSON 格式
    std::string exportOTLP();
    /// 导出 metrics 为 OTLP JSON 格式
    std::string exportMetricsOTLP();

    /// 记录一次请求的结果和延迟，更新 metrics 计数器
    void recordRequest(bool isError, uint64_t latencyMs);
    uint64_t getRequestCount() const { return metrics_.requestCount.load(); }
    uint64_t getErrorCount() const { return metrics_.errorCount.load(); }

    const std::string& serviceName() const { return serviceName_; }

private:
    Telemetry();

    friend class Span;
    /// 生成指定字节数的随机 hex ID（traceId 16 字节，spanId 8 字节）
    static std::string generateId(int bytes);

    std::string serviceName_{"heartlake"};
    std::string otlpEndpoint_;
    std::vector<SpanData> spans_;
    std::mutex mutex_;
    bool initialized_{false};
    MetricsData metrics_;
};

/// 便捷宏：在当前作用域创建一个自动管理生命周期的 Span
#define OTEL_SPAN(name) \
    auto _otel_span_ = heartlake::utils::Telemetry::getInstance().startSpan(name)

/// 便捷宏：创建带父上下文的 Span
#define OTEL_SPAN_WITH_PARENT(name, traceId, spanId) \
    auto _otel_span_ = heartlake::utils::Telemetry::getInstance().startSpan(name, traceId, spanId)

} // namespace utils
} // namespace heartlake
