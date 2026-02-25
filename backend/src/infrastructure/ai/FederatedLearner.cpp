#include "infrastructure/ai/FederatedLearner.h"
#include <cmath>
#include <drogon/drogon.h>

namespace heartlake {
namespace ai {

void FederatedLearner::submitLocalModel(const FederatedModelParams& params) {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    localModels_.push_back(params);
    LOG_DEBUG << "[FederatedLearner] Local model submitted from node: " << params.nodeId
              << ", samples: " << params.sampleCount
              << ", loss: " << params.localLoss;
}

FederatedModelParams FederatedLearner::aggregateFedAvg(float clippingBound, float noiseSigma, float mu) {
    std::lock_guard<std::mutex> lock(federatedMutex_);

    FederatedModelParams global;
    global.modelId = "global_fedavg";
    global.epoch = ++federatedRound_;
    global.nodeId = "aggregator";
    global.mu = mu;

    if (localModels_.empty()) {
        LOG_WARN << "[FederatedLearner] No local models to aggregate";
        global.sampleCount = 0;
        global.localLoss = 0.0f;
        // 确保返回的模型有空的 weights/biases，避免调用方解引用未初始化数据
        global.weights = {};
        global.biases = {};
        return global;
    }

    // DP-SGD Step 1: 对每个本地模型的权重进行 ℓ2-norm 梯度裁剪
    if (clippingBound > 0.0f) {
        for (auto& model : localModels_) {
            // 计算该模型所有权重的 ℓ2 范数
            float l2norm = 0.0f;
            for (const auto& layer : model.weights) {
                for (float w : layer) {
                    l2norm += w * w;
                }
            }
            for (float b : model.biases) {
                l2norm += b * b;
            }
            l2norm = std::sqrt(l2norm);

            // 如果范数超过阈值C，按比例缩放: w = w * (C / ||w||)
            if (l2norm > clippingBound) {
                float scale = clippingBound / l2norm;
                for (auto& layer : model.weights) {
                    for (float& w : layer) {
                        w *= scale;
                    }
                }
                for (float& b : model.biases) {
                    b *= scale;
                }
                LOG_DEBUG << "[FederatedLearner] Clipped model " << model.nodeId
                          << " from norm=" << l2norm << " to " << clippingBound;
            }
        }
    }

    // 计算总样本数
    size_t totalSamples = 0;
    for (const auto& model : localModels_) {
        totalSamples += model.sampleCount;
    }
    if (totalSamples == 0) {
        LOG_WARN << "[FederatedLearner] Total samples is 0, using equal weights";
        totalSamples = localModels_.size();  // 等权重回退
    }

    if (totalSamples == 0) {
        LOG_WARN << "[FederatedLearner] Total sample count is zero, cannot aggregate";
        global.sampleCount = 0;
        global.localLoss = 0.0f;
        localModels_.clear();
        return global;
    }

    global.sampleCount = totalSamples;

    // 确定权重矩阵维度（以第一个模型为参考）
    const auto& ref = localModels_[0];
    size_t numLayers = ref.weights.size();
    size_t biasSize = ref.biases.size();

    // 初始化全局权重为零
    global.weights.resize(numLayers);
    for (size_t l = 0; l < numLayers; ++l) {
        global.weights[l].resize(ref.weights[l].size(), 0.0f);
    }
    global.biases.resize(biasSize, 0.0f);
    global.localLoss = 0.0f;

    // FedAvg加权聚合: w_global = Σ(n_k / n) * w_k
    for (const auto& model : localModels_) {
        float weight = static_cast<float>(model.sampleCount) / static_cast<float>(totalSamples);

        // 聚合权重矩阵
        for (size_t l = 0; l < numLayers && l < model.weights.size(); ++l) {
            for (size_t j = 0; j < model.weights[l].size() && j < global.weights[l].size(); ++j) {
                global.weights[l][j] += weight * model.weights[l][j];
            }
        }

        // 聚合偏置
        for (size_t j = 0; j < model.biases.size() && j < global.biases.size(); ++j) {
            global.biases[j] += weight * model.biases[j];
        }

        // 加权平均损失
        global.localLoss += weight * model.localLoss;
    }

    // FedProx近端修正: w_final = (w_aggregated + mu * w_global) / (1 + mu)
    // 当mu=0时退化为标准FedAvg；当有全局模型参考时施加近端约束
    // 参考: Li et al., "Federated Optimization in Heterogeneous Networks", MLSys 2020
    if (mu > 0.0f && hasGlobalModel_) {
        float denom = 1.0f / (1.0f + mu);

        for (size_t l = 0; l < numLayers; ++l) {
            size_t layerSize = global.weights[l].size();
            size_t refLayerSize = (l < globalModel_.weights.size()) ? globalModel_.weights[l].size() : 0;
            for (size_t j = 0; j < layerSize; ++j) {
                float wGlobal = (j < refLayerSize) ? globalModel_.weights[l][j] : 0.0f;
                // (w_aggregated + mu * w_prev_global) / (1 + mu)
                global.weights[l][j] = (global.weights[l][j] + mu * wGlobal) * denom;
            }
        }

        size_t refBiasSize = globalModel_.biases.size();
        for (size_t j = 0; j < biasSize; ++j) {
            float bGlobal = (j < refBiasSize) ? globalModel_.biases[j] : 0.0f;
            global.biases[j] = (global.biases[j] + mu * bGlobal) * denom;
        }

        LOG_INFO << "[FederatedLearner] FedProx proximal correction applied: mu=" << mu;
    } else if (mu > 0.0f && !hasGlobalModel_) {
        LOG_INFO << "[FederatedLearner] FedProx mu=" << mu << " but no prior global model, "
                 << "first round uses standard FedAvg";
    }

    // DP-SGD Step 2: 对聚合后的全局权重添加高斯噪声 N(0, σ²C²I)
    if (noiseSigma > 0.0f && clippingBound > 0.0f) {
        float noiseScale = noiseSigma * clippingBound;
        std::normal_distribution<float> dist(0.0f, noiseScale);

        for (auto& layer : global.weights) {
            for (float& w : layer) {
                w += dist(rng_);
            }
        }
        for (float& b : global.biases) {
            b += dist(rng_);
        }

        LOG_INFO << "[FederatedLearner] Added Gaussian noise: σ=" << noiseSigma
                 << ", C=" << clippingBound << ", noise_scale=" << noiseScale;
    }

    LOG_INFO << "[FederatedLearner] FedProx aggregation complete: round=" << federatedRound_
             << ", participants=" << localModels_.size()
             << ", totalSamples=" << totalSamples
             << ", avgLoss=" << global.localLoss
             << ", mu=" << mu;

    // 保存当前全局模型作为下一轮FedProx的近端参考
    globalModel_ = global;
    hasGlobalModel_ = true;

    // 清空本地模型缓存，准备下一轮
    localModels_.clear();

    return global;
}

size_t FederatedLearner::getPendingModelCount() const {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    return localModels_.size();
}

int FederatedLearner::getCurrentRound() const {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    return federatedRound_;
}

void FederatedLearner::clear() {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    localModels_.clear();
    globalModel_ = FederatedModelParams{};
    hasGlobalModel_ = false;
    federatedRound_ = 0;
}

Json::Value FederatedLearner::getFederatedStatus() const {
    std::lock_guard<std::mutex> lock(federatedMutex_);
    Json::Value status;
    status["current_round"] = federatedRound_;
    status["pending_models"] = static_cast<int>(localModels_.size());

    size_t totalSamples = 0;
    Json::Value nodes(Json::arrayValue);
    for (const auto& model : localModels_) {
        Json::Value node;
        node["node_id"] = model.nodeId;
        node["sample_count"] = static_cast<Json::UInt64>(model.sampleCount);
        node["local_loss"] = model.localLoss;
        node["epoch"] = model.epoch;
        nodes.append(node);
        totalSamples += model.sampleCount;
    }
    status["nodes"] = nodes;
    status["total_pending_samples"] = static_cast<Json::UInt64>(totalSamples);

    return status;
}

} // namespace ai
} // namespace heartlake
