/**
 * @file EdgeAIEngine.cpp
 * @brief 边缘AI推理引擎 - 完整实现
 *
 * 八大核心子系统完整实现：
 * 1. 轻量级情感分析（规则+词典+统计模型三层融合）
 * 2. 本地文本审核（AC自动机多模式匹配+语义规则）
 * 3. 实时情绪脉搏检测（滑动窗口统计）
 * 4. 联邦学习聚合器（FedAvg加权聚合）
 * 5. 差分隐私噪声注入（Laplace机制+隐私预算追踪）
 * 6. 本地向量相似度检索（HNSW近似最近邻）
 * 7. 模型量化推理模拟（INT8量化/反量化）
 * 8. 边缘节点健康监控与自适应负载均衡
 *
 * Created by 王璐瑶
 */

#include "infrastructure/ai/EdgeAIEngine.h"
#include "utils/PsychologicalRiskAssessment.h"
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <cassert>
#include <set>
#include <limits>
#include <random>

namespace heartlake {
namespace ai {

namespace {

std::string toLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool parseBoolLiteral(const std::string& raw, bool& value) {
    const std::string normalized = toLowerCopy(raw);
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

int parsePositiveIntEnv(const char* rawValue, int fallback) {
    if (!rawValue) {
        return fallback;
    }
    char* endPtr = nullptr;
    long value = std::strtol(rawValue, &endPtr, 10);
    if (endPtr == rawValue || *endPtr != '\0' || value <= 0) {
        return fallback;
    }
    return static_cast<int>(value);
}

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

bool isDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool containsAnyPhrase(const std::string& text, const std::vector<std::string>& markers) {
    for (const auto& marker : markers) {
        if (text.find(marker) != std::string::npos) {
            return true;
        }
    }
    return false;
}

struct AffectiveAnchorSignal {
    float score{0.0f};      // [-1, 1]
    float strength{0.0f};   // [0, 1]
    float conflict{0.0f};   // [0, 1]
};

float weightedPhraseScore(const std::string& text,
                          const std::vector<std::pair<std::string, float>>& lexicon) {
    float score = 0.0f;
    for (const auto& [phrase, weight] : lexicon) {
        size_t pos = 0;
        while ((pos = text.find(phrase, pos)) != std::string::npos) {
            score += weight;
            pos += phrase.size();
        }
    }
    return score;
}

std::string refineMoodFromCues(const std::string& text,
                               float score,
                               const std::string& fallbackMood) {
    static const std::vector<std::pair<std::string, float>> kHappyCues = {
        {"开心", 1.2f}, {"高兴", 1.1f}, {"快乐", 1.1f}, {"喜悦", 1.0f},
        {"幸福", 1.0f}, {"暖暖", 0.9f}, {"被认可", 0.95f}, {"认可", 0.75f},
        {"被夸", 0.90f}, {"夸了我", 0.95f}, {"礼物", 0.90f}, {"惊喜", 0.85f}
    };
    static const std::vector<std::pair<std::string, float>> kCalmCues = {
        {"平静", 1.3f}, {"放松", 1.1f}, {"放松下来", 1.2f}, {"安定", 1.2f},
        {"安稳", 1.0f}, {"从容", 0.9f}, {"淡定", 0.9f}, {"舒缓", 0.95f},
        {"慢下来", 0.8f}, {"继续努力", 0.95f}, {"散步聊天", 0.85f},
        {"喝茶", 0.75f}, {"心里安定", 1.1f}
    };
    static const std::vector<std::pair<std::string, float>> kAnxiousCues = {
        {"焦虑", 1.4f}, {"担心", 1.2f}, {"紧张", 1.1f}, {"不安", 1.1f},
        {"压力", 1.0f}, {"发紧", 1.0f}, {"心跳很快", 1.1f}, {"慌", 0.95f},
        {"害怕", 1.05f}, {"恐惧", 1.2f}
    };
    static const std::vector<std::pair<std::string, float>> kSadCues = {
        {"难过", 1.3f}, {"伤心", 1.3f}, {"失落", 1.1f}, {"受挫", 1.0f},
        {"低落", 1.0f}, {"沉默了很久", 0.95f}, {"难受", 1.1f}, {"痛苦", 1.2f}
    };
    static const std::vector<std::pair<std::string, float>> kAngryCues = {
        {"生气", 1.4f}, {"愤怒", 1.5f}, {"恼火", 1.4f}, {"火大", 1.5f},
        {"气炸", 1.3f}, {"抓狂", 1.1f}, {"气死", 1.2f}, {"被连续打断", 0.95f},
        {"莫名否定", 0.95f}, {"连续否决", 0.95f}
    };
    static const std::vector<std::pair<std::string, float>> kSurprisedCues = {
        {"震惊", 1.4f}, {"惊讶", 1.3f}, {"惊到", 1.3f}, {"惊到了", 1.3f},
        {"出乎意料", 1.3f}, {"没反应过来", 1.2f}, {"太突然", 1.0f},
        {"突然", 0.8f}, {"居然", 0.9f}, {"意外", 0.9f}
    };
    static const std::vector<std::pair<std::string, float>> kConfusedCues = {
        {"困惑", 1.4f}, {"懵", 1.3f}, {"迷茫", 1.1f}, {"乱了", 1.2f},
        {"搞不懂", 1.2f}, {"前后矛盾", 1.1f}, {"不知道该", 1.0f},
        {"信息太多", 1.0f}, {"没头绪", 1.0f}
    };
    static const std::vector<std::pair<std::string, float>> kNeutralCues = {
        {"没什么特别情绪", 1.5f}, {"心情平平", 1.4f}, {"日程正常", 1.2f},
        {"按计划", 1.1f}, {"一般", 0.75f}, {"就这样", 0.8f},
        {"发呆", 0.95f}, {"没有好消息也没有坏消息", 1.6f}, {"什么都没发生", 1.4f}
    };

    const float happyCue = weightedPhraseScore(text, kHappyCues);
    const float calmCue = weightedPhraseScore(text, kCalmCues);
    const float anxiousCue = weightedPhraseScore(text, kAnxiousCues);
    const float sadCue = weightedPhraseScore(text, kSadCues);
    const float angryCue = weightedPhraseScore(text, kAngryCues);
    const float surprisedCue = weightedPhraseScore(text, kSurprisedCues);
    const float confusedCue = weightedPhraseScore(text, kConfusedCues);
    float neutralCue = weightedPhraseScore(text, kNeutralCues);

    if (text.find("不开心") != std::string::npos &&
        text.find("不难过") != std::string::npos) {
        neutralCue += 1.6f;
    }

    const float maxCue = std::max(
        {happyCue, calmCue, anxiousCue, sadCue, angryCue, surprisedCue, confusedCue, neutralCue});

    if (neutralCue >= 1.1f &&
        neutralCue >= calmCue &&
        neutralCue >= anxiousCue &&
        neutralCue >= sadCue &&
        std::abs(score) <= 0.75f) {
        return "neutral";
    }
    if (angryCue >= 1.0f && angryCue >= sadCue * 0.90f && angryCue >= anxiousCue * 0.90f) {
        return "angry";
    }
    if (surprisedCue >= 1.0f && surprisedCue + 0.10f >= happyCue && surprisedCue >= anxiousCue) {
        return "surprised";
    }
    if (confusedCue >= 1.0f && confusedCue + 0.10f >= anxiousCue && confusedCue + 0.10f >= sadCue) {
        return "confused";
    }
    if (anxiousCue >= 0.95f && anxiousCue >= sadCue * 0.85f) {
        return "anxious";
    }
    if (sadCue >= 0.95f && sadCue >= anxiousCue * 1.05f) {
        return "sad";
    }
    if (calmCue >= 1.0f && calmCue >= happyCue * 0.90f && std::abs(score) <= 0.90f) {
        return "calm";
    }
    if (happyCue >= 0.95f && score >= -0.20f) {
        return "happy";
    }
    if (maxCue < 0.35f && std::abs(score) < 0.18f) {
        return "neutral";
    }

    return fallbackMood;
}

float calibrateScoreByMoodCue(const std::string& text,
                              const std::string& mood,
                              float score) {
    static const std::vector<std::string> kNeutralStrongSignals = {
        "没什么特别情绪", "心情平平", "日程正常", "按计划", "没有好消息也没有坏消息",
        "什么都没发生", "不开心，也不难过", "不开心也不难过", "就是发呆", "心情一般"
    };

    if (mood == "neutral") {
        if (containsAnyPhrase(text, kNeutralStrongSignals)) {
            return std::clamp(score, -0.08f, 0.08f);
        }
        return std::clamp(score, -0.15f, 0.15f);
    }

    if ((mood == "anxious" || mood == "sad" || mood == "angry" || mood == "confused") &&
        score > -0.05f) {
        return -std::max(0.12f, std::abs(score) * 0.45f);
    }

    if ((mood == "happy" || mood == "calm" || mood == "surprised") &&
        score < 0.05f) {
        return std::max(0.12f, std::abs(score) * 0.45f);
    }

    return score;
}

AffectiveAnchorSignal computeAffectiveAnchor(const std::string& text) {
    static const std::vector<std::pair<std::string, float>> kPositiveAnchors = {
        {"今天", 0.18f}, {"礼物", 0.95f}, {"收到了", 0.70f}, {"收到", 0.55f},
        {"夸", 0.52f}, {"表扬", 0.92f}, {"认可", 0.82f}, {"肯定", 0.80f},
        {"通过", 0.72f}, {"成功", 0.78f}, {"晋级", 0.70f}, {"获奖", 0.86f},
        {"惊喜", 0.82f}, {"感谢", 0.66f}, {"幸福", 0.78f}, {"开心", 0.86f},
        {"快乐", 0.84f}, {"好看", 0.50f}, {"喜欢", 0.58f}, {"治愈", 0.64f}
    };
    static const std::vector<std::pair<std::string, float>> kNegativeAnchors = {
        {"焦虑", 0.95f}, {"担心", 0.75f}, {"不安", 0.72f}, {"紧张", 0.62f},
        {"难过", 0.88f}, {"伤心", 0.92f}, {"失落", 0.72f}, {"害怕", 0.84f},
        {"恐惧", 0.90f}, {"压力", 0.72f}, {"烦躁", 0.74f}, {"痛苦", 0.95f},
        {"绝望", 1.00f}, {"崩溃", 1.00f}, {"失败", 0.72f}, {"后悔", 0.68f}
    };
    static const std::vector<std::string> kContrastMarkers = {
        "但是", "但", "不过", "然而", "可是", "只是"
    };

    float posScore = weightedPhraseScore(text, kPositiveAnchors);
    float negScore = weightedPhraseScore(text, kNegativeAnchors);

    size_t markerPos = std::string::npos;
    size_t markerLen = 0;
    for (const auto& marker : kContrastMarkers) {
        const auto pos = text.rfind(marker);
        if (pos != std::string::npos && (markerPos == std::string::npos || pos > markerPos)) {
            markerPos = pos;
            markerLen = marker.size();
        }
    }
    if (markerPos != std::string::npos && markerPos + markerLen < text.size()) {
        const auto tail = text.substr(markerPos + markerLen);
        posScore += 1.25f * weightedPhraseScore(tail, kPositiveAnchors);
        negScore += 1.25f * weightedPhraseScore(tail, kNegativeAnchors);
    }

    const float evidence = posScore + negScore;
    if (evidence <= 0.01f) {
        return {};
    }

    const float score = std::clamp((posScore - negScore) / (evidence + 1.2f), -1.0f, 1.0f);
    const float strength = std::clamp(std::tanh(evidence / 4.5f), 0.0f, 1.0f);
    const float conflict = (posScore > 0.01f && negScore > 0.01f)
        ? std::clamp(std::min(posScore, negScore) / std::max(posScore, negScore), 0.0f, 1.0f)
        : 0.0f;
    return {score, strength, conflict};
}

std::filesystem::path resolveRuntimeRelativePath(const std::filesystem::path& path) {
    if (path.empty() || path.is_absolute()) {
        return path;
    }

    static const std::vector<std::filesystem::path> prefixes = {
        {},
        std::filesystem::path(".."),
        std::filesystem::path("backend"),
        std::filesystem::path("../backend")
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
    if (isDirectory(modelPath) || (!modelPath.has_extension() && !isRegularFile(modelPath))) {
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
    if (isDirectory(vocabPath) || (!vocabPath.has_extension() && !isRegularFile(vocabPath))) {
        vocabPath /= "vocab.txt";
    }
    return vocabPath;
}

float medianValue(std::vector<float> values) {
    if (values.empty()) {
        return 0.0f;
    }
    const size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + mid, values.end());
    if (values.size() % 2 == 1) {
        return values[mid];
    }
    const float upper = values[mid];
    std::nth_element(values.begin(), values.begin() + mid - 1, values.end());
    const float lower = values[mid - 1];
    return (lower + upper) * 0.5f;
}

} // namespace

// ============================================================================
// 单例 & 初始化
// ============================================================================

EdgeAIEngine& EdgeAIEngine::getInstance() {
    static EdgeAIEngine instance;
    return instance;
}

void EdgeAIEngine::initialize(const Json::Value& config) {
    LOG_INFO << "[EdgeAI] Initializing Edge AI Engine...";

    enabled_ = config.get("enabled", true).asBool();
    if (!enabled_) {
        LOG_WARN << "[EdgeAI] Edge AI Engine is disabled by configuration";
        return;
    }

    // HNSW参数配置
    hnswM_ = config.get("hnsw_m", 16).asInt();
    hnswMMax0_ = config.get("hnsw_mmax0", 32).asInt();
    hnswEfConstruction_ = config.get("hnsw_ef_construction", 200).asInt();
    hnswEfSearch_ = config.get("hnsw_ef_search", 50).asInt();
    hnswLevelMult_ = 1.0f / std::log(std::max(2.0f, static_cast<float>(hnswM_)));
    hnswRng_.seed(std::random_device{}());

    // 差分隐私配置
    dpConfig_.epsilon = config.get("dp_epsilon", 1.0f).asFloat();
    dpConfig_.delta = config.get("dp_delta", 1e-5f).asFloat();
    dpConfig_.sensitivity = config.get("dp_sensitivity", 1.0f).asFloat();
    dpConfig_.maxEpsilonBudget = config.get("dp_max_budget", 10.0f).asFloat();
    consumedEpsilon_.store(0.0f);
    dpRng_.seed(std::random_device{}());

    // 情绪脉搏配置
    pulseWindowSeconds_ = config.get("pulse_window_seconds", 300).asInt();
    maxPulseHistory_ = config.get("max_pulse_history", 100).asInt();

    // 情感分析缓存配置（高并发场景显著降低重复推理开销）
    sentimentCacheTTLSeconds_ = std::max(
        1,
        config.get(
            "sentiment_cache_ttl_sec",
            parsePositiveIntEnv(std::getenv("EDGE_AI_SENTIMENT_CACHE_TTL_SEC"), sentimentCacheTTLSeconds_)
        ).asInt()
    );
    sentimentCacheMaxSize_ = static_cast<size_t>(std::max(
        128,
        config.get(
            "sentiment_cache_max_size",
            parsePositiveIntEnv(std::getenv("EDGE_AI_SENTIMENT_CACHE_MAX_SIZE"),
                                static_cast<int>(sentimentCacheMaxSize_))
        ).asInt()
    ));

    // 加载情感词典
    loadEdgeSentimentLexicon();

    // 构建审核AC自动机
    buildModerationAC();

    // 清空HNSW索引（重新初始化时需要）
    {
        std::unique_lock<std::shared_mutex> lock(hnswMutex_);
        hnswNodes_.clear();
        hnswIdMap_.clear();
        hnswEntryPoint_ = -1;
        hnswMaxLevel_ = 0;
    }

    // 清空情绪脉搏数据
    {
        std::unique_lock<std::shared_mutex> lock(pulseMutex_);
        emotionWindow_.clear();
        pulseHistory_.clear();
    }

    // 清空节点注册表
    {
        std::unique_lock<std::shared_mutex> lock(nodeMutex_);
        nodeRegistry_.clear();
    }

    // 清空情感缓存与并发合并状态
    {
        std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
        sentimentCache_.clear();
        sentimentInFlight_.clear();
    }
    sentimentCacheTick_.store(0);
    sentimentCacheHits_.store(0);
    sentimentCacheMisses_.store(0);

    initialized_ = true;
    LOG_INFO << "[EdgeAI] Edge AI Engine initialized successfully"
             << ", sentiment_cache_ttl_sec=" << sentimentCacheTTLSeconds_
             << ", sentiment_cache_max_size=" << sentimentCacheMaxSize_;

#ifdef HEARTLAKE_USE_ONNX
    onnxEngine_.reset();
    onnxEnabled_ = false;

    // 初始化 ONNX 情感分析引擎
    const char* onnxEnabledEnv = std::getenv("EDGE_AI_ONNX_ENABLED");
    bool onnxWanted = false;
    const bool hasExplicitOnnxSwitch = (onnxEnabledEnv != nullptr);
    if (hasExplicitOnnxSwitch) {
        if (!parseBoolLiteral(onnxEnabledEnv, onnxWanted)) {
            LOG_WARN << "[EdgeAI] Invalid EDGE_AI_ONNX_ENABLED=" << onnxEnabledEnv
                     << ", expected true/false/1/0/on/off. ONNX will stay disabled.";
            onnxWanted = false;
        }
    }

    const std::string modelPathSetting = getConfigOrEnvPath(
        config, "model_path", "EDGE_AI_MODEL_PATH", "./models/sentiment_zh.onnx");
    const std::string vocabPathSetting = getConfigOrEnvPath(
        config, "vocab_path", "EDGE_AI_VOCAB_PATH", "");
    const std::filesystem::path resolvedModelPath = resolveModelPath(modelPathSetting);
    const std::filesystem::path resolvedVocabPath = resolveVocabPath(vocabPathSetting, resolvedModelPath);
    const bool modelExists = isRegularFile(resolvedModelPath);
    const bool vocabExists = isRegularFile(resolvedVocabPath);

    if (!hasExplicitOnnxSwitch) {
        onnxWanted = modelExists && vocabExists;
        if (onnxWanted) {
            LOG_INFO << "[EdgeAI] EDGE_AI_ONNX_ENABLED not set, auto-enable ONNX with detected artifacts";
        } else {
            LOG_INFO << "[EdgeAI] ONNX sentiment engine not enabled (missing model/vocab artifacts). "
                     << "Set EDGE_AI_ONNX_ENABLED=true to force ONNX attempt.";
        }
    }

    if (onnxWanted) {
        if (!modelExists || !vocabExists) {
            LOG_WARN << "[EdgeAI] ONNX requested but artifacts are missing. "
                     << "model=" << resolvedModelPath.string() << " exists=" << modelExists
                     << ", vocab=" << resolvedVocabPath.string() << " exists=" << vocabExists
                     << ". Fallback sentiment pipeline will be used.";
        } else {
            const int onnxThreads = parsePositiveIntEnv(std::getenv("EDGE_AI_ONNX_THREADS"), 2);
            onnxEngine_ = std::make_unique<OnnxSentimentEngine>();
            if (onnxEngine_->initialize(resolvedModelPath.string(),
                                        resolvedVocabPath.string(),
                                        onnxThreads)) {
                onnxEnabled_ = true;
                LOG_INFO << "[EdgeAI] ONNX sentiment engine enabled: " << resolvedModelPath.string();
            } else {
                LOG_WARN << "[EdgeAI] ONNX sentiment engine failed to initialize, using fallback";
                onnxEngine_.reset();
                onnxEnabled_ = false;
            }
        }
    } else {
        LOG_INFO << "[EdgeAI] ONNX sentiment engine disabled by configuration";
    }
#endif
}

bool EdgeAIEngine::isEnabled() const {
    return enabled_ && initialized_;
}

// ============================================================================
// 子系统1: 轻量级情感分析
// ============================================================================

void EdgeAIEngine::loadEdgeSentimentLexicon() {
    // 正面情感词典
    sentimentLexicon_["happy"] = 0.8f;
    sentimentLexicon_["love"] = 0.9f;
    sentimentLexicon_["great"] = 0.7f;
    sentimentLexicon_["wonderful"] = 0.85f;
    sentimentLexicon_["excellent"] = 0.9f;
    sentimentLexicon_["amazing"] = 0.85f;
    sentimentLexicon_["fantastic"] = 0.85f;
    sentimentLexicon_["beautiful"] = 0.75f;
    sentimentLexicon_["good"] = 0.6f;
    sentimentLexicon_["nice"] = 0.55f;
    sentimentLexicon_["awesome"] = 0.8f;
    sentimentLexicon_["joy"] = 0.85f;
    sentimentLexicon_["delight"] = 0.8f;
    sentimentLexicon_["pleased"] = 0.65f;
    sentimentLexicon_["grateful"] = 0.75f;
    sentimentLexicon_["thankful"] = 0.7f;
    sentimentLexicon_["excited"] = 0.8f;
    sentimentLexicon_["cheerful"] = 0.75f;
    sentimentLexicon_["brilliant"] = 0.8f;
    sentimentLexicon_["perfect"] = 0.9f;

    // 中文正面词
    sentimentLexicon_["开心"] = 0.8f;
    sentimentLexicon_["快乐"] = 0.85f;
    sentimentLexicon_["喜欢"] = 0.7f;
    sentimentLexicon_["爱"] = 0.9f;
    sentimentLexicon_["棒"] = 0.75f;
    sentimentLexicon_["好"] = 0.5f;
    sentimentLexicon_["优秀"] = 0.8f;
    sentimentLexicon_["感谢"] = 0.7f;
    sentimentLexicon_["幸福"] = 0.9f;
    sentimentLexicon_["温暖"] = 0.7f;
    sentimentLexicon_["美好"] = 0.75f;
    sentimentLexicon_["精彩"] = 0.8f;
    sentimentLexicon_["赞"] = 0.7f;
    sentimentLexicon_["厉害"] = 0.7f;
    sentimentLexicon_["感动"] = 0.75f;
    sentimentLexicon_["舒适"] = 0.65f;
    sentimentLexicon_["满足"] = 0.7f;
    sentimentLexicon_["自信"] = 0.7f;
    sentimentLexicon_["希望"] = 0.65f;
    sentimentLexicon_["勇敢"] = 0.7f;
    // 扩充中文正面词
    sentimentLexicon_["高兴"] = 0.8f;
    sentimentLexicon_["愉快"] = 0.75f;
    sentimentLexicon_["欣喜"] = 0.8f;
    sentimentLexicon_["欢乐"] = 0.8f;
    sentimentLexicon_["兴奋"] = 0.8f;
    sentimentLexicon_["激动"] = 0.75f;
    sentimentLexicon_["享受"] = 0.7f;
    sentimentLexicon_["充实"] = 0.65f;
    sentimentLexicon_["感恩"] = 0.75f;
    sentimentLexicon_["幸运"] = 0.7f;
    sentimentLexicon_["愉悦"] = 0.75f;
    sentimentLexicon_["欣慰"] = 0.7f;
    sentimentLexicon_["惊喜"] = 0.8f;
    sentimentLexicon_["甜蜜"] = 0.8f;
    sentimentLexicon_["浪漫"] = 0.7f;
    sentimentLexicon_["舒心"] = 0.7f;
    sentimentLexicon_["安心"] = 0.65f;
    sentimentLexicon_["放松"] = 0.6f;
    sentimentLexicon_["宁静"] = 0.6f;
    sentimentLexicon_["平静"] = 0.55f;
    sentimentLexicon_["从容"] = 0.6f;
    sentimentLexicon_["淡定"] = 0.55f;
    sentimentLexicon_["自在"] = 0.65f;
    sentimentLexicon_["轻松"] = 0.6f;
    sentimentLexicon_["惬意"] = 0.7f;
    sentimentLexicon_["舒畅"] = 0.7f;
    sentimentLexicon_["治愈"] = 0.75f;
    sentimentLexicon_["暖心"] = 0.75f;
    sentimentLexicon_["可爱"] = 0.7f;
    sentimentLexicon_["有趣"] = 0.65f;
    sentimentLexicon_["好玩"] = 0.65f;
    sentimentLexicon_["期待"] = 0.65f;
    sentimentLexicon_["向往"] = 0.6f;
    sentimentLexicon_["珍惜"] = 0.65f;
    sentimentLexicon_["值得"] = 0.6f;
    sentimentLexicon_["成功"] = 0.75f;
    sentimentLexicon_["进步"] = 0.65f;
    sentimentLexicon_["成长"] = 0.6f;
    sentimentLexicon_["收获"] = 0.7f;
    sentimentLexicon_["骄傲"] = 0.7f;
    sentimentLexicon_["自豪"] = 0.75f;
    sentimentLexicon_["加油"] = 0.7f;
    sentimentLexicon_["奥利给"] = 0.75f;
    sentimentLexicon_["绝绝子"] = 0.7f;
    sentimentLexicon_["yyds"] = 0.75f;
    sentimentLexicon_["真香"] = 0.7f;
    sentimentLexicon_["上头"] = 0.65f;
    // 认可/被表扬语义（保证“被夸被肯定”不被低估为平静）
    sentimentLexicon_["夸奖"] = 0.82f;
    sentimentLexicon_["表扬"] = 0.85f;
    sentimentLexicon_["认可"] = 0.82f;
    sentimentLexicon_["肯定"] = 0.78f;
    sentimentLexicon_["称赞"] = 0.80f;
    sentimentLexicon_["赞扬"] = 0.80f;
    sentimentLexicon_["赞许"] = 0.78f;
    sentimentLexicon_["嘉奖"] = 0.86f;
    sentimentLexicon_["鼓励"] = 0.72f;
    sentimentLexicon_["夸"] = 0.58f;
    sentimentLexicon_["被夸"] = 0.82f;
    sentimentLexicon_["被表扬"] = 0.86f;

    // 英文正面补充
    sentimentLexicon_["hopeful"] = 0.7f;
    sentimentLexicon_["proud"] = 0.75f;
    sentimentLexicon_["confident"] = 0.7f;
    sentimentLexicon_["peaceful"] = 0.65f;
    sentimentLexicon_["content"] = 0.6f;
    sentimentLexicon_["inspired"] = 0.75f;

    // 负面情感词典
    sentimentLexicon_["sad"] = -0.7f;
    sentimentLexicon_["angry"] = -0.8f;
    sentimentLexicon_["hate"] = -0.9f;
    sentimentLexicon_["terrible"] = -0.85f;
    sentimentLexicon_["horrible"] = -0.85f;
    sentimentLexicon_["awful"] = -0.8f;
    sentimentLexicon_["bad"] = -0.6f;
    sentimentLexicon_["disgusting"] = -0.85f;
    sentimentLexicon_["fear"] = -0.7f;
    sentimentLexicon_["scared"] = -0.65f;
    sentimentLexicon_["worried"] = -0.5f;
    sentimentLexicon_["anxious"] = -0.55f;
    sentimentLexicon_["depressed"] = -0.85f;
    sentimentLexicon_["miserable"] = -0.8f;
    sentimentLexicon_["frustrated"] = -0.65f;
    sentimentLexicon_["disappointed"] = -0.6f;
    sentimentLexicon_["upset"] = -0.65f;
    sentimentLexicon_["lonely"] = -0.6f;
    sentimentLexicon_["painful"] = -0.7f;
    sentimentLexicon_["annoyed"] = -0.55f;

    // 中文负面词
    sentimentLexicon_["难过"] = -0.7f;
    sentimentLexicon_["伤心"] = -0.75f;
    sentimentLexicon_["生气"] = -0.7f;
    sentimentLexicon_["愤怒"] = -0.85f;
    sentimentLexicon_["讨厌"] = -0.7f;
    sentimentLexicon_["恨"] = -0.9f;
    sentimentLexicon_["害怕"] = -0.65f;
    sentimentLexicon_["恐惧"] = -0.75f;
    sentimentLexicon_["焦虑"] = -0.6f;
    sentimentLexicon_["抑郁"] = -0.85f;
    sentimentLexicon_["痛苦"] = -0.8f;
    sentimentLexicon_["绝望"] = -0.9f;
    sentimentLexicon_["烦躁"] = -0.6f;
    sentimentLexicon_["委屈"] = -0.65f;
    sentimentLexicon_["失落"] = -0.6f;
    sentimentLexicon_["崩溃"] = -0.85f;
    // 扩充中文负面词
    sentimentLexicon_["沮丧"] = -0.7f;
    sentimentLexicon_["悲伤"] = -0.75f;
    sentimentLexicon_["心痛"] = -0.75f;
    sentimentLexicon_["心碎"] = -0.8f;
    sentimentLexicon_["无助"] = -0.75f;
    sentimentLexicon_["寂寞"] = -0.6f;
    sentimentLexicon_["空虚"] = -0.6f;
    sentimentLexicon_["心酸"] = -0.65f;
    sentimentLexicon_["煎熬"] = -0.75f;
    sentimentLexicon_["折磨"] = -0.8f;
    sentimentLexicon_["难受"] = -0.7f;
    sentimentLexicon_["郁闷"] = -0.65f;
    sentimentLexicon_["无语"] = -0.5f;
    sentimentLexicon_["心累"] = -0.65f;
    sentimentLexicon_["emo"] = -0.6f;
    sentimentLexicon_["破防"] = -0.7f;
    sentimentLexicon_["裂开"] = -0.6f;
    sentimentLexicon_["麻了"] = -0.55f;
    sentimentLexicon_["无聊"] = -0.4f;
    sentimentLexicon_["烦死"] = -0.75f;
    sentimentLexicon_["受不了"] = -0.7f;
    sentimentLexicon_["忍无可忍"] = -0.8f;
    sentimentLexicon_["恼火"] = -0.65f;
    sentimentLexicon_["气愤"] = -0.7f;
    sentimentLexicon_["不爽"] = -0.6f;
    sentimentLexicon_["火大"] = -0.7f;
    sentimentLexicon_["暴怒"] = -0.85f;
    sentimentLexicon_["担心"] = -0.5f;
    sentimentLexicon_["不安"] = -0.55f;
    sentimentLexicon_["紧张"] = -0.5f;
    sentimentLexicon_["忧虑"] = -0.55f;
    sentimentLexicon_["慌"] = -0.55f;
    sentimentLexicon_["忐忑"] = -0.5f;
    sentimentLexicon_["迷茫"] = -0.5f;
    sentimentLexicon_["困惑"] = -0.45f;
    sentimentLexicon_["纠结"] = -0.45f;
    sentimentLexicon_["压力"] = -0.55f;
    sentimentLexicon_["疲惫"] = -0.55f;
    sentimentLexicon_["厌倦"] = -0.6f;
    sentimentLexicon_["后悔"] = -0.65f;
    sentimentLexicon_["遗憾"] = -0.55f;
    sentimentLexicon_["可惜"] = -0.45f;
    sentimentLexicon_["尴尬"] = -0.4f;
    sentimentLexicon_["丢脸"] = -0.55f;
    sentimentLexicon_["羞耻"] = -0.6f;
    sentimentLexicon_["嫉妒"] = -0.55f;
    sentimentLexicon_["悲催"] = -0.7f;
    sentimentLexicon_["惨"] = -0.7f;
    sentimentLexicon_["糟糕"] = -0.6f;
    sentimentLexicon_["倒霉"] = -0.6f;
    sentimentLexicon_["绷不住"] = -0.65f;
    sentimentLexicon_["窒息"] = -0.75f;
    sentimentLexicon_["社死"] = -0.6f;
    sentimentLexicon_["摆烂"] = -0.5f;
    sentimentLexicon_["失眠"] = -0.7f;
    sentimentLexicon_["睡不着"] = -0.75f;
    sentimentLexicon_["睡不好"] = -0.65f;
    sentimentLexicon_["睡不醒"] = -0.5f;
    sentimentLexicon_["难入睡"] = -0.75f;
    sentimentLexicon_["夜醒"] = -0.65f;
    sentimentLexicon_["浅眠"] = -0.5f;
    sentimentLexicon_["多梦"] = -0.45f;
    sentimentLexicon_["熬夜"] = -0.45f;
    sentimentLexicon_["胸闷"] = -0.7f;
    sentimentLexicon_["心慌"] = -0.75f;
    sentimentLexicon_["压抑"] = -0.75f;
    sentimentLexicon_["透不过气"] = -0.85f;
    sentimentLexicon_["喘不过气"] = -0.85f;
    sentimentLexicon_["提不起劲"] = -0.65f;
    sentimentLexicon_["提不起精神"] = -0.7f;
    sentimentLexicon_["精疲力尽"] = -0.75f;
    sentimentLexicon_["焦躁"] = -0.65f;
    sentimentLexicon_["崩了"] = -0.8f;
    sentimentLexicon_["崩不住了"] = -0.85f;
    sentimentLexicon_["快撑不住了"] = -0.9f;
    sentimentLexicon_["压力山大"] = -0.7f;
    sentimentLexicon_["没意义"] = -0.8f;
    sentimentLexicon_["很累"] = -0.55f;
    sentimentLexicon_["睡眠"] = -0.2f;

    // 英文负面补充
    sentimentLexicon_["hopeless"] = -0.85f;
    sentimentLexicon_["heartbroken"] = -0.8f;
    sentimentLexicon_["devastated"] = -0.9f;
    sentimentLexicon_["overwhelmed"] = -0.6f;
    sentimentLexicon_["exhausted"] = -0.55f;
    sentimentLexicon_["helpless"] = -0.75f;
    sentimentLexicon_["insomnia"] = -0.75f;
    sentimentLexicon_["sleepless"] = -0.7f;
    sentimentLexicon_["panic"] = -0.8f;
    sentimentLexicon_["burnout"] = -0.75f;
    sentimentLexicon_["drained"] = -0.6f;
    sentimentLexicon_["numb"] = -0.55f;
    sentimentLexicon_["失望"] = -0.65f;
    sentimentLexicon_["烦"] = -0.5f;
    sentimentLexicon_["累"] = -0.4f;
    sentimentLexicon_["孤独"] = -0.6f;

    // 程度副词（乘数因子）
    intensifiers_["very"] = 1.5f;
    intensifiers_["extremely"] = 2.0f;
    intensifiers_["really"] = 1.4f;
    intensifiers_["so"] = 1.3f;
    intensifiers_["quite"] = 1.2f;
    intensifiers_["absolutely"] = 1.8f;
    intensifiers_["incredibly"] = 1.7f;
    intensifiers_["slightly"] = 0.6f;
    intensifiers_["somewhat"] = 0.7f;
    intensifiers_["a bit"] = 0.5f;
    intensifiers_["非常"] = 1.5f;
    intensifiers_["特别"] = 1.6f;
    intensifiers_["超"] = 1.5f;
    intensifiers_["太"] = 1.5f;
    intensifiers_["极其"] = 1.8f;
    intensifiers_["有点"] = 0.6f;
    intensifiers_["稍微"] = 0.5f;
    intensifiers_["真"] = 1.3f;
    intensifiers_["好"] = 1.2f;
    // 扩充中文程度副词
    intensifiers_["十分"] = 1.5f;
    intensifiers_["格外"] = 1.5f;
    intensifiers_["尤其"] = 1.4f;
    intensifiers_["相当"] = 1.4f;
    intensifiers_["挺"] = 1.2f;
    intensifiers_["蛮"] = 1.1f;
    intensifiers_["贼"] = 1.5f;
    intensifiers_["巨"] = 1.6f;
    intensifiers_["狠"] = 1.5f;
    intensifiers_["死了"] = 1.7f;
    intensifiers_["要命"] = 1.6f;
    intensifiers_["透了"] = 1.5f;
    intensifiers_["极了"] = 1.7f;

    // 否定词（翻转因子）
    negators_["not"] = -1.0f;
    negators_["no"] = -1.0f;
    negators_["never"] = -1.0f;
    negators_["neither"] = -1.0f;
    negators_["don't"] = -1.0f;
    negators_["doesn't"] = -1.0f;
    negators_["didn't"] = -1.0f;
    negators_["won't"] = -1.0f;
    negators_["wouldn't"] = -1.0f;
    negators_["couldn't"] = -1.0f;
    negators_["shouldn't"] = -1.0f;
    negators_["isn't"] = -1.0f;
    negators_["aren't"] = -1.0f;
    negators_["wasn't"] = -1.0f;
    negators_["不"] = -1.0f;
    negators_["没"] = -1.0f;
    negators_["没有"] = -1.0f;
    negators_["别"] = -1.0f;
    negators_["未"] = -1.0f;
    negators_["无"] = -1.0f;
    // 扩充中文否定词
    negators_["非"] = -1.0f;
    negators_["莫"] = -1.0f;
    negators_["勿"] = -1.0f;
    negators_["毫不"] = -1.0f;
    negators_["并非"] = -1.0f;
    negators_["绝非"] = -1.0f;
    negators_["从未"] = -1.0f;
    negators_["不是"] = -1.0f;
    negators_["不会"] = -1.0f;
    negators_["不能"] = -1.0f;
    negators_["不想"] = -1.0f;
    negators_["不要"] = -1.0f;

    LOG_INFO << "[EdgeAI] Sentiment lexicon loaded: "
             << sentimentLexicon_.size() << " words, "
             << intensifiers_.size() << " intensifiers, "
             << negators_.size() << " negators";
}

std::vector<std::string> EdgeAIEngine::tokenizeUTF8(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string current;

    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);

        // ASCII字母/数字 -> 累积为英文token
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '\'') {
            current += static_cast<char>(std::tolower(c));
            ++i;
            continue;
        }

        // 遇到非字母数字，先flush当前英文token
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }

        // 判断UTF-8多字节字符（中文等）
        if (c >= 0xC0) {
            int byteCount = 1;
            if ((c & 0xE0) == 0xC0) byteCount = 2;
            else if ((c & 0xF0) == 0xE0) byteCount = 3;
            else if ((c & 0xF8) == 0xF0) byteCount = 4;

            if (i + byteCount <= text.size()) {
                // 每个中文字符作为独立token
                std::string utf8Char = text.substr(i, byteCount);
                tokens.push_back(utf8Char);

                // 同时尝试提取2-6字的中文词组（覆盖“睡不着/提不起精神”等短语）
                size_t j = i + byteCount;
                for (int wordLen = 2; wordLen <= 6 && j < text.size(); ++wordLen) {
                    unsigned char nc = static_cast<unsigned char>(text[j]);
                    if (nc < 0xC0) break;
                    int nb = 1;
                    if ((nc & 0xE0) == 0xC0) nb = 2;
                    else if ((nc & 0xF0) == 0xE0) nb = 3;
                    else if ((nc & 0xF8) == 0xF0) nb = 4;
                    if (j + nb > text.size()) break;

                    std::string compound = text.substr(i, (j + nb) - i);
                    // 只有在词典中存在的组合才加入
                    if (sentimentLexicon_.count(compound) ||
                        intensifiers_.count(compound) ||
                        negators_.count(compound)) {
                        tokens.push_back(compound);
                    }
                    j += nb;
                }
            }
            i += byteCount;
        } else {
            // 空格、标点等跳过
            ++i;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

float EdgeAIEngine::ruleSentiment(const std::string& text) const {
    float score = 0.0f;
    int signals = 0;

    // 表情符号检测
    // 正面表情
    const std::vector<std::string> positiveEmojis = {
        "😊", "😄", "😃", "🥰", "❤", "💕", "👍", "🎉", "✨", "😁",
        "🤗", "💪", "🌟", "😍", "🥳", "💖", "😘", "🙂", "☺", "💯"
    };
    for (const auto& emoji : positiveEmojis) {
        size_t pos = 0;
        while ((pos = text.find(emoji, pos)) != std::string::npos) {
            score += 0.3f;
            ++signals;
            pos += emoji.size();
        }
    }

    // 负面表情
    const std::vector<std::string> negativeEmojis = {
        "😢", "😭", "😡", "😠", "💔", "😞", "😔", "😤", "😰", "😱",
        "🤮", "👎", "😩", "😫", "😣", "😖", "😿", "🥺", "😥", "😓"
    };
    for (const auto& emoji : negativeEmojis) {
        size_t pos = 0;
        while ((pos = text.find(emoji, pos)) != std::string::npos) {
            score -= 0.3f;
            ++signals;
            pos += emoji.size();
        }
    }

    // 标点模式分析
    int exclamationCount = 0;
    int questionCount = 0;
    int ellipsisCount = 0;
    for (char c : text) {
        if (c == '!') ++exclamationCount;
        if (c == '?') ++questionCount;
    }
    // 中文标点
    {
        size_t pos = 0;
        while ((pos = text.find("！", pos)) != std::string::npos) {
            ++exclamationCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("？", pos)) != std::string::npos) {
            ++questionCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("...", pos)) != std::string::npos) {
            ++ellipsisCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("……", pos)) != std::string::npos) {
            ++ellipsisCount;
            pos += 6;
        }
    }

    // 感叹号增强情感强度
    if (exclamationCount > 0) {
        score += 0.1f * std::min(exclamationCount, 5);
        ++signals;
    }
    // 省略号暗示犹豫/消极
    if (ellipsisCount > 0) {
        score -= 0.1f * std::min(ellipsisCount, 3);
        ++signals;
    }

    // 全大写检测（英文，表示强烈情感）
    int upperCount = 0;
    int letterCount = 0;
    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            ++letterCount;
            if (std::isupper(static_cast<unsigned char>(c))) ++upperCount;
        }
    }
    if (letterCount > 5 && upperCount > letterCount * 0.7) {
        // 全大写 -> 强烈情感，放大现有分数
        score *= 1.5f;
        ++signals;
    }

    // 文本颜文字检测
    const std::vector<std::pair<std::string, float>> kaomoji = {
        {"(^_^)", 0.4f}, {"(^^)", 0.3f}, {"(≧▽≦)", 0.5f}, {"(╥_╥)", -0.5f},
        {"(T_T)", -0.4f}, {"(>_<)", -0.3f}, {":)", 0.3f}, {":(", -0.3f},
        {":D", 0.4f}, {";)", 0.2f}, {"XD", 0.4f}, {"orz", -0.3f},
        {"QAQ", -0.4f}, {"TAT", -0.4f}, {"^_^", 0.3f}, {">_<", -0.3f}
    };
    for (const auto& [pattern, val] : kaomoji) {
        if (text.find(pattern) != std::string::npos) {
            score += val;
            ++signals;
        }
    }

    if (signals == 0) return 0.0f;
    return std::clamp(score / std::max(1, signals) * 1.5f, -1.0f, 1.0f);
}

float EdgeAIEngine::lexiconSentiment(const std::vector<std::string>& tokens) const {
    float totalScore = 0.0f;
    int matchedCount = 0;
    float intensifierMult = 1.0f;
    int negationWindow = 0;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];

        // 检查是否为否定词
        auto negIt = negators_.find(token);
        if (negIt != negators_.end()) {
            // 否定词仅影响后续少量 token，避免中文短语中的“不”污染整句。
            negationWindow = 2;
            continue;
        }

        // 检查是否为程度副词
        auto intIt = intensifiers_.find(token);
        if (intIt != intensifiers_.end()) {
            intensifierMult = intIt->second;
            continue;
        }

        // 查找情感词典
        auto lexIt = sentimentLexicon_.find(token);
        if (lexIt != sentimentLexicon_.end()) {
            float wordScore = lexIt->second * intensifierMult;
            if (negationWindow > 0) {
                wordScore *= -0.75f;  // 否定翻转，但强度略减
            }
            totalScore += wordScore;
            ++matchedCount;

            // 重置修饰状态
            intensifierMult = 1.0f;
            negationWindow = 0;
        } else {
            // 非情感词时，否定词作用窗口逐步衰减，避免跨短语误翻转。
            if (negationWindow > 0) {
                --negationWindow;
            }
        }
    }

    if (matchedCount == 0) return 0.0f;
    // 线性归一化，避免 sqrt 过度稀释多情感词文本
    return std::clamp(totalScore / static_cast<float>(matchedCount), -1.0f, 1.0f);
}

float EdgeAIEngine::statisticalSentiment(const std::vector<std::string>& tokens,
                                          const std::string& text) const {
    // 基于文本统计特征的简单情感分类
    float score = 0.0f;
    int features = 0;

    // 特征1: 词汇丰富度（type-token ratio）
    std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
    float ttr = tokens.empty() ? 0.0f :
                static_cast<float>(uniqueTokens.size()) / static_cast<float>(tokens.size());
    // 高丰富度略偏正面（表达丰富）
    if (ttr > 0.8f) { score += 0.1f; ++features; }

    // 特征2: 平均词长
    float avgLen = 0.0f;
    for (const auto& t : tokens) avgLen += static_cast<float>(t.size());
    if (!tokens.empty()) avgLen /= static_cast<float>(tokens.size());
    // 较长的词汇使用暗示更正式/理性的表达
    if (avgLen > 6.0f) { score += 0.05f; ++features; }

    // 特征3: 文本长度特征
    float textLen = static_cast<float>(text.size());
    // 极短文本可能是情绪化表达
    if (textLen < 10.0f) {
        // 短文本情感倾向不确定，不加分
        ++features;
    } else if (textLen > 200.0f) {
        // 长文本通常更理性
        score += 0.05f;
        ++features;
    }

    // 特征4: 正面/负面词比例
    int posCount = 0, negCount = 0;
    for (const auto& t : tokens) {
        auto it = sentimentLexicon_.find(t);
        if (it != sentimentLexicon_.end()) {
            if (it->second > 0) ++posCount;
            else if (it->second < 0) ++negCount;
        }
    }
    int sentimentWords = posCount + negCount;
    if (sentimentWords > 0) {
        float posRatio = static_cast<float>(posCount) / static_cast<float>(sentimentWords);
        score += (posRatio - 0.5f) * 0.6f;
        ++features;
    }

    // 特征5: 重复字符检测（如"哈哈哈哈"、"啊啊啊"）
    int repeatCount = 0;
    for (size_t i = 1; i < text.size(); ++i) {
        if (text[i] == text[i - 1]) ++repeatCount;
    }
    float repeatRatio = text.empty() ? 0.0f :
                        static_cast<float>(repeatCount) / static_cast<float>(text.size());
    if (repeatRatio > 0.3f) {
        // 大量重复可能是强烈情感（正面如"哈哈哈"或负面如"呜呜呜"）
        score *= 1.3f;
        ++features;
    }

    if (features == 0) return 0.0f;
    return std::clamp(score, -1.0f, 1.0f);
}

std::string EdgeAIEngine::scoresToMood(float score) const {
    if (score > 0.58f) return "happy";
    if (score > 0.25f) return "calm";
    if (score > -0.2f) return "neutral";
    if (score > -0.62f) return "anxious";
    return "sad";
}

std::string EdgeAIEngine::normalizeSentimentText(const std::string& text) const {
    std::string normalized;
    normalized.reserve(text.size());

    bool previousSpace = true;
    for (unsigned char ch : text) {
        if (ch < 0x80) {
            if (std::isspace(ch)) {
                if (!previousSpace) {
                    normalized.push_back(' ');
                }
                previousSpace = true;
                continue;
            }
            normalized.push_back(static_cast<char>(std::tolower(ch)));
            previousSpace = false;
            continue;
        }
        normalized.push_back(static_cast<char>(ch));
        previousSpace = false;
    }

    while (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }

    return normalized;
}

bool EdgeAIEngine::getSentimentCacheHit(const std::string& key, EdgeSentimentResult& result) {
    if (key.empty()) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    {
        std::shared_lock<std::shared_mutex> lock(sentimentCacheMutex_);
        auto it = sentimentCache_.find(key);
        if (it != sentimentCache_.end() && it->second.expiresAt > now) {
            result = it->second.result;
            sentimentCacheHits_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }

    {
        std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
        auto it = sentimentCache_.find(key);
        if (it != sentimentCache_.end() && it->second.expiresAt <= now) {
            sentimentCache_.erase(it);
        }
    }

    sentimentCacheMisses_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

void EdgeAIEngine::compactSentimentCacheLocked(std::unique_lock<std::shared_mutex>& lock) {
    (void)lock;
    const auto now = std::chrono::steady_clock::now();
    for (auto it = sentimentCache_.begin(); it != sentimentCache_.end();) {
        if (it->second.expiresAt <= now) {
            it = sentimentCache_.erase(it);
            continue;
        }
        ++it;
    }

    while (sentimentCache_.size() > sentimentCacheMaxSize_) {
        auto victim = sentimentCache_.begin();
        for (auto it = sentimentCache_.begin(); it != sentimentCache_.end(); ++it) {
            if (it->second.lastAccessTick < victim->second.lastAccessTick) {
                victim = it;
            }
        }
        if (victim == sentimentCache_.end()) {
            break;
        }
        sentimentCache_.erase(victim);
    }
}

void EdgeAIEngine::putSentimentCache(const std::string& key, const EdgeSentimentResult& result) {
    if (key.empty() || sentimentCacheMaxSize_ == 0) {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
    SentimentCacheEntry entry;
    entry.result = result;
    entry.expiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(sentimentCacheTTLSeconds_);
    entry.lastAccessTick = sentimentCacheTick_.fetch_add(1, std::memory_order_relaxed) + 1;
    sentimentCache_[key] = std::move(entry);
    compactSentimentCacheLocked(lock);
}

EdgeSentimentResult EdgeAIEngine::analyzeSentimentLocal(const std::string& text) {
    ++totalSentimentCalls_;

    if (!isEnabled() || text.empty()) {
        return {0.0f, "neutral", 0.0f, "disabled"};
    }

    const std::string cacheKey = normalizeSentimentText(text);
    if (cacheKey.empty()) {
        return analyzeSentimentLocalUncached(text);
    }

    EdgeSentimentResult cached;
    if (getSentimentCacheHit(cacheKey, cached)) {
        return cached;
    }

    std::shared_future<EdgeSentimentResult> sharedFuture;
    std::optional<std::promise<EdgeSentimentResult>> ownerPromise;
    {
        std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
        const auto now = std::chrono::steady_clock::now();
        auto cacheIt = sentimentCache_.find(cacheKey);
        if (cacheIt != sentimentCache_.end()) {
            if (cacheIt->second.expiresAt > now) {
                cacheIt->second.lastAccessTick = sentimentCacheTick_.fetch_add(1, std::memory_order_relaxed) + 1;
                sentimentCacheHits_.fetch_add(1, std::memory_order_relaxed);
                return cacheIt->second.result;
            }
            sentimentCache_.erase(cacheIt);
        }

        auto inflightIt = sentimentInFlight_.find(cacheKey);
        if (inflightIt != sentimentInFlight_.end()) {
            sharedFuture = inflightIt->second;
        } else {
            ownerPromise.emplace();
            sharedFuture = ownerPromise->get_future().share();
            sentimentInFlight_[cacheKey] = sharedFuture;
        }
    }

    if (!ownerPromise.has_value()) {
        try {
            return sharedFuture.get();
        } catch (...) {
            // 仅在协同请求异常时回退，避免因为单个promise失败导致整体不可用。
            return analyzeSentimentLocalUncached(text);
        }
    }

    try {
        auto computed = analyzeSentimentLocalUncached(text);
        putSentimentCache(cacheKey, computed);
        {
            std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
            sentimentInFlight_.erase(cacheKey);
        }
        ownerPromise->set_value(computed);
        return computed;
    } catch (...) {
        {
            std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);
            sentimentInFlight_.erase(cacheKey);
        }
        ownerPromise->set_exception(std::current_exception());
        throw;
    }
}

EdgeSentimentResult EdgeAIEngine::analyzeSentimentLocalUncached(const std::string& text) {
    if (!isEnabled() || text.empty()) {
        return {0.0f, "neutral", 0.0f, "disabled"};
    }

    auto tokens = tokenizeUTF8(text);

    float ruleScore = ruleSentiment(text);
    float lexScore = lexiconSentiment(tokens);
    float statScore = statisticalSentiment(tokens, text);

    const float wRule = 0.30f;
    const float wLex = 0.60f;
    const float wStat = 0.10f;
    float ensembleScore = wRule * ruleScore + wLex * lexScore + wStat * statScore;
    ensembleScore = std::clamp(ensembleScore, -1.0f, 1.0f);

    auto applyContrastBoost = [&](float baseScore) {
        static const std::vector<std::string> contrastMarkers = {
            "但是", "但", "不过", "然而", "可是", "只是"
        };
        size_t markerPos = std::string::npos;
        size_t markerLen = 0;
        for (const auto& marker : contrastMarkers) {
            size_t pos = text.rfind(marker);
            if (pos != std::string::npos && (markerPos == std::string::npos || pos > markerPos)) {
                markerPos = pos;
                markerLen = marker.size();
            }
        }
        if (markerPos == std::string::npos || markerPos + markerLen >= text.size()) {
            return baseScore;
        }

        std::string tailText = text.substr(markerPos + markerLen);
        if (tailText.empty()) {
            return baseScore;
        }

        auto tailTokens = tokenizeUTF8(tailText);
        const float tailRule = ruleSentiment(tailText);
        const float tailLex = lexiconSentiment(tailTokens);
        const float tailStat = statisticalSentiment(tailTokens, tailText);
        float tailScore = wRule * tailRule + wLex * tailLex + wStat * tailStat;
        tailScore = std::clamp(tailScore, -1.0f, 1.0f);

        if (std::abs(tailLex) < 0.12f && std::abs(tailScore) < 0.18f) {
            return baseScore;
        }

        if (baseScore * tailScore < -0.06f) {
            return std::clamp(baseScore * 0.25f + tailScore * 0.75f, -1.0f, 1.0f);
        }
        return std::clamp(baseScore * 0.45f + tailScore * 0.55f, -1.0f, 1.0f);
    };
    ensembleScore = applyContrastBoost(ensembleScore);

    auto applyPraiseBoost = [&](float baseScore) {
        static const std::vector<std::string> praiseMarkers = {
            "夸", "夸奖", "表扬", "认可", "肯定", "称赞", "赞扬", "赞许", "嘉奖", "鼓励", "被夸", "被表扬"
        };
        static const std::vector<std::string> selfMarkers = {"我", "自己", "本人"};
        static const std::vector<std::string> contrastMarkers = {"但是", "但", "不过", "然而", "可是", "只是"};
        static const std::vector<std::string> negativeMarkers = {
            "焦虑", "担心", "不安", "难过", "伤心", "害怕", "恐惧", "压力", "烦躁", "痛苦", "绝望", "崩溃", "失眠"
        };

        bool hasPraise = false;
        for (const auto& marker : praiseMarkers) {
            if (text.find(marker) != std::string::npos) {
                hasPraise = true;
                break;
            }
        }
        if (!hasPraise) {
            return baseScore;
        }

        bool hasSelf = false;
        for (const auto& marker : selfMarkers) {
            if (text.find(marker) != std::string::npos) {
                hasSelf = true;
                break;
            }
        }
        if (!hasSelf) {
            return baseScore;
        }

        for (const auto& marker : contrastMarkers) {
            if (text.find(marker) != std::string::npos) {
                return baseScore;
            }
        }
        for (const auto& marker : negativeMarkers) {
            if (text.find(marker) != std::string::npos) {
                return baseScore;
            }
        }
        if (ruleScore < -0.2f || lexScore < -0.12f) {
            return baseScore;
        }
        return std::max(baseScore, 0.64f);
    };
    ensembleScore = applyPraiseBoost(ensembleScore);

    auto applyPositiveEventBoost = [&](float baseScore) {
        static const std::vector<std::string> positiveEventMarkers = {
            "收到", "收到了", "礼物", "礼物很好看", "表扬", "夸奖", "夸了我", "夸我", "老师夸", "老师表扬",
            "认可", "肯定", "赞扬", "通过", "成功", "晋级", "被录取", "拿到", "获奖", "中奖", "惊喜", "感谢", "感恩"
        };
        static const std::vector<std::string> negativeMarkers = {
            "焦虑", "担心", "不安", "难过", "伤心", "害怕", "恐惧", "压力", "烦躁",
            "痛苦", "绝望", "崩溃", "失眠", "失败", "糟糕", "后悔"
        };
        static const std::vector<std::string> contrastMarkers = {
            "但是", "但", "不过", "然而", "可是", "只是"
        };

        if (!containsAnyPhrase(text, positiveEventMarkers)) {
            return baseScore;
        }
        if (containsAnyPhrase(text, negativeMarkers)) {
            return baseScore;
        }

        for (const auto& marker : contrastMarkers) {
            const auto pos = text.rfind(marker);
            if (pos == std::string::npos) continue;
            if (pos + marker.size() >= text.size()) continue;
            const auto tail = text.substr(pos + marker.size());
            if (containsAnyPhrase(tail, negativeMarkers)) {
                return baseScore;
            }
        }

        // 正向事件语义在无负向上下文时，最低提升到开心区间，避免误判为焦虑/平静。
        const float floorScore = text.find("礼物") != std::string::npos ? 0.74f : 0.68f;
        return std::max(baseScore, floorScore);
    };
    ensembleScore = applyPositiveEventBoost(ensembleScore);

    const auto anchorSignal = computeAffectiveAnchor(text);
    if (anchorSignal.strength > 0.08f) {
        const float anchorWeight = std::clamp(0.16f + anchorSignal.strength * 0.24f, 0.16f, 0.40f);
        ensembleScore = std::clamp(
            ensembleScore * (1.0f - anchorWeight) + anchorSignal.score * anchorWeight,
            -1.0f,
            1.0f
        );
    }

    float agreement = 1.0f;
    bool allPositive = (ruleScore >= 0 && lexScore >= 0 && statScore >= 0);
    bool allNegative = (ruleScore <= 0 && lexScore <= 0 && statScore <= 0);
    if (allPositive || allNegative) {
        agreement = 0.8f + 0.2f * std::min({std::abs(ruleScore), std::abs(lexScore), std::abs(statScore)});
    } else {
        float variance = ((ruleScore - ensembleScore) * (ruleScore - ensembleScore) +
                          (lexScore - ensembleScore) * (lexScore - ensembleScore) +
                          (statScore - ensembleScore) * (statScore - ensembleScore)) / 3.0f;
        agreement = std::max(0.2f, 0.7f - variance);
    }

    // 词典匹配数也影响置信度
    int lexMatches = 0;
    for (const auto& t : tokens) {
        if (sentimentLexicon_.count(t)) ++lexMatches;
    }
    float coverageBoost = std::min(0.2f, static_cast<float>(lexMatches) * 0.05f);
    float confidence = std::clamp(agreement + coverageBoost, 0.0f, 1.0f);
    if (anchorSignal.strength > 0.10f) {
        confidence = std::clamp(confidence * 0.82f + anchorSignal.strength * 0.18f, 0.0f, 1.0f);
    }
    if (anchorSignal.conflict > 0.42f && std::abs(ensembleScore) < 0.55f) {
        const float penalty = std::clamp((anchorSignal.conflict - 0.42f) * 0.45f, 0.0f, 0.22f);
        confidence = std::max(0.10f, confidence - penalty);
    }

    static const std::vector<std::string> positiveEventHints = {
        "收到", "收到了", "礼物", "礼物很好看", "表扬", "夸奖", "夸了我", "夸我", "老师夸", "老师表扬", "认可", "肯定", "赞扬",
        "通过", "成功", "晋级", "被录取", "拿到", "获奖", "中奖", "惊喜"
    };
    static const std::vector<std::string> negativeEventHints = {
        "焦虑", "担心", "不安", "难过", "伤心", "害怕", "恐惧", "压力", "烦躁", "痛苦", "绝望", "崩溃"
    };
    const bool positiveEventSignal =
        (containsAnyPhrase(text, positiveEventHints) && !containsAnyPhrase(text, negativeEventHints))
        || (anchorSignal.score > 0.35f && anchorSignal.strength > 0.18f);

    auto finalizeSentiment = [&](float rawScore, float rawConfidence, const char* methodName) {
        const std::string mood = refineMoodFromCues(text, rawScore, scoresToMood(rawScore));
        const float calibratedScore = calibrateScoreByMoodCue(text, mood, rawScore);
        return EdgeSentimentResult{calibratedScore, mood, rawConfidence, methodName};
    };

#ifdef HEARTLAKE_USE_ONNX
    if (onnxEnabled_ && onnxEngine_ && onnxEngine_->isInitialized()) {
        auto onnxResult = onnxEngine_->analyze(text);
        const float scoreGap = std::abs(onnxResult.score - ensembleScore);
        const bool signConflict = (onnxResult.score * ensembleScore < -0.06f);
        const bool lexSignalStrong = (lexMatches > 0 && std::abs(lexScore) >= 0.18f);

        if (signConflict && lexSignalStrong && scoreGap >= 0.45f) {
            const float ensembleWeight = positiveEventSignal ? 0.90f : 0.75f;
            const float onnxWeightGuarded = 1.0f - ensembleWeight;
            const float fusedScore = std::clamp(
                onnxResult.score * onnxWeightGuarded + ensembleScore * ensembleWeight,
                -1.0f,
                1.0f
            );
            const float fusedConf = std::clamp((onnxResult.confidence * 0.40f + confidence * 0.60f) - 0.10f, 0.15f, 0.95f);
            return finalizeSentiment(fusedScore, fusedConf, "onnx_guarded_ensemble");
        }

        float onnxWeight = std::clamp(0.35f + onnxResult.confidence * 0.35f, 0.35f, 0.75f);
        if (lexSignalStrong) {
            onnxWeight = std::max(0.25f, onnxWeight - 0.10f);
        }
        if (anchorSignal.strength > 0.20f && std::abs(anchorSignal.score) > 0.32f) {
            const bool anchorConflictWithOnnx = (onnxResult.score * anchorSignal.score < -0.08f);
            if (anchorConflictWithOnnx) {
                onnxWeight = std::max(0.20f, onnxWeight - 0.18f);
            } else {
                onnxWeight = std::min(0.88f, onnxWeight + 0.06f);
            }
        }
        if (positiveEventSignal && ensembleScore > 0.45f && onnxResult.score < -0.10f) {
            // 正向事件语义明确时，避免ONNX异常负向结果主导最终判断。
            onnxWeight = std::min(onnxWeight, 0.18f);
        }
        if (scoreGap < 0.20f) {
            onnxWeight = std::min(0.82f, onnxWeight + 0.08f);
        }

        const float fusedScore = std::clamp(onnxResult.score * onnxWeight + ensembleScore * (1.0f - onnxWeight), -1.0f, 1.0f);
        const float fusedConf = std::clamp((onnxResult.confidence * 0.55f + confidence * 0.45f) - scoreGap * 0.15f, 0.10f, 0.98f);
        return finalizeSentiment(fusedScore, fusedConf, "onnx_ensemble");
    }
#endif

    return finalizeSentiment(ensembleScore, confidence, "ensemble");
}

// ============================================================================
// 子系统2: 本地文本审核（AC自动机 + 语义规则）
// ============================================================================

void EdgeAIEngine::buildModerationAC() {
    std::lock_guard<std::mutex> lock(moderationMutex_);

    // category: 1=self_harm, 2=violence, 3=sexual, 0=general_profanity
    // level: 1=low, 2=medium, 3=high

    // 自伤/自杀相关 (category=1, high risk)
    moderationAC_.addPattern("suicide", 1, 3);
    moderationAC_.addPattern("kill myself", 1, 3);
    moderationAC_.addPattern("end my life", 1, 3);
    moderationAC_.addPattern("self harm", 1, 3);
    moderationAC_.addPattern("want to die", 1, 3);
    moderationAC_.addPattern("自杀", 1, 3);
    moderationAC_.addPattern("自残", 1, 3);
    moderationAC_.addPattern("不想活", 1, 3);
    moderationAC_.addPattern("轻生", 1, 3);
    moderationAC_.addPattern("结束生命", 1, 3);

    // 暴力相关 (category=2)
    moderationAC_.addPattern("murder", 2, 3);
    moderationAC_.addPattern("kill you", 2, 3);
    moderationAC_.addPattern("beat you up", 2, 2);
    moderationAC_.addPattern("violence", 2, 2);
    moderationAC_.addPattern("attack", 2, 1);
    moderationAC_.addPattern("杀", 2, 2);
    moderationAC_.addPattern("打死", 2, 3);
    moderationAC_.addPattern("暴力", 2, 2);
    moderationAC_.addPattern("殴打", 2, 2);

    // 不当内容 (category=3)
    moderationAC_.addPattern("porn", 3, 3);
    moderationAC_.addPattern("nude", 3, 2);
    moderationAC_.addPattern("sexual", 3, 2);
    moderationAC_.addPattern("色情", 3, 3);
    moderationAC_.addPattern("裸体", 3, 2);

    // 一般脏话/侮辱 (category=0)
    moderationAC_.addPattern("fuck", 0, 2);
    moderationAC_.addPattern("shit", 0, 1);
    moderationAC_.addPattern("damn", 0, 1);
    moderationAC_.addPattern("idiot", 0, 1);
    moderationAC_.addPattern("stupid", 0, 1);
    moderationAC_.addPattern("傻逼", 0, 2);
    moderationAC_.addPattern("操", 0, 2);
    moderationAC_.addPattern("垃圾", 0, 1);
    moderationAC_.addPattern("白痴", 0, 1);

    moderationAC_.build();
    moderationACBuilt_ = true;

    LOG_INFO << "[EdgeAI] Moderation AC automaton built successfully";
}

float EdgeAIEngine::semanticRiskAnalysis(const std::string& text,
                                          const std::vector<perf::ACAutomaton::Match>& matches) const {
    if (matches.empty()) return 0.0f;

    float maxRisk = 0.0f;
    float totalRisk = 0.0f;

    for (const auto& m : matches) {
        float baseRisk = 0.0f;
        switch (m.level) {
            case 1: baseRisk = 0.3f; break;
            case 2: baseRisk = 0.6f; break;
            case 3: baseRisk = 0.9f; break;
            default: baseRisk = 0.2f; break;
        }

        // 自伤类别额外加权
        if (m.category == 1) {
            baseRisk = std::min(1.0f, baseRisk * 1.2f);
        }

        maxRisk = std::max(maxRisk, baseRisk);
        totalRisk += baseRisk;
    }

    // 多个匹配叠加风险
    float densityFactor = 1.0f + std::log2(static_cast<float>(matches.size()));
    float combinedRisk = maxRisk * 0.6f + (totalRisk / static_cast<float>(matches.size())) * 0.4f;
    combinedRisk *= std::min(densityFactor, 2.0f);

    // 文本长度归一化：短文本中出现敏感词风险更高
    float lengthFactor = 1.0f;
    if (text.size() < 50) {
        lengthFactor = 1.3f;
    } else if (text.size() < 200) {
        lengthFactor = 1.1f;
    } else {
        lengthFactor = 0.9f;
    }

    float acRisk = std::clamp(combinedRisk * lengthFactor, 0.0f, 1.0f);

    // 五因子心理风险评估融合
    auto& pra = utils::PsychologicalRiskAssessment::getInstance();
    auto praResult = pra.assessRisk(text, "", 0.0f, "");
    float fiveFactorScore = praResult.overallScore;

    // 融合策略：取AC匹配风险和五因子风险的最大值
    // 确保任一维度的高风险都不会被低估
    return std::clamp(std::max(acRisk, fiveFactorScore), 0.0f, 1.0f);
}

EdgeModerationResult EdgeAIEngine::moderateTextLocal(const std::string& text) {
    ++totalModerationCalls_;

    EdgeModerationResult result;
    result.passed = true;
    result.riskLevel = "safe";
    result.confidence = 0.9f;
    result.needsAlert = false;
    result.suggestion = "";

    if (!isEnabled() || text.empty()) {
        return result;
    }

    // 转小写用于匹配
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 阶段1: AC自动机快速多模式匹配
    auto matches = moderationAC_.match(lowerText);

    if (matches.empty()) {
        result.passed = true;
        result.riskLevel = "safe";
        result.confidence = 0.85f;
        return result;
    }

    // 收集匹配到的模式和类别
    std::set<std::string> categorySet;
    bool hasSelfHarm = false;
    uint8_t maxLevel = 0;

    for (const auto& m : matches) {
        // 从匹配位置提取模式文本
        result.matchedPatterns.push_back(
            lowerText.substr(std::max(0, m.position - 20), 40));

        switch (m.category) {
            case 0: categorySet.insert("profanity"); break;
            case 1: categorySet.insert("self_harm"); hasSelfHarm = true; break;
            case 2: categorySet.insert("violence"); break;
            case 3: categorySet.insert("sexual"); break;
            default: categorySet.insert("other"); break;
        }
        maxLevel = std::max(maxLevel, m.level);
    }

    result.categories.assign(categorySet.begin(), categorySet.end());

    // 阶段2: 语义风险分析（已融合五因子模型）
    float riskScore = semanticRiskAnalysis(text, matches);

    // 阶段3: 提取五因子详情填充到结果中
    auto& pra = utils::PsychologicalRiskAssessment::getInstance();
    auto praResult = pra.assessRisk(text, "", 0.0f, "");

    FiveFactorRiskDetail fiveFactorDetail;
    fiveFactorDetail.selfHarmIntent = 0.0f;
    fiveFactorDetail.hopelessness = 0.0f;
    fiveFactorDetail.socialIsolation = 0.0f;
    fiveFactorDetail.temporalUrgency = 0.0f;
    fiveFactorDetail.linguisticMarkers = 0.0f;
    fiveFactorDetail.compositeScore = praResult.overallScore;

    // 从 PsychologicalRiskResult 的 factors 中提取各因子分数
    for (const auto& factor : praResult.factors) {
        if (factor.name == "self_harm_intent") {
            fiveFactorDetail.selfHarmIntent = factor.score;
        } else if (factor.name == "hopelessness") {
            fiveFactorDetail.hopelessness = factor.score;
        } else if (factor.name == "social_isolation") {
            fiveFactorDetail.socialIsolation = factor.score;
        } else if (factor.name == "temporal_urgency") {
            fiveFactorDetail.temporalUrgency = factor.score;
        } else if (factor.category == "linguistic") {
            // analyzeLinguisticMarkers 产生的因子
            fiveFactorDetail.linguisticMarkers = std::max(fiveFactorDetail.linguisticMarkers, factor.score);
        }
    }

    result.fiveFactorDetail = fiveFactorDetail;

    // 确定风险等级
    if (riskScore >= 0.8f || maxLevel >= 3) {
        result.riskLevel = "high_risk";
        result.passed = false;
        result.confidence = std::min(0.95f, riskScore);
    } else if (riskScore >= 0.75f) {
        result.riskLevel = "medium_risk";
        result.passed = false;
        result.confidence = std::min(0.85f, riskScore + 0.1f);
    } else {
        result.riskLevel = "low_risk";
        result.passed = true;
        result.confidence = 0.7f;
    }

    // 自伤检测需要紧急关注（AC匹配或五因子模型检出）
    bool fiveFactorSelfHarm = fiveFactorDetail.selfHarmIntent > 0.5f;
    if (hasSelfHarm || fiveFactorSelfHarm || praResult.needsImmediateAttention) {
        result.needsAlert = true;
        result.passed = false;
        result.riskLevel = "high_risk";
        result.suggestion = "Detected potential self-harm content. "
                           "Please provide crisis support resources and escalate to human moderator.";
        if (!praResult.supportMessage.empty()) {
            result.suggestion += " " + praResult.supportMessage;
        }
    } else if (result.riskLevel == "high_risk") {
        result.suggestion = "High-risk content detected. Recommend blocking and review by human moderator.";
    } else if (result.riskLevel == "medium_risk") {
        result.suggestion = "Medium-risk content detected. Consider flagging for review.";
    }

    return result;
}

// ============================================================================
// 子系统3: 实时情绪脉搏检测
// ============================================================================

void EdgeAIEngine::pruneEmotionWindow() {
    auto now = std::chrono::steady_clock::now();
    auto windowDuration = std::chrono::seconds(pulseWindowSeconds_);

    while (!emotionWindow_.empty()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - emotionWindow_.front().timestamp);
        if (age > windowDuration) {
            emotionWindow_.pop_front();
        } else {
            break;
        }
    }
}

EmotionPulse EdgeAIEngine::computePulseFromWindow() const {
    EmotionPulse pulse;
    pulse.timestamp = std::chrono::steady_clock::now();
    pulse.sampleCount = static_cast<int>(emotionWindow_.size());

    if (emotionWindow_.empty()) {
        pulse.avgScore = 0.0f;
        pulse.stddev = 0.0f;
        pulse.trendSlope = 0.0f;
        pulse.dominantMood = "neutral";
        return pulse;
    }

    const auto now = std::chrono::steady_clock::now();
    const float tauSec = std::max(45.0f, static_cast<float>(pulseWindowSeconds_) * 0.45f);

    // 计算时间衰减 + 置信度加权平均分（越新的样本、越高置信度样本权重越高）
    float weightedSum = 0.0f;
    float weightTotal = 0.0f;
    std::unordered_map<std::string, int> moodCounts;
    std::unordered_map<std::string, float> moodWeights;
    for (const auto& s : emotionWindow_) {
        const float ageSec = std::max(
            0.0f,
            static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s.timestamp).count()) / 1000.0f);
        const float confidenceWeight = std::clamp(s.weight, 0.1f, 1.0f);
        const float weight = std::exp(-ageSec / tauSec) * confidenceWeight;
        weightedSum += s.score * weight;
        weightTotal += weight;
        moodCounts[s.mood]++;
        moodWeights[s.mood] += weight;
    }
    pulse.avgScore = weightTotal > 0.0f ? (weightedSum / weightTotal) : 0.0f;
    pulse.moodDistribution = moodCounts;

    // 计算加权标准差
    float varianceSum = 0.0f;
    for (const auto& s : emotionWindow_) {
        const float ageSec = std::max(
            0.0f,
            static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s.timestamp).count()) / 1000.0f);
        const float confidenceWeight = std::clamp(s.weight, 0.1f, 1.0f);
        const float weight = std::exp(-ageSec / tauSec) * confidenceWeight;
        float diff = s.score - pulse.avgScore;
        varianceSum += weight * diff * diff;
    }
    pulse.stddev = std::sqrt(varianceSum / std::max(weightTotal, 1e-6f));

    // 计算趋势斜率（基于真实时间戳 + EWMA平滑 + 时间衰减加权回归，单位: 每分钟）
    if (emotionWindow_.size() >= 4) {
        float sumW = 0.0f, sumWX = 0.0f, sumWY = 0.0f, sumWXY = 0.0f, sumWXX = 0.0f;
        const auto t0 = emotionWindow_.front().timestamp;
        float ewma = emotionWindow_.front().score;
        constexpr float ewmaAlpha = 0.35f;
        for (const auto& sample : emotionWindow_) {
            const float x = std::chrono::duration_cast<std::chrono::seconds>(
                sample.timestamp - t0).count() / 60.0f;
            ewma = ewmaAlpha * sample.score + (1.0f - ewmaAlpha) * ewma;
            const float ageSec = std::max(
                0.0f,
                static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - sample.timestamp).count()) / 1000.0f);
            const float confidenceWeight = std::clamp(sample.weight, 0.1f, 1.0f);
            const float w = std::exp(-ageSec / tauSec) * confidenceWeight;

            sumW += w;
            sumWX += w * x;
            sumWY += w * ewma;
            sumWXY += w * x * ewma;
            sumWXX += w * x * x;
        }
        const float denominator = sumW * sumWXX - sumWX * sumWX;
        if (std::abs(denominator) > 1e-6f) {
            pulse.trendSlope = (sumW * sumWXY - sumWX * sumWY) / denominator;
        } else {
            pulse.trendSlope = 0.0f;
        }
    } else {
        pulse.trendSlope = 0.0f;
    }

    // 找主导情绪
    std::string dominant = "neutral";
    float maxWeight = -1.0f;
    for (const auto& [mood, weight] : moodWeights) {
        if (weight > maxWeight) {
            maxWeight = weight;
            dominant = mood;
        }
    }
    pulse.dominantMood = dominant;

    return pulse;
}

void EdgeAIEngine::submitEmotionSample(float score, const std::string& mood, float confidence) {
    if (!isEnabled()) return;

    std::unique_lock<std::shared_mutex> lock(pulseMutex_);

    float adjustedScore = std::clamp(score, -1.0f, 1.0f);
    // 低置信度样本保留但降权，避免误判显著拉偏脉搏。
    float normalizedConfidence = std::clamp(confidence, 0.0f, 1.0f);
    float sampleWeight = 0.25f + 0.75f * normalizedConfidence;
    // 鲁棒抗噪：使用最近窗口中位数 + MAD 限制离群点，避免脉搏被极端噪声污染。
    if (emotionWindow_.size() >= 6) {
        const size_t recentCount = std::min<size_t>(emotionWindow_.size(), 15);
        std::vector<float> recentScores;
        recentScores.reserve(recentCount);
        const size_t beginIdx = emotionWindow_.size() - recentCount;
        for (size_t i = beginIdx; i < emotionWindow_.size(); ++i) {
            recentScores.push_back(emotionWindow_[i].score);
        }
        const float median = medianValue(recentScores);
        std::vector<float> absDeviation;
        absDeviation.reserve(recentScores.size());
        for (float s : recentScores) {
            absDeviation.push_back(std::abs(s - median));
        }
        const float mad = std::max(0.05f, medianValue(absDeviation));
        // 低置信度样本采用更严格边界。
        const float confidenceTightening = 1.0f - 0.35f * (1.0f - normalizedConfidence);
        const float lower = median - 3.0f * mad * confidenceTightening;
        const float upper = median + 3.0f * mad * confidenceTightening;
        if (adjustedScore < lower || adjustedScore > upper) {
            const float bounded = std::clamp(adjustedScore, lower, upper);
            adjustedScore = 0.75f * bounded + 0.25f * median;
        }
    }

    EmotionSample sample;
    sample.score = adjustedScore;
    sample.mood = mood;
    sample.weight = sampleWeight;
    sample.timestamp = std::chrono::steady_clock::now();

    emotionWindow_.push_back(sample);
    pruneEmotionWindow();

    // 每积累一定样本数，生成一个脉搏快照
    if (emotionWindow_.size() % 10 == 0 && !emotionWindow_.empty()) {
        EmotionPulse pulse = computePulseFromWindow();
        pulseHistory_.push_back(pulse);
        if (static_cast<int>(pulseHistory_.size()) > maxPulseHistory_) {
            pulseHistory_.erase(pulseHistory_.begin());
        }
    }
}

EmotionPulse EdgeAIEngine::getCurrentPulse() {
    std::unique_lock<std::shared_mutex> lock(pulseMutex_);
    pruneEmotionWindow();
    return computePulseFromWindow();
}

std::vector<EmotionPulse> EdgeAIEngine::getPulseHistory(int count) {
    std::shared_lock<std::shared_mutex> lock(pulseMutex_);
    int n = std::min(count, static_cast<int>(pulseHistory_.size()));
    if (n <= 0) return {};
    return std::vector<EmotionPulse>(pulseHistory_.end() - n, pulseHistory_.end());
}

// ============================================================================
// 子系统4: 联邦学习聚合器（FedAvg）
// ============================================================================

void EdgeAIEngine::submitLocalModel(const FederatedModelParams& params) {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    localModels_.push_back(params);
    LOG_DEBUG << "[EdgeAI] Local model submitted from node: " << params.nodeId
              << ", samples: " << params.sampleCount
              << ", loss: " << params.localLoss;
}

FederatedModelParams EdgeAIEngine::aggregateFedAvg(float clippingBound, float noiseSigma) {
    std::lock_guard<std::mutex> lock(federatedMutex_);

    FederatedModelParams global;
    global.modelId = "global_fedavg";
    global.epoch = ++federatedRound_;
    global.nodeId = "aggregator";

    if (localModels_.empty()) {
        LOG_WARN << "[EdgeAI] No local models to aggregate";
        global.sampleCount = 0;
        global.localLoss = 0.0f;
        return global;
    }

    // DP-SGD Step 1: 对每个本地模型的权重进行 ℓ2-norm 梯度裁剪
    if (clippingBound > 0.0f) {
        for (auto& model : localModels_) {
            // 计算该模型所有权重的 ℓ2 范数
            float l2norm = 0.0f;
            for (const auto& layer : model.weights) {
                for (float w : layer) {
                    l2norm += w * w;
                }
            }
            for (float b : model.biases) {
                l2norm += b * b;
            }
            l2norm = std::sqrt(l2norm);

            // 如果范数超过阈值C，按比例缩放: w = w * (C / ||w||)
            if (l2norm > clippingBound) {
                float scale = clippingBound / l2norm;
                for (auto& layer : model.weights) {
                    for (float& w : layer) {
                        w *= scale;
                    }
                }
                for (float& b : model.biases) {
                    b *= scale;
                }
                LOG_DEBUG << "[EdgeAI] Clipped model " << model.nodeId
                          << " from norm=" << l2norm << " to " << clippingBound;
            }
        }
    }

    // 计算总样本数
    size_t totalSamples = 0;
    for (const auto& model : localModels_) {
        totalSamples += model.sampleCount;
    }
    if (totalSamples == 0) {
        LOG_WARN << "[EdgeAI] Total samples is 0, using equal weights";
        totalSamples = localModels_.size();  // 等权重回退
    }

    if (totalSamples == 0) {
        LOG_WARN << "[EdgeAI] Total sample count is zero, cannot aggregate";
        global.sampleCount = 0;
        global.localLoss = 0.0f;
        localModels_.clear();
        return global;
    }

    global.sampleCount = totalSamples;

    // 确定权重矩阵维度（以第一个模型为参考）
    const auto& ref = localModels_[0];
    size_t numLayers = ref.weights.size();
    size_t biasSize = ref.biases.size();

    // 初始化全局权重为零
    global.weights.resize(numLayers);
    for (size_t l = 0; l < numLayers; ++l) {
        global.weights[l].resize(ref.weights[l].size(), 0.0f);
    }
    global.biases.resize(biasSize, 0.0f);
    global.localLoss = 0.0f;

    // FedAvg加权聚合: w_global = Σ(n_k / n) * w_k
    for (const auto& model : localModels_) {
        float weight = static_cast<float>(model.sampleCount) / static_cast<float>(totalSamples);

        // 聚合权重矩阵
        for (size_t l = 0; l < numLayers && l < model.weights.size(); ++l) {
            for (size_t j = 0; j < model.weights[l].size() && j < global.weights[l].size(); ++j) {
                global.weights[l][j] += weight * model.weights[l][j];
            }
        }

        // 聚合偏置
        for (size_t j = 0; j < model.biases.size() && j < global.biases.size(); ++j) {
            global.biases[j] += weight * model.biases[j];
        }

        // 加权平均损失
        global.localLoss += weight * model.localLoss;
    }

    // DP-SGD Step 2: 对聚合后的全局权重添加高斯噪声 N(0, σ²C²I)
    if (noiseSigma > 0.0f && clippingBound > 0.0f) {
        std::random_device rd;
        std::mt19937 gen(rd());
        float noiseScale = noiseSigma * clippingBound;
        std::normal_distribution<float> dist(0.0f, noiseScale);

        for (auto& layer : global.weights) {
            for (float& w : layer) {
                w += dist(gen);
            }
        }
        for (float& b : global.biases) {
            b += dist(gen);
        }

        LOG_INFO << "[EdgeAI] Added Gaussian noise: σ=" << noiseSigma
                 << ", C=" << clippingBound << ", noise_scale=" << noiseScale;
    }

    LOG_INFO << "[EdgeAI] FedAvg aggregation complete: round=" << federatedRound_
             << ", participants=" << localModels_.size()
             << ", totalSamples=" << totalSamples
             << ", avgLoss=" << global.localLoss;

    // 清空本地模型缓存，准备下一轮
    localModels_.clear();

    return global;
}

Json::Value EdgeAIEngine::getFederatedStatus() const {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    Json::Value status;
    status["current_round"] = federatedRound_;
    status["pending_models"] = static_cast<int>(localModels_.size());

    size_t totalSamples = 0;
    Json::Value nodes(Json::arrayValue);
    for (const auto& model : localModels_) {
        Json::Value node;
        node["node_id"] = model.nodeId;
        node["sample_count"] = static_cast<Json::UInt64>(model.sampleCount);
        node["local_loss"] = model.localLoss;
        node["epoch"] = model.epoch;
        nodes.append(node);
        totalSamples += model.sampleCount;
    }
    status["nodes"] = nodes;
    status["total_pending_samples"] = static_cast<Json::UInt64>(totalSamples);

    return status;
}

// ============================================================================
// 子系统5: 差分隐私引擎（Laplace机制）
// ============================================================================

float EdgeAIEngine::sampleLaplace(float scale) {
    // Laplace分布采样：使用逆CDF方法
    // 如果 U ~ Uniform(0,1)，则 X = μ - b * sign(U-0.5) * ln(1 - 2|U-0.5|)
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float u;
    {
        std::lock_guard<std::mutex> lock(dpMutex_);
        u = dist(dpRng_);
    }

    // 避免 log(0)
    u = std::clamp(u, 1e-7f, 1.0f - 1e-7f);

    float centered = u - 0.5f;
    float sign = (centered >= 0.0f) ? 1.0f : -1.0f;
    float absVal = std::abs(centered);

    return -scale * sign * std::log(1.0f - 2.0f * absVal);
}

float EdgeAIEngine::addLaplaceNoise(float value, float sensitivity) {
    if (!isEnabled()) return value;

    // 检查隐私预算
    float remaining = dpConfig_.maxEpsilonBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[EdgeAI] Privacy budget exhausted, returning original value";
        return value;
    }

    // 使用配置的epsilon，但不超过剩余预算
    float epsilon = std::min(dpConfig_.epsilon, remaining);
    if (epsilon < 1e-10f) epsilon = 1e-10f;  // 防止除零

    // Laplace噪声尺度: b = Δf / ε
    float scale = sensitivity / epsilon;
    float noise = sampleLaplace(scale);

    // 消耗隐私预算（组合定理：线性累加）
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    return value + noise;
}

std::vector<float> EdgeAIEngine::addLaplaceNoiseVec(const std::vector<float>& values,
                                                      float sensitivity) {
    if (!isEnabled()) return values;

    float remaining = dpConfig_.maxEpsilonBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[EdgeAI] Privacy budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(dpConfig_.epsilon, remaining);
    if (epsilon < 1e-10f) epsilon = 1e-10f;  // 防止除零
    float scale = sensitivity / epsilon;

    std::vector<float> result(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        result[i] = values[i] + sampleLaplace(scale);
    }

    // 消耗隐私预算
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    return result;
}

float EdgeAIEngine::getRemainingPrivacyBudget() const {
    float consumed = consumedEpsilon_.load();
    return std::max(0.0f, dpConfig_.maxEpsilonBudget - consumed);
}

void EdgeAIEngine::resetPrivacyBudget() {
    consumedEpsilon_.store(0.0f);
    LOG_INFO << "[EdgeAI] Privacy budget reset. Max budget: " << dpConfig_.maxEpsilonBudget;
}

// ============================================================================
// 子系统6: HNSW向量检索
// ============================================================================

int EdgeAIEngine::randomLevel() {
    // 指数衰减的随机层级生成
    // P(level = l) = (1/M)^l * (1 - 1/M)
    // 注意：调用者必须已持有 hnswMutex_
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(hnswRng_);
    int level = static_cast<int>(-std::log(r) * hnswLevelMult_);
    return std::max(0, level);
}

float EdgeAIEngine::vectorDistance(const std::vector<float>& a,
                                    const std::vector<float>& b) const {
    // L2 (欧氏) 距离的平方 — 避免开方以提升性能
    if (a.size() != b.size()) return std::numeric_limits<float>::max();

    float dist = 0.0f;
    size_t dim = a.size();

    // 4路展开减少循环开销
    size_t i = 0;
    for (; i + 3 < dim; i += 4) {
        float d0 = a[i]     - b[i];
        float d1 = a[i + 1] - b[i + 1];
        float d2 = a[i + 2] - b[i + 2];
        float d3 = a[i + 3] - b[i + 3];
        dist += d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3;
    }
    for (; i < dim; ++i) {
        float d = a[i] - b[i];
        dist += d * d;
    }
    return dist;
}

std::vector<size_t> EdgeAIEngine::searchLayer(const std::vector<float>& query,
                                               size_t entryPoint, int ef,
                                               int level) const {
    // 贪心搜索 + 动态候选列表（beam search 变体）
    // 使用 max-heap 维护结果集（最远的在堆顶，方便淘汰）
    // 使用 min-heap 维护候选集（最近的在堆顶，优先扩展）

    if (entryPoint >= hnswNodes_.size()) {
        return {};
    }

    using DistIdx = std::pair<float, size_t>;

    // 结果集：max-heap（距离最大的在顶部）
    std::priority_queue<DistIdx> results;
    // 候选集：min-heap（距离最小的在顶部）
    std::priority_queue<DistIdx, std::vector<DistIdx>, std::greater<DistIdx>> candidates;

    std::unordered_set<size_t> visited;

    float entryDist = vectorDistance(query, hnswNodes_[entryPoint].vector);
    results.push({entryDist, entryPoint});
    candidates.push({entryDist, entryPoint});
    visited.insert(entryPoint);

    while (!candidates.empty()) {
        auto [candDist, candIdx] = candidates.top();
        float worstResult = results.top().first;

        // 如果最近的候选比结果集中最远的还远，停止
        if (candDist > worstResult && static_cast<int>(results.size()) >= ef) {
            break;
        }
        candidates.pop();

        // 扩展该候选的邻居
        if (level < static_cast<int>(hnswNodes_[candIdx].neighbors.size())) {
            for (size_t neighborIdx : hnswNodes_[candIdx].neighbors[level]) {
                if (visited.count(neighborIdx)) continue;
                visited.insert(neighborIdx);

                float neighborDist = vectorDistance(query, hnswNodes_[neighborIdx].vector);
                worstResult = results.top().first;

                if (static_cast<int>(results.size()) < ef || neighborDist < worstResult) {
                    results.push({neighborDist, neighborIdx});
                    candidates.push({neighborDist, neighborIdx});

                    // 保持结果集大小不超过 ef
                    if (static_cast<int>(results.size()) > ef) {
                        results.pop();
                    }
                }
            }
        }
    }

    // 提取结果（按距离从小到大排序）
    std::vector<DistIdx> sorted;
    sorted.reserve(results.size());
    while (!results.empty()) {
        sorted.push_back(results.top());
        results.pop();
    }
    std::sort(sorted.begin(), sorted.end());

    std::vector<size_t> result;
    result.reserve(sorted.size());
    for (const auto& [d, idx] : sorted) {
        result.push_back(idx);
    }
    return result;
}

void EdgeAIEngine::connectNeighbors(size_t nodeIdx,
                                     const std::vector<size_t>& neighbors,
                                     int level, int maxM) {
    // 双向连接：将 nodeIdx 与 neighbors 互相连接
    // 如果邻居数超过 maxM，执行启发式裁剪（保留最近的 maxM 个）

    if (nodeIdx >= hnswNodes_.size() ||
        level < 0 ||
        level >= static_cast<int>(hnswNodes_[nodeIdx].neighbors.size())) {
        return;
    }

    auto& nodeNeighbors = hnswNodes_[nodeIdx].neighbors[level];
    for (size_t neighborIdx : neighbors) {
        // 添加正向连接
        if (std::find(nodeNeighbors.begin(), nodeNeighbors.end(), neighborIdx)
            == nodeNeighbors.end()) {
            nodeNeighbors.push_back(neighborIdx);
        }

        // 添加反向连接
        if (level < static_cast<int>(hnswNodes_[neighborIdx].neighbors.size())) {
            auto& revNeighbors = hnswNodes_[neighborIdx].neighbors[level];
            if (std::find(revNeighbors.begin(), revNeighbors.end(), nodeIdx)
                == revNeighbors.end()) {
                revNeighbors.push_back(nodeIdx);
            }

            // 裁剪：如果反向邻居超过 maxM，保留最近的
            if (static_cast<int>(revNeighbors.size()) > maxM) {
                // 按距离排序
                std::vector<std::pair<float, size_t>> distPairs;
                distPairs.reserve(revNeighbors.size());
                for (size_t nIdx : revNeighbors) {
                    float d = vectorDistance(hnswNodes_[neighborIdx].vector,
                                             hnswNodes_[nIdx].vector);
                    distPairs.push_back({d, nIdx});
                }
                std::sort(distPairs.begin(), distPairs.end());

                revNeighbors.clear();
                for (int i = 0; i < maxM; ++i) {
                    revNeighbors.push_back(distPairs[i].second);
                }
            }
        }
    }

    // 裁剪正向邻居
    if (static_cast<int>(nodeNeighbors.size()) > maxM) {
        std::vector<std::pair<float, size_t>> distPairs;
        distPairs.reserve(nodeNeighbors.size());
        for (size_t nIdx : nodeNeighbors) {
            float d = vectorDistance(hnswNodes_[nodeIdx].vector,
                                     hnswNodes_[nIdx].vector);
            distPairs.push_back({d, nIdx});
        }
        std::sort(distPairs.begin(), distPairs.end());

        nodeNeighbors.clear();
        for (int i = 0; i < maxM; ++i) {
            nodeNeighbors.push_back(distPairs[i].second);
        }
    }
}

void EdgeAIEngine::hnswInsert(const std::string& id, const std::vector<float>& vec) {
    if (!isEnabled()) return;

    std::unique_lock<std::shared_mutex> lock(hnswMutex_);

    if (vec.empty()) {
        LOG_WARN << "[EdgeAI] HNSW: empty vector for id '" << id << "', skipping";
        return;
    }

    // 检查是否已存在
    if (hnswIdMap_.count(id)) {
        LOG_WARN << "[EdgeAI] HNSW: vector '" << id << "' already exists, skipping";
        return;
    }

    if (!hnswNodes_.empty()) {
        const size_t expectedDim = hnswNodes_.front().vector.size();
        if (vec.size() != expectedDim) {
            LOG_WARN << "[EdgeAI] HNSW: dimension mismatch for id '" << id
                     << "', expected=" << expectedDim
                     << ", got=" << vec.size() << ", skipping";
            return;
        }
    }

    int nodeLevel = randomLevel();
    size_t nodeIdx = hnswNodes_.size();

    // 创建新节点
    HNSWNode node;
    node.id = id;
    node.vector = vec;
    node.maxLevel = nodeLevel;
    node.neighbors.resize(nodeLevel + 1);

    hnswNodes_.push_back(std::move(node));
    hnswIdMap_[id] = nodeIdx;

    // 第一个节点：直接设为入口点
    if (hnswNodes_.size() == 1) {
        hnswEntryPoint_ = 0;
        hnswMaxLevel_ = nodeLevel;
        return;
    }

    // 从最高层开始贪心搜索到 nodeLevel+1 层
    size_t currentEntry = hnswEntryPoint_;

    for (int lv = hnswMaxLevel_; lv > nodeLevel; --lv) {
        auto nearest = searchLayer(vec, currentEntry, 1, lv);
        if (!nearest.empty()) {
            currentEntry = nearest[0];
        }
    }

    // 从 min(nodeLevel, hnswMaxLevel_) 层到第0层，搜索并连接邻居
    for (int lv = std::min(nodeLevel, hnswMaxLevel_); lv >= 0; --lv) {
        int efConstruction = hnswEfConstruction_;
        auto neighbors = searchLayer(vec, currentEntry, efConstruction, lv);

        int maxM = (lv == 0) ? hnswMMax0_ : hnswM_;

        // 取最近的 maxM 个作为邻居
        if (static_cast<int>(neighbors.size()) > maxM) {
            neighbors.resize(maxM);
        }

        connectNeighbors(nodeIdx, neighbors, lv, maxM);

        if (!neighbors.empty()) {
            currentEntry = neighbors[0];
        }
    }

    // 更新入口点
    if (nodeLevel > hnswMaxLevel_) {
        hnswMaxLevel_ = nodeLevel;
        hnswEntryPoint_ = nodeIdx;
    }
}

std::vector<VectorSearchResult> EdgeAIEngine::hnswSearch(const std::vector<float>& query,
                                                          int k) {
    if (!isEnabled()) return {};
    if (query.empty()) return {};

    totalHNSWSearches_.fetch_add(1);

    std::shared_lock<std::shared_mutex> lock(hnswMutex_);

    if (hnswNodes_.empty()) return {};
    const size_t expectedDim = hnswNodes_.front().vector.size();
    if (expectedDim == 0 || query.size() != expectedDim) {
        LOG_WARN << "[EdgeAI] HNSW search dimension mismatch: expected="
                 << expectedDim << ", got=" << query.size();
        return {};
    }
    const int requestedK = std::max(1, k);

    // 从最高层贪心搜索到第1层
    size_t currentEntry = hnswEntryPoint_;
    for (int lv = hnswMaxLevel_; lv > 0; --lv) {
        auto nearest = searchLayer(query, currentEntry, 1, lv);
        if (!nearest.empty()) {
            currentEntry = nearest[0];
        }
    }

    // 在第0层执行自适应搜索（Ada-EF思路）：
    // 先用小规模pilot估计难度，再动态调整ef，兼顾召回与延迟。
    int baseEf = std::max(requestedK, hnswEfSearch_);
    int adaptiveEf = baseEf;
    if (hnswNodes_.size() >= 256 && baseEf >= 24) {
        const int pilotEf = std::clamp(std::max(requestedK * 2, 12), 12, std::min(baseEf, 32));
        auto pilotCandidates = searchLayer(query, currentEntry, pilotEf, 0);
        if (pilotCandidates.size() >= 2) {
            const float best = vectorDistance(query, hnswNodes_[pilotCandidates[0]].vector);
            const float secondBest = vectorDistance(query, hnswNodes_[pilotCandidates[1]].vector);
            const float marginRatio = (secondBest - best) / (std::abs(best) + 1e-6f);

            if (marginRatio < 0.08f) {
                adaptiveEf = static_cast<int>(std::ceil(baseEf * 1.6f));
            } else if (marginRatio > 0.20f) {
                adaptiveEf = static_cast<int>(std::floor(baseEf * 0.75f));
            }

            const int entryDegree = (currentEntry < hnswNodes_.size() &&
                                     !hnswNodes_[currentEntry].neighbors.empty())
                ? static_cast<int>(hnswNodes_[currentEntry].neighbors[0].size())
                : 0;
            if (entryDegree >= hnswMMax0_ && marginRatio < 0.12f) {
                adaptiveEf = static_cast<int>(std::ceil(adaptiveEf * 1.2f));
            }
        }
        const int efCap = std::max(96, hnswEfSearch_ * 6);
        adaptiveEf = std::clamp(adaptiveEf, requestedK, efCap);
    }
    auto candidates = searchLayer(query, currentEntry, adaptiveEf, 0);

    // 取 top-k
    if (static_cast<int>(candidates.size()) > requestedK) {
        candidates.resize(requestedK);
    }

    // 构建结果
    std::vector<VectorSearchResult> results;
    results.reserve(candidates.size());

    for (size_t idx : candidates) {
        float dist = vectorDistance(query, hnswNodes_[idx].vector);
        float sqrtDist = std::sqrt(dist);

        VectorSearchResult r;
        r.id = hnswNodes_[idx].id;
        r.distance = sqrtDist;
        // 相似度：使用高斯核映射 sim = exp(-dist / (2 * dim))
        float dim = std::max(1.0f, static_cast<float>(query.size()));
        r.similarity = std::exp(-dist / (2.0f * dim));
        results.push_back(std::move(r));
    }

    return results;
}

std::vector<VectorSearchResult> EdgeAIEngine::rerankHNSWCandidates(
    const std::vector<float>& query,
    const std::vector<VectorSearchResult>& candidates,
    int topK) const {
    if (!isEnabled()) return {};
    if (query.empty() || candidates.empty()) return {};

    std::shared_lock<std::shared_mutex> lock(hnswMutex_);
    if (hnswNodes_.empty()) return {};

    const size_t expectedDim = hnswNodes_.front().vector.size();
    if (expectedDim == 0 || query.size() != expectedDim) {
        return {};
    }

    const int requestedTopK = std::max(1, topK);
    const int coarseFromEnv = parsePositiveIntEnv(std::getenv("MATRYOSHKA_COARSE_DIM"), 64);
    const size_t coarseDim = std::clamp<size_t>(
        static_cast<size_t>(coarseFromEnv), 16, expectedDim);

    auto cosinePrefix = [](const std::vector<float>& a,
                           const std::vector<float>& b,
                           size_t dim) -> float {
        float dot = 0.0f;
        float normA = 0.0f;
        float normB = 0.0f;
        for (size_t i = 0; i < dim; ++i) {
            dot += a[i] * b[i];
            normA += a[i] * a[i];
            normB += b[i] * b[i];
        }
        const float denom = std::sqrt(normA) * std::sqrt(normB);
        return denom > 1e-6f ? dot / denom : 0.0f;
    };

    std::vector<VectorSearchResult> reranked;
    reranked.reserve(candidates.size());

    for (const auto& candidate : candidates) {
        const auto it = hnswIdMap_.find(candidate.id);
        if (it == hnswIdMap_.end()) {
            continue;
        }
        const auto& node = hnswNodes_[it->second];
        if (node.vector.size() != expectedDim) {
            continue;
        }

        const float fullCosine = cosinePrefix(query, node.vector, expectedDim);
        const float coarseCosine = cosinePrefix(query, node.vector, coarseDim);
        const float fusedCosine = std::clamp(
            fullCosine * 0.72f + coarseCosine * 0.28f, -1.0f, 1.0f);
        const float similarity = std::clamp((fusedCosine + 1.0f) * 0.5f, 0.0f, 1.0f);

        VectorSearchResult item;
        item.id = candidate.id;
        item.similarity = similarity;
        item.distance = 1.0f - similarity;
        reranked.push_back(std::move(item));
    }

    std::sort(reranked.begin(), reranked.end(),
              [](const VectorSearchResult& a, const VectorSearchResult& b) {
                  if (a.similarity == b.similarity) {
                      return a.distance < b.distance;
                  }
                  return a.similarity > b.similarity;
              });

    if (static_cast<int>(reranked.size()) > requestedTopK) {
        reranked.resize(requestedTopK);
    }
    return reranked;
}

Json::Value EdgeAIEngine::getHNSWStats() const {
    std::shared_lock<std::shared_mutex> lock(hnswMutex_);

    Json::Value stats;
    stats["total_nodes"] = static_cast<Json::UInt64>(hnswNodes_.size());
    stats["max_level"] = hnswMaxLevel_;
    stats["m"] = hnswM_;
    stats["m_max0"] = hnswMMax0_;
    stats["ef_construction"] = hnswEfConstruction_;
    stats["ef_search"] = hnswEfSearch_;
    stats["total_searches"] = static_cast<Json::UInt64>(totalHNSWSearches_.load());

    // 统计各层节点数
    Json::Value levelCounts(Json::arrayValue);
    std::vector<int> counts(hnswMaxLevel_ + 1, 0);
    for (const auto& node : hnswNodes_) {
        for (int lv = 0; lv <= node.maxLevel; ++lv) {
            if (lv < static_cast<int>(counts.size())) {
                counts[lv]++;
            }
        }
    }
    for (int c : counts) {
        levelCounts.append(c);
    }
    stats["level_node_counts"] = levelCounts;

    // 平均邻居数（第0层）
    if (!hnswNodes_.empty()) {
        double totalNeighbors = 0;
        for (const auto& node : hnswNodes_) {
            if (!node.neighbors.empty()) {
                totalNeighbors += node.neighbors[0].size();
            }
        }
        stats["avg_neighbors_level0"] = totalNeighbors / hnswNodes_.size();
    }

    return stats;
}

size_t EdgeAIEngine::getHNSWVectorDimension() const {
    std::shared_lock<std::shared_mutex> lock(hnswMutex_);
    if (hnswNodes_.empty()) return 0;
    return hnswNodes_.front().vector.size();
}

// ============================================================================
// 子系统7: 模型量化推理
// ============================================================================

QuantizedTensor EdgeAIEngine::quantizeToInt8(const std::vector<float>& tensor,
                                              const std::vector<size_t>& shape) {
    totalQuantizedOps_.fetch_add(1);

    QuantizedTensor result;
    result.shape = shape;
    result.zeroPoint = 0;  // 对称量化，零点为0

    if (tensor.empty()) {
        result.scale = 1.0f;
        return result;
    }

    // 对称量化：scale = max(|x|) / 127
    float absMax = 0.0f;
    for (float v : tensor) {
        float av = std::abs(v);
        if (av > absMax) absMax = av;
    }

    // 防止除零
    result.scale = (absMax < 1e-8f) ? 1e-8f : (absMax / 127.0f);
    float invScale = 1.0f / result.scale;

    result.data.resize(tensor.size());
    for (size_t i = 0; i < tensor.size(); ++i) {
        // q = clamp(round(x / scale), -128, 127)
        float q = std::round(tensor[i] * invScale);
        q = std::clamp(q, -128.0f, 127.0f);
        result.data[i] = static_cast<int8_t>(q);
    }

    return result;
}

std::vector<float> EdgeAIEngine::quantizedMatMul(const QuantizedTensor& a,
                                                   const QuantizedTensor& b,
                                                   size_t M, size_t K, size_t N) {
    totalQuantizedOps_.fetch_add(1);

    // 验证维度
    if (a.data.size() != M * K || b.data.size() != K * N) {
        LOG_ERROR << "[EdgeAI] quantizedMatMul dimension mismatch: "
                  << "A(" << M << "x" << K << ") * B(" << K << "x" << N << ") "
                  << "but got data sizes " << a.data.size() << " and " << b.data.size();
        return std::vector<float>(M * N, 0.0f);
    }

    // 在INT32域执行累加（避免INT8溢出）
    // C_float[i][j] = scale_a * scale_b * Σ_k (a_int8[i][k] * b_int8[k][j])
    float combinedScale = a.scale * b.scale;
    std::vector<float> result(M * N, 0.0f);

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            int32_t acc = 0;
            const int8_t* aRow = a.data.data() + i * K;
            const int8_t* bCol = b.data.data() + j;  // 列步长为 N

            // 4路展开
            size_t k = 0;
            for (; k + 3 < K; k += 4) {
                acc += static_cast<int32_t>(aRow[k])     * static_cast<int32_t>(bCol[k * N]);
                acc += static_cast<int32_t>(aRow[k + 1]) * static_cast<int32_t>(bCol[(k + 1) * N]);
                acc += static_cast<int32_t>(aRow[k + 2]) * static_cast<int32_t>(bCol[(k + 2) * N]);
                acc += static_cast<int32_t>(aRow[k + 3]) * static_cast<int32_t>(bCol[(k + 3) * N]);
            }
            for (; k < K; ++k) {
                acc += static_cast<int32_t>(aRow[k]) * static_cast<int32_t>(bCol[k * N]);
            }

            // 反量化回 float32
            result[i * N + j] = combinedScale * static_cast<float>(acc);
        }
    }

    return result;
}

std::vector<float> EdgeAIEngine::quantizedForward(const std::vector<float>& input,
                                                    const QuantizedTensor& weights,
                                                    const std::vector<float>& biases) {
    totalQuantizedOps_.fetch_add(1);

    if (weights.shape.size() < 2) {
        LOG_ERROR << "[EdgeAI] quantizedForward: weights must be 2D (outDim x inDim)";
        return {};
    }

    size_t outDim = weights.shape[0];
    size_t inDim = weights.shape[1];

    if (input.size() != inDim) {
        LOG_ERROR << "[EdgeAI] quantizedForward: input size " << input.size()
                  << " != weight inDim " << inDim;
        return {};
    }

    if (!biases.empty() && biases.size() != outDim) {
        LOG_ERROR << "[EdgeAI] quantizedForward: bias size " << biases.size()
                  << " != outDim " << outDim;
        return {};
    }

    // 量化输入
    QuantizedTensor qInput = quantizeToInt8(input, {1, inDim});

    // 执行量化矩阵乘法: output = input(1 x inDim) * weights^T(inDim x outDim)
    // 但 weights 存储为 (outDim x inDim)，需要转置
    // 等价于：对每个输出维度，计算 input · weights[outIdx]
    float combinedScale = qInput.scale * weights.scale;
    std::vector<float> output(outDim, 0.0f);

    for (size_t o = 0; o < outDim; ++o) {
        int32_t acc = 0;
        const int8_t* wRow = weights.data.data() + o * inDim;

        size_t k = 0;
        for (; k + 3 < inDim; k += 4) {
            acc += static_cast<int32_t>(qInput.data[k])     * static_cast<int32_t>(wRow[k]);
            acc += static_cast<int32_t>(qInput.data[k + 1]) * static_cast<int32_t>(wRow[k + 1]);
            acc += static_cast<int32_t>(qInput.data[k + 2]) * static_cast<int32_t>(wRow[k + 2]);
            acc += static_cast<int32_t>(qInput.data[k + 3]) * static_cast<int32_t>(wRow[k + 3]);
        }
        for (; k < inDim; ++k) {
            acc += static_cast<int32_t>(qInput.data[k]) * static_cast<int32_t>(wRow[k]);
        }

        output[o] = combinedScale * static_cast<float>(acc);

        // 加偏置
        if (!biases.empty()) {
            output[o] += biases[o];
        }
    }

    // ReLU 激活
    for (size_t o = 0; o < outDim; ++o) {
        output[o] = std::max(0.0f, output[o]);
    }

    return output;
}

// ============================================================================
// 子系统8: 边缘节点健康监控
// ============================================================================

float EdgeAIEngine::computeHealthScore(const EdgeNodeStatus& status) const {
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

    // 心跳超时惩罚：超过30秒未心跳，分数衰减
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - status.lastHeartbeat).count();
    if (elapsed > 30) {
        float decay = std::exp(-0.05f * static_cast<float>(elapsed - 30));
        score *= decay;
    }

    return std::clamp(score, 0.0f, 1.0f);
}

void EdgeAIEngine::registerNode(const std::string& nodeId) {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);

    if (nodeRegistry_.count(nodeId)) {
        LOG_WARN << "[EdgeAI] Node '" << nodeId << "' already registered";
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
    LOG_INFO << "[EdgeAI] Node registered: " << nodeId;
}

void EdgeAIEngine::updateNodeStatus(const EdgeNodeStatus& status) {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);

    auto it = nodeRegistry_.find(status.nodeId);
    if (it == nodeRegistry_.end()) {
        LOG_WARN << "[EdgeAI] Unknown node: " << status.nodeId << ", auto-registering";
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
                LOG_WARN << "[EdgeAI] Circuit OPEN for node '" << node.nodeId
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
                LOG_INFO << "[EdgeAI] Circuit HALF_OPEN for node '" << node.nodeId
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
                    LOG_INFO << "[EdgeAI] Circuit CLOSED for node '" << node.nodeId
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
                LOG_WARN << "[EdgeAI] Circuit re-OPEN for node '" << node.nodeId
                         << "', probe failed, failRate=" << failRate;
            }
            break;
        }
    }

    if (!node.isHealthy) {
        LOG_WARN << "[EdgeAI] Node '" << status.nodeId
                 << "' marked unhealthy, score=" << node.healthScore
                 << ", circuit=" << (node.circuitState == CircuitState::CLOSED ? "CLOSED" :
                                     node.circuitState == CircuitState::OPEN   ? "OPEN" : "HALF_OPEN");
    }
}

std::optional<std::string> EdgeAIEngine::selectBestNode() {
    std::shared_lock<std::shared_mutex> lock(nodeMutex_);

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

std::vector<EdgeNodeStatus> EdgeAIEngine::getAllNodeStatus() const {
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

// ============================================================================
// 引擎综合统计
// ============================================================================

Json::Value EdgeAIEngine::getEngineStats() const {
    Json::Value stats;

    stats["enabled"] = enabled_;
    stats["initialized"] = initialized_;

    // 各子系统调用统计
    Json::Value calls;
    calls["sentiment_analysis"] = static_cast<Json::UInt64>(totalSentimentCalls_.load());
    calls["text_moderation"] = static_cast<Json::UInt64>(totalModerationCalls_.load());
    calls["hnsw_searches"] = static_cast<Json::UInt64>(totalHNSWSearches_.load());
    calls["quantized_ops"] = static_cast<Json::UInt64>(totalQuantizedOps_.load());
    stats["call_counts"] = calls;

    Json::Value sentimentCache;
    sentimentCache["ttl_sec"] = sentimentCacheTTLSeconds_;
    sentimentCache["max_size"] = static_cast<Json::UInt64>(sentimentCacheMaxSize_);
    sentimentCache["hits"] = static_cast<Json::UInt64>(sentimentCacheHits_.load());
    sentimentCache["misses"] = static_cast<Json::UInt64>(sentimentCacheMisses_.load());
    const auto hitBase = sentimentCacheHits_.load() + sentimentCacheMisses_.load();
    sentimentCache["hit_rate"] = hitBase == 0
        ? 0.0
        : static_cast<double>(sentimentCacheHits_.load()) / static_cast<double>(hitBase);
    {
        std::shared_lock<std::shared_mutex> lock(sentimentCacheMutex_);
        sentimentCache["entries"] = static_cast<Json::UInt64>(sentimentCache_.size());
        sentimentCache["inflight"] = static_cast<Json::UInt64>(sentimentInFlight_.size());
    }
    stats["sentiment_cache"] = sentimentCache;

    // HNSW索引状态
    {
        std::shared_lock<std::shared_mutex> lock(hnswMutex_);
        stats["hnsw_node_count"] = static_cast<Json::UInt64>(hnswNodes_.size());
        stats["hnsw_max_level"] = hnswMaxLevel_;
    }

    // 情绪脉搏状态
    {
        std::shared_lock<std::shared_mutex> lock(pulseMutex_);
        stats["emotion_window_size"] = static_cast<Json::UInt64>(emotionWindow_.size());
        stats["pulse_history_size"] = static_cast<Json::UInt64>(pulseHistory_.size());
    }

    // 联邦学习状态
    {
        std::lock_guard<std::mutex> lock(federatedMutex_);
        stats["federated_round"] = federatedRound_;
        stats["pending_local_models"] = static_cast<Json::UInt64>(localModels_.size());
    }

    // 差分隐私状态
    stats["dp_remaining_budget"] = getRemainingPrivacyBudget();
    stats["dp_consumed_epsilon"] = consumedEpsilon_.load();

    // 节点监控状态
    {
        std::shared_lock<std::shared_mutex> lock(nodeMutex_);
        stats["registered_nodes"] = static_cast<Json::UInt64>(nodeRegistry_.size());
        int healthyCount = 0;
        for (const auto& [id, status] : nodeRegistry_) {
            if (status.isHealthy) ++healthyCount;
        }
        stats["healthy_nodes"] = healthyCount;
    }

    return stats;
}

} // namespace ai
} // namespace heartlake
