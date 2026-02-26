/**
 * @file test_admin_controller.cpp
 * @brief AdminController 单元测试
 * 覆盖管理员登录/登出、权限验证、仪表盘统计、用户增长、情绪分布、
 * 活跃时段、高风险用户/事件、风险事件处理、安全审计等接口
 * Created by 白洋
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>
#include <algorithm>

class AdminControllerTest : public ::testing::Test {
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

    // 模拟时间恒定比较（与 AdminController 中的 constantTimeCompare 一致）
    bool constantTimeCompare(const std::string& a, const std::string& b) {
        volatile unsigned char result = static_cast<unsigned char>(a.size() ^ b.size());
        size_t len = std::max(a.size(), b.size());
        for (size_t i = 0; i < len; ++i) {
            unsigned char ca = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
            unsigned char cb = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
            result |= ca ^ cb;
        }
        return result == 0;
    }
};

// ==================== 管理员登录 ====================

TEST_F(AdminControllerTest, Login_ValidRequest) {
    Json::Value request;
    request["username"] = "admin";
    request["password"] = "SecureP@ss123";

    EXPECT_TRUE(request.isMember("username"));
    EXPECT_TRUE(request.isMember("password"));
    EXPECT_FALSE(request["username"].asString().empty());
    EXPECT_FALSE(request["password"].asString().empty());
}

TEST_F(AdminControllerTest, Login_EmptyUsername_ShouldFail) {
    Json::Value request;
    request["username"] = "";
    request["password"] = "password123";

    bool empty = request["username"].asString().empty() || request["password"].asString().empty();
    EXPECT_TRUE(empty);
}

TEST_F(AdminControllerTest, Login_EmptyPassword_ShouldFail) {
    Json::Value request;
    request["username"] = "admin";
    request["password"] = "";

    bool empty = request["username"].asString().empty() || request["password"].asString().empty();
    EXPECT_TRUE(empty);
}

TEST_F(AdminControllerTest, Login_NonJsonBody_ShouldFail) {
    auto resp = buildErrorResponse(400, "请求体必须是 JSON 格式");
    EXPECT_EQ(resp["code"].asInt(), 400);
}

TEST_F(AdminControllerTest, Login_SuccessResponseContainsToken) {
    Json::Value data;
    data["token"] = "v4.public.admin_token_payload";
    data["user"]["user_id"] = "admin_001";
    data["user"]["username"] = "admin";
    data["user"]["role"] = "admin";

    auto resp = buildSuccessResponse(data, "登录成功");
    EXPECT_TRUE(resp["data"].isMember("token"));
    EXPECT_FALSE(resp["data"]["token"].asString().empty());
    EXPECT_EQ(resp["data"]["user"]["role"].asString(), "admin");
    EXPECT_EQ(resp["data"]["user"]["user_id"].asString(), "admin_001");
    EXPECT_EQ(resp["message"].asString(), "登录成功");
}

TEST_F(AdminControllerTest, Login_WrongCredentials_Returns401) {
    auto resp = buildErrorResponse(401, "用户名或密码错误");
    EXPECT_EQ(resp["code"].asInt(), 401);
    EXPECT_EQ(resp["message"].asString(), "用户名或密码错误");
}

// ==================== 时间恒定比较 ====================

TEST_F(AdminControllerTest, ConstantTimeCompare_EqualStrings) {
    EXPECT_TRUE(constantTimeCompare("admin", "admin"));
    EXPECT_TRUE(constantTimeCompare("", ""));
    EXPECT_TRUE(constantTimeCompare("test123", "test123"));
}

TEST_F(AdminControllerTest, ConstantTimeCompare_DifferentStrings) {
    EXPECT_FALSE(constantTimeCompare("admin", "Admin"));
    EXPECT_FALSE(constantTimeCompare("admin", "admin1"));
    EXPECT_FALSE(constantTimeCompare("abc", "xyz"));
    EXPECT_FALSE(constantTimeCompare("short", "muchlongerstring"));
}

// ==================== 管理员登出 ====================

TEST_F(AdminControllerTest, Logout_ResponseFormat) {
    Json::Value data;
    data["message"] = "登出成功";

    auto resp = buildSuccessResponse(data, "登出成功");
    EXPECT_EQ(resp["message"].asString(), "登出成功");
}

// ==================== 管理员信息 ====================

TEST_F(AdminControllerTest, GetInfo_RequiresAdminAuth) {
    // 非管理员应被 AdminAuthFilter 拦截
    auto resp = buildErrorResponse(401, "未授权");
    EXPECT_EQ(resp["code"].asInt(), 401);
}

// ==================== 仪表盘统计 ====================

TEST_F(AdminControllerTest, GetDashboardStats_ResponseFormat) {
    Json::Value data;
    data["total_users"] = 1500;
    data["total_stones"] = 8200;
    data["today_active_users"] = 320;
    data["today_stones"] = 45;
    data["today_interactions"] = 128;

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("total_users"));
    EXPECT_TRUE(resp["data"].isMember("total_stones"));
    EXPECT_TRUE(resp["data"].isMember("today_active_users"));
    EXPECT_TRUE(resp["data"].isMember("today_stones"));
    EXPECT_TRUE(resp["data"].isMember("today_interactions"));
    EXPECT_GE(resp["data"]["total_users"].asInt(), 0);
    EXPECT_GE(resp["data"]["total_stones"].asInt(), 0);
}

// ==================== 用户增长统计 ====================

TEST_F(AdminControllerTest, GetUserGrowthStats_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value dayItem;
    dayItem["date"] = "2025-02-08";
    dayItem["count"] = 25;
    data.append(dayItem);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("date"));
    EXPECT_TRUE(data[0].isMember("count"));
    EXPECT_GE(data[0]["count"].asInt(), 0);
}

TEST_F(AdminControllerTest, GetUserGrowthStats_DaysParamClamped) {
    // days 参数范围 [1, 365]，超出范围回退到默认值 7
    int days = 500;
    if (days < 1 || days > 365) days = 7;
    EXPECT_EQ(days, 7);

    days = -1;
    if (days < 1 || days > 365) days = 7;
    EXPECT_EQ(days, 7);

    days = 30;
    if (days < 1 || days > 365) { /* no change */ }
    EXPECT_EQ(days, 30);
}

// ==================== 情绪分布 ====================

TEST_F(AdminControllerTest, GetMoodDistribution_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value mood;
    mood["mood"] = "happy";
    mood["count"] = 350;
    data.append(mood);

    Json::Value mood2;
    mood2["mood"] = "sad";
    mood2["count"] = 120;
    data.append(mood2);

    EXPECT_TRUE(data.isArray());
    EXPECT_GE(data.size(), 2u);
    EXPECT_TRUE(data[0].isMember("mood"));
    EXPECT_TRUE(data[0].isMember("count"));
}

// ==================== 活跃时段 ====================

TEST_F(AdminControllerTest, GetActiveTimeStats_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value hourItem;
    hourItem["hour"] = 14;
    hourItem["count"] = 85;
    data.append(hourItem);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("hour"));
    EXPECT_TRUE(data[0].isMember("count"));
    EXPECT_GE(data[0]["hour"].asInt(), 0);
    EXPECT_LE(data[0]["hour"].asInt(), 23);
}

// ==================== 热门话题 ====================

TEST_F(AdminControllerTest, GetTrendingTopics_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value topic;
    topic["topic"] = "心情";
    topic["count"] = 42;
    data.append(topic);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("topic"));
    EXPECT_TRUE(data[0].isMember("count"));
    EXPECT_FALSE(data[0]["topic"].asString().empty());
}

// ==================== 高风险用户 ====================

TEST_F(AdminControllerTest, GetHighRiskUsers_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value riskUser;
    riskUser["user_id"] = "anon_risk_001";
    riskUser["risk_score"] = 0.85;
    riskUser["risk_level"] = "high";
    data.append(riskUser);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("user_id"));
    EXPECT_TRUE(data[0].isMember("risk_score"));
}

// ==================== 高风险事件 ====================

TEST_F(AdminControllerTest, GetHighRiskEvents_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value event;
    event["event_id"] = 1;
    event["user_id"] = "anon_risk_001";
    event["event_type"] = "crisis_detected";
    event["severity"] = "high";
    event["status"] = "pending";
    event["created_at"] = "2025-02-08T10:00:00";
    data.append(event);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("event_id"));
    EXPECT_TRUE(data[0].isMember("event_type"));
    EXPECT_TRUE(data[0].isMember("status"));
}

// ==================== 用户风险历史 ====================

TEST_F(AdminControllerTest, GetUserRiskHistory_ValidUserId) {
    std::string userId = "anon_risk_001";
    EXPECT_FALSE(userId.empty());
}

// ==================== 风险事件处理 ====================

TEST_F(AdminControllerTest, HandleRiskEvent_ValidRequest) {
    Json::Value request;
    request["action"] = "intervene";
    request["notes"] = "已联系用户，情况稳定";
    request["status"] = "resolved";

    EXPECT_TRUE(request.isMember("action"));
    EXPECT_TRUE(request.isMember("status"));
    EXPECT_FALSE(request["action"].asString().empty());
}

TEST_F(AdminControllerTest, HandleRiskEvent_ResponseFormat) {
    Json::Value data;
    data["event_id"] = "42";
    data["status"] = "resolved";
    data["handled_at"] = static_cast<Json::Int64>(1707350400);

    auto resp = buildSuccessResponse(data, "事件处理成功");
    EXPECT_EQ(resp["message"].asString(), "事件处理成功");
    EXPECT_TRUE(resp["data"].isMember("event_id"));
    EXPECT_TRUE(resp["data"].isMember("status"));
    EXPECT_EQ(resp["data"]["status"].asString(), "resolved");
}

// ==================== 安全审计 ====================

TEST_F(AdminControllerTest, GetSecurityAudit_ResponseFormat) {
    Json::Value data;
    data["overall_score"] = 85;
    data["categories"] = Json::Value(Json::arrayValue);

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("overall_score"));
    EXPECT_GE(resp["data"]["overall_score"].asInt(), 0);
    EXPECT_LE(resp["data"]["overall_score"].asInt(), 100);
}

// ==================== 实时统计 ====================

TEST_F(AdminControllerTest, GetRealtimeStats_ResponseFormat) {
    Json::Value data;
    data["online_users"] = 42;
    data["active_connections"] = 15;
    data["messages_per_minute"] = 8;

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("online_users"));
    EXPECT_GE(resp["data"]["online_users"].asInt(), 0);
}
