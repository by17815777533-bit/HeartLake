/**
 * @file MetricsCollector.cpp
 * @brief MetricsCollector 模块实现
 * Created by 王璐瑶
 */
#include "utils/MetricsCollector.h"
#include <sstream>
#include <iomanip>
#include <set>
#include <drogon/drogon.h>

using namespace heartlake::utils;

MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

void MetricsCollector::initialize() {
    startTime_ = std::chrono::steady_clock::now();
    
    // 初始化默认histogram的buckets
    std::vector<double> defaultBuckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
    
    // HTTP请求延迟histogram
    HistogramMetric httpLatency;
    httpLatency.name = "http_request_duration_seconds";
    httpLatency.help = "HTTP request latency in seconds";
    httpLatency.buckets = defaultBuckets;
    httpLatency.bucketCounts.resize(defaultBuckets.size() + 1);
    histograms_["http_request_duration_seconds"] = std::move(httpLatency);
    
    // DB查询延迟histogram
    HistogramMetric dbLatency;
    dbLatency.name = "db_query_duration_seconds";
    dbLatency.help = "Database query latency in seconds";
    dbLatency.buckets = defaultBuckets;
    dbLatency.bucketCounts.resize(defaultBuckets.size() + 1);
    histograms_["db_query_duration_seconds"] = std::move(dbLatency);
    
    // AI请求延迟histogram
    HistogramMetric aiLatency;
    aiLatency.name = "ai_request_duration_seconds";
    aiLatency.help = "AI API request latency in seconds";
    aiLatency.buckets = {0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0};
    aiLatency.bucketCounts.resize(8);
    histograms_["ai_request_duration_seconds"] = std::move(aiLatency);
    
    LOG_INFO << "Metrics collector initialized";
}

void MetricsCollector::incrementCounter(const std::string& name, double value,
                                        const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeKey(name, labels);
    
    if (counters_.find(key) == counters_.end()) {
        Metric m;
        m.name = name;
        m.type = "counter";
        m.labels = labels;
        counters_[key] = m;
    }
    
    counters_[key].value += value;
}

void MetricsCollector::setGauge(const std::string& name, double value,
                                const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeKey(name, labels);
    
    if (gauges_.find(key) == gauges_.end()) {
        Metric m;
        m.name = name;
        m.type = "gauge";
        m.labels = labels;
        gauges_[key] = m;
    }
    
    gauges_[key].value = value;
}

void MetricsCollector::incrementGauge(const std::string& name, double delta,
                                      const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeKey(name, labels);
    
    if (gauges_.find(key) == gauges_.end()) {
        Metric m;
        m.name = name;
        m.type = "gauge";
        m.labels = labels;
        gauges_[key] = m;
    }
    
    gauges_[key].value += delta;
}

void MetricsCollector::decrementGauge(const std::string& name, double delta,
                                      const std::map<std::string, std::string>& labels) {
    incrementGauge(name, -delta, labels);
}

void MetricsCollector::observeHistogram(const std::string& name, double value,
                                        [[maybe_unused]] const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = histograms_.find(name);
    if (it == histograms_.end()) {
        return;
    }

    auto& hist = it->second;
    hist.sum += value;
    hist.count++;

    // 找到对应的bucket（只增加第一个满足条件的bucket，exportMetrics会做累积）
    for (size_t i = 0; i < hist.buckets.size(); ++i) {
        if (value <= hist.buckets[i]) {
            hist.bucketCounts[i]++;
            return;
        }
    }
    // 超出所有bucket范围，放入+Inf bucket
    hist.bucketCounts[hist.buckets.size()]++;
}

void MetricsCollector::recordHttpRequest(const std::string& method, const std::string& path,
                                         int statusCode, double duration) {
    totalRequests_++;
    if (statusCode >= 400) {
        errorRequests_++;
    }
    
    std::map<std::string, std::string> labels = {
        {"method", method},
        {"path", path},
        {"status", std::to_string(statusCode)}
    };
    
    incrementCounter("http_requests_total", 1.0, labels);
    observeHistogram("http_request_duration_seconds", duration, labels);
}

void MetricsCollector::recordDbQuery(const std::string& operation, double duration, bool success) {
    std::map<std::string, std::string> labels = {
        {"operation", operation},
        {"success", success ? "true" : "false"}
    };
    
    incrementCounter("db_queries_total", 1.0, labels);
    observeHistogram("db_query_duration_seconds", duration, labels);
}

void MetricsCollector::recordAIRequest(const std::string& type, double duration, bool success) {
    std::map<std::string, std::string> labels = {
        {"type", type},
        {"success", success ? "true" : "false"}
    };
    
    incrementCounter("ai_requests_total", 1.0, labels);
    observeHistogram("ai_request_duration_seconds", duration, labels);
}

void MetricsCollector::recordCacheHit(const std::string& type, bool hit) {
    std::map<std::string, std::string> labels = {
        {"type", type},
        {"hit", hit ? "true" : "false"}
    };
    
    incrementCounter("cache_operations_total", 1.0, labels);
}

void MetricsCollector::recordActiveConnections(int count) {
    setGauge("active_connections", static_cast<double>(count));
}

void MetricsCollector::recordMemoryUsage(size_t bytes) {
    setGauge("memory_usage_bytes", static_cast<double>(bytes));
}

void MetricsCollector::recordError(const std::string& type, [[maybe_unused]] const std::string& message) {
    std::map<std::string, std::string> labels = {
        {"type", type}
    };
    
    incrementCounter("errors_total", 1.0, labels);
}

std::string MetricsCollector::exportMetrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    std::set<std::string> seenNames;

    // Counters
    for (const auto& [key, metric] : counters_) {
        if (seenNames.find(metric.name) == seenNames.end()) {
            ss << "# HELP " << metric.name << " Counter metric\n";
            ss << "# TYPE " << metric.name << " counter\n";
            seenNames.insert(metric.name);
        }
        ss << metric.name << formatLabels(metric.labels) << " " << metric.value << "\n";
    }

    // Gauges
    seenNames.clear();
    for (const auto& [key, metric] : gauges_) {
        if (seenNames.find(metric.name) == seenNames.end()) {
            ss << "# HELP " << metric.name << " Gauge metric\n";
            ss << "# TYPE " << metric.name << " gauge\n";
            seenNames.insert(metric.name);
        }
        ss << metric.name << formatLabels(metric.labels) << " " << metric.value << "\n";
    }
    
    // Histograms
    for (const auto& [key, hist] : histograms_) {
        ss << "# HELP " << hist.name << " " << hist.help << "\n";
        ss << "# TYPE " << hist.name << " histogram\n";
        
        uint64_t cumulative = 0;
        for (size_t i = 0; i < hist.buckets.size(); ++i) {
            cumulative += hist.bucketCounts[i];
            ss << hist.name << "_bucket{le=\"" << hist.buckets[i] << "\"} " << cumulative << "\n";
        }
        cumulative += hist.bucketCounts[hist.buckets.size()];
        ss << hist.name << "_bucket{le=\"+Inf\"} " << cumulative << "\n";
        ss << hist.name << "_sum " << hist.sum << "\n";
        ss << hist.name << "_count " << hist.count << "\n";
    }
    
    // 运行时间
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime_
    ).count();
    ss << "# HELP process_uptime_seconds Process uptime in seconds\n";
    ss << "# TYPE process_uptime_seconds gauge\n";
    ss << "process_uptime_seconds " << uptime << "\n";
    
    return ss.str();
}

HttpResponsePtr MetricsCollector::metricsEndpoint() {
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_PLAIN);
    resp->setBody(exportMetrics());
    return resp;
}

Json::Value MetricsCollector::getStats() {
    Json::Value stats;
    
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime_
    ).count();
    
    stats["uptime_seconds"] = static_cast<Json::Int64>(uptime);
    stats["total_requests"] = static_cast<Json::UInt64>(totalRequests_.load());
    stats["error_requests"] = static_cast<Json::UInt64>(errorRequests_.load());
    
    if (totalRequests_ > 0) {
        stats["error_rate"] = static_cast<double>(errorRequests_.load()) / totalRequests_.load();
    } else {
        stats["error_rate"] = 0.0;
    }
    
    return stats;
}

std::string MetricsCollector::makeKey(const std::string& name, 
                                      const std::map<std::string, std::string>& labels) {
    std::ostringstream ss;
    ss << name;
    for (const auto& [k, v] : labels) {
        ss << "," << k << "=" << v;
    }
    return ss.str();
}

std::string MetricsCollector::formatLabels(const std::map<std::string, std::string>& labels) {
    if (labels.empty()) return "";
    
    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& [k, v] : labels) {
        if (!first) ss << ",";
        ss << k << "=\"" << v << "\"";
        first = false;
    }
    ss << "}";
    return ss.str();
}
