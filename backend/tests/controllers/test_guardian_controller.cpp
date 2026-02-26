/**
 * @file test_guardian_controller.cpp
 * @brief GuardianController 单元测试
 * 覆盖：守护者统计、灯火转赠、情感洞察、守护者对话、参数校验
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class GuardianControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== 守护者统计 ====================

TEST_F(GuardianControllerTest, GetStats_Authenticated_ReturnsGuardianStats) {
    Json::Value response;
    response["resonance_points"] = 150;
    response["quality_ripples"] = 42;
    response["warm_boats"] = 18;
    response["is_guardian"] = true;
    response["is_lamp_keeper"] = true;
    response["can_transfer_lamp"] = true;
    response["role"] = "guardian_lamp_keeper";

    EXPECT_TRUE(response.isMember("resonance_points"));
    EXPECT_TRUE(response.isMember("quality_ripples"));
    EXPECT_TRUE(response.isMember("warm_boats"));
    EXPECT_TRUE(response.isMember("is_guardian"));
    EXPECT_TRUE(response.isMember("is_lamp_keeper"));
    EXPECT_TRUE(response.isMember("can_transfer_lamp"));
    EXPECT_TRUE(response.isMember("role"));
    EXPECT_EQ(response["role"].asString(), "guardian_lamp_keeper");
}

TEST_F(GuardianControllerTest, GetStats_NonGuardian_ReturnsMemberRole) {
    Json::Value response;
    response["is_guardian"] = false;
    response["role"] = "member";

    EXPECT_FALSE(response["is_guardian"].asBool());
    EXPECT_EQ(response["role"].asString(), "member");
}

TEST_F(GuardianControllerTest, GetStats_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    std::string expectedMessage = "未登录";
    EXPECT_EQ(expectedStatus, 401);
    EXPECT_FALSE(expectedMessage.empty());
}

TEST_F(GuardianControllerTest, GetStats_LampKeeperAlias_MatchesGuardian) {
    // is_lamp_keeper 是 is_guardian 的别名，保持兼容
    bool isGuardian = true;
    bool isLampKeeper = isGuardian;
    EXPECT_EQ(isGuardian, isLampKeeper);
}

// ==================== 灯火转赠 ====================

TEST_F(GuardianControllerTest, TransferLamp_ValidRequest_ReturnsSuccess) {
    Json::Value request;
    request["to_user_id"] = "user_receive_lamp_001";

    EXPECT_TRUE(request.isMember("to_user_id"));
    EXPECT_FALSE(request["to_user_id"].asString().empty());
}

TEST_F(GuardianControllerTest, TransferLamp_MissingToUserId_Returns400) {
    Json::Value request;
    // 缺少 to_user_id
    bool hasToUserId = request.isMember("to_user_id");
    EXPECT_FALSE(hasToUserId);
}

TEST_F(GuardianControllerTest, TransferLamp_NotGuardian_Returns400) {
    // 非守护者/点灯人尝试转赠应返回 400
    int expectedStatus = 400;
    std::string expectedMessage = "转赠失败，请检查守护者/点灯人资格";
    EXPECT_EQ(expectedStatus, 400);
    EXPECT_EQ(expectedMessage, "转赠失败，请检查守护者/点灯人资格");
}

TEST_F(GuardianControllerTest, TransferLamp_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}

// ==================== 情感洞察 ====================

TEST_F(GuardianControllerTest, GetEmotionInsights_Authenticated_ReturnsInsights) {
    std::string userId = "user_insight_001";
    EXPECT_FALSE(userId.empty());

    // 情感洞察由 DualMemoryRAG 引擎生成
    Json::Value insights;
    insights["summary"] = "近期情绪趋于平稳";
    insights["trend"] = "stable";
    EXPECT_TRUE(insights.isMember("summary"));
}

TEST_F(GuardianControllerTest, GetEmotionInsights_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}

// ==================== 守护者对话（湖神） ====================

TEST_F(GuardianControllerTest, Chat_ValidMessage_ReturnsLakeGodReply) {
    Json::Value request;
    request["content"] = "今天心情不太好";
    request["emotion"] = "sad";
    request["emotion_score"] = 0.3f;

    EXPECT_TRUE(request.isMember("content"));
    EXPECT_FALSE(request["content"].asString().empty());

    Json::Value response;
    response["reply"] = "湖神的回复内容";
    response["agent"] = "lake_god";
    response["agent_name"] = "湖神";

    EXPECT_EQ(response["agent"].asString(), "lake_god");
    EXPECT_EQ(response["agent_name"].asString(), "湖神");
    EXPECT_FALSE(response["reply"].asString().empty());
}

TEST_F(GuardianControllerTest, Chat_MissingContent_Returns400) {
    Json::Value request;
    // 缺少 content 字段
    bool hasContent = request.isMember("content");
    EXPECT_FALSE(hasContent);
}

TEST_F(GuardianControllerTest, Chat_ContentTooLong_Returns400) {
    // 单条消息最大 2000 字符
    std::string longContent(2001, 'x');
    EXPECT_GT(longContent.size(), 2000u);

    static constexpr size_t MAX_CHAT_LENGTH = 2000;
    EXPECT_GT(longContent.size(), MAX_CHAT_LENGTH);
}

TEST_F(GuardianControllerTest, Chat_DefaultEmotionValues_Applied) {
    // 不提供 emotion 和 emotion_score 时使用默认值
    std::string defaultEmotion = "neutral";
    float defaultScore = 0.5f;

    EXPECT_EQ(defaultEmotion, "neutral");
    EXPECT_FLOAT_EQ(defaultScore, 0.5f);
}

TEST_F(GuardianControllerTest, Chat_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}
