/**
 * @file RecommendationEngine.cpp
 * @brief 多算法融合推荐引擎 —— 协同过滤 + 内容推荐 + 探索的混合推荐系统
 *
 * 算法组成：
 *   - User-based CF：基于用户相似度矩阵找相似用户喜欢的内容
 *   - Item-based CF：基于用户历史交互物品的共现关系推荐
 *   - Content-based：基于情绪兼容性矩阵的内容推荐
 *   - Exploration：Thompson Sampling 探索未知内容，平衡 exploit/explore
 *
 * 后处理：
 *   - 时效衰减（72h 半衰期）+ 热门抑制（log 阻尼）
 *   - MMR 重排序：贪心选择兼顾相关性和多样性，附带作者去重惩罚
 *
 * 用户画像：
 *   - 潜在因子向量（MF 风格）+ 交互计数 + 探索率衰减
 *   - 用户相似度 = 0.7 * 加权 Jaccard 交互重叠 + 0.3 * latent cosine
 */

#include "infrastructure/ai/RecommendationEngine.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include <drogon/orm/DbClient.h>
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <ctime>

namespace heartlake {
namespace ai {

namespace {

using DbClientPtr = RecommendationEngine::DbClientPtr;

RecommendationCandidate buildStoneCandidate(
    const drogon::orm::Row& row,
    double score,
    const char* reason,
    const char* algorithm
) {
    RecommendationCandidate cand;
    cand.itemId = row["stone_id"].as<std::string>();
    cand.itemType = "stone";
    cand.score = score;
    cand.reason = reason;
    cand.algorithm = algorithm;
    cand.metadata["content"] = row["content"].as<std::string>();
    cand.metadata["mood_type"] = row["mood_type"].isNull()
        ? "neutral"
        : row["mood_type"].as<std::string>();
    cand.metadata["emotion_score"] = row["emotion_score"].isNull()
        ? 0.0
        : row["emotion_score"].as<double>();
    cand.metadata["author_name"] = row["nickname"].isNull()
        ? ""
        : row["nickname"].as<std::string>();
    cand.metadata["author_id"] = row["author_id"].isNull()
        ? ""
        : row["author_id"].as<std::string>();
    cand.metadata["ripple_count"] = row["ripple_count"].isNull()
        ? 0
        : row["ripple_count"].as<int>();
    cand.metadata["created_at"] = row["created_at"].isNull()
        ? ""
        : row["created_at"].as<std::string>();
    cand.metadata["hours_old"] = row["hours_old"].isNull()
        ? 72.0
        : row["hours_old"].as<double>();
    return cand;
}

std::vector<RecommendationCandidate> queryUserBasedCFCandidates(
    const DbClientPtr& dbClient,
    const std::string& userId,
    int topK
) {
    auto rows = dbClient->execSqlSync(
        "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
        "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
        "u.nickname, u.user_id AS author_id, "
        "COALESCE(s.ripple_count, 0) AS ripple_count, "
        "s.created_at, "
        "EXTRACT(EPOCH FROM (NOW() - s.created_at)) / 3600.0 AS hours_old, "
        "AVG(us.similarity_score) as avg_sim "
        "FROM stones s "
        "JOIN user_interaction_history uih ON s.stone_id = uih.stone_id "
        "JOIN user_similarity us ON uih.user_id = us.user2_id AND us.user1_id = $1 "
        "JOIN users u ON s.user_id = u.user_id "
        "WHERE us.similarity_score > 0.5 AND s.status = 'published' AND s.deleted_at IS NULL "
        "AND NOT EXISTS (SELECT 1 FROM user_interaction_history h WHERE h.stone_id = s.stone_id AND h.user_id = $1) "
        "GROUP BY s.stone_id, s.content, s.mood_type, s.emotion_score, "
        "u.nickname, u.user_id, s.ripple_count, s.created_at "
        "ORDER BY avg_sim DESC, s.created_at DESC LIMIT $2",
        userId, static_cast<int64_t>(topK)
    );

    std::vector<RecommendationCandidate> results;
    results.reserve(rows.size());
    for (const auto& row : rows) {
        const double score = row["avg_sim"].isNull() ? 0.5 : row["avg_sim"].as<double>();
        results.push_back(buildStoneCandidate(
            row,
            score,
            "和你有相似感受的人也喜欢",
            "user_cf"));
    }
    return results;
}

std::vector<RecommendationCandidate> queryItemBasedCFCandidates(
    const DbClientPtr& dbClient,
    const std::string& userId,
    int topK
) {
    auto rows = dbClient->execSqlSync(
        "WITH user_items AS ("
        "  SELECT stone_id FROM user_interaction_history "
        "  WHERE user_id = $1 AND interaction_weight >= 1.0 "
        "  ORDER BY created_at DESC LIMIT 10"
        ") "
        "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
        "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
        "u.nickname, u.user_id AS author_id, "
        "COALESCE(s.ripple_count, 0) AS ripple_count, "
        "s.created_at, "
        "EXTRACT(EPOCH FROM (NOW() - s.created_at)) / 3600.0 AS hours_old, "
        "COUNT(*) as co_occur "
        "FROM stones s "
        "JOIN users u ON s.user_id = u.user_id "
        "JOIN user_interaction_history uih ON s.stone_id = uih.stone_id "
        "WHERE uih.user_id IN ("
        "  SELECT DISTINCT user_id FROM user_interaction_history "
        "  WHERE stone_id IN (SELECT stone_id FROM user_items)"
        ") "
        "AND NOT EXISTS (SELECT 1 FROM user_items ui WHERE ui.stone_id = s.stone_id) "
        "AND NOT EXISTS (SELECT 1 FROM user_interaction_history h WHERE h.stone_id = s.stone_id AND h.user_id = $1) "
        "AND s.status = 'published' AND s.deleted_at IS NULL "
        "GROUP BY s.stone_id, s.content, s.mood_type, s.emotion_score, "
        "u.nickname, u.user_id, s.ripple_count, s.created_at "
        "ORDER BY co_occur DESC, s.created_at DESC LIMIT $2",
        userId, static_cast<int64_t>(topK)
    );

    std::vector<RecommendationCandidate> results;
    results.reserve(rows.size());
    for (const auto& row : rows) {
        const double score = std::min(1.0, row["co_occur"].as<int>() / 10.0);
        results.push_back(buildStoneCandidate(
            row,
            score,
            "喜欢类似内容的人也看过",
            "item_cf"));
    }
    return results;
}

std::vector<RecommendationCandidate> queryContentBasedCandidates(
    const DbClientPtr& dbClient,
    const std::string& userId,
    const std::string& userMood,
    const std::function<double(const std::string&, const std::string&)>& scoreResolver,
    int topK
) {
    auto rows = dbClient->execSqlSync(
        "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
        "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
        "u.nickname, u.user_id AS author_id, "
        "COALESCE(s.ripple_count, 0) AS ripple_count, "
        "s.created_at, "
        "EXTRACT(EPOCH FROM (NOW() - s.created_at)) / 3600.0 AS hours_old "
        "FROM stones s "
        "JOIN users u ON s.user_id = u.user_id "
        "LEFT JOIN emotion_compatibility ec ON "
        "  (ec.mood_type_1 = $2 AND ec.mood_type_2 = s.mood_type) OR "
        "  (ec.mood_type_2 = $2 AND ec.mood_type_1 = s.mood_type) "
        "WHERE s.user_id != $1 AND s.status = 'published' AND s.deleted_at IS NULL "
        "AND NOT EXISTS (SELECT 1 FROM user_interaction_history h WHERE h.stone_id = s.stone_id AND h.user_id = $1) "
        "ORDER BY COALESCE(ec.compatibility_score, 0.5) DESC, s.created_at DESC "
        "LIMIT $3",
        userId, userMood, static_cast<int64_t>(topK)
    );

    std::vector<RecommendationCandidate> results;
    results.reserve(rows.size());
    for (const auto& row : rows) {
        const std::string itemMood = row["mood_type"].isNull()
            ? "neutral"
            : row["mood_type"].as<std::string>();
        results.push_back(buildStoneCandidate(
            row,
            scoreResolver(userMood, itemMood),
            "与你当前心情契合",
            "content_emotion"));
    }
    return results;
}

}  // namespace

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

void RecommendationEngine::setDbClientProvider(DbClientProvider provider) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    dbClientProvider_ = std::move(provider);
}

// ===== 核心算法实现 =====

/// Matrix Factorization 预测：用户/物品潜在因子点积 → Sigmoid 归一化到 [0,1]
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

/// UCB1 算法：exploitation(平均奖励) + exploration(置信上界)，未探索项优先
double RecommendationEngine::computeUCB(double avgReward, int itemCount, int totalCount) {
    if (itemCount == 0) return std::numeric_limits<double>::max(); // 未探索优先
    double exploitation = avgReward;
    double exploration = ucbC_ * std::sqrt(std::log(totalCount + 1) / itemCount);
    return exploitation + exploration;
}

/// Thompson Sampling：Beta(α,β) 采样，用 Gamma 分布近似实现
double RecommendationEngine::thompsonSample(int successes, int failures) {
    // Beta分布采样 (使用Gamma分布近似)
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::gamma_distribution<double> gamma_a(successes + 1, 1.0);
    std::gamma_distribution<double> gamma_b(failures + 1, 1.0);
    double x = gamma_a(rng_);
    double y = gamma_b(rng_);
    double sum = x + y;
    if (sum < 1e-15) return 0.5;
    return x / sum;
}

/// 时间衰减函数：指数衰减，halfLifeHours 控制半衰期（默认 24h）
double RecommendationEngine::timeDecay(int64_t timestampMs, double halfLifeHours) {
    if (halfLifeHours <= 0.0) halfLifeHours = 24.0;  // 默认24小时
    auto now = std::chrono::system_clock::now().time_since_epoch();
    int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    double hoursOld = (nowMs - timestampMs) / (1000.0 * 3600.0);
    return std::exp(-0.693 * hoursOld / halfLifeHours); // ln(2) ≈ 0.693
}

/// 情绪兼容性评分：基于心理学情绪共鸣模型的静态矩阵查表
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

/**
 * MMR (Maximal Marginal Relevance) 重排序：
 *   贪心选择 score = λ * relevance - (1-λ) * maxSimilarity - authorPenalty
 *   embedding 已 L2 归一化，cosine similarity 直接用点积计算。
 *   作者去重惩罚避免同一用户的内容垄断推荐列表。
 */
std::vector<RecommendationCandidate> RecommendationEngine::mmrRerank(
    std::vector<RecommendationCandidate> pool,
    double lambda,
    int topK
) {
    if (pool.empty()) return {};
    if (topK <= 0) return {};
    const size_t preselectLimit = static_cast<size_t>(std::max(topK * 4, topK));
    if (pool.size() > preselectLimit) {
        std::partial_sort(
            pool.begin(),
            pool.begin() + preselectLimit,
            pool.end(),
            [](const RecommendationCandidate& a, const RecommendationCandidate& b) {
                return a.score > b.score;
            });
        pool.resize(preselectLimit);
    }

    auto& embEngine = AdvancedEmbeddingEngine::getInstance();

    // 预计算并去重候选 embedding，避免同一 stone / content 在同轮重排里反复编码
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(pool.size());
    std::unordered_map<std::string, size_t> embeddingIndexByKey;
    std::vector<size_t> candidateEmbeddingIndices;
    candidateEmbeddingIndices.reserve(pool.size());
    for (const auto& cand : pool) {
        const std::string content = cand.metadata.get("content", "").asString();
        const std::string cacheKey = !cand.itemId.empty() ? cand.itemId : content;
        auto it = embeddingIndexByKey.find(cacheKey);
        if (it == embeddingIndexByKey.end()) {
            const size_t embeddingIndex = embeddings.size();
            embeddings.push_back(
                content.empty() ? std::vector<float>{} : embEngine.generateEmbedding(content));
            embeddingIndexByKey.emplace(cacheKey, embeddingIndex);
            candidateEmbeddingIndices.push_back(embeddingIndex);
        } else {
            candidateEmbeddingIndices.push_back(it->second);
        }
    }

    std::vector<RecommendationCandidate> result;
    std::vector<bool> selected(pool.size(), false);
    std::vector<size_t> selectedIndices;
    std::unordered_map<std::string, int> selectedAuthorCounts;

    // 贪心选择
    while (result.size() < static_cast<size_t>(topK) && result.size() < pool.size()) {
        double bestScore = -std::numeric_limits<double>::max();
        size_t bestIdx = 0;

        for (size_t i = 0; i < pool.size(); ++i) {
            if (selected[i]) continue;

            double relevance = pool[i].score;
            double maxSim = 0.0;

            // embeddings 已 L2 归一化，cosine similarity = dot product
            // 直接点积省去两次 norm 计算
            for (size_t selIdx : selectedIndices) {
                const auto& a = embeddings[candidateEmbeddingIndices[i]];
                const auto& b = embeddings[candidateEmbeddingIndices[selIdx]];
                double dot = 0.0;
                size_t dim = std::min(a.size(), b.size());
                size_t d = 0;
                for (; d + 3 < dim; d += 4) {
                    dot += static_cast<double>(a[d])     * b[d]
                         + static_cast<double>(a[d + 1]) * b[d + 1]
                         + static_cast<double>(a[d + 2]) * b[d + 2]
                         + static_cast<double>(a[d + 3]) * b[d + 3];
                }
                for (; d < dim; ++d) {
                    dot += static_cast<double>(a[d]) * b[d];
                }
                maxSim = std::max(maxSim, dot);
            }

            const std::string authorId = pool[i].metadata.get("author_id", "").asString();
            const int repeatedAuthor = authorId.empty() ? 0 : selectedAuthorCounts[authorId];
            const double authorPenalty = std::min(0.12, repeatedAuthor * 0.06);

            double mmrScore = lambda * relevance - (1.0 - lambda) * maxSim - authorPenalty;
            if (mmrScore > bestScore) {
                bestScore = mmrScore;
                bestIdx = i;
            }
        }

        selected[bestIdx] = true;
        selectedIndices.push_back(bestIdx);
        const std::string authorId = pool[bestIdx].metadata.get("author_id", "").asString();
        if (!authorId.empty()) {
            selectedAuthorCounts[authorId]++;
        }
        result.push_back(pool[bestIdx]);
    }

    return result;
}

/// 图传播评分：通过 user-item 二部图的两跳路径计算关联强度
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

        if (std::find(otherItems.begin(), otherItems.end(), itemId) == otherItems.end()) continue;

        for (const auto& item : userItems) {
            if (std::find(otherItems.begin(), otherItems.end(), item) != otherItems.end()) {
                score += 1.0;
                pathCount++;
                break;
            }
        }
    }

    return pathCount > 0 ? score / pathCount : 0.0;
}

/// 获取或创建用户画像：潜在因子用正态分布随机初始化，初始探索率 0.2
UserProfile& RecommendationEngine::getOrCreateProfile(const std::string& userId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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

/// SGD 更新潜在因子：error * 交叉梯度 - L2 正则化
void RecommendationEngine::updateLatentFactors(
    std::vector<float>& userFactors,
    std::vector<float>& itemFactors,
    double error,
    double learningRate,
    double regularization
) {
    for (size_t i = 0; i < userFactors.size(); ++i) {
        float userGrad = static_cast<float>(error * itemFactors[i] - regularization * userFactors[i]);
        float itemGrad = static_cast<float>(error * userFactors[i] - regularization * itemFactors[i]);
        userFactors[i] += static_cast<float>(learningRate * userGrad);
        itemFactors[i] += static_cast<float>(learningRate * itemGrad);
    }
}

/// 记录用户交互并衰减探索率（每次交互 × 0.99，下限 0.05）
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

/// 用户相似度 = 0.7 * 加权 Jaccard 交互重叠 + 0.3 * latent cosine
double RecommendationEngine::computeUserSimilarity(
    const std::string& userId1,
    const std::string& userId2
) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it1 = userProfiles_.find(userId1);
    auto it2 = userProfiles_.find(userId2);
    if (it1 == userProfiles_.end() || it2 == userProfiles_.end()) return 0.0;
    if (userId1 == userId2) return 1.0;

    const auto& p1 = it1->second;
    const auto& p2 = it2->second;

    // latent 相似度归一到 [0,1]
    const double latentRaw = AdvancedEmbeddingEngine::cosineSimilarity(
        p1.latentFactors, p2.latentFactors);
    const double latentSim = std::clamp((latentRaw + 1.0) * 0.5, 0.0, 1.0);

    // 交互重叠（加权 Jaccard）：更贴近推荐业务语义，减少随机向量抖动
    double sumMin = 0.0;
    double sumMax = 0.0;
    std::unordered_set<std::string> allItems;
    allItems.reserve(p1.interactionCounts.size() + p2.interactionCounts.size());
    for (const auto& [item, _] : p1.interactionCounts) allItems.insert(item);
    for (const auto& [item, _] : p2.interactionCounts) allItems.insert(item);

    for (const auto& item : allItems) {
        const auto itCount1 = p1.interactionCounts.find(item);
        const auto itCount2 = p2.interactionCounts.find(item);
        const double c1 = (itCount1 == p1.interactionCounts.end())
            ? 0.0
            : static_cast<double>(itCount1->second);
        const double c2 = (itCount2 == p2.interactionCounts.end())
            ? 0.0
            : static_cast<double>(itCount2->second);
        sumMin += std::min(c1, c2);
        sumMax += std::max(c1, c2);
    }

    if (sumMax <= 1e-12) {
        return latentSim;
    }
    const double interactionSim = sumMin / sumMax;

    // 交互重叠优先，latent 作为兜底（避免完全冷启动时不稳定）
    return std::clamp(0.7 * interactionSim + 0.3 * latentSim, 0.0, 1.0);
}

/// 物品相似度：基于用户共现的 Cosine-like 计算
double RecommendationEngine::computeItemSimilarity(
    const std::string& itemId1,
    const std::string& itemId2
) {
    // 基于共现计算物品相似度
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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

/// User-based CF：找相似用户喜欢但当前用户未交互的石头
std::vector<RecommendationCandidate> RecommendationEngine::userBasedCF(
    const std::string& userId, int topK) {
    if (!dbClientProvider_) { LOG_ERROR << "userBasedCF: no DB provider"; return {}; }
    auto dbClient = dbClientProvider_();
    if (!dbClient) {
        LOG_ERROR << "userBasedCF: failed to get db client";
        return {};
    }
    return queryUserBasedCFCandidates(dbClient, userId, topK);
}

/// Item-based CF：基于用户最近交互的 10 个石头，通过共现关系推荐相似内容
std::vector<RecommendationCandidate> RecommendationEngine::itemBasedCF(
    const std::string& userId, int topK) {
    if (!dbClientProvider_) { LOG_ERROR << "itemBasedCF: no DB provider"; return {}; }
    auto dbClient = dbClientProvider_();
    if (!dbClient) {
        LOG_ERROR << "itemBasedCF: failed to get db client";
        return {};
    }
    return queryItemBasedCFCandidates(dbClient, userId, topK);
}

/// 基于情绪兼容性的内容推荐：用户当前心情 → 情绪矩阵 → 匹配石头
std::vector<RecommendationCandidate> RecommendationEngine::contentBasedRecommend(
    const std::string& userId, const std::string& userMood, int topK) {
    if (!dbClientProvider_) { LOG_ERROR << "contentBasedRecommend: no DB provider"; return {}; }
    auto dbClient = dbClientProvider_();
    if (!dbClient) {
        LOG_ERROR << "contentBasedRecommend: failed to get db client";
        return {};
    }
    return queryContentBasedCandidates(
        dbClient,
        userId,
        userMood,
        [this](const std::string& moodA, const std::string& moodB) {
            return emotionCompatibilityScore(moodA, moodB);
        },
        topK);
}

/**
 * 混合推荐主入口：
 *   1. 获取用户最近情绪画像
 *   2. 并行调用 userCF / itemCF / contentBased 三路候选
 *   3. 加权合并 → 时效衰减 + 热门抑制后处理
 *   4. Thompson Sampling 探索项补充
 *   5. MMR 重排序输出最终 topK
 */
std::vector<RecommendationCandidate> RecommendationEngine::hybridRecommend(
    const std::string& userId, int topK,
    double cfWeight, double contentWeight, double exploreWeight) {

    topK = std::max(1, topK);
    if (!dbClientProvider_) { LOG_ERROR << "hybridRecommend: no DB provider"; return {}; }
    auto dbClient = dbClientProvider_();
    if (!dbClient) {
        LOG_ERROR << "hybridRecommend: failed to get db client";
        return {};
    }
    std::string userMood = "neutral";
    auto moodResult = dbClient->execSqlSync(
        "SELECT mood_type FROM user_emotion_profile WHERE user_id = $1 "
        "ORDER BY date DESC LIMIT 1", userId);
    if (!moodResult.empty()) {
        userMood = moodResult[0]["mood_type"].as<std::string>();
    }

    // 各算法获取候选（单个失败不影响整体）
    int cfCount = std::max(2, static_cast<int>(topK * cfWeight * 2));
    int contentCount = std::max(2, static_cast<int>(topK * contentWeight * 2));

    std::vector<RecommendationCandidate> userCFResults, itemCFResults, contentResults;
    try { userCFResults = queryUserBasedCFCandidates(dbClient, userId, cfCount / 2); }
    catch (const std::exception& e) { LOG_WARN << "userBasedCF failed: " << e.what(); }
    try { itemCFResults = queryItemBasedCFCandidates(dbClient, userId, cfCount / 2); }
    catch (const std::exception& e) { LOG_WARN << "itemBasedCF failed: " << e.what(); }
    try {
        contentResults = queryContentBasedCandidates(
            dbClient,
            userId,
            userMood,
            [this](const std::string& moodA, const std::string& moodB) {
                return emotionCompatibilityScore(moodA, moodB);
            },
            contentCount);
    }
    catch (const std::exception& e) { LOG_WARN << "contentBasedRecommend failed: " << e.what(); }

    // 合并并加权
    std::unordered_map<std::string, RecommendationCandidate> merged;
    merged.reserve(userCFResults.size() + itemCFResults.size() + contentResults.size());

    auto mergeWeighted = [&merged](std::vector<RecommendationCandidate>& candidates, double weight) {
        for (auto& cand : candidates) {
            cand.score *= weight;
            auto it = merged.find(cand.itemId);
            if (it == merged.end()) {
                merged.emplace(cand.itemId, std::move(cand));
            } else {
                it->second.score += cand.score;
            }
        }
    };

    mergeWeighted(userCFResults, cfWeight);
    mergeWeighted(itemCFResults, cfWeight);
    mergeWeighted(contentResults, contentWeight);

    // 后处理：结合时效与热门抑制，降低“热门枢纽内容”垄断，提升长尾质量。
    auto calibrateCandidateScore = [](RecommendationCandidate& cand) {
        const double hoursOld = std::max(0.0, cand.metadata.get("hours_old", 72.0).asDouble());
        const double recency = std::exp(-0.693 * hoursOld / 72.0);  // 72h半衰
        const double rippleCount = std::max(0.0, cand.metadata.get("ripple_count", 0.0).asDouble());
        const double popularityDamp = 1.0 / (1.0 + 0.08 * std::log1p(rippleCount));

        cand.score *= popularityDamp;
        cand.score = cand.score * 0.88 + recency * 0.12;
    };

    // 转为vector并排序
    std::vector<RecommendationCandidate> results;
    results.reserve(merged.size() + std::max(0, static_cast<int>(topK * exploreWeight)));
    for (auto& [id, cand] : merged) {
        calibrateCandidateScore(cand);
        results.push_back(std::move(cand));
    }
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.score > b.score; });
    if (static_cast<int>(results.size()) > topK) {
        results.resize(topK);
    }

    // 添加探索项
    int exploreCount = static_cast<int>(topK * exploreWeight);
    if (exploreCount > 0) {
        std::unordered_set<std::string> existingIds;
        existingIds.reserve(results.size());
        for (const auto& cand : results) {
            existingIds.insert(cand.itemId);
        }

        const int exploreCandidateWindow = std::max(exploreCount * 8, 80);
        auto exploreRows = dbClient->execSqlSync(
            "SELECT s.stone_id, s.content, COALESCE(s.mood_type, 'neutral') AS mood_type, "
            "COALESCE(s.emotion_score, 0.0) AS emotion_score, "
            "u.nickname, u.user_id AS author_id, "
            "COALESCE(s.ripple_count, 0) AS ripple_count, "
            "s.created_at, "
            "EXTRACT(EPOCH FROM (NOW() - s.created_at)) / 3600.0 AS hours_old "
            "FROM stones s JOIN users u ON s.user_id = u.user_id "
            "WHERE s.user_id != $1 AND s.status = 'published' AND s.deleted_at IS NULL "
            "AND s.created_at >= NOW() - INTERVAL '21 days' "
            "AND NOT EXISTS (SELECT 1 FROM user_interaction_history h WHERE h.stone_id = s.stone_id AND h.user_id = $1) "
            "ORDER BY s.created_at DESC LIMIT $2",
            userId, static_cast<int64_t>(exploreCandidateWindow));

        std::vector<drogon::orm::Row> shuffledRows;
        shuffledRows.reserve(exploreRows.size());
        for (const auto& row : exploreRows) {
            shuffledRows.push_back(row);
        }
        const auto bucket = static_cast<long long>(std::time(nullptr) / 1800);
        std::mt19937 exploreRng(static_cast<uint32_t>(
            std::hash<std::string>{}(userId + ":" + std::to_string(bucket) + ":explore")));
        std::shuffle(shuffledRows.begin(), shuffledRows.end(), exploreRng);

        int addedExplore = 0;
        for (const auto& row : shuffledRows) {
            if (addedExplore >= exploreCount) {
                break;
            }
            const std::string stoneId = row["stone_id"].as<std::string>();
            if (existingIds.find(stoneId) != existingIds.end()) {
                continue;
            }
            const double exploreScore = exploreWeight * thompsonSample(1, 1);
            RecommendationCandidate cand = buildStoneCandidate(
                row,
                exploreScore,
                "为你发现的新内容",
                "exploration");
            calibrateCandidateScore(cand);
            results.push_back(cand);
            existingIds.insert(stoneId);
            ++addedExplore;
        }
    }

    // MMR重排序增加多样性
    return mmrRerank(std::move(results), 0.7, topK);
}

} // namespace ai
} // namespace heartlake
