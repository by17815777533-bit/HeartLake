/**
 * @file ContentFilter.cpp
 * @brief ContentFilter 模块实现
 * Created by 林子怡
 */

#include "utils/ContentFilter.h"

namespace heartlake {

void ContentFilter::initialize() {
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

void ContentFilter::reload(const std::vector<std::tuple<std::string, uint8_t, uint8_t>>& words) {
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

std::string ContentFilter::normalize(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        // 处理3字节UTF-8序列中的全角字符
        if (c == 0xEF && i + 2 < text.size()) {
            unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
            // 全角 U+FF01-U+FF5E → 半角 U+0021-U+007E
            if (c1 == 0xBC && c2 >= 0x81 && c2 <= 0xBF) {
                char half = static_cast<char>(c2 - 0x60);
                if (half != ' ' && half != '*' && half != '_' && half != '-' && half != '.') {
                    result += half;
                }
                i += 3;
                continue;
            }
            if (c1 == 0xBD && c2 >= 0x80 && c2 <= 0x9E) {
                char half = static_cast<char>(c2 + 0x20);
                if (half != ' ' && half != '*' && half != '_' && half != '-' && half != '.') {
                    result += half;
                }
                i += 3;
                continue;
            }
        }
        // 全角空格 U+3000 = 0xE3 0x80 0x80
        if (c == 0xE3 && i + 2 < text.size()) {
            unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
            if (c1 == 0x80 && c2 == 0x80) {
                i += 3;
                continue;
            }
        }
        // 原有ASCII过滤逻辑
        if (c == ' ' || c == '*' || c == '_' || c == '-' || c == '.') {
            ++i;
            continue;
        }
        result += text[i];
        ++i;
    }
    return result;
}

void ContentFilter::addWord(const std::string& word, uint8_t category, uint8_t level) {
    bloomFilter_.insert(word);
    acAutomaton_.addPattern(word, category, level);
    ++wordCount_;
}

std::string ContentFilter::checkContentSafety(const std::string& content) {
    return getInstance().checkSafety(content);
}

std::string ContentFilter::checkSafety(const std::string& content) {
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
    auto normalizedMatches = acAutomaton_.match(normalized);
    matches.insert(matches.end(), normalizedMatches.begin(), normalizedMatches.end());

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

std::vector<ACAutomaton::Match> ContentFilter::getMatchedWords(const std::string& content) {
    if (!initialized_) initialize();
    return acAutomaton_.match(content);
}

std::string ContentFilter::getPattern(uint16_t id) const {
    return acAutomaton_.getPattern(id);
}

bool ContentFilter::containsHighRiskWords(const std::string& content) {
    return getInstance().checkSafety(content) == "high_risk";
}

std::string ContentFilter::getMentalHealthTip() {
    return "如果你正在经历严重的心理困扰，请拨打心理援助热线：\n"
           "全国心理援助热线：400-161-9995\n"
           "希望24小时热线：4001619995\n"
           "北京心理危机研究与干预中心：010-82951332\n"
           "生命热线：400-821-1215\n\n"
           "你不是一个人，我们在这里陪伴你。💙";
}

} // namespace heartlake
