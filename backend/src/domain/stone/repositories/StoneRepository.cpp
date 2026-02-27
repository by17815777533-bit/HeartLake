/**
 * 石头仓储实现
 */

#include "domain/stone/repositories/StoneRepository.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include <set>

namespace heartlake::domain::stone {
using namespace heartlake::utils;

StoneEntity StoneRepository::rowToEntity(const drogon::orm::Row& row) {
    StoneEntity entity;
    entity.stoneId = row["stone_id"].as<std::string>();
    entity.userId = row["user_id"].as<std::string>();
    entity.content = row["content"].as<std::string>();
    entity.stoneType = row["stone_type"].as<std::string>();
    entity.stoneColor = row["stone_color"].as<std::string>();
    entity.moodType = row["mood_type"].as<std::string>();
    entity.isAnonymous = row["is_anonymous"].as<bool>();
    entity.ripplesCount = row["ripple_count"].as<int>();
    entity.boatsCount = row["boat_count"].as<int>();
    entity.viewsCount = row["view_count"].as<int>();
    entity.status = row["status"].as<std::string>();
    entity.createdAt = row["created_at"].as<std::string>();
    return entity;
}

StoneEntity StoneRepository::save(const StoneEntity& stone) {
    auto db = drogon::app().getDbClient("default");
    std::string stoneId = stone.stoneId.empty() ? utils::IdGenerator::generateStoneId() : stone.stoneId;

    db->execSqlSync(
        "INSERT INTO stones (stone_id, user_id, content, stone_type, stone_color, "
        "mood_type, is_anonymous, status, ripple_count, boat_count, view_count,"
        "created_at, updated_at) VALUES ($1, $2, $3, $4, $5, $6, $7, 'published', 0, 0, 0, NOW(), NOW())",
        stoneId, stone.userId, stone.content, stone.stoneType, stone.stoneColor,
        stone.moodType, stone.isAnonymous
    );

    StoneEntity saved = stone;
    saved.stoneId = stoneId;
    saved.status = "published";
    return saved;
}

std::optional<StoneEntity> StoneRepository::findById(const std::string& stoneId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT * FROM stones WHERE stone_id = $1 AND status = 'published' AND deleted_at IS NULL", stoneId
    );
    auto row = safeRow(result);
    if (!row) return std::nullopt;
    return rowToEntity(*row);
}

std::vector<StoneEntity> StoneRepository::findByUserId(const std::string& userId, int page, int pageSize) {
    auto db = drogon::app().getDbClient("default");
    int offset = (page - 1) * pageSize;
    auto result = db->execSqlSync(
        "SELECT * FROM stones WHERE user_id = $1 AND status = 'published' AND deleted_at IS NULL "
        "ORDER BY created_at DESC LIMIT $2 OFFSET $3",
        userId, pageSize, offset
    );
    std::vector<StoneEntity> stones;
    for (const auto& row : result) {
        stones.push_back(rowToEntity(row));
    }
    return stones;
}

std::vector<StoneEntity> StoneRepository::findAll(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) {
    auto db = drogon::app().getDbClient("default");
    int offset = (page - 1) * pageSize;

    // Whitelist sortBy to prevent SQL injection
    static const std::set<std::string> allowedSortColumns = {"created_at", "ripple_count", "boat_count", "view_count"};
    std::string safeSortBy = allowedSortColumns.count(sortBy) ? sortBy : "created_at";

    if (!filterMood.empty()) {
        auto result = db->execSqlSync(
            "SELECT * FROM stones WHERE status = 'published' AND deleted_at IS NULL AND mood_type = $1 ORDER BY " + safeSortBy + " DESC LIMIT $2 OFFSET $3",
            filterMood, pageSize, offset);
        std::vector<StoneEntity> stones;
        for (const auto& row : result) stones.push_back(rowToEntity(row));
        return stones;
    }
    auto result = db->execSqlSync(
        "SELECT * FROM stones WHERE status = 'published' AND deleted_at IS NULL ORDER BY " + safeSortBy + " DESC LIMIT $1 OFFSET $2",
        pageSize, offset);
    std::vector<StoneEntity> stones;
    for (const auto& row : result) stones.push_back(rowToEntity(row));
    return stones;
}

int StoneRepository::countAll(const std::string& filterMood) {
    auto db = drogon::app().getDbClient("default");
    if (!filterMood.empty()) {
        auto result = db->execSqlSync("SELECT COUNT(*) as total FROM stones WHERE status = 'published' AND deleted_at IS NULL AND mood_type = $1", filterMood);
        return safeCount(result);
    }
    auto result = db->execSqlSync("SELECT COUNT(*) as total FROM stones WHERE status = 'published' AND deleted_at IS NULL");
    return safeCount(result);
}

int StoneRepository::countByUserId(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT COUNT(*) as total FROM stones WHERE user_id = $1 AND status = 'published' AND deleted_at IS NULL", userId
    );
    return safeCount(result);
}

void StoneRepository::deleteById(const std::string& stoneId) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlSync("UPDATE stones SET status = 'deleted', deleted_at = NOW(), updated_at = NOW() WHERE stone_id = $1", stoneId);
}

void StoneRepository::incrementViewCount(const std::string& stoneId) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "UPDATE stones SET view_count = view_count + 1 WHERE stone_id = $1",
        [](const drogon::orm::Result&) {},
        [stoneId](const drogon::orm::DrogonDbException& e) {
            LOG_WARN << "Failed to increment view_count for stone " << stoneId << ": " << e.base().what();
        },
        stoneId
    );
}

void StoneRepository::updateEmotionScore(const std::string& stoneId, float score, const std::string& mood) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "UPDATE stones SET emotion_score = $1, mood_type = $2, updated_at = NOW() WHERE stone_id = $3",
        [](const drogon::orm::Result&) {},
        [stoneId](const drogon::orm::DrogonDbException& e) {
            LOG_WARN << "Failed to update emotion_score for stone " << stoneId << ": " << e.base().what();
        },
        score, mood, stoneId
    );
}

} // namespace heartlake::domain::stone
