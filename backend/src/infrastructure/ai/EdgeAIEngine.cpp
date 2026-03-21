/**
 * @brief 边缘 AI 推理引擎 —— Facade 模式统一入口
 *
 * 将情感分析、内容审核、HNSW 向量检索、联邦学习、差分隐私、
 * 情绪脉搏、节点监控、模型量化等子系统聚合为统一接口。
 * 所有公开方法委托给对应的子系统实例，本类不包含业务逻辑。
 */

#include "infrastructure/ai/EdgeAIEngine.h"
#include "utils/EnvUtils.h"
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <thread>

namespace heartlake {
namespace ai {

namespace {

bool parseBoolLiteral(const std::string& raw, bool& value) {
    std::string normalized = raw;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        value = true;
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        value = false;
        return true;
    }
    return false;
}

#ifdef HEARTLAKE_USE_ONNX
std::string getConfigOrEnvPath(const Json::Value& config,
                               const char* configKey,
                               const char* envKey,
                               const std::string& fallback) {
    if (config.isObject() && config.isMember(configKey) && config[configKey].isString()) {
        const auto configured = config[configKey].asString();
        if (!configured.empty()) {
            return configured;
        }
    }
    if (const char* envValue = std::getenv(envKey); envValue && *envValue != '\0') {
        return envValue;
    }
    return fallback;
}

bool isRegularFile(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

std::filesystem::path resolveRuntimeRelativePath(const std::filesystem::path& path) {
    if (path.empty() || path.is_absolute()) {
        return path;
    }
    static const std::vector<std::filesystem::path> prefixes = {
        {}, std::filesystem::path(".."),
        std::filesystem::path("backend"), std::filesystem::path("../backend")
    };
    for (const auto& prefix : prefixes) {
        const auto candidate = prefix.empty() ? path : (prefix / path);
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
    return path;
}

std::filesystem::path resolveModelPath(const std::string& configuredPath) {
    std::filesystem::path modelPath = configuredPath.empty()
        ? std::filesystem::path("./models/sentiment_zh.onnx")
        : std::filesystem::path(configuredPath);
    modelPath = resolveRuntimeRelativePath(modelPath);
    std::error_code ec;
    if (std::filesystem::is_directory(modelPath, ec) ||
        (!modelPath.has_extension() && !isRegularFile(modelPath))) {
        modelPath /= "sentiment_zh.onnx";
    }
    return modelPath;
}

std::filesystem::path resolveVocabPath(const std::string& configuredPath,
                                       const std::filesystem::path& modelPath) {
    std::filesystem::path vocabPath;
    if (configuredPath.empty()) {
        const auto modelDir = modelPath.parent_path();
        vocabPath = modelDir.empty()
            ? std::filesystem::path("./models/vocab.txt")
            : modelDir / "vocab.txt";
    } else {
        vocabPath = configuredPath;
    }
    vocabPath = resolveRuntimeRelativePath(vocabPath);
    std::error_code ec;
    if (std::filesystem::is_directory(vocabPath, ec) ||
        (!vocabPath.has_extension() && !isRegularFile(vocabPath))) {
        vocabPath /= "vocab.txt";
    }
    return vocabPath;
}
#endif

#ifdef HEARTLAKE_USE_ONNX
int defaultOnnxThreadsForRuntime(bool lowResource) {
    return lowResource ? 1 : 2;
}
#endif

} // namespace

// ============================================================================
// 单例 & 初始化
// ============================================================================

EdgeAIEngine& EdgeAIEngine::getInstance() {
    static EdgeAIEngine instance;
    return instance;
}

void EdgeAIEngine::initialize(const Json::Value& config) {
    bool didInitialize = false;
    std::call_once(initFlag_, [this, &config, &didInitialize]() {
        didInitialize = true;
        initializeImpl(config);
    });

    if (!didInitialize && initialized_.load(std::memory_order_acquire)) {
        applyRuntimeConfig(config);
    }
}

void EdgeAIEngine::initializeImpl(const Json::Value& config) {
    LOG_INFO << "[EdgeAI] Initializing Edge AI Engine...";

    lowResourceDefaultsApplied_ = []() {
        if (const char* raw = std::getenv("HEARTLAKE_LOW_RESOURCE_MODE");
            raw && *raw != '\0') {
            bool enabled = false;
            if (parseBoolLiteral(raw, enabled)) {
                return enabled;
            }
        }

        const unsigned int hw = std::thread::hardware_concurrency();
        return hw > 0 && hw <= 2;
    }();
    if (lowResourceDefaultsApplied_) {
        hnswEfConstructionConfig_ = std::min(hnswEfConstructionConfig_, 128);
        hnswEfSearchConfig_ = std::min(hnswEfSearchConfig_, 32);
        sentimentCacheMaxConfig_ = std::min<size_t>(sentimentCacheMaxConfig_, 2048);
        LOG_INFO << "[EdgeAI] Applying low-resource defaults for 2c2g-class runtime";
    }

    applyRuntimeConfig(config);

    initialized_.store(true, std::memory_order_release);
    LOG_INFO << "[EdgeAI] Edge AI Engine initialized successfully (facade mode)";

    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }

#ifdef HEARTLAKE_USE_ONNX
    onnxEngine_.reset();
    onnxEnabled_ = false;

    std::optional<bool> configuredOnnxEnabled;
    if (config.isObject() && config.isMember("onnx_enabled")) {
        const auto& configuredValue = config["onnx_enabled"];
        if (configuredValue.isBool()) {
            configuredOnnxEnabled = configuredValue.asBool();
        } else if (configuredValue.isInt() || configuredValue.isUInt()) {
            configuredOnnxEnabled = configuredValue.asInt() != 0;
        } else if (configuredValue.isString()) {
            const auto normalized = configuredValue.asString();
            if (!normalized.empty() && normalized != "auto") {
                bool parsed = false;
                if (parseBoolLiteral(normalized, parsed)) {
                    configuredOnnxEnabled = parsed;
                } else {
                    LOG_WARN << "[EdgeAI] Invalid onnx_enabled config=" << normalized;
                }
            }
        }
    }
    if (!configuredOnnxEnabled.has_value()) {
        if (const char* onnxEnabledEnv = std::getenv("EDGE_AI_ONNX_ENABLED");
            onnxEnabledEnv && *onnxEnabledEnv != '\0') {
            bool parsed = false;
            if (parseBoolLiteral(onnxEnabledEnv, parsed)) {
                configuredOnnxEnabled = parsed;
            } else {
                LOG_WARN << "[EdgeAI] Invalid EDGE_AI_ONNX_ENABLED=" << onnxEnabledEnv;
            }
        }
    }
    const bool allowOnnx = !configuredOnnxEnabled.has_value() ||
                           configuredOnnxEnabled.value();

    const std::string modelPathSetting = getConfigOrEnvPath(
        config, "model_path", "EDGE_AI_MODEL_PATH", "./models/sentiment_zh.onnx");
    const std::string vocabPathSetting = getConfigOrEnvPath(
        config, "vocab_path", "EDGE_AI_VOCAB_PATH", "");
    const std::filesystem::path resolvedModelPath = resolveModelPath(modelPathSetting);
    const std::filesystem::path resolvedVocabPath = resolveVocabPath(vocabPathSetting, resolvedModelPath);
    const bool modelExists = isRegularFile(resolvedModelPath);

    if (modelExists && allowOnnx) {
        try {
            onnxEngine_ = std::make_unique<OnnxSentimentEngine>();
            int onnxThreads = defaultOnnxThreadsForRuntime(lowResourceDefaultsApplied_);
            if (config.isObject() && config.isMember("onnx_threads")) {
                const auto& configuredThreads = config["onnx_threads"];
                if (configuredThreads.isInt() || configuredThreads.isUInt()) {
                    onnxThreads = std::max(1, configuredThreads.asInt());
                } else if (configuredThreads.isString()) {
                    onnxThreads = heartlake::utils::parsePositiveInt(
                        configuredThreads.asCString(), onnxThreads);
                }
            } else {
                onnxThreads = heartlake::utils::parsePositiveIntEnv(
                    "EDGE_AI_ONNX_THREADS", onnxThreads);
            }
            onnxEngine_->initialize(
                resolvedModelPath.string(),
                resolvedVocabPath.string(),
                onnxThreads
            );
            if (onnxEngine_->isInitialized()) {
                onnxEnabled_ = true;
                sentiment_->setOnnxEngine(std::move(onnxEngine_));
                LOG_INFO << "[EdgeAI] ONNX sentiment engine loaded: " << resolvedModelPath;
            }
        } catch (const std::exception& e) {
            LOG_WARN << "[EdgeAI] ONNX init failed: " << e.what();
            onnxEngine_.reset();
            onnxEnabled_ = false;
        }
    } else if (!modelExists) {
        LOG_WARN << "[EdgeAI] ONNX model file missing: " << resolvedModelPath.string();
    }
#endif
}

void EdgeAIEngine::ensureSubsystemsReady() {
    std::lock_guard<std::mutex> lock(subsystemInitMutex_);

    const bool createSentiment = !sentiment_;
    const bool createModerator = !moderator_;

    if (createSentiment) {
        sentiment_ = std::make_unique<SentimentAnalyzer>();
    }
    if (createModerator) {
        moderator_ = std::make_unique<ContentModerator>();
    }
    if (!pulse_) {
        pulse_ = std::make_unique<EmotionPulseDetector>();
    }
    if (!federated_) {
        federated_ = std::make_unique<FederatedLearner>();
    }
    if (!dp_) {
        dp_ = std::make_unique<EdgeDifferentialPrivacy>();
    }
    if (!hnsw_) {
        hnsw_ = std::make_unique<HNSWIndex>();
    }
    if (!quantizer_) {
        quantizer_ = std::make_unique<ModelQuantizer>();
    }
    if (!monitor_) {
        monitor_ = std::make_unique<EdgeNodeMonitor>();
    }

    if (createModerator) {
        moderator_->buildModerationAC();
    }
    if (createSentiment) {
        sentimentCacheTTLConfig_ =
            heartlake::utils::parsePositiveIntEnv("SENTIMENT_CACHE_TTL",
                                                  sentimentCacheTTLConfig_);
        sentimentCacheMaxConfig_ = static_cast<size_t>(
            heartlake::utils::parsePositiveIntEnv(
                "SENTIMENT_CACHE_MAX",
                static_cast<int>(sentimentCacheMaxConfig_)));
        sentiment_->loadLexicon();
    }
}

void EdgeAIEngine::applyRuntimeConfig(const Json::Value& config) {
    if (config.isObject()) {
        if (config.isMember("enabled")) {
            enabled_.store(config.get("enabled", true).asBool(), std::memory_order_release);
        } else if (!initialized_.load(std::memory_order_relaxed)) {
            enabled_.store(true, std::memory_order_release);
        }

        hnswMConfig_ = config.get("hnsw_m", hnswMConfig_).asInt();
        hnswMMax0Config_ = config.get("hnsw_mmax0", hnswMMax0Config_).asInt();
        hnswEfConstructionConfig_ = config.get("hnsw_ef_construction", hnswEfConstructionConfig_).asInt();
        hnswEfSearchConfig_ = config.get("hnsw_ef_search", hnswEfSearchConfig_).asInt();

        dpEpsilonConfig_ = config.get("dp_epsilon", dpEpsilonConfig_).asFloat();
        dpDeltaConfig_ = config.get("dp_delta", dpDeltaConfig_).asFloat();
        dpSensitivityConfig_ = config.get("dp_sensitivity", dpSensitivityConfig_).asFloat();
        dpMaxBudgetConfig_ = config.get("dp_max_budget", dpMaxBudgetConfig_).asFloat();
        dpMaxDeltaBudgetConfig_ = config.get("dp_max_delta_budget", dpMaxDeltaBudgetConfig_).asFloat();

        pulseWindowSecondsConfig_ = config.get("pulse_window_seconds", pulseWindowSecondsConfig_).asInt();
        maxPulseHistoryConfig_ = config.get("max_pulse_history", maxPulseHistoryConfig_).asInt();
        sentimentCacheTTLConfig_ = config.get("sentiment_cache_ttl", sentimentCacheTTLConfig_).asInt();
        const auto configuredSentimentCacheMax = config.get(
            "sentiment_cache_max",
            static_cast<Json::UInt64>(sentimentCacheMaxConfig_)).asLargestUInt();
        sentimentCacheMaxConfig_ = static_cast<size_t>(
            std::max<Json::Value::LargestUInt>(
                static_cast<Json::Value::LargestUInt>(1),
                configuredSentimentCacheMax));
    } else if (!initialized_.load(std::memory_order_relaxed)) {
        enabled_.store(true, std::memory_order_release);
    }

    if (enabled_.load(std::memory_order_acquire)) {
        ensureSubsystemsReady();
    }

    if (hnsw_) {
        hnsw_->configure(hnswMConfig_, hnswMMax0Config_,
                         hnswEfConstructionConfig_, hnswEfSearchConfig_);
    }

    if (dp_) {
        DPConfig dpConfig;
        dpConfig.epsilon = dpEpsilonConfig_;
        dpConfig.delta = dpDeltaConfig_;
        dpConfig.sensitivity = dpSensitivityConfig_;
        dpConfig.maxEpsilonBudget = dpMaxBudgetConfig_;
        dpConfig.maxDeltaBudget = dpMaxDeltaBudgetConfig_;
        dp_->configure(dpConfig);
    }

    if (pulse_) {
        pulse_->configure(pulseWindowSecondsConfig_, maxPulseHistoryConfig_);
    }

    if (sentiment_) {
        sentiment_->configure(sentimentCacheTTLConfig_, sentimentCacheMaxConfig_);
    }

    if (!enabled_.load(std::memory_order_acquire)) {
        LOG_WARN << "[EdgeAI] Edge AI Engine disabled by configuration";
    }
}

bool EdgeAIEngine::isEnabled() const {
    return enabled_.load(std::memory_order_acquire)
        && initialized_.load(std::memory_order_acquire);
}

// ============================================================================
// 子系统1: 情感分析 → SentimentAnalyzer
// ============================================================================

EdgeSentimentResult EdgeAIEngine::analyzeSentimentLocal(const std::string& text, bool preferOnnx) {
    if (!isEnabled() || !sentiment_) {
        return {0.0f, "neutral", 0.0f, "disabled"};
    }
    return sentiment_->analyzeSentiment(text, preferOnnx);
}

void EdgeAIEngine::loadEdgeSentimentLexicon() {
    if (sentiment_) sentiment_->loadLexicon();
}

// ============================================================================
// 子系统2: 内容审核 → ContentModerator
// ============================================================================

EdgeModerationResult EdgeAIEngine::moderateTextLocal(const std::string& text) {
    if (!isEnabled() || !moderator_) {
        EdgeModerationResult r;
        r.passed = true;
        r.riskLevel = "safe";
        r.confidence = 0.9f;
        r.needsAlert = false;
        return r;
    }
    return moderator_->moderateTextLocal(text);
}

void EdgeAIEngine::buildModerationAC() {
    if (moderator_) moderator_->buildModerationAC();
}

// ============================================================================
// 子系统3: 情绪脉搏 → EmotionPulseDetector
// ============================================================================

void EdgeAIEngine::recordEmotion(const std::string& /*userId*/, const std::string& mood, float intensity) {
    if (!isEnabled() || !pulse_) return;
    pulse_->submitEmotionSample(intensity, mood, 1.0f);
}

void EdgeAIEngine::submitEmotionSample(float score, const std::string& mood, float confidence) {
    if (!isEnabled() || !pulse_) return;
    pulse_->submitEmotionSample(score, mood, confidence);
}

EmotionPulse EdgeAIEngine::getCurrentPulse() {
    if (!isEnabled() || !pulse_) return {};
    return pulse_->getCurrentPulse();
}

std::vector<EmotionPulse> EdgeAIEngine::getPulseHistory(int n) {
    if (!isEnabled() || !pulse_) return {};
    return pulse_->getPulseHistory(n);
}

// ============================================================================
// 子系统4: 联邦学习 → FederatedLearner
// ============================================================================

void EdgeAIEngine::submitLocalModel(const FederatedModelParams& params) {
    if (!isEnabled() || !federated_) return;
    federated_->submitLocalModel(params);
}

FederatedModelParams EdgeAIEngine::aggregateFedAvg(float clippingBound, float noiseSigma, float mu) {
    if (!isEnabled() || !federated_) return {};
    return federated_->aggregateFedAvg(clippingBound, noiseSigma, mu);
}

Json::Value EdgeAIEngine::getFederatedStatus() const {
    if (!isEnabled() || !federated_) return {};
    return federated_->getFederatedStatus();
}

// ============================================================================
// 子系统5: 差分隐私 → EdgeDifferentialPrivacy
// ============================================================================

float EdgeAIEngine::addLaplaceNoise(float value, float sensitivity) {
    if (!isEnabled() || !dp_) return value;
    return dp_->addLaplaceNoise(value, sensitivity);
}

std::vector<float> EdgeAIEngine::addLaplaceNoiseVec(const std::vector<float>& values, float sensitivity) {
    if (!isEnabled() || !dp_) return values;
    return dp_->addLaplaceNoiseVec(values, sensitivity);
}

std::vector<float> EdgeAIEngine::addGaussianNoiseVec(const std::vector<float>& values,
                                                      float sensitivity, float delta) {
    if (!isEnabled() || !dp_) return values;
    return dp_->addGaussianNoiseVec(values, sensitivity, delta);
}

float EdgeAIEngine::getRemainingPrivacyBudget() const {
    if (!dp_) return 0.0f;
    return dp_->getRemainingPrivacyBudget();
}

float EdgeAIEngine::getRemainingDeltaBudget() const {
    if (!dp_) return 0.0f;
    return dp_->getRemainingDeltaBudget();
}

void EdgeAIEngine::resetPrivacyBudget() {
    if (dp_) dp_->resetPrivacyBudget();
}

// ============================================================================
// 子系统6: HNSW向量检索 → HNSWIndex
// ============================================================================

void EdgeAIEngine::hnswInsert(const std::string& id, const std::vector<float>& vec) {
    if (!isEnabled() || !hnsw_) return;
    hnsw_->addVector(id, vec);
}

std::vector<VectorSearchResult> EdgeAIEngine::hnswSearch(const std::vector<float>& query, int k) {
    if (!isEnabled() || !hnsw_) return {};
    return hnsw_->searchKNN(query, k);
}

std::vector<VectorSearchResult> EdgeAIEngine::rerankHNSWCandidates(
    const std::vector<float>& query,
    const std::vector<VectorSearchResult>& candidates,
    int topK) const {
    if (!isEnabled() || !hnsw_) return {};
    return hnsw_->rerankCandidates(query, candidates, topK);
}

bool EdgeAIEngine::hnswRemove(const std::string& id) {
    if (!isEnabled() || !hnsw_) return false;
    return hnsw_->removeVector(id);
}

size_t EdgeAIEngine::getHNSWVectorCount() const {
    if (!hnsw_) return 0;
    return hnsw_->getVectorCount();
}

Json::Value EdgeAIEngine::getHNSWStats() const {
    if (!hnsw_) return {};
    return hnsw_->getHNSWStats();
}

size_t EdgeAIEngine::getHNSWVectorDimension() const {
    if (!hnsw_) return 0;
    return hnsw_->getHNSWVectorDimension();
}

// ============================================================================
// 子系统7: 模型量化 → ModelQuantizer
// ============================================================================

ModelQuantizer::QuantizedTensor EdgeAIEngine::quantizeToInt8(
    const std::vector<float>& tensor, const std::vector<size_t>& shape) {
    if (!quantizer_) return {};
    return quantizer_->quantizeToInt8(tensor, shape);
}

std::vector<float> EdgeAIEngine::quantizedMatMul(
    const ModelQuantizer::QuantizedTensor& a,
    const ModelQuantizer::QuantizedTensor& b,
    size_t M, size_t K, size_t N) {
    if (!quantizer_) return std::vector<float>(M * N, 0.0f);
    return quantizer_->quantizedMatMul(a, b, M, K, N);
}

std::vector<float> EdgeAIEngine::quantizedForward(
    const std::vector<float>& input,
    const ModelQuantizer::QuantizedTensor& weights,
    const std::vector<float>& biases) {
    if (!quantizer_) return {};
    return quantizer_->quantizedForward(input, weights, biases);
}

// ============================================================================
// 子系统8: 节点监控 → EdgeNodeMonitor
// ============================================================================

void EdgeAIEngine::registerNode(const std::string& nodeId) {
    if (!isEnabled() || !monitor_) return;
    monitor_->registerNode(nodeId);
}

void EdgeAIEngine::reportNodeStatus(const EdgeNodeStatus& status) {
    if (!isEnabled() || !monitor_) return;
    monitor_->updateNodeStatus(status);
}

void EdgeAIEngine::updateNodeStatus(const EdgeNodeStatus& status) {
    if (!isEnabled() || !monitor_) return;
    monitor_->updateNodeStatus(status);
}

std::optional<std::string> EdgeAIEngine::selectBestNode() const {
    if (!isEnabled() || !monitor_) return std::nullopt;
    return monitor_->selectBestNode();
}

std::vector<EdgeNodeStatus> EdgeAIEngine::getAllNodeStatus() const {
    if (!monitor_) return {};
    return monitor_->getAllNodeStatus();
}

Json::Value EdgeAIEngine::getNodeDashboard() const {
    if (!monitor_) return {};
    return monitor_->getNodeMonitorStats();
}

// ============================================================================
// 综合统计
// ============================================================================

Json::Value EdgeAIEngine::getEngineStats() const {
    Json::Value stats;
    Json::Value callCounts(Json::objectValue);
    stats["enabled"] = enabled_.load(std::memory_order_acquire);
    stats["initialized"] = initialized_.load(std::memory_order_acquire);
    stats["low_resource_defaults_applied"] = lowResourceDefaultsApplied_;
    stats["hnsw_m"] = hnswMConfig_;
    stats["sentiment_cache_ttl_seconds"] = sentimentCacheTTLConfig_;
    stats["sentiment_cache_max_entries"] = static_cast<Json::UInt64>(sentimentCacheMaxConfig_);
    stats["hnsw_ef_construction"] = hnswEfConstructionConfig_;
    stats["hnsw_ef_search"] = hnswEfSearchConfig_;
    stats["dp_epsilon"] = dpEpsilonConfig_;
    stats["dp_delta"] = dpDeltaConfig_;
    stats["pulse_window_seconds"] = pulseWindowSecondsConfig_;

    if (sentiment_) {
        stats["total_sentiment_calls"] = static_cast<Json::UInt64>(sentiment_->getTotalCalls());
        stats["sentiment_cache_hits"] = static_cast<Json::UInt64>(sentiment_->getCacheHits());
        stats["sentiment_cache_misses"] = static_cast<Json::UInt64>(sentiment_->getCacheMisses());
        callCounts["sentiment_analysis"] =
            static_cast<Json::UInt64>(sentiment_->getTotalCalls());
    }

    if (moderator_) {
        callCounts["text_moderation"] =
            static_cast<Json::UInt64>(moderator_->getTotalCalls());
    }

    if (hnsw_) {
        auto hnswStats = hnsw_->getHNSWStats();
        stats["hnsw_node_count"] = hnswStats.get("node_count", 0u);
        stats["hnsw_max_level"] = hnswStats.get("max_level", 0);
        callCounts["hnsw_searches"] = hnswStats.get("total_searches", 0u);
    }

    if (pulse_) {
        auto pulse = pulse_->getCurrentPulse();
        stats["emotion_window_size"] = static_cast<Json::UInt64>(pulse.sampleCount);
    }

    if (federated_) {
        stats["federated_round"] = federated_->getCurrentRound();
        stats["pending_local_models"] = static_cast<Json::UInt64>(federated_->getPendingModelCount());
    }

    if (dp_) {
        stats["dp_remaining_budget"] = dp_->getRemainingPrivacyBudget();
    }

    if (monitor_) {
        auto dashboard = monitor_->getNodeMonitorStats();
        stats["registered_nodes"] = dashboard.get("total_nodes", 0u);
        stats["healthy_nodes"] = dashboard.get("healthy_nodes", 0);
    }

    if (quantizer_) {
        callCounts["quantized_ops"] =
            static_cast<Json::UInt64>(quantizer_->getTotalOps());
    }
    stats["call_counts"] = callCounts;

    return stats;
}

} // namespace ai
} // namespace heartlake
