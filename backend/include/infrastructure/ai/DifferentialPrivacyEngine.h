/**
 * 差分隐私引擎
 *
 * 从 EdgeAIEngine 提取的独立模块。Laplace机制，隐私预算追踪。
 *
 */

#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <random>
#include <vector>

namespace heartlake::ai {

/**
 * 差分隐私配置
 */
struct DPConfig {
    float epsilon;          ///< 隐私预算 ε（越小越隐私）
    float delta;            ///< 松弛参数 δ（Gaussian机制使用，默认1e-5）
    float sensitivity;      ///< 查询敏感度 Δf
    float maxEpsilonBudget; ///< 最大累计隐私预算
    float maxDeltaBudget{1e-3f}; ///< 最大累计 δ 预算（(ε,δ)-DP 组合追踪）
};

/**
 * 差分隐私引擎
 *
 * 提供 Laplace / Gaussian 噪声注入与隐私预算追踪。
 * 线程安全：内部使用 mutex 保护随机数生成器，atomic 保护预算计数器。
 */
class DifferentialPrivacyEngine {
public:
    DifferentialPrivacyEngine();
    explicit DifferentialPrivacyEngine(const DPConfig& config);

    /**
     * 设置是否启用
     */
    void setEnabled(bool enabled);

    /**
     * 检查引擎是否已启用
     */
    bool isEnabled() const;

    /**
     * 更新配置
     */
    void setConfig(const DPConfig& config);

    /**
     * 获取当前配置
     */
    DPConfig getConfig() const;

    /**
     * 对浮点值添加Laplace噪声
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
     * 对向量添加Laplace噪声（组合定理均分ε到每维）
     * @param values 原始向量
     * @param sensitivity L1敏感度
     * @return 添加噪声后的向量
     */
    std::vector<float> addLaplaceNoiseVec(const std::vector<float>& values, float sensitivity);

    /**
     * 对向量添加Gaussian噪声（(ε,δ)-DP，高维场景推荐）
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
     * 获取剩余隐私预算
     * @return 剩余 ε 值
     */
    float getRemainingPrivacyBudget() const;

    /**
     * 获取剩余 δ 预算
     * @return 剩余 δ 值
     */
    float getRemainingDeltaBudget() const;

    /**
     * 重置隐私预算（新一轮计算，同时重置 ε 和 δ）
     */
    void resetPrivacyBudget();

private:
    float sampleLaplace(float scale);
    float sampleGaussian(float sigma);

    std::atomic<bool> enabled_{false};  ///< 多线程读写，必须原子
    DPConfig dpConfig_;
    std::atomic<float> consumedEpsilon_{0.0f};  ///< 已消耗隐私预算 ε
    std::atomic<float> consumedDelta_{0.0f};    ///< 已消耗隐私预算 δ
    std::atomic<float> consumedRho_{0.0f};      ///< 已消耗 zCDP 预算 ρ（Gaussian机制）
    mutable std::shared_mutex dpMutex_;
    std::mt19937 dpRng_;                        ///< 随机数生成器
};

}  // namespace heartlake::ai
