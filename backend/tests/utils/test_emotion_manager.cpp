/**
 * EmotionManager 单元测试 - 情绪分类、分数映射、关键词分析
 */

#include <gtest/gtest.h>
#include "utils/EmotionManager.h"
#include <string>
#include <vector>
#include <algorithm>

using namespace heartlake::emotion;

// =====================================================================
// EmotionManager 测试
// =====================================================================

class EmotionManagerTest : public ::testing::Test {
protected:
    EmotionManager& mgr = EmotionManager::getInstance();
};

// -------------------- getEmotionFromScore --------------------

TEST_F(EmotionManagerTest, Score_HighPositive_ReturnsHappy) {
    auto emotion = mgr.getEmotionFromScore(0.8f);
    EXPECT_EQ(emotion, "happy");
}

TEST_F(EmotionManagerTest, Score_MildPositive_ReturnsCalm) {
    auto emotion = mgr.getEmotionFromScore(0.45f);
    EXPECT_EQ(emotion, "calm");
}

TEST_F(EmotionManagerTest, Score_Zero_ReturnsNeutral) {
    auto emotion = mgr.getEmotionFromScore(0.0f);
    EXPECT_EQ(emotion, "neutral");
}

TEST_F(EmotionManagerTest, Score_MildNegative_ReturnsAnxious) {
    auto emotion = mgr.getEmotionFromScore(-0.45f);
    EXPECT_EQ(emotion, "anxious");
}

TEST_F(EmotionManagerTest, Score_HighNegative_ReturnsSad) {
    auto emotion = mgr.getEmotionFromScore(-0.8f);
    EXPECT_EQ(emotion, "sad");
}

TEST_F(EmotionManagerTest, Score_MaxPositive_ReturnsHappy) {
    auto emotion = mgr.getEmotionFromScore(1.0f);
    EXPECT_EQ(emotion, "happy");
}

TEST_F(EmotionManagerTest, Score_MaxNegative_ReturnsSad) {
    auto emotion = mgr.getEmotionFromScore(-1.0f);
    EXPECT_EQ(emotion, "sad");
}

// -------------------- analyzeEmotionFromText --------------------

TEST_F(EmotionManagerTest, TextAnalysis_HappyKeywords) {
    auto [score, emotion] = mgr.analyzeEmotionFromText("今天好开心，真快乐");
    EXPECT_GT(score, 0.0f);
    // 应该识别出正面情绪
    EXPECT_TRUE(emotion == "happy" || emotion == "calm");
}

TEST_F(EmotionManagerTest, TextAnalysis_SadKeywords) {
    auto [score, emotion] = mgr.analyzeEmotionFromText("好难过，真伤心");
    EXPECT_LT(score, 0.0f);
    EXPECT_TRUE(emotion == "sad" || emotion == "anxious");
}

TEST_F(EmotionManagerTest, TextAnalysis_NeutralText) {
    auto [score, emotion] = mgr.analyzeEmotionFromText("还好吧，一般般");
    // 中性文本分数应接近0
    EXPECT_GE(score, -0.6f);
    EXPECT_LE(score, 0.6f);
}

TEST_F(EmotionManagerTest, TextAnalysis_EmptyText) {
    auto [score, emotion] = mgr.analyzeEmotionFromText("");
    // 空文本应返回中性
    EXPECT_EQ(emotion, "neutral");
}

TEST_F(EmotionManagerTest, TextAnalysis_ScoreInRange) {
    auto [score, emotion] = mgr.analyzeEmotionFromText("非常开心快乐愉快幸福喜悦");
    EXPECT_GE(score, -1.0f);
    EXPECT_LE(score, 1.0f);
}

// -------------------- getEmotionColor --------------------

TEST_F(EmotionManagerTest, Color_Happy_ReturnsOrange) {
    auto color = mgr.getEmotionColor("happy");
    EXPECT_EQ(color, "#FF8A65");
}

TEST_F(EmotionManagerTest, Color_Sad_ReturnsBlue) {
    auto color = mgr.getEmotionColor("sad");
    EXPECT_EQ(color, "#42A5F5");
}

TEST_F(EmotionManagerTest, Color_Unknown_ReturnsDefaultGray) {
    auto color = mgr.getEmotionColor("nonexistent_emotion");
    EXPECT_EQ(color, "#9E9E9E");
}

// -------------------- getEmotionConfig --------------------

TEST_F(EmotionManagerTest, Config_Happy_HasCorrectLabel) {
    auto config = mgr.getEmotionConfig("happy");
    EXPECT_EQ(config.label, "开心");
    EXPECT_EQ(config.name, "happy");
}

TEST_F(EmotionManagerTest, Config_Unknown_ReturnNeutralDefault) {
    auto config = mgr.getEmotionConfig("unknown_type");
    EXPECT_EQ(config.name, "neutral");
    EXPECT_EQ(config.label, "中性");
}

TEST_F(EmotionManagerTest, Config_ScoreRange_Valid) {
    auto config = mgr.getEmotionConfig("happy");
    EXPECT_LT(config.scoreMin, config.scoreMax);
    EXPECT_FLOAT_EQ(config.scoreMin, 0.6f);
    EXPECT_FLOAT_EQ(config.scoreMax, 1.0f);
}

// -------------------- isValidEmotion --------------------

TEST_F(EmotionManagerTest, IsValid_KnownEmotions) {
    EXPECT_TRUE(mgr.isValidEmotion("happy"));
    EXPECT_TRUE(mgr.isValidEmotion("calm"));
    EXPECT_TRUE(mgr.isValidEmotion("neutral"));
    EXPECT_TRUE(mgr.isValidEmotion("anxious"));
    EXPECT_TRUE(mgr.isValidEmotion("sad"));
}

TEST_F(EmotionManagerTest, IsValid_UnknownEmotion_ReturnsFalse) {
    EXPECT_FALSE(mgr.isValidEmotion(""));
    EXPECT_FALSE(mgr.isValidEmotion("rage"));
    EXPECT_FALSE(mgr.isValidEmotion("HAPPY"));  // 大小写敏感
}

// -------------------- getAllEmotions --------------------

TEST_F(EmotionManagerTest, GetAllEmotions_ContainsExpectedTypes) {
    auto all = mgr.getAllEmotions();
    EXPECT_GE(all.size(), 5u);  // 至少 happy/calm/neutral/anxious/sad

    // 验证核心情绪类型都在列表中
    EXPECT_NE(std::find(all.begin(), all.end(), "happy"), all.end());
    EXPECT_NE(std::find(all.begin(), all.end(), "sad"), all.end());
    EXPECT_NE(std::find(all.begin(), all.end(), "neutral"), all.end());
}

// =====================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
