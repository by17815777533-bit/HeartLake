/**
 * @file StoneService.h
 * @brief 石头领域服务
 */

#pragma once

#include "domain/stone/repositories/IStoneRepository.h"
#include <memory>

namespace heartlake::domain::stone {

class StoneService {
public:
    explicit StoneService(std::shared_ptr<IStoneRepository> repository)
        : repository_(repository) {}

    StoneEntity createStone(const std::string& userId, const std::string& content,
                           const std::string& stoneType, const std::string& stoneColor,
                           const std::string& moodType, bool isAnonymous);

    std::optional<StoneEntity> getStone(const std::string& stoneId);
    std::vector<StoneEntity> getUserStones(const std::string& userId, int page, int pageSize);
    std::vector<StoneEntity> listStones(int page, int pageSize, const std::string& sortBy, const std::string& filterMood);
    int getTotalCount(const std::string& filterMood = "");
    int getUserStoneCount(const std::string& userId);
    void deleteStone(const std::string& stoneId, const std::string& userId);
    void incrementViews(const std::string& stoneId);

private:
    std::shared_ptr<IStoneRepository> repository_;
};

} // namespace heartlake::domain::stone
