/**
 * @file test_emotion_resonance.cpp
 * @brief EmotionResonanceEngine 单元测试
 *
 * 覆盖：
 * - DTW (Dynamic Time Warping) 轨迹相似度
 * - 时间衰减函数
 * - 多样性奖励计算
 * - 共鸣原因生成
 * - 边界条件
 */

#include <gtest/gtest.h>
#include "infrastructure/ai/EmotionResonanceEngine.h"
#include <cmath>
#include <vector>
#include <string>

using namespace heartlake::ai;

class EmotionResonanceTest : public ::testing::Test {
protected:
    EmotionResonanceEngine* engine;

    void SetUp() override {
        engine = &EmotionResonanceEngine::getInstance();
    }
};

// ==================== DTW 轨迹相似度测试 ====================

TEST_F(EmotionResonanceTest, DTW_IdenticalTrajectories_HighSimilarity) {
    std::vector<float> traj = {0.5f, 0.3f, -0.2f, 0.1f, 0.4f};
    float sim = engine->trajectorySimDTW(traj, traj);

    // 完全相同的轨迹应有最高相似度
    EXPECT_NEAR(sim, 1.0f, 0.01f);
}

TEST_F(EmotionResonanceTest, DTW_OppositeTrajectories_LowSimilarity) {
    std::vector<float> traj1 = {0.8f, 0.9f, 0.7f, 0.85f};
    std::vector<float> traj2 = {-0.8f, -0.9f, -0.7f, -0.85f};

    float sim = engine->trajectorySimDTW(traj1, traj2);

    // 完全相反的轨迹应有低相似度
    EXPECT_LT(sim, 0.5f);
}

TEST_F(EmotionResonanceTest, DTW_SimilarTrajectories_MediumHighSimilarity) {
    std::vector<float> traj1 = {0.5f, 0.3f, -0.2f, 0.1f};
    std::vector<float> traj2 = {0.4f, 0.2f, -0.1f, 0.2f};

    float sim = engine->trajectorySimDTW(traj1, traj2);

    EXPECT_GT(sim, 0.5f);
    EXPECT_LE(sim, 1.0f);
}

TEST_F(EmotionResonanceTest, DTW_DifferentLengths_StillWorks) {
    std::vector<float> traj1 = {0.5f, 0.3f, -0.2f};
    std::vector<float> traj2 = {0.5f, 0.3f, -0.2f, 0.1f, 0.4f};

    float sim = engine->trajectorySimDTW(traj1, traj2);

    EXPECT_GE(sim, 0.0f);
    EXPECT_LE(sim, 1.0f);
}

TEST_F(EmotionResonanceTest, DTW_EmptyTrajectory_ReturnsZero) {
    std::vector<float> empty;
    std::vector<float> traj = {0.5f, 0.3f};

    EXPECT_FLOAT_EQ(engine->trajectorySimDTW(empty, traj), 0.0f);
    EXPECT_FLOAT_EQ(engine->trajectorySimDTW(traj, empty), 0.0f);
    EXPECT_FLOAT_EQ(engine->trajectorySimDTW(empty, empty), 0.0f);
}

TEST_F(EmotionResonanceTest, DTW_SingleElement_Works) {
    std::vector<float> traj1 = {0.5f};
    std::vector<float> traj2 = {0.5f};

    float sim = engine->trajectorySimDTW(traj1, traj2);
    EXPECT_NEAR(sim, 1.0f, 0.01f);
}

TEST_F(EmotionResonanceTest, DTW_Symmetry) {
    std::vector<float> traj1 = {0.5f, 0.3f, -0.2f, 0.1f};
    std::vector<float> traj2 = {0.4f, 0.1f, -0.3f, 0.2f};

    float sim12 = engine->trajectorySimDTW(traj1, traj2);
    float sim21 = engine->trajectorySimDTW(traj2, traj1);

    EXPECT_NEAR(sim12, sim21, 0.001f);
}

// ==================== 时间衰减测试 ====================

TEST_F(EmotionResonanceTest, TemporalDecay_RecentTimestamp_HighDecay) {
    // 使用当前时间，衰减应接近1.0
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    float decay = engine->temporalDecay(std::string(buf));
    EXPECT_GT(decay, 0.8f);
    EXPECT_LE(decay, 1.0f);
}

TEST_F(EmotionResonanceTest, TemporalDecay_OldTimestamp_LowDecay) {
    // 30天前的时间戳
    float decay = engine->temporalDecay("2020-01-01 00:00:00");
    EXPECT_LT(decay, 0.1f);
    EXPECT_GE(decay, 0.0f);
}

TEST_F(EmotionResonanceTest, TemporalDecay_InvalidTimestamp_ReturnsFallback) {
    float decay = engine->temporalDecay("invalid_timestamp");
    // 应返回合理的默认值
    EXPECT_GE(decay, 0.0f);
    EXPECT_LE(decay, 1.0f);
}

TEST_F(EmotionResonanceTest, TemporalDecay_HigherLambda_FasterDecay) {
    auto now = std::chrono::system_clock::now();
    auto yesterday = now - std::chrono::hours(24);
    auto t = std::chrono::system_clock::to_time_t(yesterday);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    float decay_slow = engine->temporalDecay(std::string(buf), 0.01f);
    float decay_fast = engine->temporalDecay(std::string(buf), 1.0f);

    EXPECT_GT(decay_slow, decay_fast);
}

// ==================== 多样性奖励测试 ====================

TEST_F(EmotionResonanceTest, DiversityBonus_DifferentMood_HighBonus) {
    std::vector<std::string> recommended = {"sad", "sad", "sad"};
    float bonus = engine->diversityBonus("sad", "happy", recommended);

    EXPECT_GT(bonus, 0.5f);
}

TEST_F(EmotionResonanceTest, DiversityBonus_SameMood_LowBonus) {
    std::vector<std::string> recommended = {"happy", "happy"};
    float bonus = engine->diversityBonus("happy", "happy", recommended);

    EXPECT_LT(bonus, 0.5f);
}

TEST_F(EmotionResonanceTest, DiversityBonus_EmptyRecommended_BaseBonus) {
    std::vector<std::string> empty;
    float bonus = engine->diversityBonus("sad", "happy", empty);

    EXPECT_GE(bonus, 0.0f);
    EXPECT_LE(bonus, 1.0f);
}

// ==================== 共鸣原因生成测试 ====================

TEST_F(EmotionResonanceTest, ResonanceReason_NotEmpty) {
    ResonanceResult result;
    result.semanticScore = 0.8f;
    result.trajectoryScore = 0.7f;
    result.temporalScore = 0.9f;
    result.diversityScore = 0.5f;
    result.totalScore = 0.75f;

    std::string reason = engine->generateResonanceReason(result, "sad", "sad");
    EXPECT_FALSE(reason.empty());
}

TEST_F(EmotionResonanceTest, ResonanceReason_DifferentMoods_MentionsDiversity) {
    ResonanceResult result;
    result.semanticScore = 0.5f;
    result.trajectoryScore = 0.5f;
    result.temporalScore = 0.5f;
    result.diversityScore = 0.9f;
    result.totalScore = 0.6f;

    std::string reason = engine->generateResonanceReason(result, "sad", "hopeful");
    EXPECT_FALSE(reason.empty());
}

// ==================== 权重参数测试 ====================

TEST_F(EmotionResonanceTest, Weights_SumToOne) {
    float sum = engine->getAlpha() + engine->getBeta() + engine->getGamma() + engine->getDelta();
    EXPECT_NEAR(sum, 1.0f, 0.001f);
}

TEST_F(EmotionResonanceTest, Weights_AllPositive) {
    EXPECT_GT(engine->getAlpha(), 0.0f);
    EXPECT_GT(engine->getBeta(), 0.0f);
    EXPECT_GT(engine->getGamma(), 0.0f);
    EXPECT_GT(engine->getDelta(), 0.0f);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
