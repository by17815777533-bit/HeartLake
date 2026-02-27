/**
 * WebSocketHub 单元测试
 * WebSocketHub 是单例的内存状态管理器，大部分方法可以直接测试。
 * 需要 mock WebSocketConnection 来模拟真实连接对象。
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include "infrastructure/realtime/WebSocketHub.h"
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

using namespace heartlake::realtime;

/**
 * Mock WebSocket 连接 - 继承 drogon::WebSocketConnection 抽象类
 * 记录所有发送的消息，用于验证广播/单播/房间消息行为
 */
class MockWebSocketConnection : public drogon::WebSocketConnection {
public:
    mutable std::vector<std::string> sentMessages;
    mutable bool closed = false;
    mutable bool forceCloseCalled = false;
    trantor::InetAddress localAddr_{"127.0.0.1", 8080};
    trantor::InetAddress peerAddr_{"192.168.1.100", 54321};

    void send(const char* msg, uint64_t len,
              const drogon::WebSocketMessageType /*type*/) override {
        sentMessages.emplace_back(msg, len);
    }

    void send(std::string_view msg,
              const drogon::WebSocketMessageType /*type*/) override {
        sentMessages.emplace_back(msg);
    }

    void sendJson(const Json::Value& json,
                  const drogon::WebSocketMessageType /*type*/) override {
        sentMessages.push_back(Json::FastWriter().write(json));
    }

    const trantor::InetAddress& localAddr() const override {
        return localAddr_;
    }

    const trantor::InetAddress& peerAddr() const override {
        return peerAddr_;
    }

    bool connected() const override { return !closed; }

    bool disconnected() const override { return closed; }

    void shutdown(const drogon::CloseCode /*code*/,
                  const std::string& /*reason*/) override {
        closed = true;
    }

    void forceClose() override {
        closed = true;
        forceCloseCalled = true;
    }

    void setPingMessage(const std::string&,
                        const std::chrono::duration<double>&) override {}

    void disablePing() override {}
};

// 辅助函数：创建 mock 连接
static std::shared_ptr<MockWebSocketConnection> makeMockConn() {
    return std::make_shared<MockWebSocketConnection>();
}

class WebSocketHubTest : public ::testing::Test {
protected:
    WebSocketHub& hub_ = WebSocketHub::getInstance();

    // 每个测试前后清理所有连接，保证隔离
    std::vector<drogon::WebSocketConnectionPtr> testConns_;

    void SetUp() override {
        // 清理残留连接
        for (auto& c : testConns_) {
            hub_.removeConnection(c);
        }
        testConns_.clear();
    }

    void TearDown() override {
        for (auto& c : testConns_) {
            hub_.removeConnection(c);
        }
        testConns_.clear();
    }

    // 注册连接并记录，方便 TearDown 清理
    drogon::WebSocketConnectionPtr addConn(const std::string& userId) {
        auto conn = makeMockConn();
        hub_.addConnection(conn, userId);
        testConns_.push_back(conn);
        return conn;
    }
};

// ==================== 连接管理 ====================

TEST_F(WebSocketHubTest, AddConnection_IncreasesCount) {
    size_t before = hub_.getConnectionCount();
    auto conn = addConn("user_ws_1");
    EXPECT_EQ(hub_.getConnectionCount(), before + 1);
}

TEST_F(WebSocketHubTest, RemoveConnection_DecreasesCount) {
    auto conn = addConn("user_ws_2");
    size_t after_add = hub_.getConnectionCount();

    hub_.removeConnection(conn);
    testConns_.clear(); // 已手动移除，不需要 TearDown 再移除
    EXPECT_EQ(hub_.getConnectionCount(), after_add - 1);
}

TEST_F(WebSocketHubTest, RemoveConnection_NonExistent_NoError) {
    auto fakeConn = makeMockConn();
    EXPECT_NO_THROW(hub_.removeConnection(fakeConn));
}

TEST_F(WebSocketHubTest, AddConnection_UserOnline) {
    auto conn = addConn("user_online_check");
    EXPECT_TRUE(hub_.isUserOnline("user_online_check"));
}

TEST_F(WebSocketHubTest, RemoveConnection_UserOffline) {
    auto conn = addConn("user_offline_check");
    hub_.removeConnection(conn);
    testConns_.clear();
    EXPECT_FALSE(hub_.isUserOnline("user_offline_check"));
}

TEST_F(WebSocketHubTest, MultipleConnections_SameUser) {
    auto conn1 = addConn("user_multi");
    auto conn2 = addConn("user_multi");
    EXPECT_TRUE(hub_.isUserOnline("user_multi"));

    // 移除一个连接，用户仍在线
    hub_.removeConnection(conn1);
    EXPECT_TRUE(hub_.isUserOnline("user_multi"));

    // 移除全部连接，用户离线
    hub_.removeConnection(conn2);
    testConns_.clear();
    EXPECT_FALSE(hub_.isUserOnline("user_multi"));
}

// ==================== 房间管理 ====================

TEST_F(WebSocketHubTest, JoinRoom_IncreasesRoomSize) {
    auto conn = addConn("user_room_1");
    hub_.joinRoom(conn, "room_alpha");
    EXPECT_EQ(hub_.getRoomSize("room_alpha"), 1u);
}

TEST_F(WebSocketHubTest, LeaveRoom_DecreasesRoomSize) {
    auto conn = addConn("user_room_2");
    hub_.joinRoom(conn, "room_beta");
    EXPECT_EQ(hub_.getRoomSize("room_beta"), 1u);

    hub_.leaveRoom(conn, "room_beta");
    EXPECT_EQ(hub_.getRoomSize("room_beta"), 0u);
}

TEST_F(WebSocketHubTest, JoinRoom_MultipleUsers) {
    auto conn1 = addConn("user_r1");
    auto conn2 = addConn("user_r2");
    auto conn3 = addConn("user_r3");

    hub_.joinRoom(conn1, "room_group");
    hub_.joinRoom(conn2, "room_group");
    hub_.joinRoom(conn3, "room_group");

    EXPECT_EQ(hub_.getRoomSize("room_group"), 3u);
}

TEST_F(WebSocketHubTest, LeaveRoom_NonMember_NoError) {
    auto conn = addConn("user_not_in_room");
    EXPECT_NO_THROW(hub_.leaveRoom(conn, "room_nonexistent"));
}

TEST_F(WebSocketHubTest, RemoveConnection_CleansUpRooms) {
    auto conn = addConn("user_room_cleanup");
    hub_.joinRoom(conn, "room_cleanup_test");
    EXPECT_EQ(hub_.getRoomSize("room_cleanup_test"), 1u);

    hub_.removeConnection(conn);
    testConns_.clear();
    EXPECT_EQ(hub_.getRoomSize("room_cleanup_test"), 0u);
}

TEST_F(WebSocketHubTest, EmptyRoom_SizeIsZero) {
    EXPECT_EQ(hub_.getRoomSize("room_that_never_existed"), 0u);
}

// ==================== 消息广播 ====================

TEST_F(WebSocketHubTest, Broadcast_AllConnectionsReceive) {
    auto conn1 = addConn("user_bc_1");
    auto conn2 = addConn("user_bc_2");

    hub_.broadcast("hello everyone");

    auto* mock1 = dynamic_cast<MockWebSocketConnection*>(conn1.get());
    auto* mock2 = dynamic_cast<MockWebSocketConnection*>(conn2.get());
    ASSERT_NE(mock1, nullptr);
    ASSERT_NE(mock2, nullptr);

    // 两个连接都应该收到消息
    bool found1 = false, found2 = false;
    for (const auto& msg : mock1->sentMessages) {
        if (msg == "hello everyone") found1 = true;
    }
    for (const auto& msg : mock2->sentMessages) {
        if (msg == "hello everyone") found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

// ==================== 单播消息 ====================

TEST_F(WebSocketHubTest, SendToUser_OnlyTargetReceives) {
    auto conn1 = addConn("user_target");
    auto conn2 = addConn("user_other");

    hub_.sendToUser("user_target", "private message");

    auto* mock1 = dynamic_cast<MockWebSocketConnection*>(conn1.get());
    auto* mock2 = dynamic_cast<MockWebSocketConnection*>(conn2.get());

    bool targetGot = false;
    for (const auto& msg : mock1->sentMessages) {
        if (msg == "private message") targetGot = true;
    }
    EXPECT_TRUE(targetGot);

    // 其他用户不应收到
    bool otherGot = false;
    for (const auto& msg : mock2->sentMessages) {
        if (msg == "private message") otherGot = true;
    }
    EXPECT_FALSE(otherGot);
}

TEST_F(WebSocketHubTest, SendToUser_MultipleConns_AllReceive) {
    auto conn1 = addConn("user_multi_msg");
    auto conn2 = addConn("user_multi_msg");

    hub_.sendToUser("user_multi_msg", "multi conn msg");

    auto* mock1 = dynamic_cast<MockWebSocketConnection*>(conn1.get());
    auto* mock2 = dynamic_cast<MockWebSocketConnection*>(conn2.get());

    bool got1 = false, got2 = false;
    for (const auto& msg : mock1->sentMessages) {
        if (msg == "multi conn msg") got1 = true;
    }
    for (const auto& msg : mock2->sentMessages) {
        if (msg == "multi conn msg") got2 = true;
    }
    EXPECT_TRUE(got1);
    EXPECT_TRUE(got2);
}

TEST_F(WebSocketHubTest, SendToUser_NonExistent_NoError) {
    EXPECT_NO_THROW(hub_.sendToUser("ghost_user", "hello?"));
}

// ==================== 房间消息 ====================

TEST_F(WebSocketHubTest, SendToRoom_MembersReceive) {
    auto conn1 = addConn("user_rm_1");
    auto conn2 = addConn("user_rm_2");
    auto conn3 = addConn("user_rm_3");

    hub_.joinRoom(conn1, "chat_room");
    hub_.joinRoom(conn2, "chat_room");
    // conn3 不在房间里

    hub_.sendToRoom("chat_room", "room message");

    auto* mock1 = dynamic_cast<MockWebSocketConnection*>(conn1.get());
    auto* mock2 = dynamic_cast<MockWebSocketConnection*>(conn2.get());
    auto* mock3 = dynamic_cast<MockWebSocketConnection*>(conn3.get());

    bool got1 = false, got2 = false, got3 = false;
    for (const auto& msg : mock1->sentMessages) {
        if (msg == "room message") got1 = true;
    }
    for (const auto& msg : mock2->sentMessages) {
        if (msg == "room message") got2 = true;
    }
    for (const auto& msg : mock3->sentMessages) {
        if (msg == "room message") got3 = true;
    }
    EXPECT_TRUE(got1);
    EXPECT_TRUE(got2);
    EXPECT_FALSE(got3);
}

TEST_F(WebSocketHubTest, SendToRoom_ExcludeSender) {
    auto sender = addConn("user_sender");
    auto receiver = addConn("user_receiver");

    hub_.joinRoom(sender, "exclude_room");
    hub_.joinRoom(receiver, "exclude_room");

    // 发送时排除 sender
    hub_.sendToRoom("exclude_room", "msg from sender", sender);

    auto* mockSender = dynamic_cast<MockWebSocketConnection*>(sender.get());
    auto* mockReceiver = dynamic_cast<MockWebSocketConnection*>(receiver.get());

    bool senderGot = false, receiverGot = false;
    for (const auto& msg : mockSender->sentMessages) {
        if (msg == "msg from sender") senderGot = true;
    }
    for (const auto& msg : mockReceiver->sentMessages) {
        if (msg == "msg from sender") receiverGot = true;
    }
    EXPECT_FALSE(senderGot);
    EXPECT_TRUE(receiverGot);
}

// ==================== 64KB 消息大小限制 ====================

TEST_F(WebSocketHubTest, MessageSizeLimit_WithinLimit_Delivered) {
    auto conn = addConn("user_size_ok");
    std::string normalMsg(1024, 'A'); // 1KB

    hub_.sendToUser("user_size_ok", normalMsg);

    auto* mock = dynamic_cast<MockWebSocketConnection*>(conn.get());
    bool received = false;
    for (const auto& msg : mock->sentMessages) {
        if (msg == normalMsg) received = true;
    }
    EXPECT_TRUE(received);
}

TEST_F(WebSocketHubTest, MessageSizeLimit_ExceedsLimit_Dropped) {
    auto conn = addConn("user_size_big");
    std::string hugeMsg(65 * 1024, 'X'); // 65KB > 64KB 限制

    hub_.sendToUser("user_size_big", hugeMsg);

    auto* mock = dynamic_cast<MockWebSocketConnection*>(conn.get());
    // 超限消息应该被丢弃
    bool received = false;
    for (const auto& msg : mock->sentMessages) {
        if (msg == hugeMsg) received = true;
    }
    EXPECT_FALSE(received);
}

TEST_F(WebSocketHubTest, BroadcastSizeLimit_ExceedsLimit_Dropped) {
    auto conn = addConn("user_bc_big");
    std::string hugeMsg(65 * 1024, 'Y');

    hub_.broadcast(hugeMsg);

    auto* mock = dynamic_cast<MockWebSocketConnection*>(conn.get());
    bool received = false;
    for (const auto& msg : mock->sentMessages) {
        if (msg == hugeMsg) received = true;
    }
    EXPECT_FALSE(received);
}

TEST_F(WebSocketHubTest, RoomMessageSizeLimit_ExceedsLimit_Dropped) {
    auto conn = addConn("user_room_big");
    hub_.joinRoom(conn, "big_room");
    std::string hugeMsg(65 * 1024, 'Z');

    hub_.sendToRoom("big_room", hugeMsg);

    auto* mock = dynamic_cast<MockWebSocketConnection*>(conn.get());
    bool received = false;
    for (const auto& msg : mock->sentMessages) {
        if (msg == hugeMsg) received = true;
    }
    EXPECT_FALSE(received);
}

// ==================== 心跳与超时 ====================

TEST_F(WebSocketHubTest, UpdatePing_NoError) {
    auto conn = addConn("user_ping");
    EXPECT_NO_THROW(hub_.updatePing(conn));
}

TEST_F(WebSocketHubTest, UpdatePing_NonExistentConn_NoError) {
    auto fakeConn = makeMockConn();
    EXPECT_NO_THROW(hub_.updatePing(fakeConn));
}

// ==================== 私有房间格式验证 ====================

TEST_F(WebSocketHubTest, PrivateRoom_FormatValidation) {
    // 私有房间格式: private:{userId1}_{userId2}
    std::string room = "private:user_A_user_B";
    EXPECT_EQ(room.find("private:"), 0u);

    std::string body = room.substr(8);
    EXPECT_EQ(body, "user_A_user_B");
}

TEST_F(WebSocketHubTest, PrivateRoom_ParticipantExtraction) {
    std::string room = "private:alice_bob";
    std::string body = room.substr(8);

    // 按 '_' 分割
    std::vector<std::string> participants;
    std::istringstream ss(body);
    std::string part;
    while (std::getline(ss, part, '_')) {
        participants.push_back(part);
    }

    EXPECT_EQ(participants.size(), 2u);
    EXPECT_EQ(participants[0], "alice");
    EXPECT_EQ(participants[1], "bob");
}

TEST_F(WebSocketHubTest, PrivateRoom_AuthorizedUser) {
    std::string connUserId = "alice";
    std::string roomBody = "alice_bob";

    bool authorized = false;
    std::istringstream ss(roomBody);
    std::string participant;
    while (std::getline(ss, participant, '_')) {
        if (participant == connUserId) {
            authorized = true;
            break;
        }
    }
    EXPECT_TRUE(authorized);
}

TEST_F(WebSocketHubTest, PrivateRoom_UnauthorizedUser) {
    std::string connUserId = "charlie";
    std::string roomBody = "alice_bob";

    bool authorized = false;
    std::istringstream ss(roomBody);
    std::string participant;
    while (std::getline(ss, participant, '_')) {
        if (participant == connUserId) {
            authorized = true;
            break;
        }
    }
    EXPECT_FALSE(authorized);
}

// ==================== 并发安全 ====================

TEST_F(WebSocketHubTest, ConcurrentAddRemove_NoDataRace) {
    // 多线程同时添加和移除连接，验证无崩溃
    constexpr int threadCount = 8;
    constexpr int opsPerThread = 50;
    std::atomic<int> completedOps{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([this, t, &completedOps]() {
            for (int i = 0; i < opsPerThread; ++i) {
                auto conn = makeMockConn();
                std::string userId = "concurrent_user_" + std::to_string(t) + "_" + std::to_string(i);
                std::string room = "concurrent_room_" + std::to_string(t);
                hub_.addConnection(conn, userId);
                hub_.joinRoom(conn, room);
                hub_.leaveRoom(conn, room);
                hub_.removeConnection(conn);
                completedOps++;
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(completedOps.load(), threadCount * opsPerThread);
}
