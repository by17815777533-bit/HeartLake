/**
 * 高性能内容安全过滤器
 *
 * 三级过滤架构实现 O(n) 级别的敏感词检测：
 * 1. CountingBloomFilter 预检测 — 快速排除明显安全的文本，减少后续开销
 * 2. ACAutomaton 多模式匹配 — 对可疑文本执行精确的 Aho-Corasick 匹配
 * 3. ShardedLRUCache 结果缓存 — 热点文本直接命中缓存，避免重复计算
 *
 * 支持运行时热更新敏感词库（reload），无需重启服务。
 * 词条按类别（自伤/暴力/色情等）和级别（低/中/高）分级，
 * 配合白名单机制减少误报。
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "utils/HighPerformance.h"

namespace heartlake {

using namespace perf;

/**
 * 内容过滤器（单例，线程安全）
 *
 * 初始化时加载内置敏感词库并构建 AC 自动机，
 * 后续可通过 reload() 动态替换词库。
 */
class ContentFilter {
public:
    static ContentFilter& getInstance() {
        static ContentFilter instance;
        return instance;
    }

    /// 加载内置敏感词库并构建 AC 自动机
    void initialize();

    /**
     * 热更新敏感词库，替换当前全部词条并重建自动机
     * @param words 词条列表: {word, category, level}
     */
    void reload(const std::vector<std::tuple<std::string, uint8_t, uint8_t>>& words);

    /// 规范化文本：统一全半角、移除零宽字符等干扰，提高匹配召回率
    static std::string normalize(const std::string& text);

    /// 添加单个敏感词到当前词库
    void addWord(const std::string& word, uint8_t category = 0, uint8_t level = 2);

    /// 静态版本的安全检查（使用单例实例）
    static std::string checkContentSafety(const std::string& content);

    /// 检查文本安全性，返回空字符串表示安全，否则返回违规原因
    std::string checkSafety(const std::string& content);

    /// 获取文本中所有匹配的敏感词详情
    std::vector<ACAutomaton::Match> getMatchedWords(const std::string& content);

    /// 根据模式ID获取原始敏感词文本
    std::string getPattern(uint16_t id) const;

    /// 检测是否包含高风险词汇（自伤/暴力等），用于触发安全港机制
    static bool containsHighRiskWords(const std::string& content);

    /// 获取心理健康提示语，用于高风险内容的温和引导
    static std::string getMentalHealthTip();

    size_t cacheSize() const { return resultCache_.size(); }
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
