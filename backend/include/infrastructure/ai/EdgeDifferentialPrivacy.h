#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <random>

namespace heartlake {
namespace ai {

/**
 * @brief 差分隐私配置
 */
struct DPConfig {
    float epsilon;              ///< 隐私预算 ε（越小越隐私）
    float delta;                ///< 松弛参数 δ（Gaussian机制使用，默认1e-5）
    float sensitivity;          ///< 查询敏感度 Δf
    float maxEpsilonBudget;     ///< 最大累计隐私预算
    float maxDeltaBudget{1e-3f}; ///< 最大累计 δ 预算（(ε,δ)-DP 组合追踪）
};

/**
 * @brief 独立差分隐私引擎（从 EdgeAIEngine 提取）
 *
 * 支持 Laplace 机制和 Gaussian 机制，带隐私预算追踪。
 * - Laplace 机制：noise ~ Lap(Δf / ε)
 * - Gaussian 机制：σ = Δ₂ · √(2·ln(1.25/δ)) / ε
 *
 * 线程安全：所有公开方法均可并发调用。
 */
class EdgeDifferentialPrivacy {
public:
    /**
     * @brief 配置差分隐私参数
     * @param config 差分隐私配置
     */
    void configure(const DPConfig& config);

    /**
     * @brief 对浮点值添加Laplace噪声
     *
     * Laplace机制：noise ~ Lap(Δf / ε)
     * 其中 Δf 为敏感度，ε 为隐私预算
     *
     * @param value 原始值
     * @param sensitivity 查询敏感度 Δf
     * @return 添加噪声后的值
     */
    float addLaplaceNoise(float value, float sensitivity);

    /**
     * @brief 对向量添加Laplace噪声（组合定理均分ε到每维）
     * @param values 原始向量
     * @param sensitivity L1敏感度
     * @return 添加噪声后的向量
     */
    std::vector<float> addLaplaceNoiseVec(const std::vector<float>& values, float sensitivity);

    /**
     * @brief 对向量添加Gaussian噪声（(ε,δ)-DP，高维场景推荐）
     *
     * Gaussian机制：σ = Δ₂ · √(2·ln(1.25/δ)) / ε
     * 噪声尺度 O(√d) 而非 Laplace 的 O(d)，d=128 时噪声降低约 11 倍
     *
     * 参考：Balle & Wang, "Improving the Gaussian Mechanism for DP", ICML 2018
     *
     * @param values 原始向量
     * @param sensitivity L2敏感度 Δ₂
     * @param delta 松弛参数 δ（默认使用 dpConfig_.delta）
     * @return 添加噪声后的向量
     */
    std::vector<float> addGaussianNoiseVec(const std::vector<float>& values,
                                            float sensitivity,
                                            float delta = 0.0f);

    /**
     * @brief 获取剩余隐私预算
     * @return 剩余 ε 值
     */
    float getRemainingPrivacyBudget() const;

    /**
     * @brief 获取剩余 δ 预算
     * @return 剩余 δ 值
     */
    float getRemainingDeltaBudget() const;

    /**
     * @brief 隐私预算是否已耗尽
     * @return true 如果 ε 预算已用完
     */
    bool isPrivacyBudgetExhausted() const;

    /**
     * @brief 重置隐私预算（新一轮计算，同时重置 ε 和 δ）
     */
    void resetPrivacyBudget();

    /**
     * @brief 获取当前差分隐私配置
     * @return DPConfig 配置副本
     */
    DPConfig getDPConfig() const;

    float getConsumedEpsilon() const { return consumedEpsilon_.load(); }
    float getConsumedDelta() const { return consumedDelta_.load(); }

private:
    DPConfig dpConfig_{};
    std::atomic<float> consumedEpsilon_{0.0f};
    std::atomic<float> consumedDelta_{0.0f};
    std::mt19937 dpRng_{std::random_device{}()};
    mutable std::mutex dpMutex_;

    /**
     * @brief Laplace分布采样（逆CDF方法）
     * @param scale 尺度参数 b
     * @return Laplace(0, scale) 样本
     */
    float sampleLaplace(float scale);

    /**
     * @brief Gaussian分布采样
     * @param sigma 标准差 σ
     * @return N(0, σ²) 样本
     */
    float sampleGaussian(float sigma);
};

} // namespace ai
} // namespace heartlake
