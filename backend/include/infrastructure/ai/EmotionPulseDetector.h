/**
 * @file EmotionPulseDetector.h
 * @brief 实时情绪脉搏检测子系统
 *
 * 从 EdgeAIEngine 提取的独立模块。基于滑动窗口统计，
 * 支持时间衰减加权、EWMA趋势分析、鲁棒抗噪（中位数+MAD离群点检测）。
 *
 * Created by 王璐瑶
 */

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <json/json.h>

namespace heartlake {
namespace ai {

// ============================================================================
// 数据结构
// ============================================================================

/**
 * @brief 情绪脉搏快照
 */
struct EmotionPulse {
    std::chrono::steady_clock::time_point timestamp;
    float avgScore;                                    ///< 窗口内加权平均情感分数
    float stddev;                                      ///< 情感波动标准差
    std::unordered_map<std::string, int> moodDistribution; ///< 情绪类型分布
    int sampleCount;                                   ///< 样本数量
    float trendSlope;                                  ///< 情绪趋势斜率（正=好转，负=恶化）
    std::string dominantMood;                          ///< 主导情绪

    Json::Value toJson() const {
        Json::Value j;
        j["avg_score"] = avgScore;
        j["stddev"] = stddev;
        j["sample_count"] = sampleCount;
        j["trend_slope"] = trendSlope;
        j["dominant_mood"] = dominantMood;
        Json::Value dist;
        for (const auto& [mood, count] : moodDistribution) {
            dist[mood] = count;
        }
        j["mood_distribution"] = dist;
        return j;
    }
};

// ============================================================================
// EmotionPulseDetector
// ============================================================================

/**
 * @brief 实时情绪脉搏检测器
 *
 * 线程安全。通过滑动窗口收集情绪样本，计算加权统计指标，
 * 每积累一定样本数自动生成脉搏快照。
 *
 * 特性：
 * - 时间衰减加权（指数衰减，越新样本权重越高）
 * - 置信度加权（低置信度样本降权）
 * - EWMA平滑趋势斜率
 * - 鲁棒抗噪（中位数 + MAD 离群点检测）
 */
class EmotionPulseDetector {
public:
    /**
     * @brief 配置检测器参数
     * @param windowSeconds 滑动窗口大小（秒）
     * @param maxHistory 最大脉搏历史数
     */
    void configure(int windowSeconds, int maxHistory);

    /**
     * @brief 提交情绪样本
     * @param score 情感分数 [-1.0, 1.0]
     * @param mood 情绪类型
     * @param confidence 置信度 [0.0, 1.0]
     */
    void submitEmotionSample(float score, const std::string& mood, float confidence = 1.0f);

    /**
     * @brief 获取当前情绪脉搏快照
     * @return 当前窗口的情绪统计
     */
    EmotionPulse getCurrentPulse();

    /**
     * @brief 获取情绪脉搏历史（最近N个快照）
     * @param count 快照数量
     * @return 历史脉搏列表
     */
    std::vector<EmotionPulse> getPulseHistory(int count = 10);

    /**
     * @brief 清空所有数据
     */
    void clear();

private:
    struct EmotionSample {
        float score;
        std::string mood;
        float weight;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::deque<EmotionSample> emotionWindow_;
    std::vector<EmotionPulse> pulseHistory_;
    mutable std::shared_mutex pulseMutex_;
    int pulseWindowSeconds_ = 300;
    int maxPulseHistory_ = 100;

    void pruneEmotionWindow();
    EmotionPulse computePulseFromWindow() const;
    static float medianValue(std::vector<float> values);
};

} // namespace ai
} // namespace heartlake
