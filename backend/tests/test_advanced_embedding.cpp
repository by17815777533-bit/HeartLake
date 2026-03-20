/**
 * AdvancedEmbeddingEngine 单元测试
 * @author 白洋
 * @date 2025-02-08
 */

#include <gtest/gtest.h>
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include <vector>
#include <string>
#include <cmath>

using namespace heartlake::ai;

class AdvancedEmbeddingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = &AdvancedEmbeddingEngine::getInstance();
        engine->initialize(128, 1000);
    }

    AdvancedEmbeddingEngine* engine;
};

// 测试基本的嵌入向量生成
TEST_F(AdvancedEmbeddingEngineTest, BasicEmbeddingGeneration) {
    std::string text = "我今天很开心";
    auto embedding = engine->generateEmbedding(text);

    EXPECT_EQ(embedding.size(), 128);

    // 检查向量是否已归一化
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    EXPECT_NEAR(norm, 1.0f, 0.01f);
}

// 测试空文本处理
TEST_F(AdvancedEmbeddingEngineTest, EmptyTextHandling) {
    std::string text = "";
    auto embedding = engine->generateEmbedding(text);

    EXPECT_EQ(embedding.size(), 128);

    // 空文本应该返回零向量
    float sum = 0.0f;
    for (float val : embedding) {
        sum += std::abs(val);
    }
    EXPECT_FLOAT_EQ(sum, 0.0f);
}

// 测试中文分词
TEST_F(AdvancedEmbeddingEngineTest, ChineseTokenization) {
    std::string text1 = "我很开心";
    std::string text2 = "我很难过";

    auto embedding1 = engine->generateEmbedding(text1);
    auto embedding2 = engine->generateEmbedding(text2);

    // 两个不同情感的文本应该有不同的嵌入向量
    float similarity = AdvancedEmbeddingEngine::cosineSimilarity(embedding1, embedding2);
    EXPECT_LT(similarity, 0.99f);  // 不应该完全相同
}

// 测试相似文本的相似度
TEST_F(AdvancedEmbeddingEngineTest, SimilarTextSimilarity) {
    std::string text1 = "今天天气很好";
    std::string text2 = "今天天气不错";
    std::string text3 = "我喜欢编程";

    auto embedding1 = engine->generateEmbedding(text1);
    auto embedding2 = engine->generateEmbedding(text2);
    auto embedding3 = engine->generateEmbedding(text3);

    float sim12 = AdvancedEmbeddingEngine::cosineSimilarity(embedding1, embedding2);
    float sim13 = AdvancedEmbeddingEngine::cosineSimilarity(embedding1, embedding3);

    // 相似文本的相似度应该更高
    EXPECT_GT(sim12, sim13);
}

// 测试批量处理
TEST_F(AdvancedEmbeddingEngineTest, BatchProcessing) {
    std::vector<std::string> texts = {
        "我很开心",
        "今天天气很好",
        "我喜欢编程"
    };

    auto embeddings = engine->generateEmbeddingBatch(texts);

    EXPECT_EQ(embeddings.size(), 3);
    for (const auto& embedding : embeddings) {
        EXPECT_EQ(embedding.size(), 128);
    }
}

// 测试缓存功能
TEST_F(AdvancedEmbeddingEngineTest, CacheFunctionality) {
    std::string text = "测试缓存功能";

    // 第一次调用
    auto embedding1 = engine->generateEmbedding(text);
    auto stats1 = engine->getCacheStats();

    // 第二次调用（应该命中缓存）
    auto embedding2 = engine->generateEmbedding(text);
    auto stats2 = engine->getCacheStats();

    // 验证缓存命中
    EXPECT_GT(stats2.hits, stats1.hits);

    // 验证结果一致
    EXPECT_EQ(embedding1.size(), embedding2.size());
    for (size_t i = 0; i < embedding1.size(); ++i) {
        EXPECT_FLOAT_EQ(embedding1[i], embedding2[i]);
    }
}

// 测试余弦相似度计算
TEST_F(AdvancedEmbeddingEngineTest, CosineSimilarityCalculation) {
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {1.0f, 0.0f, 0.0f};
    std::vector<float> vec3 = {0.0f, 1.0f, 0.0f};

    float sim12 = AdvancedEmbeddingEngine::cosineSimilarity(vec1, vec2);
    float sim13 = AdvancedEmbeddingEngine::cosineSimilarity(vec1, vec3);

    EXPECT_FLOAT_EQ(sim12, 1.0f);  // 相同
    EXPECT_FLOAT_EQ(sim13, 0.0f);  // 正交
}

// 测试混合中英文
TEST_F(AdvancedEmbeddingEngineTest, MixedChineseEnglish) {
    std::string text = "Hello世界123";
    auto embedding = engine->generateEmbedding(text);

    EXPECT_EQ(embedding.size(), 128);

    // 验证向量不是零向量
    float sum = 0.0f;
    for (float val : embedding) {
        sum += std::abs(val);
    }
    EXPECT_GT(sum, 0.0f);
}

// 测试长文本处理
TEST_F(AdvancedEmbeddingEngineTest, LongTextHandling) {
    std::string repeated;
    repeated.reserve(1000 * 3);
    for (int i = 0; i < 1000; ++i) {
        repeated += "测";
    }
    std::string longText = "这是一段很长的文本。" + repeated + "结束。";
    auto embedding = engine->generateEmbedding(longText);

    EXPECT_EQ(embedding.size(), 128);
}

// 测试特殊字符处理
TEST_F(AdvancedEmbeddingEngineTest, SpecialCharacterHandling) {
    std::string text = "测试！@#$%^&*()_+-=[]{}|;':\",./<>?";
    auto embedding = engine->generateEmbedding(text);

    EXPECT_EQ(embedding.size(), 128);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
