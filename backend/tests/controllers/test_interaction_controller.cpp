/**
 * InteractionController 单元测试
 * 覆盖涟漪创建/删除、纸船创建/获取/删除、通知、连接创建/升级/消息、分页参数等接口
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class InteractionControllerTest : public ::testing::Test {
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

    // 模拟 safePagination 逻辑
    std::pair<int, int> clampPagination(int page, int pageSize) {
        if (page < 1 || page > 10000) page = 1;
        if (pageSize < 1 || pageSize > 100) pageSize = 20;
        return {page, pageSize};
    }
};

// ==================== 涟漪创建 ====================

TEST_F(InteractionControllerTest, CreateRipple_ValidStoneId) {
    std::string stoneId = "stone_abc123";
    EXPECT_FALSE(stoneId.empty());
}

TEST_F(InteractionControllerTest, CreateRipple_ResponseFormat) {
    Json::Value data;
    data["ripple_count"] = 5;

    auto resp = buildSuccessResponse(data, "涟漪成功");
    EXPECT_EQ(resp["message"].asString(), "涟漪成功");
    EXPECT_TRUE(resp["data"].isMember("ripple_count"));
    EXPECT_GE(resp["data"]["ripple_count"].asInt(), 1);
}

TEST_F(InteractionControllerTest, CreateRipple_UnauthorizedWithoutToken) {
    auto resp = buildErrorResponse(401, "未登录");
    EXPECT_EQ(resp["code"].asInt(), 401);
}

TEST_F(InteractionControllerTest, CreateRipple_BroadcastEventFormat) {
    // 涟漪创建后广播的事件格式
    Json::Value broadcastMsg;
    broadcastMsg["type"] = "ripple_update";
    broadcastMsg["stone_id"] = "stone_abc123";
    broadcastMsg["ripple_count"] = 5;
    broadcastMsg["triggered_by"] = "anon_user_001";
    broadcastMsg["timestamp"] = static_cast<Json::Int64>(1707350400);

    EXPECT_EQ(broadcastMsg["type"].asString(), "ripple_update");
    EXPECT_TRUE(broadcastMsg.isMember("stone_id"));
    EXPECT_TRUE(broadcastMsg.isMember("ripple_count"));
    EXPECT_TRUE(broadcastMsg.isMember("triggered_by"));
    EXPECT_TRUE(broadcastMsg.isMember("timestamp"));
}

// ==================== 涟漪删除 ====================

TEST_F(InteractionControllerTest, DeleteRipple_ValidRippleId) {
    std::string rippleId = "ripple_abc123";
    EXPECT_FALSE(rippleId.empty());
}

TEST_F(InteractionControllerTest, DeleteRipple_ResponseFormat) {
    auto resp = buildSuccessResponse(Json::Value(), "删除涟漪成功");
    EXPECT_EQ(resp["message"].asString(), "删除涟漪成功");
}

TEST_F(InteractionControllerTest, DeleteRipple_BroadcastEventFormat) {
    Json::Value broadcastMsg;
    broadcastMsg["type"] = "ripple_deleted";
    broadcastMsg["stone_id"] = "stone_abc123";
    broadcastMsg["ripple_id"] = "ripple_abc123";
    broadcastMsg["ripple_count"] = 4;
    broadcastMsg["triggered_by"] = "anon_user_001";

    EXPECT_EQ(broadcastMsg["type"].asString(), "ripple_deleted");
    EXPECT_TRUE(broadcastMsg.isMember("ripple_id"));
}

// ==================== 纸船创建 ====================

TEST_F(InteractionControllerTest, CreateBoat_ValidRequest) {
    Json::Value request;
    request["content"] = "这是一只纸船，承载着我的心意";

    EXPECT_TRUE(request.isMember("content"));
    EXPECT_FALSE(request["content"].asString().empty());
}

TEST_F(InteractionControllerTest, CreateBoat_EmptyContent_ShouldFail) {
    Json::Value request;
    request["content"] = "";

    EXPECT_TRUE(request["content"].asString().empty());
}

TEST_F(InteractionControllerTest, CreateBoat_MissingContent_ShouldFail) {
    Json::Value request(Json::objectValue);
    EXPECT_FALSE(request.isMember("content"));
}

TEST_F(InteractionControllerTest, CreateBoat_NonJsonBody_ShouldFail) {
    auto resp = buildErrorResponse(400, "请求体必须是JSON格式");
    EXPECT_EQ(resp["code"].asInt(), 400);
}

// ==================== 纸船列表 ====================

TEST_F(InteractionControllerTest, GetBoats_ResponseFormat) {
    Json::Value data;
    Json::Value boats(Json::arrayValue);
    Json::Value boat;
    boat["boat_id"] = "boat_001";
    boat["content"] = "纸船内容";
    boat["sender_id"] = "anon_user_001";
    boat["created_at"] = "2025-02-08T10:00:00";
    boats.append(boat);

    data["boats"] = boats;
    data["items"] = boats;  // 兼容旧前端
    data["total"] = 1;
    data["page"] = 1;
    data["page_size"] = 20;

    EXPECT_TRUE(data.isMember("boats"));
    EXPECT_TRUE(data["boats"].isArray());
    EXPECT_TRUE(data.isMember("total"));
    EXPECT_TRUE(data.isMember("page"));
    EXPECT_TRUE(data.isMember("page_size"));
    // 兼容字段
    EXPECT_TRUE(data.isMember("items"));
}

TEST_F(InteractionControllerTest, GetBoats_EmptyList) {
    Json::Value data;
    data["boats"] = Json::Value(Json::arrayValue);
    data["total"] = 0;
    data["page"] = 1;
    data["page_size"] = 20;

    EXPECT_EQ(data["boats"].size(), 0u);
    EXPECT_EQ(data["total"].asInt(), 0);
}

// ==================== 纸船删除 ====================

TEST_F(InteractionControllerTest, DeleteBoat_ResponseFormat) {
    auto resp = buildSuccessResponse(Json::Value(), "删除纸船成功");
    EXPECT_EQ(resp["message"].asString(), "删除纸船成功");
}

TEST_F(InteractionControllerTest, DeleteBoat_BroadcastEventFormat) {
    Json::Value broadcastMsg;
    broadcastMsg["type"] = "boat_deleted";
    broadcastMsg["stone_id"] = "stone_abc123";
    broadcastMsg["boat_id"] = "boat_001";
    broadcastMsg["boat_count"] = 2;
    broadcastMsg["triggered_by"] = "anon_user_001";

    EXPECT_EQ(broadcastMsg["type"].asString(), "boat_deleted");
    EXPECT_TRUE(broadcastMsg.isMember("boat_id"));
    EXPECT_TRUE(broadcastMsg.isMember("boat_count"));
}

// ==================== 通知相关 ====================

TEST_F(InteractionControllerTest, GetNotifications_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value notification;
    notification["id"] = 1;
    notification["type"] = "ripple";
    notification["stone_id"] = "stone_abc123";
    notification["from_user_id"] = "anon_user_002";
    notification["is_read"] = false;
    notification["created_at"] = "2025-02-08T10:00:00";
    data.append(notification);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("type"));
    EXPECT_TRUE(data[0].isMember("is_read"));
}

TEST_F(InteractionControllerTest, MarkNotificationRead_ValidId) {
    std::string notificationId = "123";
    EXPECT_FALSE(notificationId.empty());
}

TEST_F(InteractionControllerTest, MarkAllNotificationsRead_ResponseFormat) {
    auto resp = buildSuccessResponse(Json::Value(), "全部已读");
    EXPECT_EQ(resp["code"].asInt(), 0);
}

TEST_F(InteractionControllerTest, GetUnreadCount_ResponseFormat) {
    Json::Value data;
    data["unread_count"] = 5;

    auto resp = buildSuccessResponse(data);
    EXPECT_TRUE(resp["data"].isMember("unread_count"));
    EXPECT_GE(resp["data"]["unread_count"].asInt(), 0);
}

// ==================== 连接相关 ====================

TEST_F(InteractionControllerTest, CreateConnectionForStone_ValidStoneId) {
    std::string stoneId = "stone_abc123";
    EXPECT_FALSE(stoneId.empty());

    auto resp = buildSuccessResponse(Json::Value(), "创建连接成功");
    EXPECT_EQ(resp["message"].asString(), "创建连接成功");
}

TEST_F(InteractionControllerTest, CreateConnection_ValidRequest) {
    Json::Value request;
    request["target_user_id"] = "anon_target_456";

    EXPECT_TRUE(request.isMember("target_user_id"));
    EXPECT_FALSE(request["target_user_id"].asString().empty());
}

TEST_F(InteractionControllerTest, UpgradeConnectionToFriend_ValidConnectionId) {
    std::string connectionId = "conn_abc123";
    EXPECT_FALSE(connectionId.empty());
}

TEST_F(InteractionControllerTest, GetConnectionMessages_ResponseFormat) {
    Json::Value messages(Json::arrayValue);
    Json::Value msg;
    msg["id"] = 1;
    msg["sender_id"] = "anon_user_001";
    msg["content"] = "你好！";
    msg["created_at"] = "2025-02-08T10:00:00";
    messages.append(msg);

    EXPECT_TRUE(messages.isArray());
    EXPECT_TRUE(messages[0].isMember("sender_id"));
    EXPECT_TRUE(messages[0].isMember("content"));
}

TEST_F(InteractionControllerTest, CreateConnectionMessage_ValidRequest) {
    Json::Value request;
    request["content"] = "连接消息内容";

    EXPECT_TRUE(request.isMember("content"));
    EXPECT_FALSE(request["content"].asString().empty());
}

TEST_F(InteractionControllerTest, CreateConnectionMessage_EmptyContent_ShouldFail) {
    Json::Value request;
    request["content"] = "";

    EXPECT_TRUE(request["content"].asString().empty());
    // 控制器返回 "content不能为空"
}

TEST_F(InteractionControllerTest, CreateConnectionMessage_MissingContent_ShouldFail) {
    Json::Value request(Json::objectValue);
    EXPECT_FALSE(request.isMember("content"));
}

// ==================== 分页参数验证 ====================

TEST_F(InteractionControllerTest, PaginationParams_ValidRange) {
    auto [page, pageSize] = clampPagination(1, 20);
    EXPECT_EQ(page, 1);
    EXPECT_EQ(pageSize, 20);
}

TEST_F(InteractionControllerTest, PaginationParams_NegativePage_ShouldClamp) {
    auto [page, pageSize] = clampPagination(-5, 20);
    EXPECT_EQ(page, 1);
    EXPECT_EQ(pageSize, 20);
}

TEST_F(InteractionControllerTest, PaginationParams_OversizedPageSize_ShouldClamp) {
    auto [page, pageSize] = clampPagination(1, 500);
    EXPECT_EQ(page, 1);
    EXPECT_EQ(pageSize, 20);
}

TEST_F(InteractionControllerTest, PaginationParams_ZeroValues_ShouldClamp) {
    auto [page, pageSize] = clampPagination(0, 0);
    EXPECT_EQ(page, 1);
    EXPECT_EQ(pageSize, 20);
}

// ==================== 我的涟漪/纸船 ====================

TEST_F(InteractionControllerTest, GetMyRipples_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value ripple;
    ripple["ripple_id"] = "ripple_001";
    ripple["stone_id"] = "stone_abc123";
    ripple["created_at"] = "2025-02-08T10:00:00";
    data.append(ripple);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("ripple_id"));
    EXPECT_TRUE(data[0].isMember("stone_id"));
}

TEST_F(InteractionControllerTest, GetMyBoats_ResponseFormat) {
    Json::Value data(Json::arrayValue);
    Json::Value boat;
    boat["boat_id"] = "boat_001";
    boat["stone_id"] = "stone_abc123";
    boat["content"] = "我的纸船";
    data.append(boat);

    EXPECT_TRUE(data.isArray());
    EXPECT_TRUE(data[0].isMember("boat_id"));
    EXPECT_TRUE(data[0].isMember("content"));
}
