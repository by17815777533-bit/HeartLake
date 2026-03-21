#include "utils/StoneCacheKeys.h"

#include <atomic>
#include <sstream>

namespace {

std::atomic<uint64_t> gStoneListNamespace{1};
std::atomic<uint64_t> gCreatedAtNamespace{1};
std::atomic<uint64_t> gViewCountNamespace{1};
std::atomic<uint64_t> gBoatCountNamespace{1};
std::atomic<uint64_t> gRippleCountNamespace{1};

std::string normalizeStoneSortKey(const std::string &sortBy) {
  if (sortBy == "view_count" || sortBy == "boat_count" ||
      sortBy == "ripple_count") {
    return sortBy;
  }
  return "created_at";
}

std::atomic<uint64_t> &selectSortNamespace(const std::string &sortBy) {
  const auto normalizedSort = normalizeStoneSortKey(sortBy);
  if (normalizedSort == "view_count") {
    return gViewCountNamespace;
  }
  if (normalizedSort == "boat_count") {
    return gBoatCountNamespace;
  }
  if (normalizedSort == "ripple_count") {
    return gRippleCountNamespace;
  }
  return gCreatedAtNamespace;
}

} // namespace

namespace heartlake::utils::stone_cache {

std::string buildStoneDetailCacheKey(const std::string &stoneId) {
  return "stone:" + stoneId;
}

std::string buildStoneListCacheKey(int page, int pageSize,
                                   const std::string &sortBy,
                                   const std::string &filterMood,
                                   const std::string &userId) {
  const auto normalizedSort = normalizeStoneSortKey(sortBy);

  std::ostringstream key;
  key << "stone_list:v4:g=" << currentStoneListNamespace()
      << ":sort=" << normalizedSort
      << ":s=" << currentStoneListSortNamespace(normalizedSort)
      << ":mood=" << filterMood << ":user=" << userId << ":p=" << page
      << ":ps=" << pageSize;
  return key.str();
}

std::string buildStoneRippleStateCacheKey(const std::string &userId,
                                          const std::string &stoneId) {
  return "stone:rippled:" + userId + ":" + stoneId;
}

uint64_t currentStoneListNamespace() noexcept {
  return gStoneListNamespace.load(std::memory_order_relaxed);
}

uint64_t currentStoneListSortNamespace(const std::string &sortBy) noexcept {
  return selectSortNamespace(sortBy).load(std::memory_order_relaxed);
}

void bumpStoneListNamespace() noexcept {
  gStoneListNamespace.fetch_add(1, std::memory_order_relaxed);
}

void bumpStoneListSortNamespace(const std::string &sortBy) noexcept {
  selectSortNamespace(sortBy).fetch_add(1, std::memory_order_relaxed);
}

} // namespace heartlake::utils::stone_cache
