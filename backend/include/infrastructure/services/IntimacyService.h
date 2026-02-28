/**
 * @brief 亲密分服务 -- 基于石头互动自动量化用户关系亲密度
 *
 * @details
 * HeartLake 的好友关系不需要手动申请/通过，而是通过互动行为
 * 自然演化。本服务综合 18 个维度计算两个用户之间的亲密分：
 *
 * 核心维度：
 * - interactionStrength：互动强度（涟漪 + 纸船评论次数加权）
 * - reciprocityScore：互惠度（双向互动的均衡程度）
 * - coRippleScore：共鸣度（对同一石头产生涟漪的频率）
 * - moodResonance：情绪共振（情绪曲线的相关性）
 * - semanticSimilarity：语义相似度（石头内容的 embedding 余弦距离）
 *
 * 防刷维度：
 * - antiGamingPenalty：反作弊惩罚（检测刷分行为）
 * - behaviorHealth：行为健康度（异常模式检测）
 * - temporalDiversity：时间多样性（避免集中刷互动）
 *
 * 亲密分达到 12.0 时解锁私聊能力（canChat），
 * 等级从 stranger -> acquaintance -> friend -> close_friend 递进。
 */

#pragma once

#include <string>
#include <vector>

namespace heartlake::infrastructure {

/// 用户间亲密关系的完整画像
struct IntimacyProfile {
    std::string userId;
    std::string username;
    std::string nickname;
    double intimacyScore{0.0};              ///< 综合亲密分
    std::string intimacyLevel{"stranger"};  ///< 关系等级
    int interactionCount{0};                ///< 总互动次数
    int boatComments{0};                    ///< 纸船评论数
    int rippleCount{0};                     ///< 涟漪数
    double interactionStrength{0.0};        ///< 互动强度
    double reciprocityScore{0.0};           ///< 互惠度
    double coRippleScore{0.0};              ///< 共鸣度
    double moodResonance{0.5};              ///< 情绪共振
    double semanticSimilarity{0.5};         ///< 语义相似度
    double emotionTrendAlignment{0.5};      ///< 情绪趋势对齐度
    double freshnessScore{0.0};             ///< 互动新鲜度（近期权重更高）
    double temporalDiversity{0.0};          ///< 时间多样性
    double antiGamingPenalty{0.0};          ///< 反作弊惩罚
    double behaviorHealth{1.0};             ///< 行为健康度
    double dialogueCohesion{0.0};           ///< 对话连贯性
    double responseAgility{0.0};            ///< 回复敏捷度
    double graphAffinity{0.5};              ///< 社交图谱亲和度
    double emotionSynchrony{0.5};           ///< 情绪同步性
    double aiCompatibility{0.5};            ///< AI 预测的兼容性
    std::string lastInteractedAt;           ///< 最近互动时间
    bool canChat{false};                    ///< 是否已解锁私聊
};

class IntimacyService {
public:
    static IntimacyService& getInstance();

    /**
     * @brief 获取与指定用户亲密度最高的 N 个用户
     * @param userId 目标用户
     * @param limit 返回数量上限，默认 80
     * @param minScore 最低亲密分过滤阈值，默认 12.0
     * @return 按亲密分降序排列的用户画像列表
     */
    std::vector<IntimacyProfile> getTopIntimacyPeers(
        const std::string& userId,
        int limit = 80,
        double minScore = 12.0
    ) const;

    /**
     * @brief 查询两个用户之间的亲密分
     * @param userId 用户 A
     * @param peerId 用户 B
     * @return 亲密分，无互动记录时返回 0.0
     */
    double getIntimacyScore(
        const std::string& userId,
        const std::string& peerId
    ) const;

    /**
     * @brief 判断两个用户是否可以私聊
     * @param userId 用户 A
     * @param peerId 用户 B
     * @param threshold 私聊解锁阈值，默认 12.0
     */
    bool canChat(
        const std::string& userId,
        const std::string& peerId,
        double threshold = 12.0
    ) const;

    /// 根据亲密分映射到关系等级字符串
    static std::string levelFromScore(double score);

private:
    IntimacyService() = default;
    ~IntimacyService() = default;
    IntimacyService(const IntimacyService&) = delete;
    IntimacyService& operator=(const IntimacyService&) = delete;
};

}  // namespace heartlake::infrastructure
