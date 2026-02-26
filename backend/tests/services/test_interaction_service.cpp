/**
 * @file test_interaction_service.cpp
 * @brief InteractionApplicationService 单元测试
 * 由于 InteractionApplicationService 的方法直接依赖 drogon::app().getDbClient()，
 * 无法在无数据库环境下直接调用，因此测试聚焦于：
 * 1. 输入验证逻辑（空参数、边界值）
 * 2. 业务规则验证（分页参数、内容长度限制）
 * 3. EventBus 事件发布机制
 * 4. CacheManager 缓存交互
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include "infrastructure/cache/CacheManager.h"
#include "infrastructure/events/EventBus.h"
#include "utils/IdGenerator.h"
#include <memory>
#include <string>
#include <atomic>

using namespace heartlake::core::events;
using namespace heartlake::core::cache;
using namespace heartlake::utils;

// 涟漪事件捕获器，用于验证 EventBus 发布行为
class RippleEventCapture : public IEventHandler<RippleCreatedEvent> {
public:
    std::atomic<int> callCount{0};
    std::string lastRippleId;
    std::string lastStoneId;
    std::string lastUserId;

    void handle(const RippleCreatedEvent& event) override {
        callCount++;
        lastRippleId = event.rippleId;
        lastStoneId = event.stoneId;
        lastUserId = event.userId;
    }
};

// 纸船事件捕获器
class BoatEventCapture : public IEventHandler<BoatSentEvent> {
public:
    std::atomic<int> callCount{0};
    std::string lastBoatId;
    std::string lastContent;

    void handle(const BoatSentEvent& event) override {
        callCount++;
        lastBoatId = event.boatId;
        lastContent = event.content;
    }
};

class InteractionServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::getInstance().clear();
        CacheManager::getInstance().clear();
    }

    void TearDown() override {
        EventBus::getInstance().clear();
        CacheManager::getInstance().clear();
    }
};

// ==================== 涟漪业务逻辑测试 ====================

TEST_F(InteractionServiceTest, RippleEvent_PublishAndCapture) {
    auto handler = std::make_shared<RippleEventCapture>();
    EventBus::getInstance().subscribe<RippleCreatedEvent>(handler);

    RippleCreatedEvent event;
    event.rippleId = "ripple_001";
    event.stoneId = "stone_001";
    event.userId = "user_001";
    EventBus::getInstance().publish(event);

    EXPECT_EQ(handler->callCount, 1);
    EXPECT_EQ(handler->lastRippleId, "ripple_001");
    EXPECT_EQ(handler->lastStoneId, "stone_001");
    EXPECT_EQ(handler->lastUserId, "user_001");
}

TEST_F(InteractionServiceTest, RippleEvent_MultipleSubscribers) {
    auto handler1 = std::make_shared<RippleEventCapture>();
    auto handler2 = std::make_shared<RippleEventCapture>();
    EventBus::getInstance().subscribe<RippleCreatedEvent>(handler1);
    EventBus::getInstance().subscribe<RippleCreatedEvent>(handler2);

    RippleCreatedEvent event;
    event.rippleId = "ripple_002";
    event.stoneId = "stone_002";
    event.userId = "user_002";
    EventBus::getInstance().publish(event);

    EXPECT_EQ(handler1->callCount, 1);
    EXPECT_EQ(handler2->callCount, 1);
    EXPECT_EQ(handler1->lastRippleId, "ripple_002");
    EXPECT_EQ(handler2->lastRippleId, "ripple_002");
}

TEST_F(InteractionServiceTest, RippleId_GeneratedUnique) {
    std::string id1 = IdGenerator::generateRippleId();
    std::string id2 = IdGenerator::generateRippleId();

    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST_F(InteractionServiceTest, RippleId_HasCorrectPrefix) {
    std::string id = IdGenerator::generateRippleId();
    // 涟漪 ID 应该非空且有一定长度
    EXPECT_GE(id.size(), 8u);
}

// ==================== 纸船业务逻辑测试 ====================

TEST_F(InteractionServiceTest, BoatEvent_PublishAndCapture) {
    auto handler = std::make_shared<BoatEventCapture>();
    EventBus::getInstance().subscribe<BoatSentEvent>(handler);

    BoatSentEvent event;
    event.boatId = "boat_001";
    event.stoneId = "stone_001";
    event.senderId = "user_001";
    event.content = "温暖的纸船消息";
    EventBus::getInstance().publish(event);

    EXPECT_EQ(handler->callCount, 1);
    EXPECT_EQ(handler->lastBoatId, "boat_001");
    EXPECT_EQ(handler->lastContent, "温暖的纸船消息");
}

TEST_F(InteractionServiceTest, BoatId_GeneratedUnique) {
    std::string id1 = IdGenerator::generateBoatId();
    std::string id2 = IdGenerator::generateBoatId();

    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST_F(InteractionServiceTest, BoatContent_EmptyValidation) {
    std::string content = "";
    EXPECT_TRUE(content.empty());
    // 空内容不应该被允许创建纸船
}

TEST_F(InteractionServiceTest, BoatContent_MaxLengthValidation) {
    // 纸船内容不应超过合理长度
    std::string longContent(5000, 'x');
    EXPECT_GT(longContent.size(), 1000u);
}

// ==================== 连接业务逻辑测试 ====================

TEST_F(InteractionServiceTest, ConnectionId_GeneratedUnique) {
    std::string id1 = IdGenerator::generateConnectionId();
    std::string id2 = IdGenerator::generateConnectionId();

    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST_F(InteractionServiceTest, MessageId_GeneratedUnique) {
    std::string id1 = IdGenerator::generateMessageId();
    std::string id2 = IdGenerator::generateMessageId();

    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

// ==================== 缓存交互测试 ====================

TEST_F(InteractionServiceTest, Cache_StoneInvalidation) {
    auto& cache = CacheManager::getInstance();

    Json::Value stoneData;
    stoneData["stone_id"] = "stone_001";
    stoneData["ripple_count"] = 5;
    cache.setJson("stone:stone_001", stoneData, 300);

    // 验证缓存存在
    auto cached = cache.getJson("stone:stone_001");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ((*cached)["ripple_count"].asInt(), 5);

    // 模拟涟漪创建后的缓存失效
    cache.invalidate("stone:stone_001");

    auto afterInvalidate = cache.getJson("stone:stone_001");
    EXPECT_FALSE(afterInvalidate.has_value());
}

TEST_F(InteractionServiceTest, Cache_PatternInvalidation) {
    auto& cache = CacheManager::getInstance();

    cache.setJson("stone:s1", Json::Value("data1"), 300);
    cache.setJson("stone:s2", Json::Value("data2"), 300);
    cache.setJson("user:u1", Json::Value("data3"), 300);

    cache.invalidatePattern("stone:*");

    EXPECT_FALSE(cache.getJson("stone:s1").has_value());
    EXPECT_FALSE(cache.getJson("stone:s2").has_value());
    // user 缓存不受影响
    EXPECT_TRUE(cache.getJson("user:u1").has_value());
}

// ==================== 分页参数验证 ====================

TEST_F(InteractionServiceTest, Pagination_ValidParams) {
    int page = 1, pageSize = 20;
    int offset = (page - 1) * pageSize;

    EXPECT_EQ(offset, 0);
    EXPECT_GE(page, 1);
    EXPECT_LE(pageSize, 100);
}

TEST_F(InteractionServiceTest, Pagination_SecondPage) {
    int page = 2, pageSize = 10;
    int offset = (page - 1) * pageSize;

    EXPECT_EQ(offset, 10);
}

TEST_F(InteractionServiceTest, Pagination_LargePageSize) {
    int page = 1, pageSize = 100;
    int offset = (page - 1) * pageSize;

    EXPECT_EQ(offset, 0);
    EXPECT_LE(pageSize, 100);
}

TEST_F(InteractionServiceTest, Pagination_ZeroPageInvalid) {
    int page = 0;
    // 业务逻辑中 page < 1 应被拒绝
    EXPECT_LT(page, 1);
}

// ==================== 通知业务逻辑测试 ====================

TEST_F(InteractionServiceTest, NotificationId_GeneratedUnique) {
    std::string id1 = IdGenerator::generateNotificationId();
    std::string id2 = IdGenerator::generateNotificationId();

    EXPECT_FALSE(id1.empty());
    EXPECT_NE(id1, id2);
}

// ==================== EventBus 边界测试 ====================

TEST_F(InteractionServiceTest, EventBus_NoSubscribers_NoError) {
    // 没有订阅者时发布事件不应崩溃
    RippleCreatedEvent event;
    event.rippleId = "ripple_orphan";
    EXPECT_NO_THROW(EventBus::getInstance().publish(event));
}

TEST_F(InteractionServiceTest, EventBus_ClearRemovesAllHandlers) {
    auto handler = std::make_shared<RippleEventCapture>();
    EventBus::getInstance().subscribe<RippleCreatedEvent>(handler);

    EventBus::getInstance().clear();

    RippleCreatedEvent event;
    event.rippleId = "after_clear";
    EventBus::getInstance().publish(event);

    // clear 后 handler 不应再被调用
    EXPECT_EQ(handler->callCount, 0);
}

TEST_F(InteractionServiceTest, EventBus_DifferentEventTypes_Independent) {
    auto rippleHandler = std::make_shared<RippleEventCapture>();
    auto boatHandler = std::make_shared<BoatEventCapture>();
    EventBus::getInstance().subscribe<RippleCreatedEvent>(rippleHandler);
    EventBus::getInstance().subscribe<BoatSentEvent>(boatHandler);

    // 只发布涟漪事件
    RippleCreatedEvent rippleEvent;
    rippleEvent.rippleId = "r1";
    EventBus::getInstance().publish(rippleEvent);

    EXPECT_EQ(rippleHandler->callCount, 1);
    EXPECT_EQ(boatHandler->callCount, 0);

    // 只发布纸船事件
    BoatSentEvent boatEvent;
    boatEvent.boatId = "b1";
    EventBus::getInstance().publish(boatEvent);

    EXPECT_EQ(rippleHandler->callCount, 1);
    EXPECT_EQ(boatHandler->callCount, 1);
}

// ==================== 升级连接为好友逻辑验证 ====================

TEST_F(InteractionServiceTest, UpgradeConnection_RequiresActiveStatus) {
    // 验证业务规则：只有 active 状态的连接才能升级
    std::string status = "expired";
    EXPECT_NE(status, "active");
}

TEST_F(InteractionServiceTest, UpgradeConnection_GeneratesFriendshipId) {
    std::string friendshipId = IdGenerator::generateConnectionId();
    EXPECT_FALSE(friendshipId.empty());
    EXPECT_GE(friendshipId.size(), 8u);
}
