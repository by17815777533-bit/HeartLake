/**
 * @file safe_harbor_service_test.cpp
 * @brief SafeHarborService 单元测试
 */

#include <gtest/gtest.h>
#include "infrastructure/services/SafeHarborService.h"

using namespace heartlake::infrastructure;

class SafeHarborServiceTest : public ::testing::Test {
protected:
    SafeHarborService& service_ = SafeHarborService::getInstance();
};

TEST_F(SafeHarborServiceTest, GetHotlines_ReturnsNonEmpty) {
    auto hotlines = service_.getHotlines();
    EXPECT_TRUE(hotlines.isArray());
    EXPECT_GT(hotlines.size(), 0u);
}

TEST_F(SafeHarborServiceTest, GetHotlines_HasRequiredFields) {
    auto hotlines = service_.getHotlines();
    for (const auto& h : hotlines) {
        EXPECT_TRUE(h.isMember("name"));
        EXPECT_TRUE(h.isMember("phone"));
        EXPECT_TRUE(h.isMember("is_24_hours"));
    }
}

TEST_F(SafeHarborServiceTest, GetSelfHelpTools_ReturnsTools) {
    auto tools = service_.getSelfHelpTools();
    EXPECT_TRUE(tools.isArray());
    EXPECT_EQ(tools.size(), 3u);
}

TEST_F(SafeHarborServiceTest, GetWarmPrompt_HighRisk_ShowsHotline) {
    auto prompt = service_.getWarmPrompt("HIGH");
    EXPECT_TRUE(prompt["show_hotline"].asBool());
}

TEST_F(SafeHarborServiceTest, GetWarmPrompt_CriticalRisk_ShowsHotline) {
    auto prompt = service_.getWarmPrompt("CRITICAL");
    EXPECT_TRUE(prompt["show_hotline"].asBool());
}

TEST_F(SafeHarborServiceTest, GetWarmPrompt_LowRisk_HidesHotline) {
    auto prompt = service_.getWarmPrompt("LOW");
    EXPECT_FALSE(prompt["show_hotline"].asBool());
}
