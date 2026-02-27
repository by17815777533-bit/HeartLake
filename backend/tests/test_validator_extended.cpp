/**
 * Validator 扩展单元测试 - URL、手机号、验证码、SQL注入、XSS、路径遍历、sanitizeHtml
 */

#include <gtest/gtest.h>
#include "utils/Validator.h"
#include <json/json.h>
#include <string>
#include <vector>

using namespace heartlake::utils;

class ValidatorExtendedTest : public ::testing::Test {};

// =====================================================================
// URL 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, Url_Empty_Invalid) {
    auto r = Validator::url("", "URL");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_Http_Valid) {
    auto r = Validator::url("http://example.com", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_Https_Valid) {
    auto r = Validator::url("https://example.com", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_WithPort_Valid) {
    auto r = Validator::url("http://example.com:8080", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_WithPath_Valid) {
    auto r = Validator::url("https://example.com/path/to/resource", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_WithQueryParams_Valid) {
    auto r = Validator::url("https://example.com/search?q=hello&page=1", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_WithFragment_Valid) {
    auto r = Validator::url("https://example.com/page#section", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_WithUserInfo_Valid) {
    auto r = Validator::url("https://user:pass@example.com", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_NoProtocol_Invalid) {
    auto r = Validator::url("example.com", "URL");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_FtpProtocol_Invalid) {
    auto r = Validator::url("ftp://example.com", "URL");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_JustProtocol_Invalid) {
    auto r = Validator::url("http://", "URL");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_WithSubdomain_Valid) {
    auto r = Validator::url("https://api.v2.example.com/data", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_WithEncodedChars_Valid) {
    auto r = Validator::url("https://example.com/path%20with%20spaces", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_Localhost_Valid) {
    auto r = Validator::url("http://localhost:3000", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_IpAddress_Valid) {
    auto r = Validator::url("http://192.168.1.1:8080/api", "URL");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_DefaultFieldName) {
    auto r = Validator::url("https://example.com");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Url_LongPath_Valid) {
    auto r = Validator::url("https://example.com/a/b/c/d/e/f/g/h/i/j", "URL");
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// 手机号验证
// =====================================================================

TEST_F(ValidatorExtendedTest, Phone_Empty_Invalid) {
    auto r = Validator::phoneNumber("");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_130) {
    auto r = Validator::phoneNumber("13012345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_131) {
    auto r = Validator::phoneNumber("13112345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_135) {
    auto r = Validator::phoneNumber("13512345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_138) {
    auto r = Validator::phoneNumber("13812345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_139) {
    auto r = Validator::phoneNumber("13912345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_150) {
    auto r = Validator::phoneNumber("15012345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_166) {
    auto r = Validator::phoneNumber("16612345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_175) {
    auto r = Validator::phoneNumber("17512345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_188) {
    auto r = Validator::phoneNumber("18812345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Valid_199) {
    auto r = Validator::phoneNumber("19912345678");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_TooShort_Invalid) {
    auto r = Validator::phoneNumber("1381234567");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_TooLong_Invalid) {
    auto r = Validator::phoneNumber("138123456789");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_StartWith2_Invalid) {
    auto r = Validator::phoneNumber("23812345678");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_StartWith10_Invalid) {
    auto r = Validator::phoneNumber("10012345678");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_StartWith12_Invalid) {
    auto r = Validator::phoneNumber("12012345678");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_WithDashes_Invalid) {
    auto r = Validator::phoneNumber("138-1234-5678");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_WithSpaces_Invalid) {
    auto r = Validator::phoneNumber("138 1234 5678");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_WithCountryCode_Invalid) {
    auto r = Validator::phoneNumber("+8613812345678");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Phone_Letters_Invalid) {
    auto r = Validator::phoneNumber("1381234abcd");
    EXPECT_FALSE(r.isValid);
}

// =====================================================================
// 验证码验证
// =====================================================================

TEST_F(ValidatorExtendedTest, VerificationCode_Empty_Invalid) {
    auto r = Validator::verificationCode("");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_Valid_6Digits) {
    auto r = Validator::verificationCode("123456");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_Valid_AllZeros) {
    auto r = Validator::verificationCode("000000");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_Valid_AllNines) {
    auto r = Validator::verificationCode("999999");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_TooShort_Invalid) {
    auto r = Validator::verificationCode("12345");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_TooLong_Invalid) {
    auto r = Validator::verificationCode("1234567");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_WithLetters_Invalid) {
    auto r = Validator::verificationCode("12345a");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_WithSpaces_Invalid) {
    auto r = Validator::verificationCode("123 56");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_AllLetters_Invalid) {
    auto r = Validator::verificationCode("abcdef");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, VerificationCode_SpecialChars_Invalid) {
    auto r = Validator::verificationCode("12!@#$");
    EXPECT_FALSE(r.isValid);
}

// =====================================================================
// SQL 注入检测
// =====================================================================

TEST_F(ValidatorExtendedTest, SqlInjection_NormalText_Valid) {
    auto r = Validator::checkSqlInjection("正常的用户输入", "字段");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_DropTable_Invalid) {
    auto r = Validator::checkSqlInjection("DROP TABLE users", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_SelectStar_Invalid) {
    auto r = Validator::checkSqlInjection("SELECT * FROM users", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_UnionSelect_Invalid) {
    auto r = Validator::checkSqlInjection("1 UNION SELECT password FROM users", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_DeleteFrom_Invalid) {
    auto r = Validator::checkSqlInjection("DELETE FROM users WHERE 1=1", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_InsertInto_Invalid) {
    auto r = Validator::checkSqlInjection("INSERT INTO users VALUES('hack')", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_UpdateSet_Invalid) {
    auto r = Validator::checkSqlInjection("UPDATE users SET role='admin'", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_DoubleDash_Invalid) {
    auto r = Validator::checkSqlInjection("admin'--", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_BlockComment_Invalid) {
    auto r = Validator::checkSqlInjection("admin'/*comment*/", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_ExecCmd_Invalid) {
    auto r = Validator::checkSqlInjection("exec xp_cmdshell", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_Execute_Invalid) {
    auto r = Validator::checkSqlInjection("execute sp_help", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_LowerCase_Invalid) {
    auto r = Validator::checkSqlInjection("drop table users", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_MixedCase_Invalid) {
    auto r = Validator::checkSqlInjection("DrOp TaBlE users", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_SemicolonDash_Invalid) {
    auto r = Validator::checkSqlInjection("value;--", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_Xp_Invalid) {
    auto r = Validator::checkSqlInjection("xp_cmdshell 'dir'", "字段");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, SqlInjection_EmptyString_Valid) {
    auto r = Validator::checkSqlInjection("", "字段");
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// XSS / sanitizeHtml
// =====================================================================

TEST_F(ValidatorExtendedTest, SanitizeHtml_NoSpecialChars) {
    EXPECT_EQ(Validator::sanitizeHtml("hello world"), "hello world");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_LessThan) {
    EXPECT_EQ(Validator::sanitizeHtml("<"), "&lt;");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_GreaterThan) {
    EXPECT_EQ(Validator::sanitizeHtml(">"), "&gt;");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_Ampersand) {
    EXPECT_EQ(Validator::sanitizeHtml("&"), "&amp;");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_DoubleQuote) {
    EXPECT_EQ(Validator::sanitizeHtml("\""), "&quot;");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_SingleQuote) {
    EXPECT_EQ(Validator::sanitizeHtml("'"), "&#39;");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_ScriptTag) {
    std::string input = "<script>alert('xss')</script>";
    std::string result = Validator::sanitizeHtml(input);
    EXPECT_EQ(result.find("<script>"), std::string::npos);
    EXPECT_NE(result.find("&lt;script&gt;"), std::string::npos);
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_ImgOnError) {
    std::string input = "<img src=x onerror=alert(1)>";
    std::string result = Validator::sanitizeHtml(input);
    EXPECT_EQ(result.find("<img"), std::string::npos);
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_DivTag) {
    std::string input = "<div style=\"color:red\">text</div>";
    std::string result = Validator::sanitizeHtml(input);
    EXPECT_EQ(result.find("<div"), std::string::npos);
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_Empty) {
    EXPECT_EQ(Validator::sanitizeHtml(""), "");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_MultipleSpecialChars) {
    std::string input = "<b>\"Hello\" & 'World'</b>";
    std::string result = Validator::sanitizeHtml(input);
    EXPECT_EQ(result.find("<"), std::string::npos);
    EXPECT_EQ(result.find(">"), std::string::npos);
    EXPECT_EQ(result.find("\""), std::string::npos);
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_ChineseText) {
    EXPECT_EQ(Validator::sanitizeHtml("你好世界"), "你好世界");
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_MixedContent) {
    std::string input = "Hello <b>World</b> & \"Friends\"";
    std::string result = Validator::sanitizeHtml(input);
    EXPECT_NE(result.find("&lt;b&gt;"), std::string::npos);
    EXPECT_NE(result.find("&amp;"), std::string::npos);
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_IframeTag) {
    std::string input = "<iframe src='http://evil.com'></iframe>";
    std::string result = Validator::sanitizeHtml(input);
    EXPECT_EQ(result.find("<iframe"), std::string::npos);
}

TEST_F(ValidatorExtendedTest, SanitizeHtml_EventHandler) {
    std::string input = "<a onmouseover=\"alert(1)\">link</a>";
    std::string result = Validator::sanitizeHtml(input);
    EXPECT_EQ(result.find("<a"), std::string::npos);
}

// =====================================================================
// 路径遍历检测
// =====================================================================

TEST_F(ValidatorExtendedTest, PathTraversal_Normal_Valid) {
    auto r = Validator::checkPathTraversal("/home/user/file.txt", "路径");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PathTraversal_DotDotSlash_Invalid) {
    auto r = Validator::checkPathTraversal("../../etc/passwd", "路径");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PathTraversal_DotDotBackslash_Invalid) {
    auto r = Validator::checkPathTraversal("..\\..\\windows\\system32", "路径");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PathTraversal_MiddleDotDot_Invalid) {
    auto r = Validator::checkPathTraversal("/home/user/../../../etc/passwd", "路径");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PathTraversal_Empty_Valid) {
    auto r = Validator::checkPathTraversal("", "路径");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PathTraversal_SingleDot_Valid) {
    auto r = Validator::checkPathTraversal("./file.txt", "路径");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PathTraversal_JustDotDot_Invalid) {
    auto r = Validator::checkPathTraversal("..", "路径");
    EXPECT_FALSE(r.isValid);
}

// =====================================================================
// 密码强度验证 (passwordStrong)
// =====================================================================

TEST_F(ValidatorExtendedTest, PasswordStrong_Empty_Invalid) {
    auto r = Validator::passwordStrong("");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_TooShort_Invalid) {
    auto r = Validator::passwordStrong("Aa1!xyz");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_TooLong_Invalid) {
    auto r = Validator::passwordStrong(std::string(65, 'A'));
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_OnlyLowercase_Invalid) {
    auto r = Validator::passwordStrong("abcdefgh");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_ThreeCategories_Valid) {
    auto r = Validator::passwordStrong("Abcdef1!");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_UpperLowerDigit_Valid) {
    auto r = Validator::passwordStrong("Abcdefg1");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_UpperLowerSpecial_Valid) {
    auto r = Validator::passwordStrong("Abcdefg!");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_ExactMin8_Valid) {
    auto r = Validator::passwordStrong("Abc1234!");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, PasswordStrong_Exact64_Valid) {
    std::string pw = std::string(30, 'A') + std::string(30, 'a') + "12!@";
    auto r = Validator::passwordStrong(pw);
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// 密码强度分数
// =====================================================================

TEST_F(ValidatorExtendedTest, PasswordStrength_Empty_Zero) {
    EXPECT_EQ(Validator::calculatePasswordStrength(""), 0);
}

TEST_F(ValidatorExtendedTest, PasswordStrength_Short_Low) {
    int score = Validator::calculatePasswordStrength("abc");
    EXPECT_LE(score, 2);
}

TEST_F(ValidatorExtendedTest, PasswordStrength_AllCategories_High) {
    int score = Validator::calculatePasswordStrength("Abcdef123!@#");
    EXPECT_GE(score, 5);
}

TEST_F(ValidatorExtendedTest, PasswordStrength_LongAllCategories_Max) {
    int score = Validator::calculatePasswordStrength("MyP@ssw0rd!!");
    EXPECT_GE(score, 6);
}

TEST_F(ValidatorExtendedTest, PasswordStrength_OnlyDigits) {
    int score = Validator::calculatePasswordStrength("12345678");
    EXPECT_GE(score, 2);
    EXPECT_LE(score, 3);
}

// =====================================================================
// 文件扩展名验证
// =====================================================================

TEST_F(ValidatorExtendedTest, FileExtension_Jpg_Valid) {
    auto r = Validator::fileExtension("photo.jpg", {"jpg", "png", "gif"});
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, FileExtension_PNG_CaseInsensitive_Valid) {
    auto r = Validator::fileExtension("photo.PNG", {"jpg", "png", "gif"});
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, FileExtension_Exe_Invalid) {
    auto r = Validator::fileExtension("virus.exe", {"jpg", "png", "gif"});
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, FileExtension_NoExtension_Invalid) {
    auto r = Validator::fileExtension("noext", {"jpg", "png"});
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, FileExtension_DotPrefix_Valid) {
    auto r = Validator::fileExtension("photo.jpg", {".jpg", ".png"});
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, FileExtension_MultipleDots_Valid) {
    auto r = Validator::fileExtension("archive.tar.gz", {"gz"});
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// userId 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, UserId_Empty_Invalid) {
    auto r = Validator::userId("");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, UserId_ValidUUID) {
    auto r = Validator::userId("550e8400-e29b-41d4-a716-446655440000");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, UserId_InvalidFormat) {
    auto r = Validator::userId("not-a-uuid");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, UserId_UpperCase_Invalid) {
    auto r = Validator::userId("550E8400-E29B-41D4-A716-446655440000");
    EXPECT_FALSE(r.isValid);
}

// =====================================================================
// hasField 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, HasField_Exists) {
    Json::Value json;
    json["name"] = "test";
    auto r = Validator::hasField(json, "name");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, HasField_Missing) {
    Json::Value json;
    json["name"] = "test";
    auto r = Validator::hasField(json, "age");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, HasField_EmptyJson) {
    Json::Value json(Json::objectValue);
    auto r = Validator::hasField(json, "anything");
    EXPECT_FALSE(r.isValid);
}

// =====================================================================
// inEnum 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, InEnum_Valid) {
    auto r = Validator::inEnum("happy", {"happy", "sad", "neutral"}, "mood");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, InEnum_Invalid) {
    auto r = Validator::inEnum("angry", {"happy", "sad", "neutral"}, "mood");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, InEnum_Empty_Invalid) {
    auto r = Validator::inEnum("", {"happy", "sad"}, "mood");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, InEnum_CaseSensitive) {
    auto r = Validator::inEnum("Happy", {"happy", "sad"}, "mood");
    EXPECT_FALSE(r.isValid);
}

// =====================================================================
// numberRange 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, NumberRange_InRange_Valid) {
    auto r = Validator::numberRange(5, 1, 10, "数值");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, NumberRange_AtMin_Valid) {
    auto r = Validator::numberRange(1, 1, 10, "数值");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, NumberRange_AtMax_Valid) {
    auto r = Validator::numberRange(10, 1, 10, "数值");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, NumberRange_BelowMin_Invalid) {
    auto r = Validator::numberRange(0, 1, 10, "数值");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, NumberRange_AboveMax_Invalid) {
    auto r = Validator::numberRange(11, 1, 10, "数值");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, NumberRange_Negative_Valid) {
    auto r = Validator::numberRange(-5, -10, 0, "数值");
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// paginationParams 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, Pagination_Valid) {
    auto r = Validator::paginationParams(1, 20);
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Pagination_PageZero_Invalid) {
    auto r = Validator::paginationParams(0, 20);
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Pagination_PageSizeZero_Invalid) {
    auto r = Validator::paginationParams(1, 0);
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Pagination_PageSizeTooLarge_Invalid) {
    auto r = Validator::paginationParams(1, 101);
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Pagination_MaxPage_Valid) {
    auto r = Validator::paginationParams(1000, 100);
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Pagination_PageTooLarge_Invalid) {
    auto r = Validator::paginationParams(1001, 20);
    EXPECT_FALSE(r.isValid);
}

// =====================================================================
// arrayLength 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, ArrayLength_WithinLimit_Valid) {
    Json::Value arr(Json::arrayValue);
    arr.append("a");
    arr.append("b");
    auto r = Validator::arrayLength(arr, 5, "标签");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, ArrayLength_ExceedsLimit_Invalid) {
    Json::Value arr(Json::arrayValue);
    for (int i = 0; i < 6; i++) arr.append("x");
    auto r = Validator::arrayLength(arr, 5, "标签");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, ArrayLength_NotArray_Valid) {
    Json::Value val("not an array");
    auto r = Validator::arrayLength(val, 5, "标签");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, ArrayLength_EmptyArray_Valid) {
    Json::Value arr(Json::arrayValue);
    auto r = Validator::arrayLength(arr, 5, "标签");
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// combine 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, Combine_AllValid) {
    auto r = Validator::combine({
        ValidationResult::valid(),
        ValidationResult::valid(),
        ValidationResult::valid()
    });
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Combine_FirstInvalid) {
    auto r = Validator::combine({
        ValidationResult::invalid("error1"),
        ValidationResult::valid(),
        ValidationResult::valid()
    });
    EXPECT_FALSE(r.isValid);
    EXPECT_EQ(r.errorMessage, "error1");
}

TEST_F(ValidatorExtendedTest, Combine_LastInvalid) {
    auto r = Validator::combine({
        ValidationResult::valid(),
        ValidationResult::valid(),
        ValidationResult::invalid("error3")
    });
    EXPECT_FALSE(r.isValid);
    EXPECT_EQ(r.errorMessage, "error3");
}

TEST_F(ValidatorExtendedTest, Combine_Empty_Valid) {
    auto r = Validator::combine({});
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// email 验证
// =====================================================================

TEST_F(ValidatorExtendedTest, Email_Empty_Invalid) {
    auto r = Validator::email("");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Email_Valid_Simple) {
    auto r = Validator::email("user@example.com");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Email_Valid_WithDots) {
    auto r = Validator::email("first.last@example.com");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Email_Valid_WithPlus) {
    auto r = Validator::email("user+tag@example.com");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Email_NoAt_Invalid) {
    auto r = Validator::email("userexample.com");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Email_NoDomain_Invalid) {
    auto r = Validator::email("user@");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Email_NoTLD_Invalid) {
    auto r = Validator::email("user@example");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Email_Valid_Subdomain) {
    auto r = Validator::email("user@mail.example.com");
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// password 基础验证
// =====================================================================

TEST_F(ValidatorExtendedTest, Password_Empty_Invalid) {
    auto r = Validator::password("");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Password_TooShort_Invalid) {
    auto r = Validator::password("12345");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Password_TooLong_Invalid) {
    auto r = Validator::password(std::string(21, 'a'));
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Password_ExactMin6_Valid) {
    auto r = Validator::password("123456");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, Password_ExactMax20_Valid) {
    auto r = Validator::password(std::string(20, 'a'));
    EXPECT_TRUE(r.isValid);
}

// =====================================================================
// ValidationRules
// =====================================================================

TEST_F(ValidatorExtendedTest, ValidationRules_Content_Empty_Invalid) {
    auto r = ValidationRules::content("");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, ValidationRules_Content_Valid) {
    auto r = ValidationRules::content("这是一条有效内容");
    EXPECT_TRUE(r.isValid);
}

TEST_F(ValidatorExtendedTest, ValidationRules_Nickname_TooShort_Invalid) {
    auto r = ValidationRules::nickname("A");
    EXPECT_FALSE(r.isValid);
}

TEST_F(ValidatorExtendedTest, ValidationRules_Nickname_Valid) {
    auto r = ValidationRules::nickname("心湖旅人");
    EXPECT_TRUE(r.isValid);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
