/**
 * @file AIService.cpp
 * @brief AIService 模块实现
 * Created by 王璐瑶
 */
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "utils/ContentFilter.h"
#include "utils/EmotionManager.h"
#include "utils/EnvUtils.h"
#include <drogon/HttpClient.h>
#include <drogon/drogon.h>
#include <regex>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <cctype>
#include <cstdlib>

using namespace drogon;

namespace heartlake {
namespace ai {

namespace {

std::string trimCopy(const std::string& input) {
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

std::string sanitizeModelText(std::string content) {
    // thread_local 避免 std::regex 多线程并发匹配时的数据竞争
    thread_local static const std::regex thinkRegex("<think>[\\s\\S]*?</think>\\s*");
    content = std::regex_replace(content, thinkRegex, "");

    thread_local static const std::regex fenceRegex("```(?:json)?|```");
    content = std::regex_replace(content, fenceRegex, "");
    return trimCopy(content);
}

std::string canonicalizeMood(std::string moodRaw) {
    auto mood = trimCopy(moodRaw);
    std::transform(mood.begin(), mood.end(), mood.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (mood == "joy" || mood == "excited" || mood == "love" || mood == "happiness") {
        return "happy";
    }
    if (mood == "sadness" || mood == "depressed") {
        return "sad";
    }
    if (mood == "fear" || mood == "fearful" || mood == "stressed" || mood == "worried") {
        return "anxious";
    }
    if (mood == "surprise") {
        return "surprised";
    }
    if (mood == "anger") {
        return "angry";
    }
    if (mood == "peaceful") {
        return "calm";
    }
    if (mood == "gratitude") {
        return "grateful";
    }
    if (mood == "uncertain") {
        return "confused";
    }

    return mood;
}

std::string normalizeForTemplateCheck(const std::string& raw) {
    std::string normalized;
    normalized.reserve(raw.size());
    for (unsigned char ch : raw) {
        if (std::isspace(ch)) {
            continue;
        }
        if (ch == ',' || ch == '.' || ch == '!' || ch == '?' ||
            ch == ';' || ch == ':' || ch == '"' || ch == '\'') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    return normalized;
}

bool isGenericLakeGodReply(const std::string& reply) {
    if (reply.empty()) {
        return true;
    }
    const std::string normalized = normalizeForTemplateCheck(reply);
    static const std::vector<std::string> blocked = {
        normalizeForTemplateCheck("我感受到了你此刻的心情"),
        normalizeForTemplateCheck("无论如何，你并不孤单"),
        normalizeForTemplateCheck("我理解你的感受，有什么想聊的吗"),
        normalizeForTemplateCheck("我现在有点忙，稍后再聊好吗")
    };

    for (const auto& phrase : blocked) {
        if (!phrase.empty() && normalized.find(phrase) != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string briefUserHint(const std::string& userMessage) {
    std::string hint = trimCopy(userMessage);
    if (hint.size() > 20) {
        hint = hint.substr(0, 20);
    }
    for (char& c : hint) {
        if (c == '\n' || c == '\r' || c == '\t') {
            c = ' ';
        }
    }
    return hint;
}

std::string buildLakeGodFallbackReply(const std::string& userMessage) {
    static const std::vector<std::string> opening = {
        "我在，先陪你把这段感受放稳。",
        "你这段话我认真看完了，我们慢慢说。",
        "湖神在这儿，先和你一起把心口这块石头放下。"
    };
    static const std::vector<std::string> nextStep = {
        "如果你愿意，先说说现在最刺痛你的那一件事。",
        "我们先不求一次解决，先把最难受的那一层讲清楚就好。",
        "你可以先告诉我：此刻最想被理解的是哪一句话。"
    };

    const size_t openIdx = std::hash<std::string>{}(userMessage + "|open") % opening.size();
    const size_t stepIdx = std::hash<std::string>{}(userMessage + "|step") % nextStep.size();
    std::ostringstream oss;
    oss << opening[openIdx];
    const auto hint = briefUserHint(userMessage);
    if (!hint.empty()) {
        oss << "你刚刚提到“" << hint << "”，这件事对你很重要。";
    }
    oss << nextStep[stepIdx];
    return oss.str();
}

bool parseJsonObjectLenient(const std::string& raw, Json::Value& parsed) {
    Json::Reader reader;
    auto cleaned = sanitizeModelText(raw);
    if (reader.parse(cleaned, parsed)) {
        return true;
    }

    const auto start = cleaned.find('{');
    const auto end = cleaned.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return false;
    }

    return reader.parse(cleaned.substr(start, end - start + 1), parsed);
}

bool parseSentimentResult(const std::string& raw, float& score, std::string& mood) {
    Json::Value parsed;
    if (!parseJsonObjectLenient(raw, parsed)) {
        return false;
    }

    score = std::clamp(parsed.get("score", 0.0f).asFloat(), -1.0f, 1.0f);
    mood = canonicalizeMood(parsed.get("mood", "neutral").asString());

    static const std::unordered_set<std::string> validMoods = {
        "happy", "calm", "neutral", "anxious", "sad", "angry",
        "surprised", "confused", "hopeful", "grateful", "lonely"
    };
    if (validMoods.find(mood) == validMoods.end()) {
        mood = "neutral";
    }
    return true;
}

void runLocalSentimentFallback(
    const std::string& text,
    const std::function<void(float score, const std::string& mood, const std::string& error)>& callback
) {
    auto& edgeAI = heartlake::ai::EdgeAIEngine::getInstance();
    if (edgeAI.isEnabled()) {
        auto result = edgeAI.analyzeSentimentLocal(text);
        callback(result.score, result.mood, "");
        return;
    }

    auto& emotionMgr = heartlake::emotion::EmotionManager::getInstance();
    auto [score, mood] = emotionMgr.analyzeEmotionFromText(text);
    callback(score, mood, "");
}

int parseNonNegativeIntEnv(const char* envName, int defaultValue) {
    const char* rawValue = std::getenv(envName);
    if (!rawValue) {
        return defaultValue;
    }
    char* end = nullptr;
    long parsed = std::strtol(rawValue, &end, 10);
    if (end == rawValue || *end != '\0' || parsed < 0) {
        return defaultValue;
    }
    return static_cast<int>(parsed);
}

float parseFloatEnv(const char* envName, float defaultValue, float minValue, float maxValue) {
    const char* rawValue = std::getenv(envName);
    if (!rawValue) {
        return defaultValue;
    }
    char* end = nullptr;
    float parsed = std::strtof(rawValue, &end);
    if (end == rawValue || *end != '\0') {
        return defaultValue;
    }
    return std::clamp(parsed, minValue, maxValue);
}

}  // namespace

AIService& AIService::getInstance() {
    static AIService instance;
    return instance;
}

void AIService::initialize(const Json::Value& config) {
    provider_ = config.get("provider", "deepseek").asString();
    apiKey_ = config.get("api_key", "").asString();
    baseUrl_ = config.get("base_url", "https://api.deepseek.com").asString();
    model_ = config.get("model", "deepseek-chat").asString();
    timeout_ = config.get("timeout", 10).asInt();
    const int defaultRetries = (provider_ == "ollama") ? 1 : 2;
    maxRetries_ = parseNonNegativeIntEnv("AI_MAX_RETRIES", defaultRetries);
    circuitFailureThreshold_ = heartlake::utils::parsePositiveIntEnv("AI_CIRCUIT_BREAKER_FAILURES", 3);
    circuitCooldownSeconds_ = heartlake::utils::parsePositiveIntEnv("AI_CIRCUIT_BREAKER_COOLDOWN_SEC", 20);
    localSentimentConfidenceThreshold_ = parseFloatEnv(
        "AI_SENTIMENT_LOCAL_CONF_THRESHOLD", 0.72f, 0.50f, 0.95f);
    sentimentCacheTTLSeconds_ = heartlake::utils::parsePositiveIntEnv(
        "AI_SENTIMENT_CACHE_TTL_SEC", sentimentCacheTTLSeconds_);
    sentimentCacheMaxSize_ = static_cast<size_t>(heartlake::utils::parsePositiveIntEnv(
        "AI_SENTIMENT_CACHE_MAX_SIZE", static_cast<int>(sentimentCacheMaxSize_)));
    sentimentAdaptiveInflightThreshold_ = heartlake::utils::parsePositiveIntEnv(
        "AI_SENTIMENT_ADAPTIVE_INFLIGHT_THRESHOLD", sentimentAdaptiveInflightThreshold_);
    sentimentAdaptiveLocalConfDelta_ = parseFloatEnv(
        "AI_SENTIMENT_ADAPTIVE_LOCAL_CONF_DELTA", sentimentAdaptiveLocalConfDelta_, 0.0f, 0.25f);
    {
        std::lock_guard<std::mutex> lock(circuitMutex_);
        circuitOpen_ = false;
        consecutiveFailures_ = 0;
    }

    // 初始化语义缓存
    SemanticCache::getInstance().initialize(0.92f, 5000, 86400);

    // 创建复用的 HTTP 客户端
    ollamaClient_ = HttpClient::newHttpClient(baseUrl_);
    ollamaClient_->setUserAgent("HeartLake/2.0");

    const bool ollamaForceGpu = heartlake::utils::parseBoolEnv(
        std::getenv("AI_OLLAMA_FORCE_GPU"), true);
    const int ollamaNumGpu = parseNonNegativeIntEnv("AI_OLLAMA_NUM_GPU", 999);
    const int ollamaMainGpu = parseNonNegativeIntEnv("AI_OLLAMA_MAIN_GPU", 0);
    const int ollamaNumThread = parseNonNegativeIntEnv("AI_OLLAMA_NUM_THREAD", 0);
    const int ollamaNumCtx = parseNonNegativeIntEnv("AI_OLLAMA_NUM_CTX", 0);

    LOG_INFO << "AIService initialized with provider: " << provider_
             << ", max_retries=" << maxRetries_
             << ", circuit_threshold=" << circuitFailureThreshold_
             << ", circuit_cooldown_sec=" << circuitCooldownSeconds_
             << ", local_sentiment_conf_threshold=" << localSentimentConfidenceThreshold_
             << ", sentiment_cache_ttl_sec=" << sentimentCacheTTLSeconds_
             << ", sentiment_cache_max_size=" << sentimentCacheMaxSize_
             << ", sentiment_adaptive_inflight_threshold=" << sentimentAdaptiveInflightThreshold_
             << ", sentiment_adaptive_local_conf_delta=" << sentimentAdaptiveLocalConfDelta_
             << ", ollama_force_gpu=" << (ollamaForceGpu ? "true" : "false")
             << ", ollama_num_gpu=" << ollamaNumGpu
             << ", ollama_main_gpu=" << ollamaMainGpu
             << ", ollama_num_thread=" << ollamaNumThread
             << ", ollama_num_ctx=" << ollamaNumCtx;
}

AIService::LocalModerationResult AIService::localModerate(const std::string& text) {
    LocalModerationResult result;
    result.passed = true;
    result.needsAlert = false;
    result.category = "normal";

    // 委托给ContentFilter的AC自动机实现
    auto& filter = heartlake::ContentFilter::getInstance();
    auto matches = filter.getMatchedWords(text);

    for (const auto& m : matches) {
        result.matchedWords.push_back(filter.getPattern(m.patternId));
        // category: 1=self_harm, 2=violence, 3=sexual
        if (m.category == 1) result.category = "self_harm";
        else if (m.category == 2) result.category = "violence";
        else if (m.category == 3) result.category = "sexual";

        if (m.level >= 2) result.passed = false;
        if (m.level == 3) result.needsAlert = true;
    }

    return result;
}

void AIService::analyzeSentiment(
    const std::string& text,
    std::function<void(float score, const std::string& mood, const std::string& error)> callback
) {
    const std::string normalizedText = normalizeSentimentText(text);
    const std::string sentimentKey = computeTextHash(normalizedText);

    float cachedScore = 0.0f;
    std::string cachedMood;
    if (getSentimentFromCache(sentimentKey, cachedScore, cachedMood)) {
        callback(cachedScore, cachedMood, "");
        return;
    }

    if (!registerSentimentInFlight(sentimentKey, std::move(callback))) {
        return;
    }

    auto fanout = [this, sentimentKey](float score, const std::string& mood, const std::string& error) {
        const std::string canonicalMood = canonicalizeMood(mood);
        if (error.empty()) {
            putSentimentToCache(sentimentKey, score, canonicalMood);
        }
        completeSentimentInFlight(sentimentKey, score, canonicalMood, error);
    };

    // 本地模型优先：高置信度直接返回；高并发下启用自适应阈值保护尾延迟。
    bool hasLocalCandidate = false;
    float localScoreCandidate = 0.0f;
    std::string localMoodCandidate = "neutral";
    try {
        auto& edgeAI = heartlake::ai::EdgeAIEngine::getInstance();
        if (edgeAI.isEnabled()) {
            auto local = edgeAI.analyzeSentimentLocal(text);
            localScoreCandidate = local.score;
            localMoodCandidate = local.mood;
            hasLocalCandidate = true;

            const float adaptiveThreshold = std::max(
                0.50f, localSentimentConfidenceThreshold_ - sentimentAdaptiveLocalConfDelta_);
            const bool highInflight = currentSentimentInFlightCount() >=
                static_cast<size_t>(sentimentAdaptiveInflightThreshold_);
            const bool adaptiveBypass = highInflight && (
                local.confidence >= adaptiveThreshold || local.confidence >= 0.40f);

            if (local.confidence >= localSentimentConfidenceThreshold_ || adaptiveBypass) {
                if (adaptiveBypass && local.confidence < localSentimentConfidenceThreshold_) {
                    LOG_WARN << "Adaptive sentiment bypass triggered, confidence=" << local.confidence;
                }
                fanout(local.score, local.mood, "");
                return;
            }
        }
    } catch (const std::exception& e) {
        LOG_WARN << "Local sentiment path exception: " << e.what();
    }

    // 低置信度时调用大模型做情感分析；失败时降级到本地候选结果
    Json::Value payload;
    payload["model"] = model_;
    payload["messages"] = Json::arrayValue;

    Json::Value systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] =
        "你是情绪分类器。只输出JSON对象，不要解释。\n"
        "格式: {\"score\":-1到1浮点数,\"mood\":\"happy|calm|neutral|anxious|sad|angry|surprised|confused\"}\n"
        "若不确定请输出 neutral，score 接近0。";
    payload["messages"].append(systemMsg);

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;
    payload["messages"].append(userMsg);

    payload["temperature"] = 0.05;
    payload["top_p"] = 0.7;
    payload["max_tokens"] = 40;
    Json::Value responseFormat;
    responseFormat["type"] = "json_object";
    payload["response_format"] = responseFormat;

    auto fallback = [text, fanout, hasLocalCandidate, localScoreCandidate, localMoodCandidate]() {
        if (hasLocalCandidate) {
            LOG_WARN << "Sentiment fallback to local candidate";
            fanout(localScoreCandidate, localMoodCandidate, "");
            return;
        }
        LOG_WARN << "Sentiment fallback to local model";
        runLocalSentimentFallback(
            text,
            [fanout](float score, const std::string& mood, const std::string& error) {
                fanout(score, mood, error);
            }
        );
    };

    callAIAPI("/v1/chat/completions", payload,
        [this, text, fanout, fallback](const Json::Value& response, const std::string& error) {
            if (!error.empty()) {
                LOG_WARN << "LLM sentiment request failed: " << error;
                fallback();
                return;
            }

            try {
                const std::string content = response["choices"][0]["message"]["content"].asString();
                float score = 0.0f;
                std::string mood;
                if (parseSentimentResult(content, score, mood)) {
                    LOG_INFO << "LLM sentiment parsed: score=" << score
                             << " mood=" << mood
                             << " text=" << text.substr(0, 50);
                    fanout(score, mood, "");
                    return;
                }

                // 一次轻量修复重试：把模型输出规范化为JSON
                Json::Value repairPayload;
                repairPayload["model"] = model_;
                repairPayload["messages"] = Json::arrayValue;
                Json::Value repairSystem;
                repairSystem["role"] = "system";
                repairSystem["content"] =
                    "将输入转换为合法JSON对象，字段仅保留 score,mood。"
                    "mood 只能是 happy/calm/neutral/anxious/sad/angry/surprised/confused。"
                    "只输出JSON。";
                Json::Value repairUser;
                repairUser["role"] = "user";
                repairUser["content"] = content;
                repairPayload["messages"].append(repairSystem);
                repairPayload["messages"].append(repairUser);
                repairPayload["temperature"] = 0.0;
                repairPayload["top_p"] = 0.3;
                repairPayload["max_tokens"] = 48;
                Json::Value repairResponseFormat;
                repairResponseFormat["type"] = "json_object";
                repairPayload["response_format"] = repairResponseFormat;

                callAIAPI("/v1/chat/completions", repairPayload,
                    [fanout, fallback](const Json::Value& repairResp, const std::string& repairErr) {
                        if (!repairErr.empty()) {
                            fallback();
                            return;
                        }
                        try {
                            const auto repaired = repairResp["choices"][0]["message"]["content"].asString();
                            float repairedScore = 0.0f;
                            std::string repairedMood;
                            if (parseSentimentResult(repaired, repairedScore, repairedMood)) {
                                fanout(repairedScore, repairedMood, "");
                                return;
                            }
                        } catch (const std::exception& e) {
                            LOG_WARN << "AI sentiment repair failed: " << e.what();
                            // 忽略并走降级
                        }
                        fallback();
                    });
            } catch (const std::exception& e) {
                LOG_WARN << "Sentiment parse exception: " << e.what();
                fallback();
            }
        }
    );
}

void AIService::moderateText(
    const std::string& text,
    std::function<void(bool passed, const std::vector<std::string>& categories,
                      float confidence, const std::string& reason)> callback
) {
    // 计算文本哈希用于缓存
    std::string textHash = computeTextHash(text);

    // 检查缓存
    ModerationCacheEntry cachedEntry;
    if (getModerationFromCache(textHash, cachedEntry)) {
        moderationCacheHits_++;
        callback(cachedEntry.passed, cachedEntry.categories,
                cachedEntry.confidence, cachedEntry.reason);
        return;
    }

    moderationCacheMisses_++;
    totalLocalCalls_++;

    // 先进行本地快速检测
    auto localResult = localModerate(text);
    if (!localResult.passed) {
        ModerationCacheEntry entry;
        entry.passed = false;
        entry.categories = {localResult.category};
        entry.confidence = 1.0f;
        entry.reason = "内容包含敏感词: " + (localResult.matchedWords.empty() ? "" : localResult.matchedWords[0]);
        entry.timestamp = std::chrono::steady_clock::now();

        putModerationToCache(textHash, entry);

        callback(false, entry.categories, entry.confidence, entry.reason);
        return;
    }

    // 如果本地检测通过，调用AI进行深度审核
    totalAPICalls_++;

    Json::Value payload;
    payload["model"] = model_;
    payload["messages"] = Json::arrayValue;

    Json::Value systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = buildModerationPrompt("");
    payload["messages"].append(systemMsg);

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;
    payload["messages"].append(userMsg);

    payload["temperature"] = 0.1;
    payload["top_p"] = 0.7;
    payload["max_tokens"] = 64;
    payload["response_format"] = "json_object";

    callAIAPI("/v1/chat/completions", payload,
        [callback, textHash, this](const Json::Value& response, const std::string& error) {
            if (!error.empty()) {
                // AI审核失败时，默认放行但标记为待人工审核
                callback(true, {"pending_review"}, 0.0f, error);
                return;
            }

            try {
                std::string content = response["choices"][0]["message"]["content"].asString();

                Json::Value parsed;
                if (parseJsonObjectLenient(content, parsed)) {
                    bool passed = parsed.get("safe", true).asBool();
                    float confidence = parsed.get("confidence", 0.9f).asFloat();
                    std::string reason = parsed.get("reason", "").asString();

                    std::vector<std::string> categories;
                    if (parsed.isMember("categories") && parsed["categories"].isArray()) {
                        for (const auto& cat : parsed["categories"]) {
                            categories.push_back(cat.asString());
                        }
                    }

                    // 缓存结果
                    ModerationCacheEntry entry;
                    entry.passed = passed;
                    entry.categories = categories;
                    entry.confidence = confidence;
                    entry.reason = reason;
                    entry.timestamp = std::chrono::steady_clock::now();
                    putModerationToCache(textHash, entry);

                    callback(passed, categories, confidence, reason);
                } else {
                    callback(true, {}, 0.8f, "");
                }
            } catch (const std::exception& e) {
                callback(true, {"error"}, 0.0f, e.what());
            }
        }
    );
}

void AIService::generateReply(
    const std::string& userMessage,
    const std::string& context,
    std::function<void(const std::string& reply, const std::string& error)> callback
) {
    // RAG/个性化上下文场景禁用语义缓存，避免跨上下文误命中同一回复。
    const bool useSemanticCache = context.empty();
    std::vector<float> queryEmbedding;
    if (useSemanticCache) {
        auto& embeddingEngine = AdvancedEmbeddingEngine::getInstance();
        queryEmbedding = embeddingEngine.generateEmbedding(userMessage);
    }

    // 检查语义缓存
    std::string cachedReply;
    if (useSemanticCache &&
        SemanticCache::getInstance().get(userMessage, queryEmbedding, cachedReply)) {
        callback(cachedReply, "");
        return;
    }

    Json::Value payload;
    payload["model"] = model_;
    payload["messages"] = Json::arrayValue;

    Json::Value systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = buildReplyPrompt("", context);
    payload["messages"].append(systemMsg);

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    payload["messages"].append(userMsg);

    payload["temperature"] = 0.5;
    payload["top_p"] = 0.85;
    payload["max_tokens"] = 320;

    callAIAPI("/v1/chat/completions", payload,
        [callback, userMessage, queryEmbedding, useSemanticCache](const Json::Value& response, const std::string& error) {
            if (!error.empty()) {
                callback(buildLakeGodFallbackReply(userMessage), error);
                return;
            }

            try {
                std::string reply = response["choices"][0]["message"]["content"].asString();
                if (reply.empty() || reply.find_first_not_of(" \t\n\r") == std::string::npos ||
                    isGenericLakeGodReply(reply)) {
                    callback(buildLakeGodFallbackReply(userMessage), "");
                    return;
                }
                // 存入语义缓存
                if (useSemanticCache) {
                    SemanticCache::getInstance().put(userMessage, queryEmbedding, reply);
                }
                callback(reply, "");
            } catch (const std::exception& e) {
                callback(buildLakeGodFallbackReply(userMessage), e.what());
            }
        }
    );
}

void AIService::generateBoatReply(
    const std::string& boatContent,
    const std::string& senderMood,
    std::function<void(const std::string& reply, const std::string& error)> callback
) {
    Json::Value payload;
    payload["model"] = model_;
    payload["messages"] = Json::arrayValue;

    Json::Value systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = buildBoatReplyPrompt("", senderMood);
    payload["messages"].append(systemMsg);

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = boatContent;
    payload["messages"].append(userMsg);

    payload["temperature"] = 0.6;
    payload["top_p"] = 0.9;
    payload["max_tokens"] = 220;

    callAIAPI("/v1/chat/completions", payload,
        [callback](const Json::Value& response, const std::string& error) {
            if (!error.empty()) {
                callback("收到你的纸船了，愿你一切安好。", error);
                return;
            }

            try {
                std::string reply = response["choices"][0]["message"]["content"].asString();
                if (reply.empty() || reply.find_first_not_of(" \t\n\r") == std::string::npos) {
                    callback("收到你的纸船了，愿你一切安好。", "");
                    return;
                }
                callback(reply, "");
            } catch (const std::exception& e) {
                callback("收到你的纸船了，愿你一切安好。", e.what());
            }
        }
    );
}

void AIService::generateStoneComment(
    const std::string& stoneContent,
    const std::string& stoneMood,
    std::function<void(const std::string& comment, const std::string& error)> callback
) {
    Json::Value payload;
    payload["model"] = model_;
    payload["messages"] = Json::arrayValue;

    Json::Value systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = buildStoneCommentPrompt("", stoneMood);
    payload["messages"].append(systemMsg);

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = stoneContent;
    payload["messages"].append(userMsg);

    payload["temperature"] = 0.55;
    payload["top_p"] = 0.85;
    payload["max_tokens"] = 120;

    callAIAPI("/v1/chat/completions", payload,
        [callback](const Json::Value& response, const std::string& error) {
            if (!error.empty()) {
                callback("看到你的分享了，愿你今天也能有好心情。", error);
                return;
            }

            try {
                std::string comment = response["choices"][0]["message"]["content"].asString();
                if (comment.empty() || comment.find_first_not_of(" \t\n\r") == std::string::npos) {
                    callback("看到你的分享了，愿你今天也能有好心情。", "");
                    return;
                }
                callback(comment, "");
            } catch (const std::exception& e) {
                callback("看到你的分享了，愿你今天也能有好心情。", e.what());
            }
        }
    );
}

void AIService::generateEmbedding(
    const std::string& text,
    std::function<void(const std::vector<float>& embedding, const std::string& error)> callback
) {
    // 使用本地高性能嵌入向量引擎，避免API调用成本
    try {
        auto& embeddingEngine = heartlake::ai::AdvancedEmbeddingEngine::getInstance();
        auto embedding = embeddingEngine.generateEmbedding(text);
        callback(embedding, "");
    } catch (const std::exception& e) {
        LOG_ERROR << "Local embedding generation failed: " << e.what();
        callback({}, e.what());
    }
}

void AIService::callAIAPI(
    const std::string& endpoint,
    const Json::Value& payload,
    std::function<void(const Json::Value& response, const std::string& error)> callback
) {
    if (isCircuitOpen()) {
        callback(Json::Value(), "AI service temporarily unavailable (circuit open)");
        return;
    }

    // 本地 ollama 不需要 API Key
    if (apiKey_.empty() && provider_ != "ollama") {
        LOG_ERROR << "AI API Key is empty! Please configure api_key in config.json";
        callback(Json::Value(), "AI API Key not configured");
        return;
    }

    // Ollama: 转换为原生 /api/chat 格式，关闭 thinking mode
    if (provider_ == "ollama") {
        Json::Value ollamaPayload;
        ollamaPayload["model"] = payload.get("model", model_).asString();
        ollamaPayload["messages"] = payload["messages"];
        ollamaPayload["stream"] = false;
        ollamaPayload["think"] = false;
        if (payload.isMember("max_tokens")) {
            ollamaPayload["options"]["num_predict"] = payload["max_tokens"];
        }
        if (payload.isMember("temperature")) {
            ollamaPayload["options"]["temperature"] = payload["temperature"];
        }
        if (payload.isMember("top_p")) {
            ollamaPayload["options"]["top_p"] = payload["top_p"];
        }

        // 默认强制走 GPU，若机器不支持由 Ollama 自动报错并可回退
        const bool forceGpu = heartlake::utils::parseBoolEnv(
            std::getenv("AI_OLLAMA_FORCE_GPU"), true);
        if (forceGpu) {
            ollamaPayload["options"]["num_gpu"] = parseNonNegativeIntEnv("AI_OLLAMA_NUM_GPU", 999);
            ollamaPayload["options"]["main_gpu"] = parseNonNegativeIntEnv("AI_OLLAMA_MAIN_GPU", 0);
            ollamaPayload["options"]["low_vram"] = heartlake::utils::parseBoolEnv(
                std::getenv("AI_OLLAMA_LOW_VRAM"), false);
        }

        const int numThread = parseNonNegativeIntEnv("AI_OLLAMA_NUM_THREAD", 0);
        if (numThread > 0) {
            ollamaPayload["options"]["num_thread"] = numThread;
        }

        const int numCtx = parseNonNegativeIntEnv("AI_OLLAMA_NUM_CTX", 0);
        if (numCtx > 0) {
            ollamaPayload["options"]["num_ctx"] = numCtx;
        }

        if (payload.isMember("response_format")) {
            const auto format = payload["response_format"];
            if (format.isString() && format.asString() == "json_object") {
                ollamaPayload["format"] = "json";
            } else if (format.isObject()) {
                // Ollama /api/chat 不接受 OpenAI 的 response_format schema 对象，统一降维为 json。
                ollamaPayload["format"] = "json";
            }
        }
        if (payload.isMember("format")) {
            const auto& explicitFormat = payload["format"];
            if (explicitFormat.isString()) {
                const auto formatValue = explicitFormat.asString();
                if (formatValue == "json_object" || formatValue == "json_schema") {
                    ollamaPayload["format"] = "json";
                } else {
                    ollamaPayload["format"] = formatValue;
                }
            } else if (explicitFormat.isObject()) {
                ollamaPayload["format"] = "json";
            }
        }
        callAIAPIWithRetry("/api/chat", ollamaPayload,
            [callback](const Json::Value& ollamaResp, const std::string& error) {
                auto safeReturn = [&](const Json::Value& responseValue, const std::string& errorValue) {
                    try {
                        callback(responseValue, errorValue);
                    } catch (const std::exception& cbEx) {
                        LOG_ERROR << "AI callback threw exception: " << cbEx.what();
                    } catch (...) {
                        LOG_ERROR << "AI callback threw non-std exception";
                    }
                };
                if (!error.empty()) {
                    safeReturn(Json::Value(), error);
                    return;
                }
                try {
                    if (!ollamaResp.isObject()) {
                        safeReturn(Json::Value(), "Invalid Ollama response shape");
                        return;
                    }

                    std::string content;
                    if (ollamaResp.isMember("message")) {
                        const auto& msg = ollamaResp["message"];
                        if (msg.isObject()) {
                            content = msg.get("content", "").asString();
                        } else if (msg.isString()) {
                            content = msg.asString();
                        }
                    }
                    if (content.empty() && ollamaResp.isMember("response") && ollamaResp["response"].isString()) {
                        content = ollamaResp["response"].asString();
                    }

                    // 转换 Ollama 原生响应为 OpenAI 格式
                    Json::Value openaiResp;
                    openaiResp["choices"] = Json::arrayValue;
                    Json::Value choice;
                    choice["index"] = 0;
                    choice["message"]["role"] = "assistant";
                    choice["message"]["content"] = content;
                    choice["finish_reason"] = ollamaResp.get("done_reason", "stop").asString();
                    openaiResp["choices"].append(choice);
                    openaiResp["model"] = ollamaResp.get("model", "").asString();
                    safeReturn(openaiResp, "");
                } catch (const std::exception& e) {
                    safeReturn(Json::Value(), std::string("Ollama response parse error: ") + e.what());
                }
            }, 0);
        return;
    }

    callAIAPIWithRetry(endpoint, payload, callback, 0);
}

void AIService::callAIAPIWithRetry(
    const std::string& endpoint,
    const Json::Value& payload,
    std::function<void(const Json::Value& response, const std::string& error)> callback,
    int retryCount
) {
    auto client = ollamaClient_ ? ollamaClient_ : HttpClient::newHttpClient(baseUrl_);

    LOG_INFO << "Calling AI API: " << baseUrl_ << endpoint << " (attempt " << (retryCount + 1) << ")";

    auto req = HttpRequest::newHttpJsonRequest(payload);
    req->setMethod(drogon::Post);
    req->setPath(endpoint);
    // ollama 本地模型不需要 Authorization 头
    if (!apiKey_.empty()) {
        req->addHeader("Authorization", "Bearer " + apiKey_);
    }
    req->addHeader("Content-Type", "application/json");
    req->addHeader("Accept", "application/json");

    auto self = this;
    client->sendRequest(req,
        [self, callback, endpoint, payload, retryCount](ReqResult result, const HttpResponsePtr& response) {
            auto safeReturn = [&](const Json::Value& responseValue, const std::string& errorValue) {
                try {
                    callback(responseValue, errorValue);
                } catch (const std::exception& cbEx) {
                    LOG_ERROR << "AI callback threw exception: " << cbEx.what();
                } catch (...) {
                    LOG_ERROR << "AI callback threw unknown exception";
                }
            };

            // 网络层错误处理
            if (result != ReqResult::Ok) {
                std::string errorMsg = self->getReqResultError(result);
                LOG_ERROR << "AI API network error: " << errorMsg;

                // 可重试的错误
                if (retryCount < self->maxRetries_ && self->isRetryableError(result)) {
                    int delayMs = self->getRetryDelay(retryCount);
                    LOG_WARN << "Retrying in " << delayMs << "ms...";
                    drogon::app().getLoop()->runAfter(delayMs / 1000.0, [self, endpoint, payload, callback, retryCount]() {
                        self->callAIAPIWithRetry(endpoint, payload, callback, retryCount + 1);
                    });
                    return;
                }

                LOG_ERROR << "AI API failed after " << (retryCount + 1) << " attempts";
                self->onRequestFailure();
                safeReturn(Json::Value(), errorMsg);
                return;
            }

            auto statusCode = response->getStatusCode();

            // HTTP错误处理
            if (statusCode != k200OK) {
                auto body = response->getBody();
                std::string errorMsg = "HTTP " + std::to_string(statusCode);

                // 解析API错误详情
                Json::Value errJson;
                Json::Reader reader;
                if (reader.parse(std::string(body), errJson) &&
                    errJson.isObject() &&
                    errJson.isMember("error")) {
                    const auto& errorNode = errJson["error"];
                    if (errorNode.isObject()) {
                        errorMsg += ": " + errorNode.get("message", "Unknown error").asString();
                    } else if (errorNode.isString()) {
                        errorMsg += ": " + errorNode.asString();
                    } else {
                        errorMsg += ": Unknown error";
                    }
                }

                LOG_ERROR << "AI API error: " << errorMsg;

                // 5xx错误可重试
                if (retryCount < self->maxRetries_ && statusCode >= 500) {
                    int delayMs = self->getRetryDelay(retryCount);
                    LOG_WARN << "Retrying in " << delayMs << "ms...";
                    drogon::app().getLoop()->runAfter(delayMs / 1000.0, [self, endpoint, payload, callback, retryCount]() {
                        self->callAIAPIWithRetry(endpoint, payload, callback, retryCount + 1);
                    });
                    return;
                }

                self->onRequestFailure();
                safeReturn(Json::Value(), errorMsg);
                return;
            }

            auto jsonPtr = response->getJsonObject();
            if (jsonPtr) {
                // 清理 Qwen3 thinking mode 的 <think>...</think> 标签
                auto& json = *jsonPtr;
                try {
                    if (json.isObject() && json.isMember("choices") && json["choices"].isArray()) {
                        for (auto& choice : json["choices"]) {
                            if (!choice.isObject() || !choice.isMember("message")) {
                                continue;
                            }
                            auto& message = choice["message"];
                            if (!message.isObject() || !message.isMember("content")) {
                                continue;
                            }
                            std::string content = message["content"].asString();
                            static const std::regex thinkRegex("<think>[\\s\\S]*?</think>\\s*");
                            content = std::regex_replace(content, thinkRegex, "");
                            message["content"] = content;
                        }
                    }
                } catch (const std::exception& e) {
                    LOG_WARN << "AI response post-process skipped: " << e.what();
                }
                self->onRequestSuccess();
                safeReturn(json, "");
            } else {
                LOG_ERROR << "Invalid JSON response from AI API";
                self->onRequestFailure();
                safeReturn(Json::Value(), "Invalid JSON response");
            }
        },
        self->timeout_
    );
}

std::string AIService::getReqResultError(ReqResult result) const {
    switch (result) {
        case ReqResult::BadResponse: return "Bad response from server";
        case ReqResult::NetworkFailure: return "Network failure";
        case ReqResult::BadServerAddress: return "Invalid server address";
        case ReqResult::Timeout: return "Request timeout";
        default: return "Unknown network error";
    }
}

bool AIService::isRetryableError(ReqResult result) const {
    return result == ReqResult::Timeout || result == ReqResult::NetworkFailure;
}

int AIService::getRetryDelay(int retryCount) const {
    // 指数退避: 1s, 2s, 4s
    return 1000 * (1 << retryCount);
}

bool AIService::isCircuitOpen() {
    std::lock_guard<std::mutex> lock(circuitMutex_);
    if (!circuitOpen_) {
        return false;
    }
    const auto now = std::chrono::steady_clock::now();
    const auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(now - circuitOpenedAt_).count();
    if (elapsedSec >= circuitCooldownSeconds_) {
        circuitOpen_ = false;
        consecutiveFailures_ = 0;
        LOG_INFO << "AI circuit closed after cooldown";
        return false;
    }
    return true;
}

void AIService::onRequestSuccess() {
    std::lock_guard<std::mutex> lock(circuitMutex_);
    consecutiveFailures_ = 0;
    if (circuitOpen_) {
        circuitOpen_ = false;
        LOG_INFO << "AI circuit recovered to closed state";
    }
}

void AIService::onRequestFailure() {
    std::lock_guard<std::mutex> lock(circuitMutex_);
    ++consecutiveFailures_;
    if (!circuitOpen_ && consecutiveFailures_ >= circuitFailureThreshold_) {
        circuitOpen_ = true;
        circuitOpenedAt_ = std::chrono::steady_clock::now();
        LOG_WARN << "AI circuit opened after consecutive failures: " << consecutiveFailures_;
    }
}

std::string AIService::buildSentimentPrompt([[maybe_unused]] const std::string& text) {
    return R"(你是一个情感分析专家。请分析用户输入文本的情感倾向。

返回JSON格式：
{
  "score": 0.5,  // -1.0(极负面) 到 1.0(极正面)
  "mood": "neutral",  // happy, sad, neutral, anxious, grateful, angry, hopeful
  "keywords": ["关键词1", "关键词2"]
}

只返回JSON，不要其他解释。)";
}

std::string AIService::buildModerationPrompt([[maybe_unused]] const std::string& text) {
    return R"(你是一个内容安全审核员。请审核用户输入的内容是否安全。

需要检测的问题类别：
- violence: 暴力内容
- sexual: 色情内容  
- hate: 仇恨言论
- self_harm: 自伤倾向
- harassment: 骚扰内容
- spam: 垃圾信息

返回JSON格式：
{
  "safe": true,
  "confidence": 0.95,
  "categories": [],
  "reason": ""
}

注意：对于表达负面情绪但不违规的内容（如倾诉烦恼、求助等），应该标记为safe。
只返回JSON，不要其他解释。)";
}

std::string AIService::buildReplyPrompt([[maybe_unused]] const std::string& message, const std::string& context) {
    return R"(你是「心湖湖神」，一位温暖而智慧的陪伴者。

你的特点：
- 善于倾听和理解用户的真实感受
- 用温和、自然的语气交流，像朋友一样
- 提供有洞察力的建议，但不说教
- 在用户需要时给予情感支持和实用建议
- 对于严重的心理问题，会建议寻求专业帮助

回复要求：
- 90-160字，简洁但有温度
- 直接回复，不需要称呼
- 使用自然、口语化的表达
- emoji最多1个，避免花哨
- 如果用户表达负面情绪，先共情再引导
- 信息不足时，优先提出1个简短澄清问题
- 禁止使用模板套话：不要出现“我感受到了你此刻的心情”“无论如何，你并不孤单”等陈词

背景：)" + context + R"(

现在请回复用户的消息。)";
}

std::string AIService::buildBoatReplyPrompt([[maybe_unused]] const std::string& content, const std::string& mood) {
    return R"(你收到了一只飘过心湖的纸船，上面写着某人的心声。

发送者的情绪状态：)" + mood + R"(

🌊 **你的回应方式**：
- 像收到朋友来信一样，给予真挚的回应
- 可以分享相似的经历或感受，建立情感连接
- 提供新的视角或温暖的鼓励
- 让对方感到被理解、不孤单
- 适当的幽默或诗意可以让回复更有温度

📝 **回复要求**：
- 90-150字
- 真诚自然，像给朋友写信
- 可以有个人色彩和情感温度
- 避免说教，重在共鸣和陪伴

请直接写回复内容，不需要称呼或落款。)";
}

std::string AIService::buildStoneCommentPrompt([[maybe_unused]] const std::string& content, const std::string& mood) {
    return R"(你是「心湖」的湖神陪伴者，看到有人在湖中投下了一颗石头，分享了他们的湖底。

石头作者的情绪状态：)" + mood + R"(

💙 **你的回应方式**：
- 用温暖、真诚的语气给予回应
- 理解并共情对方的感受，让TA感到被看见
- 根据情绪类型给予适当的安慰、鼓励或陪伴
- 如果是负面情绪，先共情再温柔引导
- 如果是正面情绪，真诚地为TA感到开心
- 可以用简单的比喻或诗意的表达增加温度

✨ **回复要求**：
- 70-120字，简短但有温度
- 直接回复，不需要称呼
- 语气自然、口语化，像朋友间的对话
- 可以适度使用emoji增加亲切感（1-2个）
- 避免说教和空洞的鸡汤
- 重在情感连接和真诚陪伴

请直接写评论内容。)";
}

std::string AIService::parseMoodFromScore(float score) {
    // 使用统一的EmotionManager
    auto& emotionMgr = heartlake::emotion::EmotionManager::getInstance();
    return emotionMgr.getEmotionFromScore(score);
}

// 计算文本哈希（用于缓存key）
std::string AIService::computeTextHash(const std::string& text) const {
    std::hash<std::string> hasher;
    size_t hash = hasher(text);
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

std::string AIService::normalizeSentimentText(const std::string& text) const {
    if (text.empty()) {
        return text;
    }

    std::string normalized;
    normalized.reserve(std::min<size_t>(text.size(), 2048));
    bool prevSpace = false;

    for (unsigned char ch : text) {
        if (ch < 128) {
            if (std::isspace(ch)) {
                if (!prevSpace && !normalized.empty()) {
                    normalized.push_back(' ');
                }
                prevSpace = true;
                continue;
            }
            if (std::ispunct(ch)) {
                continue;
            }
            normalized.push_back(static_cast<char>(std::tolower(ch)));
            prevSpace = false;
        } else {
            normalized.push_back(static_cast<char>(ch));
            prevSpace = false;
        }

        if (normalized.size() >= 2048) {
            break;
        }
    }

    if (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    return normalized.empty() ? text : normalized;
}

// 从缓存获取审核结果
bool AIService::getModerationFromCache(const std::string& textHash, ModerationCacheEntry& entry) {
    std::lock_guard<std::mutex> lock(moderationCacheMutex_);

    auto it = moderationCache_.find(textHash);
    if (it == moderationCache_.end()) {
        return false;
    }

    // 检查是否过期
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();

    if (age > moderationCacheTTLSeconds_) {
        moderationCache_.erase(it);
        return false;
    }

    entry = it->second;
    return true;
}

// 将审核结果放入缓存
void AIService::putModerationToCache(const std::string& textHash, const ModerationCacheEntry& entry) {
    std::lock_guard<std::mutex> lock(moderationCacheMutex_);

    // 如果缓存已满，清理过期条目
    if (moderationCache_.size() >= moderationCacheMaxSize_) {
        cleanExpiredCache();

        // 如果清理后仍然满，删除最旧的条目
        if (moderationCache_.size() >= moderationCacheMaxSize_) {
            auto oldest = moderationCache_.begin();
            for (auto it = moderationCache_.begin(); it != moderationCache_.end(); ++it) {
                if (it->second.timestamp < oldest->second.timestamp) {
                    oldest = it;
                }
            }
            moderationCache_.erase(oldest);
        }
    }

    moderationCache_[textHash] = entry;
}

// 清理过期缓存
void AIService::cleanExpiredCache() {
    auto now = std::chrono::steady_clock::now();

    for (auto it = moderationCache_.begin(); it != moderationCache_.end();) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
        if (age > moderationCacheTTLSeconds_) {
            it = moderationCache_.erase(it);
        } else {
            ++it;
        }
    }
}

bool AIService::getSentimentFromCache(const std::string& textHash, float& score, std::string& mood) {
    {
        std::shared_lock<std::shared_mutex> readLock(sentimentCacheMutex_);
        auto it = sentimentCache_.find(textHash);
        if (it != sentimentCache_.end()) {
            auto now = std::chrono::steady_clock::now();
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
            if (age <= sentimentCacheTTLSeconds_) {
                score = it->second.score;
                mood = it->second.mood;
                sentimentCacheHits_++;
                return true;
            }
        }
    }

    sentimentCacheMisses_++;

    // 过期删除放到写锁中做，减少读路径锁冲突。
    std::unique_lock<std::shared_mutex> writeLock(sentimentCacheMutex_);
    auto it = sentimentCache_.find(textHash);
    if (it != sentimentCache_.end()) {
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
        if (age > sentimentCacheTTLSeconds_) {
            sentimentCache_.erase(it);
        } else {
            score = it->second.score;
            mood = it->second.mood;
            sentimentCacheHits_++;
            return true;
        }
    }
    return false;
}

void AIService::cleanExpiredSentimentCacheLocked() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = sentimentCache_.begin(); it != sentimentCache_.end();) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
        if (age > sentimentCacheTTLSeconds_) {
            it = sentimentCache_.erase(it);
        } else {
            ++it;
        }
    }
}

void AIService::putSentimentToCache(const std::string& textHash, float score, const std::string& mood) {
    std::unique_lock<std::shared_mutex> lock(sentimentCacheMutex_);

    if (sentimentCache_.size() >= sentimentCacheMaxSize_) {
        cleanExpiredSentimentCacheLocked();
        if (sentimentCache_.size() >= sentimentCacheMaxSize_) {
            auto oldest = sentimentCache_.begin();
            for (auto it = sentimentCache_.begin(); it != sentimentCache_.end(); ++it) {
                if (it->second.timestamp < oldest->second.timestamp) {
                    oldest = it;
                }
            }
            sentimentCache_.erase(oldest);
        }
    }

    SentimentCacheEntry entry;
    entry.score = score;
    entry.mood = canonicalizeMood(mood);
    entry.timestamp = std::chrono::steady_clock::now();
    sentimentCache_[textHash] = std::move(entry);
}

bool AIService::registerSentimentInFlight(
    const std::string& textHash,
    std::function<void(float, const std::string&, const std::string&)> callback
) {
    std::lock_guard<std::mutex> lock(sentimentInFlightMutex_);
    auto it = sentimentInFlight_.find(textHash);
    if (it != sentimentInFlight_.end()) {
        it->second.push_back(std::move(callback));
        sentimentCoalesced_++;
        return false;
    }

    std::vector<std::function<void(float, const std::string&, const std::string&)>> callbacks;
    callbacks.reserve(2);
    callbacks.push_back(std::move(callback));
    sentimentInFlight_.emplace(textHash, std::move(callbacks));
    return true;
}

void AIService::completeSentimentInFlight(
    const std::string& textHash,
    float score,
    const std::string& mood,
    const std::string& error
) {
    std::vector<std::function<void(float, const std::string&, const std::string&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(sentimentInFlightMutex_);
        auto it = sentimentInFlight_.find(textHash);
        if (it == sentimentInFlight_.end()) {
            return;
        }
        callbacks = std::move(it->second);
        sentimentInFlight_.erase(it);
    }

    for (auto& cb : callbacks) {
        try {
            cb(score, mood, error);
        } catch (const std::exception& e) {
            LOG_ERROR << "Sentiment callback threw exception: " << e.what();
        } catch (...) {
            LOG_ERROR << "Sentiment callback threw non-std exception";
        }
    }
}

size_t AIService::currentSentimentInFlightCount() {
    std::lock_guard<std::mutex> lock(sentimentInFlightMutex_);
    return sentimentInFlight_.size();
}

// 清除审核缓存
void AIService::clearModerationCache() {
    std::lock_guard<std::mutex> lock(moderationCacheMutex_);
    moderationCache_.clear();
    LOG_INFO << "Moderation cache cleared";
}

// 获取AI服务统计信息
AIService::AIStats AIService::getStats() const {
    AIStats stats;
    stats.totalAPICalls = totalAPICalls_.load();
    stats.totalLocalCalls = totalLocalCalls_.load();
    stats.moderationCacheHits = moderationCacheHits_.load();
    stats.moderationCacheMisses = moderationCacheMisses_.load();

    size_t total = stats.moderationCacheHits + stats.moderationCacheMisses;
    stats.moderationCacheHitRate = total > 0 ? static_cast<float>(stats.moderationCacheHits) / total : 0.0f;

    // 估算成本（DeepSeek价格：¥0.001/次）
    stats.totalCost = stats.totalAPICalls * 1;  // 单位：分

    return stats;
}

void AIService::analyzeSentimentBatch(
    const std::vector<std::string>& texts,
    std::function<void(const std::vector<std::pair<float, std::string>>& results, const std::string& error)> callback
) {
    std::vector<std::pair<float, std::string>> results;
    results.reserve(texts.size());

    // 优先使用 EdgeAIEngine 三层融合分析
    auto& edgeAI = heartlake::ai::EdgeAIEngine::getInstance();
    if (edgeAI.isEnabled()) {
        for (const auto& text : texts) {
            auto result = edgeAI.analyzeSentimentLocal(text);
            results.emplace_back(result.score, result.mood);
        }
    } else {
        auto& emotionMgr = heartlake::emotion::EmotionManager::getInstance();
        for (const auto& text : texts) {
            auto [score, mood] = emotionMgr.analyzeEmotionFromText(text);
            results.emplace_back(score, mood);
        }
    }

    callback(results, "");
}

void AIService::moderateImage(
    const std::string& imageUrl,
    std::function<void(bool passed, const std::vector<std::string>& categories,
                      float confidence, const std::string& reason)> callback
) {
    (void)imageUrl;
    // 媒体审核链路未就绪，默认拦截待人工复核
    callback(false, {"media_disabled"}, 0.0f, "图片审核服务不可用，需人工复核");
}

SemanticCacheStats AIService::getSemanticCacheStats() const {
    return SemanticCache::getInstance().getStats();
}

} // namespace ai
} // namespace heartlake
