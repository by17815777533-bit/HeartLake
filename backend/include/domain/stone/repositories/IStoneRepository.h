/**
 * @file IStoneRepository.h
 * @brief 石头仓储接口
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <json/json.h>

namespace heartlake::domain::stone {

struct StoneEntity {
    std::string stoneId;
    std::string userId;
    std::string content;
    std::string stoneType;
    std::string stoneColor;
    std::string moodType;
    bool isAnonymous;
    int ripplesCount;
    int boatsCount;
    int viewsCount;
    std::string status;
    std::string createdAt;
};

class IStoneRepository {
public:
    virtual ~IStoneRepository() = default;
    virtual StoneEntity save(const StoneEntity& stone) = 0;
    virtual std::optional<StoneEntity> findById(const std::string& stoneId) = 0;
    virtual std::vector<StoneEntity> findByUserId(const std::string& userId, int page, int pageSize) = 0;
    virtual std::vector<StoneEntity> findAll(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) = 0;
    virtual int countAll(const std::string& filterMood = "") = 0;
    virtual int countByUserId(const std::string& userId) = 0;
    virtual void deleteById(const std::string& stoneId) = 0;
    virtual void incrementViewCount(const std::string& stoneId) = 0;
    virtual void updateEmotionScore(const std::string& stoneId, float score, const std::string& mood) = 0;
};

} // namespace heartlake::domain::stone
