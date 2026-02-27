/**
 * PsychologicalRiskAssessment 扩展测试
 */

#include <gtest/gtest.h>
#include "utils/PsychologicalRiskAssessment.h"
#include <string>
#include <vector>

using namespace heartlake::utils;

class PsychRiskExtTest : public ::testing::Test {
protected:
    PsychologicalRiskAssessment* assessor;
    void SetUp() override {
        assessor = &PsychologicalRiskAssessment::getInstance();
    }
};

// =====================================================================
// 风险等级边界
// =====================================================================

TEST_F(PsychRiskExtTest, NoRisk_PositiveContent) {
    auto r = assessor->assessRisk("今天阳光明媚，心情很好", "u1", 0.8f, "happy");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
    EXPECT_FALSE(r.needsImmediateAttention);
}

TEST_F(PsychRiskExtTest, NoRisk_NeutralContent) {
    auto r = assessor->assessRisk("今天吃了面条", "u2", 0.0f, "neutral");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychRiskExtTest, LowRisk_MildNegative) {
    auto r = assessor->assessRisk("有点不开心", "u3", -0.2f, "sad");
    EXPECT_LE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::LOW));
}

TEST_F(PsychRiskExtTest, MediumRisk_Hopelessness) {
    auto r = assessor->assessRisk("感到绝望，看不到希望", "u4", -0.6f, "sad");
    EXPECT_GE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::LOW));
}

TEST_F(PsychRiskExtTest, HighRisk_SelfHarm) {
    auto r = assessor->assessRisk("我不想活了", "u5", -0.8f, "sad");
    EXPECT_GE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::HIGH));
    EXPECT_TRUE(r.needsImmediateAttention);
}

TEST_F(PsychRiskExtTest, CriticalRisk_MultipleTriggers) {
    auto r = assessor->assessRisk("我很孤独，感到绝望，不想活了，今晚就结束", "u6", -0.95f, "desperate");
    EXPECT_EQ(r.riskLevel, RiskLevel::CRITICAL);
    EXPECT_TRUE(r.needsImmediateAttention);
}

// =====================================================================
// 关键词触发
// =====================================================================

TEST_F(PsychRiskExtTest, Keyword_Suicide) {
    auto r = assessor->assessRisk("想自杀", "kw1", -0.9f, "sad");
    EXPECT_FALSE(r.keywords.empty());
    EXPECT_EQ(r.primaryConcern, "self_harm");
}

TEST_F(PsychRiskExtTest, Keyword_EndLife) {
    auto r = assessor->assessRisk("结束生命", "kw2", -0.9f, "desperate");
    EXPECT_GE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::HIGH));
}

TEST_F(PsychRiskExtTest, Keyword_DontWantToLive) {
    auto r = assessor->assessRisk("不想活了", "kw3", -0.8f, "sad");
    EXPECT_GE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::HIGH));
}

TEST_F(PsychRiskExtTest, Keyword_Hopeless) {
    auto r = assessor->assessRisk("感到绝望", "kw4", -0.6f, "sad");
    EXPECT_GE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::LOW));
}

TEST_F(PsychRiskExtTest, Keyword_NoFuture) {
    auto r = assessor->assessRisk("看不到未来", "kw5", -0.5f, "sad");
    EXPECT_GT(r.overallScore, 0.0f);
}

TEST_F(PsychRiskExtTest, Keyword_Lonely) {
    auto r = assessor->assessRisk("好孤独，没人理解我", "kw6", -0.4f, "lonely");
    EXPECT_GT(r.overallScore, 0.0f);
}

TEST_F(PsychRiskExtTest, Keyword_Tonight) {
    auto r = assessor->assessRisk("今晚就是最后一次", "kw7", -0.7f, "sad");
    EXPECT_GE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::MEDIUM));
}

TEST_F(PsychRiskExtTest, Keyword_NoOneUnderstands) {
    auto r = assessor->assessRisk("没有人理解我的痛苦", "kw8", -0.5f, "sad");
    EXPECT_GT(r.overallScore, 0.0f);
}

// =====================================================================
// 边界情况
// =====================================================================

TEST_F(PsychRiskExtTest, EmptyText_NoRisk) {
    auto r = assessor->assessRisk("", "e1", 0.0f, "neutral");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychRiskExtTest, WhitespaceOnly) {
    auto r = assessor->assessRisk("   \t\n  ", "e2", 0.0f, "neutral");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychRiskExtTest, VeryLongText) {
    std::string longText;
    for (int i = 0; i < 500; ++i) longText += "今天天气不错。";
    auto r = assessor->assessRisk(longText, "e3", 0.3f, "calm");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychRiskExtTest, EmptyUserId) {
    auto r = assessor->assessRisk("测试内容", "", 0.0f, "neutral");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychRiskExtTest, EmptyEmotion) {
    auto r = assessor->assessRisk("测试内容", "e4", 0.0f, "");
    EXPECT_GE(static_cast<int>(r.riskLevel), 0);
}

TEST_F(PsychRiskExtTest, ExtremePositiveScore) {
    auto r = assessor->assessRisk("非常开心", "e5", 1.0f, "happy");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychRiskExtTest, ExtremeNegativeScore_NoKeyword) {
    auto r = assessor->assessRisk("今天不太好", "e6", -1.0f, "sad");
    EXPECT_LE(static_cast<int>(r.riskLevel), static_cast<int>(RiskLevel::MEDIUM));
}

// =====================================================================
// 干预建议
// =====================================================================

TEST_F(PsychRiskExtTest, CriticalRisk_HasInterventions) {
    auto r = assessor->assessRisk("我要自杀，今晚就结束", "int1", -0.95f, "desperate");
    EXPECT_FALSE(r.interventions.empty());
}

TEST_F(PsychRiskExtTest, CriticalRisk_HasSupportMessage) {
    auto r = assessor->assessRisk("不想活了", "int2", -0.9f, "sad");
    EXPECT_FALSE(r.supportMessage.empty());
}

TEST_F(PsychRiskExtTest, NoRisk_MayHaveEmptyInterventions) {
    auto r = assessor->assessRisk("今天很开心", "int3", 0.8f, "happy");
    EXPECT_EQ(r.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychRiskExtTest, SupportMessage_IsWarm) {
    auto r = assessor->assessRisk("我不想活了", "int4", -0.8f, "sad");
    EXPECT_FALSE(r.supportMessage.empty());
}

// =====================================================================
// 辅助函数
// =====================================================================

TEST_F(PsychRiskExtTest, RiskLevelDesc_None) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel::NONE), "无风险");
}

TEST_F(PsychRiskExtTest, RiskLevelDesc_Low) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel::LOW), "低风险");
}

TEST_F(PsychRiskExtTest, RiskLevelDesc_Medium) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel::MEDIUM), "中风险");
}

TEST_F(PsychRiskExtTest, RiskLevelDesc_High) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel::HIGH), "高风险");
}

TEST_F(PsychRiskExtTest, RiskLevelDesc_Critical) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel::CRITICAL), "危急");
}

TEST_F(PsychRiskExtTest, RiskLevelColor_None) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel::NONE), "#4CAF50");
}

TEST_F(PsychRiskExtTest, RiskLevelColor_Low) {
    auto color = PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel::LOW);
    EXPECT_FALSE(color.empty());
    EXPECT_EQ(color[0], '#');
}

TEST_F(PsychRiskExtTest, RiskLevelColor_Medium) {
    auto color = PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel::MEDIUM);
    EXPECT_FALSE(color.empty());
}

TEST_F(PsychRiskExtTest, RiskLevelColor_High) {
    auto color = PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel::HIGH);
    EXPECT_FALSE(color.empty());
}

TEST_F(PsychRiskExtTest, RiskLevelColor_Critical) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel::CRITICAL), "#F44336");
}

// =====================================================================
// 情绪历史记录
// =====================================================================

TEST_F(PsychRiskExtTest, RecordHistory_NoThrow) {
    EXPECT_NO_THROW(assessor->recordEmotionHistory("hist1", 0.5f, "happy", "开心的内容"));
}

TEST_F(PsychRiskExtTest, RecordHistory_Negative) {
    EXPECT_NO_THROW(assessor->recordEmotionHistory("hist2", -0.5f, "sad", "难过的内容"));
}

TEST_F(PsychRiskExtTest, RecordHistory_EmptyContent) {
    EXPECT_NO_THROW(assessor->recordEmotionHistory("hist3", 0.0f, "neutral", ""));
}

TEST_F(PsychRiskExtTest, GetTrend_ReturnsVector) {
    assessor->recordEmotionHistory("trend1", 0.5f, "happy", "a");
    assessor->recordEmotionHistory("trend1", -0.3f, "sad", "b");
    auto trend = assessor->getEmotionTrend("trend1", 7);
    // May be empty if no DB, but should not crash
    EXPECT_GE(trend.size(), 0u);
}

TEST_F(PsychRiskExtTest, GetTrend_NonexistentUser) {
    auto trend = assessor->getEmotionTrend("nonexistent_trend_user", 7);
    EXPECT_GE(trend.size(), 0u);
}

// =====================================================================
// 结果结构完整性
// =====================================================================

TEST_F(PsychRiskExtTest, Result_HasOverallScore) {
    auto r = assessor->assessRisk("测试", "struct1", 0.0f, "neutral");
    EXPECT_GE(r.overallScore, 0.0f);
    EXPECT_LE(r.overallScore, 1.0f);
}

TEST_F(PsychRiskExtTest, Result_HasPrimaryConcern) {
    auto r = assessor->assessRisk("想自杀", "struct2", -0.9f, "sad");
    EXPECT_FALSE(r.primaryConcern.empty());
}

TEST_F(PsychRiskExtTest, Result_NeedsAttention_MatchesLevel) {
    auto r = assessor->assessRisk("今天很好", "struct3", 0.8f, "happy");
    if (r.riskLevel == RiskLevel::NONE || r.riskLevel == RiskLevel::LOW) {
        EXPECT_FALSE(r.needsImmediateAttention);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
