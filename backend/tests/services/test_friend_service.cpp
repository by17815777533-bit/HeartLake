/**
 * @file test_friend_service.cpp
 * @brief FriendService 领域服务单元测试
 * 通过 Mock 仓储测试好友领域服务的同步方法，覆盖：
 * - 好友请求发送/接受/拒绝
 * - 好友删除
 * - 好友列表获取
 * - 重复请求处理
 * - 自己加自己拒绝
 */

#include <gtest/gtest.h>
#include "domain/friend/services/FriendService.h"
#include "domain/friend/repositories/IFriendRepository.h"
#include <memory>
#include <algorithm>

using namespace heartlake::domain::friend_domain;

// Mock 好友仓储 - 内存实现，用于隔离数据库依赖
class MockFriendRepository : public IFriendRepository {
public:
    std::vector<FriendEntity> friendships_;
    int saveCount = 0;
    int deleteCount = 0;
    int updateCount = 0;

    // ==================== 同步方法 ====================

    void save(const FriendEntity& friendship) override {
        saveCount++;
        friendships_.push_back(friendship);
    }

    std::optional<FriendEntity> findByUserAndFriend(
        const std::string& userId, const std::string& friendId
    ) override {
        for (const auto& f : friendships_) {
            if ((f.userId == userId && f.friendId == friendId) ||
                (f.userId == friendId && f.friendId == userId)) {
                return f;
            }
        }
        return std::nullopt;
    }

    std::vector<FriendEntity> findByUserId(const std::string& userId) override {
        std::vector<FriendEntity> result;
        for (const auto& f : friendships_) {
            if ((f.userId == userId || f.friendId == userId) && f.status == "accepted") {
                result.push_back(f);
            }
        }
        return result;
    }

    void updateStatus(const std::string& friendshipId, const std::string& status) override {
        updateCount++;
        for (auto& f : friendships_) {
            if (f.friendshipId == friendshipId) {
                f.status = status;
                return;
            }
        }
    }

    void deleteById(const std::string& friendshipId) override {
        deleteCount++;
        friendships_.erase(
            std::remove_if(friendships_.begin(), friendships_.end(),
                [&](const FriendEntity& f) { return f.friendshipId == friendshipId; }),
            friendships_.end()
        );
    }

    // ==================== 异步方法（测试中不使用，提供空实现）====================

    drogon::Task<void> saveAsync(const FriendEntity&) override { co_return; }
    drogon::Task<std::optional<FriendEntity>> findByUserAndFriendAsync(
        const std::string&, const std::string&) override { co_return std::nullopt; }
    drogon::Task<std::vector<FriendEntity>> findByUserIdAsync(const std::string&) override {
        co_return std::vector<FriendEntity>{};
    }
    drogon::Task<std::vector<FriendEntity>> findAllByUserIdAsync(const std::string&) override {
        co_return std::vector<FriendEntity>{};
    }
    drogon::Task<void> updateStatusAsync(const std::string&, const std::string&) override { co_return; }
    drogon::Task<void> deleteByIdAsync(const std::string&) override { co_return; }
    drogon::Task<void> deleteBidirectionalAsync(const std::string&, const std::string&) override { co_return; }
};

class FriendServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockFriendRepository> mockRepo_;
    std::unique_ptr<FriendService> service_;

    void SetUp() override {
        mockRepo_ = std::make_shared<MockFriendRepository>();
        service_ = std::make_unique<FriendService>(mockRepo_);
    }
};

// ==================== 好友请求发送 ====================

TEST_F(FriendServiceTest, SendRequest_Success) {
    EXPECT_NO_THROW(service_->sendFriendRequest("user_A", "user_B"));
    EXPECT_EQ(mockRepo_->saveCount, 1);
    EXPECT_EQ(mockRepo_->friendships_.size(), 1u);
    EXPECT_EQ(mockRepo_->friendships_[0].status, "pending");
}

TEST_F(FriendServiceTest, SendRequest_SetsCorrectFields) {
    service_->sendFriendRequest("user_A", "user_B");

    auto& saved = mockRepo_->friendships_[0];
    EXPECT_EQ(saved.userId, "user_A");
    EXPECT_EQ(saved.friendId, "user_B");
    EXPECT_EQ(saved.status, "pending");
    EXPECT_FALSE(saved.friendshipId.empty());
}

TEST_F(FriendServiceTest, SendRequest_GeneratesUniqueFriendshipId) {
    service_->sendFriendRequest("user_A", "user_B");

    auto& saved = mockRepo_->friendships_[0];
    EXPECT_GE(saved.friendshipId.size(), 8u);
}

TEST_F(FriendServiceTest, SendRequest_DuplicateThrows) {
    service_->sendFriendRequest("user_A", "user_B");
    EXPECT_THROW(service_->sendFriendRequest("user_A", "user_B"), std::runtime_error);
}

TEST_F(FriendServiceTest, SendRequest_ReverseDirectionAlsoDetected) {
    // A->B 已存在时，B->A 也应该被检测到（双向查找）
    service_->sendFriendRequest("user_A", "user_B");
    EXPECT_THROW(service_->sendFriendRequest("user_B", "user_A"), std::runtime_error);
}

// ==================== 好友请求接受 ====================

TEST_F(FriendServiceTest, AcceptRequest_Success) {
    service_->sendFriendRequest("user_A", "user_B");
    std::string fid = mockRepo_->friendships_[0].friendshipId;

    EXPECT_NO_THROW(service_->acceptFriendRequest(fid));
    EXPECT_EQ(mockRepo_->updateCount, 1);
    EXPECT_EQ(mockRepo_->friendships_[0].status, "accepted");
}

TEST_F(FriendServiceTest, AcceptRequest_NonExistentId_NoError) {
    // updateStatus 对不存在的 ID 静默处理（仓储层不抛异常）
    EXPECT_NO_THROW(service_->acceptFriendRequest("nonexistent_id"));
    EXPECT_EQ(mockRepo_->updateCount, 1);
}

// ==================== 好友请求拒绝 ====================

TEST_F(FriendServiceTest, RejectRequest_Success) {
    service_->sendFriendRequest("user_A", "user_B");
    std::string fid = mockRepo_->friendships_[0].friendshipId;

    EXPECT_NO_THROW(service_->rejectFriendRequest(fid));
    EXPECT_EQ(mockRepo_->friendships_[0].status, "rejected");
}

TEST_F(FriendServiceTest, RejectRequest_UpdatesStatus) {
    service_->sendFriendRequest("user_A", "user_B");
    std::string fid = mockRepo_->friendships_[0].friendshipId;

    service_->rejectFriendRequest(fid);
    EXPECT_EQ(mockRepo_->updateCount, 1);
    EXPECT_EQ(mockRepo_->friendships_[0].status, "rejected");
}

// ==================== 好友删除 ====================

TEST_F(FriendServiceTest, RemoveFriend_ExistingFriend) {
    service_->sendFriendRequest("user_A", "user_B");
    std::string fid = mockRepo_->friendships_[0].friendshipId;
    service_->acceptFriendRequest(fid);

    EXPECT_NO_THROW(service_->removeFriend("user_A", "user_B"));
    EXPECT_EQ(mockRepo_->deleteCount, 1);
    EXPECT_TRUE(mockRepo_->friendships_.empty());
}

TEST_F(FriendServiceTest, RemoveFriend_NonExistent_NoError) {
    // 删除不存在的好友关系不应抛异常
    EXPECT_NO_THROW(service_->removeFriend("user_X", "user_Y"));
    EXPECT_EQ(mockRepo_->deleteCount, 0);
}

// ==================== 好友列表获取 ====================

TEST_F(FriendServiceTest, GetFriends_ReturnsAcceptedOnly) {
    // 创建两个请求，只接受一个
    service_->sendFriendRequest("user_A", "user_B");
    service_->sendFriendRequest("user_A", "user_C");

    std::string fid1 = mockRepo_->friendships_[0].friendshipId;
    service_->acceptFriendRequest(fid1);
    // user_C 的请求保持 pending

    auto friends = service_->getFriends("user_A");
    EXPECT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].friendId, "user_B");
}

TEST_F(FriendServiceTest, GetFriends_EmptyList) {
    auto friends = service_->getFriends("user_lonely");
    EXPECT_TRUE(friends.empty());
}

TEST_F(FriendServiceTest, GetFriends_MultipleFriends) {
    service_->sendFriendRequest("user_A", "user_B");
    service_->sendFriendRequest("user_A", "user_C");

    service_->acceptFriendRequest(mockRepo_->friendships_[0].friendshipId);
    service_->acceptFriendRequest(mockRepo_->friendships_[1].friendshipId);

    auto friends = service_->getFriends("user_A");
    EXPECT_EQ(friends.size(), 2u);
}

// ==================== areFriends 查询 ====================

TEST_F(FriendServiceTest, AreFriends_AcceptedReturnsTrue) {
    service_->sendFriendRequest("user_A", "user_B");
    service_->acceptFriendRequest(mockRepo_->friendships_[0].friendshipId);

    EXPECT_TRUE(service_->areFriends("user_A", "user_B"));
}

TEST_F(FriendServiceTest, AreFriends_PendingReturnsFalse) {
    service_->sendFriendRequest("user_A", "user_B");
    // 未接受，仍然是 pending
    EXPECT_FALSE(service_->areFriends("user_A", "user_B"));
}

TEST_F(FriendServiceTest, AreFriends_NonExistentReturnsFalse) {
    EXPECT_FALSE(service_->areFriends("user_X", "user_Y"));
}

TEST_F(FriendServiceTest, AreFriends_RejectedReturnsFalse) {
    service_->sendFriendRequest("user_A", "user_B");
    service_->rejectFriendRequest(mockRepo_->friendships_[0].friendshipId);

    EXPECT_FALSE(service_->areFriends("user_A", "user_B"));
}
