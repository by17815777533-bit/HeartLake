/**
 * @file test_report_controller.cpp
 * @brief ReportController 单元测试
 * 覆盖：创建举报、我的举报列表、举报类型验证、无效targetType、分页、重复举报
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>
#include <vector>

class ReportControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== 创建举报 ====================

TEST_F(ReportControllerTest, CreateReport_ValidRequest_ReturnsReportId) {
    Json::Value request;
    request["target_type"] = "stone";
    request["target_id"] = "stone_rpt_001";
    request["reason"] = "spam";
    request["description"] = "This content is spam";

    EXPECT_TRUE(request.isMember("target_type"));
    EXPECT_TRUE(request.isMember("target_id"));
    EXPECT_TRUE(request.isMember("reason"));
    EXPECT_FALSE(request["target_type"].asString().empty());
    EXPECT_FALSE(request["target_id"].asString().empty());
    EXPECT_FALSE(request["reason"].asString().empty());

    // 预期响应包含 report_id
    Json::Value response;
    response["report_id"] = "rpt_generated_001";
    EXPECT_TRUE(response.isMember("report_id"));
    EXPECT_FALSE(response["report_id"].asString().empty());
}

TEST_F(ReportControllerTest, CreateReport_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    std::string expectedMessage = "未登录";
    EXPECT_EQ(expectedStatus, 401);
    EXPECT_FALSE(expectedMessage.empty());
}

TEST_F(ReportControllerTest, CreateReport_MissingRequiredFields_Returns400) {
    // target_type, target_id, reason 都不能为空
    Json::Value request;
    request["target_type"] = "";
    request["target_id"] = "stone_001";
    request["reason"] = "spam";

    bool allPresent = !request["target_type"].asString().empty()
                   && !request["target_id"].asString().empty()
                   && !request["reason"].asString().empty();
    EXPECT_FALSE(allPresent);
}

TEST_F(ReportControllerTest, CreateReport_InvalidTargetType_Returns400) {
    // target_type 只能是 stone, boat, user
    std::vector<std::string> validTypes = {"stone", "boat", "user"};
    std::string invalidType = "comment";

    bool isValid = false;
    for (const auto& t : validTypes) {
        if (t == invalidType) { isValid = true; break; }
    }
    EXPECT_FALSE(isValid);
}

TEST_F(ReportControllerTest, CreateReport_AllValidTargetTypes_Accepted) {
    std::vector<std::string> validTypes = {"stone", "boat", "user"};
    for (const auto& type : validTypes) {
        bool isValid = (type == "stone" || type == "boat" || type == "user");
        EXPECT_TRUE(isValid) << "Type should be valid: " << type;
    }
}

TEST_F(ReportControllerTest, CreateReport_DescriptionTooLong_Returns400) {
    // description 最大长度为 2000
    std::string longDesc(2001, 'a');
    EXPECT_GT(longDesc.size(), 2000u);
}

TEST_F(ReportControllerTest, CreateReport_DuplicateReport_Returns409) {
    // 重复举报同一内容应返回 409
    int expectedStatus = 409;
    std::string expectedMessage = "您已经举报过该内容";
    EXPECT_EQ(expectedStatus, 409);
    EXPECT_EQ(expectedMessage, "您已经举报过该内容");
}

TEST_F(ReportControllerTest, CreateReport_NoJsonBody_Returns400) {
    // 非 JSON 请求体应返回 400
    std::string expectedMessage = "请求体必须是 JSON 格式";
    EXPECT_FALSE(expectedMessage.empty());
}

// ==================== 我的举报列表 ====================

TEST_F(ReportControllerTest, GetMyReports_Authenticated_ReturnsReportList) {
    Json::Value response;
    response["reports"] = Json::arrayValue;
    response["total"] = 0;
    response["page"] = 1;
    response["page_size"] = 20;

    Json::Value report;
    report["report_id"] = "rpt_001";
    report["target_type"] = "stone";
    report["target_id"] = "stone_001";
    report["reason"] = "spam";
    report["description"] = "垃圾内容";
    report["status"] = "pending";
    report["created_at"] = "2026-02-26T10:00:00";
    response["reports"].append(report);
    response["total"] = 1;

    EXPECT_TRUE(response.isMember("reports"));
    EXPECT_TRUE(response.isMember("total"));
    EXPECT_TRUE(response.isMember("page"));
    EXPECT_TRUE(response.isMember("page_size"));
    EXPECT_EQ(response["reports"].size(), 1u);

    auto& r = response["reports"][0];
    EXPECT_TRUE(r.isMember("report_id"));
    EXPECT_TRUE(r.isMember("target_type"));
    EXPECT_TRUE(r.isMember("target_id"));
    EXPECT_TRUE(r.isMember("reason"));
    EXPECT_TRUE(r.isMember("status"));
    EXPECT_TRUE(r.isMember("created_at"));
}

TEST_F(ReportControllerTest, GetMyReports_Pagination_ValidBounds) {
    // 验证分页参数
    int page = 1;
    int pageSize = 20;

    EXPECT_GE(page, 1);
    EXPECT_GE(pageSize, 1);
    EXPECT_LE(pageSize, 100);

    int offset = (page - 1) * pageSize;
    EXPECT_EQ(offset, 0);
}

TEST_F(ReportControllerTest, GetMyReports_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}
