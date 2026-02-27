/**
 * 差分隐私引擎
 *
 * 从 EdgeAIEngine 提取的独立模块。Laplace机制，隐私预算追踪。
 *
 */

#include "infrastructure/ai/DifferentialPrivacyEngine.h"

#include <algorithm>
#include <cmath>
#include <mutex>

#include <drogon/drogon.h>

namespace heartlake::ai {

namespace {

constexpr float kMinDelta = 1e-12f;

float clampDelta(float delta) {
    return std::clamp(delta, kMinDelta, 1.0f - kMinDelta);
}

float rhoFromEpsilonDelta(float epsilon, float delta) {
    const float eps = std::max(epsilon, 1e-10f);
    const float d = clampDelta(delta);
    const float a = std::sqrt(std::log(1.0f / d));
    const float x = -a + std::sqrt(a * a + eps);
    return std::max(x * x, 1e-12f);
}

float epsilonFromRhoDelta(float rho, float delta) {
    const float d = clampDelta(delta);
    const float logInv = std::log(1.0f / d);
    return rho + 2.0f * std::sqrt(std::max(0.0f, rho * logInv));
}

}  // namespace

DifferentialPrivacyEngine::DifferentialPrivacyEngine()
    : dpConfig_{1.0f, 1e-5f, 1.0f, 10.0f, 1e-3f}
    , dpRng_(std::random_device{}()) {}

DifferentialPrivacyEngine::DifferentialPrivacyEngine(const DPConfig& config)
    : dpConfig_(config)
    , dpRng_(std::random_device{}()) {}

void DifferentialPrivacyEngine::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool DifferentialPrivacyEngine::isEnabled() const {
    return enabled_;
}

void DifferentialPrivacyEngine::setConfig(const DPConfig& config) {
    std::unique_lock<std::shared_mutex> lock(dpMutex_);
    dpConfig_ = config;
    consumedRho_.store(0.0f);
}

DPConfig DifferentialPrivacyEngine::getConfig() const {
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    return dpConfig_;
}

float DifferentialPrivacyEngine::sampleLaplace(float scale) {
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

float DifferentialPrivacyEngine::sampleGaussian(float sigma) {
    // Gaussian 采样：使用 std::normal_distribution
    std::normal_distribution<float> dist(0.0f, sigma);
    std::unique_lock<std::shared_mutex> lock(dpMutex_);
    return dist(dpRng_);
}

float DifferentialPrivacyEngine::addLaplaceNoise(float value, float sensitivity) {
    if (!isEnabled()) return value;

    // 读取配置需要 shared_lock
    float maxBudget, cfgEpsilon;
    {
        std::shared_lock<std::shared_mutex> rlock(dpMutex_);
        maxBudget = dpConfig_.maxEpsilonBudget;
        cfgEpsilon = dpConfig_.epsilon;
    }

    // 检查隐私预算
    float remaining = maxBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Privacy budget exhausted, returning original value";
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

std::vector<float> DifferentialPrivacyEngine::addLaplaceNoiseVec(const std::vector<float>& values,
                                                                  float sensitivity) {
    if (!isEnabled()) return values;

    float maxBudget, cfgEpsilon;
    {
        std::shared_lock<std::shared_mutex> rlock(dpMutex_);
        maxBudget = dpConfig_.maxEpsilonBudget;
        cfgEpsilon = dpConfig_.epsilon;
    }

    float remaining = maxBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Privacy budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(cfgEpsilon, remaining);
    if (epsilon < 1e-10f) epsilon = 1e-10f;  // 防止除零
    // 组合定理：将总预算均分到每个维度，确保总隐私消耗为 epsilon
    float perDimEpsilon = epsilon / static_cast<float>(values.size());
    float scale = sensitivity / perDimEpsilon;

    std::vector<float> result(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        result[i] = values[i] + sampleLaplace(scale);
    }

    // 消耗隐私预算
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    return result;
}

std::vector<float> DifferentialPrivacyEngine::addGaussianNoiseVec(const std::vector<float>& values,
                                                                   float sensitivity,
                                                                   float delta) {
    if (!isEnabled()) return values;
    if (values.empty()) return values;

    // 读取配置需要 shared_lock
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
        LOG_WARN << "[DifferentialPrivacy] Invalid delta=" << useDelta << ", falling back to Laplace";
        return addLaplaceNoiseVec(values, sensitivity);
    }

    // 检查 epsilon 预算
    float remainingEps = maxEpsBudget - consumedEpsilon_.load();
    if (remainingEps <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Epsilon budget exhausted, returning original vector";
        return values;
    }

    // 检查 delta 预算
    float remainingDelta = maxDeltaBudget - consumedDelta_.load();
    if (remainingDelta <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Delta budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(cfgEpsilon, remainingEps);
    if (epsilon < 1e-10f) epsilon = 1e-10f;
    float effectiveDelta = std::min(useDelta, remainingDelta);
    if (effectiveDelta <= 0.0f || effectiveDelta >= 1.0f) {
        LOG_WARN << "[DifferentialPrivacy] Effective delta invalid, returning original vector";
        return values;
    }

    const float targetRho = rhoFromEpsilonDelta(epsilon, effectiveDelta);
    const float rhoBudget = rhoFromEpsilonDelta(
        maxEpsBudget,
        std::min(std::max(maxDeltaBudget, kMinDelta), 1.0f - kMinDelta));
    const float remainingRho = rhoBudget - consumedRho_.load();
    if (remainingRho <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Rho budget exhausted, returning original vector";
        return values;
    }
    const float rhoToUse = std::min(targetRho, remainingRho);
    if (rhoToUse <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Rho budget insufficient, returning original vector";
        return values;
    }

    // Gaussian 对应 zCDP: ρ = Δ²/(2σ²) => σ = Δ/sqrt(2ρ)
    const float sigma = sensitivity / std::sqrt(2.0f * rhoToUse);

    std::vector<float> result(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        result[i] = values[i] + sampleGaussian(sigma);
    }

    float oldRho = consumedRho_.load();
    while (!consumedRho_.compare_exchange_weak(oldRho, oldRho + rhoToUse)) {}

    // 消耗隐私预算 (ε, δ) — 线性组合
    const float epsilonSpent = epsilonFromRhoDelta(rhoToUse, effectiveDelta);
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilonSpent)) {}

    float oldDelta = consumedDelta_.load();
    while (!consumedDelta_.compare_exchange_weak(oldDelta, oldDelta + effectiveDelta)) {}

    return result;
}

float DifferentialPrivacyEngine::getRemainingPrivacyBudget() const {
    float consumed = consumedEpsilon_.load();
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    return std::max(0.0f, dpConfig_.maxEpsilonBudget - consumed);
}

float DifferentialPrivacyEngine::getRemainingDeltaBudget() const {
    float consumed = consumedDelta_.load();
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    return std::max(0.0f, dpConfig_.maxDeltaBudget - consumed);
}

void DifferentialPrivacyEngine::resetPrivacyBudget() {
    consumedEpsilon_.store(0.0f);
    consumedDelta_.store(0.0f);
    consumedRho_.store(0.0f);
    std::shared_lock<std::shared_mutex> rlock(dpMutex_);
    LOG_INFO << "[DifferentialPrivacy] Privacy budget reset. Max epsilon budget: "
             << dpConfig_.maxEpsilonBudget << ", max delta budget: " << dpConfig_.maxDeltaBudget;
}

}  // namespace heartlake::ai
