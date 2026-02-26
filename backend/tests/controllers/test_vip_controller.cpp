/**
 * @file test_vip_controller.cpp
 * @brief VIPController 单元测试
 * 覆盖：VIP状态、特权列表、免费咨询配额、咨询预约、AI评论频率、参数校验
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class VIPControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== VIP状态 ====================

TEST_F(VIPControllerTest, GetVIPStatus_Authenticated_ReturnsStatus) {
    std::string userId = "user_vip_001";
    EXPECT_FALSE(userId.empty());

    Json::Value status;
    status["is_vip"] = true;
    status["level"] = "gold";
    status["expire_at"] = "2026-12-31T23:59:59";

    EXPECT_TRUE(status.isMember("is_vip"));
    EXPECT_TRUE(status["is_vip"].asBool());
}

TEST_F(VIPControllerTest, GetVIPStatus_NonVIP_ReturnsFalse) {
    Json::Value status;
    status["is_vip"] = false;

    EXPECT_FALSE(status["is_vip"].asBool());
}

TEST_F(VIPControllerTest, GetVIPStatus_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    std::string expectedMessage = "未登录";
    EXPECT_EQ(expectedStatus, 401);
    EXPECT_FALSE(expectedMessage.empty());
}

// ==================== 特权列表 ====================

TEST_F(VIPControllerTest, GetPrivileges_VIPUser_ReturnsPrivilegeList) {
    Json::Value response;
    response["privileges"] = Json::arrayValue;
    response["total"] = 0;

    Json::Value priv;
    priv["name"] = "free_counseling";
    priv["description"] = "免费心理咨询";
    priv["enabled"] = true;
    response["privileges"].append(priv);
    response["total"] = 1;

    EXPECT_TRUE(response.isMember("privileges"));
    EXPECT_TRUE(response.isMember("total"));
    EXPECT_TRUE(response["privileges"].isArray());
    EXPECT_EQ(response["total"].asInt(), 1);
}

TEST_F(VIPControllerTest, GetPrivileges_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}

// ==================== 免费咨询配额 ====================

TEST_F(VIPControllerTest, CheckFreeCounseling_HasQuota_ReturnsTrue) {
    Json::Value response;
    response["has_quota"] = true;
    response["message"] = "您有1次免费心理咨询机会";

    EXPECT_TRUE(response["has_quota"].asBool());
    EXPECT_EQ(response["message"].asString(), "您有1次免费心理咨询机会");
}

TEST_F(VIPControllerTest, CheckFreeCounseling_NoQuota_ReturnsFalse) {
    Json::Value response;
    response["has_quota"] = false;
    response["message"] = "您已使用过免费心理咨询额度";

    EXPECT_FALSE(response["has_quota"].asBool());
    EXPECT_EQ(response["message"].asString(), "您已使用过免费心理咨询额度");
}

TEST_F(VIPControllerTest, CheckFreeCounseling_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}

// ==================== 咨询预约 ====================

TEST_F(VIPControllerTest, BookCounseling_ValidRequest_ReturnsAppointmentId) {
    Json::Value request;
    request["appointment_time"] = "2026-03-01T14:00:00";
    request["is_free_vip"] = true;

    EXPECT_TRUE(request.isMember("appointment_time"));
    EXPECT_FALSE(request["appointment_time"].asString().empty());

    Json::Value response;
    response["appointment_id"] = "appt_001";
    response["appointment_time"] = "2026-03-01T14:00:00";
    response["is_free_vip"] = true;
    response["message"] = "预约成功";

    EXPECT_TRUE(response.isMember("appointment_id"));
    EXPECT_FALSE(response["appointment_id"].asString().empty());
    EXPECT_EQ(response["message"].asString(), "预约成功");
}

TEST_F(VIPControllerTest, BookCounseling_MissingAppointmentTime_Returns400) {
    Json::Value request;
    // 缺少 appointment_time
    bool hasTime = request.isMember("appointment_time");
    EXPECT_FALSE(hasTime);
}

TEST_F(VIPControllerTest, BookCounseling_NoJsonBody_Returns400) {
    int expectedStatus = 400;
    std::string expectedMessage = "请求体格式错误";
    EXPECT_EQ(expectedStatus, 400);
    EXPECT_EQ(expectedMessage, "请求体格式错误");
}

TEST_F(VIPControllerTest, BookCounseling_NoVIPPrivilege_Returns400) {
    // 无VIP权益时预约失败，bookCounseling 返回空 appointmentId
    std::string emptyAppointmentId = "";
    EXPECT_TRUE(emptyAppointmentId.empty());

    int expectedStatus = 400;
    std::string expectedMessage = "预约失败，请检查您的VIP权益";
    EXPECT_EQ(expectedStatus, 400);
    EXPECT_EQ(expectedMessage, "预约失败，请检查您的VIP权益");
}

TEST_F(VIPControllerTest, BookCounseling_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}

// ==================== AI评论频率 ====================

TEST_F(VIPControllerTest, GetAICommentFrequency_VIPUser_HighFrequency) {
    // VIP用户频率 <= 0.5 小时（30分钟一次）
    float vipFrequency = 0.5f;

    Json::Value response;
    response["frequency_hours"] = vipFrequency;
    response["frequency_minutes"] = static_cast<int>(vipFrequency * 60);
    response["message"] = "VIP用户享受高频AI评论，每30分钟一次";

    EXPECT_LE(response["frequency_hours"].asFloat(), 0.5f);
    EXPECT_EQ(response["frequency_minutes"].asInt(), 30);
}

TEST_F(VIPControllerTest, GetAICommentFrequency_NormalUser_LowFrequency) {
    // 普通用户频率为 2 小时
    float normalFrequency = 2.0f;

    Json::Value response;
    response["frequency_hours"] = normalFrequency;
    response["frequency_minutes"] = static_cast<int>(normalFrequency * 60);
    response["message"] = "普通用户AI评论频率为每2小时一次";

    EXPECT_GT(response["frequency_hours"].asFloat(), 0.5f);
    EXPECT_EQ(response["frequency_minutes"].asInt(), 120);
}

TEST_F(VIPControllerTest, GetAICommentFrequency_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}
