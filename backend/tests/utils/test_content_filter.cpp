/**
 * @file test_content_filter.cpp
 * @brief ContentFilter 单元测试 - AC自动机多模式匹配、分级过滤、白名单
 */

#include <gtest/gtest.h>
#include "utils/ContentFilter.h"
#include <string>
#include <vector>

using namespace heartlake;

// =====================================================================
// ContentFilter 测试
// =====================================================================

class ContentFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& filter = ContentFilter::getInstance();
        filter.initialize();
    }
};

// -------------------- 基本安全检测 --------------------

TEST_F(ContentFilterTest, SafeContent_ReturnsSafe) {
    auto result = ContentFilter::checkContentSafety("今天天气真好，心情不错");
    EXPECT_EQ(result, "safe");
}

TEST_F(ContentFilterTest, EmptyInput_ReturnsEmpty) {
    auto result = ContentFilter::checkContentSafety("");
    EXPECT_EQ(result, "empty");
}

// -------------------- 高危词检测 (level 3 → high_risk) --------------------

TEST_F(ContentFilterTest, HighRiskWord_SelfHarm_Detected) {
    auto result = ContentFilter::checkContentSafety("我想自杀");
    EXPECT_EQ(result, "high_risk");
}

TEST_F(ContentFilterTest, HighRiskWord_SelfInjury_Detected) {
    auto result = ContentFilter::checkContentSafety("我要自残");
    EXPECT_EQ(result, "high_risk");
}

TEST_F(ContentFilterTest, HighRiskWord_EndLife_Detected) {
    auto result = ContentFilter::checkContentSafety("想要结束生命");
    EXPECT_EQ(result, "high_risk");
}

TEST_F(ContentFilterTest, HighRiskWord_DontWantToLive_Detected) {
    auto result = ContentFilter::checkContentSafety("我不想活了");
    EXPECT_EQ(result, "high_risk");
}

TEST_F(ContentFilterTest, HighRiskWord_JumpBuilding_Detected) {
    auto result = ContentFilter::checkContentSafety("想跳楼");
    EXPECT_EQ(result, "high_risk");
}

// -------------------- 中危词检测 (level 2 → medium_risk) --------------------

TEST_F(ContentFilterTest, MediumRiskWord_Violence_Detected) {
    auto result = ContentFilter::checkContentSafety("我要杀人");
    EXPECT_EQ(result, "medium_risk");
}

TEST_F(ContentFilterTest, MediumRiskWord_Sexual_Detected) {
    auto result = ContentFilter::checkContentSafety("想约炮");
    EXPECT_EQ(result, "medium_risk");
}

TEST_F(ContentFilterTest, MediumRiskWord_NoMeaning_Detected) {
    // "没有意义" 是 level 2
    auto result = ContentFilter::checkContentSafety("活着没有意义");
    EXPECT_EQ(result, "medium_risk");
}

// -------------------- 白名单放行 --------------------

TEST_F(ContentFilterTest, Whitelist_KillVirus_PassedThrough) {
    // "杀毒" 在白名单中，虽然包含 "杀" 但不应被拦截
    auto result = ContentFilter::checkContentSafety("电脑需要杀毒软件");
    EXPECT_EQ(result, "safe");
}

TEST_F(ContentFilterTest, Whitelist_Bargain_PassedThrough) {
    auto result = ContentFilter::checkContentSafety("这个价格可以杀价");
    EXPECT_EQ(result, "safe");
}

TEST_F(ContentFilterTest, Whitelist_FlashSale_PassedThrough) {
    auto result = ContentFilter::checkContentSafety("今天有秒杀活动");
    EXPECT_EQ(result, "safe");
}

// -------------------- AC自动机多模式匹配 --------------------

TEST_F(ContentFilterTest, MultipleMatches_HighestLevelWins) {
    // 同时包含 level 2 和 level 3 的词，应返回 high_risk
    auto result = ContentFilter::checkContentSafety("我想自杀，要杀人");
    EXPECT_EQ(result, "high_risk");
}

TEST_F(ContentFilterTest, GetMatchedWords_ReturnsAllMatches) {
    auto& filter = ContentFilter::getInstance();
    auto matches = filter.getMatchedWords("我想自杀和自残");
    // 至少匹配到 "自杀" 和 "自残"
    EXPECT_GE(matches.size(), 2u);
}

// -------------------- normalize 文本规范化 --------------------

TEST_F(ContentFilterTest, Normalize_RemovesSpaces) {
    auto normalized = ContentFilter::normalize("自 杀");
    EXPECT_EQ(normalized, "自杀");
}

TEST_F(ContentFilterTest, Normalize_RemovesAsterisks) {
    auto normalized = ContentFilter::normalize("自*杀");
    EXPECT_EQ(normalized, "自杀");
}

TEST_F(ContentFilterTest, Normalize_RemovesDots) {
    auto normalized = ContentFilter::normalize("自.杀");
    EXPECT_EQ(normalized, "自杀");
}

TEST_F(ContentFilterTest, Normalize_RemovesUnderscores) {
    auto normalized = ContentFilter::normalize("自_杀");
    EXPECT_EQ(normalized, "自杀");
}

TEST_F(ContentFilterTest, Normalize_RemovesDashes) {
    auto normalized = ContentFilter::normalize("自-杀");
    EXPECT_EQ(normalized, "自杀");
}

TEST_F(ContentFilterTest, Normalize_ObfuscatedWord_StillDetected) {
    // 用干扰字符混淆的敏感词，normalize后仍能检测
    auto result = ContentFilter::checkContentSafety("自*杀");
    EXPECT_EQ(result, "high_risk");
}

// -------------------- 超长文本处理 --------------------

TEST_F(ContentFilterTest, LongText_SafeContent_ReturnsSafe) {
    // 构造一段超长的安全文本
    std::string longText;
    for (int i = 0; i < 500; ++i) {
        longText += "今天天气真好，阳光明媚。";
    }
    auto result = ContentFilter::checkContentSafety(longText);
    EXPECT_EQ(result, "safe");
}

TEST_F(ContentFilterTest, LongText_WithSensitiveWord_Detected) {
    std::string longText;
    for (int i = 0; i < 500; ++i) {
        longText += "今天天气真好。";
    }
    longText += "我想自杀";
    for (int i = 0; i < 500; ++i) {
        longText += "阳光明媚。";
    }
    auto result = ContentFilter::checkContentSafety(longText);
    EXPECT_EQ(result, "high_risk");
}

// -------------------- containsHighRiskWords 静态方法 --------------------

TEST_F(ContentFilterTest, ContainsHighRiskWords_True) {
    EXPECT_TRUE(ContentFilter::containsHighRiskWords("我想自杀"));
}

TEST_F(ContentFilterTest, ContainsHighRiskWords_False_SafeContent) {
    EXPECT_FALSE(ContentFilter::containsHighRiskWords("今天心情不错"));
}

TEST_F(ContentFilterTest, ContainsHighRiskWords_False_MediumRisk) {
    // medium_risk 不算 high_risk
    EXPECT_FALSE(ContentFilter::containsHighRiskWords("杀人"));
}

// -------------------- 词库统计 --------------------

TEST_F(ContentFilterTest, WordCount_GreaterThanZero) {
    auto& filter = ContentFilter::getInstance();
    EXPECT_GT(filter.wordCount(), 0u);
}

// -------------------- getMentalHealthTip --------------------

TEST_F(ContentFilterTest, MentalHealthTip_ContainsHotline) {
    auto tip = ContentFilter::getMentalHealthTip();
    EXPECT_NE(tip.find("400-161-9995"), std::string::npos);
    EXPECT_NE(tip.find("心理援助"), std::string::npos);
}

// =====================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
