/**
 * @file test_emotion_resonance_extended.cpp
 * @brief EmotionResonanceEngine 扩展测试
 */

#include <gtest/gtest.h>
#include "infrastructure/ai/EmotionResonanceEngine.h"
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

using namespace heartlake::ai;

class EmotionResExtTest : public ::testing::Test {
protected:
    EmotionResonanceEngine* engine;
    void SetUp() override {
        engine = &EmotionResonanceEngine::getInstance();
    }
};

// =====================================================================
// DTW 距离计算正确性
// =====================================================================

TEST_F(EmotionResExtTest, DTW_Identical_Score1) {
    std::vector<float> t = {0.1f, 0.5f, -0.3f, 0.7f};
    EXPECT_NEAR(engine->trajectorySimDTW(t, t), 1.0f, 0.01f);
}

TEST_F(EmotionResExtTest, DTW_Opposite_LowScore) {
    std::vector<float> t1 = {1.0f, 1.0f, 1.0f};
    std::vector<float> t2 = {-1.0f, -1.0f, -1.0f};
    float sim = engine->trajectorySimDTW(t1, t2);
    EXPECT_LT(sim, 0.3f);
}

TEST_F(EmotionResExtTest, DTW_SlightlyDifferent_HighScore) {
    std::vector<float> t1 = {0.5f, 0.6f, 0.4f, 0.55f};
    std::vector<float> t2 = {0.48f, 0.62f, 0.38f, 0.53f};
    float sim = engine->trajectorySimDTW(t1, t2);
    EXPECT_GT(sim, 0.8f);
}

TEST_F(EmotionResExtTest, DTW_Empty_Both_Zero) {
    std::vector<float> e;
    EXPECT_FLOAT_EQ(engine->trajectorySimDTW(e, e), 0.0f);
}

TEST_F(EmotionResExtTest, DTW_Empty_One_Zero) {
    std::vector<float> e, t = {0.5f};
    EXPECT_FLOAT_EQ(engine->trajectorySimDTW(e, t), 0.0f);
    EXPECT_FLOAT_EQ(engine->trajectorySimDTW(t, e), 0.0f);
}

TEST_F(EmotionResExtTest, DTW_SingleElement_Same) {
    std::vector<float> t1 = {0.5f}, t2 = {0.5f};
    EXPECT_NEAR(engine->trajectorySimDTW(t1, t2), 1.0f, 0.01f);
}

TEST_F(EmotionResExtTest, DTW_SingleElement_Different) {
    std::vector<float> t1 = {1.0f}, t2 = {-1.0f};
    float sim = engine->trajectorySimDTW(t1, t2);
    EXPECT_LT(sim, 0.5f);
}

TEST_F(EmotionResExtTest, DTW_Symmetry) {
    std::vector<float> t1 = {0.1f, -0.3f, 0.5f, 0.2f};
    std::vector<float> t2 = {0.3f, -0.1f, 0.4f, 0.0f};
    float s12 = engine->trajectorySimDTW(t1, t2);
    float s21 = engine->trajectorySimDTW(t2, t1);
    EXPECT_NEAR(s12, s21, 0.001f);
}

TEST_F(EmotionResExtTest, DTW_DifferentLengths) {
    std::vector<float> t1 = {0.5f, 0.3f};
    std::vector<float> t2 = {0.5f, 0.3f, 0.1f, -0.2f, 0.4f};
    float sim = engine->trajectorySimDTW(t1, t2);
    EXPECT_GE(sim, 0.0f);
    EXPECT_LE(sim, 1.0f);
}

TEST_F(EmotionResExtTest, DTW_AllZeros) {
    std::vector<float> t(5, 0.0f);
    float sim = engine->trajectorySimDTW(t, t);
    EXPECT_NEAR(sim, 1.0f, 0.01f);
}

TEST_F(EmotionResExtTest, DTW_AllOnes) {
    std::vector<float> t(5, 1.0f);
    float sim = engine->trajectorySimDTW(t, t);
    EXPECT_NEAR(sim, 1.0f, 0.01f);
}

TEST_F(EmotionResExtTest, DTW_LongTrajectory) {
    std::vector<float> t1(100), t2(100);
    for (int i = 0; i < 100; ++i) {
        t1[i] = std::sin(i * 0.1f);
        t2[i] = std::sin(i * 0.1f + 0.1f);
    }
    float sim = engine->trajectorySimDTW(t1, t2);
    EXPECT_GT(sim, 0.5f);
}

TEST_F(EmotionResExtTest, DTW_ExtremeValues) {
    std::vector<float> t1 = {-100.0f, 100.0f};
    std::vector<float> t2 = {-100.0f, 100.0f};
    float sim = engine->trajectorySimDTW(t1, t2);
    EXPECT_NEAR(sim, 1.0f, 0.01f);
}

// =====================================================================
// 时间衰减
// =====================================================================

TEST_F(EmotionResExtTest, Decay_Now_NearOne) {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    float decay = engine->temporalDecay(std::string(buf));
    EXPECT_GT(decay, 0.8f);
    EXPECT_LE(decay, 1.0f);
}

TEST_F(EmotionResExtTest, Decay_VeryOld_NearZero) {
    float decay = engine->temporalDecay("2000-01-01 00:00:00");
    EXPECT_LT(decay, 0.01f);
}

TEST_F(EmotionResExtTest, Decay_OneHourAgo) {
    auto now = std::chrono::system_clock::now();
    auto hourAgo = now - std::chrono::hours(1);
    auto t = std::chrono::system_clock::to_time_t(hourAgo);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    float decay = engine->temporalDecay(std::string(buf));
    EXPECT_GT(decay, 0.5f);
}

TEST_F(EmotionResExtTest, Decay_OneDayAgo) {
    auto now = std::chrono::system_clock::now();
    auto dayAgo = now - std::chrono::hours(24);
    auto t = std::chrono::system_clock::to_time_t(dayAgo);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    float decay = engine->temporalDecay(std::string(buf));
    EXPECT_GE(decay, 0.0f);
    EXPECT_LE(decay, 1.0f);
}

TEST_F(EmotionResExtTest, Decay_InvalidTimestamp_Fallback) {
    float decay = engine->temporalDecay("not_a_timestamp");
    EXPECT_GE(decay, 0.0f);
    EXPECT_LE(decay, 1.0f);
}

TEST_F(EmotionResExtTest, Decay_EmptyTimestamp_Fallback) {
    float decay = engine->temporalDecay("");
    EXPECT_GE(decay, 0.0f);
    EXPECT_LE(decay, 1.0f);
}

TEST_F(EmotionResExtTest, Decay_HighLambda_FasterDecay) {
    auto now = std::chrono::system_clock::now();
    auto dayAgo = now - std::chrono::hours(24);
    auto t = std::chrono::system_clock::to_time_t(dayAgo);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    float slow = engine->temporalDecay(std::string(buf), 0.01f);
    float fast = engine->temporalDecay(std::string(buf), 1.0f);
    EXPECT_GT(slow, fast);
}

TEST_F(EmotionResExtTest, Decay_ZeroLambda_NoDecay) {
    float decay = engine->temporalDecay("2020-01-01 00:00:00", 0.0f);
    EXPECT_NEAR(decay, 1.0f, 0.01f);
}

// =====================================================================
// 多样性奖励
// =====================================================================

TEST_F(EmotionResExtTest, Diversity_DifferentMood_HighBonus) {
    std::vector<std::string> rec = {"sad", "sad", "sad"};
    float bonus = engine->diversityBonus("sad", "happy", rec);
    EXPECT_GT(bonus, 0.3f);
}

TEST_F(EmotionResExtTest, Diversity_SameMood_LowBonus) {
    std::vector<std::string> rec = {"happy", "happy"};
    float bonus = engine->diversityBonus("happy", "happy", rec);
    EXPECT_LT(bonus, 0.5f);
}

TEST_F(EmotionResExtTest, Diversity_EmptyRecommended) {
    std::vector<std::string> empty;
    float bonus = engine->diversityBonus("sad", "happy", empty);
    EXPECT_GE(bonus, 0.0f);
    EXPECT_LE(bonus, 1.0f);
}

TEST_F(EmotionResExtTest, Diversity_MixedRecommended) {
    std::vector<std::string> rec = {"happy", "sad", "angry", "neutral"};
    float bonus = engine->diversityBonus("neutral", "fear", rec);
    EXPECT_GE(bonus, 0.0f);
    EXPECT_LE(bonus, 1.0f);
}

TEST_F(EmotionResExtTest, Diversity_SameCurrentAndCandidate) {
    std::vector<std::string> rec = {"angry"};
    float bonus = engine->diversityBonus("happy", "happy", rec);
    EXPECT_GE(bonus, 0.0f);
}

// =====================================================================
// 共鸣原因生成
// =====================================================================

TEST_F(EmotionResExtTest, Reason_NotEmpty) {
    ResonanceResult r;
    r.semanticScore = 0.8f;
    r.trajectoryScore = 0.7f;
    r.temporalScore = 0.9f;
    r.diversityScore = 0.5f;
    r.totalScore = 0.75f;
    auto reason = engine->generateResonanceReason(r, "sad", "sad");
    EXPECT_FALSE(reason.empty());
}

TEST_F(EmotionResExtTest, Reason_DifferentMoods) {
    ResonanceResult r;
    r.semanticScore = 0.3f;
    r.trajectoryScore = 0.3f;
    r.temporalScore = 0.3f;
    r.diversityScore = 0.9f;
    r.totalScore = 0.4f;
    auto reason = engine->generateResonanceReason(r, "sad", "hopeful");
    EXPECT_FALSE(reason.empty());
}

TEST_F(EmotionResExtTest, Reason_HighSemantic) {
    ResonanceResult r;
    r.semanticScore = 0.95f;
    r.trajectoryScore = 0.1f;
    r.temporalScore = 0.1f;
    r.diversityScore = 0.1f;
    r.totalScore = 0.5f;
    auto reason = engine->generateResonanceReason(r, "happy", "happy");
    EXPECT_FALSE(reason.empty());
}

TEST_F(EmotionResExtTest, Reason_HighTrajectory) {
    ResonanceResult r;
    r.semanticScore = 0.1f;
    r.trajectoryScore = 0.95f;
    r.temporalScore = 0.1f;
    r.diversityScore = 0.1f;
    r.totalScore = 0.5f;
    auto reason = engine->generateResonanceReason(r, "neutral", "neutral");
    EXPECT_FALSE(reason.empty());
}

TEST_F(EmotionResExtTest, Reason_AllZeroScores) {
    ResonanceResult r;
    r.semanticScore = 0.0f;
    r.trajectoryScore = 0.0f;
    r.temporalScore = 0.0f;
    r.diversityScore = 0.0f;
    r.totalScore = 0.0f;
    auto reason = engine->generateResonanceReason(r, "neutral", "neutral");
    EXPECT_FALSE(reason.empty());
}

// =====================================================================
// 权重参数
// =====================================================================

TEST_F(EmotionResExtTest, Weights_SumToOne) {
    float sum = engine->alpha + engine->beta + engine->gamma + engine->delta;
    EXPECT_NEAR(sum, 1.0f, 0.001f);
}

TEST_F(EmotionResExtTest, Weights_AllPositive) {
    EXPECT_GT(engine->alpha, 0.0f);
    EXPECT_GT(engine->beta, 0.0f);
    EXPECT_GT(engine->gamma, 0.0f);
    EXPECT_GT(engine->delta, 0.0f);
}

TEST_F(EmotionResExtTest, Weights_AlphaValue) {
    EXPECT_NEAR(engine->alpha, 0.30f, 0.01f);
}

TEST_F(EmotionResExtTest, Weights_BetaValue) {
    EXPECT_NEAR(engine->beta, 0.35f, 0.01f);
}

TEST_F(EmotionResExtTest, Weights_GammaValue) {
    EXPECT_NEAR(engine->gamma, 0.20f, 0.01f);
}

TEST_F(EmotionResExtTest, Weights_DeltaValue) {
    EXPECT_NEAR(engine->delta, 0.15f, 0.01f);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
