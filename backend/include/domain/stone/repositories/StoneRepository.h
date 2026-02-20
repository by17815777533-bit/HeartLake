/**
 * @file StoneRepository.h
 * @brief 石头仓储实现
 */

#pragma once

#include "IStoneRepository.h"
#include <drogon/drogon.h>

namespace heartlake::domain::stone {

class StoneRepository : public IStoneRepository {
public:
    StoneEntity save(const StoneEntity& stone) override;
    std::optional<StoneEntity> findById(const std::string& stoneId) override;
    std::vector<StoneEntity> findByUserId(const std::string& userId, int page, int pageSize) override;
    std::vector<StoneEntity> findAll(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) override;
    int countAll(const std::string& filterMood = "") override;
    int countByUserId(const std::string& userId) override;
    void deleteById(const std::string& stoneId) override;
    void incrementViewCount(const std::string& stoneId) override;
    void updateEmotionScore(const std::string& stoneId, float score, const std::string& mood) override;

private:
    StoneEntity rowToEntity(const drogon::orm::Row& row);
};

} // namespace heartlake::domain::stone
