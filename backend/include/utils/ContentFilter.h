/**
 * @file ContentFilter.h
 * @brief ContentFilter 模块接口定义
 * Created by 林子怡
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <memory>
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
    void initialize() {
        std::lock_guard<std::mutex> lock(initMutex_);
        if (initialized_) return;

        addWord("自杀", 1, 3);
        addWord("自残", 1, 3);
        addWord("结束生命", 1, 3);
        addWord("不想活", 1, 3);
        addWord("想死", 1, 3);
        addWord("去死", 1, 3);
        addWord("跳楼", 1, 3);
        addWord("割腕", 1, 3);
        addWord("活不下去", 1, 3);
        addWord("没有意义", 1, 2);

        addWord("杀人", 2, 2);
        addWord("打死", 2, 2);
        addWord("弄死", 2, 2);

        addWord("约炮", 3, 2);
        addWord("一夜情", 3, 2);

        acAutomaton_.build();
        initialized_ = true;
    }

    /**
     * @brief 热更新敏感词库
     * @param words 词条列表: {word, category, level}
     */
    void reload(const std::vector<std::tuple<std::string, uint8_t, uint8_t>>& words) {
        std::lock_guard<std::mutex> lock(initMutex_);
        acAutomaton_.clear();
        bloomFilter_.clear();
        resultCache_.clear();
        wordCount_ = 0;

        for (const auto& [word, cat, lvl] : words) {
            addWord(word, cat, lvl);
        }
        acAutomaton_.build();
        initialized_ = true;
    }

    /**
     * @brief 规范化文本，移除干扰字符
     */
    static std::string normalize(const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (unsigned char c : text) {
            if (c == ' ' || c == '*' || c == '_' || c == '-' || c == '.') continue;
            result += c;
        }
        return result;
    }

    /**
     * @brief addWord方法
     *
     * @param word 参数说明
     * @param category 参数说明
     * @param level 参数说明
     */
    void addWord(const std::string& word, uint8_t category = 0, uint8_t level = 2) {
        bloomFilter_.insert(word);
        acAutomaton_.addPattern(word, category, level);
        ++wordCount_;
    }

    static std::string checkContentSafety(const std::string& content) {
        return getInstance().checkSafety(content);
    }
    
    std::string checkSafety(const std::string& content) {
        if (content.empty()) return "empty";
        if (!initialized_) initialize();

        auto cached = resultCache_.get(content);
        if (cached) return *cached;

        std::string normalized = normalize(content);
        if (!acAutomaton_.hasMatch(content) && !acAutomaton_.hasMatch(normalized)) {
            resultCache_.put(content, "safe");
            return "safe";
        }

        auto matches = acAutomaton_.match(content);
        
        std::string result = "low_risk";
        for (const auto& m : matches) {
            if (m.level == 3) {  // critical
                result = "high_risk";
                break;
            } else if (m.level == 2) {  // high
                result = "medium_risk";
            }
        }

        resultCache_.put(content, result);
        return result;
    }

    std::vector<ACAutomaton::Match> getMatchedWords(const std::string& content) {
        if (!initialized_) initialize();
        return acAutomaton_.match(content);
    }

    std::string getPattern(uint16_t id) const {
        return acAutomaton_.getPattern(id);
    }

    /**
     * @brief containsHighRiskWords方法
     *
     * @param content 参数说明
     * @return 返回值说明
     */
    static bool containsHighRiskWords(const std::string& content) {
        return getInstance().checkSafety(content) == "high_risk";
    }
    
    static std::string getMentalHealthTip() {
        return "如果你正在经历严重的心理困扰，请拨打心理援助热线：\n"
               "全国心理援助热线：400-161-9995\n"
               "希望24小时热线：4001619995\n"
               "北京心理危机研究与干预中心：010-82951332\n"
               "生命热线：400-821-1215\n\n"
               "你不是一个人，我们在这里陪伴你。💙";
    }

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

    ACAutomaton acAutomaton_;
    CountingBloomFilter<> bloomFilter_;
    ShardedLRUCache<std::string, std::string> resultCache_;
    std::mutex initMutex_;
    bool initialized_ = false;
    size_t wordCount_ = 0;
};

} // namespace heartlake
