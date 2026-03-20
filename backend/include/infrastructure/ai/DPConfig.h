/**
 * 差分隐私通用配置。
 *
 * 供 EdgeAI 子系统复用，避免多个头文件为了共享配置类型而互相依赖。
 */

#pragma once

namespace heartlake::ai {

struct DPConfig {
    float epsilon;               ///< 隐私预算 ε（越小越隐私）
    float delta;                 ///< 松弛参数 δ（Gaussian 机制使用，默认 1e-5）
    float sensitivity;           ///< 查询敏感度 Δf
    float maxEpsilonBudget;      ///< 最大累计隐私预算
    float maxDeltaBudget{1e-3f}; ///< 最大累计 δ 预算（(ε,δ)-DP 组合追踪）
};

}  // namespace heartlake::ai
