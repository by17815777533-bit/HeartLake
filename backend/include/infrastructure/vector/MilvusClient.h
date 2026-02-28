/**
 * @brief Milvus 向量数据库客户端
 *
 * @details
 * 封装 Milvus RESTful API，提供集合管理、向量 CRUD 和索引操作。
 * 底层通过 Drogon HttpClient 发起 HTTP 请求，使用独立的
 * EventLoopThread 避免阻塞主 IO 线程。
 *
 * 主要用于 ResonanceSearchService 的向量存储和相似度检索：
 * 石头发布时将文本 embedding 写入 Milvus，搜索共鸣时做 ANN 查询。
 *
 * 连接状态通过 ping() 主动探测，connected_ 标记供上层判断是否可用。
 * 支持自动重试（最多 maxRetries 次）和超时控制。
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>
#include <json/json.h>
#include <drogon/HttpClient.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/EventLoopThread.h>

namespace heartlake::infrastructure {

/// Milvus 连接配置
struct MilvusConfig {
    std::string host = "127.0.0.1";  ///< Milvus 服务地址
    int port = 19530;                ///< Milvus 服务端口
    int poolSize = 10;               ///< HTTP 连接池大小
    int timeoutMs = 5000;            ///< 单次请求超时（毫秒）
    int maxRetries = 3;              ///< 失败重试次数
};

/// 向量检索结果条目
struct VectorSearchResult {
    std::string id;       ///< 向量 ID
    float score;          ///< 相似度分数
    Json::Value metadata; ///< 附带的元数据
};

/**
 * @brief Milvus 客户端，全局单例
 */
class MilvusClient {
public:
    static MilvusClient& getInstance();

    /// 初始化连接，可传入自定义配置
    void initialize(const MilvusConfig& config = MilvusConfig{});

    /**
     * @brief 主动探测 Milvus 健康状态并刷新连接标记
     * @return true=可用，false=不可用
     */
    bool ping();

    // ---- 集合管理 ----

    /// 创建向量集合，指定维度
    bool createCollection(const std::string& collection, int dimension);
    /// 删除集合
    bool dropCollection(const std::string& collection);
    /// 检查集合是否存在
    bool hasCollection(const std::string& collection);

    // ---- 向量操作 ----

    /**
     * @brief 插入单条向量
     * @param collection 集合名
     * @param id 向量唯一标识
     * @param vector 向量数据
     * @param metadata 附带的元数据（可选）
     */
    bool insert(const std::string& collection, const std::string& id,
                const std::vector<float>& vector, const Json::Value& metadata = {});

    /// 批量插入向量
    bool insertBatch(const std::string& collection,
                     const std::vector<std::string>& ids,
                     const std::vector<std::vector<float>>& vectors,
                     const std::vector<Json::Value>& metadata = {});

    /**
     * @brief ANN 近似最近邻检索
     * @param collection 集合名
     * @param queryVector 查询向量
     * @param topK 返回最相似的 K 条结果
     * @param filter 过滤表达式（Milvus DSL），空串表示不过滤
     * @return 按相似度降序排列的结果列表
     */
    std::vector<VectorSearchResult> search(const std::string& collection,
                                           const std::vector<float>& queryVector,
                                           int topK = 10,
                                           const std::string& filter = "");

    /// 删除单条向量
    bool remove(const std::string& collection, const std::string& id);
    /// 批量删除向量
    bool removeBatch(const std::string& collection, const std::vector<std::string>& ids);

    // ---- 索引管理 ----

    /**
     * @brief 创建向量索引
     * @param collection 集合名
     * @param indexType 索引类型，默认 IVF_FLAT
     * @param nlist 聚类中心数，默认 1024
     */
    bool createIndex(const std::string& collection, const std::string& indexType = "IVF_FLAT", int nlist = 1024);
    /// 删除索引
    bool dropIndex(const std::string& collection);

    /// 当前是否与 Milvus 保持连接
    bool isConnected() const { return connected_; }

private:
    MilvusClient() = default;
    ~MilvusClient() = default;
    MilvusClient(const MilvusClient&) = delete;
    MilvusClient& operator=(const MilvusClient&) = delete;

    /// 发起 HTTP 请求并解析 JSON 响应
    Json::Value httpRequest(const std::string& endpoint, const std::string& method, const Json::Value& body = {});

    MilvusConfig config_;
    std::atomic<bool> connected_{false};     ///< 连接状态标记
    std::atomic<bool> initialized_{false};   ///< 是否已完成初始化
    std::mutex mutex_;                       ///< 保护 httpClient_ 的并发访问
    std::string baseUrl_;                    ///< 拼接后的 Milvus REST 基础 URL
    trantor::EventLoop* httpLoop_{nullptr};  ///< HTTP 请求使用的事件循环
    std::unique_ptr<trantor::EventLoopThread> httpLoopThread_;  ///< 独立的 IO 线程
    drogon::HttpClientPtr httpClient_;       ///< Drogon HTTP 客户端实例
};

} // namespace heartlake::infrastructure
