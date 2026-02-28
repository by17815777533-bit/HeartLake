/**
 * @file SummaryService.cpp
 * @brief AI 智能摘要服务 —— 为长文本石头自动生成精简摘要
 *
 * 采用 cache-aside 策略：先查 Redis 缓存，命中则直接返回；
 * 未命中时调用 AIService 生成摘要并回写缓存，TTL 由 CACHE_TTL 控制。
 * 短文本（< MIN_LENGTH）直接跳过，避免无意义的 API 调用。
 */
#include "infrastructure/ai/SummaryService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/cache/RedisCache.h"
#include <drogon/drogon.h>

namespace heartlake {
namespace ai {

/// Meyer's Singleton，线程安全（C++11 保证 static local 初始化只执行一次）
SummaryService& SummaryService::getInstance() {
    static SummaryService instance;
    return instance;
}

void SummaryService::initialize(const Json::Value& /*config*/) {
    initialized_ = true;
    LOG_INFO << "SummaryService initialized";
}

/// 拼接 Redis 缓存键：统一前缀 + "summary:" + stoneId
std::string SummaryService::makeCacheKey(const std::string& stoneId) {
    return std::string(cache::RedisCache::AI_CACHE) + "summary:" + stoneId;
}

/// 纯缓存读取，不触发生成逻辑；用于前端预取场景
void SummaryService::getCachedSummary(
    const std::string& stoneId,
    std::function<void(const std::string& summary, bool exists)> callback
) {
    auto key = makeCacheKey(stoneId);
    cache::RedisCache::getInstance().get(key, callback);
}

/**
 * 生成摘要的完整流程：
 *   1. 短文本直接返回空（不值得摘要）
 *   2. 查 Redis 缓存，命中则回调返回
 *   3. 未命中 → 调用 AIService 生成 → 回写缓存 → 回调返回
 *
 * 整个链路全异步，不阻塞 Drogon 事件循环。
 */
void SummaryService::generateSummary(
    const std::string& stoneId,
    const std::string& content,
    std::function<void(const std::string& summary, const std::string& error)> callback
) {
    if (content.length() < MIN_LENGTH) {
        callback("", "");
        return;
    }

    auto key = makeCacheKey(stoneId);
    cache::RedisCache::getInstance().get(key,
        [content, callback, key](const std::string& cached, bool exists) {
            if (exists && !cached.empty()) {
                callback(cached, "");
                return;
            }

            // 构造 prompt，限制摘要长度在 50 字以内
            std::string prompt = "请为以下内容生成一个简短摘要，不超过50字：\n" + content;
            AIService::getInstance().generateReply(prompt, "",
                [callback, key](const std::string& response, const std::string& error) {
                    if (!error.empty()) {
                        callback("", error);
                        return;
                    }
                    // 生成成功后回写缓存，后续请求直接命中
                    if (!response.empty()) {
                        cache::RedisCache::getInstance().setEx(key, response, CACHE_TTL);
                    }
                    callback(response, "");
                }
            );
        }
    );
}

} // namespace ai
} // namespace heartlake
