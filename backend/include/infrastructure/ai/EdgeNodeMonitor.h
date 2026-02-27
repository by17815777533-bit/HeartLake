/**
 * 边缘节点健康监控与自适应负载均衡
 *
 * 从 EdgeAIEngine 提取的独立子系统，负责：
 * - 边缘节点注册与心跳管理
 * - 综合健康评分（CPU/内存/延迟/失败率加权）
 * - 熔断器状态机（CLOSED -> OPEN -> HALF_OPEN -> CLOSED）
 * - 自适应最优节点选择
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <chrono>
#include <json/json.h>
#include <drogon/drogon.h>

namespace heartlake {
namespace ai {

/**
 * 熔断器状态枚举 (经典三状态模式)
 *
 * CLOSED  -> 正常状态，请求正常通过，失败率超阈值时转 OPEN
 * OPEN    -> 熔断状态，拒绝请求，冷却时间后转 HALF_OPEN
 * HALF_OPEN -> 探测状态，允许少量探测请求，成功转 CLOSED，失败转回 OPEN
 */
enum class CircuitState {
    CLOSED,     ///< 正常 - 请求正常通过
    OPEN,       ///< 熔断 - 拒绝所有请求
    HALF_OPEN   ///< 半开 - 允许探测请求
};

/**
 * 边缘节点状态
 */
struct EdgeNodeStatus {
    std::string nodeId;                                ///< 节点标识
    float cpuUsage;                                    ///< CPU使用率 [0, 1]
    float memoryUsage;                                 ///< 内存使用率 [0, 1]
    float latencyMs;                                   ///< 平均延迟(ms)
    int activeConnections;                             ///< 活跃连接数
    int totalRequests;                                 ///< 总请求数
    int failedRequests;                                ///< 失败请求数
    bool isHealthy;                                    ///< 是否健康
    std::chrono::steady_clock::time_point lastHeartbeat; ///< 最后心跳时间
    float healthScore;                                 ///< 综合健康分 [0, 1]

    // 熔断器状态字段
    CircuitState circuitState = CircuitState::CLOSED;  ///< 熔断器当前状态
    std::chrono::steady_clock::time_point circuitOpenedAt; ///< 进入OPEN状态的时间
    int consecutiveFailures = 0;                       ///< 连续失败计数
    int halfOpenSuccesses = 0;                         ///< HALF_OPEN状态下连续成功数

    // 熔断器配置常量
    static constexpr float FAILURE_RATE_THRESHOLD = 0.5f;    ///< 失败率阈值
    static constexpr int MIN_REQUESTS_FOR_CIRCUIT = 5;       ///< 触发熔断的最小请求数
    static inline int COOLDOWN_SECONDS = 30;                 ///< OPEN->HALF_OPEN冷却时间(秒)
    static constexpr int HALF_OPEN_SUCCESS_THRESHOLD = 3;    ///< HALF_OPEN转CLOSED所需连续成功数

    Json::Value toJson() const {
        Json::Value j;
        j["node_id"] = nodeId;
        j["cpu_usage"] = cpuUsage;
        j["memory_usage"] = memoryUsage;
        j["latency_ms"] = latencyMs;
        j["active_connections"] = activeConnections;
        j["total_requests"] = totalRequests;
        j["failed_requests"] = failedRequests;
        j["is_healthy"] = isHealthy;
        j["health_score"] = healthScore;
        j["circuit_state"] = circuitState == CircuitState::CLOSED ? "CLOSED" :
                             circuitState == CircuitState::OPEN   ? "OPEN" : "HALF_OPEN";
        j["consecutive_failures"] = consecutiveFailures;
        return j;
    }
};

/**
 * 边缘节点健康监控器
 *
 * 线程安全。提供节点注册、心跳更新、健康评分、熔断器状态机、
 * 自适应最优节点选择等完整的边缘节点管理能力。
 */
class EdgeNodeMonitor {
public:
    /**
     * 注册边缘节点
     * @param nodeId 节点标识
     */
    void registerNode(const std::string& nodeId);

    /**
     * 更新节点状态（心跳）
     * @param status 节点状态
     */
    void updateNodeStatus(const EdgeNodeStatus& status);

    /**
     * 选择最优节点（自适应负载均衡）
     *
     * 综合评分公式：
     * score = w1*(1-cpu) + w2*(1-mem) + w3*(1-latency/maxLatency) + w4*(1-failRate)
     *
     * @return 最优节点ID，如果无可用节点返回空
     */
    std::optional<std::string> selectBestNode();

    /**
     * 获取所有节点状态
     * @return 节点状态列表（按健康分降序）
     */
    std::vector<EdgeNodeStatus> getAllNodeStatus() const;

    /**
     * 获取节点监控统计信息
     * @return JSON格式统计
     */
    Json::Value getNodeMonitorStats() const;

    /** 清空所有节点注册信息 */
    void clear();

private:
    std::unordered_map<std::string, EdgeNodeStatus> nodeRegistry_; ///< 节点注册表
    mutable std::shared_mutex nodeMutex_;

    /**
     * 计算节点综合健康分
     *
     * score = w1*(1-cpu) + w2*(1-mem) + w3*(1-latency/maxLatency) + w4*(1-failRate)
     * 心跳超时30秒后指数衰减惩罚
     *
     * @param status 节点状态
     * @return 健康分 [0, 1]
     */
    float computeHealthScore(const EdgeNodeStatus& status) const;
};

} // namespace ai
} // namespace heartlake
