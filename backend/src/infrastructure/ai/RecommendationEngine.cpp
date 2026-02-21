/**
 * @file RecommendationEngine.cpp
 * @brief 高级推荐引擎实现 - 多算法融合
 */

#include "infrastructure/ai/RecommendationEngine.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <numeric>
#include <unordered_set>

using namespace drogon;

namespace heartlake {
namespace ai {

RecommendationEngine& RecommendationEngine::getInstance() {
    static RecommendationEngine instance;
    return instance;
}

void RecommendationEngine::initialize(int latentDim) {
    if (initialized_) return;
    latentDim_ = latentDim;
    initialized_ = true;
    LOG_INFO << "RecommendationEngine initialized with latent_dim=" << latentDim;
}

// ===== 核心算法实现 =====

double RecommendationEngine::predictMF(
    const std::vector<float>& userFactors,
    const std::vector<float>& itemFactors
) {
    if (userFactors.size() != itemFactors.size()) return 0.0;
    double dot = 0.0;
    for (size_t i = 0; i < userFactors.size(); ++i) {
        dot += userFactors[i] * itemFactors[i];
    }
    // Sigmoid归一化到[0,1]
    return 1.0 / (1.0 + std::exp(-dot));
}

double RecommendationEngine::computeUCB(double avgReward, int itemCount, int totalCount) {
    if (itemCount == 0) return std::numeric_limits<double>::max(); // 未探索优先
    double exploitation = avgReward;
    double exploration = ucbC_ * std::sqrt(std::log(totalCount + 1) / itemCount);
    return exploitation + exploration;
}

double RecommendationEngine::thompsonSample(int successes, int failures) {
    // Beta分布采样 (使用Gamma分布近似)
    std::gamma_distribution<double> gamma_a(successes + 1, 1.0);
    std::gamma_distribution<double> gamma_b(failures + 1, 1.0);
    double x = gamma_a(rng_);
    double y = gamma_b(rng_);
    double sum = x + y;
    if (sum < 1e-15) return 0.5;
    return x / sum;
}

double RecommendationEngine::timeDecay(int64_t timestampMs, double halfLifeHours) {
    if (halfLifeHours <= 0.0) halfLifeHours = 24.0;  // 默认24小时
    auto now = std::chrono::system_clock::now().time_since_epoch();
    int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    double hoursOld = (nowMs - timestampMs) / (1000.0 * 3600.0);
    return std::exp(-0.693 * hoursOld / halfLifeHours); // ln(2) ≈ 0.693
}

double RecommendationEngine::emotionCompatibilityScore(
    const std::string& userMood,
    const std::string& itemMood
) {
    // 情绪兼容性矩阵 - 基于心理学研究的情绪共鸣模型
    static const std::unordered_map<std::string, std::unordered_map<std::string, double>> matrix = {
        {"happy",    {{"happy", 1.0}, {"grateful", 0.9}, {"hopeful", 0.85}, {"neutral", 0.6}, {"anxious", 0.4}, {"sad", 0.5}, {"angry", 0.3}, {"confused", 0.5}}},
        {"sad",      {{"sad", 0.7}, {"hopeful", 0.8}, {"grateful", 0.7}, {"neutral", 0.5}, {"anxious", 0.6}, {"happy", 0.5}, {"angry", 0.4}, {"confused", 0.5}}},
        {"anxious",  {{"anxious", 0.6}, {"hopeful", 0.85}, {"neutral", 0.6}, {"sad", 0.6}, {"grateful", 0.7}, {"happy", 0.5}, {"angry", 0.5}, {"confused", 0.6}}},
        {"angry",    {{"angry", 0.4}, {"hopeful", 0.7}, {"neutral", 0.5}, {"anxious", 0.5}, {"sad", 0.5}, {"grateful", 0.5}, {"happy", 0.4}, {"confused", 0.5}}},
        {"grateful", {{"grateful", 1.0}, {"happy", 0.9}, {"hopeful", 0.9}, {"neutral", 0.7}, {"sad", 0.6}, {"anxious", 0.6}, {"angry", 0.4}, {"confused", 0.6}}},
        {"hopeful",  {{"hopeful", 1.0}, {"grateful", 0.9}, {"happy", 0.85}, {"neutral", 0.7}, {"sad", 0.75}, {"anxious", 0.8}, {"angry", 0.6}, {"confused", 0.75}}},
        {"neutral",  {{"neutral", 0.8}, {"hopeful", 0.7}, {"grateful", 0.7}, {"happy", 0.7}, {"sad", 0.6}, {"anxious", 0.6}, {"angry", 0.5}, {"confused", 0.7}}},
        {"confused", {{"confused", 0.7}, {"hopeful", 0.85}, {"calm", 0.8}, {"neutral", 0.7}, {"grateful", 0.7}, {"happy", 0.6}, {"sad", 0.5}, {"anxious", 0.6}}}
    };

    auto it1 = matrix.find(userMood);
    if (it1 != matrix.end()) {
        auto it2 = it1->second.find(itemMood);
        if (it2 != it1->second.end()) return it2->second;
    }
    return 0.5;
}

std::vector<RecommendationCandidate> RecommendationEngine::mmrRerank(
    const std::vector<RecommendationCandidate>& candidates,
    double lambda,
    int topK
) {
    if (candidates.empty()) return {};

    auto& embEngine = AdvancedEmbeddingEngine::getInstance();

    // 预计算所有embedding，避免重复生成
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(candidates.size());
    for (const auto& cand : candidates) {
        embeddings.push_back(embEngine.generateEmbedding(
            cand.metadata.get("content", "").asString()));
    }

    std::vector<RecommendationCandidate> result;
    std::vector<bool> selected(candidates.size(), false);
    std::vector<size_t> selectedIndices;

    // 贪心选择
    while (result.size() < static_cast<size_t>(topK) && result.size() < candidates.size()) {
        double bestScore = -std::numeric_limits<double>::max();
        size_t bestIdx = 0;

        for (size_t i = 0; i < candidates.size(); ++i) {
            if (selected[i]) continue;

            double relevance = candidates[i].score;
            double maxSim = 0.0;

            // 使用预计算的embedding
            for (size_t selIdx : selectedIndices) {
                double sim = AdvancedEmbeddingEngine::cosineSimilarity(embeddings[i], embeddings[selIdx]);
                maxSim = std::max(maxSim, sim);
            }

            double mmrScore = lambda * relevance - (1.0 - lambda) * maxSim;
            if (mmrScore > bestScore) {
                bestScore = mmrScore;
                bestIdx = i;
            }
        }

        selected[bestIdx] = true;
        selectedIndices.push_back(bestIdx);
        result.push_back(candidates[bestIdx]);
    }

    return result;
}

double RecommendationEngine::graphPropagationScore(
    const std::string& userId,
    const std::string& itemId,
    const std::unordered_map<std::string, std::vector<std::string>>& userItemGraph
) {
    auto userIt = userItemGraph.find(userId);
    if (userIt == userItemGraph.end()) return 0.0;

    // 转换为set以实现O(1)查找
    std::unordered_set<std::string> userItems(userIt->second.begin(), userIt->second.end());

    double score = 0.0;
    int pathCount = 0;

    for (const auto& [otherUser, otherItems] : userItemGraph) {
        if (otherUser == userId) continue;

        std::unordered_set<std::string> otherSet(otherItems.begin(), otherItems.end());
        if (otherSet.find(itemId) == otherSet.end()) continue;

        for (const auto& item : userItems) {
            if (otherSet.find(item) != otherSet.end()) {
                score += 1.0;
                pathCount++;
                break;
            }
        }
    }

    return pathCount > 0 ? score / pathCount : 0.0;
}

UserProfile& RecommendationEngine::getOrCreateProfile(const std::string& userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = userProfiles_.find(userId);
    if (it != userProfiles_.end()) return it->second;

    // 创建新用户画像
    UserProfile profile;
    profile.userId = userId;
    profile.latentFactors.resize(latentDim_);

    // 随机初始化潜在因子
    std::normal_distribution<float> dist(0.0f, 0.1f);
    for (auto& f : profile.latentFactors) {
        f = dist(rng_);
    }
    profile.explorationRate = 0.2;
    profile.totalInteractions = 0;

    userProfiles_[userId] = profile;
    return userProfiles_[userId];
}

void RecommendationEngine::updateLatentFactors(
    std::vector<float>& userFactors,
    std::vector<float>& itemFactors,
    double error,
    double learningRate,
    double regularization
) {
    for (size_t i = 0; i < userFactors.size(); ++i) {
        float userGrad = error * itemFactors[i] - regularization * userFactors[i];
        float itemGrad = error * userFactors[i] - regularization * itemFactors[i];
        userFactors[i] += learningRate * userGrad;
        itemFactors[i] += learningRate * itemGrad;
    }
}

void RecommendationEngine::recordInteraction(
    const std::string& userId,
    const std::string& itemId,
    const std::string& /*interactionType*/,
    double /*reward*/
) {
    auto& profile = getOrCreateProfile(userId);
    profile.totalInteractions++;
    profile.interactionCounts[itemId]++;

    // 衰减探索率
    profile.explorationRate = std::max(0.05, profile.explorationRate * 0.99);
}

double RecommendationEngine::computeUserSimilarity(
    const std::string& userId1,
    const std::string& userId2
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it1 = userProfiles_.find(userId1);
    auto it2 = userProfiles_.find(userId2);
    if (it1 == userProfiles_.end() || it2 == userProfiles_.end()) return 0.0;

    return AdvancedEmbeddingEngine::cosineSimilarity(
        it1->second.latentFactors,
        it2->second.latentFactors
    );
}

double RecommendationEngine::computeItemSimilarity(
    const std::string& itemId1,
    const std::string& itemId2
) {
    // 基于共现计算物品相似度
    std::lock_guard<std::mutex> lock(mutex_);
    int cooccur = 0, count1 = 0, count2 = 0;

    for (const auto& [userId, profile] : userProfiles_) {
        bool has1 = profile.interactionCounts.count(itemId1) > 0;
        bool has2 = profile.interactionCounts.count(itemId2) > 0;
        if (has1) count1++;
        if (has2) count2++;
        if (has1 && has2) cooccur++;
    }

    if (count1 == 0 || count2 == 0) return 0.0;
    return cooccur / std::sqrt(count1 * count2); // Cosine-like
}

void RecommendationEngine::getRecommendations(
    const std::string& userId,
    const std::string& /*itemType*/,
    int limit,
    std::function<void(const std::vector<RecommendationCandidate>& results, const std::string& error)> callback
) {
    try {
        // 使用混合推荐策略: 协同过滤40% + 内容推荐40% + 探索20%
        auto results = hybridRecommend(userId, limit, 0.4, 0.4, 0.2);
        callback(results, "");
    } catch (const std::exception& e) {
        LOG_ERROR << "RecommendationEngine error: " << e.what();
        callback({}, e.what());
    }
}

// ===== 协同过滤算法实现 =====

std::vector<RecommendationCandidate> RecommendationEngine::userBasedCF(
    const std::string& userId, int topK) {
    std::vector<RecommendationCandidate> results;
    auto dbClient = app().getDbClient("default");

    // 找相似用户喜欢但当前用户未交互的物品
    auto rows = dbClient->execSqlSync(
        "SELECT s.stone_id, s.content, s.mood_type, u.nickname, "
        "AVG(us.similarity_score) as avg_sim "
        "FROM stones s "
        "JOIN user_interaction_history uih ON s.stone_id = uih.stone_id "
        "JOIN user_similarity us ON uih.user_id = us.user2_id AND us.user1_id = $1"
        "JOIN users u ON s.user_id = u.user_id "
        "WHERE us.similarity_score > 0.5 AND s.status = 'published' "
        "AND s.stone_id NOT IN (SELECT stone_id FROM user_interaction_history WHERE user_id = $1) "
        "GROUP BY s.stone_id, s.content, s.mood_type, u.nickname "
        "ORDER BY avg_sim DESC LIMIT $2",
        userId, topK
    );

    for (const auto& row : rows) {
        RecommendationCandidate cand;
        cand.itemId = row["stone_id"].as<std::string>();
        cand.itemType = "stone";
        cand.score = row["avg_sim"].isNull() ? 0.5 : row["avg_sim"].as<double>();
        cand.reason = "和你有相似感受的人也喜欢";
        cand.algorithm = "user_cf";
        cand.metadata["content"] = row["content"].as<std::string>();
        cand.metadata["mood_type"] = row["mood_type"].as<std::string>();
        cand.metadata["author_name"] = row["nickname"].as<std::string>();
        results.push_back(cand);
    }
    return results;
}

std::vector<RecommendationCandidate> RecommendationEngine::itemBasedCF(
    const std::string& userId, int topK) {
    std::vector<RecommendationCandidate> results;
    auto dbClient = app().getDbClient("default");

    // 基于用户历史交互物品找相似物品
    auto rows = dbClient->execSqlSync(
        "WITH user_items AS ("
        "  SELECT stone_id FROM user_interaction_history "
        "  WHERE user_id = $1 AND interaction_weight >= 1.0 "
        "  ORDER BY created_at DESC LIMIT 10"
        ") "
        "SELECT s.stone_id, s.content, s.mood_type, u.nickname, "
        "COUNT(*) as co_occur "
        "FROM stones s "
        "JOIN users u ON s.user_id = u.user_id "
        "JOIN user_interaction_history uih ON s.stone_id = uih.stone_id "
        "WHERE uih.user_id IN ("
        "  SELECT DISTINCT user_id FROM user_interaction_history "
        "  WHERE stone_id IN (SELECT stone_id FROM user_items)"
        ") "
        "AND s.stone_id NOT IN (SELECT stone_id FROM user_items) "
        "AND s.stone_id NOT IN (SELECT stone_id FROM user_interaction_history WHERE user_id = $1) "
        "AND s.status = 'published' "
        "GROUP BY s.stone_id, s.content, s.mood_type, u.nickname "
        "ORDER BY co_occur DESC LIMIT $2",
        userId, topK
    );

    for (const auto& row : rows) {
        RecommendationCandidate cand;
        cand.itemId = row["stone_id"].as<std::string>();
        cand.itemType = "stone";
        cand.score = std::min(1.0, row["co_occur"].as<int>() / 10.0);
        cand.reason = "喜欢类似内容的人也看过";
        cand.algorithm = "item_cf";
        cand.metadata["content"] = row["content"].as<std::string>();
        cand.metadata["mood_type"] = row["mood_type"].as<std::string>();
        cand.metadata["author_name"] = row["nickname"].as<std::string>();
        results.push_back(cand);
    }
    return results;
}

std::vector<RecommendationCandidate> RecommendationEngine::contentBasedRecommend(
    const std::string& userId, const std::string& userMood, int topK) {
    std::vector<RecommendationCandidate> results;
    auto dbClient = app().getDbClient("default");

    // 基于情绪向量的内容推荐
    auto rows = dbClient->execSqlSync(
        "SELECT s.stone_id, s.content, s.mood_type, s.emotion_score, u.nickname "
        "FROM stones s "
        "JOIN users u ON s.user_id = u.user_id "
        "LEFT JOIN emotion_compatibility ec ON "
        "  (ec.mood_type_1 = $2 AND ec.mood_type_2 = s.mood_type) OR "
        "  (ec.mood_type_2 = $2 AND ec.mood_type_1 = s.mood_type) "
        "WHERE s.user_id != $1 AND s.status = 'published' "
        "AND s.stone_id NOT IN (SELECT stone_id FROM user_interaction_history WHERE user_id = $1) "
        "ORDER BY COALESCE(ec.compatibility_score, 0.5) DESC, s.created_at DESC "
        "LIMIT $3",
        userId, userMood, topK
    );

    for (const auto& row : rows) {
        RecommendationCandidate cand;
        cand.itemId = row["stone_id"].as<std::string>();
        cand.itemType = "stone";
        std::string itemMood = row["mood_type"].isNull() ? "neutral" : row["mood_type"].as<std::string>();
        cand.score = emotionCompatibilityScore(userMood, itemMood);
        cand.reason = "与你当前心情契合";
        cand.algorithm = "content_emotion";
        cand.metadata["content"] = row["content"].as<std::string>();
        cand.metadata["mood_type"] = itemMood;
        cand.metadata["author_name"] = row["nickname"].as<std::string>();
        results.push_back(cand);
    }
    return results;
}

std::vector<RecommendationCandidate> RecommendationEngine::hybridRecommend(
    const std::string& userId, int topK,
    double cfWeight, double contentWeight, double exploreWeight) {

    auto dbClient = app().getDbClient("default");
    std::string userMood = "neutral";
    auto moodResult = dbClient->execSqlSync(
        "SELECT mood_type FROM user_emotion_profile WHERE user_id = $1 "
        "ORDER BY date DESC LIMIT 1", userId);
    if (!moodResult.empty()) {
        userMood = moodResult[0]["mood_type"].as<std::string>();
    }

    // 各算法获取候选
    int cfCount = static_cast<int>(topK * cfWeight * 2);
    int contentCount = static_cast<int>(topK * contentWeight * 2);

    auto userCFResults = userBasedCF(userId, cfCount / 2);
    auto itemCFResults = itemBasedCF(userId, cfCount / 2);
    auto contentResults = contentBasedRecommend(userId, userMood, contentCount);

    // 合并并加权
    std::unordered_map<std::string, RecommendationCandidate> merged;

    for (auto& c : userCFResults) {
        c.score *= cfWeight;
        merged[c.itemId] = c;
    }
    for (auto& c : itemCFResults) {
        c.score *= cfWeight;
        if (merged.count(c.itemId)) merged[c.itemId].score += c.score;
        else merged[c.itemId] = c;
    }
    for (auto& c : contentResults) {
        c.score *= contentWeight;
        if (merged.count(c.itemId)) merged[c.itemId].score += c.score;
        else merged[c.itemId] = c;
    }

    // 转为vector并排序
    std::vector<RecommendationCandidate> results;
    results.reserve(merged.size());
    for (auto& [id, cand] : merged) {
        results.push_back(std::move(cand));
    }
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.score > b.score; });

    // 添加探索项
    int exploreCount = static_cast<int>(topK * exploreWeight);
    if (exploreCount > 0) {
        auto exploreRows = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, s.mood_type, u.nickname "
            "FROM stones s JOIN users u ON s.user_id = u.user_id "
            "WHERE s.user_id != $1 AND s.status = 'published' "
            "AND s.stone_id NOT IN (SELECT stone_id FROM user_interaction_history WHERE user_id = $1) "
            "ORDER BY RANDOM() LIMIT $2",
            userId, exploreCount);
        for (const auto& row : exploreRows) {
            RecommendationCandidate cand;
            cand.itemId = row["stone_id"].as<std::string>();
            cand.itemType = "stone";
            cand.score = exploreWeight * thompsonSample(1, 1);
            cand.reason = "为你发现的新内容";
            cand.algorithm = "exploration";
            cand.metadata["content"] = row["content"].as<std::string>();
            cand.metadata["mood_type"] = row["mood_type"].as<std::string>();
            cand.metadata["author_name"] = row["nickname"].as<std::string>();
            results.push_back(cand);
        }
    }

    // MMR重排序增加多样性
    return mmrRerank(results, 0.7, topK);
}

} // namespace ai
} // namespace heartlake
