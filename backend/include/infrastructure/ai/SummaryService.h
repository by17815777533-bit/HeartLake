/**
 * @file SummaryService.h
 * @brief AI智能摘要服务 - 使用千问API为长文本生成摘要
 */

#pragma once

#include <string>
#include <atomic>
#include <functional>
#include <json/json.h>

namespace heartlake {
namespace ai {

class SummaryService {
public:
    static SummaryService& getInstance();

    void initialize(const Json::Value& config);

    // 异步生成摘要，超过100字的文本自动生成
    void generateSummary(
        const std::string& stoneId,
        const std::string& content,
        std::function<void(const std::string& summary, const std::string& error)> callback
    );

    // 获取缓存的摘要
    void getCachedSummary(
        const std::string& stoneId,
        std::function<void(const std::string& summary, bool exists)> callback
    );

    static constexpr size_t MIN_LENGTH = 100;  // 最小触发长度
    static constexpr int CACHE_TTL = 86400 * 7;  // 缓存7天

private:
    SummaryService() = default;

    std::string makeCacheKey(const std::string& stoneId);

    std::atomic<bool> initialized_{false};  ///< 多线程读写，必须原子
};

} // namespace ai
} // namespace heartlake
