/**
 * @file QdrantClient.h
 * @brief Qdrant向量数据库客户端 (HTTP REST API)
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <json/json.h>

namespace heartlake::infrastructure {

struct QdrantConfig {
    std::string host = "127.0.0.1";
    int port = 6333;
    int timeoutMs = 5000;
    std::string apiKey;
};

struct QdrantSearchResult {
    std::string id;
    float score;
    Json::Value payload;
};

class QdrantClient {
public:
    static QdrantClient& getInstance();

    void initialize(const QdrantConfig& config = QdrantConfig{});

    // 集合管理
    bool createCollection(const std::string& collection, int dimension);
    bool deleteCollection(const std::string& collection);
    bool collectionExists(const std::string& collection);

    // 向量操作
    bool upsert(const std::string& collection, const std::string& id,
                const std::vector<float>& vector, const Json::Value& payload = {});

    bool upsertBatch(const std::string& collection,
                     const std::vector<std::string>& ids,
                     const std::vector<std::vector<float>>& vectors,
                     const std::vector<Json::Value>& payloads = {});

    std::vector<QdrantSearchResult> search(const std::string& collection,
                                           const std::vector<float>& queryVector,
                                           int topK = 10,
                                           const Json::Value& filter = {});

    // 情感标签过滤搜索
    std::vector<QdrantSearchResult> searchByEmotion(const std::string& collection,
                                                    const std::vector<float>& queryVector,
                                                    const std::string& emotion,
                                                    int topK = 10);

    bool remove(const std::string& collection, const std::string& id);
    bool removeBatch(const std::string& collection, const std::vector<std::string>& ids);

    bool isConnected() const { return connected_; }

private:
    QdrantClient() = default;
    ~QdrantClient() = default;
    QdrantClient(const QdrantClient&) = delete;
    QdrantClient& operator=(const QdrantClient&) = delete;

    Json::Value httpRequest(const std::string& endpoint, const std::string& method,
                            const Json::Value& body = {});
    std::string buildUrl(const std::string& endpoint) const;

    QdrantConfig config_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> initialized_{false};
    std::mutex mutex_;
    std::string baseUrl_;
};

} // namespace heartlake::infrastructure
