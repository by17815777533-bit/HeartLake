/**
 * @brief 同频共鸣搜索服务 -- 基于向量相似度匹配灵魂伴侣
 *
 * @details
 * 当用户投出一颗石头时，本服务将石头内容转化为 embedding 向量，
 * 在 Milvus 向量库中检索语义最相似的其他石头，实现"同频共鸣"匹配。
 *
 * 匹配结果不仅包含基础的余弦相似度，还融合四维共鸣评分：
 * - semanticScore：语义层面的内容相似度
 * - trajectoryScore：情绪轨迹的趋势吻合度
 * - temporalScore：时间维度的活跃重叠度
 * - diversityScore：多样性加分（避免总是匹配同一批用户）
 *
 * 投递延迟机制：相似度越高的石头投递越快（模拟"距离越近越快到达"），
 * 通过 calculateDeliveryDelay() 将相似度映射为秒级延迟。
 *
 * 底层可选 Milvus 向量库或内存 HNSW 索引，通过 useMilvus_ 标记切换。
 */

#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <optional>

namespace heartlake::infrastructure {

/// 共鸣匹配结果
struct ResonanceMatch {
    std::string stoneId;
    std::string userId;
    std::string content;
    std::string stoneType = "medium";
    std::string stoneColor = "#7A92A3";
    std::string moodType = "neutral";
    std::string authorName;
    std::string createdAt;
    float emotionScore = 0.0f;
    int boatCount = 0;
    float similarity;              ///< 基础余弦相似度
    int deliveryDelaySeconds;      ///< 基于相似度计算的投递延迟（秒）
    // 四维共鸣分数
    float resonanceTotal    = 0.0f;  ///< 综合共鸣分
    float semanticScore     = 0.0f;  ///< 语义相似度分
    float trajectoryScore   = 0.0f;  ///< 情绪轨迹吻合分
    float temporalScore     = 0.0f;  ///< 时间活跃重叠分
    float diversityScore    = 0.0f;  ///< 多样性加分
    std::string resonanceReason;     ///< 共鸣原因的自然语言描述
};

class ResonanceSearchService {
public:
    static ResonanceSearchService& getInstance();

    /**
     * @brief 初始化搜索服务
     * @param embeddingDim embedding 向量维度，默认 256
     */
    void initialize(size_t embeddingDim = 256);

    /**
     * @brief 为石头生成 embedding 并存入向量索引
     * @param stoneId 石头 ID
     * @param content 石头文本内容
     */
    void indexStone(const std::string& stoneId, const std::string& content);

    /**
     * @brief 搜索与指定石头共鸣的其他石头
     * @param requesterUserId 发起搜索的用户 ID，用于加载情绪轨迹并过滤已交互内容
     * @param stoneId 查询石头 ID
     * @param content 查询石头内容
     * @param threshold 最低相似度阈值，默认 0.85
     * @param limit 最大返回数量，默认 10
     * @return 按综合共鸣分降序排列的匹配结果
     */
    std::vector<ResonanceMatch> searchResonance(
        const std::string& requesterUserId,
        const std::string& stoneId,
        const std::string& content,
        float threshold = 0.85f,
        int limit = 10
    );

    /**
     * @brief 根据相似度计算投递延迟
     * @param similarity 相似度 [0, 1]
     * @return 延迟秒数，相似度越高延迟越短
     */
    int calculateDeliveryDelay(float similarity);

    /// 从向量索引中删除指定石头
    void removeStone(const std::string& stoneId);

private:
    ResonanceSearchService() = default;
    ~ResonanceSearchService() = default;
    ResonanceSearchService(const ResonanceSearchService&) = delete;
    ResonanceSearchService& operator=(const ResonanceSearchService&) = delete;

    static constexpr const char* COLLECTION_NAME = "stone_embeddings";  ///< Milvus 集合名
    size_t embeddingDim_ = 256;                ///< 向量维度
    std::atomic<bool> initialized_{false};     ///< 多线程读写，必须原子
    bool useMilvus_ = false;                   ///< 是否使用 Milvus（否则走内存 HNSW）
};

} // namespace heartlake::infrastructure
