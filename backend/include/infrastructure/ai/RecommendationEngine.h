/**
 * 多算法融合推荐引擎
 *
 * 融合五种推荐策略，通过 MMR 多样性重排序输出最终结果：
 *   1. 用户协同过滤（User-CF）— 基于潜在因子余弦相似度
 *   2. 物品协同过滤（Item-CF）— 基于共现矩阵
 *   3. 基于内容的推荐 — 情绪向量兼容性匹配
 *   4. UCB 探索-利用平衡 — Upper Confidence Bound 多臂老虎机
 *   5. 图传播评分 — 简化 PageRank 在用户-物品二部图上的传播
 *
 * 在线学习：
 *   - 用户交互实时更新潜在因子（SGD 增量更新）
 *   - Thompson Sampling 自适应探索率
 *   - 时间衰减权重（指数半衰期，默认 24 小时）
 *
 * 依赖注入：通过 setDbClientProvider() 注入数据库客户端，
 * 不设置时 DB 相关方法会显式失败，避免把基础设施异常伪装成“无推荐”。
 */

#pragma once

#include <string>
#include <vector>
#include <atomic>
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
 * @brief 推荐候选项，携带评分、来源算法和推荐理由
 */
struct RecommendationCandidate {
    std::string itemId;        ///< 物品 ID
    std::string itemType;      ///< 物品类型: "stone", "user", "boat"
    double score;              ///< 综合推荐分数
    std::string reason;        ///< 人类可读的推荐理由
    std::string algorithm;     ///< 产生该候选的算法名称
    Json::Value metadata;      ///< 附加元数据（如情绪标签、创建时间等）
};

/**
 * @brief 用户画像，维护潜在因子向量和交互统计
 * @details 用于协同过滤和 UCB 探索-利用平衡计算
 */
struct UserProfile {
    std::string userId;                                        ///< 用户 ID
    std::vector<float> latentFactors;                          ///< 潜在因子向量（维度 = latentDim_）
    std::unordered_map<std::string, double> moodPreferences;   ///< 情绪偏好分布 (mood -> 偏好度)
    std::unordered_map<std::string, int> interactionCounts;    ///< 物品交互计数 (itemId -> 次数)
    double explorationRate;                                    ///< UCB 探索率
    int totalInteractions;                                     ///< 累计交互总数
};

/**
 * @brief 多算法融合推荐引擎
 *
 * @details 单例模式。融合 User-CF、Item-CF、内容推荐、UCB 探索和图传播五种策略，
 * 通过 MMR (Maximal Marginal Relevance) 多样性重排序输出最终结果。
 *
 * 在线学习能力：
 * - 用户交互实时 SGD 增量更新潜在因子
 * - Thompson Sampling 自适应探索率
 * - 指数半衰期时间衰减（默认 24 小时）
 *
 * 依赖注入：通过 setDbClientProvider() 注入数据库客户端，
 * 不设置时 DB 相关方法会显式失败，避免把基础设施异常伪装成“无推荐”。
 */
class RecommendationEngine {
public:
    /** @brief 获取全局单例 */
    static RecommendationEngine& getInstance();

    /**
     * @brief 初始化引擎，设置潜在因子维度
     * @param latentDim 潜在因子向量维度，默认 32
     */
    void initialize(int latentDim = 32);

    /**
     * @brief 注入数据库客户端提供者（依赖注入）
     * @details 生产环境中注入 [](){ return app().getDbClient("default"); }，
     *          不设置则 DB 相关方法会显式失败。
     * @param provider 返回 DbClient 共享指针的工厂函数
     */
    using DbClientPtr = std::shared_ptr<drogon::orm::DbClient>;
    using DbClientProvider = std::function<DbClientPtr()>;
    void setDbClientProvider(DbClientProvider provider);

    /**
     * @brief 获取个性化推荐结果
     * @param userId 用户 ID
     * @param itemType 推荐物品类型（"stone", "user", "boat"）
     * @param limit 返回数量上限
     * @param callback 完成回调，results 为推荐列表，error 非空表示失败
     */
    void getRecommendations(
        const std::string& userId,
        const std::string& itemType,
        int limit,
        std::function<void(const std::vector<RecommendationCandidate>& results, const std::string& error)> callback
    );

    /**
     * @brief 记录用户交互，触发在线学习更新
     * @param userId 用户 ID
     * @param itemId 物品 ID
     * @param interactionType 交互类型（"click", "like", "share" 等）
     * @param reward 奖励信号，正值表示正反馈
     */
    void recordInteraction(
        const std::string& userId,
        const std::string& itemId,
        const std::string& interactionType,
        double reward
    );

    /**
     * @brief 计算两个用户的相似度（基于潜在因子余弦距离）
     * @param userId1 用户 A 的 ID
     * @param userId2 用户 B 的 ID
     * @return 相似度 [0, 1]
     */
    double computeUserSimilarity(const std::string& userId1, const std::string& userId2);

    /**
     * @brief 计算两个物品的相似度（基于共现矩阵）
     * @param itemId1 物品 A 的 ID
     * @param itemId2 物品 B 的 ID
     * @return 相似度 [0, 1]
     */
    double computeItemSimilarity(const std::string& itemId1, const std::string& itemId2);

    /**
     * @brief User-CF 推荐：基于相似用户的协同过滤
     * @param userId 目标用户 ID
     * @param topK 返回数量
     * @return 推荐候选列表
     */
    std::vector<RecommendationCandidate> userBasedCF(
        const std::string& userId, int topK);

    /**
     * @brief Item-CF 推荐：基于相似物品的协同过滤
     * @param userId 目标用户 ID
     * @param topK 返回数量
     * @return 推荐候选列表
     */
    std::vector<RecommendationCandidate> itemBasedCF(
        const std::string& userId, int topK);

    /**
     * @brief 基于内容的推荐（情绪向量兼容性匹配）
     * @param userId 目标用户 ID
     * @param userMood 用户当前情绪类型
     * @param topK 返回数量
     * @return 推荐候选列表
     */
    std::vector<RecommendationCandidate> contentBasedRecommend(
        const std::string& userId, const std::string& userMood, int topK);

    /**
     * @brief 混合推荐：融合 CF、内容和探索三种策略
     * @param userId 目标用户 ID
     * @param topK 返回数量
     * @param cfWeight 协同过滤权重，默认 0.4
     * @param contentWeight 内容推荐权重，默认 0.4
     * @param exploreWeight 探索权重，默认 0.2
     * @return 经 MMR 重排序后的推荐候选列表
     */
    std::vector<RecommendationCandidate> hybridRecommend(
        const std::string& userId, int topK,
        double cfWeight = 0.4, double contentWeight = 0.4, double exploreWeight = 0.2);

private:
    RecommendationEngine() = default;
    ~RecommendationEngine() = default;
    RecommendationEngine(const RecommendationEngine&) = delete;
    RecommendationEngine& operator=(const RecommendationEngine&) = delete;

    std::atomic<bool> initialized_{false};  ///< 初始化标记
    int latentDim_ = 32;                   ///< 潜在因子向量维度
    mutable std::recursive_mutex mutex_;   ///< 递归锁（hybridRecommend 内部会嵌套调用子算法）
    std::mt19937 rng_{std::random_device{}()};  ///< Thompson Sampling 和随机初始化用
    DbClientProvider dbClientProvider_;    ///< 依赖注入的数据库客户端提供者

    std::unordered_map<std::string, UserProfile> userProfiles_;  ///< 用户画像缓存

    double ucbC_ = 2.0;  ///< UCB 探索系数，越大越倾向探索未知物品

    // ===== 核心算法 =====

    /**
     * @brief 矩阵分解预测分数（ALS 风格点积）
     * @param userFactors 用户潜在因子
     * @param itemFactors 物品潜在因子
     * @return 预测评分
     */
    double predictMF(const std::vector<float>& userFactors, const std::vector<float>& itemFactors);

    /**
     * @brief UCB (Upper Confidence Bound) 探索-利用平衡分数
     * @param avgReward 物品历史平均奖励
     * @param itemCount 物品被选择次数
     * @param totalCount 总选择次数
     * @return UCB 分数
     */
    double computeUCB(double avgReward, int itemCount, int totalCount);

    /**
     * @brief Thompson Sampling 采样（Beta 分布）
     * @param successes 成功次数（正反馈）
     * @param failures 失败次数（负反馈/忽略）
     * @return 采样值 [0, 1]
     */
    double thompsonSample(int successes, int failures);

    /**
     * @brief 指数时间衰减权重
     * @param timestampMs 事件时间戳（毫秒）
     * @param halfLifeHours 半衰期（小时），默认 24
     * @return 衰减权重 (0, 1]
     */
    double timeDecay(int64_t timestampMs, double halfLifeHours = 24.0);

    /**
     * @brief MMR 多样性感知重排序
     * @details 在相关性和多样性之间取平衡，lambda 控制权衡比例。
     *          每轮贪心选择 lambda*relevance + (1-lambda)*diversity 最大的候选。
     * @param pool 候选池
     * @param lambda 相关性权重 [0, 1]，越大越偏相关性
     * @param topK 返回数量
     */
    std::vector<RecommendationCandidate> mmrRerank(
        std::vector<RecommendationCandidate> pool,
        double lambda,
        int topK
    );

    /**
     * @brief 简化 PageRank 图传播分数
     * @details 在用户-物品二部图上做 2 跳传播，计算用户到物品的可达性分数
     * @param userId 用户 ID
     * @param itemId 物品 ID
     * @param userItemGraph 用户-物品交互图（邻接表）
     */
    double graphPropagationScore(
        const std::string& userId,
        const std::string& itemId,
        const std::unordered_map<std::string, std::vector<std::string>>& userItemGraph
    );

    /**
     * @brief 情绪兼容性分数
     * @details 相同情绪得高分，互补情绪（如 sad-hopeful）也有适度加分
     */
    double emotionCompatibilityScore(const std::string& userMood, const std::string& itemMood);

    /**
     * @brief 获取或创建用户画像（懒初始化潜在因子）
     */
    UserProfile& getOrCreateProfile(const std::string& userId);

    /**
     * @brief SGD 在线更新潜在因子
     * @param userFactors 用户因子（会被修改）
     * @param itemFactors 物品因子（会被修改）
     * @param error 预测误差 (actual - predicted)
     * @param learningRate 学习率
     * @param regularization L2 正则化系数
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
