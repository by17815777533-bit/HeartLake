/**
 * @file EmotionResonanceEngine.h
 * @brief 情绪感知时序共鸣引擎
 *
 * 创新点：融合语义相似度、情绪轨迹匹配、时间衰减和多样性促进的
 * 多维度共鸣推荐算法。不同于传统的内容推荐，本算法关注用户的
 * 情绪变化轨迹，将处于相似情绪旅程中的用户连接起来。
 *
 * 算法公式：
 * ResonanceScore = α·SemanticSim + β·EmotionTrajectorySim + γ·TemporalDecay + δ·DiversityBonus
 * 其中 α+β+γ+δ = 1, 默认 α=0.3, β=0.35, γ=0.2, δ=0.15
 */
#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <json/json.h>
#include <drogon/drogon.h>

namespace heartlake::ai {

struct EmotionTrajectory {
    std::string userId;
    std::vector<float> scores;      // 时序情绪分数
    std::vector<std::string> moods; // 时序情绪类型
    float currentScore;
    std::string currentMood;
};

struct ResonanceResult {
    std::string stoneId;
    std::string userId;
    float totalScore;
    float semanticScore;
    float trajectoryScore;
    float temporalScore;
    float diversityScore;
    std::string resonanceReason; // 人类可读的共鸣原因
};

class EmotionResonanceEngine {
public:
    static EmotionResonanceEngine& getInstance();

    /**
     * @brief 计算多维度共鸣推荐
     */
    std::vector<ResonanceResult> findResonance(
        const std::string& userId,
        const std::string& stoneId,
        int limit = 10
    );

    /**
     * @brief LB_Keogh 下界：DTW 距离的快速下界估计，O(n) 时间
     * 用于剪枝，避免不必要的完整 DTW 计算
     */
    float lbKeogh(
        const std::vector<float>& query,
        const std::vector<float>& candidate,
        int bandWidth = 10
    );

    /**
     * @brief 计算两个情绪轨迹的相似度
     * 使用DTW (Dynamic Time Warping) + Sakoe-Chiba band 约束
     */
    float trajectorySimDTW(
        const std::vector<float>& traj1,
        const std::vector<float>& traj2
    );

    /**
     * @brief 计算时间衰减因子
     * 使用指数衰减：decay = exp(-λ * Δt)
     */
    float temporalDecay(const std::string& timestamp, float lambda = 0.1f);

    /**
     * @brief 计算多样性奖励
     * 避免推荐相同情绪类型的内容，促进情绪多样性
     */
    float diversityBonus(
        const std::string& currentMood,
        const std::string& candidateMood,
        const std::vector<std::string>& alreadyRecommended
    );

    /**
     * @brief 生成人类可读的共鸣原因
     */
    std::string generateResonanceReason(
        const ResonanceResult& result,
        const std::string& currentMood,
        const std::string& candidateMood
    );

    // 权重参数
    float alpha = 0.30f;  // 语义相似度权重
    float beta  = 0.35f;  // 情绪轨迹权重
    float gamma = 0.20f;  // 时间衰减权重
    float delta = 0.15f;  // 多样性权重

private:
    EmotionResonanceEngine() = default;
    ~EmotionResonanceEngine() = default;
    EmotionResonanceEngine(const EmotionResonanceEngine&) = delete;
    EmotionResonanceEngine& operator=(const EmotionResonanceEngine&) = delete;

    EmotionTrajectory loadTrajectory(const std::string& userId, int days = 7);
};

} // namespace heartlake::ai
