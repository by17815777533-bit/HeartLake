/**
 * AI 智能摘要服务
 *
 * 对超过 100 字的长文本异步调用千问 API 生成摘要，
 * 结果缓存 7 天（Redis key 前缀 "summary:"），避免重复调用。
 *
 * 线程安全：initialized_ 使用 atomic，缓存操作委托给 Redis 异步接口。
 */

#pragma once

#include <string>
#include <atomic>
#include <functional>
#include <json/json.h>

namespace heartlake {
namespace ai {

/**
 * @brief AI 智能摘要服务，对长文本异步调用千问 API 生成摘要
 *
 * @details 单例模式。当文本超过 MIN_LENGTH (100字) 时自动触发摘要生成，
 * 结果缓存到 Redis（key 前缀 "summary:"，TTL 7天），避免重复调用。
 * 线程安全：initialized_ 使用 atomic，缓存操作委托给 Redis 异步接口。
 */
class SummaryService {
public:
    /** @brief 获取全局单例 */
    static SummaryService& getInstance();

    /**
     * @brief 初始化服务，读取千问 API 配置
     * @param config JSON 配置（api_key, base_url 等）
     */
    void initialize(const Json::Value& config);

    /**
     * @brief 异步生成文本摘要
     * @details 仅当 content 长度超过 MIN_LENGTH 时才触发 API 调用，
     *          否则直接回调空摘要。生成结果自动写入 Redis 缓存。
     * @param stoneId 石头 ID，用于构建缓存 key
     * @param content 原始文本内容
     * @param callback 完成回调，summary 为摘要文本，error 非空表示失败
     */
    void generateSummary(
        const std::string& stoneId,
        const std::string& content,
        std::function<void(const std::string& summary, const std::string& error)> callback
    );

    /**
     * @brief 从 Redis 获取已缓存的摘要
     * @param stoneId 石头 ID
     * @param callback 完成回调，exists 为 true 时 summary 有效
     */
    void getCachedSummary(
        const std::string& stoneId,
        std::function<void(const std::string& summary, bool exists)> callback
    );

    static constexpr size_t MIN_LENGTH = 100;    ///< 触发摘要生成的最小文本长度
    static constexpr int CACHE_TTL = 86400 * 7;  ///< Redis 缓存过期时间（7天，单位秒）

private:
    SummaryService() = default;

    /**
     * @brief 构建 Redis 缓存 key
     * @param stoneId 石头 ID
     * @return "summary:{stoneId}" 格式的 key
     */
    std::string makeCacheKey(const std::string& stoneId);

    std::atomic<bool> initialized_{false};  ///< 初始化标记，多线程读写需原子操作
};

} // namespace ai
} // namespace heartlake
