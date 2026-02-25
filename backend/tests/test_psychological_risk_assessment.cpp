/**
 * @file test_psychological_risk_assessment.cpp
 * @brief PsychologicalRiskAssessment 单元测试
 */

#include <gtest/gtest.h>
#include "utils/PsychologicalRiskAssessment.h"

using namespace heartlake::utils;

class PsychologicalRiskAssessmentTest : public ::testing::Test {
protected:
    PsychologicalRiskAssessment* assessor;

    void SetUp() override {
        assessor = &PsychologicalRiskAssessment::getInstance();
    }
};

// ==================== 风险等级判定测试 ====================

TEST_F(PsychologicalRiskAssessmentTest, NoRisk_NormalContent) {
    auto result = assessor->assessRisk("今天天气真好，心情不错", "test_user", 0.5f, "happy");
    EXPECT_EQ(result.riskLevel, RiskLevel::NONE);
    EXPECT_FALSE(result.needsImmediateAttention);
}

TEST_F(PsychologicalRiskAssessmentTest, LowRisk_SlightNegative) {
    auto result = assessor->assessRisk("有点累，想休息一下", "test_user", -0.2f, "tired");
    EXPECT_LE(static_cast<int>(result.riskLevel), static_cast<int>(RiskLevel::LOW));
}

TEST_F(PsychologicalRiskAssessmentTest, HighRisk_SelfHarmKeyword) {
    auto result = assessor->assessRisk("我不想活了", "test_user", -0.8f, "sad");
    EXPECT_GE(static_cast<int>(result.riskLevel), static_cast<int>(RiskLevel::HIGH));
    EXPECT_TRUE(result.needsImmediateAttention);
}

TEST_F(PsychologicalRiskAssessmentTest, CriticalRisk_MultipleSelfHarmKeywords) {
    auto result = assessor->assessRisk("今晚我要结束生命，再见了", "test_user", -0.9f, "desperate");
    EXPECT_EQ(result.riskLevel, RiskLevel::CRITICAL);
    EXPECT_TRUE(result.needsImmediateAttention);
}

// ==================== 关键词检测测试 ====================

TEST_F(PsychologicalRiskAssessmentTest, DetectSelfHarmKeywords) {
    auto result = assessor->assessRisk("想自杀", "test_user", -0.8f, "sad");
    EXPECT_FALSE(result.keywords.empty());
    EXPECT_TRUE(result.primaryConcern == "self_harm");
}

TEST_F(PsychologicalRiskAssessmentTest, DetectHopelessnessKeywords) {
    auto result = assessor->assessRisk("感到绝望，看不到未来", "test_user", -0.6f, "sad");
    EXPECT_GE(static_cast<int>(result.riskLevel), static_cast<int>(RiskLevel::MEDIUM));
}

TEST_F(PsychologicalRiskAssessmentTest, DetectIsolationKeywords) {
    auto result = assessor->assessRisk("好孤独，没人理解我", "test_user", -0.4f, "lonely");
    EXPECT_GT(result.overallScore, 0.0f);
}

TEST_F(PsychologicalRiskAssessmentTest, DetectUrgencyKeywords) {
    auto result = assessor->assessRisk("今晚就是最后一次", "test_user", -0.7f, "sad");
    EXPECT_GE(static_cast<int>(result.riskLevel), static_cast<int>(RiskLevel::MEDIUM));
}

// ==================== 边界情况测试 ====================

TEST_F(PsychologicalRiskAssessmentTest, EmptyText) {
    auto result = assessor->assessRisk("", "test_user", 0.0f, "neutral");
    EXPECT_EQ(result.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychologicalRiskAssessmentTest, NeutralContent) {
    auto result = assessor->assessRisk("分享一首歌给大家", "test_user", 0.3f, "calm");
    EXPECT_EQ(result.riskLevel, RiskLevel::NONE);
}

TEST_F(PsychologicalRiskAssessmentTest, MultipleKeywordsCombined) {
    auto result = assessor->assessRisk("我很孤独，感到绝望，不想活了", "test_user", -0.9f, "desperate");
    EXPECT_EQ(result.riskLevel, RiskLevel::CRITICAL);
    EXPECT_TRUE(result.needsImmediateAttention);
}

// ==================== 干预建议测试 ====================

TEST_F(PsychologicalRiskAssessmentTest, CriticalRisk_HasInterventions) {
    auto result = assessor->assessRisk("我要自杀", "test_user", -0.9f, "desperate");
    EXPECT_FALSE(result.interventions.empty());
    EXPECT_FALSE(result.supportMessage.empty());
}

TEST_F(PsychologicalRiskAssessmentTest, SupportMessage_IsWarm) {
    auto result = assessor->assessRisk("我不想活了", "test_user", -0.8f, "sad");
    EXPECT_TRUE(result.supportMessage.find("陪") != std::string::npos ||
                result.supportMessage.find("帮助") != std::string::npos);
}

// ==================== 辅助函数测试 ====================

TEST_F(PsychologicalRiskAssessmentTest, RiskLevelDescription) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel::NONE), "无风险");
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelDescription(RiskLevel::CRITICAL), "危急");
}

TEST_F(PsychologicalRiskAssessmentTest, RiskLevelColor) {
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel::NONE), "#4CAF50");
    EXPECT_EQ(PsychologicalRiskAssessment::getRiskLevelColor(RiskLevel::CRITICAL), "#F44336");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
