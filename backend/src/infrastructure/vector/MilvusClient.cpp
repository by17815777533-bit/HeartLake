/**
 * @file MilvusClient.cpp
 * @brief Milvus 向量检索服务接入客户端的核心实现
 *
 * 包装基础 HttpClient 发送符合 Milvus v2 HTTP RESTful 规范的网络报文协议，负责提供如下能力基座：
 * - **元数据存储拓扑控制**：管控索引集合 (Collection) 及向量度量模式。
 * - **向量持久化及高维检索**：单目及批量写入和查询支持近似近邻检索 (ANN Search)。
 * - **网络存活侦测**：定期维持通信热度并监测连通性。
 *
 * @note 并发与鲁棒性注意事项：
 * - 为防御 I/O 饥饿及系统性死锁（尤其是 Drogon 主线程下调用阻塞 API 产生的事件断言错误），
 *   本类维护了一套专属在隔离态系统线程中分配的独立由 `trantor::EventLoopThread` 纳管的事件分发池。
 * - 加入了底层参数隔离检查（通过特定的转义过滤器化解标量内蕴含的回车、引号等干扰字符），免受向量库执行注入。
 */

#include "infrastructure/vector/MilvusClient.h"
#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <sstream>

namespace heartlake::infrastructure {

/**
 * @brief 内部安全组件：条件从句的值面转义处理器
 * 
 * 清洗被摄入查询和删除指令条件断言域内的用户或业务随机态输入面变量，将控制字符和引用转义。
 * 进而防止恶意组合或破坏后端词法层析解器。
 * 
 * @param value 原生字符串表达值
 * @return 构造过的安全逃逸字符串
 */
static std::string escapeFilterValue(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char c : value) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        // 过滤非典型控制码元
        if (c >= 0x20) {
            escaped += c;
        }
    }
    return escaped;
}

/**
 * @brief 抓取客户端单例（符合 Meyers' Singleton 范式，无锁具象保证安全）
 * @return MilvusClient 控制器实体的全局唯一静态引用
 */
MilvusClient& MilvusClient::getInstance() {
    static MilvusClient instance;
    return instance;
}

/**
 * @brief 使用运行时挂载点初始驱动及联通注册
 *
 * 初始化引擎组件、锁配专属事件队列与 HttpClient 上下文。必须在其余业务方法启用前完成一次成功调用。
 *
 * @param config 参数群组结构体，承载 IP, 端口以及容错等系统性设定
 */
void MilvusClient::initialize(const MilvusConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        config_ = config;
        baseUrl_ = "http://" + config_.host + ":" + std::to_string(config_.port);
        connected_ = false;  // 启用滞后延保感知，首位请求再实盘落定状态
        initialized_ = true;
    }

    if (!httpLoopThread_) {
        // 分离出的专用通讯轮询器以解决主死锁及并发争用的系统缺陷设计
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

/**
 * @brief 常规信道检测兵函数
 *
 * 向底层发起一次无副作用存活状态勘测；若组件未历经首次唤醒装配则级联触发自主初始化。
 * 
 * @return 目标引擎是否给予了完整通讯可达验证反馈通过标识
 */
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

/**
 * @brief 标准化底层通导及收发处理管线
 *
 * 生成特定的 JSON 负载报元发往对应的 URL 点位，承担全局所有指令操作与向量库端进行网络数据互汇和回退容灾重投底线工作。
 *
 * @param endpoint 数据路由标位点相对径址串
 * @param method REST 标准枚举方法标定
 * @param body 需要包装进请求数据域内的预组装 JSON 对象域
 * @return 服务器解码后的完整标准 JSON 应答树，若多次尝试失败或下限击穿均被截获以赋予无效返回结构
 */
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

/**
 * @brief 系统数据表级创建
 *
 * 构造指定结构的多维数据集载体映射库及设定预制分析核。
 * 
 * @param collection 新集合名词标示
 * @param dimension 特性向量所必须契合的关键特征宽度数
 * @return 配置与指令交互并完成下发后获得服务端验证承认情况
 */
bool MilvusClient::createCollection(const std::string& collection, int dimension) {
    Json::Value body;
    body["collectionName"] = collection;
    body["dimension"] = dimension;
    body["metricType"] = "COSINE";

    auto result = httpRequest("/v2/vectordb/collections/create", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

/**
 * @brief 模型集合全本物理清除
 *
 * @param collection 选定摧毁实体名
 * @return 执行反馈状态及下层指令完成有效标识
 */
bool MilvusClient::dropCollection(const std::string& collection) {
    Json::Value body;
    body["collectionName"] = collection;
    auto result = httpRequest("/v2/vectordb/collections/drop", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

/**
 * @brief 表元生存探查指示判定
 *
 * @param collection 代查验的数据池库表名称
 * @return 从后台映射的数据体存续认定表征
 */
bool MilvusClient::hasCollection(const std::string& collection) {
    Json::Value body;
    body["collectionName"] = collection;
    auto result = httpRequest("/v2/vectordb/collections/has", "POST", body);
    return result.isMember("data") && result["data"]["has"].asBool();
}

/**
 * @brief 实装单体记录到分析集合中库内记录落点并维护向量标量
 *
 * @param collection 输入库地址指向空间名字标识
 * @param id 用以主键确定的具体实体句柄字符串
 * @param vector 代表表主体特征分析点的多态维向数据数值串列
 * @param metadata 其他相关的需要一起注入存储并作后期回查附属值的业务级信息表域 JSON 对
 * @return 指示写入和验证阶段全部合规有效通过
 */
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

/**
 * @brief 管状列级批量注入记录集
 *
 * 为突破高频 IO 瓶颈在系统管道层并轨多次传输载入而使用打包重处理方法组合载具。
 * 
 * @param collection 要推送挂载集合点名称 
 * @param ids 主键组合名录队列（长度应具备唯一对应性同组等量）
 * @param vectors 特征数据列表队群
 * @param metadata 对应记录随行的各附属字典集队群
 * @return 处理完毕状态是否达成全量一致录入
 */
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

/**
 * @brief 执行最相近近似匹配查询下投搜索作业返回推导结论
 *
 * 以当前参数为中心距离扩散检索最近似的空间节点条列作为聚类发散报告。
 * 
 * @param collection 搜索被限定约束作用在的目标集名
 * @param queryVector 主导标针定位的数值张量基点
 * @param topK 要求强制回送的候选体命中数量上限阈值范围
 * @param filter 精确排斥掉某类属性或标量的表达文法算式（可缺省不过滤）
 * @return 通过分数排名并包含所有副联属性参数组合而来的命中标靶清单
 */
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

/**
 * @brief 执行指定目标单品下架除籍脱落操作执行
 *
 * @param collection 发出定点清理之被修改集合源名
 * @param id 应当清理排除的具体资源数据签标定位符
 * @return 收到了清理过程生效成功的有效回返证实
 */
bool MilvusClient::remove(const std::string& collection, const std::string& id) {
    Json::Value body;
    body["collectionName"] = collection;
    body["filter"] = "id == \"" + escapeFilterValue(id) + "\"";
    auto result = httpRequest("/v2/vectordb/entities/delete", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

/**
 * @brief 大容量针对性集合多条并行注销执行管线
 *
 * @param collection 被管束集合库目标点
 * @param ids 清除目标的主索引名称名录排期清单
 * @return 远端引擎接管此系列清理下发的实际是否全员成功的标定回送
 */
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

/**
 * @brief 主导建设系统表结构的对应高效索引建立以加成数据吞吐处理极限速率
 *
 * 索引决定下级算子树寻找高维节点的算法类型及其相关内部内存拓扑排列结构逻辑实现树图。
 *
 * @param collection 被绑定的父系数据库源映射结构集合
 * @param indexType 建设计划指引下特定分配使用的底层搜索类项类型，影响计算复杂阶梯级
 * @param nlist (主要配给特定量变参数控制分桶数量指引项)
 * @return 表明建立进程没有突发中断的良性质检回执
 */
bool MilvusClient::createIndex(const std::string& collection, const std::string& indexType, int nlist) {
    Json::Value body;
    body["collectionName"] = collection;
    body["indexParams"]["index_type"] = indexType;
    body["indexParams"]["metric_type"] = "COSINE";
    body["indexParams"]["params"]["nlist"] = nlist;

    auto result = httpRequest("/v2/vectordb/indexes/create", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

/**
 * @brief 主动解绑已搭载数据映射系带之底层检索性能结构优化附加树索引
 *
 * @param collection 应从属执行物理断开处理的具体集合
 * @return 结束脱锁返回指示通畅标志状态布尔类型量元
 */
bool MilvusClient::dropIndex(const std::string& collection) {
    Json::Value body;
    body["collectionName"] = collection;
    auto result = httpRequest("/v2/vectordb/indexes/drop", "POST", body);
    return result.isMember("code") && result["code"].asInt() == 0;
}

} // namespace heartlake::infrastructure
