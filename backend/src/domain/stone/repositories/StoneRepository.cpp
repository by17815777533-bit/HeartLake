/**
 * @file StoneRepository.cpp
 * @brief 石头仓储实现 —— 封装 stones 表的持久化操作
 *
 * 核心职责：
 *   - CRUD：创建/查询/软删除石头
 *   - 分页查询：支持按时间、涟漪数、纸船数、浏览数排序
 *   - 计数器：异步递增浏览量、更新情感分数
 *
 * 排序字段使用白名单校验，防止 SQL 注入。
 * 删除采用软删除（status='deleted' + deleted_at 时间戳）。
 */

#include "domain/stone/repositories/StoneRepository.h"
#include "utils/IdGenerator.h"
#include "utils/RequestHelper.h"
#include <set>

namespace heartlake::domain::stone {
using namespace heartlake::utils;

/// 将数据库行映射为 StoneEntity 领域对象
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

/// 持久化石头到数据库，若 stoneId 为空则自动生成
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

/// 按 stone_id 查找已发布且未删除的石头
std::optional<StoneEntity> StoneRepository::findById(const std::string& stoneId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT * FROM stones WHERE stone_id = $1 AND status = 'published' AND deleted_at IS NULL", stoneId
    );
    auto row = safeRow(result);
    if (!row) return std::nullopt;
    return rowToEntity(*row);
}

/// 查询指定用户的石头列表，按创建时间倒序分页
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

/// 全量分页查询，支持排序字段和情绪类型过滤
/// sortBy 经白名单校验，防止拼接注入
std::vector<StoneEntity> StoneRepository::findAll(int page, int pageSize, const std::string& sortBy, const std::string& filterMood) {
    auto db = drogon::app().getDbClient("default");
    int offset = (page - 1) * pageSize;

    // Whitelist sortBy to prevent SQL injection
    static const std::set<std::string> allowedSortColumns = {"created_at", "ripple_count", "boat_count", "view_count"};
    std::string safeSortBy = allowedSortColumns.count(sortBy) ? sortBy : "created_at";

    // 有情绪过滤条件时走参数化查询分支
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

/// 统计已发布石头总数，可选按情绪类型过滤
int StoneRepository::countAll(const std::string& filterMood) {
    auto db = drogon::app().getDbClient("default");
    if (!filterMood.empty()) {
        auto result = db->execSqlSync("SELECT COUNT(*) as total FROM stones WHERE status = 'published' AND deleted_at IS NULL AND mood_type = $1", filterMood);
        return safeCount(result);
    }
    auto result = db->execSqlSync("SELECT COUNT(*) as total FROM stones WHERE status = 'published' AND deleted_at IS NULL");
    return safeCount(result);
}

/// 统计指定用户的已发布石头数
int StoneRepository::countByUserId(const std::string& userId) {
    auto db = drogon::app().getDbClient("default");
    auto result = db->execSqlSync(
        "SELECT COUNT(*) as total FROM stones WHERE user_id = $1 AND status = 'published' AND deleted_at IS NULL", userId
    );
    return safeCount(result);
}

/// 软删除：将 status 置为 'deleted' 并记录删除时间
void StoneRepository::deleteById(const std::string& stoneId) {
    auto db = drogon::app().getDbClient("default");
    db->execSqlSync("UPDATE stones SET status = 'deleted', deleted_at = NOW(), updated_at = NOW() WHERE stone_id = $1", stoneId);
}

/// 异步递增浏览量，fire-and-forget 不阻塞请求处理
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

/// 异步更新情感分析结果（分数 + 情绪标签）
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
