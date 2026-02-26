/**
 * @file test_response_util.cpp
 * @brief ResponseUtil 单元测试 - 响应格式、状态码映射、分页
 */

#include <gtest/gtest.h>
#include "utils/ResponseUtil.h"
#include "utils/ErrorCode.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <string>

using namespace heartlake::utils;

// =====================================================================
// ResponseUtil 测试
// =====================================================================

class ResponseUtilTest : public ::testing::Test {
protected:
    // 从 HttpResponse 中提取 JSON body
    Json::Value parseBody(const drogon::HttpResponsePtr& resp) {
        auto body = std::string(resp->body());
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream stream(body);
        Json::parseFromStream(builder, stream, &root, &errs);
        return root;
    }
};

// -------------------- success --------------------

TEST_F(ResponseUtilTest, Success_DefaultMessage) {
    auto resp = ResponseUtil::success();
    EXPECT_EQ(resp->statusCode(), drogon::k200OK);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 0);
    EXPECT_EQ(body["message"].asString(), "Success");
    EXPECT_TRUE(body.isMember("data"));
}

TEST_F(ResponseUtilTest, Success_WithData) {
    Json::Value data;
    data["id"] = 42;
    data["name"] = "test";

    auto resp = ResponseUtil::success(data, "操作成功");
    auto body = parseBody(resp);

    EXPECT_EQ(body["code"].asInt(), 0);
    EXPECT_EQ(body["message"].asString(), "操作成功");
    EXPECT_EQ(body["data"]["id"].asInt(), 42);
    EXPECT_EQ(body["data"]["name"].asString(), "test");
}

TEST_F(ResponseUtilTest, Success_NullData_ReturnsEmptyObject) {
    auto resp = ResponseUtil::success(Json::Value());
    auto body = parseBody(resp);

    // data 应该是空对象而非 null
    EXPECT_TRUE(body["data"].isObject());
}

// -------------------- error (ErrorCode) --------------------

TEST_F(ResponseUtilTest, Error_WithErrorCode_HasCorrectCode) {
    auto resp = ResponseUtil::error(ErrorCode::UNAUTHORIZED);
    auto body = parseBody(resp);

    EXPECT_NE(body["code"].asInt(), 0);
    EXPECT_EQ(body["code"].asInt(), static_cast<int>(ErrorCode::UNAUTHORIZED));
}

TEST_F(ResponseUtilTest, Error_WithDetail_OverridesDefaultMessage) {
    auto resp = ResponseUtil::error(ErrorCode::INVALID_REQUEST, "自定义错误信息");
    auto body = parseBody(resp);

    EXPECT_EQ(body["message"].asString(), "自定义错误信息");
}

TEST_F(ResponseUtilTest, Error_WithExtra_IncludesExtraData) {
    Json::Value extra;
    extra["field"] = "username";
    auto resp = ResponseUtil::error(ErrorCode::INVALID_PARAMETER, "参数无效", extra);
    auto body = parseBody(resp);

    EXPECT_EQ(body["data"]["field"].asString(), "username");
}

// -------------------- error (int httpStatus) --------------------

TEST_F(ResponseUtilTest, Error_CustomHttpStatus_400) {
    auto resp = ResponseUtil::error(400, "Bad Request");
    EXPECT_EQ(resp->statusCode(), drogon::k400BadRequest);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 400);
    EXPECT_EQ(body["message"].asString(), "Bad Request");
}

TEST_F(ResponseUtilTest, Error_CustomHttpStatus_500) {
    auto resp = ResponseUtil::error(500, "Internal Error");
    EXPECT_EQ(resp->statusCode(), drogon::k500InternalServerError);
}

// -------------------- paged --------------------

TEST_F(ResponseUtilTest, Paged_HasCorrectStructure) {
    Json::Value items(Json::arrayValue);
    Json::Value item1;
    item1["id"] = 1;
    items.append(item1);
    Json::Value item2;
    item2["id"] = 2;
    items.append(item2);

    auto resp = ResponseUtil::paged(items, 100, 1, 10);
    EXPECT_EQ(resp->statusCode(), drogon::k200OK);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 0);
    EXPECT_EQ(body["data"]["total"].asInt(), 100);
    EXPECT_EQ(body["data"]["page"].asInt(), 1);
    EXPECT_EQ(body["data"]["pageSize"].asInt(), 10);
    EXPECT_EQ(body["data"]["totalPages"].asInt(), 10);
    EXPECT_EQ(body["data"]["items"].size(), 2u);
}

TEST_F(ResponseUtilTest, Paged_TotalPages_CeilDivision) {
    Json::Value items(Json::arrayValue);
    // 101 条记录，每页 10 条 → 11 页
    auto resp = ResponseUtil::paged(items, 101, 1, 10);
    auto body = parseBody(resp);
    EXPECT_EQ(body["data"]["totalPages"].asInt(), 11);
}

TEST_F(ResponseUtilTest, Paged_EmptyItems) {
    Json::Value items(Json::arrayValue);
    auto resp = ResponseUtil::paged(items, 0, 1, 10);
    auto body = parseBody(resp);

    EXPECT_EQ(body["data"]["total"].asInt(), 0);
    EXPECT_EQ(body["data"]["totalPages"].asInt(), 0);
    EXPECT_EQ(body["data"]["items"].size(), 0u);
}

// -------------------- 便捷方法 --------------------

TEST_F(ResponseUtilTest, Created_Returns201) {
    auto resp = ResponseUtil::created();
    EXPECT_EQ(resp->statusCode(), drogon::k201Created);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 0);
    EXPECT_EQ(body["message"].asString(), "Created");
}

TEST_F(ResponseUtilTest, BadRequest_Returns400) {
    auto resp = ResponseUtil::badRequest("参数错误");
    EXPECT_EQ(resp->statusCode(), drogon::k400BadRequest);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 400);
    EXPECT_EQ(body["message"].asString(), "参数错误");
}

TEST_F(ResponseUtilTest, Unauthorized_Returns401) {
    auto resp = ResponseUtil::unauthorized();
    EXPECT_EQ(resp->statusCode(), drogon::k401Unauthorized);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 401);
}

TEST_F(ResponseUtilTest, Forbidden_Returns403) {
    auto resp = ResponseUtil::forbidden();
    EXPECT_EQ(resp->statusCode(), drogon::k403Forbidden);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 403);
}

TEST_F(ResponseUtilTest, NotFound_Returns404) {
    auto resp = ResponseUtil::notFound("找不到资源");
    EXPECT_EQ(resp->statusCode(), drogon::k404NotFound);

    auto body = parseBody(resp);
    EXPECT_EQ(body["message"].asString(), "找不到资源");
}

TEST_F(ResponseUtilTest, Conflict_Returns409) {
    auto resp = ResponseUtil::conflict();
    EXPECT_EQ(resp->statusCode(), drogon::k409Conflict);
}

TEST_F(ResponseUtilTest, TooManyRequests_Returns429) {
    auto resp = ResponseUtil::tooManyRequests();
    EXPECT_EQ(resp->statusCode(), drogon::k429TooManyRequests);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 429);
}

TEST_F(ResponseUtilTest, InternalError_Returns500) {
    auto resp = ResponseUtil::internalError();
    EXPECT_EQ(resp->statusCode(), drogon::k500InternalServerError);

    auto body = parseBody(resp);
    EXPECT_EQ(body["code"].asInt(), 500);
}

// -------------------- noContent --------------------

TEST_F(ResponseUtilTest, NoContent_Returns204) {
    auto resp = ResponseUtil::noContent();
    EXPECT_EQ(resp->statusCode(), drogon::k204NoContent);
}

// -------------------- 默认消息 --------------------

TEST_F(ResponseUtilTest, BadRequest_DefaultMessage) {
    auto resp = ResponseUtil::badRequest();
    auto body = parseBody(resp);
    EXPECT_EQ(body["message"].asString(), "请求参数错误");
}

TEST_F(ResponseUtilTest, Unauthorized_DefaultMessage) {
    auto resp = ResponseUtil::unauthorized();
    auto body = parseBody(resp);
    EXPECT_EQ(body["message"].asString(), "未授权");
}

TEST_F(ResponseUtilTest, Forbidden_DefaultMessage) {
    auto resp = ResponseUtil::forbidden();
    auto body = parseBody(resp);
    EXPECT_EQ(body["message"].asString(), "禁止访问");
}

TEST_F(ResponseUtilTest, NotFound_DefaultMessage) {
    auto resp = ResponseUtil::notFound();
    auto body = parseBody(resp);
    EXPECT_EQ(body["message"].asString(), "资源不存在");
}

TEST_F(ResponseUtilTest, InternalError_DefaultMessage) {
    auto resp = ResponseUtil::internalError();
    auto body = parseBody(resp);
    EXPECT_EQ(body["message"].asString(), "服务器内部错误");
}

// =====================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
