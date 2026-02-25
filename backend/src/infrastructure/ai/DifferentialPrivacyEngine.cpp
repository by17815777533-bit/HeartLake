/**
 * @file DifferentialPrivacyEngine.cpp
 * @brief 差分隐私引擎
 *
 * 从 EdgeAIEngine 提取的独立模块。Laplace机制，隐私预算追踪。
 *
 * Created by 王璐瑶
 */

#include "infrastructure/ai/DifferentialPrivacyEngine.h"

#include <algorithm>
#include <cmath>
#include <mutex>

#include <drogon/drogon.h>

namespace heartlake::ai {

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
    dpConfig_ = config;
}

const DPConfig& DifferentialPrivacyEngine::getConfig() const {
    return dpConfig_;
}

float DifferentialPrivacyEngine::sampleLaplace(float scale) {
    // Laplace分布采样：使用逆CDF方法
    // 如果 U ~ Uniform(0,1)，则 X = μ - b * sign(U-0.5) * ln(1 - 2|U-0.5|)
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float u;
    {
        std::lock_guard<std::mutex> lock(dpMutex_);
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
    std::lock_guard<std::mutex> lock(dpMutex_);
    return dist(dpRng_);
}

float DifferentialPrivacyEngine::addLaplaceNoise(float value, float sensitivity) {
    if (!isEnabled()) return value;

    // 检查隐私预算
    float remaining = dpConfig_.maxEpsilonBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Privacy budget exhausted, returning original value";
        return value;
    }

    // 使用配置的epsilon，但不超过剩余预算
    float epsilon = std::min(dpConfig_.epsilon, remaining);
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

    float remaining = dpConfig_.maxEpsilonBudget - consumedEpsilon_.load();
    if (remaining <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Privacy budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(dpConfig_.epsilon, remaining);
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

    // 使用传入的 delta，若为 0 则使用配置默认值
    float useDelta = (delta > 0.0f) ? delta : dpConfig_.delta;
    if (useDelta <= 0.0f || useDelta >= 1.0f) {
        LOG_WARN << "[DifferentialPrivacy] Invalid delta=" << useDelta << ", falling back to Laplace";
        return addLaplaceNoiseVec(values, sensitivity);
    }

    // 检查 epsilon 预算
    float remainingEps = dpConfig_.maxEpsilonBudget - consumedEpsilon_.load();
    if (remainingEps <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Epsilon budget exhausted, returning original vector";
        return values;
    }

    // 检查 delta 预算
    float remainingDelta = dpConfig_.maxDeltaBudget - consumedDelta_.load();
    if (remainingDelta <= 0.0f) {
        LOG_WARN << "[DifferentialPrivacy] Delta budget exhausted, returning original vector";
        return values;
    }

    float epsilon = std::min(dpConfig_.epsilon, remainingEps);
    if (epsilon < 1e-10f) epsilon = 1e-10f;
    float effectiveDelta = std::min(useDelta, remainingDelta);

    // Gaussian 机制: σ = Δ₂ · √(2·ln(1.25/δ)) / ε
    // L2 sensitivity = sensitivity (调用方提供的是 L2 敏感度)
    float sigma = sensitivity * std::sqrt(2.0f * std::log(1.25f / effectiveDelta)) / epsilon;

    std::vector<float> result(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        result[i] = values[i] + sampleGaussian(sigma);
    }

    // 消耗隐私预算 (ε, δ) — 线性组合
    float oldEps = consumedEpsilon_.load();
    while (!consumedEpsilon_.compare_exchange_weak(oldEps, oldEps + epsilon)) {}

    float oldDelta = consumedDelta_.load();
    while (!consumedDelta_.compare_exchange_weak(oldDelta, oldDelta + effectiveDelta)) {}

    return result;
}

float DifferentialPrivacyEngine::getRemainingPrivacyBudget() const {
    float consumed = consumedEpsilon_.load();
    return std::max(0.0f, dpConfig_.maxEpsilonBudget - consumed);
}

float DifferentialPrivacyEngine::getRemainingDeltaBudget() const {
    float consumed = consumedDelta_.load();
    return std::max(0.0f, dpConfig_.maxDeltaBudget - consumed);
}

void DifferentialPrivacyEngine::resetPrivacyBudget() {
    consumedEpsilon_.store(0.0f);
    consumedDelta_.store(0.0f);
    LOG_INFO << "[DifferentialPrivacy] Privacy budget reset. Max epsilon budget: "
             << dpConfig_.maxEpsilonBudget << ", max delta budget: " << dpConfig_.maxDeltaBudget;
}

}  // namespace heartlake::ai
