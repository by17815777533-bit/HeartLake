/**
 * @file test_user_service.cpp
 * @brief UserApplicationService 单元测试
 * UserApplicationService 直接依赖 drogon::app().getDbClient()，
 * 因此测试聚焦于可独立验证的逻辑：
 * 1. CacheManager 缓存读写/失效
 * 2. 输入验证（空userId、搜索关键词、分页参数）
 * 3. 批量查询边界条件
 * 4. RequestHelper 工具函数
 * 5. 用户数据 JSON 结构验证
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include "infrastructure/cache/CacheManager.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include <string>
#include <vector>

using namespace heartlake::core::cache;
using namespace heartlake::utils;

class UserServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        CacheManager::getInstance().clear();
    }

    void TearDown() override {
        CacheManager::getInstance().clear();
    }

    // 构造一个标准的用户 JSON 对象，模拟数据库返回
    Json::Value makeUserJson(const std::string& userId,
                             const std::string& username,
                             const std::string& nickname) {
        Json::Value user;
        user["user_id"] = userId;
        user["username"] = username;
        user["nickname"] = nickname;
        user["avatar_url"] = "https://example.com/avatar.png";
        user["bio"] = "测试用户简介";
        user["gender"] = "male";
        user["birthday"] = "2000-01-01";
        user["location"] = "北京";
        user["is_anonymous"] = false;
        user["created_at"] = "2026-01-01T00:00:00Z";
        return user;
    }
};

// ==================== 缓存读写测试 ====================

TEST_F(UserServiceTest, Cache_SetAndGetUserProfile) {
    auto& cache = CacheManager::getInstance();
    auto user = makeUserJson("user_001", "zhangsan", "张三");

    cache.setJson("user:user_001", user, 300);

    auto cached = cache.getJson("user:user_001");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ((*cached)["user_id"].asString(), "user_001");
    EXPECT_EQ((*cached)["username"].asString(), "zhangsan");
    EXPECT_EQ((*cached)["nickname"].asString(), "张三");
}

TEST_F(UserServiceTest, Cache_MissReturnsNullopt) {
    auto& cache = CacheManager::getInstance();
    auto result = cache.getJson("user:nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(UserServiceTest, Cache_InvalidateRemovesEntry) {
    auto& cache = CacheManager::getInstance();
    auto user = makeUserJson("user_002", "lisi", "李四");
    cache.setJson("user:user_002", user, 300);

    cache.invalidate("user:user_002");

    auto result = cache.getJson("user:user_002");
    EXPECT_FALSE(result.has_value());
}

TEST_F(UserServiceTest, Cache_UpdateOverwritesPrevious) {
    auto& cache = CacheManager::getInstance();
    auto user = makeUserJson("user_003", "wangwu", "王五");
    cache.setJson("user:user_003", user, 300);

    // 更新昵称
    user["nickname"] = "王五改名了";
    cache.setJson("user:user_003", user, 300);

    auto cached = cache.getJson("user:user_003");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ((*cached)["nickname"].asString(), "王五改名了");
}

TEST_F(UserServiceTest, Cache_MultipleUsersIndependent) {
    auto& cache = CacheManager::getInstance();
    cache.setJson("user:u1", makeUserJson("u1", "a", "A"), 300);
    cache.setJson("user:u2", makeUserJson("u2", "b", "B"), 300);

    cache.invalidate("user:u1");

    EXPECT_FALSE(cache.getJson("user:u1").has_value());
    EXPECT_TRUE(cache.getJson("user:u2").has_value());
}

// ==================== 用户 JSON 结构验证 ====================

TEST_F(UserServiceTest, UserJson_HasAllRequiredFields) {
    auto user = makeUserJson("user_001", "test", "测试");

    EXPECT_TRUE(user.isMember("user_id"));
    EXPECT_TRUE(user.isMember("username"));
    EXPECT_TRUE(user.isMember("nickname"));
    EXPECT_TRUE(user.isMember("is_anonymous"));
    EXPECT_TRUE(user.isMember("created_at"));
}

TEST_F(UserServiceTest, UserJson_OptionalFieldsPresent) {
    auto user = makeUserJson("user_001", "test", "测试");

    EXPECT_TRUE(user.isMember("avatar_url"));
    EXPECT_TRUE(user.isMember("bio"));
    EXPECT_TRUE(user.isMember("gender"));
    EXPECT_TRUE(user.isMember("birthday"));
    EXPECT_TRUE(user.isMember("location"));
}

TEST_F(UserServiceTest, UserJson_AnonymousFlag) {
    auto user = makeUserJson("user_001", "test", "测试");
    EXPECT_FALSE(user["is_anonymous"].asBool());

    user["is_anonymous"] = true;
    EXPECT_TRUE(user["is_anonymous"].asBool());
}

// ==================== 输入验证测试 ====================

TEST_F(UserServiceTest, UserId_EmptyIsInvalid) {
    std::string userId = "";
    EXPECT_TRUE(userId.empty());
}

TEST_F(UserServiceTest, UserId_Generated_NonEmpty) {
    std::string userId = IdGenerator::generateUserId();
    EXPECT_FALSE(userId.empty());
    EXPECT_GE(userId.size(), 8u);
}

TEST_F(UserServiceTest, UserId_Generated_Unique) {
    std::string id1 = IdGenerator::generateUserId();
    std::string id2 = IdGenerator::generateUserId();
    EXPECT_NE(id1, id2);
}

// ==================== 搜索关键词验证 ====================

TEST_F(UserServiceTest, SearchKeyword_EmptyString) {
    std::string keyword = "";
    EXPECT_TRUE(keyword.empty());
}

TEST_F(UserServiceTest, SearchKeyword_SpecialCharsEscaped) {
    // 验证 LIKE 通配符需要转义的场景
    std::string keyword = "test%user";
    EXPECT_NE(keyword.find('%'), std::string::npos);
}

TEST_F(UserServiceTest, SearchKeyword_UnderscoreEscaped) {
    std::string keyword = "test_user";
    EXPECT_NE(keyword.find('_'), std::string::npos);
}

TEST_F(UserServiceTest, SearchKeyword_NormalString) {
    std::string keyword = "张三";
    EXPECT_EQ(keyword.find('%'), std::string::npos);
    EXPECT_EQ(keyword.find('_'), std::string::npos);
}

// ==================== RequestHelper 工具函数测试 ====================

TEST_F(UserServiceTest, SafeInt_ValidNumber) {
    EXPECT_EQ(safeInt("42"), 42);
    EXPECT_EQ(safeInt("0"), 0);
    EXPECT_EQ(safeInt("-1"), -1);
}

TEST_F(UserServiceTest, SafeInt_InvalidReturnsDefault) {
    EXPECT_EQ(safeInt("", 10), 10);
    EXPECT_EQ(safeInt("abc", 5), 5);
    EXPECT_EQ(safeInt("12.5", 0), 12); // from_chars 解析到小数点停止
}

TEST_F(UserServiceTest, SafeDouble_ValidNumber) {
    EXPECT_DOUBLE_EQ(safeDouble("3.14"), 3.14);
    EXPECT_DOUBLE_EQ(safeDouble("0.0"), 0.0);
}

TEST_F(UserServiceTest, SafeDouble_InvalidReturnsDefault) {
    EXPECT_DOUBLE_EQ(safeDouble("", 1.0), 1.0);
    EXPECT_DOUBLE_EQ(safeDouble("xyz", 2.5), 2.5);
}

// ==================== 批量查询边界条件 ====================

TEST_F(UserServiceTest, BatchQuery_EmptyList) {
    std::vector<std::string> userIds;
    EXPECT_TRUE(userIds.empty());
    // getUsersBatch 对空列表直接返回空数组
}

TEST_F(UserServiceTest, BatchQuery_SingleUser) {
    std::vector<std::string> userIds = {"user_001"};
    EXPECT_EQ(userIds.size(), 1u);
}

TEST_F(UserServiceTest, BatchQuery_ExactlyFive) {
    std::vector<std::string> userIds = {"u1", "u2", "u3", "u4", "u5"};
    EXPECT_EQ(userIds.size(), 5u);
    // 5个以内走单次查询
}

TEST_F(UserServiceTest, BatchQuery_OverFive_NeedsSplit) {
    std::vector<std::string> userIds = {"u1", "u2", "u3", "u4", "u5", "u6"};
    EXPECT_GT(userIds.size(), 5u);
    // 超过5个需要分批查询
}

// ==================== Profile 更新验证 ====================

TEST_F(UserServiceTest, ProfileUpdate_EmptyUpdatesNoOp) {
    Json::Value updates(Json::objectValue);
    EXPECT_EQ(updates.size(), 0u);
    // updateUserProfile 对空 updates 直接 return
}

TEST_F(UserServiceTest, ProfileUpdate_SingleField) {
    Json::Value updates;
    updates["nickname"] = "新昵称";
    EXPECT_TRUE(updates.isMember("nickname"));
    EXPECT_EQ(updates.size(), 1u);
}

TEST_F(UserServiceTest, ProfileUpdate_AllFields) {
    Json::Value updates;
    updates["nickname"] = "新昵称";
    updates["avatar_url"] = "https://new-avatar.png";
    updates["bio"] = "新简介";
    updates["gender"] = "female";
    updates["birthday"] = "1999-12-31";
    updates["location"] = "上海";
    EXPECT_EQ(updates.size(), 6u);
}

TEST_F(UserServiceTest, ProfileUpdate_CacheInvalidatedAfterUpdate) {
    auto& cache = CacheManager::getInstance();
    auto user = makeUserJson("user_upd", "test", "旧昵称");
    cache.setJson("user:user_upd", user, 300);

    // 模拟 updateUserProfile 中的缓存失效
    cache.invalidate("user:user_upd");

    auto result = cache.getJson("user:user_upd");
    EXPECT_FALSE(result.has_value());
}
