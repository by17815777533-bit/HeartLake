/**
 * @file EdgeAIEngine.h
 * @brief 边缘AI推理引擎 - 门面模式
 *
 * 八大核心子系统的统一入口，委托给独立的子系统类实现：
 * 1. SentimentAnalyzer - 轻量级情感分析
 * 2. ContentModerator - 本地文本审核
 * 3. EmotionPulseDetector - 实时情绪脉搏检测
 * 4. FederatedLearner - 联邦学习聚合器
 * 5. EdgeDifferentialPrivacy - 差分隐私噪声注入
 * 6. HNSWIndex - 本地向量相似度检索
 * 7. ModelQuantizer - 模型量化推理
 * 8. EdgeNodeMonitor - 边缘节点健康监控
 *
 * Created by 王璐瑶
 */

#pragma once

#include <memory>
#include <atomic>
#include <optional>
#include <string>
#include <vector>
#include <json/json.h>

#include "infrastructure/ai/SentimentAnalyzer.h"
#include "infrastructure/ai/ContentModerator.h"
#include "infrastructure/ai/EmotionPulseDetector.h"
#include "infrastructure/ai/FederatedLearner.h"
#include "infrastructure/ai/EdgeDifferentialPrivacy.h"
#include "infrastructure/ai/HNSWIndex.h"
#include "infrastructure/ai/ModelQuantizer.h"
#include "infrastructure/ai/EdgeNodeMonitor.h"

#ifdef HEARTLAKE_USE_ONNX
#include "infrastructure/ai/OnnxSentimentEngine.h"
#endif

namespace heartlake {
namespace ai {

/**
 * @brief 边缘AI推理引擎 - 门面类
 *
 * 单例模式，统一管理八大子系统。
 * 所有公开方法签名与拆分前完全一致，保持 API 向后兼容。
 */
class EdgeAIEngine {
public:
    static EdgeAIEngine& getInstance();

    EdgeAIEngine(const EdgeAIEngine&) = delete;
    EdgeAIEngine& operator=(const EdgeAIEngine&) = delete;

    void initialize(const Json::Value& config);
    void initialize() { initialize(Json::Value()); }
    bool isEnabled() const;

    // ========================================================================
    // 子系统1: 轻量级情感分析
    // ========================================================================
    EdgeSentimentResult analyzeSentimentLocal(const std::string& text);
    void loadEdgeSentimentLexicon();

    // ========================================================================
    // 子系统2: 本地文本审核
    // ========================================================================
    EdgeModerationResult moderateTextLocal(const std::string& text);
    void buildModerationAC();

    // ========================================================================
    // 子系统3: 实时情绪脉搏
    // ========================================================================
    void recordEmotion(const std::string& userId, const std::string& mood, float intensity);
    void submitEmotionSample(float score, const std::string& mood, float confidence = 1.0f);
    EmotionPulse getCurrentPulse();
    std::vector<EmotionPulse> getPulseHistory(int n = 10);

    // ========================================================================
    // 子系统4: 联邦学习聚合器
    // ========================================================================
    void submitLocalModel(const FederatedModelParams& params);
    FederatedModelParams aggregateFedAvg(float clippingBound = 0.0f,
                                          float noiseSigma = 0.0f,
                                          float mu = 0.01f);
    Json::Value getFederatedStatus() const;

    // ========================================================================
    // 子系统5: 差分隐私
    // ========================================================================
    float addLaplaceNoise(float value, float sensitivity = 1.0f);
    std::vector<float> addLaplaceNoiseVec(const std::vector<float>& values,
                                           float sensitivity = 1.0f);
    std::vector<float> addGaussianNoiseVec(const std::vector<float>& values,
                                            float sensitivity = 1.0f,
                                            float delta = 0.0f);
    float getRemainingPrivacyBudget() const;
    float getRemainingDeltaBudget() const;
    void resetPrivacyBudget();

    // ========================================================================
    // 子系统6: HNSW向量检索
    // ========================================================================
    void hnswInsert(const std::string& id, const std::vector<float>& vec);
    std::vector<VectorSearchResult> hnswSearch(const std::vector<float>& query, int k = 10);
    std::vector<VectorSearchResult> rerankHNSWCandidates(
        const std::vector<float>& query,
        const std::vector<VectorSearchResult>& candidates,
        int topK) const;
    bool hnswRemove(const std::string& id);
    size_t getHNSWVectorCount() const;
    Json::Value getHNSWStats() const;
    size_t getHNSWVectorDimension() const;

    // ========================================================================
    // 子系统7: 模型量化推理
    // ========================================================================
    ModelQuantizer::QuantizedTensor quantizeToInt8(const std::vector<float>& tensor,
                                                    const std::vector<size_t>& shape);
    std::vector<float> quantizedMatMul(const ModelQuantizer::QuantizedTensor& a,
                                        const ModelQuantizer::QuantizedTensor& b,
                                        size_t M, size_t K, size_t N);
    std::vector<float> quantizedForward(const std::vector<float>& input,
                                         const ModelQuantizer::QuantizedTensor& weights,
                                         const std::vector<float>& biases);

    // ========================================================================
    // 子系统8: 边缘节点监控
    // ========================================================================
    void registerNode(const std::string& nodeId);
    void reportNodeStatus(const EdgeNodeStatus& status);
    void updateNodeStatus(const EdgeNodeStatus& status);
    std::optional<std::string> selectBestNode() const;
    std::vector<EdgeNodeStatus> getAllNodeStatus() const;
    Json::Value getNodeDashboard() const;

    // ========================================================================
    // 综合统计
    // ========================================================================
    Json::Value getEngineStats() const;

    // ---- 子系统直接访问（高级用法）----
    SentimentAnalyzer& getSentimentAnalyzer() { if (!sentiment_) throw std::runtime_error("EdgeAIEngine not initialized"); return *sentiment_; }
    ContentModerator& getContentModerator() { if (!moderator_) throw std::runtime_error("EdgeAIEngine not initialized"); return *moderator_; }
    EmotionPulseDetector& getEmotionPulseDetector() { if (!pulse_) throw std::runtime_error("EdgeAIEngine not initialized"); return *pulse_; }
    FederatedLearner& getFederatedLearner() { if (!federated_) throw std::runtime_error("EdgeAIEngine not initialized"); return *federated_; }
    EdgeDifferentialPrivacy& getDifferentialPrivacy() { if (!dp_) throw std::runtime_error("EdgeAIEngine not initialized"); return *dp_; }
    HNSWIndex& getHNSWIndex() { if (!hnsw_) throw std::runtime_error("EdgeAIEngine not initialized"); return *hnsw_; }
    ModelQuantizer& getModelQuantizer() { if (!quantizer_) throw std::runtime_error("EdgeAIEngine not initialized"); return *quantizer_; }
    EdgeNodeMonitor& getEdgeNodeMonitor() { if (!monitor_) throw std::runtime_error("EdgeAIEngine not initialized"); return *monitor_; }

private:
    EdgeAIEngine() = default;
    std::atomic<bool> enabled_{false};      ///< 多线程读写，必须原子
    std::atomic<bool> initialized_{false};  ///< 多线程读写，必须原子

    // 8 个子系统
    std::unique_ptr<SentimentAnalyzer> sentiment_;
    std::unique_ptr<ContentModerator> moderator_;
    std::unique_ptr<EmotionPulseDetector> pulse_;
    std::unique_ptr<FederatedLearner> federated_;
    std::unique_ptr<EdgeDifferentialPrivacy> dp_;
    std::unique_ptr<HNSWIndex> hnsw_;
    std::unique_ptr<ModelQuantizer> quantizer_;
    std::unique_ptr<EdgeNodeMonitor> monitor_;

#ifdef HEARTLAKE_USE_ONNX
    std::unique_ptr<OnnxSentimentEngine> onnxEngine_;
    bool onnxEnabled_ = false;
#endif
};

} // namespace ai
} // namespace heartlake