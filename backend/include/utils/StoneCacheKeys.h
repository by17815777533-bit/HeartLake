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

uint64_t currentStoneListNamespace() noexcept;

uint64_t currentStoneListSortNamespace(const std::string &sortBy) noexcept;

void bumpStoneListNamespace() noexcept;

void bumpStoneListSortNamespace(const std::string &sortBy) noexcept;

} // namespace heartlake::utils::stone_cache
