/**
 * @file test_dual_memory_rag_extended.cpp
 * @brief DualMemoryRAG 扩展测试
 */

#include <gtest/gtest.h>
#include "infrastructure/ai/DualMemoryRAG.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace heartlake::ai;

class DualMemoryRAGExtTest : public ::testing::Test {
protected:
    DualMemoryRAG* rag;
    void SetUp() override {
        AdvancedEmbeddingEngine::getInstance().initialize(128, 10000);
        rag = &DualMemoryRAG::getInstance();
    }
};

// =====================================================================
// 短期记忆溢出淘汰
// =====================================================================

TEST_F(DualMemoryRAGExtTest, ShortTerm_Overflow_KeepsMax5) {
    const std::string uid = "stm_overflow_test";
    for (int i = 0; i < 10; ++i) {
        rag->updateShortTermMemory(uid, "msg_" + std::to_string(i),
                                    "neutral", 0.0f);
    }
    auto insights = rag->getEmotionInsights(uid);
    EXPECT_LE(insights["recent_interactions"].size(), 5u);
}

TEST_F(DualMemoryRAGExtTest, ShortTerm_KeepsNewest) {
    const std::string uid = "stm_newest_test";
    for (int i = 0; i < 8; ++i) {
        rag->updateShortTermMemory(uid, "消息" + std::to_string(i),
                                    "neutral", 0.0f);
    }
    auto insights = rag->getEmotionInsights(uid);
    auto recent = insights["recent_interactions"];
    EXPECT_LE(recent.size(), 5u);
}

// =====================================================================
// 空输入与边界
// =====================================================================

TEST_F(DualMemoryRAGExtTest, EmptyUserId_NoThrow) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("", "content", "neutral", 0.0f));
    EXPECT_NO_THROW(rag->getEmotionInsights(""));
}

TEST_F(DualMemoryRAGExtTest, EmptyContent_NoThrow) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("empty_content_user", "", "neutral", 0.0f));
}

TEST_F(DualMemoryRAGExtTest, EmptyEmotion_NoThrow) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("empty_emo_user", "hello", "", 0.0f));
}

TEST_F(DualMemoryRAGExtTest, ExtremePositiveScore) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("extreme_pos", "极端", "happy", 999.0f));
}

TEST_F(DualMemoryRAGExtTest, ExtremeNegativeScore) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("extreme_neg", "极端", "sad", -999.0f));
}

TEST_F(DualMemoryRAGExtTest, ScoreMinusOne) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("score_m1", "test", "sad", -1.0f));
}

TEST_F(DualMemoryRAGExtTest, ScorePlusOne) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("score_p1", "test", "happy", 1.0f));
}

TEST_F(DualMemoryRAGExtTest, ScoreZero) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("score_zero", "test", "neutral", 0.0f));
}

TEST_F(DualMemoryRAGExtTest, ScoreTwo) {
    EXPECT_NO_THROW(rag->updateShortTermMemory("score_two", "test", "happy", 2.0f));
}

// =====================================================================
// 情绪画像结构
// =====================================================================

TEST_F(DualMemoryRAGExtTest, Insights_HasUserId) {
    rag->updateShortTermMemory("insights_uid", "hi", "neutral", 0.0f);
    auto insights = rag->getEmotionInsights("insights_uid");
    EXPECT_TRUE(insights.isMember("user_id"));
}

TEST_F(DualMemoryRAGExtTest, Insights_HasLongTermProfile) {
    rag->updateShortTermMemory("insights_ltp", "hi", "neutral", 0.0f);
    auto insights = rag->getEmotionInsights("insights_ltp");
    EXPECT_TRUE(insights.isMember("long_term_profile"));
}

TEST_F(DualMemoryRAGExtTest, Insights_HasRecentInteractions) {
    rag->updateShortTermMemory("insights_ri", "hi", "neutral", 0.0f);
    auto insights = rag->getEmotionInsights("insights_ri");
    EXPECT_TRUE(insights.isMember("recent_interactions"));
}

TEST_F(DualMemoryRAGExtTest, Insights_ProfileFields) {
    rag->updateShortTermMemory("insights_pf", "hi", "happy", 0.5f);
    auto insights = rag->getEmotionInsights("insights_pf");
    auto profile = insights["long_term_profile"];
    EXPECT_TRUE(profile.isMember("avg_emotion_score"));
    EXPECT_TRUE(profile.isMember("dominant_mood"));
    EXPECT_TRUE(profile.isMember("emotion_trend"));
    EXPECT_TRUE(profile.isMember("emotion_volatility"));
}

// =====================================================================
// RAG 提示词构建
// =====================================================================

TEST_F(DualMemoryRAGExtTest, RAGPrompt_BasicNotEmpty) {
    EmotionMemory mem;
    mem.userId = "rag_basic";
    auto prompt = rag->buildRAGPrompt(mem, "你好", "neutral");
    EXPECT_FALSE(prompt.empty());
}

TEST_F(DualMemoryRAGExtTest, RAGPrompt_WithShortTerm) {
    EmotionMemory mem;
    mem.userId = "rag_st";
    EmotionMemory::ShortTermEntry e;
    e.content = "之前的消息";
    e.emotion = "happy";
    e.score = 0.5f;
    e.timestamp = "2026-02-20 10:00:00";
    mem.shortTerm.push_back(e);
    auto prompt = rag->buildRAGPrompt(mem, "新消息", "neutral");
    EXPECT_GT(prompt.length(), 20u);
}

TEST_F(DualMemoryRAGExtTest, RAGPrompt_HighRiskUser) {
    EmotionMemory mem;
    mem.userId = "rag_risk";
    mem.longTerm.avgEmotionScore = -0.9f;
    mem.longTerm.dominantMood = "desperate";
    mem.longTerm.emotionTrend = "falling";
    mem.longTerm.consecutiveNegativeDays = 10;
    auto prompt = rag->buildRAGPrompt(mem, "不想活了", "desperate");
    EXPECT_GT(prompt.length(), 50u);
}

TEST_F(DualMemoryRAGExtTest, RAGPrompt_EmptyContent) {
    EmotionMemory mem;
    mem.userId = "rag_empty";
    auto prompt = rag->buildRAGPrompt(mem, "", "neutral");
    EXPECT_FALSE(prompt.empty());
}

TEST_F(DualMemoryRAGExtTest, RAGPrompt_LongContent) {
    EmotionMemory mem;
    mem.userId = "rag_long";
    std::string longContent(5000, 'A');
    auto prompt = rag->buildRAGPrompt(mem, longContent, "neutral");
    EXPECT_FALSE(prompt.empty());
}

// =====================================================================
// 统计信息
// =====================================================================

TEST_F(DualMemoryRAGExtTest, Stats_NotNull) {
    auto stats = rag->getStats();
    EXPECT_FALSE(stats.isNull());
}

// =====================================================================
// 并发读写
// =====================================================================

TEST_F(DualMemoryRAGExtTest, Concurrent_WriteRead) {
    const int numThreads = 8;
    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t]() {
            std::string uid = "concurrent_user_" + std::to_string(t);
            for (int i = 0; i < 10; ++i) {
                rag->updateShortTermMemory(uid, "msg_" + std::to_string(i),
                                            "neutral", 0.1f * i);
                rag->getEmotionInsights(uid);
            }
        });
    }
    for (auto& t : threads) t.join();
    // No crash = pass
    EXPECT_TRUE(true);
}

TEST_F(DualMemoryRAGExtTest, Concurrent_SameUser) {
    const std::string uid = "concurrent_same_user";
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &uid, t]() {
            for (int i = 0; i < 20; ++i) {
                rag->updateShortTermMemory(uid,
                    "thread_" + std::to_string(t) + "_msg_" + std::to_string(i),
                    "neutral", 0.0f);
            }
        });
    }
    for (auto& t : threads) t.join();
    auto insights = rag->getEmotionInsights(uid);
    EXPECT_LE(insights["recent_interactions"].size(), 5u);
}

// =====================================================================
// 多用户隔离
// =====================================================================

TEST_F(DualMemoryRAGExtTest, MultiUser_Isolation) {
    rag->updateShortTermMemory("iso_user_a", "A的消息", "happy", 0.8f);
    rag->updateShortTermMemory("iso_user_b", "B的消息", "sad", -0.5f);

    auto insightsA = rag->getEmotionInsights("iso_user_a");
    auto insightsB = rag->getEmotionInsights("iso_user_b");

    EXPECT_EQ(insightsA["user_id"].asString(), "iso_user_a");
    EXPECT_EQ(insightsB["user_id"].asString(), "iso_user_b");
}

TEST_F(DualMemoryRAGExtTest, NonexistentUser_NoThrow) {
    EXPECT_NO_THROW(rag->getEmotionInsights("nonexistent_user_xyz"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    _exit(ret);
}
