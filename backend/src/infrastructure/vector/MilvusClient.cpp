/**
 * Milvus向量数据库客户端实现
 */

#include "infrastructure/vector/MilvusClient.h"
#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <sstream>

namespace heartlake::infrastructure {

// 转义 Milvus filter 中的特殊字符，防止注入
static std::string escapeFilterValue(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char c : value) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        // 拒绝控制字符
        if (c >= 0x20) {
            escaped += c;
        }
    }
    return escaped;
}

MilvusClient& MilvusClient::getInstance() {
    static MilvusClient instance;
    return instance;
}

void MilvusClient::initialize(const MilvusConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        config_ = config;
        baseUrl_ = "http://" + config_.host + ":" + std::to_string(config_.port);
        connected_ = false;  // 懒加载，首次请求时检测连接状态
        initialized_ = true;
    }

    if (!httpLoopThread_) {
        // 独立 EventLoop 避免在 Drogon I/O 线程里调用同步 sendRequest 触发死锁断言。
        httpLoopThread_ = std::make_unique<trantor::EventLoopThread>("milvus-http-loop");
        httpLoopThread_->run();
        httpLoop_ = httpLoopThread_->getLoop();
    }
    if (!httpClient_) {
        httpClient_ = drogon::HttpClient::newHttpClient(baseUrl_, httpLoop_);
        httpClient_->setUserAgent("HeartLake/1.0");
    }

    LOG_INFO << "MilvusClient initialized (dedicated loop, base url " << baseUrl_ << ")";
}

bool MilvusClient::ping() {
    if (!initialized_) {
        initialize();
    }

    Json::Value body;
    body["collectionName"] = "__heartlake_ping__";

    const auto result = httpRequest("/v2/vectordb/collections/has", "POST", body);
    const bool reachable = result.isObject() && result.isMember("code");
    connected_ = reachable;
    return reachable;
}

Json::Value MilvusClient::httpRequest(const std::string& endpoint, const std::string& method, const Json::Value& body) {
    drogon::HttpClientPtr client;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) {
            config_ = MilvusConfig{};
            baseUrl_ = "http://" + config_.host + ":" + std::to_string(config_.port);
            initialized_ = true;
        }
        if (!httpLoopThread_) {
            httpLoopThread_ = std::make_unique<trantor::EventLoopThread>("milvus-http-loop");
            httpLoopThread_->run();
            httpLoop_ = httpLoopThread_->getLoop();
        }
        if (!httpClient_) {
            httpClient_ = drogon::HttpClient::newHttpClient(baseUrl_, httpLoop_);
            httpClient_->setUserAgent("HeartLake/1.0");
        }
        client = httpClient_;
    }

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath(endpoint);
    req->setMethod(method == "POST" ? drogon::Post : drogon::Get);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    if (!body.isNull()) {
        Json::StreamWriterBuilder writer;
        req->setBody(Json::writeString(writer, body));
    }

    std::pair<drogon::ReqResult, drogon::HttpResponsePtr> result;
    for (int retry = 0; retry < config_.maxRetries; ++retry) {
        result = client->sendRequest(req, config_.timeoutMs / 1000.0);
        if (result.first == drogon::ReqResult::Ok) break;
    }

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

bool MilvusClient::createCollection(const std::string& collection, int dimension) {
    Json::Value body;
    body["collectionName"] = collection;
    body["dimension"] = dimension;
    body["metricType"] = "COSINE";

    auto result = httpRequest("/v2/vectordb/collections/create", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

bool MilvusClient::dropCollection(const std::string& collection) {
    Json::Value body;
    body["collectionName"] = collection;
    auto result = httpRequest("/v2/vectordb/collections/drop", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

bool MilvusClient::hasCollection(const std::string& collection) {
    Json::Value body;
    body["collectionName"] = collection;
    auto result = httpRequest("/v2/vectordb/collections/has", "POST", body);
    return result.isMember("data") && result["data"]["has"].asBool();
}

bool MilvusClient::insert(const std::string& collection, const std::string& id,
                          const std::vector<float>& vector, const Json::Value& metadata) {
    Json::Value body;
    body["collectionName"] = collection;

    Json::Value row;
    row["id"] = id;
    Json::Value vec(Json::arrayValue);
    for (float v : vector) vec.append(v);
    row["vector"] = vec;

    for (const auto& key : metadata.getMemberNames()) {
        row[key] = metadata[key];
    }

    body["data"].append(row);
    auto result = httpRequest("/v2/vectordb/entities/insert", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

bool MilvusClient::insertBatch(const std::string& collection,
                               const std::vector<std::string>& ids,
                               const std::vector<std::vector<float>>& vectors,
                               const std::vector<Json::Value>& metadata) {
    if (ids.size() != vectors.size()) return false;

    Json::Value body;
    body["collectionName"] = collection;
    body["data"] = Json::arrayValue;

    for (size_t i = 0; i < ids.size(); ++i) {
        Json::Value row;
        row["id"] = ids[i];
        Json::Value vec(Json::arrayValue);
        for (float v : vectors[i]) vec.append(v);
        row["vector"] = vec;

        if (i < metadata.size()) {
            for (const auto& key : metadata[i].getMemberNames()) {
                row[key] = metadata[i][key];
            }
        }
        body["data"].append(row);
    }

    auto result = httpRequest("/v2/vectordb/entities/insert", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

std::vector<VectorSearchResult> MilvusClient::search(const std::string& collection,
                                                     const std::vector<float>& queryVector,
                                                     int topK,
                                                     const std::string& filter) {
    Json::Value body;
    body["collectionName"] = collection;
    body["limit"] = topK;

    Json::Value vec(Json::arrayValue);
    for (float v : queryVector) vec.append(v);
    body["data"].append(vec);

    if (!filter.empty()) {
        body["filter"] = filter;
    }
    body["outputFields"].append("*");

    auto result = httpRequest("/v2/vectordb/entities/search", "POST", body);

    std::vector<VectorSearchResult> results;
    if (!result.isMember("data") || !result["data"].isArray()) return results;

    for (const auto& item : result["data"][0]) {
        VectorSearchResult r;
        r.id = item["id"].asString();
        r.score = item["distance"].asFloat();
        r.metadata = item;
        results.push_back(r);
    }
    return results;
}

bool MilvusClient::remove(const std::string& collection, const std::string& id) {
    Json::Value body;
    body["collectionName"] = collection;
    body["filter"] = "id == \"" + escapeFilterValue(id) + "\"";
    auto result = httpRequest("/v2/vectordb/entities/delete", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

bool MilvusClient::removeBatch(const std::string& collection, const std::vector<std::string>& ids) {
    std::string filter = "id in [";
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) filter += ",";
        filter += "\"" + escapeFilterValue(ids[i]) + "\"";
    }
    filter += "]";

    Json::Value body;
    body["collectionName"] = collection;
    body["filter"] = filter;
    auto result = httpRequest("/v2/vectordb/entities/delete", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

bool MilvusClient::createIndex(const std::string& collection, const std::string& indexType, int nlist) {
    Json::Value body;
    body["collectionName"] = collection;
    body["indexParams"]["index_type"] = indexType;
    body["indexParams"]["metric_type"] = "COSINE";
    body["indexParams"]["params"]["nlist"] = nlist;

    auto result = httpRequest("/v2/vectordb/indexes/create", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

bool MilvusClient::dropIndex(const std::string& collection) {
    Json::Value body;
    body["collectionName"] = collection;
    auto result = httpRequest("/v2/vectordb/indexes/drop", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

} // namespace heartlake::infrastructure
