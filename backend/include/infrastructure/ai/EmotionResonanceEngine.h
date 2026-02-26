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
#include <atomic>
#include <memory>
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

/// 共鸣评分四维权重，打包为不可变值对象，通过原子指针交换避免 torn-read
struct ResonanceWeights {
    float a;  // 语义相似度权重
    float b;  // 情绪轨迹权重
    float g;  // 时间衰减权重
    float d;  // 多样性权重
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

    // 权重参数 — 原子交换 shared_ptr，读取时拿到一致的快照，无 torn-read 风险
    ResonanceWeights getWeights() const {
        return *std::atomic_load_explicit(&weights_, std::memory_order_acquire);
    }
    float getAlpha() const { return getWeights().a; }
    float getBeta()  const { return getWeights().b; }
    float getGamma() const { return getWeights().g; }
    float getDelta() const { return getWeights().d; }
    void setWeights(float a, float b, float g, float d) {
        auto newW = std::make_shared<const ResonanceWeights>(ResonanceWeights{a, b, g, d});
        std::atomic_store_explicit(&weights_, newW, std::memory_order_release);
    }

private:
    EmotionResonanceEngine() = default;
    ~EmotionResonanceEngine() = default;
    EmotionResonanceEngine(const EmotionResonanceEngine&) = delete;
    EmotionResonanceEngine& operator=(const EmotionResonanceEngine&) = delete;

    EmotionTrajectory loadTrajectory(const std::string& userId, int days = 7);

    // 共鸣评分四维权重，满足 α+β+γ+δ = 1.0
    // 调参依据（基于 A/B 测试 + 用户留存数据）：
    //   β 最高(0.35): 情绪轨迹匹配是核心差异化特性，相似情绪旅程的用户互动率最高
    //   α 次之(0.30): 语义相似度保证内容相关性，是推荐质量的基础保障
    //   γ 适中(0.20): 时间衰减避免推荐过旧内容，但不宜过强以免冷启动困难
    //   δ 最低(0.15): 多样性奖励防止回音室效应，权重过高会牺牲相关性
    // 通过 atomic shared_ptr 交换实现无锁一致读取，杜绝 torn-read
    std::shared_ptr<const ResonanceWeights> weights_{
        std::make_shared<const ResonanceWeights>(ResonanceWeights{0.30f, 0.35f, 0.20f, 0.15f})
    };
};

} // namespace heartlake::ai
