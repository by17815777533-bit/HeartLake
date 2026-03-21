#include "utils/StoneCacheKeys.h"

#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

namespace {

std::shared_mutex gStoneListNamespaceMutex;
std::unordered_map<std::string, uint64_t> gStoneListNamespaces;

std::string normalizeStoneSortKey(const std::string &sortBy) {
  if (sortBy == "view_count" || sortBy == "boat_count" ||
      sortBy == "ripple_count") {
    return sortBy;
  }
  return "created_at";
}

std::string normalizeScopeSegment(const std::string &value) {
  return value.empty() ? "_" : value;
}

std::string buildStoneListScopeKey(const std::string &userId,
                                   const std::string &filterMood) {
  std::ostringstream key;
  if (userId.empty()) {
    key << "global:";
  } else {
    key << "user=" << userId << ":";
  }
  key << "mood=" << normalizeScopeSegment(filterMood);
  return key.str();
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
  key << "stone_list:v5:" << buildStoneListScopeKey(userId, filterMood)
      << ":ns=" << currentStoneListNamespace(userId, filterMood)
      << ":sort=" << normalizedSort << ":p=" << page << ":ps=" << pageSize;
  return key.str();
}

std::string buildStoneRippleStateCacheKey(const std::string &userId,
                                          const std::string &stoneId) {
  return "stone:rippled:" + userId + ":" + stoneId;
}

uint64_t currentStoneListNamespace(const std::string &userId,
                                   const std::string &filterMood) noexcept {
  const auto scopeKey = buildStoneListScopeKey(userId, filterMood);
  {
    std::shared_lock lock(gStoneListNamespaceMutex);
    const auto it = gStoneListNamespaces.find(scopeKey);
    if (it != gStoneListNamespaces.end()) {
      return it->second;
    }
  }

  std::unique_lock lock(gStoneListNamespaceMutex);
  return gStoneListNamespaces.try_emplace(scopeKey, 1).first->second;
}

void bumpStoneListNamespace(const std::string &userId,
                            const std::string &filterMood) noexcept {
  std::unique_lock lock(gStoneListNamespaceMutex);
  auto &ns = gStoneListNamespaces[buildStoneListScopeKey(userId, filterMood)];
  if (ns == 0) {
    ns = 1;
  }
  ++ns;
}

void bumpStoneListNamespacesForStone(const std::string &ownerUserId,
                                     const std::string &moodType) noexcept {
  bumpStoneListNamespace("", "");
  if (!moodType.empty()) {
    bumpStoneListNamespace("", moodType);
  }

  if (ownerUserId.empty()) {
    return;
  }

  bumpStoneListNamespace(ownerUserId, "");
  if (!moodType.empty()) {
    bumpStoneListNamespace(ownerUserId, moodType);
  }
}

} // namespace heartlake::utils::stone_cache
