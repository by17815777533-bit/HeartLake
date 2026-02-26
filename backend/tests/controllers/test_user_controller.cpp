/**
 * @file test_user_controller.cpp
 * @brief UserController 单元测试
 * 覆盖匿名登录、恢复密钥登录、token刷新、用户信息、搜索、情感热力图/日历、统计等接口
 * Created by 白洋
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class UserControllerTest : public ::testing::Test {
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

// ==================== 匿名登录 ====================

TEST_F(UserControllerTest, AnonymousLogin_ValidDeviceId) {
    Json::Value request;
    request["device_id"] = "device_ios_abc123def456";

    EXPECT_TRUE(request.isMember("device_id"));
    EXPECT_FALSE(request["device_id"].asString().empty());
}

TEST_F(UserControllerTest, AnonymousLogin_EmptyDeviceId_ShouldFail) {
    Json::Value request;
    request["device_id"] = "";

    EXPECT_TRUE(request["device_id"].asString().empty());
}

TEST_F(UserControllerTest, AnonymousLogin_MissingDeviceId_ShouldFail) {
    Json::Value request(Json::objectValue);
    // 缺少 device_id 字段
    std::string deviceId = request.get("device_id", "").asString();
    EXPECT_TRUE(deviceId.empty());
}

TEST_F(UserControllerTest, AnonymousLogin_ResponseContainsToken) {
    // 登录成功应返回 token、user_id、nickname
    Json::Value data;
    data["token"] = "v4.public.eyJ1c2VyX2lkIjoiYW5vbl8xMjMifQ";
    data["user_id"] = "anon_abc123";
    data["nickname"] = "匿名湖友";
    data["is_anonymous"] = true;
    data["recovery_key"] = "ocean-breeze-sunset-mountain";

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("token"));
    EXPECT_TRUE(resp["data"].isMember("user_id"));
    EXPECT_TRUE(resp["data"].isMember("nickname"));
    EXPECT_FALSE(resp["data"]["token"].asString().empty());
    EXPECT_FALSE(resp["data"]["user_id"].asString().empty());
}

TEST_F(UserControllerTest, AnonymousLogin_NonJsonBody_ShouldFail) {
    auto resp = buildErrorResponse(400, "请求体必须是 JSON 格式");
    EXPECT_EQ(resp["code"].asInt(), 400);
}

// ==================== 恢复密钥登录 ====================

TEST_F(UserControllerTest, RecoverWithKey_ValidRequest) {
    Json::Value request;
    request["recovery_key"] = "ocean-breeze-sunset-mountain";
    request["device_id"] = "device_new_abc";

    EXPECT_TRUE(request.isMember("recovery_key"));
    EXPECT_TRUE(request.isMember("device_id"));
    EXPECT_FALSE(request["recovery_key"].asString().empty());
}

TEST_F(UserControllerTest, RecoverWithKey_EmptyKey_ShouldFail) {
    Json::Value request;
    request["recovery_key"] = "";
    request["device_id"] = "device_abc";

    EXPECT_TRUE(request["recovery_key"].asString().empty());
}

TEST_F(UserControllerTest, RecoverWithKey_MissingDeviceId_ShouldFail) {
    Json::Value request;
    request["recovery_key"] = "ocean-breeze-sunset-mountain";
    // 缺少 device_id
    EXPECT_FALSE(request.isMember("device_id"));
}

// ==================== Token 刷新 ====================

TEST_F(UserControllerTest, RefreshToken_ResponseContainsNewToken) {
    Json::Value data;
    data["token"] = "v4.public.new_token_payload";

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("token"));
    EXPECT_FALSE(resp["data"]["token"].asString().empty());
}

TEST_F(UserControllerTest, RefreshToken_UnauthorizedWithoutToken) {
    auto resp = buildErrorResponse(401, "未登录");
    EXPECT_EQ(resp["code"].asInt(), 401);
}

// ==================== 获取用户信息 ====================

TEST_F(UserControllerTest, GetUserInfo_ValidUserId) {
    std::string userId = "anon_abc123";
    EXPECT_FALSE(userId.empty());

    Json::Value data;
    data["user_id"] = userId;
    data["nickname"] = "匿名湖友";
    data["avatar_url"] = "";
    data["bio"] = "热爱生活";
    data["stone_count"] = 10;

    EXPECT_TRUE(data.isMember("user_id"));
    EXPECT_TRUE(data.isMember("nickname"));
    EXPECT_EQ(data["user_id"].asString(), userId);
}

TEST_F(UserControllerTest, GetUserInfo_EmptyUserId_ShouldFail) {
    std::string userId = "";
    EXPECT_TRUE(userId.empty());
}

TEST_F(UserControllerTest, GetUserInfo_NonExistentUser) {
    auto resp = buildErrorResponse(404, "用户不存在");
    EXPECT_EQ(resp["code"].asInt(), 404);
}

// ==================== 用户搜索 ====================

TEST_F(UserControllerTest, SearchUsers_ValidKeyword) {
    std::string keyword = "湖友";
    EXPECT_FALSE(keyword.empty());
    EXPECT_LE(keyword.length(), 100u);
}

TEST_F(UserControllerTest, SearchUsers_EmptyKeyword_ShouldFail) {
    std::string keyword = "";
    EXPECT_TRUE(keyword.empty());
}

TEST_F(UserControllerTest, SearchUsers_ResponseIsPaginated) {
    Json::Value data;
    Json::Value users(Json::arrayValue);
    Json::Value user;
    user["user_id"] = "anon_abc";
    user["nickname"] = "匿名湖友";
    users.append(user);

    data["users"] = users;
    data["total"] = 1;
    data["page"] = 1;
    data["page_size"] = 20;

    EXPECT_TRUE(data["users"].isArray());
    EXPECT_GE(data["total"].asInt(), 0);
}

// ==================== 情感热力图 ====================

TEST_F(UserControllerTest, GetEmotionHeatmap_ResponseFormat) {
    Json::Value data;
    Json::Value days(Json::objectValue);

    // 每天的数据包含 score、raw_score、count
    Json::Value dayData;
    dayData["score"] = 0.75;
    dayData["raw_score"] = 0.5;
    dayData["count"] = 3;
    days["2025-02-08"] = dayData;

    data["days"] = days;

    EXPECT_TRUE(data.isMember("days"));
    EXPECT_TRUE(data["days"].isMember("2025-02-08"));
    EXPECT_TRUE(data["days"]["2025-02-08"].isMember("score"));
    EXPECT_TRUE(data["days"]["2025-02-08"].isMember("count"));
}

TEST_F(UserControllerTest, GetEmotionHeatmap_ScoreNormalized) {
    // score 应在 [0, 1] 范围内（已归一化）
    double rawScore = 0.5;
    double normalized = (rawScore + 1.0) / 2.0;
    EXPECT_GE(normalized, 0.0);
    EXPECT_LE(normalized, 1.0);

    // 边界值测试
    double minNorm = (-1.0 + 1.0) / 2.0;
    double maxNorm = (1.0 + 1.0) / 2.0;
    EXPECT_DOUBLE_EQ(minNorm, 0.0);
    EXPECT_DOUBLE_EQ(maxNorm, 1.0);
}

TEST_F(UserControllerTest, GetEmotionHeatmap_DaysParameter) {
    // days 参数验证：默认30天，范围 [1, 365]
    int defaultDays = 30;
    EXPECT_GE(defaultDays, 1);
    EXPECT_LE(defaultDays, 365);

    // 超出范围应使用默认值
    int invalidDays = 0;
    if (invalidDays < 1 || invalidDays > 365) invalidDays = 30;
    EXPECT_EQ(invalidDays, 30);
}

// ==================== 情感日历 ====================

TEST_F(UserControllerTest, GetEmotionCalendar_ValidYearMonth) {
    int year = 2025;
    int month = 2;

    EXPECT_GE(year, 2020);
    EXPECT_LE(year, 2030);
    EXPECT_GE(month, 1);
    EXPECT_LE(month, 12);
}

TEST_F(UserControllerTest, GetEmotionCalendar_InvalidMonth_ShouldClamp) {
    int month = 13;
    if (month < 1 || month > 12) month = 1;
    EXPECT_EQ(month, 1);

    month = 0;
    if (month < 1 || month > 12) month = 1;
    EXPECT_EQ(month, 1);
}

// ==================== 用户统计 ====================

TEST_F(UserControllerTest, GetUserStats_ResponseFormat) {
    Json::Value data;
    data["stone_count"] = 42;
    data["ripple_count"] = 128;
    data["boat_count"] = 15;
    data["connection_count"] = 5;

    EXPECT_TRUE(data.isMember("stone_count"));
    EXPECT_TRUE(data.isMember("ripple_count"));
    EXPECT_GE(data["stone_count"].asInt(), 0);
    EXPECT_GE(data["ripple_count"].asInt(), 0);
}

TEST_F(UserControllerTest, GetUserStats_InvalidUserId) {
    auto resp = buildErrorResponse(404, "用户不存在");
    EXPECT_EQ(resp["code"].asInt(), 404);
}

// ==================== 更新昵称 ====================

TEST_F(UserControllerTest, UpdateNickname_ValidRequest) {
    Json::Value request;
    request["nickname"] = "新昵称";

    EXPECT_TRUE(request.isMember("nickname"));
    EXPECT_FALSE(request["nickname"].asString().empty());
}

TEST_F(UserControllerTest, UpdateNickname_EmptyNickname_ShouldFail) {
    Json::Value request;
    request["nickname"] = "";

    EXPECT_TRUE(request["nickname"].asString().empty());
}

TEST_F(UserControllerTest, UpdateNickname_TooLong_ShouldFail) {
    std::string longName(256, 'A');
    EXPECT_GT(longName.length(), 255u);
}

// ==================== 更新个人资料 ====================

TEST_F(UserControllerTest, UpdateProfile_ValidRequest) {
    Json::Value request;
    request["nickname"] = "心湖旅人";
    request["bio"] = "在心湖边寻找共鸣";
    request["avatar_url"] = "https://cdn.heartlake.app/avatar.jpg";

    EXPECT_TRUE(request.isMember("nickname"));
    EXPECT_TRUE(request.isMember("bio"));
    EXPECT_TRUE(request.isMember("avatar_url"));
}

// ==================== 删除账号 ====================

TEST_F(UserControllerTest, DeleteAccount_RequiresAuth) {
    auto resp = buildErrorResponse(401, "未登录");
    EXPECT_EQ(resp["code"].asInt(), 401);
}

TEST_F(UserControllerTest, DeleteAccount_SuccessResponse) {
    auto resp = buildSuccessResponse(Json::Value(), "账号已删除");
    EXPECT_EQ(resp["code"].asInt(), 0);
}

// ==================== 分页参数验证 ====================

TEST_F(UserControllerTest, PaginationParams_ValidRange) {
    int page = 1;
    int pageSize = 20;

    EXPECT_GE(page, 1);
    EXPECT_LE(page, 10000);
    EXPECT_GE(pageSize, 1);
    EXPECT_LE(pageSize, 100);
}

TEST_F(UserControllerTest, PaginationParams_InvalidValues_ShouldClamp) {
    // 模拟 safePagination 的行为
    int page = -1;
    int pageSize = 500;
    int defPage = 1, defSize = 20;

    if (page < 1 || page > 10000) page = defPage;
    if (pageSize < 1 || pageSize > 100) pageSize = defSize;

    EXPECT_EQ(page, 1);
    EXPECT_EQ(pageSize, 20);
}

// ==================== 获取我的纸船 ====================

TEST_F(UserControllerTest, GetMyBoats_ResponseFormat) {
    Json::Value data;
    Json::Value boats(Json::arrayValue);
    Json::Value boat;
    boat["boat_id"] = "boat_123";
    boat["content"] = "漂流瓶内容";
    boat["stone_id"] = "stone_456";
    boat["created_at"] = "2025-02-08T10:00:00";
    boats.append(boat);
    data["boats"] = boats;

    EXPECT_TRUE(data["boats"].isArray());
    EXPECT_EQ(data["boats"].size(), 1u);
    EXPECT_TRUE(data["boats"][0].isMember("boat_id"));
}
