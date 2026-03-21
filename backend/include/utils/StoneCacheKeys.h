#pragma once

#include <cstdint>
#include <string>

namespace heartlake::utils::stone_cache {

std::string buildStoneDetailCacheKey(const std::string &stoneId);

std::string buildStoneListCacheKey(int page, int pageSize,
                                   const std::string &sortBy,
                                   const std::string &filterMood,
                                   const std::string &userId);

std::string buildStoneRippleStateCacheKey(const std::string &userId,
                                          const std::string &stoneId);

uint64_t currentStoneListNamespace(const std::string &userId = "",
                                   const std::string &filterMood = "") noexcept;

void bumpStoneListNamespace(const std::string &userId = "",
                            const std::string &filterMood = "") noexcept;

void bumpStoneListNamespacesForStone(const std::string &ownerUserId,
                                     const std::string &moodType) noexcept;

} // namespace heartlake::utils::stone_cache
