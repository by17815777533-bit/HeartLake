/**
 * @file EdgeAIEngine.h
 * @brief 边缘AI推理引擎 - 端侧智能计算核心
 *
 * 创新点：将AI推理能力下沉到边缘节点，实现低延迟、高隐私的
 * 本地化智能服务。融合联邦学习、差分隐私、HNSW向量检索等
 * 前沿技术，构建完整的边缘AI计算框架。
 *
 * 八大核心子系统：
 * 1. 轻量级情感分析（规则+词典+统计模型，无需外部API）
 * 2. 本地文本审核（AC自动机多模式匹配+语义规则）
 * 3. 实时情绪脉搏检测（滑动窗口统计，社区情绪热力图）
 * 4. 联邦学习聚合器（FedAvg加权聚合模拟）
 * 5. 差分隐私噪声注入（Laplace机制+隐私预算追踪）
 * 6. 本地向量相似度检索（HNSW近似最近邻）
 * 7. 模型量化推理模拟（INT8量化/反量化）
 * 8. 边缘节点健康监控与自适应负载均衡
 *
 * Created by 王璐瑶
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <random>
#include <queue>
#include <functional>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <deque>
#include <optional>
#include <json/json.h>
#include <drogon/drogon.h>

#include "utils/HighPerformance.h"

namespace heartlake {
namespace ai {

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 轻量级情感分析结果
 */
struct EdgeSentimentResult {
    float score;              ///< 情感分数 [-1.0, 1.0]
    std::string mood;         ///< 情绪类型: joy, sadness, anger, fear, surprise, neutral
    float confidence;         ///< 置信度 [0.0, 1.0]
    std::string method;       ///< 分析方法: "rule", "lexicon", "statistical", "ensemble"

    Json::Value toJson() const {
        Json::Value j;
        j["score"] = score;
        j["mood"] = mood;
        j["confidence"] = confidence;
        j["method"] = method;
        return j;
    }
};

/**
 * @brief 本地审核结果
 */
/**
 * @brief 五因子心理风险评估详情
 */
struct FiveFactorRiskDetail {
    float selfHarmIntent;       ///< 自残意图 (weight 0.9)
    float hopelessness;         ///< 绝望表达 (weight 0.6)
    float socialIsolation;      ///< 社交孤立 (weight 0.1)
    float temporalUrgency;      ///< 时间紧迫性 (weight 0.5)
    float linguisticMarkers;    ///< 语言风险标记 (weight 0.3)
    float compositeScore;       ///< 五因子加权综合分数

    Json::Value toJson() const {
        Json::Value j;
        j["self_harm_intent"] = selfHarmIntent;
        j["hopelessness"] = hopelessness;
        j["social_isolation"] = socialIsolation;
        j["temporal_urgency"] = temporalUrgency;
        j["linguistic_markers"] = linguisticMarkers;
        j["composite_score"] = compositeScore;
        return j;
    }
};

struct EdgeModerationResult {
    bool passed;                              ///< 是否通过审核
    std::string riskLevel;                    ///< 风险等级: safe, low_risk, medium_risk, high_risk
    std::vector<std::string> matchedPatterns; ///< 匹配到的敏感模式
    std::vector<std::string> categories;      ///< 风险类别
    float confidence;                         ///< 置信度
    bool needsAlert;                          ///< 是否需要紧急关注（自伤倾向等）
    std::string suggestion;                   ///< 处理建议
    std::optional<FiveFactorRiskDetail> fiveFactorDetail; ///< 五因子心理风险详情

    Json::Value toJson() const {
        Json::Value j;
        j["passed"] = passed;
        j["risk_level"] = riskLevel;
        j["confidence"] = confidence;
        j["needs_alert"] = needsAlert;
        j["suggestion"] = suggestion;
        Json::Value pats(Json::arrayValue);
        for (const auto& p : matchedPatterns) pats.append(p);
        j["matched_patterns"] = pats;
        Json::Value cats(Json::arrayValue);
        for (const auto& c : categories) cats.append(c);
        j["categories"] = cats;
        if (fiveFactorDetail.has_value()) {
            j["five_factor_detail"] = fiveFactorDetail->toJson();
        }
        return j;
    }
};

/**
 * @brief 情绪脉搏快照
 */
struct EmotionPulse {
    std::chrono::steady_clock::time_point timestamp;
    float avgScore;                                    ///< 窗口内平均情感分数
    float stddev;                                      ///< 情感波动标准差
    std::unordered_map<std::string, int> moodDistribution; ///< 情绪类型分布
    int sampleCount;                                   ///< 样本数量
    float trendSlope;                                  ///< 情绪趋势斜率（正=好转，负=恶化）
    std::string dominantMood;                          ///< 主导情绪

    Json::Value toJson() const {
        Json::Value j;
        j["avg_score"] = avgScore;
        j["stddev"] = stddev;
        j["sample_count"] = sampleCount;
        j["trend_slope"] = trendSlope;
        j["dominant_mood"] = dominantMood;
        Json::Value dist;
        for (const auto& [mood, count] : moodDistribution) {
            dist[mood] = count;
        }
        j["mood_distribution"] = dist;
        return j;
    }
};

/**
 * @brief 联邦学习本地模型参数
 */
struct FederatedModelParams {
    std::string modelId;                    ///< 模型标识
    std::vector<std::vector<float>> weights; ///< 模型权重矩阵
    std::vector<float> biases;              ///< 偏置向量
    size_t sampleCount;                     ///< 本地训练样本数
    float localLoss;                        ///< 本地损失值
    int epoch;                              ///< 训练轮次
    std::string nodeId;                     ///< 节点标识
};

/**
 * @brief 差分隐私配置
 */
struct DPConfig {
    float epsilon;          ///< 隐私预算 ε（越小越隐私）
    float delta;            ///< 松弛参数 δ
    float sensitivity;      ///< 查询敏感度 Δf
    float maxEpsilonBudget; ///< 最大累计隐私预算
};

/**
 * @brief HNSW图中的节点
 */
struct HNSWNode {
    std::string id;                                    ///< 节点唯一标识
    std::vector<float> vector;                         ///< 向量数据
    std::vector<std::vector<size_t>> neighbors;        ///< 各层邻居列表
    int maxLevel;                                      ///< 节点最高层级
};

/**
 * @brief 向量检索结果
 */
struct VectorSearchResult {
    std::string id;       ///< 节点标识
    float distance;       ///< 距离（越小越相似）
    float similarity;     ///< 相似度 [0, 1]
};

/**
 * @brief INT8量化后的张量
 */
struct QuantizedTensor {
    std::vector<int8_t> data;   ///< 量化后的INT8数据
    float scale;                ///< 量化缩放因子
    int8_t zeroPoint;           ///< 零点偏移
    std::vector<size_t> shape;  ///< 张量形状

    /**
     * @brief 反量化为float32
     * @return float32向量
     */
    std::vector<float> dequantize() const {
        std::vector<float> result(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = scale * (static_cast<float>(data[i]) - static_cast<float>(zeroPoint));
        }
        return result;
    }
};

/**
 * @brief 边缘节点状态
 */
/**
 * @brief 熔断器状态枚举 (经典三状态模式)
 *
 * CLOSED  -> 正常状态，请求正常通过，失败率超阈值时转 OPEN
 * OPEN    -> 熔断状态，拒绝请求，冷却时间后转 HALF_OPEN
 * HALF_OPEN -> 探测状态，允许少量探测请求，成功转 CLOSED，失败转回 OPEN
 */
enum class CircuitState {
    CLOSED,     ///< 正常 - 请求正常通过
    OPEN,       ///< 熔断 - 拒绝所有请求
    HALF_OPEN   ///< 半开 - 允许探测请求
};

struct EdgeNodeStatus {
    std::string nodeId;                                ///< 节点标识
    float cpuUsage;                                    ///< CPU使用率 [0, 1]
    float memoryUsage;                                 ///< 内存使用率 [0, 1]
    float latencyMs;                                   ///< 平均延迟(ms)
    int activeConnections;                             ///< 活跃连接数
    int totalRequests;                                 ///< 总请求数
    int failedRequests;                                ///< 失败请求数
    bool isHealthy;                                    ///< 是否健康
    std::chrono::steady_clock::time_point lastHeartbeat; ///< 最后心跳时间
    float healthScore;                                 ///< 综合健康分 [0, 1]

    // 熔断器状态字段
    CircuitState circuitState = CircuitState::CLOSED;  ///< 熔断器当前状态
    std::chrono::steady_clock::time_point circuitOpenedAt; ///< 进入OPEN状态的时间
    int consecutiveFailures = 0;                       ///< 连续失败计数
    int halfOpenSuccesses = 0;                         ///< HALF_OPEN状态下连续成功数

    // 熔断器配置常量
    static constexpr float FAILURE_RATE_THRESHOLD = 0.5f;    ///< 失败率阈值
    static constexpr int MIN_REQUESTS_FOR_CIRCUIT = 5;       ///< 触发熔断的最小请求数
    static constexpr int COOLDOWN_SECONDS = 30;              ///< OPEN->HALF_OPEN冷却时间(秒)
    static constexpr int HALF_OPEN_SUCCESS_THRESHOLD = 3;    ///< HALF_OPEN转CLOSED所需连续成功数

    Json::Value toJson() const {
        Json::Value j;
        j["node_id"] = nodeId;
        j["cpu_usage"] = cpuUsage;
        j["memory_usage"] = memoryUsage;
        j["latency_ms"] = latencyMs;
        j["active_connections"] = activeConnections;
        j["total_requests"] = totalRequests;
        j["failed_requests"] = failedRequests;
        j["is_healthy"] = isHealthy;
        j["health_score"] = healthScore;
        j["circuit_state"] = circuitState == CircuitState::CLOSED ? "CLOSED" :
                             circuitState == CircuitState::OPEN   ? "OPEN" : "HALF_OPEN";
        j["consecutive_failures"] = consecutiveFailures;
        return j;
    }
};

// ============================================================================
// EdgeAIEngine 主类
// ============================================================================

/**
 * @brief 边缘AI推理引擎
 *
 * 单例模式，线程安全。通过环境变量 EDGE_AI_ENABLED 控制启用。
 * 提供完整的端侧AI推理能力，作为云端AIService的本地降级方案。
 *
 * @note 所有公共方法均为线程安全
 */
class EdgeAIEngine {
public:
    static EdgeAIEngine& getInstance();

    /**
     * @brief 初始化边缘AI引擎
     * @param config JSON配置（可选，缺省使用默认参数）
     *
     * 配置项：
     * - hnsw_m: HNSW每层最大邻居数（默认16）
     * - hnsw_ef_construction: 构建时搜索宽度（默认200）
     * - hnsw_ef_search: 查询时搜索宽度（默认50）
     * - dp_epsilon: 差分隐私预算（默认1.0）
     * - dp_delta: 松弛参数（默认1e-5）
     * - pulse_window_seconds: 情绪脉搏窗口（默认300秒）
     * - quantization_bits: 量化位数（默认8）
     */
    void initialize(const Json::Value& config = Json::Value::null);

    /**
     * @brief 检查引擎是否已启用
     * @return true 如果 EDGE_AI_ENABLED=1 且已初始化
     */
    bool isEnabled() const;

    // ========================================================================
    // 子系统1: 轻量级情感分析
    // ========================================================================

    /**
     * @brief 本地情感分析（无需外部API）
     *
     * 三层融合策略：
     * 1. 规则层：基于标点、表情符号、句式模式
     * 2. 词典层：基于情感词典的加权评分
     * 3. 统计层：基于文本统计特征的简单分类
     * 最终通过加权集成得到综合结果
     *
     * @param text 待分析文本
     * @return 情感分析结果
     */
    EdgeSentimentResult analyzeSentimentLocal(const std::string& text);

    // ========================================================================
    // 子系统2: 本地文本审核
    // ========================================================================

    /**
     * @brief 本地文本审核（AC自动机 + 语义规则）
     *
     * 两阶段审核：
     * 1. AC自动机快速多模式匹配（O(n)复杂度）
     * 2. 语义规则引擎二次判定（上下文分析）
     *
     * @param text 待审核文本
     * @return 审核结果
     */
    EdgeModerationResult moderateTextLocal(const std::string& text);

    // ========================================================================
    // 子系统3: 实时情绪脉搏
    // ========================================================================

    /**
     * @brief 提交情绪样本到脉搏检测器
     * @param score 情感分数
     * @param mood 情绪类型
     */
    void submitEmotionSample(float score, const std::string& mood);

    /**
     * @brief 获取当前情绪脉搏快照
     * @return 当前窗口的情绪统计
     */
    EmotionPulse getCurrentPulse();

    /**
     * @brief 获取情绪脉搏历史（最近N个快照）
     * @param count 快照数量
     * @return 历史脉搏列表
     */
    std::vector<EmotionPulse> getPulseHistory(int count = 10);

    // ========================================================================
    // 子系统4: 联邦学习聚合器
    // ========================================================================

    /**
     * @brief 提交本地模型参数用于联邦聚合
     * @param params 本地模型参数
     */
    void submitLocalModel(const FederatedModelParams& params);

    /**
     * @brief 执行FedAvg加权聚合
     *
     * 聚合公式：w_global = Σ(n_k / n) * w_k
     * 其中 n_k 为第k个节点的样本数，n为总样本数
     *
     * @param clippingBound 梯度裁剪阈值C（0表示不裁剪）
     * @param noiseSigma 高斯噪声标准差σ（0表示不加噪）
     * @return 聚合后的全局模型参数
     */
    FederatedModelParams aggregateFedAvg(float clippingBound = 0.0f, float noiseSigma = 0.0f);

    /**
     * @brief 获取当前聚合轮次信息
     * @return JSON格式的聚合状态
     */
    Json::Value getFederatedStatus() const;

    // ========================================================================
    // 子系统5: 差分隐私引擎
    // ========================================================================

    /**
     * @brief 对浮点值添加Laplace噪声
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
     * @brief 对向量添加Laplace噪声
     * @param values 原始向量
     * @param sensitivity 每维敏感度
     * @return 添加噪声后的向量
     */
    std::vector<float> addLaplaceNoiseVec(const std::vector<float>& values, float sensitivity);

    /**
     * @brief 获取剩余隐私预算
     * @return 剩余 ε 值
     */
    float getRemainingPrivacyBudget() const;

    /**
     * @brief 重置隐私预算（新一轮计算）
     */
    void resetPrivacyBudget();

    // ========================================================================
    // 子系统6: HNSW向量检索
    // ========================================================================

    /**
     * @brief 插入向量到HNSW索引
     * @param id 向量标识
     * @param vec 向量数据
     */
    void hnswInsert(const std::string& id, const std::vector<float>& vec);

    /**
     * @brief HNSW近似最近邻搜索
     * @param query 查询向量
     * @param k 返回结果数
     * @return Top-K最相似结果
     */
    std::vector<VectorSearchResult> hnswSearch(const std::vector<float>& query, int k = 10);

    /**
     * @brief 获取HNSW索引统计
     * @return 索引信息JSON
     */
    Json::Value getHNSWStats() const;

    /**
     * @brief 获取HNSW索引中向量的维度
     * @return 向量维度，索引为空时返回0
     */
    size_t getHNSWVectorDimension() const;

    // ========================================================================
    // 子系统7: 模型量化推理
    // ========================================================================

    /**
     * @brief 将float32张量量化为INT8
     *
     * 对称量化：scale = max(|x|) / 127
     * q = round(x / scale)
     *
     * @param tensor float32数据
     * @param shape 张量形状
     * @return 量化后的INT8张量
     */
    QuantizedTensor quantizeToInt8(const std::vector<float>& tensor,
                                   const std::vector<size_t>& shape);

    /**
     * @brief INT8量化矩阵乘法
     *
     * 在INT8域执行乘法，结果反量化回float32
     * 模拟边缘设备上的低精度推理加速
     *
     * @param a 量化矩阵A (M x K)
     * @param b 量化矩阵B (K x N)
     * @param M 矩阵A行数
     * @param K 内部维度
     * @param N 矩阵B列数
     * @return float32结果矩阵 (M x N)
     */
    std::vector<float> quantizedMatMul(const QuantizedTensor& a,
                                        const QuantizedTensor& b,
                                        size_t M, size_t K, size_t N);

    /**
     * @brief 量化推理前向传播（单层全连接）
     * @param input 输入向量
     * @param weights 权重矩阵（已量化）
     * @param biases 偏置向量
     * @return 输出向量
     */
    std::vector<float> quantizedForward(const std::vector<float>& input,
                                         const QuantizedTensor& weights,
                                         const std::vector<float>& biases);

    // ========================================================================
    // 子系统8: 边缘节点健康监控
    // ========================================================================

    /**
     * @brief 注册边缘节点
     * @param nodeId 节点标识
     */
    void registerNode(const std::string& nodeId);

    /**
     * @brief 更新节点状态（心跳）
     * @param status 节点状态
     */
    void updateNodeStatus(const EdgeNodeStatus& status);

    /**
     * @brief 选择最优节点（自适应负载均衡）
     *
     * 综合评分公式：
     * score = w1*(1-cpu) + w2*(1-mem) + w3*(1-latency/maxLatency) + w4*(1-failRate)
     *
     * @return 最优节点ID，如果无可用节点返回空
     */
    std::optional<std::string> selectBestNode();

    /**
     * @brief 获取所有节点状态
     * @return 节点状态列表
     */
    std::vector<EdgeNodeStatus> getAllNodeStatus() const;

    /**
     * @brief 获取引擎综合统计信息
     * @return JSON格式统计
     */
    Json::Value getEngineStats() const;

private:
    EdgeAIEngine() = default;
    ~EdgeAIEngine() = default;
    EdgeAIEngine(const EdgeAIEngine&) = delete;
    EdgeAIEngine& operator=(const EdgeAIEngine&) = delete;

    bool initialized_ = false;
    bool enabled_ = false;

    // ---- 情感分析内部 ----
    std::unordered_map<std::string, float> sentimentLexicon_;   ///< 情感词典
    std::unordered_map<std::string, float> intensifiers_;       ///< 程度副词
    std::unordered_map<std::string, float> negators_;           ///< 否定词
    void loadEdgeSentimentLexicon();
    std::vector<std::string> tokenizeUTF8(const std::string& text) const;
    float ruleSentiment(const std::string& text) const;
    float lexiconSentiment(const std::vector<std::string>& tokens) const;
    float statisticalSentiment(const std::vector<std::string>& tokens,
                               const std::string& text) const;
    std::string scoresToMood(float score) const;

    // ---- 文本审核内部 ----
    perf::ACAutomaton moderationAC_;                            ///< AC自动机
    bool moderationACBuilt_ = false;
    std::mutex moderationMutex_;
    void buildModerationAC();
    float semanticRiskAnalysis(const std::string& text,
                               const std::vector<perf::ACAutomaton::Match>& matches) const;

    // ---- 情绪脉搏内部 ----
    struct EmotionSample {
        float score;
        std::string mood;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::deque<EmotionSample> emotionWindow_;                   ///< 滑动窗口
    std::vector<EmotionPulse> pulseHistory_;                    ///< 脉搏历史
    mutable std::shared_mutex pulseMutex_;
    int pulseWindowSeconds_ = 300;                              ///< 窗口大小（秒）
    int maxPulseHistory_ = 100;                                 ///< 最大历史数
    void pruneEmotionWindow();
    EmotionPulse computePulseFromWindow() const;

    // ---- 联邦学习内部 ----
    std::vector<FederatedModelParams> localModels_;             ///< 待聚合的本地模型
    int federatedRound_ = 0;                                    ///< 当前聚合轮次
    mutable std::mutex federatedMutex_;

    // ---- 差分隐私内部 ----
    DPConfig dpConfig_;
    std::atomic<float> consumedEpsilon_{0.0f};                  ///< 已消耗隐私预算
    mutable std::mutex dpMutex_;
    std::mt19937 dpRng_;                                        ///< 随机数生成器
    float sampleLaplace(float scale);

    // ---- HNSW内部 ----
    std::vector<HNSWNode> hnswNodes_;                           ///< 所有节点
    std::unordered_map<std::string, size_t> hnswIdMap_;         ///< ID到索引映射
    int hnswM_ = 16;                                            ///< 每层最大邻居数
    int hnswMMax0_ = 32;                                        ///< 第0层最大邻居数
    int hnswEfConstruction_ = 200;                              ///< 构建搜索宽度
    int hnswEfSearch_ = 50;                                     ///< 查询搜索宽度
    int hnswMaxLevel_ = 0;                                      ///< 当前最高层级
    size_t hnswEntryPoint_ = 0;                                 ///< 入口节点索引
    float hnswLevelMult_ = 0.0f;                                ///< 层级生成因子
    mutable std::shared_mutex hnswMutex_;
    std::mt19937 hnswRng_;

    int randomLevel();
    float vectorDistance(const std::vector<float>& a, const std::vector<float>& b) const;
    std::vector<size_t> searchLayer(const std::vector<float>& query,
                                     size_t entryPoint, int ef, int level) const;
    void connectNeighbors(size_t nodeIdx, const std::vector<size_t>& neighbors,
                          int level, int maxM);

    // ---- 量化推理内部 ----
    // （无额外状态，纯函数式）

    // ---- 节点监控内部 ----
    std::unordered_map<std::string, EdgeNodeStatus> nodeRegistry_; ///< 节点注册表
    mutable std::shared_mutex nodeMutex_;
    float computeHealthScore(const EdgeNodeStatus& status) const;

    // ---- 统计信息 ----
    std::atomic<size_t> totalSentimentCalls_{0};
    std::atomic<size_t> totalModerationCalls_{0};
    std::atomic<size_t> totalHNSWSearches_{0};
    std::atomic<size_t> totalQuantizedOps_{0};
};

} // namespace ai
} // namespace heartlake
