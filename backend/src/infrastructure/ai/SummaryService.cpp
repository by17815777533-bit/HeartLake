/**
 * @file SummaryService.cpp
 * @brief AI智能摘要服务实现
 */
#include "infrastructure/ai/SummaryService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/cache/RedisCache.h"
#include <drogon/drogon.h>

namespace heartlake {
namespace ai {

SummaryService& SummaryService::getInstance() {
    static SummaryService instance;
    return instance;
}

void SummaryService::initialize(const Json::Value& /*config*/) {
    initialized_ = true;
    LOG_INFO << "SummaryService initialized";
}

std::string SummaryService::makeCacheKey(const std::string& stoneId) {
    return std::string(cache::RedisCache::AI_CACHE) + "summary:" + stoneId;
}

void SummaryService::getCachedSummary(
    const std::string& stoneId,
    std::function<void(const std::string& summary, bool exists)> callback
) {
    auto key = makeCacheKey(stoneId);
    cache::RedisCache::getInstance().get(key, callback);
}

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

            std::string prompt = "请为以下内容生成一个简短摘要，不超过50字：\n" + content;
            AIService::getInstance().generateReply(prompt, "",
                [callback, key](const std::string& response, const std::string& error) {
                    if (!error.empty()) {
                        callback("", error);
                        return;
                    }
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
