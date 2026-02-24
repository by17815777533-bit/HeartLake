/**
 * @file ContentFilter.h
 * @brief ContentFilter 模块接口定义
 * Created by 林子怡
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "utils/HighPerformance.h"

namespace heartlake {

using namespace perf;  // 使用高性能组件

/**
 * 高性能内容过滤器 v2.0
 * 使用优化的AC自动机进行O(n)多模式匹配
 * 计数布隆过滤器预检测，分片LRU缓存热点结果
 */
/**
 * @brief 内容过滤器，用于过滤不适当内容
 *
 * 详细说明
 *
 * @note 注意事项
 */
class ContentFilter {
public:
    static ContentFilter& getInstance() {
        static ContentFilter instance;
        return instance;
    }

    /**
     * @brief initialize方法
     */
    void initialize();

    /**
     * @brief 热更新敏感词库
     * @param words 词条列表: {word, category, level}
     */
    void reload(const std::vector<std::tuple<std::string, uint8_t, uint8_t>>& words);

    /**
     * @brief 规范化文本，移除干扰字符
     */
    static std::string normalize(const std::string& text);

    /**
     * @brief addWord方法
     *
     * @param word 参数说明
     * @param category 参数说明
     * @param level 参数说明
     */
    void addWord(const std::string& word, uint8_t category = 0, uint8_t level = 2);

    static std::string checkContentSafety(const std::string& content);

    std::string checkSafety(const std::string& content);

    std::vector<ACAutomaton::Match> getMatchedWords(const std::string& content);

    std::string getPattern(uint16_t id) const;

    /**
     * @brief containsHighRiskWords方法
     *
     * @param content 参数说明
     * @return 返回值说明
     */
    static bool containsHighRiskWords(const std::string& content);

    static std::string getMentalHealthTip();

    /**
     * @brief cacheSize方法
     * @return 返回值说明
     */
    size_t cacheSize() const { return resultCache_.size(); }
    /**
     * @brief wordCount方法
     * @return 返回值说明
     */
    size_t wordCount() const { return wordCount_; }

private:
    ContentFilter() = default;
    ContentFilter(const ContentFilter&) = delete;
    ContentFilter& operator=(const ContentFilter&) = delete;

    bool isWhitelisted(const std::string& text, const ACAutomaton::Match& match) const;

    ACAutomaton acAutomaton_;
    CountingBloomFilter<> bloomFilter_;
    ShardedLRUCache<std::string, std::string> resultCache_;
    std::unordered_set<std::string> whitelist_;
    std::mutex initMutex_;
    bool initialized_ = false;
    size_t wordCount_ = 0;
};

} // namespace heartlake
