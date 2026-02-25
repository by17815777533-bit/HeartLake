/**
 * @file RecommendationEngine.h
 * @brief 高级推荐引擎 - 多算法融合推荐系统
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <random>
#include <memory>
#include <json/json.h>

// 前向声明，避免头文件依赖 drogon
namespace drogon { namespace orm { class DbClient; } }

namespace heartlake {
namespace ai {

/**
 * 推荐候选项
 */
struct RecommendationCandidate {
    std::string itemId;
    std::string itemType;  // "stone", "user", "boat"
    double score;
    std::string reason;
    std::string algorithm;  // 来源算法
    Json::Value metadata;
};

/**
 * 用户画像
 */
struct UserProfile {
    std::string userId;
    std::vector<float> latentFactors;      // 潜在因子向量
    std::unordered_map<std::string, double> moodPreferences;  // 情绪偏好
    std::unordered_map<std::string, int> interactionCounts;   // 交互计数
    double explorationRate;                 // 探索率 (UCB)
    int totalInteractions;
};

/**
 * 高级推荐引擎
 * 融合多种算法：矩阵分解、图排序、多臂老虎机、时间衰减
 */
class RecommendationEngine {
public:
    static RecommendationEngine& getInstance();

    void initialize(int latentDim = 32);

    /**
     * 设置数据库客户端提供者（依赖注入）
     * 生产环境中注入 [](){ return app().getDbClient("default"); }
     * 不设置则 DB 相关方法返回空结果
     */
    using DbClientPtr = std::shared_ptr<drogon::orm::DbClient>;
    using DbClientProvider = std::function<DbClientPtr()>;
    void setDbClientProvider(DbClientProvider provider);

    /**
     * 获取个性化推荐
     * @param userId 用户ID
     * @param itemType 推荐类型
     * @param limit 数量限制
     * @param callback 回调函数
     */
    void getRecommendations(
        const std::string& userId,
        const std::string& itemType,
        int limit,
        std::function<void(const std::vector<RecommendationCandidate>& results, const std::string& error)> callback
    );

    /**
     * 更新用户交互（用于在线学习）
     */
    void recordInteraction(
        const std::string& userId,
        const std::string& itemId,
        const std::string& interactionType,
        double reward
    );

    /**
     * 计算两个用户的相似度（基于潜在因子）
     */
    double computeUserSimilarity(const std::string& userId1, const std::string& userId2);

    /**
     * 计算物品相似度（基于共现矩阵）
     */
    double computeItemSimilarity(const std::string& itemId1, const std::string& itemId2);

    /**
     * 用户-用户协同过滤推荐
     */
    std::vector<RecommendationCandidate> userBasedCF(
        const std::string& userId, int topK);

    /**
     * 物品-物品协同过滤推荐
     */
    std::vector<RecommendationCandidate> itemBasedCF(
        const std::string& userId, int topK);

    /**
     * 基于内容的推荐（情绪向量）
     */
    std::vector<RecommendationCandidate> contentBasedRecommend(
        const std::string& userId, const std::string& userMood, int topK);

    /**
     * 混合推荐策略
     */
    std::vector<RecommendationCandidate> hybridRecommend(
        const std::string& userId, int topK,
        double cfWeight = 0.4, double contentWeight = 0.4, double exploreWeight = 0.2);

private:
    RecommendationEngine() = default;
    ~RecommendationEngine() = default;
    RecommendationEngine(const RecommendationEngine&) = delete;
    RecommendationEngine& operator=(const RecommendationEngine&) = delete;

    bool initialized_ = false;
    int latentDim_ = 32;
    mutable std::recursive_mutex mutex_;
    std::mt19937 rng_{std::random_device{}()};
    DbClientProvider dbClientProvider_;  // 依赖注入的 DB 客户端提供者

    // 用户潜在因子缓存
    std::unordered_map<std::string, UserProfile> userProfiles_;

    // UCB参数
    double ucbC_ = 2.0;  // 探索系数

    // ===== 核心算法 =====

    /**
     * 矩阵分解预测分数 (ALS风格)
     */
    double predictMF(const std::vector<float>& userFactors, const std::vector<float>& itemFactors);

    /**
     * UCB (Upper Confidence Bound) 探索-利用平衡
     */
    double computeUCB(double avgReward, int itemCount, int totalCount);

    /**
     * Thompson Sampling 采样
     */
    double thompsonSample(int successes, int failures);

    /**
     * 时间衰减权重
     */
    double timeDecay(int64_t timestampMs, double halfLifeHours = 24.0);

    /**
     * 多样性感知重排序 (MMR - Maximal Marginal Relevance)
     */
    std::vector<RecommendationCandidate> mmrRerank(
        const std::vector<RecommendationCandidate>& candidates,
        double lambda,
        int topK
    );

    /**
     * 图传播分数 (简化PageRank)
     */
    double graphPropagationScore(
        const std::string& userId,
        const std::string& itemId,
        const std::unordered_map<std::string, std::vector<std::string>>& userItemGraph
    );

    /**
     * 情绪兼容性分数
     */
    double emotionCompatibilityScore(const std::string& userMood, const std::string& itemMood);

    /**
     * 加载/更新用户画像
     */
    UserProfile& getOrCreateProfile(const std::string& userId);

    /**
     * 在线更新潜在因子 (SGD)
     */
    void updateLatentFactors(
        std::vector<float>& userFactors,
        std::vector<float>& itemFactors,
        double error,
        double learningRate = 0.01,
        double regularization = 0.1
    );
};

} // namespace ai
} // namespace heartlake
