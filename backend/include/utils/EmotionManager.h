/**
 * @brief 统一情绪管理器 — 前后端共享的情绪模型定义与分析
 *
 * 维护 8 种情绪类型的完整配置（分数区间、关键词、颜色），
 * 消除前端 EmotionType/MoodType 与后端之间的重复定义。
 * 支持基于分数阈值和文本关键词两种情绪判定方式。
 */

#pragma once
#include <string>
#include <map>
#include <vector>

namespace heartlake {
namespace emotion {

/**
 * @brief 情绪类型枚举，与前端 EmotionType 和 MoodType 一一对应
 *
 * 前 5 种有明确的分数区间映射，后 3 种主要通过关键词匹配识别
 */
enum class EmotionType {
    HAPPY,      ///< 开心 (0.6 ~ 1.0)
    CALM,       ///< 平静 (0.3 ~ 0.6)
    NEUTRAL,    ///< 中性 (-0.3 ~ 0.3)
    ANXIOUS,    ///< 焦虑 (-0.6 ~ -0.3)
    SAD,        ///< 悲伤 (-1.0 ~ -0.6)
    ANGRY,      ///< 愤怒
    SURPRISED,  ///< 惊喜
    CONFUSED    ///< 迷茫
};

/**
 * @brief 单个情绪类型的完整配置
 */
struct EmotionConfig {
    std::string name;         ///< 英文标识（如 "happy"）
    std::string label;        ///< 中文显示名（如 "开心"）
    std::string color;        ///< 主色调 Hex 值（如 "#FFD700"）
    float scoreMin;           ///< 分数区间下限（含）
    float scoreMax;           ///< 分数区间上限（含）
    std::vector<std::string> keywords; ///< 文本关键词列表，用于关键词匹配分析
};

/**
 * @brief 情绪管理器（单例）
 *
 * 初始化时加载全部 8 种情绪的配置数据，后续提供分数映射、
 * 文本分析、颜色查询等只读操作，线程安全。
 */
class EmotionManager {
public:
    static EmotionManager& getInstance();

    /**
     * @brief 根据情绪分数映射到情绪类型
     * @param score 情绪分数，范围 [-1.0, 1.0]
     * @return 情绪类型英文标识（如 "happy"、"sad"）
     */
    std::string getEmotionFromScore(float score) const;

    /**
     * @brief 基于关键词匹配分析文本情绪
     * @param text 待分析的文本内容
     * @return {情绪分数, 情绪类型标识}
     */
    std::pair<float, std::string> analyzeEmotionFromText(const std::string& text) const;

    /**
     * @brief 获取情绪对应的主题色
     * @param emotion 情绪类型英文标识
     * @return Hex 颜色代码（如 "#FFD700"），未知类型返回默认灰色
     */
    std::string getEmotionColor(const std::string& emotion) const;

    /**
     * @brief 获取指定情绪的完整配置
     * @param emotion 情绪类型英文标识
     * @return 配置结构体；未知类型返回默认 neutral 配置
     */
    EmotionConfig getEmotionConfig(const std::string& emotion) const;

    /// 判断情绪标识是否在已注册的类型中
    bool isValidEmotion(const std::string& emotion) const;

    /// 返回全部已注册的情绪类型标识列表
    std::vector<std::string> getAllEmotions() const;

private:
    EmotionManager();
    ~EmotionManager() = default;

    EmotionManager(const EmotionManager&) = delete;
    EmotionManager& operator=(const EmotionManager&) = delete;

    std::map<std::string, EmotionConfig> emotionConfigs_;

    /// 构造时调用，填充 8 种情绪的默认配置
    void initializeConfigs();
};

} // namespace emotion
} // namespace heartlake
