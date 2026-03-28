/**
 * 边缘AI控制器实现 — 提供本地AI推理、联邦学习、差分隐私与向量检索的HTTP接口
 *
 * 实现 EdgeAIController.h 中声明的全部10个公开端点处理函数
 * 以及7个私有辅助方法。
 *
 */

#include "interfaces/api/EdgeAIController.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/ai/DualMemoryRAG.h"
#include "infrastructure/ai/SummaryService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "utils/AdminConfigStore.h"
#include "utils/EnvUtils.h"
#include "utils/RequestHelper.h"
#include "utils/Validator.h"

#include <drogon/drogon.h>
#include <json/json.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace heartlake {
namespace controllers {

namespace {

Json::Value buildStaticCollectionPayload(const std::string &primaryKey,
                                         const Json::Value &items) {
    const auto total = static_cast<int>(items.size());
    return ResponseUtil::buildCollectionPayload(primaryKey, items, total, 1,
                                                std::max(1, total));
}

std::string trimAscii(const std::string& input) {
    size_t start = 0;
    while (start < input.size() &&
           std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    size_t end = input.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

std::string normalizeMoodInput(std::string moodRaw) {
    std::string mood = trimAscii(moodRaw);
    std::transform(mood.begin(), mood.end(), mood.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    static const std::unordered_map<std::string, std::string> aliases = {
        {"joy", "happy"},
        {"happiness", "happy"},
        {"excited", "happy"},
        {"sadness", "sad"},
        {"depressed", "sad"},
        {"fear", "anxious"},
        {"fearful", "anxious"},
        {"stressed", "anxious"},
        {"worried", "anxious"},
        {"anger", "angry"},
        {"peaceful", "calm"},
        {"gratitude", "grateful"},
        {"uncertain", "confused"}
    };
    auto it = aliases.find(mood);
    if (it != aliases.end()) {
        mood = it->second;
    }

    static const std::unordered_set<std::string> validMoods = {
        "happy", "calm", "neutral", "anxious", "sad", "angry",
        "surprised", "confused", "hopeful", "grateful", "lonely"
    };
    if (validMoods.find(mood) == validMoods.end()) {
        return "neutral";
    }
    return mood;
}

float clampUnit(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

bool parseJsonBoolValue(const Json::Value& value, bool fallback) {
    if (value.isBool()) {
        return value.asBool();
    }
    if (value.isIntegral()) {
        return value.asInt() != 0;
    }
    if (value.isString()) {
        return heartlake::utils::parseBoolEnv(value.asCString(), fallback);
    }
    return fallback;
}

int parseJsonIntValue(const Json::Value& value, int fallback) {
    if (value.isInt()) {
        return value.asInt();
    }
    if (value.isUInt()) {
        return static_cast<int>(value.asUInt());
    }
    if (value.isString()) {
        try {
            return std::stoi(value.asString());
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

double parseJsonDoubleValue(const Json::Value& value, double fallback) {
    if (value.isDouble() || value.isInt() || value.isUInt()) {
        return value.asDouble();
    }
    if (value.isString()) {
        try {
            return std::stod(value.asString());
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

std::string parseJsonStringValue(const Json::Value& value,
                                 const std::string& fallback = "") {
    if (value.isString()) {
        const auto normalized = trimAscii(value.asString());
        return normalized.empty() ? fallback : normalized;
    }
    if (value.isBool()) {
        return value.asBool() ? "true" : "false";
    }
    return fallback;
}

void setMirroredConfig(Json::Value& config,
                       const std::string& snakeKey,
                       const std::string& camelKey,
                       const Json::Value& value) {
    config[snakeKey] = value;
    config[camelKey] = value;
}

Json::Value buildAdminEdgeAIConfigPayload(const Json::Value& aiConfig,
                                          const Json::Value& engineStats,
                                          bool engineEnabled) {
    const bool edgeAiEnabled = aiConfig.isMember("edge_ai_enabled")
        ? parseJsonBoolValue(aiConfig["edge_ai_enabled"],
                             heartlake::utils::parseBoolEnv(
                                 std::getenv("EDGE_AI_ENABLED"), true))
        : heartlake::utils::parseBoolEnv(std::getenv("EDGE_AI_ENABLED"), true);
    const std::string onnxEnabled = aiConfig.isMember("onnx_enabled")
        ? parseJsonStringValue(aiConfig["onnx_enabled"], "auto")
        : parseJsonStringValue(Json::Value(
              std::getenv("EDGE_AI_ONNX_ENABLED")
                  ? std::getenv("EDGE_AI_ONNX_ENABLED")
                  : "auto"),
              "auto");

    Json::Value config;
    setMirroredConfig(config, "edge_ai_enabled", "edgeAIEnabled", edgeAiEnabled);
    config["enabled"] = edgeAiEnabled;
    setMirroredConfig(config, "onnx_enabled", "onnxEnabled", onnxEnabled);
    config["inferenceEngine"] =
        onnxEnabled == "false" ? "builtin" : "onnx";
    setMirroredConfig(
        config,
        "model_path",
        "modelPath",
        aiConfig.get("model_path",
                     std::getenv("EDGE_AI_MODEL_PATH")
                         ? std::getenv("EDGE_AI_MODEL_PATH")
                         : "./models/sentiment_zh.onnx"));
    setMirroredConfig(
        config,
        "vocab_path",
        "vocabPath",
        aiConfig.get("vocab_path",
                     std::getenv("EDGE_AI_VOCAB_PATH")
                         ? std::getenv("EDGE_AI_VOCAB_PATH")
                         : ""));
    setMirroredConfig(config, "onnx_threads", "onnxThreads",
                      aiConfig.get("onnx_threads", 2));
    setMirroredConfig(config, "hnsw_m", "hnswM",
                      aiConfig.get("hnsw_m", engineStats.get("hnsw_m", 16)));
    setMirroredConfig(config, "hnsw_ef_construction", "hnswEfConstruction",
                      aiConfig.get("hnsw_ef_construction",
                                   engineStats.get("hnsw_ef_construction", 200)));
    setMirroredConfig(config, "hnsw_ef_search", "hnswEfSearch",
                      aiConfig.get("hnsw_ef_search",
                                   engineStats.get("hnsw_ef_search", 50)));
    setMirroredConfig(config, "dp_epsilon", "dpEpsilon",
                      aiConfig.get("dp_epsilon",
                                   engineStats.get("dp_epsilon", 1.0)));
    setMirroredConfig(config, "dp_delta", "dpDelta",
                      aiConfig.get("dp_delta",
                                   engineStats.get("dp_delta", 1e-5)));
    setMirroredConfig(
        config,
        "pulse_window_seconds",
        "pulseWindowSeconds",
        aiConfig.get("pulse_window_seconds",
                     engineStats.get("pulse_window_seconds", 300)));
    setMirroredConfig(
        config,
        "sentiment_cache_ttl",
        "cacheTTL",
        aiConfig.get("sentiment_cache_ttl",
                     engineStats.get("sentiment_cache_ttl_seconds", 300)));
    setMirroredConfig(
        config,
        "sentiment_cache_max",
        "sentimentCacheMax",
        aiConfig.get("sentiment_cache_max",
                     engineStats.get("sentiment_cache_max_entries",
                                     Json::UInt64(4096))));
    config["privacyEpsilon"] = config["dp_epsilon"];
    config["maxEpsilon"] = config["dp_epsilon"];
    setMirroredConfig(
        config,
        "cache_strategy",
        "cacheStrategy",
        aiConfig.get("cacheStrategy", aiConfig.get("cache_strategy", "lru")));
    setMirroredConfig(
        config,
        "max_batch_size",
        "maxBatchSize",
        aiConfig.get("maxBatchSize", aiConfig.get("max_batch_size", 32)));
    setMirroredConfig(
        config,
        "cache_size_mb",
        "cacheSizeMB",
        aiConfig.get("cacheSizeMB", aiConfig.get("cache_size_mb", 256)));
    setMirroredConfig(
        config,
        "federated_interval",
        "federatedInterval",
        aiConfig.get("federatedInterval", aiConfig.get("federated_interval", 300)));
    setMirroredConfig(
        config,
        "emotion_model",
        "emotionModel",
        aiConfig.get("emotionModel", aiConfig.get("emotion_model", "standard")));
    setMirroredConfig(
        config,
        "vector_search_enabled",
        "vectorSearchEnabled",
        aiConfig.get("vectorSearchEnabled",
                     aiConfig.get("vector_search_enabled", edgeAiEnabled)));
    setMirroredConfig(
        config,
        "cache_enabled",
        "cacheEnabled",
        aiConfig.get("cacheEnabled", aiConfig.get("cache_enabled", true)));
    setMirroredConfig(
        config,
        "federated_enabled",
        "federatedEnabled",
        aiConfig.get("federatedEnabled",
                     aiConfig.get("federated_enabled", edgeAiEnabled)));
    setMirroredConfig(
        config,
        "inference_timeout",
        "inferenceTimeout",
        aiConfig.get("inferenceTimeout", aiConfig.get("inference_timeout", 5000)));
    config["quantization_bits"] = aiConfig.get("quantization_bits", 8);
    config["engine_enabled"] = engineEnabled;
    return config;
}

int countLoadedSubsystems(const Json::Value& subsystems) {
    int loaded = 0;
    for (const auto& key : subsystems.getMemberNames()) {
        if (subsystems[key].asString() != "disabled") {
            ++loaded;
        }
    }
    return loaded;
}

double resolveCacheHitRatePercent(const Json::Value& cacheMetrics) {
    double hitRate = cacheMetrics.get("hit_rate", 0.0).asDouble();
    if (cacheMetrics.isMember("semantic_cache")) {
        hitRate = cacheMetrics["semantic_cache"].get("hit_rate", hitRate).asDouble();
    }
    if (hitRate <= 1.0) {
        hitRate *= 100.0;
    }
    return std::clamp(hitRate, 0.0, 100.0);
}

std::string joinStrings(const std::vector<std::string>& values,
                        const std::string& delimiter) {
    std::string joined;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            joined += delimiter;
        }
        joined += values[i];
    }
    return joined;
}

std::string buildPgTextArrayLiteral(const std::vector<std::string>& values) {
    std::string literal = "{";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            literal += ",";
        }
        literal += "\"";
        for (const char ch : values[i]) {
            if (ch == '\\' || ch == '"') {
                literal += '\\';
            }
            literal += ch;
        }
        literal += "\"";
    }
    literal += "}";
    return literal;
}

struct StoneVectorMetadata {
    std::string content;
    std::string moodType;
    std::string createdAt;
};

struct ConfidenceCalibration {
    float calibratedConfidence{0.0f};
    float uncertainty{1.0f};
    bool abstained{false};
    std::string reliabilityTier{"low"};
};

ConfidenceCalibration calibrateConfidence(float score, float confidence, size_t textLength) {
    const float margin = std::min(1.0f, std::abs(score));
    const float lengthFactor = textLength < 6 ? 0.82f : (textLength < 12 ? 0.90f : 1.0f);
    const float neutralPenalty = std::max(0.0f, 0.18f - margin) * 1.8f;
    const float calibrated = clampUnit(confidence * (0.60f + 0.40f * margin) * lengthFactor - neutralPenalty);
    const float uncertainty = clampUnit(1.0f - calibrated + std::max(0.0f, 0.16f - margin) * 1.2f);
    const bool abstained = calibrated < 0.42f || (margin < 0.12f && calibrated < 0.62f);

    std::string tier = "low";
    if (calibrated >= 0.78f) {
        tier = "high";
    } else if (calibrated >= 0.55f) {
        tier = "medium";
    }

    return {calibrated, uncertainty, abstained, tier};
}

}  // namespace

// ==================== 公开接口处理函数 ====================

void EdgeAIController::getStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getStatus called";
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        const auto subsystems = collectSubsystemHealth();
        const auto engineStats = engine.getEngineStats();
        const auto cacheMetrics = collectCacheMetrics();
        const auto embeddingMetrics = collectEmbeddingMetrics();

        Json::Value data;
        data["enabled"] = engine.isEnabled();
        data["timestamp"] = trantor::Date::now().toFormattedString(false);
        data["subsystems"] = subsystems;

        // 引擎统计信息
        data["engine_stats"] = engineStats;
        data["cache"] = cacheMetrics;
        data["embedding"] = embeddingMetrics;

        // 节点状态
        auto nodes = engine.getAllNodeStatus();
        Json::Value nodesJson = Json::arrayValue;
        float totalLatency = 0.0f;
        for (const auto &node : nodes) {
            nodesJson.append(node.toJson());
            totalLatency += node.latencyMs;
        }
        data["nodes"] = nodesJson;
        data["node_count"] = static_cast<int>(nodes.size());
        const auto nodeCount = static_cast<int>(nodes.size());
        const auto totalModules = static_cast<int>(subsystems.getMemberNames().size());
        const auto modulesLoaded = countLoadedSubsystems(subsystems);
        const auto avgLatency = nodes.empty() ? 0.0 : totalLatency / nodes.size();
        const auto cacheHitRate = resolveCacheHitRatePercent(cacheMetrics);

        setMirroredConfig(data, "modules_loaded", "modulesLoaded", modulesLoaded);
        setMirroredConfig(data, "total_modules", "totalModules", totalModules);
        setMirroredConfig(data, "active_nodes", "activeNodes", nodeCount);
        setMirroredConfig(data, "cache_hit_rate", "cacheHitRate", cacheHitRate);
        setMirroredConfig(data, "avg_latency", "avgLatency", avgLatency);
        setMirroredConfig(data, "inference_count", "inferenceCount",
                          engineStats.get("total_sentiment_calls", 0u));
        data["throughput"] = embeddingMetrics.get("throughput_per_sec", 0.0);
        data["throughput_estimated"] =
            embeddingMetrics.get("throughput_estimated", true);

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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getMetrics called";
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
        const auto subsystems = collectSubsystemHealth();
        setMirroredConfig(data, "active_nodes", "activeNodes",
                          static_cast<int>(nodes.size()));
        setMirroredConfig(data, "avg_latency", "avgLatency",
                          nodeMetrics["avg_latency_ms"]);
        data["avg_latency_ms"] = nodeMetrics["avg_latency_ms"];
        setMirroredConfig(data, "cache_hit_rate", "cacheHitRate",
                          resolveCacheHitRatePercent(data["cache"]));
        setMirroredConfig(data, "inference_count", "inferenceCount",
                          data["engine_stats"].get("total_sentiment_calls", 0u));
        setMirroredConfig(data, "modules_loaded", "modulesLoaded",
                          countLoadedSubsystems(subsystems));
        setMirroredConfig(data, "total_modules", "totalModules",
                          static_cast<int>(subsystems.getMemberNames().size()));
        data["throughput"] = data["embedding"].get("throughput_per_sec", 0.0);
        data["throughput_estimated"] =
            data["embedding"].get("throughput_estimated", true);

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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_DEBUG << "[trace:" << traceId << "] analyzeLocal called";
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

        bool preferOnnx = false;
        if (jsonPtr->isMember("prefer_onnx")) {
            const auto& rawPreferOnnx = (*jsonPtr)["prefer_onnx"];
            if (rawPreferOnnx.isBool()) {
                preferOnnx = rawPreferOnnx.asBool();
            } else if (rawPreferOnnx.isInt() || rawPreferOnnx.isUInt()) {
                preferOnnx = rawPreferOnnx.asInt() != 0;
            } else if (rawPreferOnnx.isString()) {
                std::string normalized = rawPreferOnnx.asString();
                std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                preferOnnx = (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes");
            }
        }
        if (!preferOnnx && jsonPtr->isMember("analysis_mode")) {
            std::string mode = (*jsonPtr)["analysis_mode"].asString();
            std::transform(mode.begin(), mode.end(), mode.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            preferOnnx = (mode == "onnx" || mode == "onnx_preferred" || mode == "high_accuracy");
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        auto result = engine.analyzeSentimentLocal(text, preferOnnx);

        Json::Value data;
        const auto calibrated = calibrateConfidence(result.score, result.confidence, text.size());
        data["score"] = result.score;
        data["mood"] = result.mood;
        data["sentiment"] = result.mood;
        data["emotion"] = result.mood;
        data["sentiment_score"] = result.score;
        data["confidence"] = calibrated.calibratedConfidence;
        data["calibrated_confidence"] = calibrated.calibratedConfidence;
        data["raw_confidence"] = result.confidence;
        data["confidence_gap"] = std::max(0.0f, result.confidence - calibrated.calibratedConfidence);
        data["signal_strength"] = std::abs(result.score);
        data["uncertainty"] = calibrated.uncertainty;
        data["abstained"] = calibrated.abstained;
        data["reliability_tier"] = calibrated.reliabilityTier;
        data["decision"] = calibrated.abstained ? "abstain" : "accept";
        data["recommended_action"] = calibrated.abstained ? "ask_for_more_context" : "use_as_reference";
        data["method"] = result.method;
        data["prefer_onnx"] = preferOnnx;
        data["analysis_mode"] = "multi_expert_fusion_v2";
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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] moderateLocal called";
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
        data["pass"] = result.passed;
        data["risk_level"] = result.riskLevel;
        data["risk"] = result.riskLevel;
        data["confidence"] = result.confidence;
        for (const auto &p : result.matchedPatterns) data["matched_patterns"].append(p);
        for (const auto &c : result.categories) data["categories"].append(c);
        std::vector<std::string> reasonParts;
        if (!result.categories.empty()) {
            reasonParts.push_back("分类: " + joinStrings(result.categories, " / "));
        }
        if (!result.matchedPatterns.empty()) {
            reasonParts.push_back("命中: " + joinStrings(result.matchedPatterns, " / "));
        }
        const auto reason = joinStrings(reasonParts, "; ");
        data["reason"] = reason;
        data["reject_reason"] = reason;

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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getEmotionPulse called";
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        auto pulse = engine.getCurrentPulse();

        Json::Value data;
        data["avg_score"] = pulse.avgScore;
        const auto normalizedScore =
            std::clamp((pulse.avgScore + 1.0f) / 2.0f, 0.0f, 1.0f);
        int topMoodCount = 0;
        for (const auto& [_, count] : pulse.moodDistribution) {
            topMoodCount = std::max(topMoodCount, count);
        }
        data["normalized_score"] = normalizedScore;
        data["temperature"] =
            static_cast<int>(std::round(normalizedScore * 100.0));
        data["value"] = data["temperature"];
        data["dominant_mood"] = pulse.dominantMood;
        data["top_mood"] = pulse.dominantMood;
        data["sample_count"] = pulse.sampleCount;
        data["today_matches"] = pulse.sampleCount;
        data["top_mood_share_percent"] = pulse.sampleCount > 0
            ? (static_cast<double>(topMoodCount) /
               static_cast<double>(pulse.sampleCount)) * 100.0
            : 0.0;
        data["trend_slope"] = pulse.trendSlope;
        if (pulse.trendSlope > 0.01f) {
            data["trend"] = "rising";
        } else if (pulse.trendSlope < -0.01f) {
            data["trend"] = "falling";
        } else {
            data["trend"] = "stable";
        }
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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] federatedAggregate called";
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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getPrivacyBudget called";
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();

        double remaining = engine.getRemainingPrivacyBudget();
        auto stats = engine.getEngineStats();

        Json::Value data;
        const auto totalBudget = stats.get("dp_total_budget", 10.0);
        const auto consumed = stats.get("dp_consumed", 0.0);
        const auto utilizationPercent = totalBudget.asDouble() > 0
            ? (1.0 - remaining / totalBudget.asDouble()) * 100.0
            : 0.0;
        data["remaining_budget"] = remaining;
        data["total_budget"] = totalBudget;
        data["consumed"] = consumed;
        data["query_count"] = stats.get("dp_query_count", 0);
        data["utilization_percent"] = utilizationPercent;
        setMirroredConfig(data, "epsilon_total", "epsilonTotal", totalBudget);
        setMirroredConfig(data, "epsilon_used", "epsilonUsed", consumed);
        setMirroredConfig(data, "epsilon_remaining", "epsilonRemaining", remaining);
        setMirroredConfig(data, "epsilon_percent", "epsilonPercent",
                          utilizationPercent);
        setMirroredConfig(data, "delta_value", "deltaValue",
                          stats.get("dp_delta", 1e-5));

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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] vectorSearch called";
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

        if (query.empty() || query.size() > 10000) {
            callback(ResponseUtil::badRequest("query长度必须在1-10000之间"));
            return;
        }

        int topK = 10;
        if (jsonPtr->isMember("topK")) {
            topK = (*jsonPtr)["topK"].asInt();
        } else if (jsonPtr->isMember("top_k")) {
            // 兼容 snake_case 字段
            topK = (*jsonPtr)["top_k"].asInt();
        }
        double threshold = (*jsonPtr).get("threshold", 0.0).asDouble();

        if (topK < 1 || topK > 100) {
            callback(ResponseUtil::badRequest("topK必须在1-100之间"));
            return;
        }

        int candidateMultiplier = (*jsonPtr).get("candidate_multiplier", 4).asInt();
        candidateMultiplier = std::clamp(candidateMultiplier, 1, 8);

        bool enableSecondStage = true;
        if (jsonPtr->isMember("enable_second_stage")) {
            const auto& v = (*jsonPtr)["enable_second_stage"];
            if (v.isBool()) {
                enableSecondStage = v.asBool();
            } else if (v.isInt()) {
                enableSecondStage = v.asInt() != 0;
            } else if (v.isString()) {
                auto flag = trimAscii(v.asString());
                std::transform(flag.begin(), flag.end(), flag.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                enableSecondStage = !(flag == "0" || flag == "false" || flag == "off");
            }
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        // 如果请求中提供了预计算向量则直接使用，否则使用真实 embedding 生成查询向量。
        std::vector<float> queryVec;
        if (jsonPtr->isMember("vector") && (*jsonPtr)["vector"].isArray()) {
            for (const auto &v : (*jsonPtr)["vector"]) {
                queryVec.push_back(v.asFloat());
            }
        }
        if (queryVec.empty()) {
            if (query.empty() || query.size() > 5000) {
                callback(ResponseUtil::badRequest("query/text长度必须在1-5000之间，或提供vector"));
                return;
            }
            queryVec = heartlake::ai::AdvancedEmbeddingEngine::getInstance().generateEmbedding(query);
            if (queryVec.empty()) {
                callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "查询向量生成失败"));
                return;
            }
        } else if (!query.empty() && query.size() > 5000) {
            callback(ResponseUtil::badRequest("query/text长度必须在1-5000之间"));
            return;
        }

        const size_t hnswDim = engine.getHNSWVectorDimension();
        if (hnswDim > 0 && queryVec.size() != hnswDim) {
            callback(ResponseUtil::badRequest("查询向量维度不匹配"));
            return;
        }

        const int candidateK = std::min(400, std::max(topK, topK * candidateMultiplier));
        auto results = engine.hnswSearch(queryVec, enableSecondStage ? candidateK : topK);
        if (enableSecondStage && !results.empty()) {
            results = engine.rerankHNSWCandidates(queryVec, results, topK);
        } else if (static_cast<int>(results.size()) > topK) {
            results.resize(static_cast<size_t>(topK));
        }
        const int returnedCandidates = static_cast<int>(results.size());
        std::vector<std::string> matchedStoneIds;
        matchedStoneIds.reserve(results.size());
        for (const auto &r : results) {
            if (r.similarity >= threshold && !r.id.empty()) {
                matchedStoneIds.push_back(r.id);
            }
        }

        std::unordered_map<std::string, StoneVectorMetadata> stoneMetadataById;
        if (!matchedStoneIds.empty()) {
            try {
                auto dbClient = drogon::app().getDbClient("default");
                auto rows = dbClient->execSqlSync(
                    "SELECT stone_id, content, mood_type, created_at "
                    "FROM stones "
                    "WHERE stone_id = ANY($1::text[])",
                    buildPgTextArrayLiteral(matchedStoneIds));
                for (const auto& row : rows) {
                    StoneVectorMetadata metadata;
                    metadata.content =
                        row["content"].isNull() ? "" : row["content"].as<std::string>();
                    metadata.moodType =
                        row["mood_type"].isNull() ? "" : row["mood_type"].as<std::string>();
                    metadata.createdAt =
                        row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
                    stoneMetadataById.emplace(row["stone_id"].as<std::string>(),
                                              std::move(metadata));
                }
            } catch (const std::exception& e) {
                LOG_WARN << "EdgeAI vectorSearch metadata lookup failed: "
                         << e.what();
            }
        }

        Json::Value data;
        Json::Value resultArray(Json::arrayValue);
        for (const auto &r : results) {
            if (r.similarity >= threshold) {
                Json::Value item;
                item["id"] = r.id;
                const auto metadataIt = stoneMetadataById.find(r.id);
                if (metadataIt != stoneMetadataById.end()) {
                    item["content"] = metadataIt->second.content;
                    item["text"] = metadataIt->second.content;
                    item["mood_type"] = metadataIt->second.moodType;
                    item["created_at"] = metadataIt->second.createdAt;
                    item["content_type"] = "stone";
                }
                item["similarity"] = r.similarity;
                item["distance"] = r.distance;
                resultArray.append(item);
            }
        }
        data["results"] = resultArray;
        data["total"] = static_cast<int>(resultArray.size());
        data["query_length"] = static_cast<int>(query.size());
        data["retrieval_mode"] = enableSecondStage ? "two_stage_ann_rerank" : "single_stage_ann";
        data["candidate_count"] = enableSecondStage ? candidateK : returnedCandidates;
        data["returned_candidates"] = returnedCandidates;

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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getAdminConfig called";
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        auto stats = engine.getEngineStats();
        const auto aiConfig = heartlake::utils::AdminConfigStore::load()["ai"];
        const auto config =
            buildAdminEdgeAIConfigPayload(aiConfig, stats, engine.isEnabled());

        Json::Value data;
        data = config;
        data["enabled"] = config.get("edge_ai_enabled", engine.isEnabled());
        data["engine_enabled"] = engine.isEnabled();
        data["engine_stats"] = stats;
#ifdef HEARTLAKE_USE_ONNX
        data["onnx_compiled"] = true;
#else
        data["onnx_compiled"] = false;
#endif
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
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] updateAdminConfig called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        const auto& body = *jsonPtr;
        auto persisted = heartlake::utils::AdminConfigStore::load();
        Json::Value persistedAiConfig = persisted["ai"];

        // 记录配置变更日志
        LOG_INFO << "Admin updating EdgeAI config: " << body.toStyledString();

        Json::Value runtimeConfig;
        Json::Value appliedKeys(Json::arrayValue);
        Json::Value restartRequiredKeys(Json::arrayValue);
        Json::Value ignoredKeys(Json::arrayValue);

        for (const auto& key : body.getMemberNames()) {
            const auto& value = body[key];

            if (key == "enabled" || key == "edge_ai_enabled" ||
                key == "edgeAIEnabled") {
                const bool enabled = parseJsonBoolValue(value, true);
                persistedAiConfig["edge_ai_enabled"] = enabled;
                runtimeConfig["enabled"] = enabled;
                appliedKeys.append(key);
            } else if (key == "hnsw_m" || key == "hnswM") {
                const int parsed = parseJsonIntValue(value, 16);
                persistedAiConfig["hnsw_m"] = parsed;
                runtimeConfig["hnsw_m"] = parsed;
                appliedKeys.append(key);
            } else if (key == "hnsw_ef_construction" ||
                       key == "hnswEfConstruction") {
                const int parsed = parseJsonIntValue(value, 200);
                persistedAiConfig["hnsw_ef_construction"] = parsed;
                runtimeConfig["hnsw_ef_construction"] = parsed;
                appliedKeys.append(key);
            } else if (key == "hnsw_ef_search" || key == "hnswEfSearch") {
                const int parsed = parseJsonIntValue(value, 50);
                persistedAiConfig["hnsw_ef_search"] = parsed;
                runtimeConfig["hnsw_ef_search"] = parsed;
                appliedKeys.append(key);
            } else if (key == "dp_epsilon" || key == "dpEpsilon" ||
                       key == "maxEpsilon" || key == "privacyEpsilon" ||
                       key == "max_epsilon" || key == "privacy_epsilon") {
                const double parsed = parseJsonDoubleValue(value, 1.0);
                persistedAiConfig["dp_epsilon"] = parsed;
                runtimeConfig["dp_epsilon"] = parsed;
                appliedKeys.append(key);
            } else if (key == "dp_delta" || key == "dpDelta") {
                const double parsed = parseJsonDoubleValue(value, 1e-5);
                persistedAiConfig["dp_delta"] = parsed;
                runtimeConfig["dp_delta"] = parsed;
                appliedKeys.append(key);
            } else if (key == "pulse_window_seconds" ||
                       key == "pulseWindowSeconds") {
                const int parsed = parseJsonIntValue(value, 300);
                persistedAiConfig["pulse_window_seconds"] = parsed;
                runtimeConfig["pulse_window_seconds"] = parsed;
                appliedKeys.append(key);
            } else if (key == "cacheTTL" || key == "cache_ttl" ||
                       key == "sentiment_cache_ttl") {
                const int parsed = parseJsonIntValue(value, 300);
                persistedAiConfig["sentiment_cache_ttl"] = parsed;
                runtimeConfig["sentiment_cache_ttl"] = parsed;
                appliedKeys.append(key);
            } else if (key == "sentimentCacheMax" ||
                       key == "sentiment_cache_max") {
                const int parsed = std::max(1, parseJsonIntValue(value, 4096));
                persistedAiConfig["sentiment_cache_max"] = parsed;
                runtimeConfig["sentiment_cache_max"] = parsed;
                appliedKeys.append(key);
            } else if (key == "onnx_enabled" || key == "onnxEnabled") {
                persistedAiConfig["onnx_enabled"] =
                    parseJsonStringValue(value, "auto");
                restartRequiredKeys.append(key);
            } else if (key == "inferenceEngine") {
                const auto engineName = parseJsonStringValue(value, "onnx");
                persistedAiConfig["onnx_enabled"] =
                    engineName == "onnx" ? "true" : "false";
                restartRequiredKeys.append(key);
            } else if (key == "model_path" || key == "modelPath") {
                persistedAiConfig["model_path"] = parseJsonStringValue(value);
                restartRequiredKeys.append(key);
            } else if (key == "vocab_path" || key == "vocabPath") {
                persistedAiConfig["vocab_path"] = parseJsonStringValue(value);
                restartRequiredKeys.append(key);
            } else if (key == "onnx_threads" || key == "onnxThreads") {
                persistedAiConfig["onnx_threads"] =
                    parseJsonIntValue(value, 2);
                restartRequiredKeys.append(key);
            } else if (key == "cacheStrategy" || key == "cache_strategy" ||
                       key == "maxBatchSize" || key == "max_batch_size" ||
                       key == "cacheSizeMB" || key == "cache_size_mb" ||
                       key == "federatedInterval" || key == "federated_interval" ||
                       key == "emotionModel" || key == "emotion_model" ||
                       key == "vectorSearchEnabled" || key == "vector_search_enabled" ||
                       key == "inferenceTimeout" || key == "inference_timeout" ||
                       key == "cacheEnabled" || key == "cache_enabled" ||
                       key == "federatedEnabled" || key == "federated_enabled" ||
                       key == "quantization_bits") {
                persistedAiConfig[key] = value;
                restartRequiredKeys.append(key);
            } else {
                ignoredKeys.append(key);
            }
        }

        if (appliedKeys.empty() && restartRequiredKeys.empty()) {
            callback(ResponseUtil::badRequest(
                "没有可应用的配置项"));
            return;
        }

        persisted["ai"] = persistedAiConfig;
        if (!heartlake::utils::AdminConfigStore::save(persisted)) {
            callback(ResponseUtil::internalError("配置持久化失败"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!runtimeConfig.empty()) {
            engine.initialize(runtimeConfig);
        }
        const auto stats = engine.getEngineStats();
        const auto config =
            buildAdminEdgeAIConfigPayload(persistedAiConfig, stats, engine.isEnabled());

        Json::Value data;
        data["status"] = "config_updated";
        if (!appliedKeys.empty()) {
            data["applied_keys"] = appliedKeys;
        }
        if (!restartRequiredKeys.empty()) {
            data["restart_required_keys"] = restartRequiredKeys;
        }
        if (!ignoredKeys.empty()) {
            data["ignored_keys"] = ignoredKeys;
        }
        data["engine_enabled"] = engine.isEnabled();
        data["config"] = config;

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
        double hitRate = std::min(1.0, static_cast<double>(totalNodes) / static_cast<double>(totalSearches));
        metrics["hit_rate"] = hitRate;
        metrics["miss_rate"] = 1.0 - hitRate;
    } else {
        metrics["hit_rate"] = 0.0;
        metrics["miss_rate"] = totalSearches > 0 ? 1.0 : 0.0;
    }

    // SemanticCache 真实命中率
    auto& aiService = heartlake::ai::AIService::getInstance();
    auto scStats = aiService.getSemanticCacheStats();
    Json::Value sc;
    sc["exact_hits"] = static_cast<Json::UInt64>(scStats.exactHits);
    sc["semantic_hits"] = static_cast<Json::UInt64>(scStats.semanticHits);
    sc["misses"] = static_cast<Json::UInt64>(scStats.misses);
    sc["hit_rate"] = scStats.hitRate();
    sc["estimated_savings"] = static_cast<Json::UInt64>(scStats.estimatedSavings());
    metrics["semantic_cache"] = sc;

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
    metrics["throughput_estimated"] = true;

    return metrics;
}

void EdgeAIController::vectorInsert(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] vectorInsert called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        std::string id = (*jsonPtr).get("id", "").asString();
        if (id.empty()) {
            callback(ResponseUtil::badRequest("id不能为空"));
            return;
        }

        if (!jsonPtr->isMember("vector") || !(*jsonPtr)["vector"].isArray()) {
            callback(ResponseUtil::badRequest("vector必须为数组"));
            return;
        }

        std::vector<float> vec;
        for (const auto &v : (*jsonPtr)["vector"]) {
            vec.push_back(v.asFloat());
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        engine.hnswRemove(id);
        engine.hnswInsert(id, vec);

        Json::Value data;
        data["id"] = id;
        data["dimension"] = static_cast<int>(vec.size());
        callback(ResponseUtil::success(data, "向量已插入"));
    } catch (const std::exception &e) {
        LOG_ERROR << "vectorInsert error: " << e.what();
        callback(ResponseUtil::internalError("向量插入失败"));
    }
}

void EdgeAIController::submitEmotionSample(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] submitEmotionSample called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        float score = (*jsonPtr).get("score", 0.0f).asFloat();
        std::string mood = normalizeMoodInput((*jsonPtr).get("mood", "neutral").asString());
        float confidence = std::clamp((*jsonPtr).get("confidence", 1.0f).asFloat(), 0.0f, 1.0f);
        std::string scoreScale = (*jsonPtr).get("score_scale", "").asString();

        // 兼容两种分数口径：
        // -1~1（推荐，signed）
        // 0~1（历史客户端，可通过 score_scale=zero_one 显式声明）
        if (scoreScale == "zero_one") {
            score = score * 2.0f - 1.0f;
        } else if (scoreScale.empty() && score >= 0.0f && score <= 1.0f) {
            static const std::unordered_set<std::string> negativeMoods = {
                "sad", "anxious", "angry", "lonely"
            };
            if (negativeMoods.find(mood) != negativeMoods.end()) {
                score = score * 2.0f - 1.0f;
            }
        }

        if (score < -1.0f || score > 1.0f) {
            callback(ResponseUtil::badRequest("score必须在-1到1之间"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        engine.submitEmotionSample(score, mood, confidence);

        Json::Value data;
        data["score"] = score;
        data["normalized_score"] = std::clamp((score + 1.0f) / 2.0f, 0.0f, 1.0f);
        data["mood"] = mood;
        data["confidence"] = confidence;
        callback(ResponseUtil::success(data, "情绪样本已提交"));
    } catch (const std::exception &e) {
        LOG_ERROR << "submitEmotionSample error: " << e.what();
        callback(ResponseUtil::internalError("情绪样本提交失败"));
    }
}

// ==================== 新增端点实现 ====================

void EdgeAIController::getPulseHistory(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] getPulseHistory called";
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        int count = 10;
        auto countParam = req->getParameter("count");
        if (!countParam.empty()) {
            count = safeInt(countParam, 10);
            if (count < 1 || count > 100) count = 10;
        }

        auto history = engine.getPulseHistory(count);
        Json::Value data(Json::arrayValue);
        for (const auto &pulse : history) {
            data.append(pulse.toJson());
        }

        Json::Value result = buildStaticCollectionPayload("history", data);
        result["count"] = static_cast<int>(history.size());
        callback(ResponseUtil::success(result, "情绪脉搏历史"));
    } catch (const std::exception &e) {
        LOG_ERROR << "getPulseHistory error: " << e.what();
        callback(ResponseUtil::internalError("获取脉搏历史失败"));
    }
}

void EdgeAIController::submitLocalModel(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] submitLocalModel called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        heartlake::ai::FederatedModelParams params;
        params.modelId = (*jsonPtr).get("model_id", "").asString();
        params.nodeId = (*jsonPtr).get("node_id", "").asString();
        params.sampleCount = (*jsonPtr).get("sample_count", 0).asUInt();
        params.localLoss = (*jsonPtr).get("local_loss", 0.0f).asFloat();
        params.epoch = (*jsonPtr).get("epoch", 0).asInt();

        if (jsonPtr->isMember("weights") && (*jsonPtr)["weights"].isArray()) {
            for (const auto &row : (*jsonPtr)["weights"]) {
                std::vector<float> weightRow;
                if (row.isArray()) {
                    for (const auto &v : row) weightRow.push_back(v.asFloat());
                }
                params.weights.push_back(weightRow);
            }
        }
        if (jsonPtr->isMember("biases") && (*jsonPtr)["biases"].isArray()) {
            for (const auto &v : (*jsonPtr)["biases"]) {
                params.biases.push_back(v.asFloat());
            }
        }

        engine.submitLocalModel(params);

        Json::Value data;
        data["model_id"] = params.modelId;
        data["node_id"] = params.nodeId;
        callback(ResponseUtil::success(data, "本地模型已提交"));
    } catch (const std::exception &e) {
        LOG_ERROR << "submitLocalModel error: " << e.what();
        callback(ResponseUtil::internalError("提交本地模型失败"));
    }
}

void EdgeAIController::resetPrivacyBudget(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] resetPrivacyBudget called";
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        engine.resetPrivacyBudget();

        Json::Value data;
        data["remaining_budget"] = engine.getRemainingPrivacyBudget();
        callback(ResponseUtil::success(data, "隐私预算已重置"));
    } catch (const std::exception &e) {
        LOG_ERROR << "resetPrivacyBudget error: " << e.what();
        callback(ResponseUtil::internalError("重置隐私预算失败"));
    }
}

void EdgeAIController::registerNode(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] registerNode called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        std::string nodeId = (*jsonPtr).get("node_id", "").asString();
        if (nodeId.empty()) {
            callback(ResponseUtil::badRequest("node_id不能为空"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        engine.registerNode(nodeId);

        Json::Value data;
        data["node_id"] = nodeId;
        callback(ResponseUtil::success(data, "节点已注册"));
    } catch (const std::exception &e) {
        LOG_ERROR << "registerNode error: " << e.what();
        callback(ResponseUtil::internalError("注册节点失败"));
    }
}

void EdgeAIController::updateNodeStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] updateNodeStatus called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        heartlake::ai::EdgeNodeStatus status;
        status.nodeId = (*jsonPtr).get("node_id", "").asString();
        status.cpuUsage = (*jsonPtr).get("cpu_usage", 0.0f).asFloat();
        status.memoryUsage = (*jsonPtr).get("memory_usage", 0.0f).asFloat();
        status.latencyMs = (*jsonPtr).get("latency_ms", 0.0f).asFloat();
        status.activeConnections = (*jsonPtr).get("active_connections", 0).asInt();
        status.totalRequests = (*jsonPtr).get("total_requests", 0).asInt();
        status.failedRequests = (*jsonPtr).get("failed_requests", 0).asInt();
        status.isHealthy = (*jsonPtr).get("is_healthy", true).asBool();

        engine.updateNodeStatus(status);

        Json::Value data;
        data["node_id"] = status.nodeId;
        callback(ResponseUtil::success(data, "节点状态已更新"));
    } catch (const std::exception &e) {
        LOG_ERROR << "updateNodeStatus error: " << e.what();
        callback(ResponseUtil::internalError("更新节点状态失败"));
    }
}

void EdgeAIController::selectBestNode(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] selectBestNode called";
        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        auto bestNode = engine.selectBestNode();

        Json::Value data;
        if (bestNode.has_value()) {
            data["node_id"] = bestNode.value();
            data["found"] = true;
        } else {
            data["node_id"] = Json::nullValue;
            data["found"] = false;
        }
        callback(ResponseUtil::success(data, "最优节点查询"));
    } catch (const std::exception &e) {
        LOG_ERROR << "selectBestNode error: " << e.what();
        callback(ResponseUtil::internalError("查询最优节点失败"));
    }
}

void EdgeAIController::quantizedForward(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] quantizedForward called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        // 解析输入向量
        std::vector<float> input;
        if (jsonPtr->isMember("input") && (*jsonPtr)["input"].isArray()) {
            for (const auto &v : (*jsonPtr)["input"]) input.push_back(v.asFloat());
        }

        // 解析权重矩阵 -> 量化
        std::vector<float> weightsFlat;
        size_t outDim = 0, inDim = 0;
        if (jsonPtr->isMember("weights") && (*jsonPtr)["weights"].isArray()) {
            outDim = (*jsonPtr)["weights"].size();
            for (const auto &row : (*jsonPtr)["weights"]) {
                if (row.isArray()) {
                    if (inDim == 0) inDim = row.size();
                    for (const auto &v : row) weightsFlat.push_back(v.asFloat());
                }
            }
        }
        auto qWeights = engine.quantizeToInt8(weightsFlat, {outDim, inDim});

        // 解析偏置
        std::vector<float> biases;
        if (jsonPtr->isMember("biases") && (*jsonPtr)["biases"].isArray()) {
            for (const auto &v : (*jsonPtr)["biases"]) biases.push_back(v.asFloat());
        }

        auto output = engine.quantizedForward(input, qWeights, biases);

        Json::Value data;
        Json::Value outputArr(Json::arrayValue);
        for (float v : output) outputArr.append(v);
        data["output"] = outputArr;
        data["dimension"] = static_cast<int>(output.size());
        callback(ResponseUtil::success(data, "量化推理完成"));
    } catch (const std::exception &e) {
        LOG_ERROR << "quantizedForward error: " << e.what();
        callback(ResponseUtil::internalError("量化推理失败"));
    }
}

void EdgeAIController::addNoise(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] addNoise called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        auto &engine = heartlake::ai::EdgeAIEngine::getInstance();
        if (!engine.isEnabled()) {
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "边缘AI引擎未启用"));
            return;
        }

        float sensitivity = (*jsonPtr).get("sensitivity", 1.0f).asFloat();

        Json::Value data;
        if (jsonPtr->isMember("values") && (*jsonPtr)["values"].isArray()) {
            std::vector<float> values;
            for (const auto &v : (*jsonPtr)["values"]) values.push_back(v.asFloat());
            auto noisy = engine.addLaplaceNoiseVec(values, sensitivity);
            Json::Value arr(Json::arrayValue);
            for (float v : noisy) arr.append(v);
            data["values"] = arr;
        } else if (jsonPtr->isMember("value")) {
            float value = (*jsonPtr)["value"].asFloat();
            data["value"] = engine.addLaplaceNoise(value, sensitivity);
        } else {
            callback(ResponseUtil::badRequest("需要value或values字段"));
            return;
        }

        data["remaining_budget"] = engine.getRemainingPrivacyBudget();
        callback(ResponseUtil::success(data, "噪声已注入"));
    } catch (const std::exception &e) {
        LOG_ERROR << "addNoise error: " << e.what();
        callback(ResponseUtil::internalError("噪声注入失败"));
    }
}

void EdgeAIController::generateSummary(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] generateSummary called";
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
            callback(ResponseUtil::badRequest("请求体必须为JSON"));
            return;
        }

        std::string stoneId = (*jsonPtr).get("stone_id", "").asString();
        std::string content = (*jsonPtr).get("content", "").asString();

        if (content.empty() || content.size() > 10000) {
            callback(ResponseUtil::badRequest("content长度必须在1-10000之间"));
            return;
        }

        if (content.size() < heartlake::ai::SummaryService::MIN_LENGTH) {
            Json::Value data;
            data["summary"] = content;
            data["skipped"] = true;
            callback(ResponseUtil::success(data, "文本过短，无需摘要"));
            return;
        }

        auto cb = std::make_shared<std::function<void(const HttpResponsePtr &)>>(std::move(callback));
        heartlake::ai::SummaryService::getInstance().generateSummary(
            stoneId, content,
            [cb](const std::string& summary, const std::string& error) {
                if (!error.empty()) {
                    (*cb)(ResponseUtil::internalError("摘要生成失败: " + error));
                    return;
                }
                Json::Value data;
                data["summary"] = summary;
                data["skipped"] = false;
                (*cb)(ResponseUtil::success(data, "摘要已生成"));
            }
        );
    } catch (const std::exception &e) {
        LOG_ERROR << "generateSummary error: " << e.what();
        callback(ResponseUtil::internalError("摘要生成失败"));
    }
}

void EdgeAIController::lakeGodChat(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    drogon::async_run([=, callback = std::move(callback)]() -> drogon::Task<void> {
        auto userIdOpt = Validator::getUserId(req);
        if (!userIdOpt) {
            callback(ResponseUtil::unauthorized("未登录"));
            co_return;
        }
        auto userId = *userIdOpt;

        try {
            auto traceId = req->getAttributes()->get<std::string>("trace_id");
            LOG_INFO << "[trace:" << traceId << "] lakeGodChat called";
            auto jsonPtr = req->getJsonObject();
            if (!jsonPtr) {
                callback(ResponseUtil::error(ErrorCode::INVALID_PARAMETER, "请求体不能为空"));
                co_return;
            }

            // 兼容历史客户端: 支持 content / message 双字段。
            std::string content = jsonPtr->get("content", "").asString();
            if (content.empty()) {
                content = jsonPtr->get("message", "").asString();
            }

            if (content.empty() || content.size() > 10000) {
                callback(ResponseUtil::error(ErrorCode::INVALID_PARAMETER, "content长度必须在1-10000之间"));
                co_return;
            }

            auto &engine = heartlake::ai::EdgeAIEngine::getInstance();

            // 内容审核
            auto moderateResult = engine.moderateTextLocal(content);
            if (!moderateResult.passed) {
                callback(ResponseUtil::error(ErrorCode::CONTENT_DELETED,
                    "内容未通过审核: " + moderateResult.suggestion));
                co_return;
            }

            // 情感分析
            auto sentiment = engine.analyzeSentimentLocal(content);
            engine.submitEmotionSample(sentiment.score, sentiment.mood, sentiment.confidence);

            // 生成湖神回复
            auto &rag = heartlake::ai::DualMemoryRAG::getInstance();
            auto ragReply =
                rag.generateResponseResult(userId, content, sentiment.mood, sentiment.score);

            // 异步存入数据库
            auto db = drogon::app().getDbClient("default");
            db->execSqlAsync(
                "INSERT INTO lake_god_messages (user_id, role, content, mood, emotion_score) VALUES ($1, $2, $3, $4, $5)",
                [](const drogon::orm::Result &) {},
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Insert lake_god_messages (user) failed: " << e.base().what();
                },
                userId, "user", content, sentiment.mood, sentiment.score
            );
            db->execSqlAsync(
                "INSERT INTO lake_god_messages (user_id, role, content, mood, emotion_score) VALUES ($1, $2, $3, $4, $5)",
                [](const drogon::orm::Result &) {},
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Insert lake_god_messages (assistant) failed: " << e.base().what();
                },
                userId, "assistant", ragReply.reply, sentiment.mood, sentiment.score
            );

            Json::Value data;
            data["reply"] = ragReply.reply;
            data["mood"] = sentiment.mood;
            data["score"] = sentiment.score;
            data["agent"] = "lake_god";
            data["agent_name"] = "湖神";
            data["response_source"] = ragReply.source;
            data["degraded"] = ragReply.degraded;
            if (!ragReply.error.empty()) {
                data["warning"] = ragReply.error;
                data["ai_error"] = ragReply.error;
                LOG_WARN << "lakeGodChat degraded for user " << userId
                         << ": " << ragReply.error;
            }
            callback(ResponseUtil::success(
                data,
                ragReply.degraded ? "湖神本地陪伴回复" : "湖神回复成功"));
        } catch (const std::exception &e) {
            LOG_ERROR << "lakeGodChat error: " << e.what();
            callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "湖神对话服务暂时不可用"));
        }
    });
}

void EdgeAIController::lakeGodHistory(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录"));
        return;
    }
    auto userId = *userIdOpt;

    try {
        auto traceId = req->getAttributes()->get<std::string>("trace_id");
        LOG_INFO << "[trace:" << traceId << "] lakeGodHistory called";
        auto db = drogon::app().getDbClient("default");
        auto cb = std::make_shared<std::function<void(const HttpResponsePtr &)>>(std::move(callback));

        db->execSqlAsync(
            "SELECT role, content, mood, emotion_score, created_at FROM lake_god_messages WHERE user_id = $1 ORDER BY created_at ASC LIMIT 50",
            [cb](const drogon::orm::Result &result) {
                Json::Value messages = Json::arrayValue;
                for (const auto &row : result) {
                    Json::Value msg;
                    msg["role"] = row["role"].as<std::string>();
                    msg["content"] = row["content"].as<std::string>();
                    msg["mood"] = row["mood"].isNull() ? "" : row["mood"].as<std::string>();
                    msg["emotion_score"] = row["emotion_score"].isNull() ? 0.0 : row["emotion_score"].as<double>();
                    msg["created_at"] = row["created_at"].as<std::string>();
                    messages.append(msg);
                }
                Json::Value data;
                data["messages"] = messages;
                (*cb)(ResponseUtil::success(data, "历史消息获取成功"));
            },
            [cb](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "lakeGodHistory query failed: " << e.base().what();
                (*cb)(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "历史消息查询失败"));
            },
            userId
        );
    } catch (const std::exception &e) {
        LOG_ERROR << "lakeGodHistory error: " << e.what();
        callback(ResponseUtil::error(ErrorCode::AI_SERVICE_ERROR, "历史消息服务暂时不可用"));
    }
}

} // namespace controllers
} // namespace heartlake
