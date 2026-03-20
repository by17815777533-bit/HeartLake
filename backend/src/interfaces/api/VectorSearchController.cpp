/**
 * VectorSearchController 模块实现
 */
#include "interfaces/api/VectorSearchController.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include "utils/RequestHelper.h"
#include "utils/ResponseUtil.h"
#include "utils/Validator.h"
#include <json/json.h>
#include <algorithm>
#include <sstream>
#include <vector>

using namespace heartlake::utils;

namespace heartlake {
namespace controllers {

namespace {

std::vector<float> parseLegacyEmbedding(const std::string& raw) {
    std::vector<float> result;
    std::string normalized = raw;
    normalized.erase(std::remove(normalized.begin(), normalized.end(), '['), normalized.end());
    normalized.erase(std::remove(normalized.begin(), normalized.end(), ']'), normalized.end());

    std::stringstream ss(normalized);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) continue;
        try {
            result.push_back(std::stof(token));
        } catch (...) {
            return {};
        }
    }
    return result;
}

void respondWithLegacySimilarityFallback(
    const drogon::orm::DbClientPtr& dbClient,
    const std::string& stoneId,
    const std::string& content,
    std::function<void(const HttpResponsePtr &)>&& callback,
    const std::string& fallbackReason
) {
    auto& embeddingEngine = heartlake::ai::AdvancedEmbeddingEngine::getInstance();
    const auto queryEmbedding = embeddingEngine.generateEmbedding(content);
    if (queryEmbedding.empty()) {
        callback(ResponseUtil::internalError("向量计算失败"));
        return;
    }

    dbClient->execSqlAsync(
        "SELECT s.stone_id, s.content, s.mood_type, s.ripple_count, s.boat_count, "
        "s.created_at, u.nickname, u.username, se.embedding "
        "FROM stone_embeddings se "
        "JOIN stones s ON s.stone_id = se.stone_id "
        "LEFT JOIN users u ON s.user_id = u.user_id "
        "WHERE s.status = 'published' AND s.deleted_at IS NULL AND s.stone_id != $1",
        [callback = std::move(callback), queryEmbedding, fallbackReason](const drogon::orm::Result& res) mutable {
            struct Candidate {
                Json::Value stone;
                float similarity = 0.0f;
            };

            std::vector<Candidate> candidates;
            candidates.reserve(res.size());

            for (const auto& dbRow : res) {
                if (dbRow["embedding"].isNull()) continue;
                const auto targetEmbedding = parseLegacyEmbedding(dbRow["embedding"].as<std::string>());
                if (targetEmbedding.size() != queryEmbedding.size()) continue;

                const float similarity = heartlake::ai::AdvancedEmbeddingEngine::cosineSimilarity(
                    queryEmbedding, targetEmbedding);

                Json::Value stone;
                stone["stone_id"] = dbRow["stone_id"].as<std::string>();
                stone["content"] = dbRow["content"].as<std::string>();
                stone["mood"] = dbRow["mood_type"].isNull() ? "neutral" : dbRow["mood_type"].as<std::string>();
                stone["ripple_count"] = dbRow["ripple_count"].as<int>();
                stone["boat_count"] = dbRow["boat_count"].as<int>();
                stone["created_at"] = dbRow["created_at"].as<std::string>();
                stone["nickname"] = dbRow["nickname"].isNull() ? "" : dbRow["nickname"].as<std::string>();
                stone["similarity"] = similarity;

                candidates.push_back({stone, similarity});
            }

            std::sort(candidates.begin(), candidates.end(), [](const Candidate& lhs, const Candidate& rhs) {
                return lhs.similarity > rhs.similarity;
            });

            Json::Value resp;
            resp["code"] = 0;
            resp["message"] = "成功";

            Json::Value data;
            Json::Value stones = Json::arrayValue;
            const size_t limit = std::min<size_t>(10, candidates.size());
            for (size_t i = 0; i < limit; ++i) {
                stones.append(candidates[i].stone);
            }

            data["stones"] = stones;
            data["total"] = static_cast<int>(stones.size());
            data["method"] = "legacy_embedding_fallback";
            data["fallback_reason"] = fallbackReason;
            resp["data"] = data;

            callback(HttpResponse::newHttpJsonResponse(resp));
        },
        [callback = std::move(callback)](const drogon::orm::DrogonDbException& e) mutable {
            LOG_ERROR << "Legacy vector fallback failed: " << e.base().what();
            callback(ResponseUtil::internalError("查询失败"));
        },
        stoneId
    );
}

void persistLegacyEmbedding(
    const drogon::orm::DbClientPtr& dbClient,
    const std::string& stoneId,
    const std::string& vectorStr,
    std::function<void(bool ok, const std::string& error)> callback
) {
    dbClient->execSqlAsync(
        "INSERT INTO stone_embeddings (stone_id, embedding, created_at) "
        "VALUES ($1, $2, NOW()) "
        "ON CONFLICT (stone_id) DO UPDATE SET embedding = EXCLUDED.embedding, created_at = NOW()",
        [callback = std::move(callback)](const drogon::orm::Result&) mutable {
            callback(true, "");
        },
        [callback = std::move(callback)](const drogon::orm::DrogonDbException& e) mutable {
            callback(false, e.base().what());
        },
        stoneId, vectorStr
    );
}

}  // namespace

void VectorSearchController::getSimilarStones(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &stoneId
) {
    // 认证检查：向量搜索涉及用户内容数据，必须验证身份
    auto userIdOpt = Validator::getUserId(req);
    if (!userIdOpt) {
        callback(ResponseUtil::unauthorized("未登录，请先登录"));
        return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    
    // 先只读取基础内容，避免在缺失 pgvector 列时直接查询失败。
    dbClient->execSqlAsync(
        "SELECT stone_id, content FROM stones WHERE stone_id = $1 AND status = 'published' AND deleted_at IS NULL",
        [callback, dbClient, stoneId](const drogon::orm::Result &r) {
            if (r.empty()) {
                Json::Value resp;
                resp["code"] = 404;
                resp["message"] = "石头不存在";
                auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                httpResp->setStatusCode(k404NotFound);
                callback(httpResp);
                return;
            }

            const std::string content = r[0]["content"].as<std::string>();

            // 使用向量搜索查找相似石头
            dbClient->execSqlAsync(
                "SELECT s.stone_id, s.content, s.mood_type, s.ripple_count, s.boat_count, "
                "s.created_at, u.nickname, u.username, "
                "1 - (s.embedding <=> (SELECT embedding FROM stones WHERE stone_id = $1)) as similarity "
                "FROM stones s "
                "LEFT JOIN users u ON s.user_id = u.user_id "
                "WHERE s.status = 'published' AND s.deleted_at IS NULL AND s.stone_id != $1 "
                "AND s.embedding IS NOT NULL "
                "ORDER BY s.embedding <=> (SELECT embedding FROM stones WHERE stone_id = $1) "
                "LIMIT 10",
                [callback](const drogon::orm::Result &res) {
                    Json::Value resp;
                    resp["code"] = 0;
                    resp["message"] = "成功";

                    Json::Value data;
                    Json::Value stones = Json::arrayValue;

                    for (const auto& dbRow : res) {
                        Json::Value stone;
                        stone["stone_id"] = dbRow["stone_id"].as<std::string>();
                        stone["content"] = dbRow["content"].as<std::string>();
                        stone["mood"] = dbRow["mood_type"].isNull() ? "neutral" : dbRow["mood_type"].as<std::string>();
                        stone["ripple_count"] = dbRow["ripple_count"].as<int>();
                        stone["boat_count"] = dbRow["boat_count"].as<int>();
                        stone["created_at"] = dbRow["created_at"].as<std::string>();
                        stone["nickname"] = dbRow["nickname"].isNull() ? "" : dbRow["nickname"].as<std::string>();

                        if (!dbRow["similarity"].isNull()) {
                            stone["similarity"] = dbRow["similarity"].as<float>();
                        } else {
                            stone["similarity"] = 0.0f;
                        }
                        
                        stones.append(stone);
                    }
                    
                    data["stones"] = stones;
                    data["total"] = static_cast<int>(stones.size());
                    data["method"] = "vector_search";
                    resp["data"] = data;
                    
                    auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                    callback(httpResp);
                },
                [callback, dbClient, stoneId, content](const drogon::orm::DrogonDbException &e) mutable {
                    LOG_WARN << "Vector search unavailable, fallback to legacy embeddings: " << e.base().what();
                    respondWithLegacySimilarityFallback(
                        dbClient, stoneId, content, std::move(callback), e.base().what());
                },
                stoneId
            );
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Error getting stone: " << e.base().what();
            Json::Value resp;
            resp["code"] = 500;
            resp["message"] = "查询失败";
            auto httpResp = HttpResponse::newHttpJsonResponse(resp);
            httpResp->setStatusCode(k500InternalServerError);
            callback(httpResp);
        },
        stoneId
    );
}

void VectorSearchController::getPersonalizedRecommendations(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback
) {
    // 获取当前用户ID
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
    auto& userId = *userIdOpt;

    auto dbClient = drogon::app().getDbClient("default");
    
    // 获取最近的石头推荐
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
            
            for (const auto& row : r) {
                Json::Value stone;
                stone["stone_id"] = row["stone_id"].as<std::string>();
                stone["content"] = row["content"].as<std::string>();
                stone["mood"] = row["mood_type"].isNull() ? "neutral" : row["mood_type"].as<std::string>();
                stone["ripple_count"] = row["ripple_count"].as<int>();
                stone["boat_count"] = row["boat_count"].as<int>();
                stone["created_at"] = row["created_at"].as<std::string>();
                stone["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
                stones.append(stone);
            }
            
            data["stones"] = stones;
            data["total"] = static_cast<int>(stones.size());
            resp["data"] = data;
            
            auto httpResp = HttpResponse::newHttpJsonResponse(resp);
            callback(httpResp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Error in getPersonalizedRecommendations: " << e.base().what();
            Json::Value resp;
            resp["code"] = 500;
            resp["message"] = "查询失败";
            auto httpResp = HttpResponse::newHttpJsonResponse(resp);
            httpResp->setStatusCode(k500InternalServerError);
            callback(httpResp);
        },
        userId
    );
}

void VectorSearchController::updateStoneEmbedding(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &stoneId
) {
    // SEC-01: 认证检查 — 防止未登录用户更新任意石头的向量嵌入
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
    
    // 获取石头内容
    dbClient->execSqlAsync(
        "SELECT content FROM stones WHERE stone_id = $1",
        [callback, dbClient, stoneId](const drogon::orm::Result &r) {
            if (r.empty()) {
                Json::Value resp;
                resp["code"] = 404;
                resp["message"] = "石头不存在";
                auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                httpResp->setStatusCode(k404NotFound);
                callback(httpResp);
                return;
            }
            
            std::string content = r[0]["content"].as<std::string>();

            // 使用本地高性能嵌入向量引擎（避免API调用成本）
            auto& embeddingEngine = heartlake::ai::AdvancedEmbeddingEngine::getInstance();
            auto embedding = embeddingEngine.generateEmbedding(content);

            // 将向量转换为PostgreSQL vector格式的字符串
            std::string vectorStr = "[";
            for (size_t i = 0; i < embedding.size(); i++) {
                if (i > 0) vectorStr += ",";
                vectorStr += std::to_string(embedding[i]);
            }
            vectorStr += "]";

            persistLegacyEmbedding(
                dbClient, stoneId, vectorStr,
                [callback, dbClient, vectorStr, stoneId](bool ok, const std::string& error) {
                    if (!ok) {
                        LOG_ERROR << "Error updating legacy embedding: " << error;
                        Json::Value resp;
                        resp["code"] = 500;
                        resp["message"] = "更新失败";
                        auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                        httpResp->setStatusCode(k500InternalServerError);
                        callback(httpResp);
                        return;
                    }

                    dbClient->execSqlAsync(
                        "UPDATE stones SET embedding = $1::vector WHERE stone_id = $2",
                        [callback](const drogon::orm::Result &) {
                            Json::Value resp;
                            resp["code"] = 0;
                            resp["message"] = "向量嵌入更新成功";
                            Json::Value data;
                            data["method"] = "pgvector_and_legacy_embedding";
                            resp["data"] = data;
                            callback(HttpResponse::newHttpJsonResponse(resp));
                        },
                        [callback](const drogon::orm::DrogonDbException &e) {
                            LOG_WARN << "pgvector column unavailable, legacy embedding persisted only: "
                                     << e.base().what();
                            Json::Value resp;
                            resp["code"] = 0;
                            resp["message"] = "向量嵌入更新成功";
                            Json::Value data;
                            data["method"] = "legacy_embedding_only";
                            data["fallback_reason"] = e.base().what();
                            resp["data"] = data;
                            callback(HttpResponse::newHttpJsonResponse(resp));
                        },
                        vectorStr, stoneId
                    );
                }
            );
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Error getting stone: " << e.base().what();
            Json::Value resp;
            resp["code"] = 500;
            resp["message"] = "查询失败";
            auto httpResp = HttpResponse::newHttpJsonResponse(resp);
            httpResp->setStatusCode(k500InternalServerError);
            callback(httpResp);
        },
        stoneId
    );
}

} // namespace controllers
} // namespace heartlake
