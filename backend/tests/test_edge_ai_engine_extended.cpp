/**
 * EdgeAIEngine 扩展单元测试
 */

#include <gtest/gtest.h>
#include "infrastructure/ai/EdgeAIEngine.h"
#include <vector>
#include <string>
#include <cmath>
#include <set>
#include <thread>
#include <mutex>
#include <numeric>

using namespace heartlake::ai;

class EdgeAIEngineExtTest : public ::testing::Test {
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

// =====================================================================
// 情感分析：边界与特殊输入
// =====================================================================

TEST_F(EdgeAIEngineExtTest, Sentiment_EmptyInput) {
    auto r = engine->analyzeSentimentLocal("");
    EXPECT_GE(r.confidence, 0.0f);
    EXPECT_LE(r.confidence, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_SingleChar) {
    auto r = engine->analyzeSentimentLocal("好");
    EXPECT_GE(r.score, -1.0f);
    EXPECT_LE(r.score, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_PureEmoji) {
    auto r = engine->analyzeSentimentLocal("😊😊😊");
    EXPECT_GE(r.score, -1.0f);
    EXPECT_LE(r.score, 1.0f);
    EXPECT_FALSE(r.method.empty());
}

TEST_F(EdgeAIEngineExtTest, Sentiment_PurePunctuation) {
    auto r = engine->analyzeSentimentLocal("！！！？？？...");
    EXPECT_GE(r.confidence, 0.0f);
    EXPECT_LE(r.confidence, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_LongText) {
    std::string longText;
    for (int i = 0; i < 500; ++i) longText += "今天心情很好，阳光明媚。";
    auto r = engine->analyzeSentimentLocal(longText);
    EXPECT_GT(r.score, 0.0f);
    EXPECT_FALSE(r.mood.empty());
}

TEST_F(EdgeAIEngineExtTest, Sentiment_ChineseEnglishMixed) {
    auto r = engine->analyzeSentimentLocal("I feel very happy今天真开心");
    EXPECT_GE(r.score, -1.0f);
    EXPECT_LE(r.score, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_SqlInjectionText) {
    auto r = engine->analyzeSentimentLocal("DROP TABLE users; SELECT * FROM passwords;--");
    EXPECT_GE(r.confidence, 0.0f);
    EXPECT_LE(r.confidence, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_XssText) {
    auto r = engine->analyzeSentimentLocal("<script>alert('xss')</script>");
    EXPECT_GE(r.confidence, 0.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_NumbersOnly) {
    auto r = engine->analyzeSentimentLocal("1234567890");
    EXPECT_GE(r.score, -1.0f);
    EXPECT_LE(r.score, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_WhitespaceOnly) {
    auto r = engine->analyzeSentimentLocal("   \t\n  ");
    EXPECT_GE(r.confidence, 0.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_RepeatedChars) {
    auto r = engine->analyzeSentimentLocal("哈哈哈哈哈哈哈哈哈哈");
    EXPECT_GE(r.score, -1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_StrongPositive) {
    auto r = engine->analyzeSentimentLocal("太棒了！非常开心！幸福极了！美好的一天！");
    EXPECT_GT(r.score, 0.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_StrongNegative) {
    auto r = engine->analyzeSentimentLocal("太糟糕了，非常痛苦，悲伤难过，绝望透顶");
    EXPECT_LT(r.score, 0.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_Anger) {
    auto r = engine->analyzeSentimentLocal("气死我了！太愤怒了！怒不可遏！");
    EXPECT_LT(r.score, 0.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_Fear) {
    auto r = engine->analyzeSentimentLocal("好害怕，太恐怖了，我很恐惧");
    EXPECT_LT(r.score, 0.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_Surprise) {
    auto r = engine->analyzeSentimentLocal("天哪！太惊讶了！不敢相信！");
    EXPECT_FALSE(r.mood.empty());
}

TEST_F(EdgeAIEngineExtTest, Sentiment_MethodNotEmpty) {
    auto r = engine->analyzeSentimentLocal("测试方法字段");
    EXPECT_FALSE(r.method.empty());
}

TEST_F(EdgeAIEngineExtTest, Sentiment_MoodNotEmpty) {
    auto r = engine->analyzeSentimentLocal("今天还行吧");
    EXPECT_FALSE(r.mood.empty());
}

TEST_F(EdgeAIEngineExtTest, Sentiment_ConfidenceRange) {
    auto r = engine->analyzeSentimentLocal("随便说点什么");
    EXPECT_GE(r.confidence, 0.0f);
    EXPECT_LE(r.confidence, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_ScoreRange) {
    auto r = engine->analyzeSentimentLocal("测试分数范围");
    EXPECT_GE(r.score, -1.0f);
    EXPECT_LE(r.score, 1.0f);
}

TEST_F(EdgeAIEngineExtTest, Sentiment_ToJson) {
    auto r = engine->analyzeSentimentLocal("开心");
    auto j = r.toJson();
    EXPECT_TRUE(j.isMember("score"));
    EXPECT_TRUE(j.isMember("mood"));
    EXPECT_TRUE(j.isMember("confidence"));
    EXPECT_TRUE(j.isMember("method"));
}

// =====================================================================
// 内容审核：边界与绕过尝试
// =====================================================================

TEST_F(EdgeAIEngineExtTest, Moderation_NormalText_Passed) {
    auto r = engine->moderateTextLocal("今天天气真好，出去散步吧");
    EXPECT_TRUE(r.passed);
}

TEST_F(EdgeAIEngineExtTest, Moderation_EmptyText) {
    auto r = engine->moderateTextLocal("");
    EXPECT_TRUE(r.passed);
}

TEST_F(EdgeAIEngineExtTest, Moderation_WhitespaceOnly) {
    auto r = engine->moderateTextLocal("   \t\n   ");
    EXPECT_TRUE(r.passed);
}

TEST_F(EdgeAIEngineExtTest, Moderation_LongSafeText) {
    std::string text;
    for (int i = 0; i < 200; ++i) text += "这是安全的内容。";
    auto r = engine->moderateTextLocal(text);
    EXPECT_TRUE(r.passed);
}

TEST_F(EdgeAIEngineExtTest, Moderation_PureNumbers) {
    auto r = engine->moderateTextLocal("1234567890");
    EXPECT_TRUE(r.passed);
}

TEST_F(EdgeAIEngineExtTest, Moderation_PureEmoji) {
    auto r = engine->moderateTextLocal("😊🌸🌈🎉");
    EXPECT_TRUE(r.passed);
}

TEST_F(EdgeAIEngineExtTest, Moderation_SpecialChars) {
    auto r = engine->moderateTextLocal("!@#$%^&*()_+-=[]{}|;':\",./<>?");
    EXPECT_TRUE(r.passed);
}

TEST_F(EdgeAIEngineExtTest, Moderation_ResultHasFields) {
    auto r = engine->moderateTextLocal("测试内容");
    EXPECT_GE(r.confidence, 0.0f);
}

// =====================================================================
// HNSW 向量检索
// =====================================================================

TEST_F(EdgeAIEngineExtTest, HNSW_InsertAndSearch) {
    std::vector<float> vec(128, 0.0f);
    vec[0] = 1.0f;
    engine->hnswInsert("hnsw_ext_test_1", vec);

    auto results = engine->hnswSearch(vec, 1);
    EXPECT_GE(results.size(), 0u);
}

TEST_F(EdgeAIEngineExtTest, HNSW_ZeroVector) {
    std::vector<float> zeroVec(128, 0.0f);
    EXPECT_NO_THROW(engine->hnswInsert("hnsw_ext_zero", zeroVec));
}

TEST_F(EdgeAIEngineExtTest, HNSW_MultipleInserts) {
    for (int i = 0; i < 10; ++i) {
        std::vector<float> vec(128, 0.0f);
        vec[i % 128] = 1.0f;
        engine->hnswInsert("hnsw_ext_multi_" + std::to_string(i), vec);
    }
    std::vector<float> query(128, 0.0f);
    query[0] = 1.0f;
    auto results = engine->hnswSearch(query, 5);
    EXPECT_LE(results.size(), 5u);
}

TEST_F(EdgeAIEngineExtTest, HNSW_SearchTopK_LargerThanIndex) {
    std::vector<float> vec(128, 0.1f);
    engine->hnswInsert("hnsw_ext_topk_test", vec);
    auto results = engine->hnswSearch(vec, 1000);
    EXPECT_LE(results.size(), 1000u);
}

// =====================================================================
// 量化推理
// =====================================================================

TEST_F(EdgeAIEngineExtTest, Quantize_BasicRoundTrip) {
    std::vector<float> weights = {0.5f, -0.3f, 0.8f, -0.1f, 0.0f};
    auto quantized = engine->quantizeToInt8(weights, {weights.size()});
    EXPECT_EQ(quantized.data.size(), weights.size());
}

TEST_F(EdgeAIEngineExtTest, Quantize_AllZeros) {
    std::vector<float> weights(10, 0.0f);
    auto quantized = engine->quantizeToInt8(weights, {weights.size()});
    for (auto v : quantized.data) {
        EXPECT_EQ(v, 0);
    }
}

TEST_F(EdgeAIEngineExtTest, Quantize_EmptyVector) {
    std::vector<float> empty;
    auto quantized = engine->quantizeToInt8(empty, {0});
    EXPECT_TRUE(quantized.data.empty());
}

TEST_F(EdgeAIEngineExtTest, Quantize_SingleElement) {
    std::vector<float> weights = {0.42f};
    auto quantized = engine->quantizeToInt8(weights, {1});
    EXPECT_EQ(quantized.data.size(), 1u);
}

// =====================================================================
// 情绪脉搏
// =====================================================================

TEST_F(EdgeAIEngineExtTest, EmotionPulse_RecordAndGet) {
    engine->submitEmotionSample(0.5f, "happy");
    auto pulse = engine->getCurrentPulse();
    EXPECT_GE(pulse.sampleCount, 0);
}

TEST_F(EdgeAIEngineExtTest, EmotionPulse_MultipleRecords) {
    for (int i = 0; i < 10; ++i) {
        engine->submitEmotionSample(
            (i % 2 == 0) ? 0.5f : -0.3f,
            (i % 2 == 0) ? "happy" : "sad");
    }
    auto pulse = engine->getCurrentPulse();
    EXPECT_GE(pulse.sampleCount, 0);
}

TEST_F(EdgeAIEngineExtTest, EmotionPulse_EmptyMood) {
    EXPECT_NO_THROW(engine->submitEmotionSample(0.0f, ""));
}

TEST_F(EdgeAIEngineExtTest, EmotionPulse_ExtremeScore) {
    EXPECT_NO_THROW(engine->submitEmotionSample(999.0f, "happy"));
    EXPECT_NO_THROW(engine->submitEmotionSample(-999.0f, "sad"));
}

// =====================================================================
// 联邦学习
// =====================================================================

TEST_F(EdgeAIEngineExtTest, FederatedLearning_AggregateEmpty) {
    // No models submitted, aggregation should return empty/default
    auto result = engine->aggregateFedAvg();
    // Just verify no crash
    EXPECT_TRUE(true);
}

TEST_F(EdgeAIEngineExtTest, FederatedLearning_SingleModel) {
    FederatedModelParams params;
    params.modelId = "fed_single";
    params.weights = {{1.0f, 2.0f, 3.0f}};
    params.biases = {0.1f};
    params.sampleCount = 100;
    engine->submitLocalModel(params);
    auto result = engine->aggregateFedAvg();
    EXPECT_FALSE(result.modelId.empty());
}

TEST_F(EdgeAIEngineExtTest, FederatedLearning_TwoModels) {
    FederatedModelParams p1;
    p1.modelId = "fed_a";
    p1.weights = {{1.0f, 0.0f}};
    p1.biases = {0.0f};
    p1.sampleCount = 50;
    engine->submitLocalModel(p1);

    FederatedModelParams p2;
    p2.modelId = "fed_b";
    p2.weights = {{0.0f, 1.0f}};
    p2.biases = {1.0f};
    p2.sampleCount = 50;
    engine->submitLocalModel(p2);

    auto result = engine->aggregateFedAvg();
    EXPECT_FALSE(result.weights.empty());
}

TEST_F(EdgeAIEngineExtTest, FederatedLearning_Status) {
    auto status = engine->getFederatedStatus();
    EXPECT_FALSE(status.isNull());
}

// =====================================================================
// 节点监控
// =====================================================================

TEST_F(EdgeAIEngineExtTest, NodeMonitor_RegisterNode) {
    engine->registerNode("ext_test_node_1");
    auto all = engine->getAllNodeStatus();
    bool found = false;
    for (auto& n : all) {
        if (n.nodeId == "ext_test_node_1") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(EdgeAIEngineExtTest, NodeMonitor_UpdateStatus) {
    engine->registerNode("ext_test_node_2");
    EdgeNodeStatus status;
    status.nodeId = "ext_test_node_2";
    status.cpuUsage = 0.5f;
    status.memoryUsage = 0.4f;
    status.latencyMs = 30.0f;
    status.activeConnections = 10;
    status.totalRequests = 100;
    status.failedRequests = 2;
    status.isHealthy = true;
    status.lastHeartbeat = std::chrono::steady_clock::now();
    status.healthScore = 0.9f;
    engine->updateNodeStatus(status);

    auto all = engine->getAllNodeStatus();
    for (auto& n : all) {
        if (n.nodeId == "ext_test_node_2") {
            EXPECT_NEAR(n.cpuUsage, 0.5f, 0.01f);
        }
    }
}

TEST_F(EdgeAIEngineExtTest, NodeMonitor_SelectBestFromEmpty) {
    // After registering only unhealthy nodes, selectBestNode may return nullopt
    auto best = engine->selectBestNode();
    // Just verify it doesn't crash
    EXPECT_TRUE(true);
}

TEST_F(EdgeAIEngineExtTest, NodeMonitor_GetHNSWStats) {
    auto stats = engine->getHNSWStats();
    EXPECT_FALSE(stats.isNull());
}

// =====================================================================
// 差分隐私（通过EdgeAIEngine接口）
// =====================================================================

TEST_F(EdgeAIEngineExtTest, DP_AddNoise_Finite) {
    engine->resetPrivacyBudget();
    for (int i = 0; i < 100; ++i) {
        auto noisy = engine->addLaplaceNoise(50.0f, 1.0f);
        EXPECT_TRUE(std::isfinite(noisy));
    }
}

TEST_F(EdgeAIEngineExtTest, DP_AddNoise_MeanNearOriginal) {
    engine->resetPrivacyBudget();
    double sum = 0.0;
    const int N = 5000;
    for (int i = 0; i < N; ++i) {
        if (i % 500 == 0) engine->resetPrivacyBudget();
        sum += engine->addLaplaceNoise(100.0f, 1.0f);
    }
    double mean = sum / N;
    EXPECT_NEAR(mean, 100.0, 5.0);
}

TEST_F(EdgeAIEngineExtTest, DP_AddNoiseVec) {
    engine->resetPrivacyBudget();
    std::vector<float> vals = {1.0f, 2.0f, 3.0f};
    auto noisy = engine->addLaplaceNoiseVec(vals, 1.0f);
    EXPECT_EQ(noisy.size(), vals.size());
}

TEST_F(EdgeAIEngineExtTest, DP_RemainingBudget) {
    engine->resetPrivacyBudget();
    float budget = engine->getRemainingPrivacyBudget();
    EXPECT_GT(budget, 0.0f);
}

TEST_F(EdgeAIEngineExtTest, DP_ResetBudget) {
    engine->resetPrivacyBudget();
    float budget = engine->getRemainingPrivacyBudget();
    EXPECT_GT(budget, 0.0f);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    _exit(ret);
}
