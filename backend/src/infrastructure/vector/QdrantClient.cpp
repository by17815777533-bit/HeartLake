/**
 * @file QdrantClient.cpp
 * @brief Qdrant向量数据库客户端实现 (HTTP REST API)
 */

#include "infrastructure/vector/QdrantClient.h"
#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <sstream>

namespace heartlake::infrastructure {

QdrantClient& QdrantClient::getInstance() {
    static QdrantClient instance;
    return instance;
}

void QdrantClient::initialize(const QdrantConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return;

    config_ = config;
    baseUrl_ = "http://" + config_.host + ":" + std::to_string(config_.port);
    initialized_ = true;

    auto result = httpRequest("/collections", "GET");
    connected_ = !result.isNull();
    LOG_INFO << "QdrantClient initialized, connected=" << connected_.load();
}

std::string QdrantClient::buildUrl(const std::string& endpoint) const {
    return endpoint;
}

Json::Value QdrantClient::httpRequest(const std::string& endpoint, const std::string& method,
                                       const Json::Value& body) {
    auto client = drogon::HttpClient::newHttpClient(baseUrl_);
    client->setUserAgent("HeartLake/1.0");

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath(endpoint);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    if (method == "GET") req->setMethod(drogon::Get);
    else if (method == "POST") req->setMethod(drogon::Post);
    else if (method == "PUT") req->setMethod(drogon::Put);
    else if (method == "DELETE") req->setMethod(drogon::Delete);

    if (!config_.apiKey.empty()) {
        req->addHeader("api-key", config_.apiKey);
    }

    if (!body.isNull()) {
        Json::StreamWriterBuilder writer;
        req->setBody(Json::writeString(writer, body));
    }

    auto result = client->sendRequest(req, config_.timeoutMs / 1000.0);
    if (result.first != drogon::ReqResult::Ok || !result.second) {
        connected_ = false;
        return Json::Value();
    }

    connected_ = true;
    Json::Value response;
    Json::CharReaderBuilder reader;
    std::istringstream ss(std::string(result.second->body()));
    Json::parseFromStream(reader, ss, &response, nullptr);
    return response;
}

bool QdrantClient::createCollection(const std::string& collection, int dimension) {
    Json::Value body;
    body["vectors"]["size"] = dimension;
    body["vectors"]["distance"] = "Cosine";

    auto result = httpRequest("/collections/" + collection, "PUT", body);
    return result.isMember("result") && result["result"].asBool();
}

bool QdrantClient::deleteCollection(const std::string& collection) {
    auto result = httpRequest("/collections/" + collection, "DELETE");
    return result.isMember("result") && result["result"].asBool();
}

bool QdrantClient::collectionExists(const std::string& collection) {
    auto result = httpRequest("/collections/" + collection, "GET");
    return result.isMember("result");
}

bool QdrantClient::upsert(const std::string& collection, const std::string& id,
                          const std::vector<float>& vector, const Json::Value& payload) {
    Json::Value body;
    Json::Value point;
    point["id"] = id;

    Json::Value vec(Json::arrayValue);
    for (float v : vector) vec.append(v);
    point["vector"] = vec;
    point["payload"] = payload;

    body["points"].append(point);
    auto result = httpRequest("/collections/" + collection + "/points", "PUT", body);
    return result.isMember("status") && result["status"].asString() == "ok";
}

bool QdrantClient::upsertBatch(const std::string& collection,
                               const std::vector<std::string>& ids,
                               const std::vector<std::vector<float>>& vectors,
                               const std::vector<Json::Value>& payloads) {
    if (ids.size() != vectors.size()) return false;

    Json::Value body;
    body["points"] = Json::arrayValue;

    for (size_t i = 0; i < ids.size(); ++i) {
        Json::Value point;
        point["id"] = ids[i];

        Json::Value vec(Json::arrayValue);
        for (float v : vectors[i]) vec.append(v);
        point["vector"] = vec;

        if (i < payloads.size()) {
            point["payload"] = payloads[i];
        }
        body["points"].append(point);
    }

    auto result = httpRequest("/collections/" + collection + "/points", "PUT", body);
    return result.isMember("status") && result["status"].asString() == "ok";
}

std::vector<QdrantSearchResult> QdrantClient::search(const std::string& collection,
                                                     const std::vector<float>& queryVector,
                                                     int topK,
                                                     const Json::Value& filter) {
    Json::Value body;
    Json::Value vec(Json::arrayValue);
    for (float v : queryVector) vec.append(v);
    body["vector"] = vec;
    body["limit"] = topK;
    body["with_payload"] = true;

    if (!filter.isNull() && !filter.empty()) {
        body["filter"] = filter;
    }

    auto result = httpRequest("/collections/" + collection + "/points/search", "POST", body);

    std::vector<QdrantSearchResult> results;
    if (!result.isMember("result") || !result["result"].isArray()) return results;

    for (const auto& item : result["result"]) {
        QdrantSearchResult r;
        r.id = item["id"].asString();
        r.score = item["score"].asFloat();
        r.payload = item["payload"];
        results.push_back(r);
    }
    return results;
}

std::vector<QdrantSearchResult> QdrantClient::searchByEmotion(const std::string& collection,
                                                              const std::vector<float>& queryVector,
                                                              const std::string& emotion,
                                                              int topK) {
    Json::Value filter;
    filter["must"][0]["key"] = "emotion";
    filter["must"][0]["match"]["value"] = emotion;
    return search(collection, queryVector, topK, filter);
}

bool QdrantClient::remove(const std::string& collection, const std::string& id) {
    Json::Value body;
    body["points"].append(id);
    auto result = httpRequest("/collections/" + collection + "/points/delete", "POST", body);
    return result.isMember("status") && result["status"].asString() == "ok";
}

bool QdrantClient::removeBatch(const std::string& collection, const std::vector<std::string>& ids) {
    Json::Value body;
    body["points"] = Json::arrayValue;
    for (const auto& id : ids) {
        body["points"].append(id);
    }
    auto result = httpRequest("/collections/" + collection + "/points/delete", "POST", body);
    return result.isMember("status") && result["status"].asString() == "ok";
}

} // namespace heartlake::infrastructure
