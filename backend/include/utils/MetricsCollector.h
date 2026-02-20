/**
 * @file MetricsCollector.h
 * @brief MetricsCollector 模块接口定义
 * Created by 王璐瑶
 */

#pragma once

#include <string>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <functional>
#include <drogon/HttpResponse.h>

using namespace drogon;

namespace heartlake {
namespace utils {

/**
 * Prometheus Metrics 收集器
 * 支持：Counter, Gauge, Histogram
 */
/**
 * @brief 指标收集器，用于收集系统指标
 *
 * 详细说明
 *
 * @note 注意事项
 */
class MetricsCollector {
public:
    static MetricsCollector& getInstance();
    
    /**
     * @brief initialize方法
     */
    void initialize();
    
    void incrementCounter(const std::string& name, double value = 1.0, 
                         const std::map<std::string, std::string>& labels = {});
    
    void setGauge(const std::string& name, double value,
                  const std::map<std::string, std::string>& labels = {});
    void incrementGauge(const std::string& name, double delta = 1.0,
                        const std::map<std::string, std::string>& labels = {});
    void decrementGauge(const std::string& name, double delta = 1.0,
                        const std::map<std::string, std::string>& labels = {});
    
    void observeHistogram(const std::string& name, double value,
                         const std::map<std::string, std::string>& labels = {});
    
    void recordHttpRequest(const std::string& method, const std::string& path, 
                          int statusCode, double duration);
    /**
     * @brief recordDbQuery方法
     *
     * @param operation 参数说明
     * @param duration 参数说明
     * @param success 参数说明
     */
    void recordDbQuery(const std::string& operation, double duration, bool success);
    /**
     * @brief recordAIRequest方法
     *
     * @param type 参数说明
     * @param duration 参数说明
     * @param success 参数说明
     */
    void recordAIRequest(const std::string& type, double duration, bool success);
    /**
     * @brief recordCacheHit方法
     *
     * @param type 参数说明
     * @param hit 参数说明
     */
    void recordCacheHit(const std::string& type, bool hit);
    /**
     * @brief recordActiveConnections方法
     *
     * @param count 参数说明
     */
    void recordActiveConnections(int count);
    /**
     * @brief recordMemoryUsage方法
     *
     * @param bytes 参数说明
     */
    void recordMemoryUsage(size_t bytes);
    /**
     * @brief recordError方法
     *
     * @param type 参数说明
     * @param message 参数说明
     */
    void recordError(const std::string& type, const std::string& message);
    
    std::string exportMetrics();
    /**
     * @brief metricsEndpoint方法
     * @return 返回值说明
     */
    HttpResponsePtr metricsEndpoint();
    
    Json::Value getStats();
    
private:
    MetricsCollector() = default;
    
    struct Metric {
        std::string name;
        std::string help;
        std::string type; // counter, gauge, histogram
        double value{0.0};
        std::map<std::string, std::string> labels;
    };
    
    struct HistogramMetric {
        std::string name;
        std::string help;
        std::vector<double> buckets;
        std::vector<uint64_t> bucketCounts;
        double sum{0.0};
        uint64_t count{0};
    };
    
    std::unordered_map<std::string, Metric> counters_;
    std::unordered_map<std::string, Metric> gauges_;
    std::unordered_map<std::string, HistogramMetric> histograms_;
    std::mutex mutex_;
    
    std::string makeKey(const std::string& name, const std::map<std::string, std::string>& labels);
    std::string formatLabels(const std::map<std::string, std::string>& labels);
    
    std::atomic<uint64_t> totalRequests_{0};
    std::atomic<uint64_t> errorRequests_{0};
    std::chrono::steady_clock::time_point startTime_;
};

#define METRICS_HTTP_REQUEST(method, path, status, duration) \
    heartlake::utils::MetricsCollector::getInstance().recordHttpRequest(method, path, status, duration)

#define METRICS_DB_QUERY(op, duration, success) \
    heartlake::utils::MetricsCollector::getInstance().recordDbQuery(op, duration, success)

#define METRICS_AI_REQUEST(type, duration, success) \
    heartlake::utils::MetricsCollector::getInstance().recordAIRequest(type, duration, success)

#define METRICS_CACHE_HIT(type, hit) \
    heartlake::utils::MetricsCollector::getInstance().recordCacheHit(type, hit)

#define METRICS_ERROR(type, msg) \
    heartlake::utils::MetricsCollector::getInstance().recordError(type, msg)

} // namespace utils
} // namespace heartlake
