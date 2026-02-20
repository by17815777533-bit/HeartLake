/**
 * @file StoneService.cpp
 * @brief 石头领域服务实现
 */

#include "domain/stone/services/StoneService.h"
#include "utils/IdGenerator.h"

namespace heartlake::domain::stone {

StoneEntity StoneService::createStone(const std::string& userId, const std::string& content,
                                      const std::string& stoneType, const std::string& stoneColor,
                                      const std::string& moodType, bool isAnonymous) {
    StoneEntity stone;
    stone.userId = userId;
    stone.content = content;
    stone.stoneType = stoneType;
    stone.stoneColor = stoneColor;
    stone.moodType = moodType;
    stone.isAnonymous = isAnonymous;
    return repository_->save(stone);
}

std::optional<StoneEntity> StoneService::getStone(const std::string& stoneId) {
    return repository_->findById(stoneId);
}

std::vector<StoneEntity> StoneService::getUserStones(const std::string& userId, int page, int pageSize) {
    return repository_->findByUserId(userId, page, pageSize);
}

std::vector<StoneEntity> StoneService::listStones(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) {
    return repository_->findAll(page, pageSize, sortBy, filterMood);
}

int StoneService::getTotalCount(const std::string& filterMood) {
    return repository_->countAll(filterMood);
}

int StoneService::getUserStoneCount(const std::string& userId) {
    return repository_->countByUserId(userId);
}

void StoneService::deleteStone(const std::string& stoneId, const std::string& userId) {
    auto stone = repository_->findById(stoneId);
    if (!stone) throw std::runtime_error("石头不存在");
    if (stone->userId != userId) throw std::runtime_error("无权删除此石头");
    repository_->deleteById(stoneId);
}

void StoneService::incrementViews(const std::string& stoneId) {
    repository_->incrementViewCount(stoneId);
}

} // namespace heartlake::domain::stone
