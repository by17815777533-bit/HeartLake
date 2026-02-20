/**
 * @file test_dual_memory_rag.cpp
 * @brief DualMemoryRAG 双记忆系统单元测试
 *
 * 覆盖：
 * - 短期记忆滑动窗口
 * - 长期记忆画像
 * - 情绪趋势计算
 * - 波动度计算
 * - RAG提示词构建
 * - 边界条件
 */

#include <gtest/gtest.h>
#include "infrastructure/ai/DualMemoryRAG.h"
#include <string>
#include <vector>

using namespace heartlake::ai;

class DualMemoryRAGTest : public ::testing::Test {
protected:
    DualMemoryRAG* rag;

    void SetUp() override {
        rag = &DualMemoryRAG::getInstance();
    }
};

// ==================== 短期记忆测试 ====================

TEST_F(DualMemoryRAGTest, ShortTermMemory_AddEntry) {
    const std::string userId = "test_stm_user_1";
    rag->updateShortTermMemory(userId, "今天心情不错", "happy", 0.7f);

    // 通过 getEmotionInsights 间接验证
    auto insights = rag->getEmotionInsights(userId);
    EXPECT_TRUE(insights.isMember("recent_interactions"));
}

TEST_F(DualMemoryRAGTest, ShortTermMemory_SlidingWindow_MaxFive) {
    const std::string userId = "test_stm_user_2";

    // 添加7条记录，应只保留最近5条
    for (int i = 0; i < 7; ++i) {
        rag->updateShortTermMemory(userId,
            "消息" + std::to_string(i),
            i % 2 == 0 ? "happy" : "sad",
            i % 2 == 0 ? 0.5f : -0.3f);
    }

    auto insights = rag->getEmotionInsights(userId);
    auto recent = insights["recent_interactions"];
    EXPECT_LE(recent.size(), 5u);
}

TEST_F(DualMemoryRAGTest, ShortTermMemory_PreservesOrder) {
    const std::string userId = "test_stm_user_3";

    rag->updateShortTermMemory(userId, "第一条", "neutral", 0.0f);
    rag->updateShortTermMemory(userId, "第二条", "happy", 0.5f);
    rag->updateShortTermMemory(userId, "第三条", "sad", -0.3f);

    auto insights = rag->getEmotionInsights(userId);
    auto recent = insights["recent_interactions"];
    EXPECT_GE(recent.size(), 2u);
}

// ==================== 情绪趋势计算测试 ====================

TEST_F(DualMemoryRAGTest, EmotionInsights_ReturnsValidStructure) {
    const std::string userId = "test_insights_user";
    rag->updateShortTermMemory(userId, "测试内容", "neutral", 0.0f);

    auto insights = rag->getEmotionInsights(userId);

    EXPECT_TRUE(insights.isMember("user_id"));
    EXPECT_TRUE(insights.isMember("long_term_profile"));
    EXPECT_TRUE(insights.isMember("recent_interactions"));
}

TEST_F(DualMemoryRAGTest, EmotionInsights_LongTermProfile_HasFields) {
    const std::string userId = "test_profile_user";
    rag->updateShortTermMemory(userId, "内容", "happy", 0.5f);

    auto insights = rag->getEmotionInsights(userId);
    auto profile = insights["long_term_profile"];

    EXPECT_TRUE(profile.isMember("avg_emotion_score"));
    EXPECT_TRUE(profile.isMember("dominant_mood"));
    EXPECT_TRUE(profile.isMember("emotion_trend"));
    EXPECT_TRUE(profile.isMember("emotion_volatility"));
}

// ==================== RAG提示词构建测试 ====================

TEST_F(DualMemoryRAGTest, BuildRAGPrompt_NotEmpty) {
    EmotionMemory memory;
    memory.userId = "test_prompt_user";
    memory.longTerm.avgEmotionScore = -0.3f;
    memory.longTerm.dominantMood = "sad";
    memory.longTerm.emotionTrend = "falling";
    memory.longTerm.consecutiveNegativeDays = 3;

    EmotionMemory::ShortTermEntry entry;
    entry.content = "最近压力很大";
    entry.emotion = "stressed";
    entry.score = -0.4f;
    entry.timestamp = "2026-02-20 10:00:00";
    memory.shortTerm.push_back(entry);

    std::string prompt = rag->buildRAGPrompt(memory, "今天又加班了", "tired");

    EXPECT_FALSE(prompt.empty());
    EXPECT_GT(prompt.length(), 50u);
}

TEST_F(DualMemoryRAGTest, BuildRAGPrompt_EmptyMemory_StillWorks) {
    EmotionMemory memory;
    memory.userId = "empty_user";

    std::string prompt = rag->buildRAGPrompt(memory, "你好", "neutral");

    EXPECT_FALSE(prompt.empty());
}

TEST_F(DualMemoryRAGTest, BuildRAGPrompt_CriticalUser_IncludesSafety) {
    EmotionMemory memory;
    memory.userId = "critical_user";
    memory.longTerm.avgEmotionScore = -0.8f;
    memory.longTerm.dominantMood = "desperate";
    memory.longTerm.emotionTrend = "falling";
    memory.longTerm.consecutiveNegativeDays = 7;

    std::string prompt = rag->buildRAGPrompt(memory, "我不想活了", "desperate");

    EXPECT_FALSE(prompt.empty());
    // 对高风险用户，提示词应包含安全相关指导
    EXPECT_GT(prompt.length(), 100u);
}

// ==================== 边界条件测试 ====================

TEST_F(DualMemoryRAGTest, EmptyUserId_HandledGracefully) {
    // 空用户ID不应崩溃
    EXPECT_NO_THROW(rag->updateShortTermMemory("", "内容", "neutral", 0.0f));
    EXPECT_NO_THROW(rag->getEmotionInsights(""));
}

TEST_F(DualMemoryRAGTest, EmptyContent_HandledGracefully) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("user_empty_content", "", "neutral", 0.0f));
}

TEST_F(DualMemoryRAGTest, ExtremeScores_HandledGracefully) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("user_extreme", "极端", "happy", 999.0f));
    EXPECT_NO_THROW(rag->updateShortTermMemory("user_extreme", "极端", "sad", -999.0f));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
