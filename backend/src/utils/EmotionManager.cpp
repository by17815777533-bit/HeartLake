/**
 * @file EmotionManager.cpp
 * @brief EmotionManager 模块实现
 * Created by 王璐瑶
 */
#include "utils/EmotionManager.h"
#include <algorithm>
#include <cctype>
#include <drogon/drogon.h>

namespace heartlake {
namespace emotion {

EmotionManager& EmotionManager::getInstance() {
    static EmotionManager instance;
    return instance;
}

EmotionManager::EmotionManager() {
    initializeConfigs();
}

void EmotionManager::initializeConfigs() {
    // 开心 - 暖橙色系
    emotionConfigs_["happy"] = {
        "happy",
        "开心",
        "#FF8A65",
        0.6f,
        1.0f,
        {"开心", "快乐", "高兴", "不错", "愉快", "舒服", "满意", "幸福", "喜悦",
         "欢乐", "兴奋", "激动", "美好", "棒", "赞", "好", "爱", "喜欢", "享受",
         "满足", "充实", "温暖", "感恩", "幸运", "开怀", "欣喜", "愉悦"}
    };

    // 平静 - 淡蓝绿色系
    emotionConfigs_["calm"] = {
        "calm",
        "平静",
        "#26A69A",
        0.3f,
        0.6f,
        {"平静", "放松", "安心", "宁静", "舒适", "安宁", "淡定", "从容", "悠闲",
         "自在", "轻松", "惬意", "安详", "祥和", "舒畅", "舒心", "安逸"}
    };

    // 中性 - 灰色系
    emotionConfigs_["neutral"] = {
        "neutral",
        "中性",
        "#9E9E9E",
        -0.3f,
        0.3f,
        {"还好", "一般", "平常", "正常", "普通", "尚可", "凑合", "马马虎虎", "还行"}
    };

    // 焦虑 - 紫色系
    emotionConfigs_["anxious"] = {
        "anxious",
        "焦虑",
        "#7E57C2",
        -0.6f,
        -0.3f,
        {"焦虑", "担心", "不安", "紧张", "忧虑", "害怕", "恐惧", "慌", "慌张",
         "忐忑", "惶恐", "惊慌", "不安", "烦躁", "烦恼", "忧心", "担忧", "顾虑"}
    };

    // 悲伤 - 深蓝色系
    emotionConfigs_["sad"] = {
        "sad",
        "悲伤",
        "#42A5F5",
        -1.0f,
        -0.6f,
        {"难过", "伤心", "悲伤", "失落", "沮丧", "痛苦", "难受", "心痛", "心碎",
         "绝望", "无助", "孤独", "寂寞", "空虚", "失望", "悲痛", "哀伤", "忧伤",
         "凄凉", "悲凉", "心酸", "委屈", "难熬", "煎熬", "折磨"}
    };

    // 愤怒 - 红色系
    emotionConfigs_["angry"] = {
        "angry",
        "愤怒",
        "#EF5350",
        -0.8f,
        -0.5f,
        {"生气", "愤怒", "恼火", "气愤", "不爽", "火大", "暴怒", "恼怒", "愤恨",
         "气死", "讨厌", "厌恶", "恨", "烦", "烦死", "受不了", "忍无可忍"}
    };

    // 惊喜 - 粉色系
    emotionConfigs_["surprised"] = {
        "surprised",
        "惊喜",
        "#EC407A",
        0.5f,
        0.9f,
        {"惊喜", "意外", "震惊", "吃惊", "惊讶", "惊奇", "诧异", "惊叹", "不可思议",
         "难以置信", "出乎意料", "想不到", "没想到"}
    };

    // 迷茫 - 靛蓝色系
    emotionConfigs_["confused"] = {
        "confused",
        "迷茫",
        "#5C6BC0",
        -0.2f,
        0.2f,
        {"迷茫", "困惑", "不知道", "纠结", "茫然", "迷失", "不确定", "犹豫",
         "彷徨", "无所适从", "不知所措", "摸不着头脑", "糊涂", "混乱"}
    };
}

std::string EmotionManager::getEmotionFromScore(float score) const {
    // 按分数范围映射情绪（与前端EmotionType.fromScore完全对应）
    if (score > 0.6f) return "happy";
    if (score > 0.3f) return "calm";
    if (score > -0.3f) return "neutral";
    if (score > -0.6f) return "anxious";
    return "sad";
}

std::pair<float, std::string> EmotionManager::analyzeEmotionFromText(const std::string& text) const {
    // 安全的小写转换：只转换ASCII字母，保留中文UTF-8字节不变
    std::string lowerText;
    lowerText.reserve(text.size());
    for (unsigned char c : text) {
        if (c >= 'A' && c <= 'Z') {
            lowerText += static_cast<char>(c + 32);
        } else {
            lowerText += static_cast<char>(c);
        }
    }

    // 关键词匹配计分（使用加权计数）
    std::unordered_map<std::string, float> emotionScores;
    emotionScores["happy"] = 0.0f;
    emotionScores["sad"] = 0.0f;
    emotionScores["anxious"] = 0.0f;
    emotionScores["angry"] = 0.0f;
    emotionScores["calm"] = 0.0f;
    emotionScores["surprised"] = 0.0f;
    emotionScores["confused"] = 0.0f;

    // 否定词列表
    static const std::vector<std::string> negators = {
        "不", "没", "没有", "别", "未", "无", "非", "莫", "勿", "毫不"
    };

    // 统计每种情绪的关键词出现次数
    for (const auto& [emotion, config] : emotionConfigs_) {
        for (const auto& keyword : config.keywords) {
            size_t pos = 0;
            while ((pos = lowerText.find(keyword, pos)) != std::string::npos) {
                // 检查关键词前是否有否定词（窗口：前9字节，约3个中文字符）
                bool negated = false;
                if (pos > 0) {
                    size_t windowStart = pos > 9 ? pos - 9 : 0;
                    std::string prefix = lowerText.substr(windowStart, pos - windowStart);
                    for (const auto& neg : negators) {
                        if (prefix.rfind(neg) != std::string::npos) {
                            negated = true;
                            break;
                        }
                    }
                }

                // 用UTF-8字符数计算权重，而非字节数
                size_t charCount = 0;
                for (size_t ki = 0; ki < keyword.size(); ) {
                    unsigned char c = static_cast<unsigned char>(keyword[ki]);
                    if (c < 0x80) ki += 1;
                    else if ((c & 0xE0) == 0xC0) ki += 2;
                    else if ((c & 0xF0) == 0xE0) ki += 3;
                    else ki += 4;
                    ++charCount;
                }
                float weight = 1.0f + (charCount / 5.0f);

                if (negated) {
                    // 否定词翻转：给对立情绪加分而非当前情绪
                    // 简单处理：不给当前情绪加分
                } else {
                    emotionScores[emotion] += weight;
                }
                pos += keyword.length();
            }
        }
    }

    // 找出得分最高的情绪
    std::string dominantEmotion = "neutral";
    float maxScore = 0.0f;

    for (const auto& [emotion, score] : emotionScores) {
        if (score > maxScore) {
            maxScore = score;
            dominantEmotion = emotion;
        }
    }

    // 如果没有匹配到任何情绪词，返回中性
    if (maxScore == 0.0f) {
        return {0.0f, "neutral"};
    }

    // 根据情绪类型计算情感分数
    float sentimentScore = 0.0f;
    if (dominantEmotion == "happy") {
        sentimentScore = 0.7f + std::min(0.3f, maxScore * 0.1f);
    } else if (dominantEmotion == "sad") {
        sentimentScore = -0.7f - std::min(0.3f, maxScore * 0.1f);
    } else if (dominantEmotion == "anxious") {
        sentimentScore = -0.4f - std::min(0.2f, maxScore * 0.1f);
    } else if (dominantEmotion == "angry") {
        sentimentScore = -0.6f - std::min(0.2f, maxScore * 0.1f);
    } else if (dominantEmotion == "calm") {
        sentimentScore = 0.4f + std::min(0.2f, maxScore * 0.1f);
    } else if (dominantEmotion == "surprised") {
        sentimentScore = 0.6f + std::min(0.2f, maxScore * 0.1f);
    } else if (dominantEmotion == "confused") {
        sentimentScore = 0.0f;
    }

    // 限制分数范围在 [-1.0, 1.0]
    sentimentScore = std::max(-1.0f, std::min(1.0f, sentimentScore));

    return {sentimentScore, dominantEmotion};
}

std::string EmotionManager::getEmotionColor(const std::string& emotion) const {
    auto it = emotionConfigs_.find(emotion);
    if (it != emotionConfigs_.end()) {
        return it->second.color;
    }
    return "#9E9E9E";  // 默认灰色
}

EmotionConfig EmotionManager::getEmotionConfig(const std::string& emotion) const {
    auto it = emotionConfigs_.find(emotion);
    if (it != emotionConfigs_.end()) {
        return it->second;
    }
    // 返回默认neutral配置
    return emotionConfigs_.at("neutral");
}

bool EmotionManager::isValidEmotion(const std::string& emotion) const {
    return emotionConfigs_.find(emotion) != emotionConfigs_.end();
}

std::vector<std::string> EmotionManager::getAllEmotions() const {
    std::vector<std::string> emotions;
    for (const auto& [emotion, config] : emotionConfigs_) {
        emotions.push_back(emotion);
    }
    return emotions;
}

} // namespace emotion
} // namespace heartlake
