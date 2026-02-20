/**
 * @file ResonanceSearchService.h
 * @brief 同频共鸣搜索服务 - 向量相似度匹配灵魂伴侣
 */

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace heartlake::infrastructure {

struct ResonanceMatch {
    std::string stoneId;
    std::string userId;
    std::string content;
    float similarity;
    int deliveryDelaySeconds;  // 基于距离的投递延迟
};

class ResonanceSearchService {
public:
    static ResonanceSearchService& getInstance();

    void initialize(size_t embeddingDim = 128);

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
    size_t embeddingDim_ = 128;
    bool initialized_ = false;
    bool useMilvus_ = false;
};

} // namespace heartlake::infrastructure
