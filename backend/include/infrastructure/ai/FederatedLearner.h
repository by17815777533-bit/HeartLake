#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <random>
#include <json/json.h>

namespace heartlake {
namespace ai {

struct FederatedModelParams {
    std::string modelId;
    std::string nodeId;
    std::vector<std::vector<float>> weights;
    std::vector<float> biases;
    size_t sampleCount;
    float localLoss;
    int epoch;
    float mu = 0.0f;
};

class FederatedLearner {
public:
    void submitLocalModel(const FederatedModelParams& params);
    FederatedModelParams aggregateFedAvg(float clippingBound = 0.0f, float noiseSigma = 0.0f, float mu = 0.01f);
    Json::Value getFederatedStatus() const;
    size_t getPendingModelCount() const;
    int getCurrentRound() const;
    void clear();

private:
    std::vector<FederatedModelParams> localModels_;
    mutable std::mutex federatedMutex_;
    int federatedRound_ = 0;
    FederatedModelParams globalModel_;
    bool hasGlobalModel_ = false;
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace ai
} // namespace heartlake
