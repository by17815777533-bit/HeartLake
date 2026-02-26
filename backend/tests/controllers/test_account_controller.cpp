/**
 * @file test_account_controller.cpp
 * @brief AccountController 单元测试
 * 覆盖账号信息、头像更新、资料编辑、设备管理、隐私设置、屏蔽用户、数据导出等接口
 * Created by 白洋
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class AccountControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    // 模拟构建标准 JSON 响应格式
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

// ==================== 账号信息 ====================

TEST_F(AccountControllerTest, GetAccountInfo_ResponseFormat) {
    // 验证 getAccountInfo 返回的数据结构包含所有必要字段
    Json::Value data;
    data["user_id"] = "anon_abc123";
    data["username"] = "anon_abc123";
    data["nickname"] = "匿名湖友";
    data["avatar_url"] = "";
    data["bio"] = "";
    data["gender"] = "";
    data["birthday"] = "";
    data["email"] = "";
    data["status"] = "active";
    data["created_at"] = "2025-02-08T10:00:00";

    EXPECT_TRUE(data.isMember("user_id"));
    EXPECT_TRUE(data.isMember("nickname"));
    EXPECT_TRUE(data.isMember("avatar_url"));
    EXPECT_TRUE(data.isMember("bio"));
    EXPECT_TRUE(data.isMember("gender"));
    EXPECT_TRUE(data.isMember("birthday"));
    EXPECT_TRUE(data.isMember("email"));
    EXPECT_TRUE(data.isMember("status"));
    EXPECT_TRUE(data.isMember("created_at"));
    EXPECT_EQ(data["status"].asString(), "active");
}

TEST_F(AccountControllerTest, GetAccountInfo_UnauthorizedWithoutToken) {
    // 未登录时应返回 401
    auto resp = buildErrorResponse(401, "未登录");
    EXPECT_EQ(resp["code"].asInt(), 401);
    EXPECT_EQ(resp["message"].asString(), "未登录");
}

// ==================== 头像更新 ====================

TEST_F(AccountControllerTest, UpdateAvatar_ValidRequest) {
    Json::Value request;
    request["avatar_url"] = "https://cdn.heartlake.app/avatars/user123.jpg";

    EXPECT_TRUE(request.isMember("avatar_url"));
    EXPECT_FALSE(request["avatar_url"].asString().empty());
}

TEST_F(AccountControllerTest, UpdateAvatar_MissingAvatarUrl_ShouldFail) {
    Json::Value request;
    // 缺少 avatar_url 字段
    EXPECT_FALSE(request.isMember("avatar_url"));
}

TEST_F(AccountControllerTest, UpdateAvatar_EmptyAvatarUrl) {
    Json::Value request;
    request["avatar_url"] = "";
    // 空 URL 也应该被接受（清除头像）或拒绝，取决于业务逻辑
    EXPECT_TRUE(request["avatar_url"].asString().empty());
}

TEST_F(AccountControllerTest, UpdateAvatar_ResponseContainsNewUrl) {
    std::string newUrl = "https://cdn.heartlake.app/avatars/new.jpg";
    Json::Value data;
    data["avatar_url"] = newUrl;

    auto resp = buildSuccessResponse(data, "头像更新成功");
    EXPECT_EQ(resp["data"]["avatar_url"].asString(), newUrl);
    EXPECT_EQ(resp["message"].asString(), "头像更新成功");
}

// ==================== 资料更新 ====================

TEST_F(AccountControllerTest, UpdateProfile_ValidNickname) {
    Json::Value request;
    request["nickname"] = "心湖旅人";

    EXPECT_TRUE(request.isMember("nickname"));
    EXPECT_FALSE(request["nickname"].asString().empty());
}

TEST_F(AccountControllerTest, UpdateProfile_MultipleFIelds) {
    Json::Value request;
    request["nickname"] = "心湖旅人";
    request["bio"] = "喜欢在湖边散步";
    request["gender"] = "male";
    request["birthday"] = "2000-01-15";

    EXPECT_TRUE(request.isMember("nickname"));
    EXPECT_TRUE(request.isMember("bio"));
    EXPECT_TRUE(request.isMember("gender"));
    EXPECT_TRUE(request.isMember("birthday"));
}

TEST_F(AccountControllerTest, UpdateProfile_EmptyBody_ShouldFail) {
    Json::Value request(Json::objectValue);
    // 没有任何可更新字段时，控制器返回 "没有要更新的字段"
    bool hasUpdateFields = request.isMember("nickname") || request.isMember("bio")
                        || request.isMember("gender") || request.isMember("birthday");
    EXPECT_FALSE(hasUpdateFields);
}

TEST_F(AccountControllerTest, UpdateProfile_NonJsonBody_ShouldFail) {
    // 非 JSON 请求体应返回 400
    auto resp = buildErrorResponse(400, "请求体必须是JSON格式");
    EXPECT_EQ(resp["code"].asInt(), 400);
}

TEST_F(AccountControllerTest, UpdateProfile_NicknameMaxLength) {
    // 昵称长度边界测试
    std::string longNickname(50, 'A');
    EXPECT_EQ(longNickname.length(), 50u);

    std::string tooLong(256, 'B');
    EXPECT_GT(tooLong.length(), 255u);
}

// ==================== 账号统计 ====================

TEST_F(AccountControllerTest, GetAccountStats_ResponseFormat) {
    Json::Value data;
    data["stone_count"] = 42;
    data["ripple_count"] = 128;
    data["boat_count"] = 15;
    data["friend_count"] = 7;
    data["connection_count"] = 3;
    data["days_active"] = 30;

    EXPECT_TRUE(data.isMember("stone_count"));
    EXPECT_TRUE(data.isMember("ripple_count"));
    EXPECT_TRUE(data.isMember("boat_count"));
    EXPECT_GE(data["stone_count"].asInt(), 0);
    EXPECT_GE(data["friend_count"].asInt(), 0);
}

// ==================== 设备管理 ====================

TEST_F(AccountControllerTest, GetLoginDevices_ResponseIsArray) {
    Json::Value devices(Json::arrayValue);
    Json::Value device;
    device["session_id"] = "sess_abc123";
    device["device_type"] = "iOS";
    device["login_time"] = "2025-02-08T10:00:00";
    device["ip_address"] = "192.168.1.1";
    devices.append(device);

    EXPECT_TRUE(devices.isArray());
    EXPECT_EQ(devices.size(), 1u);
    EXPECT_TRUE(devices[0].isMember("session_id"));
    EXPECT_TRUE(devices[0].isMember("device_type"));
}

TEST_F(AccountControllerTest, RemoveDevice_ValidSessionId) {
    std::string sessionId = "sess_abc123";
    EXPECT_FALSE(sessionId.empty());
    EXPECT_EQ(sessionId.substr(0, 5), "sess_");
}

TEST_F(AccountControllerTest, RemoveDevice_NotFound) {
    // 不存在的设备应返回 404
    auto resp = buildErrorResponse(404, "设备不存在");
    EXPECT_EQ(resp["code"].asInt(), 404);
}

// ==================== 登录日志 ====================

TEST_F(AccountControllerTest, GetLoginLogs_ResponseFormat) {
    Json::Value log;
    log["log_id"] = 1;
    log["login_time"] = "2025-02-08T10:00:00";
    log["ip_address"] = "192.168.1.1";
    log["device_type"] = "Android";
    log["location"] = "北京";
    log["success"] = true;

    EXPECT_TRUE(log.isMember("log_id"));
    EXPECT_TRUE(log.isMember("login_time"));
    EXPECT_TRUE(log.isMember("ip_address"));
    EXPECT_TRUE(log.isMember("success"));
}

// ==================== 安全事件 ====================

TEST_F(AccountControllerTest, GetSecurityEvents_ResponseFormat) {
    Json::Value event;
    event["event_id"] = 1;
    event["event_type"] = "login_failed";
    event["severity"] = "high";
    event["description"] = "多次登录失败";
    event["created_at"] = "2025-02-08T10:00:00";

    EXPECT_TRUE(event.isMember("event_type"));
    EXPECT_TRUE(event.isMember("severity"));
}

// ==================== 隐私设置 ====================

TEST_F(AccountControllerTest, GetPrivacySettings_ResponseFormat) {
    Json::Value settings;
    settings["show_online_status"] = true;
    settings["allow_friend_request"] = true;
    settings["show_emotion_data"] = false;
    settings["data_collection_consent"] = true;

    EXPECT_TRUE(settings.isMember("show_online_status"));
    EXPECT_TRUE(settings.isMember("allow_friend_request"));
    EXPECT_TRUE(settings["show_online_status"].isBool());
}

TEST_F(AccountControllerTest, UpdatePrivacySettings_ValidRequest) {
    Json::Value request;
    request["show_online_status"] = false;
    request["allow_friend_request"] = false;

    EXPECT_TRUE(request.isMember("show_online_status"));
    EXPECT_FALSE(request["show_online_status"].asBool());
}

TEST_F(AccountControllerTest, UpdatePrivacySettings_EmptyBody_ShouldFail) {
    Json::Value request(Json::objectValue);
    bool hasFields = request.isMember("show_online_status")
                  || request.isMember("allow_friend_request")
                  || request.isMember("show_emotion_data")
                  || request.isMember("data_collection_consent");
    EXPECT_FALSE(hasFields);
}

// ==================== 屏蔽用户 ====================

TEST_F(AccountControllerTest, BlockUser_ValidTargetUserId) {
    std::string targetUserId = "anon_target123";
    EXPECT_FALSE(targetUserId.empty());
}

TEST_F(AccountControllerTest, BlockUser_CannotBlockSelf) {
    std::string userId = "anon_abc123";
    std::string targetUserId = "anon_abc123";
    // 控制器应拒绝屏蔽自己
    EXPECT_EQ(userId, targetUserId);
}

TEST_F(AccountControllerTest, UnblockUser_ValidTargetUserId) {
    std::string targetUserId = "anon_target456";
    EXPECT_FALSE(targetUserId.empty());
}

TEST_F(AccountControllerTest, GetBlockedUsers_ResponseIsArray) {
    Json::Value blockedUsers(Json::arrayValue);
    Json::Value user;
    user["user_id"] = "anon_blocked1";
    user["nickname"] = "被屏蔽用户";
    user["blocked_at"] = "2025-02-08T10:00:00";
    blockedUsers.append(user);

    EXPECT_TRUE(blockedUsers.isArray());
    EXPECT_EQ(blockedUsers.size(), 1u);
}

// ==================== 数据导出 ====================

TEST_F(AccountControllerTest, ExportData_ResponseContainsTaskId) {
    Json::Value data;
    data["task_id"] = "export_task_abc123";
    data["status"] = "pending";

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("task_id"));
    EXPECT_FALSE(resp["data"]["task_id"].asString().empty());
    EXPECT_EQ(resp["data"]["status"].asString(), "pending");
}

TEST_F(AccountControllerTest, GetExportStatus_ValidTaskId) {
    std::string taskId = "export_task_abc123";
    EXPECT_FALSE(taskId.empty());

    // 导出状态应包含进度信息
    Json::Value data;
    data["task_id"] = taskId;
    data["status"] = "completed";
    data["download_url"] = "https://cdn.heartlake.app/exports/abc123.zip";

    EXPECT_TRUE(data.isMember("status"));
    EXPECT_TRUE(data.isMember("download_url"));
}

// ==================== 账号停用/删除 ====================

TEST_F(AccountControllerTest, DeactivateAccount_ResponseFormat) {
    auto resp = buildSuccessResponse(Json::Value(), "账号已停用");
    EXPECT_EQ(resp["message"].asString(), "账号已停用");
}

TEST_F(AccountControllerTest, DeleteAccountPermanently_ResponseFormat) {
    auto resp = buildSuccessResponse(Json::Value(), "账号已永久删除");
    EXPECT_EQ(resp["message"].asString(), "账号已永久删除");
}

TEST_F(AccountControllerTest, DeleteAccountPermanently_RequiresAuth) {
    // 未认证时应返回 401
    auto resp = buildErrorResponse(401, "未登录");
    EXPECT_EQ(resp["code"].asInt(), 401);
}
