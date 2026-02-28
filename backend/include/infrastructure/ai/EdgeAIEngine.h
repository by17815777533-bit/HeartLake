/**
 * 边缘AI推理引擎 - 门面模式
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
 */

#pragma once

#include <memory>
#include <atomic>
#include <mutex>
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
 * @brief 边缘 AI 推理引擎 — 八大子系统的门面类
 *
 * @details 单例模式，统一管理八大子系统的生命周期和对外接口。
 * 所有公开方法签名与拆分前完全一致，保持 API 向后兼容。
 * 内部通过 unique_ptr 持有各子系统实例，initialize() 通过 call_once 保证只执行一次。
 *
 * 子系统列表：
 * 1. SentimentAnalyzer — 轻量级情感分析（三层融合 + LRU 缓存）
 * 2. ContentModerator — 本地文本审核（AC 自动机 + 五因子心理风险评估）
 * 3. EmotionPulseDetector — 实时情绪脉搏检测（滑动窗口 + EWMA）
 * 4. FederatedLearner — 联邦学习聚合器（FedAvg + 梯度裁剪 + DP 噪声）
 * 5. EdgeDifferentialPrivacy — 差分隐私噪声注入（Laplace / Gaussian）
 * 6. HNSWIndex — 本地向量相似度检索（多层图 + Ada-EF + Matryoshka 重排序）
 * 7. ModelQuantizer — 模型量化推理（INT8 对称量化 + 量化矩阵乘法）
 * 8. EdgeNodeMonitor — 边缘节点健康监控（熔断器 + 自适应负载均衡）
 */
class EdgeAIEngine {
public:
    /** @brief 获取全局单例 */
    static EdgeAIEngine& getInstance();

    EdgeAIEngine(const EdgeAIEngine&) = delete;
    EdgeAIEngine& operator=(const EdgeAIEngine&) = delete;

    /**
     * @brief 初始化引擎及全部子系统
     * @param config JSON 配置（各子系统参数，如 HNSW 维度、DP epsilon 等）
     * @note 通过 call_once 保证只执行一次，重复调用安全但无效
     */
    void initialize(const Json::Value& config);

    /** @brief 使用默认配置初始化 */
    void initialize() { initialize(Json::Value()); }

    /** @brief 引擎是否已启用（初始化成功后为 true） */
    bool isEnabled() const;

    // ========================================================================
    // 子系统1: 轻量级情感分析
    // ========================================================================

    /**
     * @brief 本地情感分析（三层融合：规则 + 词典 + 统计）
     * @param text 待分析文本
     * @param preferOnnx 是否优先使用 ONNX 模型
     * @return 情感分析结果
     */
    EdgeSentimentResult analyzeSentimentLocal(const std::string& text, bool preferOnnx = false);

    /** @brief 加载情感词典（中英文情感词 + 程度副词 + 否定词） */
    void loadEdgeSentimentLexicon();

    // ========================================================================
    // 子系统2: 本地文本审核
    // ========================================================================

    /**
     * @brief 本地文本审核（AC 自动机 + 五因子心理风险评估）
     * @param text 待审核文本
     * @return 审核结果
     */
    EdgeModerationResult moderateTextLocal(const std::string& text);

    /** @brief 构建 AC 自动机敏感词库 */
    void buildModerationAC();

    // ========================================================================
    // 子系统3: 实时情绪脉搏
    // ========================================================================

    /**
     * @brief 记录用户情绪（写入情绪脉搏检测器）
     * @param userId 用户 ID
     * @param mood 情绪类型
     * @param intensity 情绪强度 [0, 1]
     */
    void recordEmotion(const std::string& userId, const std::string& mood, float intensity);

    /**
     * @brief 提交情绪样本到脉搏检测器
     * @param score 情感分数 [-1.0, 1.0]
     * @param mood 情绪类型
     * @param confidence 置信度
     */
    void submitEmotionSample(float score, const std::string& mood, float confidence = 1.0f);

    /** @brief 获取当前情绪脉搏快照 */
    EmotionPulse getCurrentPulse();

    /**
     * @brief 获取情绪脉搏历史
     * @param n 返回最近 n 个快照
     */
    std::vector<EmotionPulse> getPulseHistory(int n = 10);

    // ========================================================================
    // 子系统4: 联邦学习聚合器
    // ========================================================================

    /**
     * @brief 提交本地训练模型参数
     * @param params 本地模型参数包
     */
    void submitLocalModel(const FederatedModelParams& params);

    /**
     * @brief 执行 FedAvg 聚合
     * @param clippingBound L2 梯度裁剪阈值（0 = 不裁剪）
     * @param noiseSigma DP 高斯噪声标准差（0 = 不加噪）
     * @param mu FedProx 近端正则化系数
     * @return 聚合后的全局模型参数
     */
    FederatedModelParams aggregateFedAvg(float clippingBound = 0.0f,
                                          float noiseSigma = 0.0f,
                                          float mu = 0.01f);

    /** @brief 获取联邦学习状态（轮次、待聚合数、全局损失等） */
    Json::Value getFederatedStatus() const;

    // ========================================================================
    // 子系统5: 差分隐私
    // ========================================================================

    /**
     * @brief 对标量添加 Laplace 噪声
     * @param value 原始值
     * @param sensitivity 查询敏感度
     */
    float addLaplaceNoise(float value, float sensitivity = 1.0f);

    /**
     * @brief 对向量添加 Laplace 噪声（组合定理均分 epsilon）
     */
    std::vector<float> addLaplaceNoiseVec(const std::vector<float>& values,
                                           float sensitivity = 1.0f);

    /**
     * @brief 对向量添加 Gaussian 噪声（高维场景推荐）
     * @param delta 松弛参数（0 = 使用默认配置）
     */
    std::vector<float> addGaussianNoiseVec(const std::vector<float>& values,
                                            float sensitivity = 1.0f,
                                            float delta = 0.0f);

    /** @brief 获取剩余 epsilon 预算 */
    float getRemainingPrivacyBudget() const;
    /** @brief 获取剩余 delta 预算 */
    float getRemainingDeltaBudget() const;
    /** @brief 重置隐私预算（开始新一轮计算） */
    void resetPrivacyBudget();

    // ========================================================================
    // 子系统6: HNSW 向量检索
    // ========================================================================

    /**
     * @brief 插入向量到 HNSW 索引
     * @param id 唯一标识
     * @param vec 向量数据
     */
    void hnswInsert(const std::string& id, const std::vector<float>& vec);

    /**
     * @brief KNN 搜索
     * @param query 查询向量
     * @param k 返回最近邻数量
     */
    std::vector<VectorSearchResult> hnswSearch(const std::vector<float>& query, int k = 10);

    /**
     * @brief Matryoshka 融合 cosine 重排序
     * @param query 查询向量
     * @param candidates 候选结果集
     * @param topK 返回数量
     */
    std::vector<VectorSearchResult> rerankHNSWCandidates(
        const std::vector<float>& query,
        const std::vector<VectorSearchResult>& candidates,
        int topK) const;

    /** @brief 删除向量（逻辑删除 + 邻居断链） */
    bool hnswRemove(const std::string& id);
    /** @brief 获取索引中向量数量 */
    size_t getHNSWVectorCount() const;
    /** @brief 获取 HNSW 索引统计信息 */
    Json::Value getHNSWStats() const;
    /** @brief 获取当前向量维度 */
    size_t getHNSWVectorDimension() const;

    // ========================================================================
    // 子系统7: 模型量化推理
    // ========================================================================

    /** @brief float32 -> INT8 对称量化 */
    ModelQuantizer::QuantizedTensor quantizeToInt8(const std::vector<float>& tensor,
                                                    const std::vector<size_t>& shape);

    /** @brief INT8 量化矩阵乘法 (M*K) x (K*N) -> (M*N) */
    std::vector<float> quantizedMatMul(const ModelQuantizer::QuantizedTensor& a,
                                        const ModelQuantizer::QuantizedTensor& b,
                                        size_t M, size_t K, size_t N);

    /** @brief 量化前向推理（量化 -> 矩阵乘 -> 加偏置 -> ReLU） */
    std::vector<float> quantizedForward(const std::vector<float>& input,
                                         const ModelQuantizer::QuantizedTensor& weights,
                                         const std::vector<float>& biases);

    // ========================================================================
    // 子系统8: 边缘节点监控
    // ========================================================================

    /** @brief 注册边缘节点 */
    void registerNode(const std::string& nodeId);
    /** @brief 上报节点状态 */
    void reportNodeStatus(const EdgeNodeStatus& status);
    /** @brief 更新节点状态（心跳） */
    void updateNodeStatus(const EdgeNodeStatus& status);
    /** @brief 选择最优节点（自适应负载均衡） */
    std::optional<std::string> selectBestNode() const;
    /** @brief 获取所有节点状态 */
    std::vector<EdgeNodeStatus> getAllNodeStatus() const;
    /** @brief 获取节点监控面板数据 */
    Json::Value getNodeDashboard() const;

    // ========================================================================
    // 综合统计
    // ========================================================================

    /** @brief 获取引擎全局统计信息（汇总各子系统指标） */
    Json::Value getEngineStats() const;

    // ---- 子系统直接访问（高级用法，绕过门面层直接操作子系统） ----
    /** @brief 获取情感分析子系统引用 @throws std::runtime_error 未初始化时抛出 */
    SentimentAnalyzer& getSentimentAnalyzer() { if (!sentiment_) throw std::runtime_error("EdgeAIEngine not initialized"); return *sentiment_; }
    /** @brief 获取内容审核子系统引用 */
    ContentModerator& getContentModerator() { if (!moderator_) throw std::runtime_error("EdgeAIEngine not initialized"); return *moderator_; }
    /** @brief 获取情绪脉搏检测子系统引用 */
    EmotionPulseDetector& getEmotionPulseDetector() { if (!pulse_) throw std::runtime_error("EdgeAIEngine not initialized"); return *pulse_; }
    /** @brief 获取联邦学习子系统引用 */
    FederatedLearner& getFederatedLearner() { if (!federated_) throw std::runtime_error("EdgeAIEngine not initialized"); return *federated_; }
    /** @brief 获取差分隐私子系统引用 */
    EdgeDifferentialPrivacy& getDifferentialPrivacy() { if (!dp_) throw std::runtime_error("EdgeAIEngine not initialized"); return *dp_; }
    /** @brief 获取 HNSW 索引子系统引用 */
    HNSWIndex& getHNSWIndex() { if (!hnsw_) throw std::runtime_error("EdgeAIEngine not initialized"); return *hnsw_; }
    /** @brief 获取模型量化子系统引用 */
    ModelQuantizer& getModelQuantizer() { if (!quantizer_) throw std::runtime_error("EdgeAIEngine not initialized"); return *quantizer_; }
    /** @brief 获取节点监控子系统引用 */
    EdgeNodeMonitor& getEdgeNodeMonitor() { if (!monitor_) throw std::runtime_error("EdgeAIEngine not initialized"); return *monitor_; }

private:
    EdgeAIEngine() = default;
    /** @brief 实际初始化逻辑，由 call_once 保证只执行一次 */
    void initializeImpl(const Json::Value& config);
    std::once_flag initFlag_;               ///< 保证 initialize() 只执行一次
    std::atomic<bool> enabled_{false};      ///< 引擎启用标记，acquire/release 语义
    std::atomic<bool> initialized_{false};  ///< 初始化完成标记，acquire/release 语义

    // 八大子系统实例
    std::unique_ptr<SentimentAnalyzer> sentiment_;        ///< 子系统1: 情感分析
    std::unique_ptr<ContentModerator> moderator_;         ///< 子系统2: 内容审核
    std::unique_ptr<EmotionPulseDetector> pulse_;         ///< 子系统3: 情绪脉搏
    std::unique_ptr<FederatedLearner> federated_;         ///< 子系统4: 联邦学习
    std::unique_ptr<EdgeDifferentialPrivacy> dp_;          ///< 子系统5: 差分隐私
    std::unique_ptr<HNSWIndex> hnsw_;                     ///< 子系统6: HNSW 向量检索
    std::unique_ptr<ModelQuantizer> quantizer_;           ///< 子系统7: 模型量化
    std::unique_ptr<EdgeNodeMonitor> monitor_;            ///< 子系统8: 节点监控

#ifdef HEARTLAKE_USE_ONNX
    std::unique_ptr<OnnxSentimentEngine> onnxEngine_;     ///< 可选 ONNX 情感分析后端
    bool onnxEnabled_ = false;                            ///< ONNX 后端是否可用
#endif
};

} // namespace ai
} // namespace heartlake
