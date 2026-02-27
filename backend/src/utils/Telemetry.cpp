/**
 * OpenTelemetry可观测性模块实现
 */

#include "utils/Telemetry.h"
#include <drogon/drogon.h>

using namespace heartlake::utils;

// Span实现
Span::Span(const std::string& name, const std::string& parentTraceId,
           const std::string& parentSpanId) {
    data_.name = name;
    data_.serviceName = Telemetry::getInstance().serviceName();
    data_.traceId = parentTraceId.empty() ? Telemetry::generateId(16) : parentTraceId;
    data_.spanId = Telemetry::generateId(8);
    data_.parentSpanId = parentSpanId;
    data_.startTime = std::chrono::system_clock::now();
}

Span::~Span() {
    if (!ended_) {
        data_.endTime = std::chrono::system_clock::now();
        Telemetry::getInstance().recordSpan(data_);
    }
}

void Span::setAttribute(const std::string& key, const std::string& value) {
    data_.attributes[key] = value;
}

void Span::addEvent(const std::string& name) {
    data_.events.emplace_back(std::chrono::system_clock::now(), name);
}

void Span::setStatus(SpanStatus status) { data_.status = status; }

void Span::setError(const std::string& message) {
    data_.status = SpanStatus::ERROR;
    data_.attributes["error.message"] = message;
}

// Telemetry实现
Telemetry::Telemetry() {
    // 初始化延迟直方图桶
    std::lock_guard<std::mutex> lock(metrics_.bucketMutex);
    for (const auto& bucket : {"10", "50", "100", "500", "1000", "5000", "inf"}) {
        metrics_.latencyBuckets[bucket].store(0);
    }
}

Telemetry& Telemetry::getInstance() {
    static Telemetry instance;
    return instance;
}

void Telemetry::initialize(const std::string& serviceName, const std::string& otlpEndpoint) {
    serviceName_ = serviceName;
    otlpEndpoint_ = otlpEndpoint;
    initialized_ = true;
    LOG_INFO << "Telemetry initialized: " << serviceName_;
}

void Telemetry::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    spans_.clear();
    initialized_ = false;
}

std::unique_ptr<Span> Telemetry::startSpan(const std::string& name,
                                            const std::string& parentTraceId,
                                            const std::string& parentSpanId) {
    return std::make_unique<Span>(name, parentTraceId, parentSpanId);
}

std::unique_ptr<Span> Telemetry::startHttpSpan(const drogon::HttpRequestPtr& req) {
    auto [traceId, parentSpanId] = extractTraceContext(req);
    auto span = startSpan(std::string("HTTP ") + req->methodString() + " " + req->path(), traceId, parentSpanId);
    span->setAttribute("http.method", req->methodString());
    span->setAttribute("http.url", req->path());
    span->setAttribute("http.scheme", "http");
    return span;
}

void Telemetry::endHttpSpan(Span* span, const drogon::HttpResponsePtr& resp) {
    if (span && resp) {
        span->setAttribute("http.status_code", std::to_string(static_cast<int>(resp->statusCode())));
        if (resp->statusCode() >= drogon::k400BadRequest) {
            span->setStatus(SpanStatus::ERROR);
        }
    }
}

std::pair<std::string, std::string> Telemetry::extractTraceContext(const drogon::HttpRequestPtr& req) {
    // W3C Trace Context: traceparent header
    auto traceparent = req->getHeader("traceparent");
    if (!traceparent.empty() && traceparent.size() >= 55) {
        // Format: 00-traceId-spanId-flags
        return {traceparent.substr(3, 32), traceparent.substr(36, 16)};
    }
    return {"", ""};
}

void Telemetry::injectTraceContext(const drogon::HttpResponsePtr& resp,
                                   const std::string& traceId, const std::string& spanId) {
    if (resp && !traceId.empty()) {
        resp->addHeader("traceparent", "00-" + traceId + "-" + spanId + "-01");
    }
}

void Telemetry::recordSpan(const SpanData& span) {
    std::lock_guard<std::mutex> lock(mutex_);
    spans_.push_back(span);
    // 限制内存中span数量
    if (spans_.size() > 10000) {
        spans_.erase(spans_.begin(), spans_.begin() + 5000);
    }
}

std::string Telemetry::generateId(int bytes) {
    static thread_local std::mt19937_64 gen(std::random_device{}());
    std::ostringstream ss;
    for (int i = 0; i < bytes; ++i) {
        ss << std::hex << std::setfill('0') << std::setw(2) << (gen() & 0xFF);
    }
    return ss.str();
}

std::string Telemetry::exportOTLP() {
    std::lock_guard<std::mutex> lock(mutex_);
    Json::Value root;
    Json::Value resourceSpans(Json::arrayValue);
    Json::Value rs;

    // Resource
    rs["resource"]["attributes"][0]["key"] = "service.name";
    rs["resource"]["attributes"][0]["value"]["stringValue"] = serviceName_;

    Json::Value scopeSpans(Json::arrayValue);
    Json::Value ss;
    ss["scope"]["name"] = "heartlake.telemetry";

    Json::Value spansArr(Json::arrayValue);
    for (const auto& s : spans_) {
        Json::Value span;
        span["traceId"] = s.traceId;
        span["spanId"] = s.spanId;
        if (!s.parentSpanId.empty()) span["parentSpanId"] = s.parentSpanId;
        span["name"] = s.name;
        span["startTimeUnixNano"] = std::to_string(
            std::chrono::duration_cast<std::chrono::nanoseconds>(s.startTime.time_since_epoch()).count());
        span["endTimeUnixNano"] = std::to_string(
            std::chrono::duration_cast<std::chrono::nanoseconds>(s.endTime.time_since_epoch()).count());
        span["status"]["code"] = s.status == SpanStatus::OK ? 1 : 2;

        Json::Value attrs(Json::arrayValue);
        for (const auto& [k, v] : s.attributes) {
            Json::Value attr;
            attr["key"] = k;
            attr["value"]["stringValue"] = v;
            attrs.append(attr);
        }
        span["attributes"] = attrs;
        spansArr.append(span);
    }

    ss["spans"] = spansArr;
    scopeSpans.append(ss);
    rs["scopeSpans"] = scopeSpans;
    resourceSpans.append(rs);
    root["resourceSpans"] = resourceSpans;

    return Json::FastWriter().write(root);
}

void Telemetry::recordRequest(bool isError, uint64_t latencyMs) {
    metrics_.requestCount++;
    if (isError) metrics_.errorCount++;

    std::lock_guard<std::mutex> lock(metrics_.bucketMutex);
    if (latencyMs <= 10) metrics_.latencyBuckets["10"]++;
    else if (latencyMs <= 50) metrics_.latencyBuckets["50"]++;
    else if (latencyMs <= 100) metrics_.latencyBuckets["100"]++;
    else if (latencyMs <= 500) metrics_.latencyBuckets["500"]++;
    else if (latencyMs <= 1000) metrics_.latencyBuckets["1000"]++;
    else if (latencyMs <= 5000) metrics_.latencyBuckets["5000"]++;
    else metrics_.latencyBuckets["inf"]++;
}

std::string Telemetry::exportMetricsOTLP() {
    Json::Value root;
    Json::Value rm;
    rm["resource"]["attributes"][0]["key"] = "service.name";
    rm["resource"]["attributes"][0]["value"]["stringValue"] = serviceName_;

    Json::Value metrics(Json::arrayValue);

    // Request counter
    Json::Value reqMetric;
    reqMetric["name"] = "http.server.request.count";
    reqMetric["sum"]["dataPoints"][0]["asInt"] = Json::UInt64(metrics_.requestCount.load());
    reqMetric["sum"]["aggregationTemporality"] = 2;
    reqMetric["sum"]["isMonotonic"] = true;
    metrics.append(reqMetric);

    // Error counter
    Json::Value errMetric;
    errMetric["name"] = "http.server.error.count";
    errMetric["sum"]["dataPoints"][0]["asInt"] = Json::UInt64(metrics_.errorCount.load());
    errMetric["sum"]["aggregationTemporality"] = 2;
    errMetric["sum"]["isMonotonic"] = true;
    metrics.append(errMetric);

    // Latency histogram
    Json::Value latencyMetric;
    latencyMetric["name"] = "http.server.duration";
    Json::Value bounds(Json::arrayValue);
    Json::Value counts(Json::arrayValue);
    std::lock_guard<std::mutex> lock(metrics_.bucketMutex);
    for (const auto& b : {"10", "50", "100", "500", "1000", "5000"}) {
        bounds.append(std::stod(b));
        counts.append(Json::UInt64(metrics_.latencyBuckets[b].load()));
    }
    counts.append(Json::UInt64(metrics_.latencyBuckets["inf"].load()));
    latencyMetric["histogram"]["dataPoints"][0]["explicitBounds"] = bounds;
    latencyMetric["histogram"]["dataPoints"][0]["bucketCounts"] = counts;
    latencyMetric["histogram"]["aggregationTemporality"] = 2;
    metrics.append(latencyMetric);

    rm["scopeMetrics"][0]["scope"]["name"] = "heartlake.telemetry";
    rm["scopeMetrics"][0]["metrics"] = metrics;
    root["resourceMetrics"][0] = rm;

    return Json::FastWriter().write(root);
}
