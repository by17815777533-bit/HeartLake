/**
 * @file TraceFilter.h
 * @brief OpenTelemetry追踪过滤器
 */

#pragma once

#include <drogon/drogon.h>
#include <drogon/HttpFilter.h>
#include "utils/Telemetry.h"

namespace heartlake {
namespace filters {

// 线程局部存储 Trace Context
class TraceContext {
public:
    static void set(const std::string& traceId, const std::string& spanId = "") {
        traceId_ = traceId;
        spanId_ = spanId;
    }
    static std::string getTraceId() { return traceId_; }
    static std::string getSpanId() { return spanId_; }
    static void clear() { traceId_.clear(); spanId_.clear(); }

private:
    static thread_local std::string traceId_;
    static thread_local std::string spanId_;
};

/**
 * @brief OpenTelemetry追踪过滤器
 */
class TraceFilter : public drogon::HttpFilter<TraceFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                 drogon::FilterCallback &&fcb,
                 drogon::FilterChainCallback &&fccb) override {
        auto startTime = std::chrono::steady_clock::now();
        auto span = utils::Telemetry::getInstance().startHttpSpan(req);

        TraceContext::set(span->traceId(), span->spanId());
        req->getAttributes()->insert("trace_id", span->traceId());
        req->getAttributes()->insert("span_id", span->spanId());
        req->getAttributes()->insert("otel_span", std::shared_ptr<utils::Span>(span.release()));
        req->getAttributes()->insert("start_time", startTime);

        fccb();
    }
};

// 响应后处理（在Controller中调用）
inline void finalizeTrace(const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
    auto spanPtr = req->getAttributes()->get<std::shared_ptr<utils::Span>>("otel_span");
    if (spanPtr) {
        utils::Telemetry::getInstance().endHttpSpan(spanPtr.get(), resp);
        utils::Telemetry::injectTraceContext(resp, spanPtr->traceId(), spanPtr->spanId());

        auto startTime = req->getAttributes()->get<std::chrono::steady_clock::time_point>("start_time");
        auto latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        bool isError = resp->statusCode() >= drogon::k400BadRequest;
        utils::Telemetry::getInstance().recordRequest(isError, latencyMs);
    }
    TraceContext::clear();
}

} // namespace filters
} // namespace heartlake
