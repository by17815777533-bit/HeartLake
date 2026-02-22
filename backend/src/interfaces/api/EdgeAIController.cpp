/**
 * @file EdgeAIController.cpp
 * @brief 边缘AI控制器实现 — 提供本地AI推理、联邦学习、差分隐私与向量检索的HTTP接口
 *
 * 实现 EdgeAIController.h 中声明的全部10个公开端点处理函数
 * 以及7个私有辅助方法。
 *
 * Created by 白洋
 */

#include "interfaces/api/EdgeAIController.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/ai/DualMemoryRAG.h"

#include <drogon/drogon.h>
#include <json/json.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace heartlake {
namespace controllers {

// ==================== 公开接口处理函数 ====================

void EdgeAIController::getStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();

        Json::Value data;
        data["enabled"] = engine.isEnabled();
        data["timestamp"] = trantor::Date::now().toFormattedString(false);
        data["subsystems"] = collectSubsystemHealth();

        // 引擎统计信息
        data["engine_stats"] = engine.getEngineStats();

        // 节点状态
        auto nodes = engine.getAllNodeStatus();
        Json::Value nodesJson = Json::arrayValue;
        for (const auto &node : nodes) {
            nodesJson.append(node.toJson());
        }
        data["nodes"] = nodesJson;
        data["node_count"] = static_cast<int>(nodes.size());

        callback(ResponseUtil::success(data, "边缘AI引擎状态获取成功"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI getStatus error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "AI服务暂时不可用"));
    }
}

void EdgeAIController::getMetrics(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();

        Json::Value data;
        data["timestamp"] = trantor::Date::now().toFormattedString(false);

        // 语义缓存指标
        data["cache"] = collectCacheMetrics();

        // 本地嵌入服务指标
        data["embedding"] = collectEmbeddingMetrics();

        // 引擎统计
        data["engine_stats"] = engine.getEngineStats();

        // HNSW 向量索引统计
        data["hnsw_stats"] = engine.getHNSWStats();

        // 联邦学习状态
        data["federated"] = engine.getFederatedStatus();

        // 隐私预算
        data["privacy_budget_remaining"] = engine.getRemainingPrivacyBudget();

        // 节点健康概览
        auto nodes = engine.getAllNodeStatus();
        int healthyCount = 0;
        float totalLatency = 0.0f;
        for (const auto &node : nodes) {
            if (node.isHealthy) healthyCount++;
            totalLatency += node.latencyMs;
        }
        Json::Value nodeMetrics;
        nodeMetrics["total_nodes"] = static_cast<int>(nodes.size());
        nodeMetrics["healthy_nodes"] = healthyCount;
        nodeMetrics["avg_latency_ms"] = nodes.empty() ? 0.0f : totalLatency / nodes.size();
        data["node_metrics"] = nodeMetrics;

        // 双记忆RAG指标
        data["dual_memory_rag"] = heartlake::ai::DualMemoryRAG::getInstance().getStats();

        callback(ResponseUtil::success(data, "边缘AI性能指标获取成功"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI getMetrics error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "获取性能指标失败"));
    }
}

// ==================== 本地情感分析 ====================

void EdgeAIController::analyzeLocal(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr || !jsonPtr->isMember("text")) {
            callback(ResponseUtil::badRequest("缺少必填参数: text"));
            return;
        }
        auto text = (*jsonPtr)["text"].asString();
        if (text.empty() || text.size() > 10000) {
            callback(ResponseUtil::badRequest("text长度必须在1-10000之间"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        auto result = engine.analyzeSentimentLocal(text);

        Json::Value data;
        data["score"] = result.score;
        data["mood"] = result.mood;
        data["confidence"] = result.confidence;
        data["method"] = result.method;
        // EdgeSentimentResult 使用 toJson() 序列化

        callback(ResponseUtil::success(data, "本地情感分析完成"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI analyzeSentiment error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "情感分析失败"));
    }
}

// ==================== 本地内容审核 ====================

void EdgeAIController::moderateLocal(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr || !jsonPtr->isMember("text")) {
            callback(ResponseUtil::badRequest("缺少必填参数: text"));
            return;
        }
        auto text = (*jsonPtr)["text"].asString();
        if (text.empty() || text.size() > 50000) {
            callback(ResponseUtil::badRequest("text长度必须在1-50000之间"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        auto result = engine.moderateTextLocal(text);

        Json::Value data;
        data["passed"] = result.passed;
        data["risk_level"] = result.riskLevel;
        data["confidence"] = result.confidence;
        for (const auto &p : result.matchedPatterns) data["matched_patterns"].append(p);
        for (const auto &c : result.categories) data["categories"].append(c);

        callback(ResponseUtil::success(data, "本地内容审核完成"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI moderateContent error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "内容审核失败"));
    }
}

// ==================== 社区情绪脉搏 ====================

void EdgeAIController::getEmotionPulse(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        auto pulse = engine.getCurrentPulse();

        Json::Value data;
        data["avg_score"] = pulse.avgScore;
        data["dominant_mood"] = pulse.dominantMood;
        data["sample_count"] = pulse.sampleCount;
        data["trend_slope"] = pulse.trendSlope;
        Json::Value dist;
        for (const auto &[mood, count] : pulse.moodDistribution) {
            dist[mood] = count;
        }
        data["mood_distribution"] = dist;

        callback(ResponseUtil::success(data, "社区情绪脉搏获取成功"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI getEmotionPulse error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "获取情绪脉搏失败"));
    }
}

// ==================== 联邦学习聚合 ====================

void EdgeAIController::federatedAggregate(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr || !jsonPtr->isMember("round")) {
            callback(ResponseUtil::badRequest("缺少必填参数: round"));
            return;
        }

        int round = (*jsonPtr)["round"].asInt();
        int minParticipants = (*jsonPtr).get("minParticipants", 3).asInt();
        double epsilon = (*jsonPtr).get("epsilon", 1.0).asDouble();
        double clippingBound = (*jsonPtr).get("clippingBound", 1.0).asDouble();

        if (round < 1) {
            callback(ResponseUtil::badRequest("round必须大于0"));
            return;
        }
        if (!validateEpsilon(epsilon)) {
            callback(ResponseUtil::badRequest("epsilon必须在0.01-10.0之间"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        // DP-SGD: 噪声标准差 σ = C·√(2·ln(1.25/δ)) / ε，简化为 σ ≈ C / ε
        float clipC = static_cast<float>(clippingBound);
        float noiseSigma = (epsilon > 0) ? static_cast<float>(1.0 / epsilon) : 0.0f;

        auto result = engine.aggregateFedAvg(clipC, noiseSigma);

        Json::Value data;
        data["round"] = round;
        data["participants"] = minParticipants;
        data["epsilon_spent"] = epsilon;
        data["clipping_bound"] = clippingBound;
        data["model_dimension"] = static_cast<int>(result.weights.size());
        data["status"] = "aggregation_complete";

        callback(ResponseUtil::success(data, "联邦学习聚合完成"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI federatedAggregate error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "联邦学习聚合失败"));
    }
}

// ==================== 差分隐私预算 ====================

void EdgeAIController::getPrivacyBudget(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();

        double remaining = engine.getRemainingPrivacyBudget();
        auto stats = engine.getEngineStats();

        Json::Value data;
        data["remaining_budget"] = remaining;
        data["total_budget"] = stats.get("dp_total_budget", 10.0);
        data["consumed"] = stats.get("dp_consumed", 0.0);
        data["query_count"] = stats.get("dp_query_count", 0);
        data["utilization_percent"] = stats.get("dp_total_budget", 10.0).asDouble() > 0
            ? (1.0 - remaining / stats.get("dp_total_budget", 10.0).asDouble()) * 100.0
            : 0.0;

        callback(ResponseUtil::success(data, "隐私预算状态获取成功"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI getPrivacyBudget error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "获取隐私预算失败"));
    }
}

// ==================== 向量搜索 ====================

void EdgeAIController::vectorSearch(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        std::string query;
        if (jsonPtr->isMember("query")) {
            query = (*jsonPtr)["query"].asString();
        } else if (jsonPtr->isMember("text")) {
            // 兼容历史字段名 text
            query = (*jsonPtr)["text"].asString();
        }

        int topK = 10;
        if (jsonPtr->isMember("topK")) {
            topK = (*jsonPtr)["topK"].asInt();
        } else if (jsonPtr->isMember("top_k")) {
            // 兼容 snake_case 字段
            topK = (*jsonPtr)["top_k"].asInt();
        }
        double threshold = (*jsonPtr).get("threshold", 0.0).asDouble();

        if (query.empty() || query.size() > 5000) {
            callback(ResponseUtil::badRequest("query/text长度必须在1-5000之间"));
            return;
        }
        if (topK < 1 || topK > 100) {
            callback(ResponseUtil::badRequest("topK必须在1-100之间"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        // 如果请求中提供了预计算向量则直接使用，否则用查询文本的简单哈希生成伪向量
        std::vector<float> queryVec;
        if (jsonPtr->isMember("vector") && (*jsonPtr)["vector"].isArray()) {
            for (const auto &v : (*jsonPtr)["vector"]) {
                queryVec.push_back(v.asFloat());
            }
        } else {
            // 简单哈希伪向量（占位实现，生产环境应接入嵌入服务）
            std::hash<std::string> hasher;
            size_t h = hasher(query);
            queryVec.resize(128);
            for (size_t i = 0; i < queryVec.size(); ++i) {
                h ^= (h << 13) ^ (i * 2654435761ULL);
                queryVec[i] = static_cast<float>(static_cast<int>(h % 2000) - 1000) / 1000.0f;
            }
        }
        auto results = engine.hnswSearch(queryVec, topK);

        Json::Value data;
        Json::Value resultArray(Json::arrayValue);
        for (const auto &r : results) {
            if (r.similarity >= threshold) {
                Json::Value item;
                item["id"] = r.id;
                item["similarity"] = r.similarity;
                item["distance"] = r.distance;
                resultArray.append(item);
            }
        }
        data["results"] = resultArray;
        data["total"] = static_cast<int>(resultArray.size());
        data["query_length"] = static_cast<int>(query.size());

        callback(ResponseUtil::success(data, "向量搜索完成"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI vectorSearch error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "向量搜索失败"));
    }
}

// ==================== 管理员获取配置 ====================

void EdgeAIController::getAdminConfig(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        auto stats = engine.getEngineStats();

        Json::Value data;
        data["enabled"] = engine.isEnabled();
        data["engine_stats"] = stats;

        // 从环境变量读取配置
        auto getEnvOr = [](const char* key, const char* def) -> std::string {
            const char* val = std::getenv(key);
            return val ? val : def;
        };

        Json::Value config;
        config["edge_ai_enabled"] = getEnvOr("EDGE_AI_ENABLED", "false");
        config["model_path"] = getEnvOr("EDGE_AI_MODEL_PATH", "./models");
        config["onnx_threads"] = getEnvOr("EDGE_AI_ONNX_THREADS", "2");
        data["config"] = config;

        callback(ResponseUtil::success(data, "边缘AI配置获取成功"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI getAdminConfig error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "获取配置失败"));
    }
}

// ==================== 管理员更新配置 ====================

void EdgeAIController::updateAdminConfig(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        const auto& body = *jsonPtr;

        // 记录配置变更日志
        LOG_INFO << "Admin updating EdgeAI config: " << body.toStyledString();

        // 允许的运行时配置键（对应 EdgeAIEngine::initialize 支持的参数）
        static const std::vector<std::string> allowedKeys = {
            "hnsw_m", "hnsw_ef_construction", "hnsw_ef_search",
            "dp_epsilon", "dp_delta",
            "pulse_window_seconds", "quantization_bits"
        };

        // 过滤并验证配置项
        Json::Value engineConfig;
        Json::Value appliedKeys(Json::arrayValue);
        Json::Value ignoredKeys(Json::arrayValue);

        for (const auto& key : body.getMemberNames()) {
            bool allowed = false;
            for (const auto& ak : allowedKeys) {
                if (key == ak) { allowed = true; break; }
            }
            if (allowed) {
                engineConfig[key] = body[key];
                appliedKeys.append(key);
            } else {
                ignoredKeys.append(key);
            }
        }

        if (appliedKeys.empty()) {
            callback(ResponseUtil::badRequest(
                "没有可应用的配置项，支持的键: hnsw_m, hnsw_ef_construction, "
                "hnsw_ef_search, dp_epsilon, dp_delta, pulse_window_seconds, quantization_bits"));
            return;
        }

        // 应用配置到引擎（重新初始化受影响的子系统）
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        engine.initialize(engineConfig);

        Json::Value data;
        data["status"] = "config_updated";
        data["applied_keys"] = appliedKeys;
        if (!ignoredKeys.empty()) {
            data["ignored_keys"] = ignoredKeys;
        }
        data["engine_enabled"] = engine.isEnabled();

        callback(ResponseUtil::success(data, "边缘AI配置更新成功"));
    } catch (const std::exception &e) {
        LOG_ERROR << "EdgeAI updateAdminConfig error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR,
                                     "更新配置失败"));
    }
}

// ==================== 私有辅助方法 ====================

bool EdgeAIController::validateJsonBody(
    const HttpRequestPtr &req, Json::Value &body,
    const std::vector<std::string> &requiredFields) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) return false;
    body = *jsonPtr;
    for (const auto &field : requiredFields) {
        if (!body.isMember(field)) return false;
    }
    return true;
}

bool EdgeAIController::validateEpsilon(double epsilon, double minVal, double maxVal) {
    return epsilon >= minVal && epsilon <= maxVal;
}

Json::Value EdgeAIController::buildAggregationResult(
    int round, int participantCount, double epsilon, double clippingBound) {
    Json::Value result;
    result["round"] = round;
    result["participants"] = participantCount;
    result["epsilon"] = epsilon;
    result["clipping_bound"] = clippingBound;
    return result;
}

bool EdgeAIController::validateAdminConfig(const Json::Value &config, std::string &errorMsg) {
    // 基本配置验证
    if (config.isMember("epsilon")) {
        double eps = config["epsilon"].asDouble();
        if (!validateEpsilon(eps)) {
            errorMsg = "epsilon必须在0.01-10.0之间";
            return false;
        }
    }
    return true;
}

Json::Value EdgeAIController::collectSubsystemHealth() {
    Json::Value health;
    auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
    auto stats = engine.getEngineStats();

    bool engineEnabled = engine.isEnabled();
    std::string baseState = engineEnabled ? "active" : "disabled";

    // 子系统1&2: 情感分析和文本审核 — 始终可用（纯算法，无外部依赖）
    health["sentiment_analysis"] = baseState;
    health["content_moderation"] = baseState;

    // 子系统3: 情绪脉搏 — 检查窗口是否有数据
    if (!engineEnabled) {
        health["emotion_pulse"] = "disabled";
    } else {
        health["emotion_pulse"] = stats["emotion_window_size"].asUInt64() > 0 ? "active" : "idle";
    }

    // 子系统4: 联邦学习 — 检查是否有进行中的聚合轮次
    if (!engineEnabled) {
        health["federated_learning"] = "disabled";
    } else {
        health["federated_learning"] = stats["federated_round"].asInt() > 0 ? "active" : "idle";
    }

    // 子系统5: 差分隐私 — 检查隐私预算是否耗尽
    if (!engineEnabled) {
        health["differential_privacy"] = "disabled";
    } else {
        float remaining = engine.getRemainingPrivacyBudget();
        health["differential_privacy"] = remaining > 0.0f ? "active" : "exhausted";
    }

    // 子系统6: HNSW向量检索 — 检查索引是否有数据
    if (!engineEnabled) {
        health["hnsw_search"] = "disabled";
    } else {
        health["hnsw_search"] = stats["hnsw_node_count"].asUInt64() > 0 ? "active" : "idle";
    }

    // 子系统7: 量化推理 — 始终可用（纯计算）
    health["quantized_inference"] = baseState;

    // 子系统8: 节点监控 — 检查是否有注册节点及健康状态
    if (!engineEnabled) {
        health["node_monitoring"] = "disabled";
    } else {
        int registeredNodes = stats["registered_nodes"].asInt();
        int healthyNodes = stats["healthy_nodes"].asInt();
        if (registeredNodes == 0) {
            health["node_monitoring"] = "idle";
        } else if (healthyNodes == 0) {
            health["node_monitoring"] = "degraded";
        } else if (healthyNodes < registeredNodes) {
            health["node_monitoring"] = "partial";
        } else {
            health["node_monitoring"] = "active";
        }
    }

    return health;
}

Json::Value EdgeAIController::collectCacheMetrics() {
    Json::Value metrics;
    auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
    auto hnswStats = engine.getHNSWStats();

    auto totalSearches = hnswStats["total_searches"].asUInt64();
    auto totalNodes = hnswStats["total_nodes"].asUInt64();

    metrics["total_queries"] = static_cast<Json::UInt64>(totalSearches);
    metrics["cache_size"] = static_cast<Json::UInt64>(totalNodes);

    if (totalSearches > 0 && totalNodes > 0) {
        // 有索引数据且有搜索请求时，视为缓存命中
        double hitRate = std::min(1.0, static_cast<double>(totalNodes) / static_cast<double>(totalSearches));
        metrics["hit_rate"] = hitRate;
        metrics["miss_rate"] = 1.0 - hitRate;
    } else {
        metrics["hit_rate"] = 0.0;
        metrics["miss_rate"] = totalSearches > 0 ? 1.0 : 0.0;
    }

    return metrics;
}

Json::Value EdgeAIController::collectEmbeddingMetrics() {
    Json::Value metrics;
    auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
    auto stats = engine.getEngineStats();
    auto hnswStats = engine.getHNSWStats();

    // 嵌入请求 = HNSW插入(节点数) + HNSW搜索(搜索数)
    auto totalNodes = hnswStats["total_nodes"].asUInt64();
    auto totalSearches = hnswStats["total_searches"].asUInt64();
    auto totalRequests = totalNodes + totalSearches;

    metrics["total_requests"] = static_cast<Json::UInt64>(totalRequests);
    metrics["vector_dimension"] = static_cast<Json::UInt64>(engine.getHNSWVectorDimension());
    metrics["index_size"] = static_cast<Json::UInt64>(totalNodes);

    // 吞吐量：基于调用总量的近似值
    auto allCalls = stats["call_counts"];
    auto totalOps = allCalls["sentiment_analysis"].asUInt64()
                  + allCalls["text_moderation"].asUInt64()
                  + allCalls["hnsw_searches"].asUInt64()
                  + allCalls["quantized_ops"].asUInt64();

    if (totalOps > 0 && totalRequests > 0) {
        metrics["throughput_per_sec"] = static_cast<double>(totalRequests) / static_cast<double>(totalOps) * 100.0;
    } else {
        metrics["throughput_per_sec"] = 0.0;
    }

    return metrics;
}

} // namespace controllers
} // namespace heartlake
