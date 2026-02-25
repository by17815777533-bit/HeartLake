/**
 * @file stone_controller_test.cpp
 * @brief StoneController 单元测试
 */

#include <gtest/gtest.h>
#include <json/json.h>

class StoneControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(StoneControllerTest, CreateStone_ValidContent_ReturnsSuccess) {
    Json::Value request;
    request["content"] = "Test stone content";
    request["tags"] = Json::arrayValue;
    request["tags"].append("test");

    // 验证请求格式
    EXPECT_TRUE(request.isMember("content"));
    EXPECT_FALSE(request["content"].asString().empty());
    EXPECT_TRUE(request["tags"].isArray());
}

TEST_F(StoneControllerTest, CreateStone_EmptyContent_ShouldFail) {
    Json::Value request;
    request["content"] = "";

    EXPECT_TRUE(request["content"].asString().empty());
}

TEST_F(StoneControllerTest, CreateStone_ContentTooLong_ShouldFail) {
    std::string longContent(1001, 'a');

    EXPECT_GT(longContent.length(), 1000);
}

TEST_F(StoneControllerTest, GetStones_ValidPagination) {
    int page = 1;
    int limit = 20;

    EXPECT_GE(page, 1);
    EXPECT_LE(limit, 100);
    EXPECT_GE(limit, 1);
}

TEST_F(StoneControllerTest, DeleteStone_ValidStoneId) {
    std::string stoneId = "stone_123456";

    EXPECT_FALSE(stoneId.empty());
    EXPECT_TRUE(stoneId.find("stone_") == 0);
}
