/**
 * 联邦学习聚合器
 *
 * 从 EdgeAIEngine 提取的独立子系统，实现 FedAvg 联邦平均聚合算法。
 * 各边缘节点提交本地训练后的模型参数，聚合器按样本量加权平均，
 * 并支持梯度裁剪（L2 范数约束）和 DP 噪声注入以保护隐私。
 *
 * 聚合公式：
 *   w_global = Σ(n_k / N) * clip(w_k, C) + N(0, σ²)
 *   其中 n_k 为节点 k 的样本数，N 为总样本数，C 为裁剪阈值
 *
 * 线程安全：所有公开方法通过 mutex 保护。
 */
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <random>
#include <json/json.h>

namespace heartlake {
namespace ai {

/**
 * @brief 联邦学习本地模型参数包
 * @details 每个边缘节点训练完成后，将模型参数打包提交给聚合器。
 *          sampleCount 用于加权平均，localLoss 用于监控收敛情况。
 */
struct FederatedModelParams {
    std::string modelId;                          ///< 模型标识
    std::string nodeId;                           ///< 提交节点标识
    std::vector<std::vector<float>> weights;      ///< 权重矩阵（各层）
    std::vector<float> biases;                    ///< 偏置向量
    size_t sampleCount;                           ///< 本地训练样本数（用于加权）
    float localLoss;                              ///< 本地训练损失
    int epoch;                                    ///< 训练轮次
    float mu = 0.0f;                              ///< FedProx 近端项系数
};

/**
 * @brief 联邦学习聚合器 — FedAvg 加权平均 + 梯度裁剪 + DP 噪声
 *
 * @details 从 EdgeAIEngine 提取的独立子系统。各边缘节点提交本地训练后的模型参数，
 * 聚合器按样本量加权平均，并支持 L2 梯度裁剪和高斯 DP 噪声注入以保护隐私。
 *
 * 聚合公式：w_global = sum(n_k / N) * clip(w_k, C) + N(0, sigma^2)
 *
 * 线程安全：所有公开方法通过 mutex 保护。
 */
class FederatedLearner {
public:
    /**
     * @brief 提交本地训练模型参数
     * @param params 本地模型参数包
     */
    void submitLocalModel(const FederatedModelParams& params);

    /**
     * @brief 执行 FedAvg 聚合
     * @details 按样本量加权平均所有已提交的本地模型，可选梯度裁剪和 DP 噪声。
     *          聚合完成后清空本地模型缓冲区，轮次计数器 +1。
     * @param clippingBound L2 梯度裁剪阈值（0 表示不裁剪）
     * @param noiseSigma DP 高斯噪声标准差（0 表示不加噪）
     * @param mu FedProx 近端正则化系数
     * @return 聚合后的全局模型参数
     */
    FederatedModelParams aggregateFedAvg(float clippingBound = 0.0f, float noiseSigma = 0.0f, float mu = 0.01f);

    /** @brief 获取联邦学习状态（当前轮次、待聚合模型数、全局损失等） */
    Json::Value getFederatedStatus() const;

    /** @brief 获取待聚合的本地模型数量 */
    size_t getPendingModelCount() const;

    /** @brief 获取当前聚合轮次 */
    int getCurrentRound() const;

    /** @brief 清空所有状态（本地模型缓冲区、全局模型、轮次计数器） */
    void clear();

private:
    std::vector<FederatedModelParams> localModels_;  ///< 待聚合的本地模型缓冲区
    mutable std::mutex federatedMutex_;
    int federatedRound_ = 0;                         ///< 当前聚合轮次
    FederatedModelParams globalModel_;               ///< 最新全局模型
    bool hasGlobalModel_ = false;
    std::mt19937 rng_{std::random_device{}()};       ///< DP 噪声采样用随机数生成器
};

} // namespace ai
} // namespace heartlake
