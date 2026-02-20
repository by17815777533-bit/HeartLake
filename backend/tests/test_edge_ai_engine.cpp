/**
 * @file test_edge_ai_engine.cpp
 * @brief EdgeAIEngine 完整单元测试
 * @date 2025-02-20
 *
 * 覆盖八大子系统：
 * 1. 轻量级情感分析
 * 2. AC自动机文本审核
 * 3. 实时情绪脉搏
 * 4. 联邦学习聚合器
 * 5. 差分隐私引擎
 * 6. HNSW向量检索
 * 7. 模型量化推理
 * 8. 边缘节点健康监控
 */

#include <gtest/gtest.h>
#include "infrastructure/ai/EdgeAIEngine.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <set>
#include <chrono>
#include <thread>

using namespace heartlake::ai;

// ============================================================================
// 测试夹具
// ============================================================================

class EdgeAIEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = &EdgeAIEngine::getInstance();
        Json::Value config;
        config["dp_epsilon"] = 1.0;
        config["dp_delta"] = 1e-5;
        config["pulse_window_seconds"] = 300;
        config["hnsw_m"] = 16;
        config["hnsw_ef_construction"] = 200;
        config["hnsw_ef_search"] = 50;
        config["quantization_bits"] = 8;
        engine->initialize(config);
    }

    EdgeAIEngine* engine;
};

// ============================================================================
// 子系统1: 轻量级情感分析测试
// ============================================================================

TEST_F(EdgeAIEngineTest, SentimentPositiveText) {
    auto result = engine->analyzeSentimentLocal("我今天非常开心，一切都很美好！");

    EXPECT_GT(result.score, 0.0f);
    EXPECT_GE(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
    EXPECT_FALSE(result.mood.empty());
    EXPECT_FALSE(result.method.empty());
}

TEST_F(EdgeAIEngineTest, SentimentNegativeText) {
    auto result = engine->analyzeSentimentLocal("我很难过，今天真是糟糕透了，什么都不顺利。");

    EXPECT_LT(result.score, 0.0f);
    EXPECT_GE(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
}

TEST_F(EdgeAIEngineTest, SentimentNeutralText) {
    auto result = engine->analyzeSentimentLocal("今天是星期三。");

    // 中性文本分数应接近0
    EXPECT_GE(result.score, -0.5f);
    EXPECT_LE(result.score, 0.5f);
}

TEST_F(EdgeAIEngineTest, SentimentNegatorFlip) {
    // 否定词应翻转情感极性
    auto positiveResult = engine->analyzeSentimentLocal("我很开心");
    auto negatedResult = engine->analyzeSentimentLocal("我不开心");

    // 否定后分数应低于原始正面分数
    EXPECT_LT(negatedResult.score, positiveResult.score);
}

TEST_F(EdgeAIEngineTest, SentimentIntensifierBoost) {
    // 程度副词应增强情感强度
    auto baseResult = engine->analyzeSentimentLocal("我开心");
    auto intensifiedResult = engine->analyzeSentimentLocal("我非常开心");

    // 带程度副词的分数绝对值应更大或相等
    EXPECT_GE(std::abs(intensifiedResult.score), std::abs(baseResult.score) - 0.1f);
}

TEST_F(EdgeAIEngineTest, SentimentEmptyText) {
    auto result = engine->analyzeSentimentLocal("");

    // 空文本应返回中性结果
    EXPECT_GE(result.score, -1.0f);
    EXPECT_LE(result.score, 1.0f);
    EXPECT_FALSE(result.method.empty());
}

// ============================================================================
// 子系统2: AC自动机文本审核测试
// ============================================================================

TEST_F(EdgeAIEngineTest, ModerationDetectsSensitiveContent) {
    // 包含敏感词的文本应被检测到
    auto result = engine->moderateTextLocal("我要杀了你这个混蛋");

    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.matchedPatterns.empty());
    EXPECT_NE(result.riskLevel, "safe");
    EXPECT_GE(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
}

TEST_F(EdgeAIEngineTest, ModerationPassesSafeText) {
    auto result = engine->moderateTextLocal("今天天气真好，我们一起去公园散步吧。");

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.riskLevel, "safe");
    EXPECT_TRUE(result.matchedPatterns.empty());
}

TEST_F(EdgeAIEngineTest, ModerationMultipleSensitiveWords) {
    // 多个敏感词同时出现
    auto result = engine->moderateTextLocal("去死吧你这个废物，我要杀了你");

    EXPECT_FALSE(result.passed);
    // 应匹配到多个敏感模式
    EXPECT_GE(result.matchedPatterns.size(), 1u);
    EXPECT_NE(result.riskLevel, "safe");
}

TEST_F(EdgeAIEngineTest, ModerationEmptyText) {
    auto result = engine->moderateTextLocal("");

    // 空文本应安全通过
    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.riskLevel, "safe");
}

// ============================================================================
// 子系统3: 实时情绪脉搏测试
// ============================================================================

TEST_F(EdgeAIEngineTest, EmotionPulseAfterMultipleSamples) {
    // 提交多个情绪样本
    engine->submitEmotionSample(0.8f, "joy");
    engine->submitEmotionSample(0.6f, "joy");
    engine->submitEmotionSample(-0.3f, "sadness");
    engine->submitEmotionSample(0.1f, "neutral");
    engine->submitEmotionSample(0.5f, "joy");

    auto pulse = engine->getCurrentPulse();

    EXPECT_EQ(pulse.sampleCount, 5);
    EXPECT_GT(pulse.avgScore, -1.0f);
    EXPECT_LT(pulse.avgScore, 1.0f);
    EXPECT_FALSE(pulse.dominantMood.empty());
}

TEST_F(EdgeAIEngineTest, EmotionPulseEmptyWindow) {
    // 不提交任何样本，获取脉搏应返回默认值
    // 重新初始化以清空窗口
    Json::Value config;
    config["pulse_window_seconds"] = 1;
    engine->initialize(config);

    // 等待窗口过期
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto pulse = engine->getCurrentPulse();

    EXPECT_EQ(pulse.sampleCount, 0);
    EXPECT_FLOAT_EQ(pulse.avgScore, 0.0f);
}

TEST_F(EdgeAIEngineTest, EmotionPulseMoodDistribution) {
    engine->submitEmotionSample(0.8f, "joy");
    engine->submitEmotionSample(0.7f, "joy");
    engine->submitEmotionSample(0.9f, "joy");
    engine->submitEmotionSample(-0.5f, "sadness");
    engine->submitEmotionSample(-0.3f, "anger");

    auto pulse = engine->getCurrentPulse();

    // 情绪分布应包含提交的类型
    EXPECT_GT(pulse.moodDistribution.count("joy"), 0u);
    EXPECT_EQ(pulse.moodDistribution.at("joy"), 3);
    EXPECT_GT(pulse.moodDistribution.count("sadness"), 0u);
    EXPECT_EQ(pulse.moodDistribution.at("sadness"), 1);
    EXPECT_GT(pulse.moodDistribution.count("anger"), 0u);
    EXPECT_EQ(pulse.moodDistribution.at("anger"), 1);

    // 主导情绪应为joy
    EXPECT_EQ(pulse.dominantMood, "joy");
}

// ============================================================================
// 子系统4: 联邦学习聚合器测试
// ============================================================================

TEST_F(EdgeAIEngineTest, FederatedTwoClientAggregation) {
    // 客户端A：100个样本
    FederatedModelParams clientA;
    clientA.modelId = "sentiment_v1";
    clientA.weights = {{1.0f, 2.0f}, {3.0f, 4.0f}};
    clientA.biases = {0.1f, 0.2f};
    clientA.sampleCount = 100;
    clientA.localLoss = 0.5f;
    clientA.epoch = 1;
    clientA.nodeId = "node_a";

    // 客户端B：200个样本
    FederatedModelParams clientB;
    clientB.modelId = "sentiment_v1";
    clientB.weights = {{4.0f, 5.0f}, {6.0f, 7.0f}};
    clientB.biases = {0.3f, 0.4f};
    clientB.sampleCount = 200;
    clientB.localLoss = 0.3f;
    clientB.epoch = 1;
    clientB.nodeId = "node_b";

    engine->submitLocalModel(clientA);
    engine->submitLocalModel(clientB);

    auto global = engine->aggregateFedAvg();

    // FedAvg加权聚合: w_global = (100/300)*wA + (200/300)*wB
    // weights[0][0] = (100/300)*1.0 + (200/300)*4.0 = 0.333 + 2.667 = 3.0
    EXPECT_EQ(global.weights.size(), 2u);
    EXPECT_EQ(global.weights[0].size(), 2u);
    EXPECT_NEAR(global.weights[0][0], 3.0f, 0.1f);
    EXPECT_NEAR(global.weights[0][1], 4.0f, 0.1f);
    EXPECT_NEAR(global.weights[1][0], 5.0f, 0.1f);
    EXPECT_NEAR(global.weights[1][1], 6.0f, 0.1f);

    // biases[0] = (100/300)*0.1 + (200/300)*0.3 = 0.033 + 0.2 = 0.233
    EXPECT_NEAR(global.biases[0], 0.233f, 0.05f);
    EXPECT_NEAR(global.biases[1], 0.333f, 0.05f);

    // 总样本数
    EXPECT_EQ(global.sampleCount, 300u);
}

TEST_F(EdgeAIEngineTest, FederatedGradientClipping) {
    // 提交带有极端权重的模型，验证梯度裁剪
    FederatedModelParams extreme;
    extreme.modelId = "test_model";
    extreme.weights = {{1000.0f, -1000.0f}};
    extreme.biases = {500.0f};
    extreme.sampleCount = 50;
    extreme.localLoss = 10.0f;
    extreme.epoch = 1;
    extreme.nodeId = "node_extreme";

    FederatedModelParams normal;
    normal.modelId = "test_model";
    normal.weights = {{1.0f, 2.0f}};
    normal.biases = {0.1f};
    normal.sampleCount = 50;
    normal.localLoss = 0.5f;
    normal.epoch = 1;
    normal.nodeId = "node_normal";

    engine->submitLocalModel(extreme);
    engine->submitLocalModel(normal);

    auto global = engine->aggregateFedAvg();

    // 聚合后的权重不应为NaN或Inf
    for (const auto& row : global.weights) {
        for (float w : row) {
            EXPECT_FALSE(std::isnan(w));
            EXPECT_FALSE(std::isinf(w));
        }
    }
    for (float b : global.biases) {
        EXPECT_FALSE(std::isnan(b));
        EXPECT_FALSE(std::isinf(b));
    }
}

TEST_F(EdgeAIEngineTest, FederatedEmptyUpdateList) {
    // 无本地模型提交时聚合
    auto global = engine->aggregateFedAvg();

    // 应返回空或默认模型参数
    EXPECT_TRUE(global.weights.empty());
    EXPECT_TRUE(global.biases.empty());
    EXPECT_EQ(global.sampleCount, 0u);
}

// ============================================================================
// 子系统5: 差分隐私引擎测试
// ============================================================================

TEST_F(EdgeAIEngineTest, DPNoiseChangesValue) {
    engine->resetPrivacyBudget();

    float original = 42.0f;
    float sensitivity = 1.0f;

    // 多次添加噪声，至少有一次应与原值不同
    bool changed = false;
    for (int i = 0; i < 20; ++i) {
        float noised = engine->addLaplaceNoise(original, sensitivity);
        if (std::abs(noised - original) > 1e-6f) {
            changed = true;
            break;
        }
    }
    EXPECT_TRUE(changed) << "Laplace噪声应使值发生变化";
}

TEST_F(EdgeAIEngineTest, DPPrivacyBudgetConsumption) {
    engine->resetPrivacyBudget();

    float initialBudget = engine->getRemainingPrivacyBudget();
    EXPECT_GT(initialBudget, 0.0f);

    // 执行多次噪声添加，消耗隐私预算
    for (int i = 0; i < 5; ++i) {
        engine->addLaplaceNoise(1.0f, 1.0f);
    }

    float remainingBudget = engine->getRemainingPrivacyBudget();

    // 剩余预算应小于初始预算
    EXPECT_LT(remainingBudget, initialBudget);
}

TEST_F(EdgeAIEngineTest, DPEpsilonAffectsNoiseScale) {
    // 较小的epsilon应产生更大的噪声（更强隐私保护）
    // 使用向量版本统计噪声幅度

    engine->resetPrivacyBudget();

    // 用大epsilon初始化
    Json::Value configLargeEps;
    configLargeEps["dp_epsilon"] = 10.0;
    configLargeEps["dp_delta"] = 1e-5;
    engine->initialize(configLargeEps);
    engine->resetPrivacyBudget();

    float sumAbsDiffLargeEps = 0.0f;
    int trials = 100;
    for (int i = 0; i < trials; ++i) {
        float noised = engine->addLaplaceNoise(0.0f, 1.0f);
        sumAbsDiffLargeEps += std::abs(noised);
    }
    float avgNoiseLargeEps = sumAbsDiffLargeEps / trials;

    // 用小epsilon初始化
    Json::Value configSmallEps;
    configSmallEps["dp_epsilon"] = 0.1;
    configSmallEps["dp_delta"] = 1e-5;
    engine->initialize(configSmallEps);
    engine->resetPrivacyBudget();

    float sumAbsDiffSmallEps = 0.0f;
    for (int i = 0; i < trials; ++i) {
        float noised = engine->addLaplaceNoise(0.0f, 1.0f);
        sumAbsDiffSmallEps += std::abs(noised);
    }
    float avgNoiseSmallEps = sumAbsDiffSmallEps / trials;

    // 小epsilon应产生更大的平均噪声
    EXPECT_GT(avgNoiseSmallEps, avgNoiseLargeEps);
}

// ============================================================================
// 子系统6: HNSW向量检索测试
// ============================================================================

TEST_F(EdgeAIEngineTest, HNSWInsertAndSearch) {
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {0.0f, 1.0f, 0.0f, 0.0f};
    std::vector<float> vec3 = {0.0f, 0.0f, 1.0f, 0.0f};

    engine->hnswInsert("v1", vec1);
    engine->hnswInsert("v2", vec2);
    engine->hnswInsert("v3", vec3);

    // 搜索与vec1最相似的向量
    std::vector<float> query = {0.9f, 0.1f, 0.0f, 0.0f};
    auto results = engine->hnswSearch(query, 3);

    EXPECT_FALSE(results.empty());
    // 最相似的应该是v1
    EXPECT_EQ(results[0].id, "v1");
}

TEST_F(EdgeAIEngineTest, HNSWTopKCount) {
    // 插入10个向量
    for (int i = 0; i < 10; ++i) {
        std::vector<float> vec(8, 0.0f);
        vec[i % 8] = 1.0f;
        engine->hnswInsert("vec_" + std::to_string(i), vec);
    }

    std::vector<float> query(8, 0.1f);

    // 请求Top-3
    auto results3 = engine->hnswSearch(query, 3);
    EXPECT_EQ(results3.size(), 3u);

    // 请求Top-5
    auto results5 = engine->hnswSearch(query, 5);
    EXPECT_EQ(results5.size(), 5u);

    // 请求超过总数的K
    auto resultsAll = engine->hnswSearch(query, 20);
    EXPECT_LE(resultsAll.size(), 10u);
}

TEST_F(EdgeAIEngineTest, HNSWSimilarVectorOrdering) {
    // 插入向量，验证搜索结果按距离排序
    std::vector<float> base = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> close = {0.9f, 0.1f, 0.0f, 0.0f};   // 与query最近
    std::vector<float> medium = {0.5f, 0.5f, 0.0f, 0.0f};   // 中等距离
    std::vector<float> far = {0.0f, 0.0f, 0.0f, 1.0f};      // 最远

    engine->hnswInsert("close", close);
    engine->hnswInsert("medium", medium);
    engine->hnswInsert("far", far);

    auto results = engine->hnswSearch(base, 3);

    EXPECT_GE(results.size(), 3u);

    // 结果应按距离升序排列（距离越小越相似）
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_LE(results[i - 1].distance, results[i].distance);
    }

    // 最近的应该是close
    EXPECT_EQ(results[0].id, "close");
}

TEST_F(EdgeAIEngineTest, HNSWEmptyIndexSearch) {
    // 重新初始化以获得空索引
    Json::Value config;
    config["hnsw_m"] = 16;
    engine->initialize(config);

    std::vector<float> query = {1.0f, 0.0f, 0.0f};
    auto results = engine->hnswSearch(query, 5);

    // 空索引搜索应返回空结果
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// 子系统7: 模型量化推理测试
// ============================================================================

TEST_F(EdgeAIEngineTest, QuantizeFloat32ToInt8) {
    std::vector<float> tensor = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    std::vector<size_t> shape = {1, 5};

    auto quantized = engine->quantizeToInt8(tensor, shape);

    // 量化后数据大小应与原始一致
    EXPECT_EQ(quantized.data.size(), tensor.size());
    EXPECT_EQ(quantized.shape, shape);

    // scale应为正数
    EXPECT_GT(quantized.scale, 0.0f);

    // 反量化后应接近原始值
    auto dequantized = quantized.dequantize();
    EXPECT_EQ(dequantized.size(), tensor.size());
    for (size_t i = 0; i < tensor.size(); ++i) {
        EXPECT_NEAR(dequantized[i], tensor[i], 0.05f)
            << "反量化误差过大 at index " << i;
    }
}

TEST_F(EdgeAIEngineTest, QuantizedMatMul) {
    // 矩阵A: 2x3
    std::vector<float> matA = {1.0f, 2.0f, 3.0f,
                                4.0f, 5.0f, 6.0f};
    // 矩阵B: 3x2
    std::vector<float> matB = {1.0f, 4.0f,
                                2.0f, 5.0f,
                                3.0f, 6.0f};

    auto qA = engine->quantizeToInt8(matA, {2, 3});
    auto qB = engine->quantizeToInt8(matB, {3, 2});

    // 量化矩阵乘法: C = A * B
    // 期望结果: [[14, 32], [32, 77]]
    auto result = engine->quantizedMatMul(qA, qB, 2, 3, 2);

    EXPECT_EQ(result.size(), 4u); // 2x2 = 4个元素

    // 量化乘法有精度损失，允许较大误差
    EXPECT_NEAR(result[0], 14.0f, 2.0f);  // C[0][0]
    EXPECT_NEAR(result[1], 32.0f, 4.0f);  // C[0][1]
    EXPECT_NEAR(result[2], 32.0f, 4.0f);  // C[1][0]
    EXPECT_NEAR(result[3], 77.0f, 8.0f);  // C[1][1]
}

TEST_F(EdgeAIEngineTest, QuantizedForwardPass) {
    // 输入向量: 3维
    std::vector<float> input = {1.0f, 2.0f, 3.0f};

    // 权重矩阵: 2x3 (2个输出神经元，3个输入)
    std::vector<float> weightData = {0.5f, 0.3f, 0.1f,
                                      0.2f, 0.4f, 0.6f};
    auto quantizedWeights = engine->quantizeToInt8(weightData, {2, 3});

    // 偏置: 2维
    std::vector<float> biases = {0.1f, 0.2f};

    auto output = engine->quantizedForward(input, quantizedWeights, biases);

    EXPECT_EQ(output.size(), 2u);

    // 期望: output[0] = 0.5*1 + 0.3*2 + 0.1*3 + 0.1 = 1.5
    // 期望: output[1] = 0.2*1 + 0.4*2 + 0.6*3 + 0.2 = 3.0
    // 量化有精度损失
    EXPECT_NEAR(output[0], 1.5f, 0.5f);
    EXPECT_NEAR(output[1], 3.0f, 0.5f);

    // 输出不应为NaN
    for (float v : output) {
        EXPECT_FALSE(std::isnan(v));
        EXPECT_FALSE(std::isinf(v));
    }
}

// ============================================================================
// 子系统8: 边缘节点健康监控测试
// ============================================================================

TEST_F(EdgeAIEngineTest, NodeRegistration) {
    engine->registerNode("edge_node_1");
    engine->registerNode("edge_node_2");

    auto allNodes = engine->getAllNodeStatus();

    // 应至少包含注册的节点
    std::set<std::string> nodeIds;
    for (const auto& node : allNodes) {
        nodeIds.insert(node.nodeId);
    }
    EXPECT_TRUE(nodeIds.count("edge_node_1") > 0);
    EXPECT_TRUE(nodeIds.count("edge_node_2") > 0);
}

TEST_F(EdgeAIEngineTest, NodeHeartbeatUpdate) {
    engine->registerNode("heartbeat_node");

    EdgeNodeStatus status;
    status.nodeId = "heartbeat_node";
    status.cpuUsage = 0.3f;
    status.memoryUsage = 0.5f;
    status.latencyMs = 10.0f;
    status.activeConnections = 5;
    status.totalRequests = 100;
    status.failedRequests = 2;
    status.isHealthy = true;
    status.lastHeartbeat = std::chrono::steady_clock::now();
    status.healthScore = 0.9f;

    engine->updateNodeStatus(status);

    auto allNodes = engine->getAllNodeStatus();
    bool found = false;
    for (const auto& node : allNodes) {
        if (node.nodeId == "heartbeat_node") {
            found = true;
            EXPECT_FLOAT_EQ(node.cpuUsage, 0.3f);
            EXPECT_FLOAT_EQ(node.memoryUsage, 0.5f);
            EXPECT_FLOAT_EQ(node.latencyMs, 10.0f);
            EXPECT_EQ(node.activeConnections, 5);
            EXPECT_EQ(node.totalRequests, 100);
            EXPECT_EQ(node.failedRequests, 2);
            EXPECT_TRUE(node.isHealthy);
            break;
        }
    }
    EXPECT_TRUE(found) << "未找到已更新心跳的节点";
}

TEST_F(EdgeAIEngineTest, SelectBestNode) {
    engine->registerNode("good_node");
    engine->registerNode("bad_node");

    // good_node: 低负载、低延迟
    EdgeNodeStatus goodStatus;
    goodStatus.nodeId = "good_node";
    goodStatus.cpuUsage = 0.1f;
    goodStatus.memoryUsage = 0.2f;
    goodStatus.latencyMs = 5.0f;
    goodStatus.activeConnections = 2;
    goodStatus.totalRequests = 1000;
    goodStatus.failedRequests = 1;
    goodStatus.isHealthy = true;
    goodStatus.lastHeartbeat = std::chrono::steady_clock::now();
    goodStatus.healthScore = 0.95f;
    engine->updateNodeStatus(goodStatus);

    // bad_node: 高负载、高延迟
    EdgeNodeStatus badStatus;
    badStatus.nodeId = "bad_node";
    badStatus.cpuUsage = 0.9f;
    badStatus.memoryUsage = 0.85f;
    badStatus.latencyMs = 200.0f;
    badStatus.activeConnections = 50;
    badStatus.totalRequests = 1000;
    badStatus.failedRequests = 100;
    badStatus.isHealthy = true;
    badStatus.lastHeartbeat = std::chrono::steady_clock::now();
    badStatus.healthScore = 0.3f;
    engine->updateNodeStatus(badStatus);

    auto bestNode = engine->selectBestNode();

    EXPECT_TRUE(bestNode.has_value());
    EXPECT_EQ(bestNode.value(), "good_node");
}
