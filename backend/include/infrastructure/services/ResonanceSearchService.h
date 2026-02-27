/**
 * 同频共鸣搜索服务 - 向量相似度匹配灵魂伴侣
 */

#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <optional>

namespace heartlake::infrastructure {

struct ResonanceMatch {
    std::string stoneId;
    std::string userId;
    std::string content;
    float similarity;
    int deliveryDelaySeconds;  // 基于距离的投递延迟
    // 四维共鸣分数
    float resonanceTotal    = 0.0f;
    float semanticScore     = 0.0f;
    float trajectoryScore   = 0.0f;
    float temporalScore     = 0.0f;
    float diversityScore    = 0.0f;
    std::string resonanceReason;
};

class ResonanceSearchService {
public:
    static ResonanceSearchService& getInstance();

    void initialize(size_t embeddingDim = 256);

    // 为石头生成并存储嵌入向量
    void indexStone(const std::string& stoneId, const std::string& content);

    // 搜索共鸣石头（相似度 > threshold）
    std::vector<ResonanceMatch> searchResonance(
        const std::string& stoneId,
        const std::string& content,
        float threshold = 0.85f,
        int limit = 10
    );

    // 计算投递延迟
    int calculateDeliveryDelay(float similarity);

    // 删除石头索引
    void removeStone(const std::string& stoneId);

private:
    ResonanceSearchService() = default;
    ~ResonanceSearchService() = default;
    ResonanceSearchService(const ResonanceSearchService&) = delete;
    ResonanceSearchService& operator=(const ResonanceSearchService&) = delete;

    static constexpr const char* COLLECTION_NAME = "stone_embeddings";
    size_t embeddingDim_ = 256;
    std::atomic<bool> initialized_{false};  ///< 多线程读写，必须原子
    bool useMilvus_ = false;
};

} // namespace heartlake::infrastructure
