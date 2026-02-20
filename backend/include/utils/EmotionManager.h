/**
 * @file EmotionManager.h
 * @brief EmotionManager 模块接口定义
 * Created by 王璐瑶
 */

#pragma once
#include <string>
#include <map>
#include <vector>

namespace heartlake {
namespace emotion {

/**
 * 情绪类型枚举
 * 与前端EmotionType和MoodType完全对应
 */
enum class EmotionType {
    HAPPY,      // 开心 (0.6 ~ 1.0)
    CALM,       // 平静 (0.3 ~ 0.6)
    NEUTRAL,    // 中性 (-0.3 ~ 0.3)
    ANXIOUS,    // 焦虑 (-0.6 ~ -0.3)
    SAD,        // 悲伤 (-1.0 ~ -0.6)
    ANGRY,      // 愤怒
    SURPRISED,  // 惊喜
    CONFUSED    // 迷茫
};

/**
 * 情绪配置
 */
struct EmotionConfig {
    std::string name;         // 英文名
    std::string label;        // 中文名
    std::string color;        // 主色调
    float scoreMin;           // 分数下限
    float scoreMax;           // 分数上限
    std::vector<std::string> keywords; // 关键词列表
};

/**
 * 统一情绪管理器
 * 消除前后端重复逻辑
 */
/**
 * @brief 情绪管理器
 *
 * 详细说明
 *
 * @note 注意事项
 */
class EmotionManager {
public:
    /**
     * 获取单例
     */
    static EmotionManager& getInstance();

    /**
     * 从分数获取情绪类型
     * @param score 情绪分数 (-1.0 ~ 1.0)
     * @return 情绪类型字符串
     */
    std::string getEmotionFromScore(float score) const;

    /**
     * 从文本关键词分析情绪
     * @param text 文本内容
     * @return pair<score, emotion>
     */
    std::pair<float, std::string> analyzeEmotionFromText(const std::string& text) const;

    /**
     * 获取情绪对应的颜色
     * @param emotion 情绪类型
     * @return 颜色代码（Hex）
     */
    std::string getEmotionColor(const std::string& emotion) const;

    /**
     * 获取情绪配置
     * @param emotion 情绪类型
     * @return 情绪配置信息
     */
    EmotionConfig getEmotionConfig(const std::string& emotion) const;

    /**
     * 验证情绪类型是否有效
     */
    bool isValidEmotion(const std::string& emotion) const;

    /**
     * 获取所有支持的情绪类型
     */
    std::vector<std::string> getAllEmotions() const;

private:
    EmotionManager();
    ~EmotionManager() = default;

    EmotionManager(const EmotionManager&) = delete;
    EmotionManager& operator=(const EmotionManager&) = delete;

    std::map<std::string, EmotionConfig> emotionConfigs_;

    /**
     * @brief initializeConfigs方法
     */
    void initializeConfigs();
};

} // namespace emotion
} // namespace heartlake
