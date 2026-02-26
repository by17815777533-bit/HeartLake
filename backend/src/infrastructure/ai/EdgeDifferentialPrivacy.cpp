#include "infrastructure/ai/EdgeDifferentialPrivacy.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <cmath>

namespace heartlake {
namespace ai {

void EdgeDifferentialPrivacy::configure(const DPConfig& config) {
    // configure() 修改 dpConfig_，而 addLaplaceNoise 等方法读取 dpConfig_，
    // 必须加锁保护避免数据竞争
    std::unique_lock<std::shared_mutex> lock(dpMutex_);
    dpConfig_ = config;
    consumedEpsilon_.store(0.0f);
    consumedDelta_.store(0.0f);
    LOG_INFO << "[EdgeDP] Configured: epsilon=" << config.epsilon
             << ", delta=" << config.delta
             << ", sensitivity=" << config.sensitivity
             << ", maxEpsilonBudget=" << config.maxEpsilonBudget
             << ", maxDeltaBudget=" << config.maxDeltaBudget;
}

// ============================================================================
// Laplace 采样（逆CDF方法）
// ============================================================================

float EdgeDifferentialPrivacy::sampleLaplace(float scale) {
    // Laplace分布采样：使用逆CDF方法
    // 如果 U ~ Uniform(0,1)，则 X = μ - b * sign(U-0.5) * ln(1 - 2|U-0.5|)
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float u;
    {
        std::unique_lock<std::shared_mutex> lock(dpMutex_);
        u = dist(dpRng_);
    }

    // 避免 log(0)
    u = std::clamp(u, 1e-7f, 1.0f - 1e-7f);

    float centered = u - 0.5f;
    float sign = (centered >= 0.0f) ? 1.0f : -1.0f;
    float absVal = std::abs(centered);

    return -scale * sign * std::log(1.0f - 2.0f * absVal);
}

// ============================================================================
// Gaussian 采样
// ============================================================================

float EdgeDifferentialPrivacy::sampleGaussian(float sigma) {
    std::normal_distribution<float> dist(0.0f, sigma);
    std::unique_lock<std::shared_mutex> lock(dpMutex_);
    return dist(dpRng_);
}

// ============================================================================
// Laplace 机制
// ============================================================================

float EdgeDifferentialPrivacy::addLaplaceNoise(float value, float sensitivity) {
    // 读取 dpConfig_ 需要 shared_lock 保护，防止与 configure() 竞争
    float maxBudget, cfgEpsilon;
    {
        std::shared_lock<std::shared_mutex> rlock(dpMutex_);
        maxBudget = dpConfig_.maxEpsilonBudget;
        cfgEpsilon = dpConfig_.epsilon;
    }

    // 检查隐私预算
    float remaining = maxBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[EdgeDP] Privacy budget exhausted, returning original value";
        return value;
    }

    // 使用配置的epsilon，但不超过剩余预算
    float epsilon = std::min(cfgEpsilon, remaining);
    if (epsilon < 1e-10f) epsilon = 1e-10f;  // 防止除零

    // Laplace噪声尺度: b = Δf / ε
    float scale = sensitivity / epsilon;
    float noise = sampleLaplace(scale);

    // 消耗隐私预算（组合定理：线性累加）
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    return value + noise;
}

std::vector<float> EdgeDifferentialPrivacy::addLaplaceNoiseVec(const std::vector<float>& values,
                                                                float sensitivity) {
    // 读取 dpConfig_ 需要 shared_lock 保护
    float maxBudget, cfgEpsilon;
    {
        std::shared_lock<std::shared_mutex> rlock(dpMutex_);
        maxBudget = dpConfig_.maxEpsilonBudget;
        cfgEpsilon = dpConfig_.epsilon;
    }

    float remaining = maxBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[EdgeDP] Privacy budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(cfgEpsilon, remaining);
    if (epsilon < 1e-10f) epsilon = 1e-10f;  // 防止除零
    // 组合定理：将总预算均分到每个维度，确保总隐私消耗为 epsilon
    float perDimEpsilon = epsilon / static_cast<float>(values.size());
    float scale = sensitivity / perDimEpsilon;

    // 批量生成随机数，单次加锁减少锁竞争
    const size_t n = values.size();
    std::vector<float> uniforms(n);
    {
        std::unique_lock<std::shared_mutex> lock(dpMutex_);
        std::uniform_real_distribution<float> dist(1e-7f, 1.0f - 1e-7f);
        for (size_t i = 0; i < n; ++i) {
            uniforms[i] = dist(dpRng_);
        }
    }

    std::vector<float> result(n);
    for (size_t i = 0; i < n; ++i) {
        float centered = uniforms[i] - 0.5f;
        float sign = (centered >= 0.0f) ? 1.0f : -1.0f;
        float noise = -scale * sign * std::log(1.0f - 2.0f * std::abs(centered));
        result[i] = values[i] + noise;
    }

    // 消耗隐私预算
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    return result;
}

// ============================================================================
// Gaussian 机制（(ε,δ)-DP，高维场景推荐）
// ============================================================================

std::vector<float> EdgeDifferentialPrivacy::addGaussianNoiseVec(const std::vector<float>& values,
                                                                 float sensitivity,
                                                                 float delta) {
    if (values.empty()) return values;

    // 读取 dpConfig_ 需要 shared_lock 保护
    float cfgDelta, maxEpsBudget, maxDeltaBudget, cfgEpsilon;
    {
        std::shared_lock<std::shared_mutex> rlock(dpMutex_);
        cfgDelta = dpConfig_.delta;
        maxEpsBudget = dpConfig_.maxEpsilonBudget;
        maxDeltaBudget = dpConfig_.maxDeltaBudget;
        cfgEpsilon = dpConfig_.epsilon;
    }

    // 使用传入的 delta，若为 0 则使用配置默认值
    float useDelta = (delta > 0.0f) ? delta : cfgDelta;
    if (useDelta <= 0.0f || useDelta >= 1.0f) {
        LOG_WARN << "[EdgeDP] Invalid delta=" << useDelta << ", falling back to Laplace";
        return addLaplaceNoiseVec(values, sensitivity);
    }

    // 检查 epsilon 预算
    float remainingEps = maxEpsBudget - consumedEpsilon_.load();
    if (remainingEps <= 0.0f) {
        LOG_WARN << "[EdgeDP] Epsilon budget exhausted, returning original vector";
        return values;
    }

    // 检查 delta 预算
    float remainingDelta = maxDeltaBudget - consumedDelta_.load();
    if (remainingDelta <= 0.0f) {
        LOG_WARN << "[EdgeDP] Delta budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(cfgEpsilon, remainingEps);
    if (epsilon < 1e-10f) epsilon = 1e-10f;
    float effectiveDelta = std::min(useDelta, remainingDelta);

    // Gaussian 机制: σ = Δ₂ · √(2·ln(1.25/δ)) / ε
    // L2 sensitivity = sensitivity (调用方提供的是 L2 敏感度)
    float sigma = sensitivity * std::sqrt(2.0f * std::log(1.25f / effectiveDelta)) / epsilon;

    // 批量生成 Gaussian 噪声，单次加锁
    const size_t n = values.size();
    std::vector<float> result(n);
    {
        std::unique_lock<std::shared_mutex> lock(dpMutex_);
        std::normal_distribution<float> dist(0.0f, sigma);
        for (size_t i = 0; i < n; ++i) {
            result[i] = values[i] + dist(dpRng_);
        }
    }

    // 消耗隐私预算 (ε, δ) — 线性组合
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    float oldDelta = consumedDelta_.load();
    while (!consumedDelta_.compare_exchange_weak(oldDelta, oldDelta + effectiveDelta)) {}

    return result;
}

// ============================================================================
// 预算查询与管理
// ============================================================================

float EdgeDifferentialPrivacy::getRemainingPrivacyBudget() const {
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    float consumed = consumedEpsilon_.load();
    return std::max(0.0f, dpConfig_.maxEpsilonBudget - consumed);
}

float EdgeDifferentialPrivacy::getRemainingDeltaBudget() const {
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    float consumed = consumedDelta_.load();
    return std::max(0.0f, dpConfig_.maxDeltaBudget - consumed);
}

bool EdgeDifferentialPrivacy::isPrivacyBudgetExhausted() const {
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    return consumedEpsilon_.load() >= dpConfig_.maxEpsilonBudget;
}

void EdgeDifferentialPrivacy::resetPrivacyBudget() {
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    consumedEpsilon_.store(0.0f);
    consumedDelta_.store(0.0f);
    LOG_INFO << "[EdgeDP] Privacy budget reset. Max epsilon budget: "
             << dpConfig_.maxEpsilonBudget
             << ", max delta budget: " << dpConfig_.maxDeltaBudget;
}

DPConfig EdgeDifferentialPrivacy::getDPConfig() const {
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    return dpConfig_;
}

} // namespace ai
} // namespace heartlake
