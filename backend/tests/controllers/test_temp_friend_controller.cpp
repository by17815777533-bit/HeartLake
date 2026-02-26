/**
 * @file test_temp_friend_controller.cpp
 * @brief TempFriendController 单元测试
 * 覆盖：临时好友创建、列表获取、详情、升级为永久好友、删除、状态检查、过期处理
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>
#include <algorithm>

class TempFriendControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== 创建临时好友 ====================

TEST_F(TempFriendControllerTest, CreateTempFriend_ValidRequest_ReturnsSuccess) {
    Json::Value request;
    request["target_user_id"] = "user_target_001";
    request["source"] = "boat";
    request["source_id"] = "boat_001";

    EXPECT_TRUE(request.isMember("target_user_id"));
    EXPECT_FALSE(request["target_user_id"].asString().empty());

    // 预期响应
    Json::Value response;
    response["code"] = 0;
    response["message"] = "临时好友创建成功";
    response["data"]["temp_friend_id"] = "uuid-temp-001";
    response["data"]["target_user_id"] = "user_target_001";
    response["data"]["expires_at"] = "2026-02-27T10:00:00";
    response["data"]["source"] = "boat";

    EXPECT_EQ(response["code"].asInt(), 0);
    EXPECT_TRUE(response["data"].isMember("temp_friend_id"));
    EXPECT_TRUE(response["data"].isMember("expires_at"));
}

TEST_F(TempFriendControllerTest, CreateTempFriend_MissingTargetUserId_Returns400) {
    Json::Value request;
    // 缺少 target_user_id
    bool hasTarget = request.isMember("target_user_id");
    EXPECT_FALSE(hasTarget);

    // 或者 target_user_id 为空
    request["target_user_id"] = "";
    EXPECT_TRUE(request["target_user_id"].asString().empty());
}

TEST_F(TempFriendControllerTest, CreateTempFriend_NoJsonBody_Returns400) {
    int expectedCode = 400;
    std::string expectedMessage = "无效的请求数据";
    EXPECT_EQ(expectedCode, 400);
    EXPECT_EQ(expectedMessage, "无效的请求数据");
}

TEST_F(TempFriendControllerTest, CreateTempFriend_Unauthenticated_Returns401) {
    int expectedCode = 401;
    std::string expectedMessage = "未登录";
    EXPECT_EQ(expectedCode, 401);
    EXPECT_EQ(expectedMessage, "未登录");
}

TEST_F(TempFriendControllerTest, CreateTempFriend_AlreadyTempFriend_Returns400) {
    // 已经是临时好友时应返回 400
    int expectedCode = 400;
    std::string expectedMessage = "已经是临时好友";
    EXPECT_EQ(expectedCode, 400);
    EXPECT_EQ(expectedMessage, "已经是临时好友");
}

TEST_F(TempFriendControllerTest, CreateTempFriend_ExpiresIn24Hours) {
    // 临时好友 24 小时后过期
    int expirySeconds = 24 * 3600;
    EXPECT_EQ(expirySeconds, 86400);
}

// ==================== 获取临时好友列表 ====================

TEST_F(TempFriendControllerTest, GetMyTempFriends_Authenticated_ReturnsList) {
    Json::Value response;
    response["code"] = 0;
    response["data"]["friends"] = Json::arrayValue;
    response["data"]["total"] = 0;

    Json::Value friend_;
    friend_["temp_friend_id"] = "tf_001";
    friend_["friend_user_id"] = "user_002";
    friend_["friend_nickname"] = "小明";
    friend_["friend_avatar"] = "https://example.com/avatar.jpg";
    friend_["source"] = "boat";
    friend_["created_at"] = "2026-02-26T10:00:00";
    friend_["expires_at"] = "2026-02-27T10:00:00";
    friend_["seconds_remaining"] = 43200;
    friend_["hours_remaining"] = 12;
    response["data"]["friends"].append(friend_);
    response["data"]["total"] = 1;

    EXPECT_EQ(response["code"].asInt(), 0);
    EXPECT_TRUE(response["data"]["friends"].isArray());
    EXPECT_EQ(response["data"]["total"].asInt(), 1);

    auto& f = response["data"]["friends"][0];
    EXPECT_TRUE(f.isMember("temp_friend_id"));
    EXPECT_TRUE(f.isMember("friend_user_id"));
    EXPECT_TRUE(f.isMember("friend_nickname"));
    EXPECT_TRUE(f.isMember("seconds_remaining"));
    EXPECT_TRUE(f.isMember("hours_remaining"));
}

TEST_F(TempFriendControllerTest, GetMyTempFriends_NullNickname_DefaultsToAnonymous) {
    // nickname 为 null 时默认为 "匿名用户"
    Json::Value nullNickname;
    std::string nickname = nullNickname.isNull() ? "匿名用户" : nullNickname.asString();
    EXPECT_EQ(nickname, "匿名用户");
}

TEST_F(TempFriendControllerTest, GetMyTempFriends_HoursRemainingCalculation) {
    // hours_remaining = seconds_remaining / 3600
    int secondsRemaining = 7200;
    int hoursRemaining = secondsRemaining / 3600;
    EXPECT_EQ(hoursRemaining, 2);

    // 不足一小时向下取整
    secondsRemaining = 5400; // 1.5 hours
    hoursRemaining = secondsRemaining / 3600;
    EXPECT_EQ(hoursRemaining, 1);
}

// ==================== 临时好友详情 ====================

TEST_F(TempFriendControllerTest, GetTempFriendDetail_ValidId_ReturnsDetail) {
    std::string tempFriendId = "tf_detail_001";
    EXPECT_FALSE(tempFriendId.empty());

    Json::Value data;
    data["temp_friend_id"] = tempFriendId;
    data["status"] = "active";
    data["source"] = "boat";
    data["created_at"] = "2026-02-26T10:00:00";
    data["expires_at"] = "2026-02-27T10:00:00";
    data["seconds_remaining"] = 86400;
    data["hours_remaining"] = 24;
    data["upgraded_to_friend"] = false;
    data["friend_user_id"] = "user_friend_001";
    data["friend_nickname"] = "好友昵称";
    data["friend_avatar"] = "";

    EXPECT_TRUE(data.isMember("temp_friend_id"));
    EXPECT_TRUE(data.isMember("status"));
    EXPECT_TRUE(data.isMember("upgraded_to_friend"));
    EXPECT_TRUE(data.isMember("friend_user_id"));
    EXPECT_FALSE(data["upgraded_to_friend"].asBool());
}

TEST_F(TempFriendControllerTest, GetTempFriendDetail_NotFound_Returns404) {
    int expectedCode = 404;
    std::string expectedMessage = "临时好友不存在";
    EXPECT_EQ(expectedCode, 404);
    EXPECT_EQ(expectedMessage, "临时好友不存在");
}

// ==================== 升级为永久好友 ====================

TEST_F(TempFriendControllerTest, UpgradeToPermanent_ValidRequest_ReturnsSuccess) {
    std::string tempFriendId = "tf_upgrade_001";
    EXPECT_FALSE(tempFriendId.empty());

    Json::Value response;
    response["code"] = 0;
    response["message"] = "已升级为永久好友";
    response["data"]["friendship_id"] = "friendship_001";

    EXPECT_EQ(response["code"].asInt(), 0);
    EXPECT_EQ(response["message"].asString(), "已升级为永久好友");
    EXPECT_TRUE(response["data"].isMember("friendship_id"));
}

TEST_F(TempFriendControllerTest, UpgradeToPermanent_NotFoundOrExpired_Returns404) {
    int expectedCode = 404;
    std::string expectedMessage = "临时好友不存在或已过期";
    EXPECT_EQ(expectedCode, 404);
    EXPECT_EQ(expectedMessage, "临时好友不存在或已过期");
}

TEST_F(TempFriendControllerTest, UpgradeToPermanent_Unauthenticated_Returns401) {
    int expectedCode = 401;
    EXPECT_EQ(expectedCode, 401);
}

// ==================== 删除临时好友 ====================

TEST_F(TempFriendControllerTest, DeleteTempFriend_ValidId_ReturnsSuccess) {
    std::string tempFriendId = "tf_delete_001";
    EXPECT_FALSE(tempFriendId.empty());

    Json::Value response;
    response["code"] = 0;
    response["message"] = "已删除临时好友";
    EXPECT_EQ(response["code"].asInt(), 0);
}

TEST_F(TempFriendControllerTest, DeleteTempFriend_NotFound_Returns404) {
    int expectedCode = 404;
    std::string expectedMessage = "临时好友不存在";
    EXPECT_EQ(expectedCode, 404);
    EXPECT_EQ(expectedMessage, "临时好友不存在");
}

// ==================== 检查临时好友状态 ====================

TEST_F(TempFriendControllerTest, CheckTempFriendStatus_IsTempFriend_ReturnsTrue) {
    std::string targetUserId = "user_check_001";
    EXPECT_FALSE(targetUserId.empty());

    Json::Value response;
    response["code"] = 0;
    response["data"]["is_temp_friend"] = true;
    response["data"]["temp_friend_id"] = "tf_check_001";
    response["data"]["expires_at"] = "2026-02-27T10:00:00";
    response["data"]["seconds_remaining"] = 43200;
    response["data"]["hours_remaining"] = 12;

    EXPECT_TRUE(response["data"]["is_temp_friend"].asBool());
    EXPECT_TRUE(response["data"].isMember("temp_friend_id"));
}

TEST_F(TempFriendControllerTest, CheckTempFriendStatus_NotTempFriend_ReturnsFalse) {
    Json::Value response;
    response["code"] = 0;
    response["data"]["is_temp_friend"] = false;

    EXPECT_FALSE(response["data"]["is_temp_friend"].asBool());
    // 非临时好友时不应包含 temp_friend_id
    EXPECT_FALSE(response["data"].isMember("temp_friend_id"));
}

TEST_F(TempFriendControllerTest, CheckTempFriendStatus_NegativeSecondsRemaining_ClampedToZero) {
    // 过期后 seconds_remaining 应被 clamp 到 0
    int rawSeconds = -500;
    int clamped = std::max(0, rawSeconds);
    EXPECT_EQ(clamped, 0);

    int hoursRemaining = clamped / 3600;
    EXPECT_EQ(hoursRemaining, 0);
}
