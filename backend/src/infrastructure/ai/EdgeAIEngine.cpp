/**
 * @file EdgeAIEngine.cpp
 * @brief 边缘AI推理引擎 - 完整实现
 *
 * 八大核心子系统完整实现：
 * 1. 轻量级情感分析（规则+词典+统计模型三层融合）
 * 2. 本地文本审核（AC自动机多模式匹配+语义规则）
 * 3. 实时情绪脉搏检测（滑动窗口统计）
 * 4. 联邦学习聚合器（FedAvg加权聚合）
 * 5. 差分隐私噪声注入（Laplace机制+隐私预算追踪）
 * 6. 本地向量相似度检索（HNSW近似最近邻）
 * 7. 模型量化推理模拟（INT8量化/反量化）
 * 8. 边缘节点健康监控与自适应负载均衡
 *
 * Created by 王璐瑶
 */

#include "infrastructure/ai/EdgeAIEngine.h"
#include "utils/PsychologicalRiskAssessment.h"
#include <trantor/utils/Logger.h>
#include <cstring>
#include <sstream>
#include <cassert>
#include <set>
#include <limits>

namespace heartlake {
namespace ai {

// ============================================================================
// 单例 & 初始化
// ============================================================================

EdgeAIEngine& EdgeAIEngine::getInstance() {
    static EdgeAIEngine instance;
    return instance;
}

void EdgeAIEngine::initialize(const Json::Value& config) {
    LOG_INFO << "[EdgeAI] Initializing Edge AI Engine...";

    enabled_ = config.get("enabled", true).asBool();
    if (!enabled_) {
        LOG_WARN << "[EdgeAI] Edge AI Engine is disabled by configuration";
        return;
    }

    // HNSW参数配置
    hnswM_ = config.get("hnsw_m", 16).asInt();
    hnswMMax0_ = config.get("hnsw_mmax0", 32).asInt();
    hnswEfConstruction_ = config.get("hnsw_ef_construction", 200).asInt();
    hnswEfSearch_ = config.get("hnsw_ef_search", 50).asInt();
    hnswLevelMult_ = 1.0f / std::log(std::max(2.0f, static_cast<float>(hnswM_)));
    hnswRng_.seed(std::random_device{}());

    // 差分隐私配置
    dpConfig_.epsilon = config.get("dp_epsilon", 1.0f).asFloat();
    dpConfig_.delta = config.get("dp_delta", 1e-5f).asFloat();
    dpConfig_.sensitivity = config.get("dp_sensitivity", 1.0f).asFloat();
    dpConfig_.maxEpsilonBudget = config.get("dp_max_budget", 10.0f).asFloat();
    consumedEpsilon_.store(0.0f);
    dpRng_.seed(std::random_device{}());

    // 情绪脉搏配置
    pulseWindowSeconds_ = config.get("pulse_window_seconds", 300).asInt();
    maxPulseHistory_ = config.get("max_pulse_history", 100).asInt();

    // 加载情感词典
    loadEdgeSentimentLexicon();

    // 构建审核AC自动机
    buildModerationAC();

    // 清空HNSW索引（重新初始化时需要）
    {
        std::unique_lock<std::shared_mutex> lock(hnswMutex_);
        hnswNodes_.clear();
        hnswIdMap_.clear();
        hnswEntryPoint_ = -1;
        hnswMaxLevel_ = 0;
    }

    // 清空情绪脉搏数据
    {
        std::unique_lock<std::shared_mutex> lock(pulseMutex_);
        emotionWindow_.clear();
        pulseHistory_.clear();
    }

    // 清空节点注册表
    {
        std::unique_lock<std::shared_mutex> lock(nodeMutex_);
        nodeRegistry_.clear();
    }

    initialized_ = true;
    LOG_INFO << "[EdgeAI] Edge AI Engine initialized successfully";
}

bool EdgeAIEngine::isEnabled() const {
    return enabled_ && initialized_;
}

// ============================================================================
// 子系统1: 轻量级情感分析
// ============================================================================

void EdgeAIEngine::loadEdgeSentimentLexicon() {
    // 正面情感词典
    sentimentLexicon_["happy"] = 0.8f;
    sentimentLexicon_["love"] = 0.9f;
    sentimentLexicon_["great"] = 0.7f;
    sentimentLexicon_["wonderful"] = 0.85f;
    sentimentLexicon_["excellent"] = 0.9f;
    sentimentLexicon_["amazing"] = 0.85f;
    sentimentLexicon_["fantastic"] = 0.85f;
    sentimentLexicon_["beautiful"] = 0.75f;
    sentimentLexicon_["good"] = 0.6f;
    sentimentLexicon_["nice"] = 0.55f;
    sentimentLexicon_["awesome"] = 0.8f;
    sentimentLexicon_["joy"] = 0.85f;
    sentimentLexicon_["delight"] = 0.8f;
    sentimentLexicon_["pleased"] = 0.65f;
    sentimentLexicon_["grateful"] = 0.75f;
    sentimentLexicon_["thankful"] = 0.7f;
    sentimentLexicon_["excited"] = 0.8f;
    sentimentLexicon_["cheerful"] = 0.75f;
    sentimentLexicon_["brilliant"] = 0.8f;
    sentimentLexicon_["perfect"] = 0.9f;

    // 中文正面词
    sentimentLexicon_["开心"] = 0.8f;
    sentimentLexicon_["快乐"] = 0.85f;
    sentimentLexicon_["喜欢"] = 0.7f;
    sentimentLexicon_["爱"] = 0.9f;
    sentimentLexicon_["棒"] = 0.75f;
    sentimentLexicon_["好"] = 0.5f;
    sentimentLexicon_["优秀"] = 0.8f;
    sentimentLexicon_["感谢"] = 0.7f;
    sentimentLexicon_["幸福"] = 0.9f;
    sentimentLexicon_["温暖"] = 0.7f;
    sentimentLexicon_["美好"] = 0.75f;
    sentimentLexicon_["精彩"] = 0.8f;
    sentimentLexicon_["赞"] = 0.7f;
    sentimentLexicon_["厉害"] = 0.7f;
    sentimentLexicon_["感动"] = 0.75f;

    // 负面情感词典
    sentimentLexicon_["sad"] = -0.7f;
    sentimentLexicon_["angry"] = -0.8f;
    sentimentLexicon_["hate"] = -0.9f;
    sentimentLexicon_["terrible"] = -0.85f;
    sentimentLexicon_["horrible"] = -0.85f;
    sentimentLexicon_["awful"] = -0.8f;
    sentimentLexicon_["bad"] = -0.6f;
    sentimentLexicon_["disgusting"] = -0.85f;
    sentimentLexicon_["fear"] = -0.7f;
    sentimentLexicon_["scared"] = -0.65f;
    sentimentLexicon_["worried"] = -0.5f;
    sentimentLexicon_["anxious"] = -0.55f;
    sentimentLexicon_["depressed"] = -0.85f;
    sentimentLexicon_["miserable"] = -0.8f;
    sentimentLexicon_["frustrated"] = -0.65f;
    sentimentLexicon_["disappointed"] = -0.6f;
    sentimentLexicon_["upset"] = -0.65f;
    sentimentLexicon_["lonely"] = -0.6f;
    sentimentLexicon_["painful"] = -0.7f;
    sentimentLexicon_["annoyed"] = -0.55f;

    // 中文负面词
    sentimentLexicon_["难过"] = -0.7f;
    sentimentLexicon_["伤心"] = -0.75f;
    sentimentLexicon_["生气"] = -0.7f;
    sentimentLexicon_["愤怒"] = -0.85f;
    sentimentLexicon_["讨厌"] = -0.7f;
    sentimentLexicon_["恨"] = -0.9f;
    sentimentLexicon_["害怕"] = -0.65f;
    sentimentLexicon_["恐惧"] = -0.75f;
    sentimentLexicon_["焦虑"] = -0.6f;
    sentimentLexicon_["抑郁"] = -0.85f;
    sentimentLexicon_["痛苦"] = -0.8f;
    sentimentLexicon_["失望"] = -0.65f;
    sentimentLexicon_["烦"] = -0.5f;
    sentimentLexicon_["累"] = -0.4f;
    sentimentLexicon_["孤独"] = -0.6f;

    // 程度副词（乘数因子）
    intensifiers_["very"] = 1.5f;
    intensifiers_["extremely"] = 2.0f;
    intensifiers_["really"] = 1.4f;
    intensifiers_["so"] = 1.3f;
    intensifiers_["quite"] = 1.2f;
    intensifiers_["absolutely"] = 1.8f;
    intensifiers_["incredibly"] = 1.7f;
    intensifiers_["slightly"] = 0.6f;
    intensifiers_["somewhat"] = 0.7f;
    intensifiers_["a bit"] = 0.5f;
    intensifiers_["非常"] = 1.5f;
    intensifiers_["特别"] = 1.6f;
    intensifiers_["超"] = 1.5f;
    intensifiers_["太"] = 1.5f;
    intensifiers_["极其"] = 1.8f;
    intensifiers_["有点"] = 0.6f;
    intensifiers_["稍微"] = 0.5f;
    intensifiers_["真"] = 1.3f;
    intensifiers_["好"] = 1.2f;

    // 否定词（翻转因子）
    negators_["not"] = -1.0f;
    negators_["no"] = -1.0f;
    negators_["never"] = -1.0f;
    negators_["neither"] = -1.0f;
    negators_["don't"] = -1.0f;
    negators_["doesn't"] = -1.0f;
    negators_["didn't"] = -1.0f;
    negators_["won't"] = -1.0f;
    negators_["wouldn't"] = -1.0f;
    negators_["couldn't"] = -1.0f;
    negators_["shouldn't"] = -1.0f;
    negators_["isn't"] = -1.0f;
    negators_["aren't"] = -1.0f;
    negators_["wasn't"] = -1.0f;
    negators_["不"] = -1.0f;
    negators_["没"] = -1.0f;
    negators_["没有"] = -1.0f;
    negators_["别"] = -1.0f;
    negators_["未"] = -1.0f;
    negators_["无"] = -1.0f;

    LOG_INFO << "[EdgeAI] Sentiment lexicon loaded: "
             << sentimentLexicon_.size() << " words, "
             << intensifiers_.size() << " intensifiers, "
             << negators_.size() << " negators";
}

std::vector<std::string> EdgeAIEngine::tokenizeUTF8(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string current;

    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);

        // ASCII字母/数字 -> 累积为英文token
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '\'') {
            current += static_cast<char>(std::tolower(c));
            ++i;
            continue;
        }

        // 遇到非字母数字，先flush当前英文token
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }

        // 判断UTF-8多字节字符（中文等）
        if (c >= 0xC0) {
            int byteCount = 1;
            if ((c & 0xE0) == 0xC0) byteCount = 2;
            else if ((c & 0xF0) == 0xE0) byteCount = 3;
            else if ((c & 0xF8) == 0xF0) byteCount = 4;

            if (i + byteCount <= text.size()) {
                // 每个中文字符作为独立token
                std::string utf8Char = text.substr(i, byteCount);
                tokens.push_back(utf8Char);

                // 同时尝试提取2-3字的中文词组
                size_t j = i + byteCount;
                for (int wordLen = 2; wordLen <= 3 && j < text.size(); ++wordLen) {
                    unsigned char nc = static_cast<unsigned char>(text[j]);
                    if (nc < 0xC0) break;
                    int nb = 1;
                    if ((nc & 0xE0) == 0xC0) nb = 2;
                    else if ((nc & 0xF0) == 0xE0) nb = 3;
                    else if ((nc & 0xF8) == 0xF0) nb = 4;
                    if (j + nb > text.size()) break;

                    std::string compound = text.substr(i, (j + nb) - i);
                    // 只有在词典中存在的组合才加入
                    if (sentimentLexicon_.count(compound) ||
                        intensifiers_.count(compound) ||
                        negators_.count(compound)) {
                        tokens.push_back(compound);
                    }
                    j += nb;
                }
            }
            i += byteCount;
        } else {
            // 空格、标点等跳过
            ++i;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

float EdgeAIEngine::ruleSentiment(const std::string& text) const {
    float score = 0.0f;
    int signals = 0;

    // 表情符号检测
    // 正面表情
    const std::vector<std::string> positiveEmojis = {
        "😊", "😄", "😃", "🥰", "❤", "💕", "👍", "🎉", "✨", "😁",
        "🤗", "💪", "🌟", "😍", "🥳", "💖", "😘", "🙂", "☺", "💯"
    };
    for (const auto& emoji : positiveEmojis) {
        size_t pos = 0;
        while ((pos = text.find(emoji, pos)) != std::string::npos) {
            score += 0.3f;
            ++signals;
            pos += emoji.size();
        }
    }

    // 负面表情
    const std::vector<std::string> negativeEmojis = {
        "😢", "😭", "😡", "😠", "💔", "😞", "😔", "😤", "😰", "😱",
        "🤮", "👎", "😩", "😫", "😣", "😖", "😿", "🥺", "😥", "😓"
    };
    for (const auto& emoji : negativeEmojis) {
        size_t pos = 0;
        while ((pos = text.find(emoji, pos)) != std::string::npos) {
            score -= 0.3f;
            ++signals;
            pos += emoji.size();
        }
    }

    // 标点模式分析
    int exclamationCount = 0;
    int questionCount = 0;
    int ellipsisCount = 0;
    for (char c : text) {
        if (c == '!') ++exclamationCount;
        if (c == '?') ++questionCount;
    }
    // 中文标点
    {
        size_t pos = 0;
        while ((pos = text.find("！", pos)) != std::string::npos) {
            ++exclamationCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("？", pos)) != std::string::npos) {
            ++questionCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("...", pos)) != std::string::npos) {
            ++ellipsisCount;
            pos += 3;
        }
    }
    {
        size_t pos = 0;
        while ((pos = text.find("……", pos)) != std::string::npos) {
            ++ellipsisCount;
            pos += 6;
        }
    }

    // 感叹号增强情感强度
    if (exclamationCount > 0) {
        score += 0.1f * std::min(exclamationCount, 5);
        ++signals;
    }
    // 省略号暗示犹豫/消极
    if (ellipsisCount > 0) {
        score -= 0.1f * std::min(ellipsisCount, 3);
        ++signals;
    }

    // 全大写检测（英文，表示强烈情感）
    int upperCount = 0;
    int letterCount = 0;
    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            ++letterCount;
            if (std::isupper(static_cast<unsigned char>(c))) ++upperCount;
        }
    }
    if (letterCount > 5 && upperCount > letterCount * 0.7) {
        // 全大写 -> 强烈情感，放大现有分数
        score *= 1.5f;
        ++signals;
    }

    // 文本颜文字检测
    const std::vector<std::pair<std::string, float>> kaomoji = {
        {"(^_^)", 0.4f}, {"(^^)", 0.3f}, {"(≧▽≦)", 0.5f}, {"(╥_╥)", -0.5f},
        {"(T_T)", -0.4f}, {"(>_<)", -0.3f}, {":)", 0.3f}, {":(", -0.3f},
        {":D", 0.4f}, {";)", 0.2f}, {"XD", 0.4f}, {"orz", -0.3f},
        {"QAQ", -0.4f}, {"TAT", -0.4f}, {"^_^", 0.3f}, {">_<", -0.3f}
    };
    for (const auto& [pattern, val] : kaomoji) {
        if (text.find(pattern) != std::string::npos) {
            score += val;
            ++signals;
        }
    }

    if (signals == 0) return 0.0f;
    return std::clamp(score / std::max(1, signals) * 1.5f, -1.0f, 1.0f);
}

float EdgeAIEngine::lexiconSentiment(const std::vector<std::string>& tokens) const {
    float totalScore = 0.0f;
    int matchedCount = 0;
    float intensifierMult = 1.0f;
    bool negated = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];

        // 检查是否为否定词
        auto negIt = negators_.find(token);
        if (negIt != negators_.end()) {
            negated = true;
            continue;
        }

        // 检查是否为程度副词
        auto intIt = intensifiers_.find(token);
        if (intIt != intensifiers_.end()) {
            intensifierMult = intIt->second;
            continue;
        }

        // 查找情感词典
        auto lexIt = sentimentLexicon_.find(token);
        if (lexIt != sentimentLexicon_.end()) {
            float wordScore = lexIt->second * intensifierMult;
            if (negated) {
                wordScore *= -0.75f;  // 否定翻转，但强度略减
            }
            totalScore += wordScore;
            ++matchedCount;

            // 重置修饰状态
            intensifierMult = 1.0f;
            negated = false;
        } else {
            // 非情感词不立即重置修饰状态
            // 让否定词和程度副词的作用范围延伸到下一个情感词
        }
    }

    if (matchedCount == 0) return 0.0f;
    return std::clamp(totalScore / std::sqrt(static_cast<float>(matchedCount)), -1.0f, 1.0f);
}

float EdgeAIEngine::statisticalSentiment(const std::vector<std::string>& tokens,
                                          const std::string& text) const {
    // 基于文本统计特征的简单情感分类
    float score = 0.0f;
    int features = 0;

    // 特征1: 词汇丰富度（type-token ratio）
    std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
    float ttr = tokens.empty() ? 0.0f :
                static_cast<float>(uniqueTokens.size()) / static_cast<float>(tokens.size());
    // 高丰富度略偏正面（表达丰富）
    if (ttr > 0.8f) { score += 0.1f; ++features; }

    // 特征2: 平均词长
    float avgLen = 0.0f;
    for (const auto& t : tokens) avgLen += static_cast<float>(t.size());
    if (!tokens.empty()) avgLen /= static_cast<float>(tokens.size());
    // 较长的词汇使用暗示更正式/理性的表达
    if (avgLen > 6.0f) { score += 0.05f; ++features; }

    // 特征3: 文本长度特征
    float textLen = static_cast<float>(text.size());
    // 极短文本可能是情绪化表达
    if (textLen < 10.0f) {
        // 短文本情感倾向不确定，不加分
        ++features;
    } else if (textLen > 200.0f) {
        // 长文本通常更理性
        score += 0.05f;
        ++features;
    }

    // 特征4: 正面/负面词比例
    int posCount = 0, negCount = 0;
    for (const auto& t : tokens) {
        auto it = sentimentLexicon_.find(t);
        if (it != sentimentLexicon_.end()) {
            if (it->second > 0) ++posCount;
            else if (it->second < 0) ++negCount;
        }
    }
    int sentimentWords = posCount + negCount;
    if (sentimentWords > 0) {
        float posRatio = static_cast<float>(posCount) / static_cast<float>(sentimentWords);
        score += (posRatio - 0.5f) * 0.6f;
        ++features;
    }

    // 特征5: 重复字符检测（如"哈哈哈哈"、"啊啊啊"）
    int repeatCount = 0;
    for (size_t i = 1; i < text.size(); ++i) {
        if (text[i] == text[i - 1]) ++repeatCount;
    }
    float repeatRatio = text.empty() ? 0.0f :
                        static_cast<float>(repeatCount) / static_cast<float>(text.size());
    if (repeatRatio > 0.3f) {
        // 大量重复可能是强烈情感（正面如"哈哈哈"或负面如"呜呜呜"）
        score *= 1.3f;
        ++features;
    }

    if (features == 0) return 0.0f;
    return std::clamp(score, -1.0f, 1.0f);
}

std::string EdgeAIEngine::scoresToMood(float score) const {
    if (score > 0.6f) return "joy";
    if (score > 0.2f) return "surprise";  // 轻度正面 -> 惊喜
    if (score > -0.2f) return "neutral";
    if (score > -0.5f) return "sadness";
    if (score > -0.7f) return "fear";
    return "anger";
}

EdgeSentimentResult EdgeAIEngine::analyzeSentimentLocal(const std::string& text) {
    ++totalSentimentCalls_;

    if (!isEnabled() || text.empty()) {
        return {0.0f, "neutral", 0.0f, "disabled"};
    }

    // 分词
    auto tokens = tokenizeUTF8(text);

    // 三层分析
    float ruleScore = ruleSentiment(text);
    float lexScore = lexiconSentiment(tokens);
    float statScore = statisticalSentiment(tokens, text);

    // 加权集成：词典权重最高，规则次之，统计辅助
    const float wRule = 0.25f;
    const float wLex = 0.50f;
    const float wStat = 0.25f;

    float ensembleScore = wRule * ruleScore + wLex * lexScore + wStat * statScore;
    ensembleScore = std::clamp(ensembleScore, -1.0f, 1.0f);

    // 计算置信度：基于各层一致性
    float agreement = 1.0f;
    // 如果三层结果符号一致，置信度高
    bool allPositive = (ruleScore >= 0 && lexScore >= 0 && statScore >= 0);
    bool allNegative = (ruleScore <= 0 && lexScore <= 0 && statScore <= 0);
    if (allPositive || allNegative) {
        agreement = 0.8f + 0.2f * std::min({std::abs(ruleScore), std::abs(lexScore), std::abs(statScore)});
    } else {
        // 不一致时降低置信度
        float variance = ((ruleScore - ensembleScore) * (ruleScore - ensembleScore) +
                          (lexScore - ensembleScore) * (lexScore - ensembleScore) +
                          (statScore - ensembleScore) * (statScore - ensembleScore)) / 3.0f;
        agreement = std::max(0.2f, 0.7f - variance);
    }

    // 词典匹配数也影响置信度
    int lexMatches = 0;
    for (const auto& t : tokens) {
        if (sentimentLexicon_.count(t)) ++lexMatches;
    }
    float coverageBoost = std::min(0.2f, static_cast<float>(lexMatches) * 0.05f);
    float confidence = std::clamp(agreement + coverageBoost, 0.0f, 1.0f);

    std::string mood = scoresToMood(ensembleScore);
    std::string method = "ensemble";

    return {ensembleScore, mood, confidence, method};
}

// ============================================================================
// 子系统2: 本地文本审核（AC自动机 + 语义规则）
// ============================================================================

void EdgeAIEngine::buildModerationAC() {
    std::lock_guard<std::mutex> lock(moderationMutex_);

    // category: 1=self_harm, 2=violence, 3=sexual, 0=general_profanity
    // level: 1=low, 2=medium, 3=high

    // 自伤/自杀相关 (category=1, high risk)
    moderationAC_.addPattern("suicide", 1, 3);
    moderationAC_.addPattern("kill myself", 1, 3);
    moderationAC_.addPattern("end my life", 1, 3);
    moderationAC_.addPattern("self harm", 1, 3);
    moderationAC_.addPattern("want to die", 1, 3);
    moderationAC_.addPattern("自杀", 1, 3);
    moderationAC_.addPattern("自残", 1, 3);
    moderationAC_.addPattern("不想活", 1, 3);
    moderationAC_.addPattern("轻生", 1, 3);
    moderationAC_.addPattern("结束生命", 1, 3);

    // 暴力相关 (category=2)
    moderationAC_.addPattern("murder", 2, 3);
    moderationAC_.addPattern("kill you", 2, 3);
    moderationAC_.addPattern("beat you up", 2, 2);
    moderationAC_.addPattern("violence", 2, 2);
    moderationAC_.addPattern("attack", 2, 1);
    moderationAC_.addPattern("杀", 2, 2);
    moderationAC_.addPattern("打死", 2, 3);
    moderationAC_.addPattern("暴力", 2, 2);
    moderationAC_.addPattern("殴打", 2, 2);

    // 不当内容 (category=3)
    moderationAC_.addPattern("porn", 3, 3);
    moderationAC_.addPattern("nude", 3, 2);
    moderationAC_.addPattern("sexual", 3, 2);
    moderationAC_.addPattern("色情", 3, 3);
    moderationAC_.addPattern("裸体", 3, 2);

    // 一般脏话/侮辱 (category=0)
    moderationAC_.addPattern("fuck", 0, 2);
    moderationAC_.addPattern("shit", 0, 1);
    moderationAC_.addPattern("damn", 0, 1);
    moderationAC_.addPattern("idiot", 0, 1);
    moderationAC_.addPattern("stupid", 0, 1);
    moderationAC_.addPattern("傻逼", 0, 2);
    moderationAC_.addPattern("操", 0, 2);
    moderationAC_.addPattern("垃圾", 0, 1);
    moderationAC_.addPattern("白痴", 0, 1);

    moderationAC_.build();
    moderationACBuilt_ = true;

    LOG_INFO << "[EdgeAI] Moderation AC automaton built successfully";
}

float EdgeAIEngine::semanticRiskAnalysis(const std::string& text,
                                          const std::vector<perf::ACAutomaton::Match>& matches) const {
    if (matches.empty()) return 0.0f;

    float maxRisk = 0.0f;
    float totalRisk = 0.0f;

    for (const auto& m : matches) {
        float baseRisk = 0.0f;
        switch (m.level) {
            case 1: baseRisk = 0.3f; break;
            case 2: baseRisk = 0.6f; break;
            case 3: baseRisk = 0.9f; break;
            default: baseRisk = 0.2f; break;
        }

        // 自伤类别额外加权
        if (m.category == 1) {
            baseRisk = std::min(1.0f, baseRisk * 1.2f);
        }

        maxRisk = std::max(maxRisk, baseRisk);
        totalRisk += baseRisk;
    }

    // 多个匹配叠加风险
    float densityFactor = 1.0f + std::log2(static_cast<float>(matches.size()));
    float combinedRisk = maxRisk * 0.6f + (totalRisk / static_cast<float>(matches.size())) * 0.4f;
    combinedRisk *= std::min(densityFactor, 2.0f);

    // 文本长度归一化：短文本中出现敏感词风险更高
    float lengthFactor = 1.0f;
    if (text.size() < 50) {
        lengthFactor = 1.3f;
    } else if (text.size() < 200) {
        lengthFactor = 1.1f;
    } else {
        lengthFactor = 0.9f;
    }

    float acRisk = std::clamp(combinedRisk * lengthFactor, 0.0f, 1.0f);

    // 五因子心理风险评估融合
    auto& pra = utils::PsychologicalRiskAssessment::getInstance();
    auto praResult = pra.assessRisk(text, "", 0.0f, "");
    float fiveFactorScore = praResult.overallScore;

    // 融合策略：取AC匹配风险和五因子风险的最大值
    // 确保任一维度的高风险都不会被低估
    return std::clamp(std::max(acRisk, fiveFactorScore), 0.0f, 1.0f);
}

EdgeModerationResult EdgeAIEngine::moderateTextLocal(const std::string& text) {
    ++totalModerationCalls_;

    EdgeModerationResult result;
    result.passed = true;
    result.riskLevel = "safe";
    result.confidence = 0.9f;
    result.needsAlert = false;
    result.suggestion = "";

    if (!isEnabled() || text.empty()) {
        return result;
    }

    // 转小写用于匹配
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 阶段1: AC自动机快速多模式匹配
    auto matches = moderationAC_.match(lowerText);

    if (matches.empty()) {
        result.passed = true;
        result.riskLevel = "safe";
        result.confidence = 0.85f;
        return result;
    }

    // 收集匹配到的模式和类别
    std::set<std::string> categorySet;
    bool hasSelfHarm = false;
    uint8_t maxLevel = 0;

    for (const auto& m : matches) {
        // 从匹配位置提取模式文本
        result.matchedPatterns.push_back(
            lowerText.substr(std::max(0, m.position - 20), 40));

        switch (m.category) {
            case 0: categorySet.insert("profanity"); break;
            case 1: categorySet.insert("self_harm"); hasSelfHarm = true; break;
            case 2: categorySet.insert("violence"); break;
            case 3: categorySet.insert("sexual"); break;
            default: categorySet.insert("other"); break;
        }
        maxLevel = std::max(maxLevel, m.level);
    }

    result.categories.assign(categorySet.begin(), categorySet.end());

    // 阶段2: 语义风险分析（已融合五因子模型）
    float riskScore = semanticRiskAnalysis(text, matches);

    // 阶段3: 提取五因子详情填充到结果中
    auto& pra = utils::PsychologicalRiskAssessment::getInstance();
    auto praResult = pra.assessRisk(text, "", 0.0f, "");

    FiveFactorRiskDetail fiveFactorDetail;
    fiveFactorDetail.selfHarmIntent = 0.0f;
    fiveFactorDetail.hopelessness = 0.0f;
    fiveFactorDetail.socialIsolation = 0.0f;
    fiveFactorDetail.temporalUrgency = 0.0f;
    fiveFactorDetail.linguisticMarkers = 0.0f;
    fiveFactorDetail.compositeScore = praResult.overallScore;

    // 从 PsychologicalRiskResult 的 factors 中提取各因子分数
    for (const auto& factor : praResult.factors) {
        if (factor.name == "self_harm_intent") {
            fiveFactorDetail.selfHarmIntent = factor.score;
        } else if (factor.name == "hopelessness") {
            fiveFactorDetail.hopelessness = factor.score;
        } else if (factor.name == "social_isolation") {
            fiveFactorDetail.socialIsolation = factor.score;
        } else if (factor.name == "temporal_urgency") {
            fiveFactorDetail.temporalUrgency = factor.score;
        } else if (factor.category == "linguistic") {
            // analyzeLinguisticMarkers 产生的因子
            fiveFactorDetail.linguisticMarkers = std::max(fiveFactorDetail.linguisticMarkers, factor.score);
        }
    }

    result.fiveFactorDetail = fiveFactorDetail;

    // 确定风险等级
    if (riskScore >= 0.8f || maxLevel >= 3) {
        result.riskLevel = "high_risk";
        result.passed = false;
        result.confidence = std::min(0.95f, riskScore);
    } else if (riskScore >= 0.5f || maxLevel >= 2) {
        result.riskLevel = "medium_risk";
        result.passed = false;
        result.confidence = std::min(0.85f, riskScore + 0.1f);
    } else {
        result.riskLevel = "low_risk";
        result.passed = true;
        result.confidence = 0.7f;
    }

    // 自伤检测需要紧急关注（AC匹配或五因子模型检出）
    bool fiveFactorSelfHarm = fiveFactorDetail.selfHarmIntent > 0.5f;
    if (hasSelfHarm || fiveFactorSelfHarm || praResult.needsImmediateAttention) {
        result.needsAlert = true;
        result.passed = false;
        result.riskLevel = "high_risk";
        result.suggestion = "Detected potential self-harm content. "
                           "Please provide crisis support resources and escalate to human moderator.";
        if (!praResult.supportMessage.empty()) {
            result.suggestion += " " + praResult.supportMessage;
        }
    } else if (result.riskLevel == "high_risk") {
        result.suggestion = "High-risk content detected. Recommend blocking and review by human moderator.";
    } else if (result.riskLevel == "medium_risk") {
        result.suggestion = "Medium-risk content detected. Consider flagging for review.";
    }

    return result;
}

// ============================================================================
// 子系统3: 实时情绪脉搏检测
// ============================================================================

void EdgeAIEngine::pruneEmotionWindow() {
    auto now = std::chrono::steady_clock::now();
    auto windowDuration = std::chrono::seconds(pulseWindowSeconds_);

    while (!emotionWindow_.empty()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - emotionWindow_.front().timestamp);
        if (age > windowDuration) {
            emotionWindow_.pop_front();
        } else {
            break;
        }
    }
}

EmotionPulse EdgeAIEngine::computePulseFromWindow() const {
    EmotionPulse pulse;
    pulse.timestamp = std::chrono::steady_clock::now();
    pulse.sampleCount = static_cast<int>(emotionWindow_.size());

    if (emotionWindow_.empty()) {
        pulse.avgScore = 0.0f;
        pulse.stddev = 0.0f;
        pulse.trendSlope = 0.0f;
        pulse.dominantMood = "neutral";
        return pulse;
    }

    // 计算平均分
    float sum = 0.0f;
    std::unordered_map<std::string, int> moodCounts;
    for (const auto& s : emotionWindow_) {
        sum += s.score;
        moodCounts[s.mood]++;
    }
    pulse.avgScore = sum / static_cast<float>(emotionWindow_.size());
    pulse.moodDistribution = moodCounts;

    // 计算标准差
    float varianceSum = 0.0f;
    for (const auto& s : emotionWindow_) {
        float diff = s.score - pulse.avgScore;
        varianceSum += diff * diff;
    }
    pulse.stddev = std::sqrt(varianceSum / static_cast<float>(emotionWindow_.size()));

    // 计算趋势斜率（简单线性回归）
    // y = score, x = 时间序列索引
    if (emotionWindow_.size() >= 2) {
        float n = static_cast<float>(emotionWindow_.size());
        float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
        for (size_t i = 0; i < emotionWindow_.size(); ++i) {
            float x = static_cast<float>(i);
            float y = emotionWindow_[i].score;
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
        }
        float denominator = n * sumX2 - sumX * sumX;
        if (std::abs(denominator) > 1e-6f) {
            pulse.trendSlope = (n * sumXY - sumX * sumY) / denominator;
        } else {
            pulse.trendSlope = 0.0f;
        }
    } else {
        pulse.trendSlope = 0.0f;
    }

    // 找主导情绪
    std::string dominant = "neutral";
    int maxCount = 0;
    for (const auto& [mood, count] : moodCounts) {
        if (count > maxCount) {
            maxCount = count;
            dominant = mood;
        }
    }
    pulse.dominantMood = dominant;

    return pulse;
}

void EdgeAIEngine::submitEmotionSample(float score, const std::string& mood) {
    if (!isEnabled()) return;

    std::unique_lock<std::shared_mutex> lock(pulseMutex_);

    EmotionSample sample;
    sample.score = std::clamp(score, -1.0f, 1.0f);
    sample.mood = mood;
    sample.timestamp = std::chrono::steady_clock::now();

    emotionWindow_.push_back(sample);
    pruneEmotionWindow();

    // 每积累一定样本数，生成一个脉搏快照
    if (emotionWindow_.size() % 10 == 0 && !emotionWindow_.empty()) {
        EmotionPulse pulse = computePulseFromWindow();
        pulseHistory_.push_back(pulse);
        if (static_cast<int>(pulseHistory_.size()) > maxPulseHistory_) {
            pulseHistory_.erase(pulseHistory_.begin());
        }
    }
}

EmotionPulse EdgeAIEngine::getCurrentPulse() {
    std::unique_lock<std::shared_mutex> lock(pulseMutex_);
    pruneEmotionWindow();
    return computePulseFromWindow();
}

std::vector<EmotionPulse> EdgeAIEngine::getPulseHistory(int count) {
    std::shared_lock<std::shared_mutex> lock(pulseMutex_);
    int n = std::min(count, static_cast<int>(pulseHistory_.size()));
    if (n <= 0) return {};
    return std::vector<EmotionPulse>(pulseHistory_.end() - n, pulseHistory_.end());
}

// ============================================================================
// 子系统4: 联邦学习聚合器（FedAvg）
// ============================================================================

void EdgeAIEngine::submitLocalModel(const FederatedModelParams& params) {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    localModels_.push_back(params);
    LOG_DEBUG << "[EdgeAI] Local model submitted from node: " << params.nodeId
              << ", samples: " << params.sampleCount
              << ", loss: " << params.localLoss;
}

FederatedModelParams EdgeAIEngine::aggregateFedAvg() {
    std::lock_guard<std::mutex> lock(federatedMutex_);

    FederatedModelParams global;
    global.modelId = "global_fedavg";
    global.epoch = ++federatedRound_;
    global.nodeId = "aggregator";

    if (localModels_.empty()) {
        LOG_WARN << "[EdgeAI] No local models to aggregate";
        global.sampleCount = 0;
        global.localLoss = 0.0f;
        return global;
    }

    // 计算总样本数
    size_t totalSamples = 0;
    for (const auto& model : localModels_) {
        totalSamples += model.sampleCount;
    }
    if (totalSamples == 0) {
        LOG_WARN << "[EdgeAI] Total samples is 0, using equal weights";
        totalSamples = localModels_.size();  // 等权重回退
    }

    if (totalSamples == 0) {
        LOG_WARN << "[EdgeAI] Total sample count is zero, cannot aggregate";
        global.sampleCount = 0;
        global.localLoss = 0.0f;
        localModels_.clear();
        return global;
    }

    global.sampleCount = totalSamples;

    // 确定权重矩阵维度（以第一个模型为参考）
    const auto& ref = localModels_[0];
    size_t numLayers = ref.weights.size();
    size_t biasSize = ref.biases.size();

    // 初始化全局权重为零
    global.weights.resize(numLayers);
    for (size_t l = 0; l < numLayers; ++l) {
        global.weights[l].resize(ref.weights[l].size(), 0.0f);
    }
    global.biases.resize(biasSize, 0.0f);
    global.localLoss = 0.0f;

    // FedAvg加权聚合: w_global = Σ(n_k / n) * w_k
    for (const auto& model : localModels_) {
        float weight = static_cast<float>(model.sampleCount) / static_cast<float>(totalSamples);

        // 聚合权重矩阵
        for (size_t l = 0; l < numLayers && l < model.weights.size(); ++l) {
            for (size_t j = 0; j < model.weights[l].size() && j < global.weights[l].size(); ++j) {
                global.weights[l][j] += weight * model.weights[l][j];
            }
        }

        // 聚合偏置
        for (size_t j = 0; j < model.biases.size() && j < global.biases.size(); ++j) {
            global.biases[j] += weight * model.biases[j];
        }

        // 加权平均损失
        global.localLoss += weight * model.localLoss;
    }

    LOG_INFO << "[EdgeAI] FedAvg aggregation complete: round=" << federatedRound_
             << ", participants=" << localModels_.size()
             << ", totalSamples=" << totalSamples
             << ", avgLoss=" << global.localLoss;

    // 清空本地模型缓存，准备下一轮
    localModels_.clear();

    return global;
}

Json::Value EdgeAIEngine::getFederatedStatus() const {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    Json::Value status;
    status["current_round"] = federatedRound_;
    status["pending_models"] = static_cast<int>(localModels_.size());

    size_t totalSamples = 0;
    Json::Value nodes(Json::arrayValue);
    for (const auto& model : localModels_) {
        Json::Value node;
        node["node_id"] = model.nodeId;
        node["sample_count"] = static_cast<Json::UInt64>(model.sampleCount);
        node["local_loss"] = model.localLoss;
        node["epoch"] = model.epoch;
        nodes.append(node);
        totalSamples += model.sampleCount;
    }
    status["nodes"] = nodes;
    status["total_pending_samples"] = static_cast<Json::UInt64>(totalSamples);

    return status;
}

// ============================================================================
// 子系统5: 差分隐私引擎（Laplace机制）
// ============================================================================

float EdgeAIEngine::sampleLaplace(float scale) {
    // Laplace分布采样：使用逆CDF方法
    // 如果 U ~ Uniform(0,1)，则 X = μ - b * sign(U-0.5) * ln(1 - 2|U-0.5|)
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float u;
    {
        std::lock_guard<std::mutex> lock(dpMutex_);
        u = dist(dpRng_);
    }

    // 避免 log(0)
    u = std::clamp(u, 1e-7f, 1.0f - 1e-7f);

    float centered = u - 0.5f;
    float sign = (centered >= 0.0f) ? 1.0f : -1.0f;
    float absVal = std::abs(centered);

    return -scale * sign * std::log(1.0f - 2.0f * absVal);
}

float EdgeAIEngine::addLaplaceNoise(float value, float sensitivity) {
    if (!isEnabled()) return value;

    // 检查隐私预算
    float remaining = dpConfig_.maxEpsilonBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[EdgeAI] Privacy budget exhausted, returning original value";
        return value;
    }

    // 使用配置的epsilon，但不超过剩余预算
    float epsilon = std::min(dpConfig_.epsilon, remaining);
    if (epsilon < 1e-10f) epsilon = 1e-10f;  // 防止除零

    // Laplace噪声尺度: b = Δf / ε
    float scale = sensitivity / epsilon;
    float noise = sampleLaplace(scale);

    // 消耗隐私预算（组合定理：线性累加）
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    return value + noise;
}

std::vector<float> EdgeAIEngine::addLaplaceNoiseVec(const std::vector<float>& values,
                                                      float sensitivity) {
    if (!isEnabled()) return values;

    float remaining = dpConfig_.maxEpsilonBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[EdgeAI] Privacy budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(dpConfig_.epsilon, remaining);
    if (epsilon < 1e-10f) epsilon = 1e-10f;  // 防止除零
    float scale = sensitivity / epsilon;

    std::vector<float> result(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        result[i] = values[i] + sampleLaplace(scale);
    }

    // 消耗隐私预算
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    return result;
}

float EdgeAIEngine::getRemainingPrivacyBudget() const {
    float consumed = consumedEpsilon_.load();
    return std::max(0.0f, dpConfig_.maxEpsilonBudget - consumed);
}

void EdgeAIEngine::resetPrivacyBudget() {
    consumedEpsilon_.store(0.0f);
    LOG_INFO << "[EdgeAI] Privacy budget reset. Max budget: " << dpConfig_.maxEpsilonBudget;
}

// ============================================================================
// 子系统6: HNSW向量检索
// ============================================================================

int EdgeAIEngine::randomLevel() {
    // 指数衰减的随机层级生成
    // P(level = l) = (1/M)^l * (1 - 1/M)
    // 注意：调用者必须已持有 hnswMutex_
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(hnswRng_);
    int level = static_cast<int>(-std::log(r) * hnswLevelMult_);
    return std::max(0, level);
}

float EdgeAIEngine::vectorDistance(const std::vector<float>& a,
                                    const std::vector<float>& b) const {
    // L2 (欧氏) 距离的平方 — 避免开方以提升性能
    if (a.size() != b.size()) return std::numeric_limits<float>::max();

    float dist = 0.0f;
    size_t dim = a.size();

    // 4路展开减少循环开销
    size_t i = 0;
    for (; i + 3 < dim; i += 4) {
        float d0 = a[i]     - b[i];
        float d1 = a[i + 1] - b[i + 1];
        float d2 = a[i + 2] - b[i + 2];
        float d3 = a[i + 3] - b[i + 3];
        dist += d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3;
    }
    for (; i < dim; ++i) {
        float d = a[i] - b[i];
        dist += d * d;
    }
    return dist;
}

std::vector<size_t> EdgeAIEngine::searchLayer(const std::vector<float>& query,
                                               size_t entryPoint, int ef,
                                               int level) const {
    // 贪心搜索 + 动态候选列表（beam search 变体）
    // 使用 max-heap 维护结果集（最远的在堆顶，方便淘汰）
    // 使用 min-heap 维护候选集（最近的在堆顶，优先扩展）

    using DistIdx = std::pair<float, size_t>;

    // 结果集：max-heap（距离最大的在顶部）
    std::priority_queue<DistIdx> results;
    // 候选集：min-heap（距离最小的在顶部）
    std::priority_queue<DistIdx, std::vector<DistIdx>, std::greater<DistIdx>> candidates;

    std::unordered_set<size_t> visited;

    float entryDist = vectorDistance(query, hnswNodes_[entryPoint].vector);
    results.push({entryDist, entryPoint});
    candidates.push({entryDist, entryPoint});
    visited.insert(entryPoint);

    while (!candidates.empty()) {
        auto [candDist, candIdx] = candidates.top();
        float worstResult = results.top().first;

        // 如果最近的候选比结果集中最远的还远，停止
        if (candDist > worstResult && static_cast<int>(results.size()) >= ef) {
            break;
        }
        candidates.pop();

        // 扩展该候选的邻居
        if (level < static_cast<int>(hnswNodes_[candIdx].neighbors.size())) {
            for (size_t neighborIdx : hnswNodes_[candIdx].neighbors[level]) {
                if (visited.count(neighborIdx)) continue;
                visited.insert(neighborIdx);

                float neighborDist = vectorDistance(query, hnswNodes_[neighborIdx].vector);
                worstResult = results.top().first;

                if (static_cast<int>(results.size()) < ef || neighborDist < worstResult) {
                    results.push({neighborDist, neighborIdx});
                    candidates.push({neighborDist, neighborIdx});

                    // 保持结果集大小不超过 ef
                    if (static_cast<int>(results.size()) > ef) {
                        results.pop();
                    }
                }
            }
        }
    }

    // 提取结果（按距离从小到大排序）
    std::vector<DistIdx> sorted;
    sorted.reserve(results.size());
    while (!results.empty()) {
        sorted.push_back(results.top());
        results.pop();
    }
    std::sort(sorted.begin(), sorted.end());

    std::vector<size_t> result;
    result.reserve(sorted.size());
    for (const auto& [d, idx] : sorted) {
        result.push_back(idx);
    }
    return result;
}

void EdgeAIEngine::connectNeighbors(size_t nodeIdx,
                                     const std::vector<size_t>& neighbors,
                                     int level, int maxM) {
    // 双向连接：将 nodeIdx 与 neighbors 互相连接
    // 如果邻居数超过 maxM，执行启发式裁剪（保留最近的 maxM 个）

    auto& nodeNeighbors = hnswNodes_[nodeIdx].neighbors[level];
    for (size_t neighborIdx : neighbors) {
        // 添加正向连接
        if (std::find(nodeNeighbors.begin(), nodeNeighbors.end(), neighborIdx)
            == nodeNeighbors.end()) {
            nodeNeighbors.push_back(neighborIdx);
        }

        // 添加反向连接
        if (level < static_cast<int>(hnswNodes_[neighborIdx].neighbors.size())) {
            auto& revNeighbors = hnswNodes_[neighborIdx].neighbors[level];
            if (std::find(revNeighbors.begin(), revNeighbors.end(), nodeIdx)
                == revNeighbors.end()) {
                revNeighbors.push_back(nodeIdx);
            }

            // 裁剪：如果反向邻居超过 maxM，保留最近的
            if (static_cast<int>(revNeighbors.size()) > maxM) {
                // 按距离排序
                std::vector<std::pair<float, size_t>> distPairs;
                distPairs.reserve(revNeighbors.size());
                for (size_t nIdx : revNeighbors) {
                    float d = vectorDistance(hnswNodes_[neighborIdx].vector,
                                             hnswNodes_[nIdx].vector);
                    distPairs.push_back({d, nIdx});
                }
                std::sort(distPairs.begin(), distPairs.end());

                revNeighbors.clear();
                for (int i = 0; i < maxM; ++i) {
                    revNeighbors.push_back(distPairs[i].second);
                }
            }
        }
    }

    // 裁剪正向邻居
    if (static_cast<int>(nodeNeighbors.size()) > maxM) {
        std::vector<std::pair<float, size_t>> distPairs;
        distPairs.reserve(nodeNeighbors.size());
        for (size_t nIdx : nodeNeighbors) {
            float d = vectorDistance(hnswNodes_[nodeIdx].vector,
                                     hnswNodes_[nIdx].vector);
            distPairs.push_back({d, nIdx});
        }
        std::sort(distPairs.begin(), distPairs.end());

        nodeNeighbors.clear();
        for (int i = 0; i < maxM; ++i) {
            nodeNeighbors.push_back(distPairs[i].second);
        }
    }
}

void EdgeAIEngine::hnswInsert(const std::string& id, const std::vector<float>& vec) {
    if (!isEnabled()) return;

    std::unique_lock<std::shared_mutex> lock(hnswMutex_);

    // 检查是否已存在
    if (hnswIdMap_.count(id)) {
        LOG_WARN << "[EdgeAI] HNSW: vector '" << id << "' already exists, skipping";
        return;
    }

    int nodeLevel = randomLevel();
    size_t nodeIdx = hnswNodes_.size();

    // 创建新节点
    HNSWNode node;
    node.id = id;
    node.vector = vec;
    node.maxLevel = nodeLevel;
    node.neighbors.resize(nodeLevel + 1);

    hnswNodes_.push_back(std::move(node));
    hnswIdMap_[id] = nodeIdx;

    // 第一个节点：直接设为入口点
    if (hnswNodes_.size() == 1) {
        hnswEntryPoint_ = 0;
        hnswMaxLevel_ = nodeLevel;
        return;
    }

    // 从最高层开始贪心搜索到 nodeLevel+1 层
    size_t currentEntry = hnswEntryPoint_;

    for (int lv = hnswMaxLevel_; lv > nodeLevel; --lv) {
        auto nearest = searchLayer(vec, currentEntry, 1, lv);
        if (!nearest.empty()) {
            currentEntry = nearest[0];
        }
    }

    // 从 min(nodeLevel, hnswMaxLevel_) 层到第0层，搜索并连接邻居
    for (int lv = std::min(nodeLevel, hnswMaxLevel_); lv >= 0; --lv) {
        int efConstruction = hnswEfConstruction_;
        auto neighbors = searchLayer(vec, currentEntry, efConstruction, lv);

        int maxM = (lv == 0) ? hnswMMax0_ : hnswM_;

        // 取最近的 maxM 个作为邻居
        if (static_cast<int>(neighbors.size()) > maxM) {
            neighbors.resize(maxM);
        }

        connectNeighbors(nodeIdx, neighbors, lv, maxM);

        if (!neighbors.empty()) {
            currentEntry = neighbors[0];
        }
    }

    // 更新入口点
    if (nodeLevel > hnswMaxLevel_) {
        hnswMaxLevel_ = nodeLevel;
        hnswEntryPoint_ = nodeIdx;
    }
}

std::vector<VectorSearchResult> EdgeAIEngine::hnswSearch(const std::vector<float>& query,
                                                          int k) {
    if (!isEnabled()) return {};

    totalHNSWSearches_.fetch_add(1);

    std::shared_lock<std::shared_mutex> lock(hnswMutex_);

    if (hnswNodes_.empty()) return {};

    // 从最高层贪心搜索到第1层
    size_t currentEntry = hnswEntryPoint_;
    for (int lv = hnswMaxLevel_; lv > 0; --lv) {
        auto nearest = searchLayer(query, currentEntry, 1, lv);
        if (!nearest.empty()) {
            currentEntry = nearest[0];
        }
    }

    // 在第0层执行精确搜索
    int ef = std::max(k, hnswEfSearch_);
    auto candidates = searchLayer(query, currentEntry, ef, 0);

    // 取 top-k
    if (static_cast<int>(candidates.size()) > k) {
        candidates.resize(k);
    }

    // 构建结果
    std::vector<VectorSearchResult> results;
    results.reserve(candidates.size());

    for (size_t idx : candidates) {
        float dist = vectorDistance(query, hnswNodes_[idx].vector);
        float sqrtDist = std::sqrt(dist);

        VectorSearchResult r;
        r.id = hnswNodes_[idx].id;
        r.distance = sqrtDist;
        // 相似度：使用高斯核映射 sim = exp(-dist / (2 * dim))
        float dim = static_cast<float>(query.size());
        r.similarity = std::exp(-dist / (2.0f * dim));
        results.push_back(std::move(r));
    }

    return results;
}

Json::Value EdgeAIEngine::getHNSWStats() const {
    std::shared_lock<std::shared_mutex> lock(hnswMutex_);

    Json::Value stats;
    stats["total_nodes"] = static_cast<Json::UInt64>(hnswNodes_.size());
    stats["max_level"] = hnswMaxLevel_;
    stats["m"] = hnswM_;
    stats["m_max0"] = hnswMMax0_;
    stats["ef_construction"] = hnswEfConstruction_;
    stats["ef_search"] = hnswEfSearch_;
    stats["total_searches"] = static_cast<Json::UInt64>(totalHNSWSearches_.load());

    // 统计各层节点数
    Json::Value levelCounts(Json::arrayValue);
    std::vector<int> counts(hnswMaxLevel_ + 1, 0);
    for (const auto& node : hnswNodes_) {
        for (int lv = 0; lv <= node.maxLevel; ++lv) {
            if (lv < static_cast<int>(counts.size())) {
                counts[lv]++;
            }
        }
    }
    for (int c : counts) {
        levelCounts.append(c);
    }
    stats["level_node_counts"] = levelCounts;

    // 平均邻居数（第0层）
    if (!hnswNodes_.empty()) {
        double totalNeighbors = 0;
        for (const auto& node : hnswNodes_) {
            if (!node.neighbors.empty()) {
                totalNeighbors += node.neighbors[0].size();
            }
        }
        stats["avg_neighbors_level0"] = totalNeighbors / hnswNodes_.size();
    }

    return stats;
}

size_t EdgeAIEngine::getHNSWVectorDimension() const {
    std::shared_lock<std::shared_mutex> lock(hnswMutex_);
    if (hnswNodes_.empty()) return 0;
    return hnswNodes_.front().vector.size();
}

// ============================================================================
// 子系统7: 模型量化推理
// ============================================================================

QuantizedTensor EdgeAIEngine::quantizeToInt8(const std::vector<float>& tensor,
                                              const std::vector<size_t>& shape) {
    totalQuantizedOps_.fetch_add(1);

    QuantizedTensor result;
    result.shape = shape;
    result.zeroPoint = 0;  // 对称量化，零点为0

    if (tensor.empty()) {
        result.scale = 1.0f;
        return result;
    }

    // 对称量化：scale = max(|x|) / 127
    float absMax = 0.0f;
    for (float v : tensor) {
        float av = std::abs(v);
        if (av > absMax) absMax = av;
    }

    // 防止除零
    result.scale = (absMax < 1e-8f) ? 1e-8f : (absMax / 127.0f);
    float invScale = 1.0f / result.scale;

    result.data.resize(tensor.size());
    for (size_t i = 0; i < tensor.size(); ++i) {
        // q = clamp(round(x / scale), -128, 127)
        float q = std::round(tensor[i] * invScale);
        q = std::clamp(q, -128.0f, 127.0f);
        result.data[i] = static_cast<int8_t>(q);
    }

    return result;
}

std::vector<float> EdgeAIEngine::quantizedMatMul(const QuantizedTensor& a,
                                                   const QuantizedTensor& b,
                                                   size_t M, size_t K, size_t N) {
    totalQuantizedOps_.fetch_add(1);

    // 验证维度
    if (a.data.size() != M * K || b.data.size() != K * N) {
        LOG_ERROR << "[EdgeAI] quantizedMatMul dimension mismatch: "
                  << "A(" << M << "x" << K << ") * B(" << K << "x" << N << ") "
                  << "but got data sizes " << a.data.size() << " and " << b.data.size();
        return std::vector<float>(M * N, 0.0f);
    }

    // 在INT32域执行累加（避免INT8溢出）
    // C_float[i][j] = scale_a * scale_b * Σ_k (a_int8[i][k] * b_int8[k][j])
    float combinedScale = a.scale * b.scale;
    std::vector<float> result(M * N, 0.0f);

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            int32_t acc = 0;
            const int8_t* aRow = a.data.data() + i * K;
            const int8_t* bCol = b.data.data() + j;  // 列步长为 N

            // 4路展开
            size_t k = 0;
            for (; k + 3 < K; k += 4) {
                acc += static_cast<int32_t>(aRow[k])     * static_cast<int32_t>(bCol[k * N]);
                acc += static_cast<int32_t>(aRow[k + 1]) * static_cast<int32_t>(bCol[(k + 1) * N]);
                acc += static_cast<int32_t>(aRow[k + 2]) * static_cast<int32_t>(bCol[(k + 2) * N]);
                acc += static_cast<int32_t>(aRow[k + 3]) * static_cast<int32_t>(bCol[(k + 3) * N]);
            }
            for (; k < K; ++k) {
                acc += static_cast<int32_t>(aRow[k]) * static_cast<int32_t>(bCol[k * N]);
            }

            // 反量化回 float32
            result[i * N + j] = combinedScale * static_cast<float>(acc);
        }
    }

    return result;
}

std::vector<float> EdgeAIEngine::quantizedForward(const std::vector<float>& input,
                                                    const QuantizedTensor& weights,
                                                    const std::vector<float>& biases) {
    totalQuantizedOps_.fetch_add(1);

    if (weights.shape.size() < 2) {
        LOG_ERROR << "[EdgeAI] quantizedForward: weights must be 2D (outDim x inDim)";
        return {};
    }

    size_t outDim = weights.shape[0];
    size_t inDim = weights.shape[1];

    if (input.size() != inDim) {
        LOG_ERROR << "[EdgeAI] quantizedForward: input size " << input.size()
                  << " != weight inDim " << inDim;
        return {};
    }

    if (!biases.empty() && biases.size() != outDim) {
        LOG_ERROR << "[EdgeAI] quantizedForward: bias size " << biases.size()
                  << " != outDim " << outDim;
        return {};
    }

    // 量化输入
    QuantizedTensor qInput = quantizeToInt8(input, {1, inDim});

    // 执行量化矩阵乘法: output = input(1 x inDim) * weights^T(inDim x outDim)
    // 但 weights 存储为 (outDim x inDim)，需要转置
    // 等价于：对每个输出维度，计算 input · weights[outIdx]
    float combinedScale = qInput.scale * weights.scale;
    std::vector<float> output(outDim, 0.0f);

    for (size_t o = 0; o < outDim; ++o) {
        int32_t acc = 0;
        const int8_t* wRow = weights.data.data() + o * inDim;

        size_t k = 0;
        for (; k + 3 < inDim; k += 4) {
            acc += static_cast<int32_t>(qInput.data[k])     * static_cast<int32_t>(wRow[k]);
            acc += static_cast<int32_t>(qInput.data[k + 1]) * static_cast<int32_t>(wRow[k + 1]);
            acc += static_cast<int32_t>(qInput.data[k + 2]) * static_cast<int32_t>(wRow[k + 2]);
            acc += static_cast<int32_t>(qInput.data[k + 3]) * static_cast<int32_t>(wRow[k + 3]);
        }
        for (; k < inDim; ++k) {
            acc += static_cast<int32_t>(qInput.data[k]) * static_cast<int32_t>(wRow[k]);
        }

        output[o] = combinedScale * static_cast<float>(acc);

        // 加偏置
        if (!biases.empty()) {
            output[o] += biases[o];
        }
    }

    // ReLU 激活
    for (size_t o = 0; o < outDim; ++o) {
        output[o] = std::max(0.0f, output[o]);
    }

    return output;
}

// ============================================================================
// 子系统8: 边缘节点健康监控
// ============================================================================

float EdgeAIEngine::computeHealthScore(const EdgeNodeStatus& status) const {
    // 综合评分公式：
    // score = w1*(1-cpu) + w2*(1-mem) + w3*(1-latency/maxLatency) + w4*(1-failRate)
    constexpr float w1 = 0.30f;  // CPU权重
    constexpr float w2 = 0.25f;  // 内存权重
    constexpr float w3 = 0.25f;  // 延迟权重
    constexpr float w4 = 0.20f;  // 失败率权重
    constexpr float maxLatencyMs = 1000.0f;  // 最大可接受延迟

    float cpuScore = 1.0f - std::clamp(status.cpuUsage, 0.0f, 1.0f);
    float memScore = 1.0f - std::clamp(status.memoryUsage, 0.0f, 1.0f);
    float latencyScore = 1.0f - std::clamp(status.latencyMs / maxLatencyMs, 0.0f, 1.0f);

    float failRate = 0.0f;
    if (status.totalRequests > 0) {
        failRate = static_cast<float>(status.failedRequests) /
                   static_cast<float>(status.totalRequests);
    }
    float failScore = 1.0f - std::clamp(failRate, 0.0f, 1.0f);

    float score = w1 * cpuScore + w2 * memScore + w3 * latencyScore + w4 * failScore;

    // 心跳超时惩罚：超过30秒未心跳，分数衰减
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - status.lastHeartbeat).count();
    if (elapsed > 30) {
        float decay = std::exp(-0.05f * static_cast<float>(elapsed - 30));
        score *= decay;
    }

    return std::clamp(score, 0.0f, 1.0f);
}

void EdgeAIEngine::registerNode(const std::string& nodeId) {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);

    if (nodeRegistry_.count(nodeId)) {
        LOG_WARN << "[EdgeAI] Node '" << nodeId << "' already registered";
        return;
    }

    EdgeNodeStatus status;
    status.nodeId = nodeId;
    status.cpuUsage = 0.0f;
    status.memoryUsage = 0.0f;
    status.latencyMs = 0.0f;
    status.activeConnections = 0;
    status.totalRequests = 0;
    status.failedRequests = 0;
    status.isHealthy = true;
    status.lastHeartbeat = std::chrono::steady_clock::now();
    status.healthScore = 1.0f;
    status.circuitState = CircuitState::CLOSED;
    status.circuitOpenedAt = std::chrono::steady_clock::time_point{};
    status.consecutiveFailures = 0;
    status.halfOpenSuccesses = 0;

    nodeRegistry_[nodeId] = std::move(status);
    LOG_INFO << "[EdgeAI] Node registered: " << nodeId;
}

void EdgeAIEngine::updateNodeStatus(const EdgeNodeStatus& status) {
    std::unique_lock<std::shared_mutex> lock(nodeMutex_);

    auto it = nodeRegistry_.find(status.nodeId);
    if (it == nodeRegistry_.end()) {
        LOG_WARN << "[EdgeAI] Unknown node: " << status.nodeId << ", auto-registering";
        nodeRegistry_[status.nodeId] = status;
        nodeRegistry_[status.nodeId].lastHeartbeat = std::chrono::steady_clock::now();
        nodeRegistry_[status.nodeId].healthScore = computeHealthScore(status);
        return;
    }

    // 更新状态
    it->second.cpuUsage = status.cpuUsage;
    it->second.memoryUsage = status.memoryUsage;
    it->second.latencyMs = status.latencyMs;
    it->second.activeConnections = status.activeConnections;
    it->second.totalRequests = status.totalRequests;
    it->second.failedRequests = status.failedRequests;
    it->second.lastHeartbeat = std::chrono::steady_clock::now();

    // 重新计算健康分
    it->second.healthScore = computeHealthScore(it->second);

    // 熔断器状态机转换
    auto& node = it->second;
    auto now = std::chrono::steady_clock::now();
    float failRate = node.totalRequests > 0
        ? static_cast<float>(node.failedRequests) / static_cast<float>(node.totalRequests)
        : 0.0f;

    switch (node.circuitState) {
        case CircuitState::CLOSED: {
            // CLOSED: 失败率超阈值且请求数足够 -> 转 OPEN
            if (node.totalRequests >= EdgeNodeStatus::MIN_REQUESTS_FOR_CIRCUIT &&
                failRate >= EdgeNodeStatus::FAILURE_RATE_THRESHOLD) {
                node.circuitState = CircuitState::OPEN;
                node.circuitOpenedAt = now;
                node.isHealthy = false;
                LOG_WARN << "[EdgeAI] Circuit OPEN for node '" << node.nodeId
                         << "', failRate=" << failRate;
            } else {
                // 正常健康判定
                node.isHealthy = (node.healthScore >= 0.3f);
            }
            break;
        }
        case CircuitState::OPEN: {
            // OPEN: 冷却时间到 -> 转 HALF_OPEN
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - node.circuitOpenedAt).count();
            if (elapsed >= EdgeNodeStatus::COOLDOWN_SECONDS) {
                node.circuitState = CircuitState::HALF_OPEN;
                node.halfOpenSuccesses = 0;
                node.consecutiveFailures = 0;
                LOG_INFO << "[EdgeAI] Circuit HALF_OPEN for node '" << node.nodeId
                         << "' after " << elapsed << "s cooldown";
            }
            // OPEN 状态下保持 isHealthy = false
            node.isHealthy = false;
            break;
        }
        case CircuitState::HALF_OPEN: {
            // HALF_OPEN: 根据最新失败率判断
            if (failRate < EdgeNodeStatus::FAILURE_RATE_THRESHOLD) {
                node.halfOpenSuccesses++;
                if (node.halfOpenSuccesses >= EdgeNodeStatus::HALF_OPEN_SUCCESS_THRESHOLD) {
                    // 探测成功足够次数 -> 转 CLOSED
                    node.circuitState = CircuitState::CLOSED;
                    node.consecutiveFailures = 0;
                    node.isHealthy = (node.healthScore >= 0.3f);
                    LOG_INFO << "[EdgeAI] Circuit CLOSED for node '" << node.nodeId
                             << "', recovered";
                } else {
                    node.isHealthy = true;  // 允许探测请求
                }
            } else {
                // 探测失败 -> 转回 OPEN
                node.circuitState = CircuitState::OPEN;
                node.circuitOpenedAt = now;
                node.halfOpenSuccesses = 0;
                node.isHealthy = false;
                LOG_WARN << "[EdgeAI] Circuit re-OPEN for node '" << node.nodeId
                         << "', probe failed, failRate=" << failRate;
            }
            break;
        }
    }

    if (!node.isHealthy) {
        LOG_WARN << "[EdgeAI] Node '" << status.nodeId
                 << "' marked unhealthy, score=" << node.healthScore
                 << ", circuit=" << (node.circuitState == CircuitState::CLOSED ? "CLOSED" :
                                     node.circuitState == CircuitState::OPEN   ? "OPEN" : "HALF_OPEN");
    }
}

std::optional<std::string> EdgeAIEngine::selectBestNode() {
    std::shared_lock<std::shared_mutex> lock(nodeMutex_);

    if (nodeRegistry_.empty()) return std::nullopt;

    std::string bestId;
    float bestScore = -1.0f;

    auto now = std::chrono::steady_clock::now();

    for (auto& [nodeId, status] : nodeRegistry_) {
        // 跳过 OPEN 熔断状态节点（完全隔离）
        if (status.circuitState == CircuitState::OPEN) continue;

        // 跳过不健康节点
        if (!status.isHealthy) continue;

        // 跳过心跳超时节点（超过60秒）
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - status.lastHeartbeat).count();
        if (elapsed > 60) {
            status.isHealthy = false;
            continue;
        }

        // HALF_OPEN 节点降低优先级（乘以0.5权重）
        float score = computeHealthScore(status);
        status.healthScore = score;
        if (status.circuitState == CircuitState::HALF_OPEN) {
            score *= 0.5f;
        }

        if (score > bestScore) {
            bestScore = score;
            bestId = nodeId;
        }
    }

    if (bestId.empty()) return std::nullopt;
    return bestId;
}

std::vector<EdgeNodeStatus> EdgeAIEngine::getAllNodeStatus() const {
    std::shared_lock<std::shared_mutex> lock(nodeMutex_);

    std::vector<EdgeNodeStatus> result;
    result.reserve(nodeRegistry_.size());
    for (const auto& [id, status] : nodeRegistry_) {
        result.push_back(status);
    }

    // 按健康分降序排列
    std::sort(result.begin(), result.end(),
              [](const EdgeNodeStatus& a, const EdgeNodeStatus& b) {
                  return a.healthScore > b.healthScore;
              });

    return result;
}

// ============================================================================
// 引擎综合统计
// ============================================================================

Json::Value EdgeAIEngine::getEngineStats() const {
    Json::Value stats;

    stats["enabled"] = enabled_;
    stats["initialized"] = initialized_;

    // 各子系统调用统计
    Json::Value calls;
    calls["sentiment_analysis"] = static_cast<Json::UInt64>(totalSentimentCalls_.load());
    calls["text_moderation"] = static_cast<Json::UInt64>(totalModerationCalls_.load());
    calls["hnsw_searches"] = static_cast<Json::UInt64>(totalHNSWSearches_.load());
    calls["quantized_ops"] = static_cast<Json::UInt64>(totalQuantizedOps_.load());
    stats["call_counts"] = calls;

    // HNSW索引状态
    {
        std::shared_lock<std::shared_mutex> lock(hnswMutex_);
        stats["hnsw_node_count"] = static_cast<Json::UInt64>(hnswNodes_.size());
        stats["hnsw_max_level"] = hnswMaxLevel_;
    }

    // 情绪脉搏状态
    {
        std::shared_lock<std::shared_mutex> lock(pulseMutex_);
        stats["emotion_window_size"] = static_cast<Json::UInt64>(emotionWindow_.size());
        stats["pulse_history_size"] = static_cast<Json::UInt64>(pulseHistory_.size());
    }

    // 联邦学习状态
    {
        std::lock_guard<std::mutex> lock(federatedMutex_);
        stats["federated_round"] = federatedRound_;
        stats["pending_local_models"] = static_cast<Json::UInt64>(localModels_.size());
    }

    // 差分隐私状态
    stats["dp_remaining_budget"] = getRemainingPrivacyBudget();
    stats["dp_consumed_epsilon"] = consumedEpsilon_.load();

    // 节点监控状态
    {
        std::shared_lock<std::shared_mutex> lock(nodeMutex_);
        stats["registered_nodes"] = static_cast<Json::UInt64>(nodeRegistry_.size());
        int healthyCount = 0;
        for (const auto& [id, status] : nodeRegistry_) {
            if (status.isHealthy) ++healthyCount;
        }
        stats["healthy_nodes"] = healthyCount;
    }

    return stats;
}

} // namespace ai
} // namespace heartlake
