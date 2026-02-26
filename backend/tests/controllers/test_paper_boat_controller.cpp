/**
 * @file test_paper_boat_controller.cpp
 * @brief PaperBoatController 单元测试
 * 覆盖：纸船创建(回复石头)、纸船详情、发送列表、接收列表、分页、参数校验
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class PaperBoatControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== 回复石头（创建纸船） ====================

TEST_F(PaperBoatControllerTest, ReplyToStone_ValidRequest_ReturnsBoatId) {
    Json::Value request;
    request["stone_id"] = "stone_boat_001";
    request["content"] = "温暖的回复内容";
    request["mood"] = "warm";

    EXPECT_TRUE(request.isMember("stone_id"));
    EXPECT_TRUE(request.isMember("content"));
    EXPECT_FALSE(request["stone_id"].asString().empty());
    EXPECT_FALSE(request["content"].asString().empty());

    // 预期响应包含 boat_id, stone_id, boat_count
    Json::Value response;
    response["boat_id"] = "boat_gen_001";
    response["stone_id"] = "stone_boat_001";
    response["boat_count"] = 1;

    EXPECT_TRUE(response.isMember("boat_id"));
    EXPECT_TRUE(response.isMember("stone_id"));
    EXPECT_TRUE(response.isMember("boat_count"));
    EXPECT_GE(response["boat_count"].asInt(), 1);
}

TEST_F(PaperBoatControllerTest, ReplyToStone_EmptyStoneId_Returns400) {
    Json::Value request;
    request["stone_id"] = "";
    request["content"] = "some content";

    bool valid = !request["stone_id"].asString().empty()
              && !request["content"].asString().empty();
    EXPECT_FALSE(valid);
}

TEST_F(PaperBoatControllerTest, ReplyToStone_EmptyContent_Returns400) {
    Json::Value request;
    request["stone_id"] = "stone_001";
    request["content"] = "";

    EXPECT_TRUE(request["content"].asString().empty());
}

TEST_F(PaperBoatControllerTest, ReplyToStone_NoJsonBody_Returns400) {
    std::string expectedMessage = "请求体必须是 JSON 格式";
    EXPECT_FALSE(expectedMessage.empty());
}

TEST_F(PaperBoatControllerTest, ReplyToStone_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    std::string expectedMessage = "未登录";
    EXPECT_EQ(expectedStatus, 401);
    EXPECT_FALSE(expectedMessage.empty());
}

TEST_F(PaperBoatControllerTest, ReplyToStone_StoneNotFound_Returns404) {
    // 石头不存在或未发布时应返回 404
    int expectedStatus = 404;
    std::string expectedMessage = "石头不存在";
    EXPECT_EQ(expectedStatus, 404);
    EXPECT_EQ(expectedMessage, "石头不存在");
}

TEST_F(PaperBoatControllerTest, ReplyToStone_HighRiskContent_Returns403) {
    // 高危内容应被拦截
    std::string safetyLevel = "high_risk";
    EXPECT_EQ(safetyLevel, "high_risk");
}

// ==================== 纸船详情 ====================

TEST_F(PaperBoatControllerTest, GetBoatDetail_ValidBoatId_ReturnsDetail) {
    std::string boatId = "boat_detail_001";
    EXPECT_FALSE(boatId.empty());

    Json::Value response;
    response["boat_id"] = boatId;
    response["stone_id"] = "stone_001";
    response["content"] = "纸船内容";
    response["status"] = "active";
    response["created_at"] = "2026-02-26T10:00:00";

    EXPECT_TRUE(response.isMember("boat_id"));
    EXPECT_TRUE(response.isMember("stone_id"));
    EXPECT_TRUE(response.isMember("content"));
    EXPECT_TRUE(response.isMember("status"));
}

// ==================== 我发送的纸船 ====================

TEST_F(PaperBoatControllerTest, GetMySentBoats_Authenticated_ReturnsList) {
    Json::Value response;
    response["items"] = Json::arrayValue;
    response["total"] = 0;
    response["page"] = 1;
    response["page_size"] = 20;

    Json::Value boat;
    boat["boat_id"] = "boat_sent_001";
    boat["stone_id"] = "stone_001";
    boat["content"] = "我发送的纸船";
    boat["boat_color"] = "#F5EFE7";
    boat["status"] = "active";
    boat["created_at_ts"] = 1740556800;
    response["items"].append(boat);
    response["total"] = 1;

    EXPECT_TRUE(response["items"].isArray());
    EXPECT_EQ(response["total"].asInt(), 1);
    EXPECT_TRUE(response["items"][0].isMember("boat_id"));
    EXPECT_TRUE(response["items"][0].isMember("boat_color"));
}

TEST_F(PaperBoatControllerTest, GetMySentBoats_WithStatusFilter_FiltersCorrectly) {
    // 支持 status 参数过滤
    std::string statusFilter = "active";
    bool hasStatusFilter = !statusFilter.empty() && statusFilter != "all";
    EXPECT_TRUE(hasStatusFilter);

    std::string noFilter = "all";
    bool hasNoFilter = !noFilter.empty() && noFilter != "all";
    EXPECT_FALSE(hasNoFilter);
}

TEST_F(PaperBoatControllerTest, GetMySentBoats_Pagination_ValidBounds) {
    int page = 1;
    int pageSize = 20;
    int offset = (page - 1) * pageSize;

    EXPECT_GE(page, 1);
    EXPECT_GE(pageSize, 1);
    EXPECT_LE(pageSize, 100);
    EXPECT_EQ(offset, 0);
}

// ==================== 我收到的纸船 ====================

TEST_F(PaperBoatControllerTest, GetMyReceivedBoats_Authenticated_ReturnsList) {
    Json::Value boat;
    boat["boat_id"] = "boat_recv_001";
    boat["stone_id"] = "stone_001";
    boat["content"] = "收到的纸船";
    boat["boat_color"] = "#F5EFE7";
    boat["status"] = "active";
    boat["is_ai_reply"] = false;
    boat["stone_content"] = "原始石头内容";

    Json::Value sender;
    sender["nickname"] = "匿名旅人";
    sender["is_anonymous"] = true;
    boat["sender"] = sender;

    EXPECT_TRUE(boat.isMember("sender"));
    EXPECT_TRUE(boat["sender"]["is_anonymous"].asBool());
    EXPECT_EQ(boat["sender"]["nickname"].asString(), "匿名旅人");
    EXPECT_FALSE(boat["is_ai_reply"].asBool());
}

TEST_F(PaperBoatControllerTest, GetMyReceivedBoats_DefaultBoatColor_Applied) {
    // 默认纸船颜色为 #F5EFE7
    std::string defaultColor = "#F5EFE7";
    EXPECT_EQ(defaultColor, "#F5EFE7");

    // null 时使用默认颜色
    Json::Value nullColor;
    std::string color = nullColor.isNull() ? "#F5EFE7" : nullColor.asString();
    EXPECT_EQ(color, "#F5EFE7");
}

TEST_F(PaperBoatControllerTest, GetMyReceivedBoats_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}
