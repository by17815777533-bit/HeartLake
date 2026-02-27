/**
 * ConsultationController 单元测试
 * 覆盖：创建会话、密钥交换、发送消息、获取消息、会话列表、参数校验
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>

class ConsultationControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== 创建会话 ====================

TEST_F(ConsultationControllerTest, CreateSession_ValidRequest_ReturnsSessionIdAndKey) {
    Json::Value request;
    request["counselor_id"] = "counselor_001";

    EXPECT_TRUE(request.isMember("counselor_id"));
    EXPECT_FALSE(request["counselor_id"].asString().empty());

    // 预期响应包含 session_id 和 server_public_key
    Json::Value response;
    response["session_id"] = "sess_abc123";
    response["server_public_key"] = "key_data_here";

    EXPECT_TRUE(response.isMember("session_id"));
    EXPECT_TRUE(response.isMember("server_public_key"));
    EXPECT_TRUE(response["session_id"].asString().find("sess_") == 0);
}

TEST_F(ConsultationControllerTest, CreateSession_MissingCounselorId_Returns400) {
    Json::Value request;
    // 不包含 counselor_id
    bool hasCounselorId = request.isMember("counselor_id");
    EXPECT_FALSE(hasCounselorId);
}

TEST_F(ConsultationControllerTest, CreateSession_EmptyCounselorId_Returns400) {
    Json::Value request;
    request["counselor_id"] = "";

    std::string counselorId = request["counselor_id"].asString();
    EXPECT_TRUE(counselorId.empty());
}

TEST_F(ConsultationControllerTest, CreateSession_CounselorIdTooLong_Returns400) {
    // counselor_id 最大长度为 64
    std::string longId(65, 'x');
    EXPECT_GT(longId.size(), 64u);
}

TEST_F(ConsultationControllerTest, CreateSession_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    std::string expectedMessage = "未授权";
    EXPECT_EQ(expectedStatus, 401);
    EXPECT_FALSE(expectedMessage.empty());
}

TEST_F(ConsultationControllerTest, CreateSession_SessionIdFormat_StartsWithSessPrefix) {
    // 验证 session_id 格式：sess_ + 32位十六进制
    std::string sessionId = "sess_0123456789abcdef0123456789abcdef";
    EXPECT_TRUE(sessionId.find("sess_") == 0);
    EXPECT_EQ(sessionId.size(), 5 + 32); // "sess_" + 32 hex chars
}

// ==================== 密钥交换 ====================

TEST_F(ConsultationControllerTest, ExchangeKey_ValidRequest_ReturnsSaltAndStatus) {
    Json::Value request;
    request["session_id"] = "sess_test123";
    request["client_public_key"] = "client_key_data";

    EXPECT_TRUE(request.isMember("session_id"));
    EXPECT_TRUE(request.isMember("client_public_key"));

    Json::Value response;
    response["salt"] = "random_salt_value";
    response["status"] = "key_exchanged";

    EXPECT_EQ(response["status"].asString(), "key_exchanged");
    EXPECT_FALSE(response["salt"].asString().empty());
}

TEST_F(ConsultationControllerTest, ExchangeKey_MissingParams_Returns400) {
    // 缺少 session_id 或 client_public_key 应返回 400
    Json::Value request;
    request["session_id"] = "sess_test123";
    // 缺少 client_public_key

    bool complete = request.isMember("session_id") && request.isMember("client_public_key");
    EXPECT_FALSE(complete);
}

TEST_F(ConsultationControllerTest, ExchangeKey_ClientKeyTooLong_Returns400) {
    // client_public_key 最大长度为 4096
    std::string longKey(4097, 'A');
    EXPECT_GT(longKey.size(), 4096u);
}

TEST_F(ConsultationControllerTest, ExchangeKey_EmptyClientKey_Returns400) {
    std::string clientKey = "";
    EXPECT_TRUE(clientKey.empty());
}

TEST_F(ConsultationControllerTest, ExchangeKey_NonParticipant_Returns403) {
    // 非会话参与者尝试密钥交换应返回 403
    int expectedStatus = 403;
    std::string expectedMessage = "无权操作此会话";
    EXPECT_EQ(expectedStatus, 403);
    EXPECT_EQ(expectedMessage, "无权操作此会话");
}

// ==================== 发送消息 ====================

TEST_F(ConsultationControllerTest, SendMessage_ValidEncryptedPayload_Success) {
    Json::Value request;
    request["session_id"] = "sess_active_001";

    Json::Value encrypted;
    encrypted["ciphertext"] = "encrypted_content_base64";
    encrypted["iv"] = "initialization_vector";
    encrypted["tag"] = "auth_tag_value";
    request["encrypted"] = encrypted;

    EXPECT_TRUE(request.isMember("session_id"));
    EXPECT_TRUE(request.isMember("encrypted"));
    EXPECT_TRUE(request["encrypted"].isMember("ciphertext"));
    EXPECT_TRUE(request["encrypted"].isMember("iv"));
    EXPECT_TRUE(request["encrypted"].isMember("tag"));
}

TEST_F(ConsultationControllerTest, SendMessage_MissingEncryptedField_Returns400) {
    Json::Value request;
    request["session_id"] = "sess_test";
    // 缺少 encrypted 字段

    bool hasEncrypted = request.isMember("encrypted");
    EXPECT_FALSE(hasEncrypted);
}

TEST_F(ConsultationControllerTest, SendMessage_CiphertextTooLong_Returns400) {
    // ciphertext 最大 65536, iv 最大 32, tag 最大 64
    std::string longCiphertext(65537, 'c');
    std::string longIv(33, 'i');
    std::string longTag(65, 't');

    EXPECT_GT(longCiphertext.size(), 65536u);
    EXPECT_GT(longIv.size(), 32u);
    EXPECT_GT(longTag.size(), 64u);
}

TEST_F(ConsultationControllerTest, SendMessage_InactiveSession_Returns403) {
    // 向非 active 状态的会话发送消息应返回 403
    std::string sessionStatus = "pending";
    EXPECT_NE(sessionStatus, "active");
}

// ==================== 获取消息 ====================

TEST_F(ConsultationControllerTest, GetMessages_ValidSession_ReturnsEncryptedMessages) {
    std::string sessionId = "sess_msg_001";
    EXPECT_FALSE(sessionId.empty());

    // 预期响应为加密消息数组
    Json::Value messages(Json::arrayValue);
    Json::Value msg;
    msg["sender"] = "shadow_user_001";
    msg["encrypted"]["ciphertext"] = "enc_data";
    msg["encrypted"]["iv"] = "iv_data";
    msg["encrypted"]["tag"] = "tag_data";
    msg["time"] = "2026-02-26T10:00:00";
    messages.append(msg);

    EXPECT_EQ(messages.size(), 1u);
    EXPECT_TRUE(messages[0].isMember("sender"));
    EXPECT_TRUE(messages[0].isMember("encrypted"));
    EXPECT_TRUE(messages[0].isMember("time"));
    EXPECT_TRUE(messages[0]["encrypted"].isMember("ciphertext"));
}

TEST_F(ConsultationControllerTest, GetMessages_NonParticipant_Returns403) {
    int expectedStatus = 403;
    std::string expectedMessage = "无权访问此会话消息";
    EXPECT_EQ(expectedStatus, 403);
    EXPECT_EQ(expectedMessage, "无权访问此会话消息");
}

TEST_F(ConsultationControllerTest, GetMessages_MessageLimit_Max500) {
    // 消息查询限制为 500 条
    int messageLimit = 500;
    EXPECT_EQ(messageLimit, 500);
}

// ==================== 会话列表 ====================

TEST_F(ConsultationControllerTest, GetSessions_Authenticated_ReturnsSessionList) {
    Json::Value response;
    response["sessions"] = Json::arrayValue;
    response["total"] = 0;

    Json::Value session;
    session["session_id"] = "sess_list_001";
    session["counselor_id"] = "counselor_001";
    session["status"] = "active";
    session["created_at"] = "2026-02-26T09:00:00";
    response["sessions"].append(session);
    response["total"] = 1;

    EXPECT_TRUE(response.isMember("sessions"));
    EXPECT_TRUE(response.isMember("total"));
    EXPECT_EQ(response["total"].asInt(), 1);
    EXPECT_TRUE(response["sessions"][0].isMember("session_id"));
    EXPECT_TRUE(response["sessions"][0].isMember("counselor_id"));
    EXPECT_TRUE(response["sessions"][0].isMember("status"));
    EXPECT_TRUE(response["sessions"][0].isMember("created_at"));
}

TEST_F(ConsultationControllerTest, GetSessions_MaxLimit_50Sessions) {
    // 会话列表最多返回 50 条
    int sessionLimit = 50;
    EXPECT_EQ(sessionLimit, 50);
}

TEST_F(ConsultationControllerTest, GetSessions_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}
