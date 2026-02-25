/**
 * @file stone_service_test.cpp
 * @brief StoneService 单元测试
 */

#include <gtest/gtest.h>
#include "domain/stone/services/StoneService.h"
#include "domain/stone/repositories/IStoneRepository.h"
#include <memory>
#include <algorithm>

using namespace heartlake::domain::stone;

class MockStoneRepository : public IStoneRepository {
public:
    std::vector<StoneEntity> stones_;
    int saveCallCount = 0;
    int deleteCallCount = 0;

    StoneEntity save(const StoneEntity& stone) override {
        saveCallCount++;
        StoneEntity saved = stone;
        saved.stoneId = "stone_" + std::to_string(stones_.size() + 1);
        stones_.push_back(saved);
        return saved;
    }

    std::optional<StoneEntity> findById(const std::string& stoneId) override {
        for (const auto& s : stones_) {
            if (s.stoneId == stoneId) return s;
        }
        return std::nullopt;
    }

    std::vector<StoneEntity> findByUserId(const std::string& userId, int page, int pageSize) override {
        std::vector<StoneEntity> result;
        for (const auto& s : stones_) {
            if (s.userId == userId) result.push_back(s);
        }
        return result;
    }

    std::vector<StoneEntity> findAll(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) override {
        return stones_;
    }

    int countAll(const std::string& filterMood) override { return stones_.size(); }
    int countByUserId(const std::string& userId) override {
        int count = 0;
        for (const auto& s : stones_) {
            if (s.userId == userId) count++;
        }
        return count;
    }

    void deleteById(const std::string& stoneId) override {
        deleteCallCount++;
        stones_.erase(std::remove_if(stones_.begin(), stones_.end(),
            [&](const StoneEntity& s) { return s.stoneId == stoneId; }), stones_.end());
    }

    void incrementViewCount(const std::string& stoneId) override {}
    void updateEmotionScore(const std::string& stoneId, float score, const std::string& mood) override {}
};

class StoneServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockStoneRepository> mockRepo_;
    std::unique_ptr<StoneService> service_;

    void SetUp() override {
        mockRepo_ = std::make_shared<MockStoneRepository>();
        service_ = std::make_unique<StoneService>(mockRepo_);
    }
};

TEST_F(StoneServiceTest, CreateStone_Success) {
    auto stone = service_->createStone("user1", "test content", "medium", "#7A92A3", "happy", false);

    EXPECT_EQ(stone.userId, "user1");
    EXPECT_EQ(stone.content, "test content");
    EXPECT_EQ(stone.stoneType, "medium");
    EXPECT_EQ(stone.moodType, "happy");
    EXPECT_FALSE(stone.stoneId.empty());
    EXPECT_EQ(mockRepo_->saveCallCount, 1);
}

TEST_F(StoneServiceTest, GetStone_Exists_ReturnsStone) {
    service_->createStone("user1", "content", "medium", "#7A92A3", "happy", false);

    auto result = service_->getStone("stone_1");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->content, "content");
}

TEST_F(StoneServiceTest, GetStone_NotExists_ReturnsEmpty) {
    auto result = service_->getStone("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(StoneServiceTest, DeleteStone_OwnerCanDelete) {
    service_->createStone("user1", "content", "medium", "#7A92A3", "happy", false);

    EXPECT_NO_THROW(service_->deleteStone("stone_1", "user1"));
    EXPECT_EQ(mockRepo_->deleteCallCount, 1);
}

TEST_F(StoneServiceTest, DeleteStone_NonOwnerThrows) {
    service_->createStone("user1", "content", "medium", "#7A92A3", "happy", false);

    EXPECT_THROW(service_->deleteStone("stone_1", "user2"), std::runtime_error);
}

TEST_F(StoneServiceTest, DeleteStone_NotExistsThrows) {
    EXPECT_THROW(service_->deleteStone("nonexistent", "user1"), std::runtime_error);
}

TEST_F(StoneServiceTest, GetUserStones_ReturnsUserStones) {
    service_->createStone("user1", "content1", "medium", "#7A92A3", "happy", false);
    service_->createStone("user2", "content2", "medium", "#7A92A3", "sad", false);
    service_->createStone("user1", "content3", "medium", "#7A92A3", "happy", false);

    auto stones = service_->getUserStones("user1", 1, 10);

    EXPECT_EQ(stones.size(), 2);
}

TEST_F(StoneServiceTest, GetTotalCount_ReturnsCorrectCount) {
    service_->createStone("user1", "c1", "medium", "#7A92A3", "happy", false);
    service_->createStone("user2", "c2", "medium", "#7A92A3", "sad", false);

    EXPECT_EQ(service_->getTotalCount(), 2);
}

TEST_F(StoneServiceTest, GetUserStoneCount_ReturnsCorrectCount) {
    service_->createStone("user1", "c1", "medium", "#7A92A3", "happy", false);
    service_->createStone("user1", "c2", "medium", "#7A92A3", "sad", false);
    service_->createStone("user2", "c3", "medium", "#7A92A3", "happy", false);

    EXPECT_EQ(service_->getUserStoneCount("user1"), 2);
    EXPECT_EQ(service_->getUserStoneCount("user2"), 1);
}
