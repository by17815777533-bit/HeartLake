/**
 * @file AIService.cpp
 * @brief AIService 模块实现
 * Created by 王璐瑶
 */
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/ai/ImageModerationEngine.h"
#include "utils/ContentFilter.h"
#include "utils/EmotionManager.h"
#include <drogon/HttpClient.h>
#include <drogon/drogon.h>
#include <regex>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace drogon;

namespace heartlake {
namespace ai {

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

    // 初始化语义缓存
    SemanticCache::getInstance().initialize(0.92f, 5000, 86400);

    LOG_INFO << "AIService initialized with provider: " << provider_;
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
    // 使用统一的EmotionManager进行情绪分析
    auto& emotionMgr = heartlake::emotion::EmotionManager::getInstance();
    auto [score, mood] = emotionMgr.analyzeEmotionFromText(text);
    
    // 直接调用回调函数返回结果
    callback(score, mood, "");
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
    payload["max_tokens"] = 200;

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
                Json::Reader reader;
                if (reader.parse(content, parsed)) {
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
    // 生成embedding用于语义缓存
    auto& embeddingEngine = AdvancedEmbeddingEngine::getInstance();
    auto queryEmbedding = embeddingEngine.generateEmbedding(userMessage);

    // 检查语义缓存
    std::string cachedReply;
    if (SemanticCache::getInstance().get(userMessage, queryEmbedding, cachedReply)) {
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

    payload["temperature"] = 0.7;
    payload["max_tokens"] = 500;

    callAIAPI("/v1/chat/completions", payload,
        [callback, userMessage, queryEmbedding](const Json::Value& response, const std::string& error) {
            if (!error.empty()) {
                callback("我现在有点忙，稍后再聊好吗？", error);
                return;
            }

            try {
                std::string reply = response["choices"][0]["message"]["content"].asString();
                if (reply.empty() || reply.find_first_not_of(" \t\n\r") == std::string::npos) {
                    callback("我理解你的感受，有什么想聊的吗？", "");
                    return;
                }
                // 存入语义缓存
                SemanticCache::getInstance().put(userMessage, queryEmbedding, reply);
                callback(reply, "");
            } catch (const std::exception& e) {
                callback("我理解你的感受，有什么想聊的吗？", e.what());
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

    payload["temperature"] = 0.8;
    payload["max_tokens"] = 300;

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

    payload["temperature"] = 0.8;
    payload["max_tokens"] = 200;

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
        callAIAPIWithRetry("/api/chat", ollamaPayload,
            [callback](const Json::Value& ollamaResp, const std::string& error) {
                if (!error.empty()) {
                    callback(Json::Value(), error);
                    return;
                }
                // 转换 Ollama 原生响应为 OpenAI 格式
                Json::Value openaiResp;
                openaiResp["choices"] = Json::arrayValue;
                Json::Value choice;
                choice["index"] = 0;
                choice["message"]["role"] = "assistant";
                choice["message"]["content"] = ollamaResp.get("message", Json::Value()).get("content", "").asString();
                choice["finish_reason"] = ollamaResp.get("done_reason", "stop").asString();
                openaiResp["choices"].append(choice);
                openaiResp["model"] = ollamaResp.get("model", "").asString();
                callback(openaiResp, "");
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
    auto client = HttpClient::newHttpClient(baseUrl_);
    client->setUserAgent("HeartLake/2.0");

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
                callback(Json::Value(), errorMsg);
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
                if (reader.parse(std::string(body), errJson) && errJson.isMember("error")) {
                    errorMsg += ": " + errJson["error"].get("message", "Unknown error").asString();
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

                callback(Json::Value(), errorMsg);
                return;
            }

            auto jsonPtr = response->getJsonObject();
            if (jsonPtr) {
                // 清理 Qwen3 thinking mode 的 <think>...</think> 标签
                auto& json = *jsonPtr;
                if (json.isMember("choices") && json["choices"].isArray()) {
                    for (auto& choice : json["choices"]) {
                        if (choice.isMember("message") && choice["message"].isMember("content")) {
                            std::string content = choice["message"]["content"].asString();
                            static const std::regex thinkRegex("<think>[\\s\\S]*?</think>\\s*");
                            content = std::regex_replace(content, thinkRegex, "");
                            choice["message"]["content"] = content;
                        }
                    }
                }
                callback(json, "");
            } else {
                LOG_ERROR << "Invalid JSON response from AI API";
                callback(Json::Value(), "Invalid JSON response");
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
    return R"(你是「心湖湖神」，一位温暖而智慧的AI陪伴者。

你的特点：
- 善于倾听和理解用户的真实感受
- 用温和、自然的语气交流，像朋友一样
- 提供有洞察力的建议，但不说教
- 在用户需要时给予情感支持和实用建议
- 对于严重的心理问题，会建议寻求专业帮助

回复要求：
- 150-250字，简洁但有温度
- 直接回复，不需要称呼
- 使用自然、口语化的表达
- 可以用emoji增加亲切感（适度）
- 如果用户表达负面情绪，先共情再引导

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
- 120-200字
- 真诚自然，像给朋友写信
- 可以有个人色彩和情感温度
- 避免说教，重在共鸣和陪伴

请直接写回复内容，不需要称呼或落款。)";
}

std::string AIService::buildStoneCommentPrompt([[maybe_unused]] const std::string& content, const std::string& mood) {
    return R"(你是「心湖」的AI陪伴者，看到有人在湖中投下了一颗石头，分享了他们的湖底。

石头作者的情绪状态：)" + mood + R"(

💙 **你的回应方式**：
- 用温暖、真诚的语气给予回应
- 理解并共情对方的感受，让TA感到被看见
- 根据情绪类型给予适当的安慰、鼓励或陪伴
- 如果是负面情绪，先共情再温柔引导
- 如果是正面情绪，真诚地为TA感到开心
- 可以用简单的比喻或诗意的表达增加温度

✨ **回复要求**：
- 80-150字，简短但有温度
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

    auto& emotionMgr = heartlake::emotion::EmotionManager::getInstance();
    for (const auto& text : texts) {
        auto [score, mood] = emotionMgr.analyzeEmotionFromText(text);
        results.emplace_back(score, mood);
    }

    callback(results, "");
}

void AIService::moderateImage(
    const std::string& imageUrl,
    std::function<void(bool passed, const std::vector<std::string>& categories,
                      float confidence, const std::string& reason)> callback
) {
    auto& imageEngine = heartlake::ai::ImageModerationEngine::getInstance();
    imageEngine.moderateImageUrl(imageUrl,
        [callback](const ImageModerationResult& result, const std::string& error) {
            if (!error.empty()) {
                callback(true, {"pending_review"}, 0.0f, error);
                return;
            }
            callback(result.passed, result.categories, result.confidence, result.reason);
        }
    );
}

SemanticCacheStats AIService::getSemanticCacheStats() const {
    return SemanticCache::getInstance().getStats();
}

} // namespace ai
} // namespace heartlake
