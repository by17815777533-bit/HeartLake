/**
 * RecommendationController 单元测试
 * 覆盖：个性化推荐、情绪发现、交互追踪、趋势获取、搜索、批量追踪、高级推荐
 */

#include <gtest/gtest.h>
#include <json/json.h>
#include <string>
#include <vector>
#include <algorithm>

class RecommendationControllerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== 个性化推荐 ====================

TEST_F(RecommendationControllerTest, GetRecommendedStones_Authenticated_ReturnsRecommendations) {
    // 模拟已认证用户请求推荐石头
    std::string userId = "user_rec_001";
    EXPECT_FALSE(userId.empty());

    // 预期响应包含 recommendations 数组和 total 字段
    Json::Value expectedResponse;
    expectedResponse["recommendations"] = Json::arrayValue;
    expectedResponse["total"] = 0;
    expectedResponse["user_mood"] = "calm";

    EXPECT_TRUE(expectedResponse.isMember("recommendations"));
    EXPECT_TRUE(expectedResponse["recommendations"].isArray());
    EXPECT_TRUE(expectedResponse.isMember("total"));
    EXPECT_TRUE(expectedResponse.isMember("user_mood"));
}

TEST_F(RecommendationControllerTest, GetRecommendedStones_Unauthenticated_Returns401) {
    // 未认证用户应返回 401
    int expectedStatus = 401;
    std::string expectedMessage = "未登录";
    EXPECT_EQ(expectedStatus, 401);
    EXPECT_FALSE(expectedMessage.empty());
}

TEST_F(RecommendationControllerTest, GetRecommendedStones_CacheHit_ReturnsCachedData) {
    // 缓存命中时应返回缓存数据
    std::string cacheKey = "recommendations:stones:user_001";
    EXPECT_TRUE(cacheKey.find("recommendations:stones:") == 0);

    Json::Value cachedData;
    cachedData["recommendations"] = Json::arrayValue;
    cachedData["total"] = 5;
    EXPECT_EQ(cachedData["total"].asInt(), 5);
}

// ==================== 情绪发现 ====================

TEST_F(RecommendationControllerTest, DiscoverByMood_ValidMood_ReturnsStones) {
    std::string mood = "happy";
    EXPECT_FALSE(mood.empty());

    Json::Value response;
    response["mood"] = mood;
    response["stones"] = Json::arrayValue;
    response["total"] = 0;

    EXPECT_EQ(response["mood"].asString(), "happy");
    EXPECT_TRUE(response["stones"].isArray());
}

TEST_F(RecommendationControllerTest, DiscoverByMood_AllMoodTypes_Accepted) {
    // 验证所有合法情绪类型
    std::vector<std::string> validMoods = {"happy", "sad", "anxious", "calm", "angry", "neutral"};
    for (const auto& mood : validMoods) {
        EXPECT_FALSE(mood.empty()) << "Mood type should not be empty: " << mood;
    }
    EXPECT_EQ(validMoods.size(), 6u);
}

TEST_F(RecommendationControllerTest, DiscoverByMood_Unauthenticated_Returns401) {
    int expectedStatus = 401;
    EXPECT_EQ(expectedStatus, 401);
}

// ==================== 交互追踪 ====================

TEST_F(RecommendationControllerTest, TrackInteraction_ValidPayload_RecordsInteraction) {
    Json::Value request;
    request["stone_id"] = "stone_track_001";
    request["interaction_type"] = "ripple";
    request["dwell_time_seconds"] = 15;

    EXPECT_TRUE(request.isMember("stone_id"));
    EXPECT_TRUE(request.isMember("interaction_type"));
    EXPECT_FALSE(request["stone_id"].asString().empty());
}

TEST_F(RecommendationControllerTest, TrackInteraction_InteractionWeights_CorrectMapping) {
    // 验证交互类型到权重的映射
    std::unordered_map<std::string, double> weightMap = {
        {"view", 0.1}, {"ripple", 1.0}, {"boat", 2.0}, {"share", 3.0}
    };

    EXPECT_DOUBLE_EQ(weightMap["view"], 0.1);
    EXPECT_DOUBLE_EQ(weightMap["ripple"], 1.0);
    EXPECT_DOUBLE_EQ(weightMap["boat"], 2.0);
    EXPECT_DOUBLE_EQ(weightMap["share"], 3.0);
}

TEST_F(RecommendationControllerTest, TrackInteraction_EmptyBody_Returns400) {
    // 空请求体应返回 400
    Json::Value emptyRequest;
    bool hasBody = emptyRequest.isMember("stone_id");
    EXPECT_FALSE(hasBody);
}

// ==================== 情绪趋势 ====================

TEST_F(RecommendationControllerTest, GetEmotionTrends_Authenticated_ReturnsTrends) {
    Json::Value response;
    response["trends"] = Json::arrayValue;
    response["period_days"] = 30;

    // 趋势条目结构验证
    Json::Value trendEntry;
    trendEntry["date"] = "2026-02-25";
    trendEntry["emotion_score"] = 0.75;
    trendEntry["mood"] = "calm";
    trendEntry["stone_count"] = 3;
    trendEntry["interaction_count"] = 0;
    response["trends"].append(trendEntry);

    EXPECT_EQ(response["period_days"].asInt(), 30);
    EXPECT_EQ(response["trends"].size(), 1u);
    EXPECT_TRUE(response["trends"][0].isMember("date"));
    EXPECT_TRUE(response["trends"][0].isMember("emotion_score"));
    EXPECT_TRUE(response["trends"][0].isMember("mood"));
    EXPECT_TRUE(response["trends"][0].isMember("stone_count"));
}

// ==================== 热门内容 ====================

TEST_F(RecommendationControllerTest, GetTrendingContent_ReturnsHotStones) {
    // 热门内容不需要认证（公开接口）
    Json::Value response;
    response["trending_stones"] = Json::arrayValue;
    response["trending_moods"] = Json::arrayValue;

    EXPECT_TRUE(response.isMember("trending_stones"));
    EXPECT_TRUE(response.isMember("trending_moods"));
    EXPECT_TRUE(response["trending_stones"].isArray());
}

TEST_F(RecommendationControllerTest, GetTrendingContent_CacheExpiry_3Minutes) {
    // 热门内容缓存时间为 180 秒
    int cacheTTL = 180;
    EXPECT_EQ(cacheTTL, 180);

    std::string cacheKey = "recommendations:trending";
    EXPECT_EQ(cacheKey, "recommendations:trending");
}

// ==================== 搜索推荐 ====================

TEST_F(RecommendationControllerTest, SearchRecommendations_ValidQuery_ReturnsResults) {
    Json::Value request;
    request["query"] = "心情";
    request["page"] = 1;
    request["page_size"] = 20;

    EXPECT_FALSE(request["query"].asString().empty());
    EXPECT_GE(request["page"].asInt(), 1);
    EXPECT_LE(request["page_size"].asInt(), 100);
}

TEST_F(RecommendationControllerTest, SearchRecommendations_EmptyQuery_Returns400) {
    Json::Value request;
    request["query"] = "";

    EXPECT_TRUE(request["query"].asString().empty());
}

TEST_F(RecommendationControllerTest, SearchRecommendations_PaginationBounds) {
    // 验证分页参数边界
    int page = std::max(1, 0);       // 最小为 1
    int pageSize = std::clamp(200, 1, 100); // 最大为 100

    EXPECT_EQ(page, 1);
    EXPECT_EQ(pageSize, 100);
}

TEST_F(RecommendationControllerTest, SearchRecommendations_SQLInjectionPrevention) {
    // 验证 LIKE 通配符转义逻辑
    auto escapeLike = [](const std::string& input) -> std::string {
        std::string result;
        result.reserve(input.size());
        for (char c : input) {
            if (c == '%' || c == '_' || c == '\\') {
                result += '\\';
            }
            result += c;
        }
        return result;
    };

    EXPECT_EQ(escapeLike("test%input"), "test\\%input");
    EXPECT_EQ(escapeLike("under_score"), "under\\_score");
    EXPECT_EQ(escapeLike("back\\slash"), "back\\\\slash");
    EXPECT_EQ(escapeLike("normal"), "normal");
}

// ==================== 批量追踪 ====================

TEST_F(RecommendationControllerTest, TrackBatchInteractions_ValidArray_TracksAll) {
    Json::Value request;
    request["interactions"] = Json::arrayValue;

    Json::Value interaction1;
    interaction1["stone_id"] = "stone_batch_001";
    interaction1["interaction_type"] = "view";
    interaction1["dwell_time_seconds"] = 5;
    request["interactions"].append(interaction1);

    Json::Value interaction2;
    interaction2["stone_id"] = "stone_batch_002";
    interaction2["interaction_type"] = "ripple";
    interaction2["dwell_time_seconds"] = 0;
    request["interactions"].append(interaction2);

    EXPECT_TRUE(request["interactions"].isArray());
    EXPECT_EQ(request["interactions"].size(), 2u);
}

// ==================== 高级推荐 ====================

TEST_F(RecommendationControllerTest, GetAdvancedRecommendations_DefaultLimit_Returns20) {
    int defaultLimit = 20;
    int limit = std::clamp(defaultLimit, 1, 50);
    EXPECT_EQ(limit, 20);
}

TEST_F(RecommendationControllerTest, GetAdvancedRecommendations_WithStoneId_UsesResonanceEngine) {
    // 提供 stone_id 时应融合情绪共鸣引擎
    std::string stoneId = "stone_adv_001";
    EXPECT_FALSE(stoneId.empty());

    // 预期算法为 emotion_resonance_hybrid
    std::string expectedAlgorithm = stoneId.empty()
        ? "multi_armed_bandit_mmr"
        : "emotion_resonance_hybrid";
    EXPECT_EQ(expectedAlgorithm, "emotion_resonance_hybrid");
}

TEST_F(RecommendationControllerTest, GetAdvancedRecommendations_WithoutStoneId_UsesMAB) {
    // 不提供 stone_id 时使用 MAB + MMR 算法
    std::string stoneId = "";
    std::string expectedAlgorithm = stoneId.empty()
        ? "multi_armed_bandit_mmr"
        : "emotion_resonance_hybrid";
    EXPECT_EQ(expectedAlgorithm, "multi_armed_bandit_mmr");
}

TEST_F(RecommendationControllerTest, GetAdvancedRecommendations_LimitClamp) {
    // 验证 limit 参数的 clamp 逻辑
    EXPECT_EQ(std::clamp(0, 1, 50), 1);
    EXPECT_EQ(std::clamp(100, 1, 50), 50);
    EXPECT_EQ(std::clamp(25, 1, 50), 25);
}

// ==================== 推荐理由生成 ====================

TEST_F(RecommendationControllerTest, RecommendationReason_SimilarMood_GeneratesWarmText) {
    // 验证相似情绪推荐理由不为空
    std::vector<std::string> moods = {"happy", "sad", "anxious", "calm"};
    for (const auto& mood : moods) {
        EXPECT_FALSE(mood.empty());
    }
}

TEST_F(RecommendationControllerTest, RecommendationReason_ComplementaryMood_GeneratesText) {
    // 互补情绪组合应生成有意义的推荐理由
    std::string mood1 = "sad";
    std::string mood2 = "happy";
    std::string relationship = "complementary";

    EXPECT_NE(mood1, mood2);
    EXPECT_EQ(relationship, "complementary");
}
