/**
 * @file MilvusClient.h
 * @brief Milvus向量数据库客户端
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <json/json.h>

namespace heartlake::infrastructure {

struct MilvusConfig {
    std::string host = "127.0.0.1";
    int port = 19530;
    int poolSize = 10;
    int timeoutMs = 5000;
    int maxRetries = 3;
};

struct VectorSearchResult {
    std::string id;
    float score;
    Json::Value metadata;
};

class MilvusClient {
public:
    static MilvusClient& getInstance();

    void initialize(const MilvusConfig& config = MilvusConfig{});

    // 集合管理
    bool createCollection(const std::string& collection, int dimension);
    bool dropCollection(const std::string& collection);
    bool hasCollection(const std::string& collection);

    // 向量操作
    bool insert(const std::string& collection, const std::string& id,
                const std::vector<float>& vector, const Json::Value& metadata = {});

    bool insertBatch(const std::string& collection,
                     const std::vector<std::string>& ids,
                     const std::vector<std::vector<float>>& vectors,
                     const std::vector<Json::Value>& metadata = {});

    std::vector<VectorSearchResult> search(const std::string& collection,
                                           const std::vector<float>& queryVector,
                                           int topK = 10,
                                           const std::string& filter = "");

    bool remove(const std::string& collection, const std::string& id);
    bool removeBatch(const std::string& collection, const std::vector<std::string>& ids);

    // 索引管理
    bool createIndex(const std::string& collection, const std::string& indexType = "IVF_FLAT", int nlist = 1024);
    bool dropIndex(const std::string& collection);

    bool isConnected() const { return connected_; }

private:
    MilvusClient() = default;
    ~MilvusClient() = default;
    MilvusClient(const MilvusClient&) = delete;
    MilvusClient& operator=(const MilvusClient&) = delete;

    Json::Value httpRequest(const std::string& endpoint, const std::string& method, const Json::Value& body = {});
    void tryReconnect();

    MilvusConfig config_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> initialized_{false};
    std::mutex mutex_;
    std::string baseUrl_;
};

} // namespace heartlake::infrastructure
