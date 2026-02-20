/**
 * @file VectorSearchController.cpp
 * @brief VectorSearchController 模块实现
 * Created by 白洋
 */
#include "interfaces/api/VectorSearchController.h"
#include "infrastructure/ai/AdvancedEmbeddingEngine.h"
#include <json/json.h>

namespace heartlake {
namespace controllers {

void VectorSearchController::getSimilarStones(
    [[maybe_unused]] const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &stoneId
) {
    auto dbClient = drogon::app().getDbClient("default");
    
    // 获取目标石头的信息
    dbClient->execSqlAsync(
        "SELECT stone_id, content, embedding FROM stones WHERE stone_id = $1 AND status = 'published' AND deleted_at IS NULL",
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
            
            auto row = r[0];
            
            // 检查是否有向量嵌入
            if (row["embedding"].isNull()) {
                // 没有向量嵌入，返回随机推荐
                dbClient->execSqlAsync(
                    "SELECT s.stone_id, s.content, s.mood_type, s.ripple_count, s.boat_count, "
                    "s.created_at, u.nickname, u.username "
                    "FROM stones s "
                    "LEFT JOIN users u ON s.user_id = u.user_id "
                    "WHERE s.status = 'published' AND s.deleted_at IS NULL AND s.stone_id != $1 "
                    "ORDER BY RANDOM() LIMIT 5",
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
                            stone["mood"] = row["mood_type"].as<std::string>();
                            stone["ripple_count"] = row["ripple_count"].as<int>();
                            stone["boat_count"] = row["boat_count"].as<int>();
                            stone["created_at"] = row["created_at"].as<std::string>();
                            stone["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
                            stone["similarity"] = 0.0;
                            stones.append(stone);
                        }
                        
                        data["stones"] = stones;
                        data["total"] = (int)stones.size();
                        data["method"] = "random";
                        resp["data"] = data;
                        
                        auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                        callback(httpResp);
                    },
                    [callback](const drogon::orm::DrogonDbException &e) {
                        LOG_ERROR << "Error in random recommendation: " << e.base().what();
                        Json::Value resp;
                        resp["code"] = 500;
                        resp["message"] = "查询失败";
                        auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                        httpResp->setStatusCode(k500InternalServerError);
                        callback(httpResp);
                    },
                    stoneId
                );
                return;
            }
            
            // 使用向量搜索查找相似石头
            dbClient->execSqlAsync(
                "SELECT s.stone_id, s.content, s.mood_type, s.ripple_count, s.boat_count, "
                "s.created_at, u.nickname, u.username, "
                "1 - (s.embedding <=> (SELECT embedding FROM stones WHERE stone_id = $1)) as similarity "
                "FROM stones s "
                "LEFT JOIN users u ON s.user_id = u.user_id "
                "WHERE s.status = 'published' AND s.stone_id != $1 "
                "AND s.embedding IS NOT NULL "
                "ORDER BY s.embedding <=> (SELECT embedding FROM stones WHERE stone_id = $1) "
                "LIMIT 10",
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
                        stone["mood"] = row["mood_type"].as<std::string>();
                        stone["ripple_count"] = row["ripple_count"].as<int>();
                        stone["boat_count"] = row["boat_count"].as<int>();
                        stone["created_at"] = row["created_at"].as<std::string>();
                        stone["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
                        
                        if (!row["similarity"].isNull()) {
                            stone["similarity"] = row["similarity"].as<float>();
                        } else {
                            stone["similarity"] = 0.0f;
                        }
                        
                        stones.append(stone);
                    }
                    
                    data["stones"] = stones;
                    data["total"] = (int)stones.size();
                    data["method"] = "vector_search";
                    resp["data"] = data;
                    
                    auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                    callback(httpResp);
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Error in vector search: " << e.base().what();
                    Json::Value resp;
                    resp["code"] = 500;
                    resp["message"] = "查询失败";
                    auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                    httpResp->setStatusCode(k500InternalServerError);
                    callback(httpResp);
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
    std::string userId;
    try {
        userId = req->getAttributes()->get<std::string>("user_id");
    } catch (...) {
        Json::Value resp;
        resp["code"] = 401;
        resp["message"] = "未登录";
        auto httpResp = HttpResponse::newHttpJsonResponse(resp);
        httpResp->setStatusCode(k401Unauthorized);
        callback(httpResp);
        return;
    }
    
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
                stone["mood"] = row["mood_type"].as<std::string>();
                stone["ripple_count"] = row["ripple_count"].as<int>();
                stone["boat_count"] = row["boat_count"].as<int>();
                stone["created_at"] = row["created_at"].as<std::string>();
                stone["nickname"] = row["nickname"].isNull() ? "" : row["nickname"].as<std::string>();
                stones.append(stone);
            }
            
            data["stones"] = stones;
            data["total"] = (int)stones.size();
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
    std::string userId;
    try {
        userId = req->getAttributes()->get<std::string>("user_id");
    } catch (...) {}
    if (userId.empty()) {
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

            // 更新数据库
            dbClient->execSqlAsync(
                "UPDATE stones SET embedding = $1::vector WHERE stone_id = $2",
                [callback](const drogon::orm::Result &) {
                    Json::Value resp;
                    resp["code"] = 0;
                    resp["message"] = "向量嵌入更新成功";
                    Json::Value data;
                    data["method"] = "local_advanced_embedding";
                    resp["data"] = data;
                    auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                    callback(httpResp);
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Error updating embedding: " << e.base().what();
                    Json::Value resp;
                    resp["code"] = 500;
                    resp["message"] = "更新失败";
                    auto httpResp = HttpResponse::newHttpJsonResponse(resp);
                    httpResp->setStatusCode(k500InternalServerError);
                    callback(httpResp);
                },
                vectorStr, stoneId
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
