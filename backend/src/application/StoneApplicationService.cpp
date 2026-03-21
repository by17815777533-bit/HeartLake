/**
 * @file StoneApplicationService.cpp
 * @brief 石头应用服务 —— 石头发布、详情、列表、删除的完整业务编排
 *
 * 发布石头的异步处理管线（processStoneAsync）：
 *   1. 情感分析 → 更新 emotion_score/mood_type → 心理风险评估 → 暖心语录推送
 *   2. 向量嵌入 → 持久化到 stone_embeddings → 插入 HNSW 索引
 *   3. 长文本自动摘要（> MIN_LENGTH 才触发）
 *   4. DualMemoryRAG 生成 AI 暖心评论（湖神纸船）
 *
 * 浏览量采用 Redis 聚合策略：每次 INCR，累积 10 次后批量写回数据库。
 */

#include "application/StoneApplicationService.h"
#include "infrastructure/ai/AIService.h"
#include "infrastructure/ai/DualMemoryRAG.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "infrastructure/ai/SummaryService.h"
#include "infrastructure/cache/RedisCache.h"
#include "infrastructure/services/EmotionTrackingService.h"
#include "infrastructure/services/NotificationPushService.h"
#include "infrastructure/services/VIPService.h"
#include "infrastructure/services/WarmQuoteService.h"
#include "interfaces/api/BroadcastWebSocketController.h"
#include "utils/IdGenerator.h"
#include "utils/PsychologicalRiskAssessment.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/StoneCacheKeys.h"
#include <algorithm>
#include <drogon/drogon.h>
#include <json/json.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#undef ERROR
#endif

using namespace heartlake::utils;

namespace heartlake {
namespace application {

namespace {

constexpr int kStoneDetailCacheTtlSeconds = 300;
constexpr int kStoneListCacheTtlSeconds = 60;
constexpr int kStoneRippleStateCacheTtlSeconds = 60;

std::string normalizeMoodType(std::string mood) {
  std::transform(mood.begin(), mood.end(), mood.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  static const std::unordered_map<std::string, std::string> aliases = {
      {"peaceful", "calm"},      {"hopeful", "calm"},
      {"grateful", "happy"},     {"lonely", "sad"},
      {"joy", "happy"},          {"happiness", "happy"},
      {"sadness", "sad"},        {"fear", "anxious"},
      {"anger", "angry"},        {"surprise", "surprised"},
      {"uncertain", "confused"},
  };
  auto it = aliases.find(mood);
  if (it != aliases.end()) {
    mood = it->second;
  }

  static const std::unordered_set<std::string> validMoods = {
      "calm",    "happy",    "sad",       "angry",
      "anxious", "confused", "surprised", "neutral"};
  if (validMoods.find(mood) == validMoods.end()) {
    return "neutral";
  }
  return mood;
}

std::string safeString(const drogon::orm::Row &row, const char *column,
                       const std::string &fallback = "") {
  return row[column].isNull() ? fallback : row[column].as<std::string>();
}

int safeInt(const drogon::orm::Row &row, const char *column, int fallback = 0) {
  return row[column].isNull() ? fallback : row[column].as<int>();
}

bool safeBool(const drogon::orm::Row &row, const char *column,
              bool fallback = false) {
  return row[column].isNull() ? fallback : row[column].as<bool>();
}

using heartlake::utils::stone_cache::buildStoneDetailCacheKey;
using heartlake::utils::stone_cache::buildStoneListCacheKey;
using heartlake::utils::stone_cache::buildStoneRippleStateCacheKey;

std::string normalizeStoneSort(const std::string &sortBy) {
  if (sortBy == "view_count" || sortBy == "boat_count" ||
      sortBy == "ripple_count") {
    return sortBy;
  }
  return "created_at";
}

std::vector<std::string> collectStoneIds(const Json::Value &stones) {
  std::vector<std::string> stoneIds;
  if (!stones.isArray()) {
    return stoneIds;
  }

  stoneIds.reserve(stones.size());
  for (const auto &stone : stones) {
    if (!stone.isMember("stone_id")) {
      continue;
    }

    const auto stoneId = stone["stone_id"].asString();
    if (!stoneId.empty()) {
      stoneIds.push_back(stoneId);
    }
  }
  return stoneIds;
}

void syncStoneCollectionAliases(Json::Value &response) {
  if (!response.isMember("stones")) {
    return;
  }
  response["items"] = response["stones"];
  response["list"] = response["stones"];
}

std::unordered_map<std::string, bool>
loadRippleStates(const std::shared_ptr<heartlake::core::cache::CacheManager>
                     &cacheManager,
                 const std::string &currentUserId,
                 const std::vector<std::string> &stoneIds) {
  std::unordered_map<std::string, bool> states;
  if (stoneIds.empty()) {
    return states;
  }

  std::vector<std::string> uniqueStoneIds;
  uniqueStoneIds.reserve(stoneIds.size());
  std::unordered_set<std::string> seen;
  for (const auto &stoneId : stoneIds) {
    if (!stoneId.empty() && seen.insert(stoneId).second) {
      uniqueStoneIds.push_back(stoneId);
    }
  }

  if (currentUserId.empty()) {
    for (const auto &stoneId : uniqueStoneIds) {
      states.emplace(stoneId, false);
    }
    return states;
  }

  std::vector<std::string> misses;
  misses.reserve(uniqueStoneIds.size());
  if (cacheManager) {
    for (const auto &stoneId : uniqueStoneIds) {
      auto cached = cacheManager->get(
          buildStoneRippleStateCacheKey(currentUserId, stoneId));
      if (cached) {
        states.emplace(stoneId, *cached == "1");
      } else {
        misses.push_back(stoneId);
      }
    }
  } else {
    misses = uniqueStoneIds;
  }

  if (!misses.empty()) {
    auto dbClient = drogon::app().getDbClient("default");
    auto rippleResult = dbClient->execSqlSync(
        "SELECT stone_id FROM ripples "
        "WHERE user_id = $1 AND stone_id = ANY($2::text[])",
        currentUserId, toPgTextArrayLiteral(misses));

    std::unordered_set<std::string> hitStoneIds;
    hitStoneIds.reserve(rippleResult.size());
    for (const auto &row : rippleResult) {
      if (!row["stone_id"].isNull()) {
        hitStoneIds.insert(row["stone_id"].as<std::string>());
      }
    }

    for (const auto &stoneId : misses) {
      const bool hasRippled = hitStoneIds.find(stoneId) != hitStoneIds.end();
      states.emplace(stoneId, hasRippled);
      if (cacheManager) {
        cacheManager->set(buildStoneRippleStateCacheKey(currentUserId, stoneId),
                          hasRippled ? "1" : "0",
                          kStoneRippleStateCacheTtlSeconds);
      }
    }
  }

  return states;
}

void applyRippleStates(Json::Value &stones,
                       const std::unordered_map<std::string, bool> &states) {
  if (!stones.isArray()) {
    return;
  }

  for (auto &stone : stones) {
    const auto stoneId =
        stone.isMember("stone_id") ? stone["stone_id"].asString() : "";
    auto it = states.find(stoneId);
    stone["has_rippled"] = it != states.end() ? it->second : false;
  }
}

void invalidateStoneListCaches(
    const std::shared_ptr<heartlake::core::cache::CacheManager> &cacheManager,
    const std::string &sortBy = "") {
  if (!cacheManager) {
    return;
  }
  if (!sortBy.empty()) {
    heartlake::utils::stone_cache::bumpStoneListSortNamespace(sortBy);
    return;
  }
  heartlake::utils::stone_cache::bumpStoneListNamespace();
}

int resolveStoneListTotal(const drogon::orm::DbClientPtr &dbClient,
                          const drogon::orm::Result &result, int page,
                          const std::string &userId,
                          const std::string &filterMood) {
  if (!result.empty()) {
    return result[0]["total_count"].isNull() ? 0
                                             : result[0]["total_count"].as<int>();
  }
  if (page <= 1) {
    return 0;
  }

  std::string countSql =
      "SELECT COUNT(*)::INTEGER AS total_count "
      "FROM stones s "
      "WHERE s.deleted_at IS NULL AND s.status = 'published' ";
  int paramIndex = 1;
  if (!userId.empty()) {
    countSql += "AND s.user_id = $" + std::to_string(paramIndex++) + " ";
  }
  if (!filterMood.empty()) {
    countSql += "AND s.mood_type = $" + std::to_string(paramIndex++) + " ";
  }

  auto countResult = [&]() -> drogon::orm::Result {
    if (!userId.empty() && !filterMood.empty()) {
      return dbClient->execSqlSync(countSql, userId, filterMood);
    }
    if (!userId.empty()) {
      return dbClient->execSqlSync(countSql, userId);
    }
    if (!filterMood.empty()) {
      return dbClient->execSqlSync(countSql, filterMood);
    }
    return dbClient->execSqlSync(countSql);
  }();

  if (countResult.empty() || countResult[0]["total_count"].isNull()) {
    return 0;
  }
  return countResult[0]["total_count"].as<int>();
}

std::string serializeRiskFactors(
    const std::vector<heartlake::utils::RiskFactor> &factors) {
  Json::Value payload(Json::arrayValue);
  for (const auto &factor : factors) {
    Json::Value item(Json::objectValue);
    item["category"] = factor.category;
    item["name"] = factor.name;
    item["score"] = factor.score;
    item["weight"] = factor.weight;
    item["description"] = factor.description;
    payload.append(item);
  }

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  return Json::writeString(writer, payload);
}

void attachAuthorPayload(Json::Value &stone, const drogon::orm::Row &row) {
  const bool isAnonymous = safeBool(row, "is_anonymous", true);
  if (isAnonymous || row["username"].isNull()) {
    return;
  }

  Json::Value author;
  const std::string userId = safeString(row, "user_id");
  const std::string username = safeString(row, "username");
  const std::string nickname = safeString(row, "nickname", username);

  author["user_id"] = userId;
  author["id"] = userId;
  author["userId"] = userId;
  author["username"] = username;
  author["nickname"] = nickname;
  if (!row["avatar_url"].isNull()) {
    author["avatar_url"] = row["avatar_url"].as<std::string>();
    author["avatarUrl"] = author["avatar_url"];
  }

  stone["author"] = author;
  stone["user"] = author;
  stone["author_name"] = nickname;
  stone["nickname"] = nickname;
}

Json::Value buildStoneJson(const drogon::orm::Row &row,
                           bool includeHasRippled = false) {
  Json::Value stone;
  stone["stone_id"] = safeString(row, "stone_id");
  stone["user_id"] = safeString(row, "user_id");
  stone["content"] = safeString(row, "content");
  stone["stone_type"] = safeString(row, "stone_type", "medium");
  stone["stone_color"] = safeString(row, "stone_color", "#7A92A3");
  stone["mood_type"] = safeString(row, "mood_type", "calm");
  stone["is_anonymous"] = safeBool(row, "is_anonymous", true);
  stone["created_at"] = safeString(row, "created_at");
  stone["view_count"] = safeInt(row, "view_count", 0);
  stone["ripple_count"] = safeInt(row, "ripple_count", 0);
  stone["boat_count"] = safeInt(row, "boat_count", 0);

  attachAuthorPayload(stone, row);
  if (includeHasRippled) {
    stone["has_rippled"] = safeBool(row, "has_rippled", false);
  }
  return stone;
}

} // namespace

/**
 * 发布石头：
 *   1. INSERT RETURNING 获取完整记录
 *   2. 发布 StonePublishedEvent 触发异步处理管线
 *   3. 本地情感分析 → 记录情绪追踪 → 负面情绪触发自动赠灯判定
 *   4. 失效列表缓存
 */
Json::Value StoneApplicationService::publishStone(
    const std::string &userId, const std::string &content,
    const std::string &stoneType, const std::string &stoneColor,
    const std::string &moodType, bool isAnonymous,
    const std::vector<std::string> &tags) {
  auto dbClient = drogon::app().getDbClient("default");
  std::string stoneId = "stone_" + drogon::utils::getUuid();
  const std::string normalizedMood = normalizeMoodType(moodType);
  const std::string tagsLiteral = toPgTextArrayLiteral(tags);

  try {
    // 创建石头
    auto result = dbClient->execSqlSync(
        "WITH inserted AS ("
        "  INSERT INTO stones (stone_id, user_id, content, stone_type, "
        "stone_color, mood_type, is_anonymous, tags, created_at) "
        "  VALUES ($1, $2, $3, $4, $5, $6, $7, NULLIF($8, '{}')::text[], "
        "NOW()) "
        "  RETURNING stone_id, user_id, content, stone_type, stone_color, "
        "mood_type, is_anonymous, "
        "            created_at, view_count, ripple_count, boat_count"
        ") "
        "SELECT i.stone_id, i.user_id, i.content, i.stone_type, i.stone_color, "
        "       i.mood_type, i.is_anonymous, i.created_at, i.view_count, "
        "i.ripple_count, i.boat_count, "
        "       u.username, u.nickname, u.avatar_url "
        "FROM inserted i "
        "LEFT JOIN users u ON i.user_id = u.user_id",
        stoneId, userId, content, stoneType, stoneColor, normalizedMood,
        isAnonymous, tagsLiteral);

    if (result.empty()) {
      throw std::runtime_error("创建石头失败");
    }

    auto row = *safeRow(result);
    Json::Value stone = buildStoneJson(row);
    stone["has_rippled"] = false;
    Json::Value stoneTags(Json::arrayValue);
    for (const auto &tag : tags) {
      stoneTags.append(tag);
    }
    stone["tags"] = stoneTags;

    // 发布事件 (异步处理情感分析等)
    if (eventBus_) {
      core::events::StonePublishedEvent event;
      event.stoneId = stone["stone_id"].asString();
      event.userId = userId;
      event.content = content;
      event.moodType = normalizedMood;
      eventBus_->publish(event);
    }

    // 记录情绪到追踪服务（用于异常检测和干预）
    try {
      auto &emotionTracker =
          heartlake::infrastructure::EmotionTrackingService::getInstance();
      auto &aiEngine = heartlake::ai::EdgeAIEngine::getInstance();
      auto sentiment = aiEngine.analyzeSentimentLocal(content);
      emotionTracker.recordEmotion(userId, sentiment.score, content);

      // 情绪明显偏负时触发自动赠灯判定（VIPService
      // 内部会做全局20%阈值与重复发放保护）
      if (sentiment.score < -0.1f) {
        heartlake::services::VIPService::checkEmotionAndGrantVIP(
            userId, sentiment.score, {});
      }
    } catch (const std::exception &e) {
      LOG_WARN << "EmotionTracking recordEmotion failed for user: " << userId
               << ": " << e.what();
    }

    // 清除缓存
    if (cacheManager_) {
      cacheManager_->invalidate("user:" + userId);
      invalidateStoneListCaches(cacheManager_);
    }

    return stone;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to publish stone: " << e.base().what();
    throw std::runtime_error("发布石头失败");
  }
}

/// 获取石头详情：cache-aside 策略，has_rippled 每次单独查询（用户相关不可缓存）
Json::Value
StoneApplicationService::getStoneDetail(const std::string &stoneId,
                                        const std::string &currentUserId) {
  // 定义数据库查询函数
  auto fetchFromDb = [&stoneId]() -> Json::Value {
    auto dbClient = drogon::app().getDbClient("default");

    try {
      auto result = dbClient->execSqlSync(
          "SELECT s.stone_id, s.user_id, s.content, s.stone_type, "
          "s.stone_color, "
          "s.mood_type, s.is_anonymous, s.created_at, s.view_count, "
          "s.ripple_count, s.boat_count, "
          "u.username, u.nickname, u.avatar_url "
          "FROM stones s "
          "LEFT JOIN users u ON s.user_id = u.user_id "
          "WHERE s.stone_id = $1 AND s.deleted_at IS NULL",
          stoneId);

      if (result.empty()) {
        throw std::runtime_error("石头不存在");
      }

      auto row = *safeRow(result);
      return buildStoneJson(row);

    } catch (const drogon::orm::DrogonDbException &e) {
      LOG_ERROR << "Failed to get stone detail: " << e.base().what();
      throw std::runtime_error("获取石头详情失败");
    }
  };

  // 尝试从缓存获取
  const std::string cacheKey = buildStoneDetailCacheKey(stoneId);
  Json::Value result;
  bool fromCache = false;
  if (cacheManager_) {
    auto cached = cacheManager_->getJson(cacheKey);
    if (cached) {
      result = *cached;
      fromCache = true;
    }
  }

  if (!fromCache) {
    // 从数据库获取
    result = fetchFromDb();

    // 缓存结果
    if (cacheManager_) {
      cacheManager_->setJson(cacheKey, result, kStoneDetailCacheTtlSeconds);
    }
  }

  try {
    const auto states =
        loadRippleStates(cacheManager_, currentUserId, std::vector<std::string>{stoneId});
    auto it = states.find(stoneId);
    result["has_rippled"] = it != states.end() ? it->second : false;
  } catch (const std::exception &e) {
    LOG_WARN << "Check has_rippled failed: " << e.what();
    result["has_rippled"] = false;
  }

  return result;
}

/// 石头列表：支持按用户/情绪过滤、多种排序方式、分页，附带 has_rippled 状态
Json::Value StoneApplicationService::getStoneList(
    int page, int pageSize, const std::string &sortBy,
    const std::string &filterMood, const std::string &userId,
    const std::string &currentUserId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    const int offset = (page - 1) * pageSize;
    const std::string normalizedSort = normalizeStoneSort(sortBy);
    const std::string cacheKey =
        buildStoneListCacheKey(page, pageSize, normalizedSort, filterMood, userId);
    Json::Value response;

    if (cacheManager_) {
      auto cached = cacheManager_->getJson(cacheKey);
      if (cached) {
        response = *cached;
      }
    }

    if (response.isNull()) {
      int paramIndex = 1;
      std::string sql =
          "SELECT s.stone_id, s.user_id, s.content, s.stone_type, "
          "s.stone_color, "
          "s.mood_type, s.is_anonymous, s.created_at, s.view_count, "
          "s.ripple_count, s.boat_count, "
          "u.username, u.nickname, u.avatar_url, COUNT(*) OVER() AS total_count "
          "FROM stones s "
          "LEFT JOIN users u ON s.user_id = u.user_id "
          "WHERE s.deleted_at IS NULL AND s.status = 'published' ";

      if (!userId.empty()) {
        sql += "AND s.user_id = $" + std::to_string(paramIndex++) + " ";
      }

      if (!filterMood.empty()) {
        sql += "AND s.mood_type = $" + std::to_string(paramIndex++) + " ";
      }

      sql += "ORDER BY ";
      if (normalizedSort == "view_count") {
        sql += "s.view_count DESC ";
      } else if (normalizedSort == "boat_count") {
        sql += "s.boat_count DESC ";
      } else if (normalizedSort == "ripple_count") {
        sql += "s.ripple_count DESC ";
      } else {
        sql += "s.created_at DESC ";
      }

      sql += "LIMIT $" + std::to_string(paramIndex++) + " ";
      sql += "OFFSET $" + std::to_string(paramIndex);
      auto result = [&]() -> drogon::orm::Result {
        if (!userId.empty() && !filterMood.empty()) {
          return dbClient->execSqlSync(sql, userId, filterMood,
                                       static_cast<int64_t>(pageSize), offset);
        }
        if (!userId.empty()) {
          return dbClient->execSqlSync(sql, userId, static_cast<int64_t>(pageSize),
                                       offset);
        }
        if (!filterMood.empty()) {
          return dbClient->execSqlSync(sql, filterMood,
                                       static_cast<int64_t>(pageSize), offset);
        }
        return dbClient->execSqlSync(
            sql, static_cast<int64_t>(pageSize), offset);
      }();

      Json::Value stones(Json::arrayValue);
      int total = 0;
      for (const auto &row : result) {
        if (total == 0 && !row["total_count"].isNull()) {
          total = row["total_count"].as<int>();
        }
        stones.append(buildStoneJson(row));
      }
      total = resolveStoneListTotal(dbClient, result, page, userId, filterMood);
      response = ResponseUtil::buildCollectionPayload("stones", stones, total, page,
                                                      pageSize);

      if (cacheManager_) {
        cacheManager_->setJson(cacheKey, response, kStoneListCacheTtlSeconds);
      }
    }

    auto stones = response["stones"];
    const auto states =
        loadRippleStates(cacheManager_, currentUserId, collectStoneIds(stones));
    applyRippleStates(stones, states);
    response["stones"] = stones;
    syncStoneCollectionAliases(response);
    return response;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to get stone list: " << e.base().what();
    throw std::runtime_error("获取石头列表失败");
  }
}

/// 软删除石头：校验所有权后标记 deleted_at，同时失效缓存
void StoneApplicationService::deleteStone(const std::string &stoneId,
                                          const std::string &userId) {
  auto dbClient = drogon::app().getDbClient("default");

  try {
    // 软删除
    auto result = dbClient->execSqlSync(
        "UPDATE stones SET deleted_at = NOW() "
        "WHERE stone_id = $1 AND user_id = $2 AND deleted_at IS NULL",
        stoneId, userId);

    if (result.affectedRows() == 0) {
      throw std::runtime_error("石头不存在或无权删除");
    }

    // 清除缓存
    if (cacheManager_) {
      cacheManager_->invalidate(buildStoneDetailCacheKey(stoneId));
      cacheManager_->invalidate("user:" + userId);
      invalidateStoneListCaches(cacheManager_);
    }

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Failed to delete stone: " << e.base().what();
    throw std::runtime_error("删除石头失败");
  }
}

/// 浏览量递增：Redis INCR 聚合，每累积 10 次批量写回数据库，降低 DB 写压力
void StoneApplicationService::incrementViewCount(const std::string &stoneId) {
  // 使用Redis聚合浏览量，带持久化保障
  auto &redis = cache::RedisCache::getInstance();
  const std::string viewKey = "view_count:" + stoneId;
  auto cacheManagerCopy = cacheManager_;

  if (!redis.isConnected()) {
    // Redis不可用时直接写数据库
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "UPDATE stones SET view_count = view_count + 1 WHERE stone_id = $1",
        [stoneId, cacheManagerCopy](const drogon::orm::Result &) {
          if (cacheManagerCopy) {
            cacheManagerCopy->invalidate(buildStoneDetailCacheKey(stoneId));
            invalidateStoneListCaches(cacheManagerCopy, "view_count");
          }
        },
        [](const drogon::orm::DrogonDbException &) {}, stoneId);
    return;
  }

  redis.incr(viewKey, [stoneId, cacheManagerCopy](int64_t count) {
    if (count > 0 && count % 10 == 0) {
      auto dbClient = drogon::app().getDbClient("default");
      dbClient->execSqlAsync(
          "UPDATE stones SET view_count = view_count + 10 WHERE stone_id = $1",
          [stoneId, cacheManagerCopy](const drogon::orm::Result &) {
            if (cacheManagerCopy) {
              cacheManagerCopy->invalidate(buildStoneDetailCacheKey(stoneId));
              invalidateStoneListCaches(cacheManagerCopy, "view_count");
            }
          },
          [](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Failed to batch update view count: "
                      << e.base().what();
          },
          stoneId);
    }
  });
}

/**
 * 石头发布后的异步处理管线（由事件驱动触发）：
 *   管线 1: 情感分析 → 更新 DB → 社区脉搏 → 心理风险评估 → 暖心语录
 *   管线 2: 向量嵌入 → 持久化 stone_embeddings → HNSW 索引插入
 *   管线 3: 长文本自动摘要
 *   管线 4: DualMemoryRAG 生成 AI 暖心评论 → WebSocket 广播
 */
void StoneApplicationService::processStoneAsync(const std::string &stoneId,
                                                const std::string &userId,
                                                const std::string &content,
                                                const std::string &moodType) {
  auto &aiService = heartlake::ai::AIService::getInstance();

  // 1. 情感分析 + 风险评估 + 通知
  auto cacheManagerCopy = cacheManager_;
  aiService.analyzeSentiment(content, [stoneId, userId, content, cacheManagerCopy](
                                          float score, const std::string &mood,
                                          const std::string &error) {
    if (!error.empty()) {
      LOG_WARN << "AI sentiment analysis failed for stone " << stoneId << ": "
               << error;
      return;
    }
    const std::string normalizedMood = normalizeMoodType(mood);
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "UPDATE stones SET emotion_score = $1, mood_type = $2, updated_at = "
        "NOW() WHERE stone_id = $3",
        [stoneId, cacheManagerCopy](const drogon::orm::Result &) {
          if (cacheManagerCopy) {
            cacheManagerCopy->invalidate(buildStoneDetailCacheKey(stoneId));
            invalidateStoneListCaches(cacheManagerCopy);
          }
        },
        [stoneId](const drogon::orm::DrogonDbException &e) {
          LOG_ERROR << "Failed to update emotion for stone " << stoneId << ": "
                    << e.base().what();
        },
        score, normalizedMood, stoneId);

    // 提交情绪样本到社区脉搏
    try {
      auto &edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
      if (edgeEngine.isEnabled()) {
        edgeEngine.submitEmotionSample(score, normalizedMood);
      }
    } catch (const std::exception &e) {
      LOG_WARN << "submitEmotionSample failed: " << e.what();
    }

    // 心理风险评估
    auto &riskAssessment =
        heartlake::utils::PsychologicalRiskAssessment::getInstance();
    auto riskResult =
        riskAssessment.assessRisk(content, userId, score, normalizedMood);

    if (riskResult.needsImmediateAttention) {
      auto &pushService =
          heartlake::services::NotificationPushService::getInstance();
      pushService.pushSystemNotice(userId, "心理健康关怀",
                                   riskResult.supportMessage);

      try {
        const auto keywordsLiteral = toPgTextArrayLiteral(riskResult.keywords);
        const auto factorsJson = serializeRiskFactors(riskResult.factors);
        auto trans = db->newTransaction();
        auto assessmentResult = trans->execSqlSync(
            "INSERT INTO psychological_assessments "
            "(user_id, content_id, content_type, risk_level, risk_score, "
            "primary_concern, needs_immediate_attention, keywords, factors, "
            "support_message, created_at) "
            "VALUES ($1, $2, 'stone', $3, $4, $5, $6, "
            "NULLIF($7, '{}')::text[], $8::jsonb, $9, NOW()) "
            "RETURNING assessment_id",
            userId, stoneId, static_cast<int>(riskResult.riskLevel),
            riskResult.overallScore, riskResult.primaryConcern,
            riskResult.needsImmediateAttention, keywordsLiteral, factorsJson,
            riskResult.supportMessage);

        const auto assessmentId =
            assessmentResult.empty() ? 0 : assessmentResult[0]["assessment_id"].as<int64_t>();

        if (static_cast<int>(riskResult.riskLevel) >=
            static_cast<int>(heartlake::utils::RiskLevel::HIGH)) {
          if (assessmentId == 0) {
            trans->execSqlSync(
                "INSERT INTO high_risk_events "
                "(user_id, content_id, content_type, risk_level, risk_score, "
                "intervention_sent, admin_notified, status, created_at) "
                "VALUES ($1, $2, 'stone', $3, $4, $5, false, 'pending', NOW())",
                userId, stoneId, static_cast<int>(riskResult.riskLevel),
                riskResult.overallScore, !riskResult.supportMessage.empty());
          } else {
            trans->execSqlSync(
                "INSERT INTO high_risk_events "
                "(user_id, content_id, content_type, risk_level, risk_score, "
                "intervention_sent, admin_notified, status, assessment_id, created_at) "
                "VALUES ($1, $2, 'stone', $3, $4, $5, false, 'pending', $6, NOW())",
                userId, stoneId, static_cast<int>(riskResult.riskLevel),
                riskResult.overallScore, !riskResult.supportMessage.empty(),
                assessmentId);
          }
        }
      } catch (const std::exception &e) {
        LOG_ERROR << "Failed to persist psychological assessment for stone "
                  << stoneId << ": " << e.what();
      }

      if (riskResult.riskLevel == heartlake::utils::RiskLevel::CRITICAL) {
        db->execSqlAsync(
            "SELECT id AS admin_id FROM admin_users "
            "WHERE role IN ('admin', 'moderator', 'super_admin')",
            [userId, stoneId](const drogon::orm::Result &r) {
              auto &pushSvc =
                  heartlake::services::NotificationPushService::getInstance();
              for (const auto &row : r) {
                std::string adminId = row["admin_id"].as<std::string>();
                pushSvc.pushSystemNotice(
                    adminId, "⚠️ 危机预警",
                    "用户 " + userId + " 发布的石头（ID: " + stoneId +
                        "）检测到危机级别心理风险，请及时关注。");
              }
            },
            [](const drogon::orm::DrogonDbException &e) {
              LOG_ERROR << "Failed to notify administrators: "
                        << e.base().what();
            });
      }
    }

    // 暖心语录
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf;
    localtime_r(&time_t_now, &tm_buf);
    int hour = tm_buf.tm_hour;
    auto scene = heartlake::infrastructure::WarmQuoteService::detectScene(
        hour, false, false, 0, 1, 0, score);
    if (scene != heartlake::infrastructure::WarmScene::Default) {
      auto msg = heartlake::infrastructure::WarmQuoteService::getInstance()
                     .getQuoteForScene(scene);
      heartlake::services::NotificationPushService::getInstance()
          .pushSystemNotice(userId, msg.title, msg.content);
    }
  });

  // 2. 向量嵌入
  aiService.generateEmbedding(content, [stoneId](
                                           const std::vector<float> &embedding,
                                           const std::string &error) {
    if (!error.empty() || embedding.empty()) {
      LOG_WARN << "Embedding generation failed for stone " << stoneId << ": "
               << error;
      return;
    }
    std::string vecStr = "[";
    for (size_t i = 0; i < embedding.size(); ++i) {
      vecStr += std::to_string(embedding[i]);
      if (i < embedding.size() - 1)
        vecStr += ",";
    }
    vecStr += "]";

    drogon::app().getDbClient("default")->execSqlAsync(
        // stone_embeddings 才是向量持久化表，避免误写 stones 导致连接进入 abort
        // pipeline。
        "INSERT INTO stone_embeddings (stone_id, embedding, created_at) "
        "VALUES ($1, $2, NOW()) "
        "ON CONFLICT (stone_id) DO UPDATE SET embedding = EXCLUDED.embedding",
        [](const drogon::orm::Result &) {},
        [stoneId](const drogon::orm::DrogonDbException &e) {
          LOG_ERROR << "Failed to persist embedding for stone " << stoneId
                    << ": " << e.base().what();
        },
        stoneId, vecStr);

    // 同步插入HNSW索引，使向量搜索可用
    try {
      auto &edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
      if (edgeEngine.isEnabled()) {
        edgeEngine.hnswInsert(stoneId, embedding);
      }
    } catch (const std::exception &e) {
      LOG_WARN << "HNSW insert failed for stone " << stoneId << ": "
               << e.what();
    }
  });

  // 3. 长文本自动摘要
  if (content.size() >= heartlake::ai::SummaryService::MIN_LENGTH) {
    heartlake::ai::SummaryService::getInstance().generateSummary(
        stoneId, content,
        [stoneId](const std::string &summary, const std::string &error) {
          if (!error.empty()) {
            LOG_WARN << "Summary generation failed for stone " << stoneId
                     << ": " << error;
            return;
          }
          drogon::app().getDbClient("default")->execSqlAsync(
              "UPDATE stones SET summary = $1 WHERE stone_id = $2",
              [](const drogon::orm::Result &) {},
              [stoneId](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "Failed to update summary for stone " << stoneId
                          << ": " << e.base().what();
              },
              summary, stoneId);
        });
  }

  // 4. AI暖心评论（基于双记忆RAG）
  // 使用DualMemoryRAG生成个性化回复，融合用户长期情绪画像和近期交互上下文
  auto asyncCacheManager = cacheManager_;
  drogon::async_run([stoneId, userId, content, moodType,
                     asyncCacheManager]() -> drogon::Task<void> {
    try {
      auto &dualMemory = heartlake::ai::DualMemoryRAG::getInstance();
      std::string comment = dualMemory.generateResponse(
          userId, content, normalizeMoodType(moodType), 0.0f);

      if (comment.empty()) {
        LOG_WARN << "DualMemoryRAG returned empty for stone " << stoneId;
        co_return;
      }

      std::string aiBoatId = heartlake::utils::IdGenerator::generateBoatId();
      auto db = drogon::app().getDbClient("default");
      auto transPtr = db->newTransaction();
      transPtr->execSqlSync(
          "INSERT INTO paper_boats (boat_id, stone_id, sender_id, content, "
          "is_anonymous, status, created_at) "
          "VALUES ($1, $2, 'ai_lakegod', $3, false, 'active', NOW())",
          aiBoatId, stoneId, comment);
      auto updateResult = transPtr->execSqlSync(
          "UPDATE stones SET boat_count = boat_count + 1, updated_at = NOW() "
          "WHERE stone_id = $1 RETURNING boat_count",
          stoneId);
      int newBoatsCount =
          updateResult.empty() ? 1 : updateResult[0]["boat_count"].as<int>();

      if (asyncCacheManager) {
        asyncCacheManager->invalidate(buildStoneDetailCacheKey(stoneId));
        invalidateStoneListCaches(asyncCacheManager);
      }

      Json::Value broadcastMsg;
      broadcastMsg["type"] = "boat_update";
      broadcastMsg["stone_id"] = stoneId;
      broadcastMsg["boat_count"] = newBoatsCount;
      broadcastMsg["boat_content"] = comment;
      broadcastMsg["boat_id"] = aiBoatId;
      broadcastMsg["is_ai"] = true;
      broadcastMsg["triggered_by"] = "ai_lakegod_rag";
      broadcastMsg["timestamp"] = static_cast<Json::Int64>(time(nullptr));

      drogon::app().getLoop()->queueInLoop(
          [stoneId, msg = std::move(broadcastMsg)]() mutable {
            heartlake::controllers::BroadcastWebSocketController::sendToRoom(
                "stone:" + stoneId, msg);
          });
    } catch (const std::exception &e) {
      LOG_ERROR << "DualMemoryRAG comment creation failed: " << e.what();
    }
    co_return;
  });
}

} // namespace application
} // namespace heartlake
