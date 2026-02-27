/**
 * SecurityAuditScore 单元测试 - 审计评分、检查项、风险等级
 */

#include <gtest/gtest.h>
#include "utils/SecurityAuditScore.h"
#include <json/json.h>
#include <string>

using namespace heartlake::utils;

// =====================================================================
// SecurityAuditScore 测试
// =====================================================================

class SecurityAuditScoreTest : public ::testing::Test {
protected:
    SecurityAuditScore& auditor = SecurityAuditScore::getInstance();
};

// -------------------- 单例 --------------------

TEST_F(SecurityAuditScoreTest, Singleton_SameInstance) {
    auto& a = SecurityAuditScore::getInstance();
    auto& b = SecurityAuditScore::getInstance();
    EXPECT_EQ(&a, &b);
}

// -------------------- runAudit 基本结构 --------------------

TEST_F(SecurityAuditScoreTest, RunAudit_ReturnsValidJson) {
    auto result = auditor.runAudit();
    EXPECT_TRUE(result.isMember("score"));
    EXPECT_TRUE(result.isMember("checks"));
    EXPECT_TRUE(result.isMember("passed_count"));
    EXPECT_TRUE(result.isMember("total_count"));
}

TEST_F(SecurityAuditScoreTest, RunAudit_ChecksIsArray) {
    auto result = auditor.runAudit();
    EXPECT_TRUE(result["checks"].isArray());
    EXPECT_GT(result["checks"].size(), 0u);
}

TEST_F(SecurityAuditScoreTest, RunAudit_TotalCountEquals12) {
    auto result = auditor.runAudit();
    // 12个安全检查项
    EXPECT_EQ(result["total_count"].asInt(), 12);
}

// -------------------- 评分计算 --------------------

TEST_F(SecurityAuditScoreTest, Score_IsNonNegative) {
    auditor.runAudit();
    EXPECT_GE(auditor.getScore(), 0.0);
}

TEST_F(SecurityAuditScoreTest, Score_MaxIs10) {
    auditor.runAudit();
    EXPECT_LE(auditor.getScore(), 10.0);
}

TEST_F(SecurityAuditScoreTest, Score_UpdatedAfterRunAudit) {
    auto result = auditor.runAudit();
    double scoreFromJson = result["score"].asDouble();
    double scoreFromGetter = auditor.getScore();
    EXPECT_DOUBLE_EQ(scoreFromJson, scoreFromGetter);
}

// -------------------- 检查项结构验证 --------------------

TEST_F(SecurityAuditScoreTest, CheckItem_HasRequiredFields) {
    auto result = auditor.runAudit();
    for (const auto& item : result["checks"]) {
        EXPECT_TRUE(item.isMember("name")) << "缺少 name 字段";
        EXPECT_TRUE(item.isMember("category")) << "缺少 category 字段";
        EXPECT_TRUE(item.isMember("passed")) << "缺少 passed 字段";
        EXPECT_TRUE(item.isMember("detail")) << "缺少 detail 字段";
    }
}

TEST_F(SecurityAuditScoreTest, CheckItem_NameNotEmpty) {
    auto result = auditor.runAudit();
    for (const auto& item : result["checks"]) {
        EXPECT_FALSE(item["name"].asString().empty());
    }
}

TEST_F(SecurityAuditScoreTest, CheckItem_CategoryNotEmpty) {
    auto result = auditor.runAudit();
    for (const auto& item : result["checks"]) {
        EXPECT_FALSE(item["category"].asString().empty());
    }
}

// -------------------- PASETO 配置检查 --------------------

TEST_F(SecurityAuditScoreTest, PasetoCheck_NoKey_NotPassed) {
    // 确保没有设置 PASETO_KEY
    unsetenv("PASETO_KEY");
    auto result = auditor.runAudit();
    // 找到 PASETO 检查项
    bool found = false;
    for (const auto& item : result["checks"]) {
        if (item["name"].asString().find("PASETO") != std::string::npos) {
            EXPECT_FALSE(item["passed"].asBool());
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "未找到 PASETO 检查项";
}

TEST_F(SecurityAuditScoreTest, PasetoCheck_ShortKey_NotPassed) {
    setenv("PASETO_KEY", "short", 1);
    auto result = auditor.runAudit();
    for (const auto& item : result["checks"]) {
        if (item["name"].asString().find("PASETO") != std::string::npos) {
            EXPECT_FALSE(item["passed"].asBool());
            break;
        }
    }
    unsetenv("PASETO_KEY");
}

TEST_F(SecurityAuditScoreTest, PasetoCheck_ValidKey_Passed) {
    // 设置一个足够长的密钥 (>=32字符)
    setenv("PASETO_KEY", "abcdefghijklmnopqrstuvwxyz123456", 1);
    auto result = auditor.runAudit();
    for (const auto& item : result["checks"]) {
        if (item["name"].asString().find("PASETO") != std::string::npos) {
            EXPECT_TRUE(item["passed"].asBool());
            break;
        }
    }
    unsetenv("PASETO_KEY");
}

// -------------------- 多次审计一致性 --------------------

TEST_F(SecurityAuditScoreTest, MultipleRuns_ConsistentResults) {
    auto result1 = auditor.runAudit();
    auto result2 = auditor.runAudit();
    EXPECT_EQ(result1["total_count"].asInt(), result2["total_count"].asInt());
    // 在相同环境下，分数应一致
    EXPECT_DOUBLE_EQ(result1["score"].asDouble(), result2["score"].asDouble());
}

// =====================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
