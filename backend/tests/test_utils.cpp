/**
 * 工具类单元测试 (Validator, PasswordUtil, IdGenerator)
 */

#include <gtest/gtest.h>
#include "utils/Validator.h"
#include "utils/PasswordUtil.h"
#include "utils/IdGenerator.h"
#include <json/json.h>
#include <set>

using namespace heartlake::utils;

// =====================================================================
// Validator 测试
// =====================================================================

class ValidatorTest : public ::testing::Test {};

// -------------------- requiredString --------------------

TEST_F(ValidatorTest, RequiredString_Empty_Invalid) {
    auto result = Validator::requiredString("", "测试字段");
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.errorMessage.find("不能为空"), std::string::npos);
}

TEST_F(ValidatorTest, RequiredString_Valid) {
    auto result = Validator::requiredString("hello", "测试字段");
    EXPECT_TRUE(result.isValid);
}

// -------------------- required (Json::Value) --------------------

TEST_F(ValidatorTest, Required_NullValue_Invalid) {
    Json::Value val(Json::nullValue);
    auto result = Validator::required(val, "字段");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Required_EmptyString_Invalid) {
    Json::Value val("");
    auto result = Validator::required(val, "字段");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Required_ValidString) {
    Json::Value val("有内容");
    auto result = Validator::required(val, "字段");
    EXPECT_TRUE(result.isValid);
}

// -------------------- length --------------------

TEST_F(ValidatorTest, Length_BelowMin_Invalid) {
    auto result = Validator::length("a", 2, 10, "字段");
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.errorMessage.find("至少需要"), std::string::npos);
}

TEST_F(ValidatorTest, Length_AboveMax_Invalid) {
    auto result = Validator::length("abcdefghijk", 2, 10, "字段");
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.errorMessage.find("不能超过"), std::string::npos);
}

TEST_F(ValidatorTest, Length_ExactMin_Valid) {
    auto result = Validator::length("ab", 2, 10, "字段");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, Length_ExactMax_Valid) {
    auto result = Validator::length("abcdefghij", 2, 10, "字段");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, Length_InRange_Valid) {
    auto result = Validator::length("hello", 2, 10, "字段");
    EXPECT_TRUE(result.isValid);
}

// -------------------- email --------------------

TEST_F(ValidatorTest, Email_Empty_Invalid) {
    auto result = Validator::email("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Email_NoAtSign_Invalid) {
    auto result = Validator::email("userexample.com");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Email_NoDomain_Invalid) {
    auto result = Validator::email("user@");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Email_Valid) {
    auto result = Validator::email("user@example.com");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, Email_ValidWithPlus) {
    auto result = Validator::email("user+tag@example.com");
    EXPECT_TRUE(result.isValid);
}

// -------------------- password --------------------

TEST_F(ValidatorTest, Password_Empty_Invalid) {
    auto result = Validator::password("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Password_TooShort_Invalid) {
    auto result = Validator::password("12345");
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.errorMessage.find("6"), std::string::npos);
}

TEST_F(ValidatorTest, Password_TooLong_Invalid) {
    auto result = Validator::password("123456789012345678901");
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.errorMessage.find("20"), std::string::npos);
}

TEST_F(ValidatorTest, Password_ExactMin6_Valid) {
    auto result = Validator::password("123456");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, Password_ExactMax20_Valid) {
    auto result = Validator::password("12345678901234567890");
    EXPECT_TRUE(result.isValid);
}

// -------------------- passwordStrong --------------------

TEST_F(ValidatorTest, PasswordStrong_Empty_Invalid) {
    auto result = Validator::passwordStrong("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, PasswordStrong_TooShort_Invalid) {
    auto result = Validator::passwordStrong("Abc1!");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, PasswordStrong_TooLong_Invalid) {
    std::string longPwd(33, 'A');
    auto result = Validator::passwordStrong(longPwd);
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, PasswordStrong_WeakOnlyLowercase_Invalid) {
    auto result = Validator::passwordStrong("abcdefgh");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, PasswordStrong_StrongMixed_Valid) {
    auto result = Validator::passwordStrong("Abc12345");
    EXPECT_TRUE(result.isValid);
}

// -------------------- calculatePasswordStrength --------------------

TEST_F(ValidatorTest, PasswordStrength_EmptyIsZero) {
    EXPECT_EQ(Validator::calculatePasswordStrength(""), 0);
}

TEST_F(ValidatorTest, PasswordStrength_ShortLowScore) {
    int score = Validator::calculatePasswordStrength("abc");
    EXPECT_LT(score, 3);
}

TEST_F(ValidatorTest, PasswordStrength_StrongHighScore) {
    int score = Validator::calculatePasswordStrength("Abc123!@#xyz");
    EXPECT_GE(score, 4);
}

// -------------------- userId --------------------

TEST_F(ValidatorTest, UserId_Empty_Invalid) {
    auto result = Validator::userId("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, UserId_InvalidFormat_Invalid) {
    auto result = Validator::userId("not-a-uuid");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, UserId_ValidUUID) {
    auto result = Validator::userId("550e8400-e29b-41d4-a716-446655440000");
    EXPECT_TRUE(result.isValid);
}

// -------------------- sanitizeHtml --------------------

TEST_F(ValidatorTest, SanitizeHtml_EscapesAngleBrackets) {
    std::string input = "<script>alert('xss')</script>";
    std::string sanitized = Validator::sanitizeHtml(input);
    EXPECT_EQ(sanitized.find("<"), std::string::npos);
    EXPECT_EQ(sanitized.find(">"), std::string::npos);
}

TEST_F(ValidatorTest, SanitizeHtml_EscapesAmpersand) {
    std::string sanitized = Validator::sanitizeHtml("a&b");
    EXPECT_NE(sanitized.find("&amp;"), std::string::npos);
}

TEST_F(ValidatorTest, SanitizeHtml_EscapesQuotes) {
    std::string sanitized = Validator::sanitizeHtml("a\"b'c");
    EXPECT_EQ(sanitized.find("\""), std::string::npos);
    EXPECT_EQ(sanitized.find("'"), std::string::npos);
}

TEST_F(ValidatorTest, SanitizeHtml_PlainTextUnchanged) {
    std::string input = "hello world";
    EXPECT_EQ(Validator::sanitizeHtml(input), input);
}

// -------------------- checkSqlInjection --------------------

TEST_F(ValidatorTest, SqlInjection_DetectsDropTable) {
    auto result = Validator::checkSqlInjection("DROP TABLE users", "input");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, SqlInjection_DetectsUnionSelect) {
    auto result = Validator::checkSqlInjection("1 UNION SELECT * FROM users", "input");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, SqlInjection_SafeInput_Valid) {
    auto result = Validator::checkSqlInjection("hello world 123", "input");
    EXPECT_TRUE(result.isValid);
}

// -------------------- checkPathTraversal --------------------

TEST_F(ValidatorTest, PathTraversal_DetectsDotDotSlash) {
    auto result = Validator::checkPathTraversal("../../etc/passwd", "path");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, PathTraversal_SafePath_Valid) {
    auto result = Validator::checkPathTraversal("images/photo.jpg", "path");
    EXPECT_TRUE(result.isValid);
}

// -------------------- url --------------------

TEST_F(ValidatorTest, Url_Empty_Invalid) {
    auto result = Validator::url("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Url_NoProtocol_Invalid) {
    auto result = Validator::url("example.com");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Url_ValidHttp) {
    auto result = Validator::url("http://example.com");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, Url_ValidHttps) {
    auto result = Validator::url("https://example.com/path?q=1");
    EXPECT_TRUE(result.isValid);
}

// -------------------- phoneNumber --------------------

TEST_F(ValidatorTest, PhoneNumber_Empty_Invalid) {
    auto result = Validator::phoneNumber("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, PhoneNumber_TooShort_Invalid) {
    auto result = Validator::phoneNumber("1381234");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, PhoneNumber_ValidChinese) {
    auto result = Validator::phoneNumber("13812345678");
    EXPECT_TRUE(result.isValid);
}

// -------------------- verificationCode --------------------

TEST_F(ValidatorTest, VerificationCode_Empty_Invalid) {
    auto result = Validator::verificationCode("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, VerificationCode_WrongLength_Invalid) {
    auto result = Validator::verificationCode("12345");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, VerificationCode_Valid6Digits) {
    auto result = Validator::verificationCode("123456");
    EXPECT_TRUE(result.isValid);
}

// -------------------- fileExtension --------------------

TEST_F(ValidatorTest, FileExtension_NotAllowed_Invalid) {
    auto result = Validator::fileExtension("test.exe", {".jpg", ".png"});
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, FileExtension_Allowed_Valid) {
    auto result = Validator::fileExtension("photo.jpg", {".jpg", ".png", ".gif"});
    EXPECT_TRUE(result.isValid);
}

// -------------------- hasField --------------------

TEST_F(ValidatorTest, HasField_Missing_Invalid) {
    Json::Value obj;
    obj["name"] = "test";
    auto result = Validator::hasField(obj, "email");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, HasField_Present_Valid) {
    Json::Value obj;
    obj["email"] = "test@example.com";
    auto result = Validator::hasField(obj, "email");
    EXPECT_TRUE(result.isValid);
}

// -------------------- inEnum --------------------

TEST_F(ValidatorTest, InEnum_NotInList_Invalid) {
    auto result = Validator::inEnum("unknown", {"male", "female", "other"}, "性别");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, InEnum_InList_Valid) {
    auto result = Validator::inEnum("male", {"male", "female", "other"}, "性别");
    EXPECT_TRUE(result.isValid);
}

// -------------------- arrayLength --------------------

TEST_F(ValidatorTest, ArrayLength_NotArray_Valid) {
    Json::Value val("not array");
    auto result = Validator::arrayLength(val, 5, "字段");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, ArrayLength_ExceedsMax_Invalid) {
    Json::Value arr(Json::arrayValue);
    for (int i = 0; i < 6; ++i) arr.append(i);
    auto result = Validator::arrayLength(arr, 5, "字段");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, ArrayLength_WithinMax_Valid) {
    Json::Value arr(Json::arrayValue);
    arr.append(1);
    arr.append(2);
    auto result = Validator::arrayLength(arr, 5, "字段");
    EXPECT_TRUE(result.isValid);
}

// -------------------- numberRange --------------------

TEST_F(ValidatorTest, NumberRange_BelowMin_Invalid) {
    auto result = Validator::numberRange(0, 1, 100, "数值");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, NumberRange_AboveMax_Invalid) {
    auto result = Validator::numberRange(101, 1, 100, "数值");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, NumberRange_InRange_Valid) {
    auto result = Validator::numberRange(50, 1, 100, "数值");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, NumberRange_ExactBoundaries_Valid) {
    EXPECT_TRUE(Validator::numberRange(1, 1, 100, "数值").isValid);
    EXPECT_TRUE(Validator::numberRange(100, 1, 100, "数值").isValid);
}

// -------------------- paginationParams --------------------

TEST_F(ValidatorTest, Pagination_ValidParams) {
    auto result = Validator::paginationParams(1, 20);
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, Pagination_PageZero_Invalid) {
    auto result = Validator::paginationParams(0, 20);
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidatorTest, Pagination_PageSizeTooLarge_Invalid) {
    auto result = Validator::paginationParams(1, 101);
    EXPECT_FALSE(result.isValid);
}

// -------------------- combine --------------------

TEST_F(ValidatorTest, Combine_AllValid) {
    auto result = Validator::combine({
        ValidationResult::valid(),
        ValidationResult::valid(),
        ValidationResult::valid()
    });
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidatorTest, Combine_OneInvalid_ReturnsFirst) {
    auto result = Validator::combine({
        ValidationResult::valid(),
        ValidationResult::invalid("错误1"),
        ValidationResult::invalid("错误2")
    });
    EXPECT_FALSE(result.isValid);
    EXPECT_EQ(result.errorMessage, "错误1");
}

TEST_F(ValidatorTest, Combine_Empty_Valid) {
    auto result = Validator::combine({});
    EXPECT_TRUE(result.isValid);
}

// =====================================================================
// ValidationRules 测试
// =====================================================================

class ValidationRulesTest : public ::testing::Test {};

TEST_F(ValidationRulesTest, Content_Empty_Invalid) {
    auto result = ValidationRules::content("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidationRulesTest, Content_Valid) {
    auto result = ValidationRules::content("这是一条有效内容");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidationRulesTest, Content_TooLong_Invalid) {
    std::string longContent(5001, 'x');
    auto result = ValidationRules::content(longContent);
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidationRulesTest, Nickname_Empty_Invalid) {
    auto result = ValidationRules::nickname("");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidationRulesTest, Nickname_TooShort_Invalid) {
    auto result = ValidationRules::nickname("a");
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidationRulesTest, Nickname_TooLong_Invalid) {
    std::string longNick(21, 'x');
    auto result = ValidationRules::nickname(longNick);
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidationRulesTest, Nickname_Valid) {
    auto result = ValidationRules::nickname("小明");
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidationRulesTest, Tags_WithinLimit_Valid) {
    Json::Value tags(Json::arrayValue);
    tags.append("tag1");
    tags.append("tag2");
    auto result = ValidationRules::tags(tags);
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidationRulesTest, Tags_ExceedsLimit_Invalid) {
    Json::Value tags(Json::arrayValue);
    for (int i = 0; i < 6; ++i) tags.append("tag" + std::to_string(i));
    auto result = ValidationRules::tags(tags);
    EXPECT_FALSE(result.isValid);
}

TEST_F(ValidationRulesTest, MediaIds_WithinLimit_Valid) {
    Json::Value ids(Json::arrayValue);
    for (int i = 0; i < 9; ++i) ids.append("id" + std::to_string(i));
    auto result = ValidationRules::mediaIds(ids);
    EXPECT_TRUE(result.isValid);
}

TEST_F(ValidationRulesTest, MediaIds_ExceedsLimit_Invalid) {
    Json::Value ids(Json::arrayValue);
    for (int i = 0; i < 10; ++i) ids.append("id" + std::to_string(i));
    auto result = ValidationRules::mediaIds(ids);
    EXPECT_FALSE(result.isValid);
}

// =====================================================================
// PasswordUtil 测试
// =====================================================================

class PasswordUtilTest : public ::testing::Test {};

TEST_F(PasswordUtilTest, GenerateSalt_NotEmpty) {
    std::string salt = PasswordUtil::generateSalt();
    EXPECT_FALSE(salt.empty());
}

TEST_F(PasswordUtilTest, GenerateSalt_CorrectLength) {
    std::string salt = PasswordUtil::generateSalt();
    // 32 bytes -> 64 hex chars
    EXPECT_EQ(salt.length(), 64u);
}

TEST_F(PasswordUtilTest, GenerateSalt_IsHex) {
    std::string salt = PasswordUtil::generateSalt();
    for (char c : salt) {
        EXPECT_TRUE(std::isxdigit(c)) << "Non-hex character found: " << c;
    }
}

TEST_F(PasswordUtilTest, GenerateSalt_Unique) {
    std::string salt1 = PasswordUtil::generateSalt();
    std::string salt2 = PasswordUtil::generateSalt();
    EXPECT_NE(salt1, salt2);
}

TEST_F(PasswordUtilTest, HashPassword_NotEmpty) {
    std::string salt = PasswordUtil::generateSalt();
    std::string hash = PasswordUtil::hashPassword("password123", salt);
    EXPECT_FALSE(hash.empty());
}

TEST_F(PasswordUtilTest, HashPassword_CorrectLength) {
    std::string salt = PasswordUtil::generateSalt();
    std::string hash = PasswordUtil::hashPassword("password123", salt);
    // 64 bytes -> 128 hex chars
    EXPECT_EQ(hash.length(), 128u);
}

TEST_F(PasswordUtilTest, HashPassword_Deterministic) {
    std::string salt = PasswordUtil::generateSalt();
    std::string hash1 = PasswordUtil::hashPassword("password123", salt);
    std::string hash2 = PasswordUtil::hashPassword("password123", salt);
    EXPECT_EQ(hash1, hash2);
}

TEST_F(PasswordUtilTest, HashPassword_DifferentSalt_DifferentHash) {
    std::string salt1 = PasswordUtil::generateSalt();
    std::string salt2 = PasswordUtil::generateSalt();
    std::string hash1 = PasswordUtil::hashPassword("password123", salt1);
    std::string hash2 = PasswordUtil::hashPassword("password123", salt2);
    EXPECT_NE(hash1, hash2);
}

TEST_F(PasswordUtilTest, HashPassword_DifferentPassword_DifferentHash) {
    std::string salt = PasswordUtil::generateSalt();
    std::string hash1 = PasswordUtil::hashPassword("password123", salt);
    std::string hash2 = PasswordUtil::hashPassword("password456", salt);
    EXPECT_NE(hash1, hash2);
}

TEST_F(PasswordUtilTest, VerifyPassword_CorrectPassword_True) {
    std::string salt = PasswordUtil::generateSalt();
    std::string hash = PasswordUtil::hashPassword("mypassword", salt);
    EXPECT_TRUE(PasswordUtil::verifyPassword("mypassword", salt, hash));
}

TEST_F(PasswordUtilTest, VerifyPassword_WrongPassword_False) {
    std::string salt = PasswordUtil::generateSalt();
    std::string hash = PasswordUtil::hashPassword("mypassword", salt);
    EXPECT_FALSE(PasswordUtil::verifyPassword("wrongpassword", salt, hash));
}

TEST_F(PasswordUtilTest, VerifyPassword_WrongSalt_False) {
    std::string salt1 = PasswordUtil::generateSalt();
    std::string salt2 = PasswordUtil::generateSalt();
    std::string hash = PasswordUtil::hashPassword("mypassword", salt1);
    EXPECT_FALSE(PasswordUtil::verifyPassword("mypassword", salt2, hash));
}

TEST_F(PasswordUtilTest, GeneratePasswordHash_OutputsNonEmpty) {
    std::string salt, hash;
    PasswordUtil::generatePasswordHash("testpassword", salt, hash);
    EXPECT_FALSE(salt.empty());
    EXPECT_FALSE(hash.empty());
}

TEST_F(PasswordUtilTest, GeneratePasswordHash_CanVerify) {
    std::string salt, hash;
    PasswordUtil::generatePasswordHash("testpassword", salt, hash);
    EXPECT_TRUE(PasswordUtil::verifyPassword("testpassword", salt, hash));
    EXPECT_FALSE(PasswordUtil::verifyPassword("wrongpassword", salt, hash));
}

// =====================================================================
// IdGenerator 测试
// =====================================================================

class IdGeneratorTest : public ::testing::Test {};

TEST_F(IdGeneratorTest, GenerateUserId_HasPrefix) {
    std::string id = IdGenerator::generateUserId();
    EXPECT_EQ(id.substr(0, 5), "user_");
}

TEST_F(IdGeneratorTest, GenerateUserId_CorrectLength) {
    std::string id = IdGenerator::generateUserId();
    // "user_" (5) + 16 hex = 21
    EXPECT_EQ(id.length(), 21u);
}

TEST_F(IdGeneratorTest, GenerateAnonymousId_HasPrefix) {
    std::string id = IdGenerator::generateAnonymousId();
    EXPECT_EQ(id.substr(0, 10), "anonymous_");
}

TEST_F(IdGeneratorTest, GenerateAnonymousId_CorrectLength) {
    std::string id = IdGenerator::generateAnonymousId();
    // "anonymous_" (10) + 12 hex = 22
    EXPECT_EQ(id.length(), 22u);
}

TEST_F(IdGeneratorTest, GenerateStoneId_HasPrefix) {
    std::string id = IdGenerator::generateStoneId();
    EXPECT_EQ(id.substr(0, 6), "stone_");
    EXPECT_EQ(id.length(), 22u);
}

TEST_F(IdGeneratorTest, GenerateRippleId_HasPrefix) {
    std::string id = IdGenerator::generateRippleId();
    EXPECT_EQ(id.substr(0, 7), "ripple_");
    EXPECT_EQ(id.length(), 23u);
}

TEST_F(IdGeneratorTest, GenerateBoatId_HasPrefix) {
    std::string id = IdGenerator::generateBoatId();
    EXPECT_EQ(id.substr(0, 5), "boat_");
    EXPECT_EQ(id.length(), 21u);
}

TEST_F(IdGeneratorTest, GenerateNotificationId_HasPrefix) {
    std::string id = IdGenerator::generateNotificationId();
    EXPECT_EQ(id.substr(0, 6), "notif_");
    EXPECT_EQ(id.length(), 22u);
}

TEST_F(IdGeneratorTest, GenerateConnectionId_HasPrefix) {
    std::string id = IdGenerator::generateConnectionId();
    EXPECT_EQ(id.substr(0, 5), "conn_");
    EXPECT_EQ(id.length(), 21u);
}

TEST_F(IdGeneratorTest, GenerateMessageId_HasPrefix) {
    std::string id = IdGenerator::generateMessageId();
    EXPECT_EQ(id.substr(0, 4), "msg_");
    EXPECT_EQ(id.length(), 20u);
}

TEST_F(IdGeneratorTest, GenerateReportId_HasPrefix) {
    std::string id = IdGenerator::generateReportId();
    EXPECT_EQ(id.substr(0, 7), "report_");
    EXPECT_EQ(id.length(), 23u);
}

TEST_F(IdGeneratorTest, GenerateSessionId_HasPrefix) {
    std::string id = IdGenerator::generateSessionId();
    EXPECT_EQ(id.substr(0, 8), "session_");
    EXPECT_EQ(id.length(), 24u);
}

TEST_F(IdGeneratorTest, GenerateUUID_CorrectLength) {
    std::string uuid = IdGenerator::generateUUID();
    EXPECT_EQ(uuid.length(), 32u);
}

TEST_F(IdGeneratorTest, GenerateUUID_IsHex) {
    std::string uuid = IdGenerator::generateUUID();
    for (char c : uuid) {
        EXPECT_TRUE(std::isxdigit(c)) << "Non-hex character: " << c;
    }
}

TEST_F(IdGeneratorTest, GenerateNickname_HasPrefix) {
    std::string nick = IdGenerator::generateNickname();
    EXPECT_NE(nick.find("旅人#"), std::string::npos);
}

TEST_F(IdGeneratorTest, GenerateNickname_HasFourDigitNumber) {
    std::string nick = IdGenerator::generateNickname();
    // "旅人#" is multi-byte, extract the number part after '#'
    size_t hashPos = nick.find('#');
    ASSERT_NE(hashPos, std::string::npos);
    std::string numStr = nick.substr(hashPos + 1);
    int num = std::stoi(numStr);
    EXPECT_GE(num, 1000);
    EXPECT_LE(num, 9999);
}

TEST_F(IdGeneratorTest, GenerateUserId_Unique) {
    std::set<std::string> ids;
    for (int i = 0; i < 100; ++i) {
        ids.insert(IdGenerator::generateUserId());
    }
    EXPECT_EQ(ids.size(), 100u);
}

TEST_F(IdGeneratorTest, GenerateUUID_Unique) {
    std::set<std::string> uuids;
    for (int i = 0; i < 100; ++i) {
        uuids.insert(IdGenerator::generateUUID());
    }
    EXPECT_EQ(uuids.size(), 100u);
}

TEST_F(IdGeneratorTest, HexCharsOnly_InSuffix) {
    std::string id = IdGenerator::generateUserId();
    std::string suffix = id.substr(5); // skip "user_"
    for (char c : suffix) {
        EXPECT_TRUE(std::isxdigit(c)) << "Non-hex character in suffix: " << c;
    }
}

// =====================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
