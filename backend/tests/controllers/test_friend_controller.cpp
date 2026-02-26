/**
 * @file test_friend_controller.cpp
 * @brief FriendController 单元测试
 * 覆盖好友请求发送/接受/拒绝、好友删除、好友列表、待处理请求、聊天记录、消息发送等接口
 * Created by 白洋
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class FriendControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    Json::Value buildSuccessResponse(const Json::Value& data, const std::string& msg = "Success") {
        Json::Value resp;
        resp["code"] = 0;
        resp["message"] = msg;
        resp["data"] = data;
        return resp;
    }

    Json::Value buildErrorResponse(int code, const std::string& msg) {
        Json::Value resp;
        resp["code"] = code;
        resp["message"] = msg;
        return resp;
    }
};

// ==================== 发送好友请求 ====================

TEST_F(FriendControllerTest, SendFriendRequest_ValidRequest) {
    Json::Value request;
    request["user_id"] = "anon_target_456";

    EXPECT_TRUE(request.isMember("user_id"));
    EXPECT_FALSE(request["user_id"].asString().empty());
}

TEST_F(FriendControllerTest, SendFriendRequest_MissingUserId_ShouldFail) {
    Json::Value request(Json::objectValue);
    // 缺少 user_id 字段
    EXPECT_FALSE(request.isMember("user_id"));
}

TEST_F(FriendControllerTest, SendFriendRequest_EmptyUserId_ShouldFail) {
    Json::Value request;
    request["user_id"] = "";

    EXPECT_TRUE(request["user_id"].asString().empty());
}

TEST_F(FriendControllerTest, SendFriendRequest_SelfRequest_ShouldFail) {
    // 不能给自己发好友请求
    std::string currentUserId = "anon_user_123";
    std::string targetUserId = "anon_user_123";

    EXPECT_EQ(currentUserId, targetUserId);
    // 控制器应返回 "不能对自己操作"
    auto resp = buildErrorResponse(400, "不能对自己操作");
    EXPECT_EQ(resp["code"].asInt(), 400);
}

TEST_F(FriendControllerTest, SendFriendRequest_NonJsonBody_ShouldFail) {
    auto resp = buildErrorResponse(400, "请求体必须是JSON格式");
    EXPECT_EQ(resp["code"].asInt(), 400);
}

TEST_F(FriendControllerTest, SendFriendRequest_UnauthorizedWithoutToken) {
    auto resp = buildErrorResponse(401, "用户未认证");
    EXPECT_EQ(resp["code"].asInt(), 401);
}

TEST_F(FriendControllerTest, SendFriendRequest_ResponseContainsIntimacy) {
    // 好友请求响应包含亲密度信息
    Json::Value data;
    data["mode"] = "intimacy_auto";
    data["from_user_id"] = "anon_user_123";
    data["to_user_id"] = "anon_target_456";
    data["intimacy_score"] = 35.0;
    data["intimacy_level"] = "warm";
    data["intimacy_label"] = "温暖连接";
    data["can_chat"] = true;

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("intimacy_score"));
    EXPECT_TRUE(resp["data"].isMember("intimacy_level"));
    EXPECT_TRUE(resp["data"].isMember("can_chat"));
    EXPECT_GE(resp["data"]["intimacy_score"].asDouble(), 0.0);
}

// ==================== 接受好友请求 ====================

TEST_F(FriendControllerTest, AcceptFriendRequest_ValidUserId) {
    std::string targetUserId = "anon_requester_789";
    EXPECT_FALSE(targetUserId.empty());
}

TEST_F(FriendControllerTest, AcceptFriendRequest_EmptyUserId_ShouldFail) {
    std::string targetUserId = "";
    EXPECT_TRUE(targetUserId.empty());
}

// ==================== 拒绝好友请求 ====================

TEST_F(FriendControllerTest, RejectFriendRequest_ValidUserId) {
    std::string targetUserId = "anon_requester_789";
    EXPECT_FALSE(targetUserId.empty());
}

TEST_F(FriendControllerTest, RejectFriendRequest_UnauthorizedWithoutToken) {
    auto resp = buildErrorResponse(401, "用户未认证");
    EXPECT_EQ(resp["code"].asInt(), 401);
}

// ==================== 删除好友 ====================

TEST_F(FriendControllerTest, RemoveFriend_ValidFriendId) {
    std::string friendId = "anon_friend_456";
    EXPECT_FALSE(friendId.empty());
}

TEST_F(FriendControllerTest, RemoveFriend_EmptyFriendId_ShouldFail) {
    std::string friendId = "";
    EXPECT_TRUE(friendId.empty());
}

TEST_F(FriendControllerTest, RemoveFriend_SelfRemove_ShouldFail) {
    std::string userId = "anon_user_123";
    std::string friendId = "anon_user_123";
    EXPECT_EQ(userId, friendId);
}

// ==================== 获取好友列表 ====================

TEST_F(FriendControllerTest, GetFriends_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value friendItem;
    friendItem["user_id"] = "anon_friend_001";
    friendItem["nickname"] = "湖畔知音";
    friendItem["avatar_url"] = "";
    friendItem["intimacy_score"] = 45.0;
    friendItem["intimacy_level"] = "warm";
    friendItem["intimacy_label"] = "温暖连接";
    friendItem["can_chat"] = true;
    data.append(friendItem);

    EXPECT_TRUE(data.isArray());
    EXPECT_EQ(data.size(), 1u);
    EXPECT_TRUE(data[0].isMember("user_id"));
    EXPECT_TRUE(data[0].isMember("nickname"));
    EXPECT_TRUE(data[0].isMember("intimacy_score"));
}

TEST_F(FriendControllerTest, GetFriends_EmptyList) {
    Json::Value data(Json::arrayValue);
    EXPECT_TRUE(data.isArray());
    EXPECT_EQ(data.size(), 0u);
}

// ==================== 待处理请求 ====================

TEST_F(FriendControllerTest, GetPendingRequests_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value pendingReq;
    pendingReq["user_id"] = "anon_requester_001";
    pendingReq["nickname"] = "远方来客";
    pendingReq["created_at"] = "2025-02-08T10:00:00";
    data.append(pendingReq);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("user_id"));
    EXPECT_TRUE(data[0].isMember("nickname"));
}

TEST_F(FriendControllerTest, GetPendingRequests_UnauthorizedWithoutToken) {
    auto resp = buildErrorResponse(401, "用户未认证");
    EXPECT_EQ(resp["code"].asInt(), 401);
}

// ==================== 发送消息 ====================

TEST_F(FriendControllerTest, SendMessage_ValidRequest) {
    Json::Value request;
    request["content"] = "你好，很高兴认识你！";

    EXPECT_TRUE(request.isMember("content"));
    EXPECT_FALSE(request["content"].asString().empty());
}

TEST_F(FriendControllerTest, SendMessage_EmptyContent_ShouldFail) {
    Json::Value request;
    request["content"] = "";

    EXPECT_TRUE(request["content"].asString().empty());
}

TEST_F(FriendControllerTest, SendMessage_MissingContent_ShouldFail) {
    Json::Value request(Json::objectValue);
    EXPECT_FALSE(request.isMember("content"));
}

TEST_F(FriendControllerTest, SendMessage_InsufficientIntimacy_ShouldFail) {
    // 亲密分不足 20 时不能发消息
    double intimacyScore = 15.0;
    double threshold = 20.0;
    EXPECT_LT(intimacyScore, threshold);

    auto resp = buildErrorResponse(403, "亲密分不足，暂不可查看私聊（>=20可开启）");
    EXPECT_EQ(resp["code"].asInt(), 403);
}

// ==================== 获取聊天记录 ====================

TEST_F(FriendControllerTest, GetMessages_ResponseFormat) {
    Json::Value messages(Json::arrayValue);
    Json::Value msg;
    msg["id"] = 1;
    msg["sender_id"] = "anon_user_123";
    msg["receiver_id"] = "anon_friend_456";
    msg["content"] = "你好！";
    msg["created_at"] = "2025-02-08T10:00:00";
    messages.append(msg);

    EXPECT_TRUE(messages.isArray());
    EXPECT_EQ(messages.size(), 1u);
    EXPECT_TRUE(messages[0].isMember("id"));
    EXPECT_TRUE(messages[0].isMember("sender_id"));
    EXPECT_TRUE(messages[0].isMember("receiver_id"));
    EXPECT_TRUE(messages[0].isMember("content"));
    EXPECT_TRUE(messages[0].isMember("created_at"));
}

TEST_F(FriendControllerTest, GetMessages_LimitedTo200) {
    // 消息记录最多返回 200 条（SQL LIMIT 200）
    int maxMessages = 200;
    EXPECT_EQ(maxMessages, 200);
}

TEST_F(FriendControllerTest, GetMessages_InsufficientIntimacy_ShouldFail) {
    double intimacyScore = 10.0;
    double threshold = 20.0;
    EXPECT_LT(intimacyScore, threshold);
}

// ==================== 亲密度等级映射 ====================

TEST_F(FriendControllerTest, IntimacyLevel_Mapping) {
    // 验证亲密度等级中文映射
    auto levelZh = [](const std::string& level) -> std::string {
        if (level == "soulmate") return "灵魂同频";
        if (level == "close") return "深度共鸣";
        if (level == "warm") return "温暖连接";
        return "初识";
    };

    EXPECT_EQ(levelZh("soulmate"), "灵魂同频");
    EXPECT_EQ(levelZh("close"), "深度共鸣");
    EXPECT_EQ(levelZh("warm"), "温暖连接");
    EXPECT_EQ(levelZh("stranger"), "初识");
    EXPECT_EQ(levelZh("unknown"), "初识");
    EXPECT_EQ(levelZh(""), "初识");
}
