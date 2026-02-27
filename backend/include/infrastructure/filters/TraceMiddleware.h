/**
 * OpenTelemetry 风格的请求链路追踪中间件
 *
 * 功能：
 * - 读取或生成 X-Trace-Id（分布式追踪 ID）
 * - 生成 Span-Id（当前请求跨度 ID）
 * - 将 trace_id / span_id 注入 request attributes，供下游 Controller 使用
 * - 在响应头中返回 X-Trace-Id 和 X-Span-Id
 * - 记录请求耗时日志
 */

#pragma once

#include <drogon/HttpFilter.h>
#include <string>
#include <random>
#include <chrono>

namespace heartlake {
namespace filters {

class TraceMiddleware : public drogon::HttpFilter<TraceMiddleware> {
public:
    TraceMiddleware() = default;

    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;

    /// 生成 32 字符的十六进制 trace ID
    static std::string generateTraceId();

    /// 生成 16 字符的十六进制 span ID
    static std::string generateSpanId();

private:
    static thread_local std::mt19937_64 rng_;
};

} // namespace filters
} // namespace heartlake
