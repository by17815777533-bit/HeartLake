/**
 * 边缘节点健康监控与自适应负载均衡 - 实现
 *
 * 从 EdgeAIEngine 提取的独立子系统实现。
 */

#include "infrastructure/ai/EdgeNodeMonitor.h"
#include <cmath>
#include <algorithm>
#include <drogon/drogon.h>

namespace heartlake {
namespace ai {

float EdgeNodeMonitor::computeHealthScore(const EdgeNodeStatus& status) const {
    // 综合评分公式：
    // score = w1*(1-cpu) + w2*(1-mem) + w3*(1-latency/maxLatency) + w4*(1-failRate)
    constexpr float w1 = 0.30f;  // CPU权重
    constexpr float w2 = 0.25f;  // 内存权重
    constexpr float w3 = 0.25f;  // 延迟权重
    constexpr float w4 = 0.20f;  // 失败率权重
    constexpr float maxLatencyMs = 1000.0f;  // 最大可接受延迟

    float cpuScore = 1.0f - std::clamp(status.cpuUsage, 0.0f, 1.0f);
    float memScore = 1.0f - std::clamp(status.memoryUsage, 0.0f, 1.0f);
    float latencyScore = 1.0f - std::clamp(status.latencyMs / maxLatencyMs, 0.0f, 1.0f);

    float failRate = 0.0f;
    if (status.totalRequests > 0) {
        failRate = static_cast<float>(status.failedRequests) /
                   static_cast<float>(status.totalRequests);
    }
    float failScore = 1.0f - std::clamp(failRate, 0.0f, 1.0f);

    float score = w1 * cpuScore + w2 * memScore + w3 * latencyScore + w4 * failScore;

    // 心跳超时惩罚：超过30秒未心跳，分数指数衰减
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - status.lastHeartbeat).count();
    if (elapsed > 30) {
        float decay = std::exp(-0.05f * static_cast<float>(elapsed - 30));
        score *= decay;
    }

    return std::clamp(score, 0.0f, 1.0f);
}

void EdgeNodeMonitor::registerNode(const std::string& nodeId) {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);

    if (nodeRegistry_.count(nodeId)) {
        LOG_WARN << "[EdgeNodeMonitor] Node '" << nodeId << "' already registered";
        return;
    }

    EdgeNodeStatus status;
    status.nodeId = nodeId;
    status.cpuUsage = 0.0f;
    status.memoryUsage = 0.0f;
    status.latencyMs = 0.0f;
    status.activeConnections = 0;
    status.totalRequests = 0;
    status.failedRequests = 0;
    status.isHealthy = true;
    status.lastHeartbeat = std::chrono::steady_clock::now();
    status.healthScore = 1.0f;
    status.circuitState = CircuitState::CLOSED;
    status.circuitOpenedAt = std::chrono::steady_clock::time_point{};
    status.consecutiveFailures = 0;
    status.halfOpenSuccesses = 0;

    nodeRegistry_[nodeId] = std::move(status);
    LOG_INFO << "[EdgeNodeMonitor] Node registered: " << nodeId;
}

void EdgeNodeMonitor::updateNodeStatus(const EdgeNodeStatus& status) {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);

    auto it = nodeRegistry_.find(status.nodeId);
    if (it == nodeRegistry_.end()) {
        LOG_WARN << "[EdgeNodeMonitor] Unknown node: " << status.nodeId << ", auto-registering";
        nodeRegistry_[status.nodeId] = status;
        nodeRegistry_[status.nodeId].lastHeartbeat = std::chrono::steady_clock::now();
        nodeRegistry_[status.nodeId].healthScore = computeHealthScore(status);
        return;
    }

    // 更新状态
    it->second.cpuUsage = status.cpuUsage;
    it->second.memoryUsage = status.memoryUsage;
    it->second.latencyMs = status.latencyMs;
    it->second.activeConnections = status.activeConnections;
    it->second.totalRequests = status.totalRequests;
    it->second.failedRequests = status.failedRequests;
    it->second.lastHeartbeat = std::chrono::steady_clock::now();

    // 重新计算健康分
    it->second.healthScore = computeHealthScore(it->second);

    // 熔断器状态机转换
    auto& node = it->second;
    auto now = std::chrono::steady_clock::now();
    float failRate = node.totalRequests > 0
        ? static_cast<float>(node.failedRequests) / static_cast<float>(node.totalRequests)
        : 0.0f;

    switch (node.circuitState) {
        case CircuitState::CLOSED: {
            // CLOSED: 失败率超阈值且请求数足够 -> 转 OPEN
            if (node.totalRequests >= EdgeNodeStatus::MIN_REQUESTS_FOR_CIRCUIT &&
                failRate >= EdgeNodeStatus::FAILURE_RATE_THRESHOLD) {
                node.circuitState = CircuitState::OPEN;
                node.circuitOpenedAt = now;
                node.isHealthy = false;
                LOG_WARN << "[EdgeNodeMonitor] Circuit OPEN for node '" << node.nodeId
                         << "', failRate=" << failRate;
            } else {
                // 正常健康判定
                node.isHealthy = (node.healthScore >= 0.3f);
            }
            break;
        }
        case CircuitState::OPEN: {
            // OPEN: 冷却时间到 -> 转 HALF_OPEN
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - node.circuitOpenedAt).count();
            if (elapsed >= EdgeNodeStatus::COOLDOWN_SECONDS) {
                node.circuitState = CircuitState::HALF_OPEN;
                node.halfOpenSuccesses = 0;
                node.consecutiveFailures = 0;
                LOG_INFO << "[EdgeNodeMonitor] Circuit HALF_OPEN for node '" << node.nodeId
                         << "' after " << elapsed << "s cooldown";
            }
            // OPEN 状态下保持 isHealthy = false
            node.isHealthy = false;
            break;
        }
        case CircuitState::HALF_OPEN: {
            // HALF_OPEN: 根据最新失败率判断
            if (failRate < EdgeNodeStatus::FAILURE_RATE_THRESHOLD) {
                node.halfOpenSuccesses++;
                if (node.halfOpenSuccesses >= EdgeNodeStatus::HALF_OPEN_SUCCESS_THRESHOLD) {
                    // 探测成功足够次数 -> 转 CLOSED
                    node.circuitState = CircuitState::CLOSED;
                    node.consecutiveFailures = 0;
                    node.isHealthy = (node.healthScore >= 0.3f);
                    LOG_INFO << "[EdgeNodeMonitor] Circuit CLOSED for node '" << node.nodeId
                             << "', recovered";
                } else {
                    node.isHealthy = true;  // 允许探测请求
                }
            } else {
                // 探测失败 -> 转回 OPEN
                node.circuitState = CircuitState::OPEN;
                node.circuitOpenedAt = now;
                node.halfOpenSuccesses = 0;
                node.isHealthy = false;
                LOG_WARN << "[EdgeNodeMonitor] Circuit re-OPEN for node '" << node.nodeId
                         << "', probe failed, failRate=" << failRate;
            }
            break;
        }
    }

    if (!node.isHealthy) {
        LOG_WARN << "[EdgeNodeMonitor] Node '" << status.nodeId
                 << "' marked unhealthy, score=" << node.healthScore
                 << ", circuit=" << (node.circuitState == CircuitState::CLOSED ? "CLOSED" :
                                     node.circuitState == CircuitState::OPEN   ? "OPEN" : "HALF_OPEN");
    }
}

std::optional<std::string> EdgeNodeMonitor::selectBestNode() {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);

    if (nodeRegistry_.empty()) return std::nullopt;

    std::string bestId;
    float bestScore = -1.0f;

    auto now = std::chrono::steady_clock::now();

    for (auto& [nodeId, status] : nodeRegistry_) {
        // 跳过 OPEN 熔断状态节点（完全隔离）
        if (status.circuitState == CircuitState::OPEN) continue;

        // 跳过不健康节点
        if (!status.isHealthy) continue;

        // 跳过心跳超时节点（超过60秒）
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - status.lastHeartbeat).count();
        if (elapsed > 60) {
            status.isHealthy = false;
            continue;
        }

        // HALF_OPEN 节点降低优先级（乘以0.5权重）
        float score = computeHealthScore(status);
        status.healthScore = score;
        if (status.circuitState == CircuitState::HALF_OPEN) {
            score *= 0.5f;
        }

        if (score > bestScore) {
            bestScore = score;
            bestId = nodeId;
        }
    }

    if (bestId.empty()) return std::nullopt;
    return bestId;
}

std::vector<EdgeNodeStatus> EdgeNodeMonitor::getAllNodeStatus() const {
    std::shared_lock<std::shared_mutex> lock(nodeMutex_);

    std::vector<EdgeNodeStatus> result;
    result.reserve(nodeRegistry_.size());
    for (const auto& [id, status] : nodeRegistry_) {
        result.push_back(status);
    }

    // 按健康分降序排列
    std::sort(result.begin(), result.end(),
              [](const EdgeNodeStatus& a, const EdgeNodeStatus& b) {
                  return a.healthScore > b.healthScore;
              });

    return result;
}

Json::Value EdgeNodeMonitor::getNodeMonitorStats() const {
    std::shared_lock<std::shared_mutex> lock(nodeMutex_);

    Json::Value stats;
    stats["registered_nodes"] = static_cast<Json::UInt64>(nodeRegistry_.size());
    int healthyCount = 0;
    for (const auto& [id, status] : nodeRegistry_) {
        if (status.isHealthy) ++healthyCount;
    }
    stats["healthy_nodes"] = healthyCount;

    return stats;
}

void EdgeNodeMonitor::clear() {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);
    nodeRegistry_.clear();
}

} // namespace ai
} // namespace heartlake
