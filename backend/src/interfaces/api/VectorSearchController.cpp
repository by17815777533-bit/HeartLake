/**
 * VectorSearchController 模块实现
 */
#include "interfaces/api/VectorSearchController.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "infrastructure/ai/EdgeAIEngine.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <json/json.h>
#include <algorithm>
#include <vector>

using namespace heartlake::utils;

namespace heartlake {
namespace controllers {

namespace {

std::string serializeEmbedding(const std::vector<float> &embedding) {
    std::string vectorStr;
    vectorStr.reserve(embedding.size() * 16 + 2);
    vectorStr.push_back('[');
    for (size_t i = 0; i < embedding.size(); ++i) {
        if (i > 0) {
            vectorStr.push_back(',');
        }
        vectorStr += std::to_string(embedding[i]);
    }
    vectorStr.push_back(']');
    return vectorStr;
}

void syncEmbeddingToEdgeIndex(const std::string &stoneId,
                              const std::vector<float> &embedding) {
    auto &edgeEngine = heartlake::ai::EdgeAIEngine::getInstance();
    if (!edgeEngine.isEnabled() || embedding.empty()) {
        return;
    }

    edgeEngine.hnswRemove(stoneId);
    edgeEngine.hnswInsert(stoneId, embedding);
}

} // namespace

void VectorSearchController::getSimilarStones(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &stoneId) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录，请先登录"));
        return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    const int limit = std::clamp(safeInt(req->getParameter("limit"), 10), 1, 50);

    dbClient->execSqlAsync(
        "SELECT s.stone_id, s.embedding "
        "FROM stones s "
        "WHERE s.stone_id = $1 AND s.status = 'published' AND s.deleted_at IS NULL",
        [callback, stoneId, limit](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(ResponseUtil::notFound("石头不存在"));
                return;
            }

            if (r[0]["embedding"].isNull()) {
                callback(ResponseUtil::conflict("石头缺少向量索引，无法进行相似搜索"));
                return;
            }

            auto dbClient = drogon::app().getDbClient("default");
            dbClient->execSqlAsync(
                "SELECT s.stone_id, s.content, s.mood_type, s.ripple_count, s.boat_count, "
                "s.created_at, u.nickname, u.username, "
                "1 - (s.embedding <=> (SELECT embedding FROM stones WHERE stone_id = $1)) as similarity "
                "FROM stones s "
                "LEFT JOIN users u ON s.user_id = u.user_id "
                "WHERE s.status = 'published' AND s.deleted_at IS NULL AND s.stone_id != $1 "
                "AND s.embedding IS NOT NULL "
                "ORDER BY s.embedding <=> (SELECT embedding FROM stones WHERE stone_id = $1) "
                "LIMIT $2",
                [callback](const drogon::orm::Result &res) {
                    Json::Value resp;
                    resp["code"] = 0;
                    resp["message"] = "成功";

                    Json::Value data;
                    Json::Value stones = Json::arrayValue;

                    for (const auto &dbRow : res) {
                        Json::Value stone;
                        stone["stone_id"] = dbRow["stone_id"].as<std::string>();
                        stone["content"] = dbRow["content"].as<std::string>();
                        stone["mood"] = dbRow["mood_type"].isNull()
                            ? "neutral"
                            : dbRow["mood_type"].as<std::string>();
                        stone["ripple_count"] = dbRow["ripple_count"].as<int>();
                        stone["boat_count"] = dbRow["boat_count"].as<int>();
                        stone["created_at"] = dbRow["created_at"].as<std::string>();
                        stone["nickname"] = dbRow["nickname"].isNull()
                            ? ""
                            : dbRow["nickname"].as<std::string>();
                        stone["similarity"] = dbRow["similarity"].isNull()
                            ? 0.0f
                            : dbRow["similarity"].as<float>();
                        stones.append(stone);
                    }

                    data["stones"] = stones;
                    data["total"] = static_cast<int>(stones.size());
                    data["method"] = "vector_search";
                    resp["data"] = data;

                    callback(HttpResponse::newHttpJsonResponse(resp));
                },
                [callback, stoneId](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Vector search failed for stone " << stoneId
                              << ": " << e.base().what();
                    callback(ResponseUtil::internalError("向量搜索失败"));
                },
                stoneId, limit);
        },
        [callback, stoneId](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Failed to load vector source stone " << stoneId
                      << ": " << e.base().what();
            callback(ResponseUtil::internalError("读取石头向量失败"));
        },
        stoneId);
}

void VectorSearchController::getPersonalizedRecommendations(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        Json::Value resp;
        resp["code"] = 401;
        resp["message"] = "未登录";
        auto httpResp = HttpResponse::newHttpJsonResponse(resp);
        httpResp->setStatusCode(k401Unauthorized);
        callback(httpResp);
        return;
    }
    auto &userId = *userIdOpt;

    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT s.stone_id, s.content, s.mood_type, s.ripple_count, s.boat_count, "
        "s.created_at, u.nickname, u.username "
        "FROM stones s "
        "LEFT JOIN users u ON s.user_id = u.user_id "
        "WHERE s.user_id != $1 AND s.status = 'published' AND s.deleted_at IS NULL "
        "ORDER BY s.created_at DESC "
        "LIMIT 20",
        [callback](const drogon::orm::Result &r) {
            Json::Value resp;
            resp["code"] = 0;
            resp["message"] = "成功";

            Json::Value data;
            Json::Value stones = Json::arrayValue;

            for (const auto &row : r) {
                Json::Value stone;
                stone["stone_id"] = row["stone_id"].as<std::string>();
                stone["content"] = row["content"].as<std::string>();
                stone["mood"] = row["mood_type"].isNull()
                    ? "neutral"
                    : row["mood_type"].as<std::string>();
                stone["ripple_count"] = row["ripple_count"].as<int>();
                stone["boat_count"] = row["boat_count"].as<int>();
                stone["created_at"] = row["created_at"].as<std::string>();
                stone["nickname"] = row["nickname"].isNull()
                    ? ""
                    : row["nickname"].as<std::string>();
                stones.append(stone);
            }

            data["stones"] = stones;
            data["total"] = static_cast<int>(stones.size());
            resp["data"] = data;

            callback(HttpResponse::newHttpJsonResponse(resp));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Error in getPersonalizedRecommendations: "
                      << e.base().what();
            Json::Value resp;
            resp["code"] = 500;
            resp["message"] = "查询失败";
            auto httpResp = HttpResponse::newHttpJsonResponse(resp);
            httpResp->setStatusCode(k500InternalServerError);
            callback(httpResp);
        },
        userId);
}

void VectorSearchController::updateStoneEmbedding(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &stoneId) {
    auto userIdOpt2 = Validator::getUserId(req);
    if (!userIdOpt2) {
        Json::Value resp;
        resp["code"] = 401;
        resp["message"] = "未登录";
        auto httpResp = HttpResponse::newHttpJsonResponse(resp);
        httpResp->setStatusCode(k401Unauthorized);
        callback(httpResp);
        return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT content FROM stones WHERE stone_id = $1",
        [callback, dbClient, stoneId](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(ResponseUtil::notFound("石头不存在"));
                return;
            }

            const std::string content = r[0]["content"].as<std::string>();
            auto &embeddingEngine =
                heartlake::ai::AdvancedEmbeddingEngine::getInstance();
            auto embedding = embeddingEngine.generateEmbedding(content);
            if (embedding.empty()) {
                callback(ResponseUtil::internalError("向量生成失败"));
                return;
            }

            const std::string vectorStr = serializeEmbedding(embedding);

            try {
                auto trans = dbClient->newTransaction();
                trans->execSqlSync(
                    "INSERT INTO stone_embeddings (stone_id, embedding, created_at) "
                    "VALUES ($1, $2, NOW()) "
                    "ON CONFLICT (stone_id) DO UPDATE SET embedding = EXCLUDED.embedding, created_at = NOW()",
                    stoneId, vectorStr);
                trans->execSqlSync(
                    "UPDATE stones SET embedding = $1::vector WHERE stone_id = $2",
                    vectorStr, stoneId);

                syncEmbeddingToEdgeIndex(stoneId, embedding);

                Json::Value resp;
                resp["code"] = 0;
                resp["message"] = "向量嵌入更新成功";
                Json::Value data;
                data["method"] = "pgvector_and_stone_embeddings";
                resp["data"] = data;
                callback(HttpResponse::newHttpJsonResponse(resp));
            } catch (const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "Failed to update embeddings for stone " << stoneId
                          << ": " << e.base().what();
                callback(ResponseUtil::internalError("更新向量索引失败"));
            } catch (const std::exception &e) {
                LOG_ERROR << "Failed to sync embeddings for stone " << stoneId
                          << ": " << e.what();
                callback(ResponseUtil::internalError("更新向量索引失败"));
            }
        },
        [callback, stoneId](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Failed to load stone " << stoneId
                      << " for embedding update: " << e.base().what();
            callback(ResponseUtil::internalError("读取石头失败"));
        },
        stoneId);
}

} // namespace controllers
} // namespace heartlake
