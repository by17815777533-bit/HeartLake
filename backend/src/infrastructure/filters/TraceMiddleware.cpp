/**
 * 请求链路追踪中间件实现
 *
 * 兼容 OpenTelemetry 规范的轻量级 trace 方案。
 * 每个请求分配 128-bit trace_id（支持上游网关透传）和 64-bit span_id，
 * 注入 request attributes 后由 postHandlingAdvice 统一写入响应头和耗时日志。
 * 使用 thread_local mt19937_64 避免跨线程竞争。
 */

#include "infrastructure/filters/TraceMiddleware.h"
#include <drogon/drogon.h>
#include <sstream>
#include <iomanip>

namespace heartlake {
namespace filters {

thread_local std::mt19937_64 TraceMiddleware::rng_{
    static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())};

void TraceMiddleware::doFilter(const drogon::HttpRequestPtr& req,
                               drogon::FilterCallback&& fcb,
                               drogon::FilterChainCallback&& fccb) {
    auto startTime = std::chrono::steady_clock::now();

    // 读取或生成 trace_id（支持上游 nginx/网关传入）
    std::string traceId = req->getHeader("X-Trace-Id");
    if (traceId.empty()) {
        traceId = generateTraceId();
    }

    // 每个请求生成新的 span_id
    std::string spanId = generateSpanId();

    // 注入 request attributes，供下游 Controller 和 postHandlingAdvice 使用
    req->getAttributes()->insert("trace_id", traceId);
    req->getAttributes()->insert("span_id", spanId);
    req->getAttributes()->insert("trace_start_time",
        std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
            startTime.time_since_epoch()).count()));

    // 继续过滤器链，响应头注入和耗时日志由 main.cpp 中的 postHandlingAdvice 统一处理
    fccb();
}

std::string TraceMiddleware::generateTraceId() {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << rng_();
    oss << std::setw(16) << rng_();
    return oss.str();
}

std::string TraceMiddleware::generateSpanId() {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << rng_();
    return oss.str();
}

} // namespace filters
} // namespace heartlake
