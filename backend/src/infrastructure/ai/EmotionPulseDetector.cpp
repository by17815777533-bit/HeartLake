/**
 * 实时情绪脉搏检测子系统实现
 *
 * 从 EdgeAIEngine 提取的独立模块。
 *
 */

#include "infrastructure/ai/EmotionPulseDetector.h"
#include <mutex>

namespace heartlake {
namespace ai {

// ============================================================================
// 配置
// ============================================================================

void EmotionPulseDetector::configure(int windowSeconds, int maxHistory) {
    std::unique_lock<std::shared_mutex> lock(pulseMutex_);
    pulseWindowSeconds_ = windowSeconds;
    maxPulseHistory_ = maxHistory;
}

// ============================================================================
// 清空
// ============================================================================

void EmotionPulseDetector::clear() {
    std::unique_lock<std::shared_mutex> lock(pulseMutex_);
    emotionWindow_.clear();
    pulseHistory_.clear();
}

// ============================================================================
// 辅助函数
// ============================================================================

float EmotionPulseDetector::medianValue(std::vector<float> values) {
    if (values.empty()) {
        return 0.0f;
    }
    const size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid), values.end());
    if (values.size() % 2 == 1) {
        return values[mid];
    }
    const float upper = values[mid];
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid - 1), values.end());
    const float lower = values[mid - 1];
    return (lower + upper) * 0.5f;
}

// ============================================================================
// 窗口修剪
// ============================================================================

void EmotionPulseDetector::pruneEmotionWindow() {
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

// ============================================================================
// 脉搏计算
// ============================================================================

EmotionPulse EmotionPulseDetector::computePulseFromWindow() const {
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

    const auto now = std::chrono::steady_clock::now();
    const float tauSec = std::max(45.0f, static_cast<float>(pulseWindowSeconds_) * 0.45f);

    // 计算时间衰减 + 置信度加权平均分（越新的样本、越高置信度样本权重越高）
    float weightedSum = 0.0f;
    float weightTotal = 0.0f;
    std::unordered_map<std::string, int> moodCounts;
    std::unordered_map<std::string, float> moodWeights;
    for (const auto& s : emotionWindow_) {
        const float ageSec = std::max(
            0.0f,
            static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s.timestamp).count()) / 1000.0f);
        const float confidenceWeight = std::clamp(s.weight, 0.1f, 1.0f);
        const float weight = std::exp(-ageSec / tauSec) * confidenceWeight;
        weightedSum += s.score * weight;
        weightTotal += weight;
        moodCounts[s.mood]++;
        moodWeights[s.mood] += weight;
    }
    pulse.avgScore = weightTotal > 0.0f ? (weightedSum / weightTotal) : 0.0f;
    pulse.moodDistribution = moodCounts;

    // 计算加权标准差
    float varianceSum = 0.0f;
    for (const auto& s : emotionWindow_) {
        const float ageSec = std::max(
            0.0f,
            static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s.timestamp).count()) / 1000.0f);
        const float confidenceWeight = std::clamp(s.weight, 0.1f, 1.0f);
        const float weight = std::exp(-ageSec / tauSec) * confidenceWeight;
        float diff = s.score - pulse.avgScore;
        varianceSum += weight * diff * diff;
    }
    pulse.stddev = std::sqrt(varianceSum / std::max(weightTotal, 1e-6f));

    // 计算趋势斜率（基于真实时间戳 + EWMA平滑 + 时间衰减加权回归，单位: 每分钟）
    if (emotionWindow_.size() >= 4) {
        float sumW = 0.0f, sumWX = 0.0f, sumWY = 0.0f, sumWXY = 0.0f, sumWXX = 0.0f;
        const auto t0 = emotionWindow_.front().timestamp;
        float ewma = emotionWindow_.front().score;
        constexpr float ewmaAlpha = 0.35f;
        for (const auto& sample : emotionWindow_) {
            const float x = static_cast<float>(std::chrono::duration_cast<std::chrono::seconds>(
                sample.timestamp - t0).count()) / 60.0f;
            ewma = ewmaAlpha * sample.score + (1.0f - ewmaAlpha) * ewma;
            const float ageSec = std::max(
                0.0f,
                static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - sample.timestamp).count()) / 1000.0f);
            const float confidenceWeight = std::clamp(sample.weight, 0.1f, 1.0f);
            const float w = std::exp(-ageSec / tauSec) * confidenceWeight;

            sumW += w;
            sumWX += w * x;
            sumWY += w * ewma;
            sumWXY += w * x * ewma;
            sumWXX += w * x * x;
        }
        const float denominator = sumW * sumWXX - sumWX * sumWX;
        if (std::abs(denominator) > 1e-6f) {
            pulse.trendSlope = (sumW * sumWXY - sumWX * sumWY) / denominator;
        } else {
            pulse.trendSlope = 0.0f;
        }
    } else {
        pulse.trendSlope = 0.0f;
    }

    // 找主导情绪
    std::string dominant = "neutral";
    float maxWeight = -1.0f;
    for (const auto& [mood, weight] : moodWeights) {
        if (weight > maxWeight) {
            maxWeight = weight;
            dominant = mood;
        }
    }
    pulse.dominantMood = dominant;

    return pulse;
}

// ============================================================================
// 提交样本
// ============================================================================

void EmotionPulseDetector::submitEmotionSample(float score, const std::string& mood, float confidence) {
    std::unique_lock<std::shared_mutex> lock(pulseMutex_);

    float adjustedScore = std::clamp(score, -1.0f, 1.0f);
    // 低置信度样本保留但降权，避免误判显著拉偏脉搏。
    float normalizedConfidence = std::clamp(confidence, 0.0f, 1.0f);
    float sampleWeight = 0.25f + 0.75f * normalizedConfidence;
    // 鲁棒抗噪：使用最近窗口中位数 + MAD 限制离群点，避免脉搏被极端噪声污染。
    if (emotionWindow_.size() >= 6) {
        const size_t recentCount = std::min<size_t>(emotionWindow_.size(), 15);
        std::vector<float> recentScores;
        recentScores.reserve(recentCount);
        const size_t beginIdx = emotionWindow_.size() - recentCount;
        for (size_t i = beginIdx; i < emotionWindow_.size(); ++i) {
            recentScores.push_back(emotionWindow_[i].score);
        }
        const float median = medianValue(recentScores);
        std::vector<float> absDeviation;
        absDeviation.reserve(recentScores.size());
        for (float s : recentScores) {
            absDeviation.push_back(std::abs(s - median));
        }
        const float mad = std::max(0.05f, medianValue(absDeviation));
        // 低置信度样本采用更严格边界。
        const float confidenceTightening = 1.0f - 0.35f * (1.0f - normalizedConfidence);
        const float lower = median - 3.0f * mad * confidenceTightening;
        const float upper = median + 3.0f * mad * confidenceTightening;
        if (adjustedScore < lower || adjustedScore > upper) {
            const float bounded = std::clamp(adjustedScore, lower, upper);
            adjustedScore = 0.75f * bounded + 0.25f * median;
        }
    }

    EmotionSample sample;
    sample.score = adjustedScore;
    sample.mood = mood;
    sample.weight = sampleWeight;
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

// ============================================================================
// 查询接口
// ============================================================================

EmotionPulse EmotionPulseDetector::getCurrentPulse() {
    std::unique_lock<std::shared_mutex> lock(pulseMutex_);
    pruneEmotionWindow();
    return computePulseFromWindow();
}

std::vector<EmotionPulse> EmotionPulseDetector::getPulseHistory(int count) {
    std::shared_lock<std::shared_mutex> lock(pulseMutex_);
    int n = std::min(count, static_cast<int>(pulseHistory_.size()));
    if (n <= 0) return {};
    return std::vector<EmotionPulse>(pulseHistory_.end() - n, pulseHistory_.end());
}

} // namespace ai
} // namespace heartlake
