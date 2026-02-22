/**
 * @file EdgeAIController.h
 * @brief 边缘AI控制器 — 提供本地AI推理、联邦学习、差分隐私与向量检索的HTTP接口
 *
 * 本控制器整合了 HeartLake 后端的多个AI基础设施组件，
 * 将本地情感分析、内容审核、语义缓存、差分隐私引擎、
 * 联邦学习聚合以及向量相似度搜索统一暴露为 RESTful API。
 *
 * 所有公开接口使用 SecurityAuditFilter 进行安全审计；
 * 管理接口额外使用 AdminAuthFilter（PASETO 令牌）进行鉴权。
 *
 * 响应格式统一为：
 * @code
 * {
 *   "code": 200,
 *   "message": "ok",
 *   "data": { ... },
 *   "timestamp": "2025-06-15T12:00:00Z"
 * }
 * @endcode
 *
 * Created by 白洋
 */
#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include "utils/ResponseUtil.h"
#include "utils/ErrorCode.h"

using namespace drogon;
using namespace heartlake::utils;

namespace heartlake {
namespace controllers {

/**
 * @brief 边缘AI HTTP控制器
 *
 * 提供10个API端点，覆盖边缘AI引擎状态查询、本地推理、
 * 联邦学习聚合、差分隐私预算管理、向量检索以及管理员配置。
 */
class EdgeAIController : public HttpController<EdgeAIController> {
public:
    METHOD_LIST_BEGIN

    // ==================== 公开接口 ====================

    /**
     * @brief 获取边缘AI引擎运行状态
     * @route GET /api/edge-ai/status
     *
     * 返回各子系统（本地嵌入、语义缓存、差分隐私引擎、情绪共鸣引擎）
     * 的健康状态与版本信息。
     */
    ADD_METHOD_TO(EdgeAIController::getStatus,
                  "/api/edge-ai/status", Get,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 获取边缘AI详细性能指标
     * @route GET /api/edge-ai/metrics
     *
     * 返回语义缓存命中率、本地嵌入吞吐量、推理延迟分位数、
     * 隐私预算消耗速率等运行时指标。
     */
    ADD_METHOD_TO(EdgeAIController::getMetrics,
                  "/api/edge-ai/metrics", Get,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 本地情感分析（不调用云端API）
     * @route POST /api/edge-ai/analyze
     *
     * 请求体：
     * @code
     * {
     *   "text": "今天心情很好",
     *   "language": "zh"          // 可选，默认 "zh"
     * }
     * @endcode
     */
    ADD_METHOD_TO(EdgeAIController::analyzeLocal,
                  "/api/edge-ai/analyze", Post,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 本地内容审核（不调用云端API）
     * @route POST /api/edge-ai/moderate
     *
     * 请求体：
     * @code
     * {
     *   "text": "待审核文本",
     *   "strictMode": false       // 可选，是否启用严格模式
     * }
     * @endcode
     */
    ADD_METHOD_TO(EdgeAIController::moderateLocal,
                  "/api/edge-ai/moderate", Post,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 社区实时情绪脉搏数据
     * @route GET /api/edge-ai/emotion-pulse
     *
     * 查询参数：
     * - windowHours (int, 可选, 默认1): 时间窗口（小时）
     * - epsilon (double, 可选, 默认2.0): 差分隐私预算
     *
     * 返回经差分隐私保护的社区情绪聚合数据，包括
     * 情绪分布、平均分数、参与用户数等。
     */
    ADD_METHOD_TO(EdgeAIController::getEmotionPulse,
                  "/api/edge-ai/emotion-pulse", Get,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 触发联邦学习聚合
     * @route POST /api/edge-ai/federated/aggregate
     *
     * 请求体：
     * @code
     * {
     *   "round": 1,                   // 聚合轮次
     *   "minParticipants": 5,         // 最少参与节点数，可选，默认3
     *   "epsilon": 1.0,               // 差分隐私预算，可选，默认1.0
     *   "clippingBound": 1.0          // 梯度裁剪上界，可选，默认1.0
     * }
     * @endcode
     */
    ADD_METHOD_TO(EdgeAIController::federatedAggregate,
                  "/api/edge-ai/federated/aggregate", Post,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 查询差分隐私预算状态
     * @route GET /api/edge-ai/privacy-budget
     *
     * 返回当前已消耗的隐私预算 ε、剩余预算、查询次数、
     * 预算消耗速率以及预计耗尽时间。
     */
    ADD_METHOD_TO(EdgeAIController::getPrivacyBudget,
                  "/api/edge-ai/privacy-budget", Get,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 本地向量相似度搜索
     * @route POST /api/edge-ai/vector-search
     *
     * 请求体：
     * @code
     * {
     *   "query": "寻找温暖的故事",
     *   "topK": 10,                   // 可选，默认10
     *   "threshold": 0.6,             // 可选，最低相似度阈值，默认0.0
     *   "mood": "calm"                // 可选，情绪过滤
     * }
     * @endcode
     */
    ADD_METHOD_TO(EdgeAIController::vectorSearch,
                  "/api/edge-ai/vector-search", Post,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 向量插入（用于填充HNSW索引）
     * @route POST /api/edge-ai/vector-insert
     * @code
     * {
     *   "id": "stone_xxx",
     *   "vector": [0.1, 0.2, ...]
     * }
     * @endcode
     */
    ADD_METHOD_TO(EdgeAIController::vectorInsert,
                  "/api/edge-ai/vector-insert", Post,
                  "heartlake::filters::SecurityAuditFilter");

    /**
     * @brief 提交情绪样本（用于社区情绪脉搏）
     * @route POST /api/edge-ai/emotion-sample
     * @code
     * {
     *   "score": 0.85,
     *   "mood": "happy"
     * }
     * @endcode
     */
    ADD_METHOD_TO(EdgeAIController::submitEmotionSample,
                  "/api/edge-ai/emotion-sample", Post,
                  "heartlake::filters::SecurityAuditFilter");

    // ==================== 管理接口（需要 PASETO 令牌） ====================

    /**
     * @brief 管理后台查询引擎状态
     * @route GET /api/admin/edge-ai/status
     */
    ADD_METHOD_TO(EdgeAIController::getStatus,
                  "/api/admin/edge-ai/status", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 管理后台查询性能指标
     * @route GET /api/admin/edge-ai/metrics
     */
    ADD_METHOD_TO(EdgeAIController::getMetrics,
                  "/api/admin/edge-ai/metrics", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 管理后台本地情感分析
     * @route POST /api/admin/edge-ai/analyze
     */
    ADD_METHOD_TO(EdgeAIController::analyzeLocal,
                  "/api/admin/edge-ai/analyze", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 管理后台本地内容审核
     * @route POST /api/admin/edge-ai/moderate
     */
    ADD_METHOD_TO(EdgeAIController::moderateLocal,
                  "/api/admin/edge-ai/moderate", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 管理后台社区情绪脉搏
     * @route GET /api/admin/edge-ai/emotion-pulse
     */
    ADD_METHOD_TO(EdgeAIController::getEmotionPulse,
                  "/api/admin/edge-ai/emotion-pulse", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 管理后台触发联邦学习聚合
     * @route POST /api/admin/edge-ai/federated/aggregate
     */
    ADD_METHOD_TO(EdgeAIController::federatedAggregate,
                  "/api/admin/edge-ai/federated/aggregate", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 管理后台查询隐私预算
     * @route GET /api/admin/edge-ai/privacy-budget
     */
    ADD_METHOD_TO(EdgeAIController::getPrivacyBudget,
                  "/api/admin/edge-ai/privacy-budget", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 管理后台向量检索
     * @route POST /api/admin/edge-ai/vector-search
     */
    ADD_METHOD_TO(EdgeAIController::vectorSearch,
                  "/api/admin/edge-ai/vector-search", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(EdgeAIController::vectorInsert,
                  "/api/admin/edge-ai/vector-insert", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    ADD_METHOD_TO(EdgeAIController::submitEmotionSample,
                  "/api/admin/edge-ai/emotion-sample", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 情绪脉搏历史
     * @route GET /api/admin/edge-ai/pulse-history?count=10
     */
    ADD_METHOD_TO(EdgeAIController::getPulseHistory,
                  "/api/admin/edge-ai/pulse-history", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 提交联邦学习本地模型
     * @route POST /api/admin/edge-ai/federated/submit
     */
    ADD_METHOD_TO(EdgeAIController::submitLocalModel,
                  "/api/admin/edge-ai/federated/submit", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 重置差分隐私预算
     * @route POST /api/admin/edge-ai/privacy/reset
     */
    ADD_METHOD_TO(EdgeAIController::resetPrivacyBudget,
                  "/api/admin/edge-ai/privacy/reset", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 注册边缘节点
     * @route POST /api/admin/edge-ai/nodes/register
     */
    ADD_METHOD_TO(EdgeAIController::registerNode,
                  "/api/admin/edge-ai/nodes/register", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 更新边缘节点状态
     * @route PUT /api/admin/edge-ai/nodes/status
     */
    ADD_METHOD_TO(EdgeAIController::updateNodeStatus,
                  "/api/admin/edge-ai/nodes/status", Put, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 选择最优边缘节点
     * @route GET /api/admin/edge-ai/nodes/best
     */
    ADD_METHOD_TO(EdgeAIController::selectBestNode,
                  "/api/admin/edge-ai/nodes/best", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 量化推理
     * @route POST /api/admin/edge-ai/quantized-forward
     */
    ADD_METHOD_TO(EdgeAIController::quantizedForward,
                  "/api/admin/edge-ai/quantized-forward", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 差分隐私噪声注入
     * @route POST /api/admin/edge-ai/privacy/add-noise
     */
    ADD_METHOD_TO(EdgeAIController::addNoise,
                  "/api/admin/edge-ai/privacy/add-noise", Post, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 获取边缘AI配置（管理员）
     * @route GET /api/admin/edge-ai/config
     *
     * 返回当前边缘AI引擎的完整配置，包括模型路径、
     * 推理参数、隐私预算上限、联邦学习参数等。
     * 需要管理员 PASETO 令牌鉴权。
     */
    ADD_METHOD_TO(EdgeAIController::getAdminConfig,
                  "/api/admin/edge-ai/config", Get, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 更新边缘AI配置（管理员）
     * @route PUT /api/admin/edge-ai/config
     *
     * 请求体：
     * @code
     * {
     *   "localModel": {
     *     "embeddingDim": 384,
     *     "maxBatchSize": 32,
     *     "timeoutMs": 5000
     *   },
     *   "privacy": {
     *     "maxEpsilon": 10.0,
     *     "defaultEpsilon": 1.0,
     *     "budgetResetIntervalHours": 24
     *   },
     *   "federated": {
     *     "minParticipants": 3,
     *     "aggregationStrategy": "fedavg",
     *     "clippingBound": 1.0
     *   },
     *   "cache": {
     *     "maxSize": 10000,
     *     "similarityThreshold": 0.85,
     *     "ttlSeconds": 3600
     *   }
     * }
     * @endcode
     *
     * 需要管理员 PASETO 令牌鉴权。
     */
    ADD_METHOD_TO(EdgeAIController::updateAdminConfig,
                  "/api/admin/edge-ai/config", Put, Options,
                  "heartlake::filters::AdminAuthFilter");

    /**
     * @brief 生成文本摘要
     * @route POST /api/edge-ai/summary
     */
    ADD_METHOD_TO(EdgeAIController::generateSummary,
                  "/api/edge-ai/summary", Post,
                  "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(EdgeAIController::lakeGodChat,
                  "/api/lake-god/chat", Post,
                  "heartlake::filters::SecurityAuditFilter");

    ADD_METHOD_TO(EdgeAIController::lakeGodHistory,
                  "/api/lake-god/history", Get,
                  "heartlake::filters::SecurityAuditFilter");

    METHOD_LIST_END

    // ==================== 公开接口处理函数 ====================

    /**
     * @brief 获取边缘AI引擎运行状态
     * @param req HTTP请求
     * @param callback 响应回调
     */
    void getStatus(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取边缘AI详细性能指标
     * @param req HTTP请求
     * @param callback 响应回调
     */
    void getMetrics(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 本地情感分析
     * @param req HTTP请求，body 需包含 "text" 字段
     * @param callback 响应回调
     */
    void analyzeLocal(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 本地内容审核
     * @param req HTTP请求，body 需包含 "text" 字段
     * @param callback 响应回调
     */
    void moderateLocal(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 获取社区实时情绪脉搏
     * @param req HTTP请求
     * @param callback 响应回调
     */
    void getEmotionPulse(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 触发联邦学习聚合
     * @param req HTTP请求，body 需包含 "round" 字段
     * @param callback 响应回调
     */
    void federatedAggregate(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 查询差分隐私预算状态
     * @param req HTTP请求
     * @param callback 响应回调
     */
    void getPrivacyBudget(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 本地向量相似度搜索
     * @param req HTTP请求，body 需包含 "query" 字段
     * @param callback 响应回调
     */
    void vectorSearch(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

    void vectorInsert(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

    void submitEmotionSample(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback);

    void getPulseHistory(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    void submitLocalModel(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    void resetPrivacyBudget(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);

    void registerNode(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

    void updateNodeStatus(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    void selectBestNode(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback);

    void quantizedForward(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);

    void addNoise(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

    void generateSummary(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);

    void lakeGodChat(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

    void lakeGodHistory(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback);

    // ==================== 管理接口处理函数 ====================

    /**
     * @brief 获取边缘AI配置（管理员）
     * @param req HTTP请求（需携带 PASETO 令牌）
     * @param callback 响应回调
     */
    void getAdminConfig(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback);

    /**
     * @brief 更新边缘AI配置（管理员）
     * @param req HTTP请求（需携带 PASETO 令牌），body 为配置 JSON
     * @param callback 响应回调
     */
    void updateAdminConfig(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);

private:
    // ==================== 内部辅助方法 ====================

    /**
     * @brief 收集各子系统健康状态
     * @return 包含各子系统状态的 JSON 对象
     */
    Json::Value collectSubsystemHealth();

    /**
     * @brief 收集语义缓存指标
     * @return 缓存命中率、大小等指标
     */
    Json::Value collectCacheMetrics();

    /**
     * @brief 收集本地嵌入服务指标
     * @return 嵌入服务吞吐量、延迟等指标
     */
    Json::Value collectEmbeddingMetrics();

    /**
     * @brief 验证并解析 JSON 请求体
     * @param req HTTP请求
     * @param[out] body 解析后的 JSON 对象
     * @param requiredFields 必需字段列表
     * @return 验证通过返回 true，否则返回 false
     */
    bool validateJsonBody(const HttpRequestPtr &req,
                          Json::Value &body,
                          const std::vector<std::string> &requiredFields);

    /**
     * @brief 验证差分隐私 epsilon 参数范围
     * @param epsilon 待验证的 epsilon 值
     * @param minVal 最小允许值（默认0.01）
     * @param maxVal 最大允许值（默认10.0）
     * @return 合法返回 true
     */
    bool validateEpsilon(double epsilon, double minVal = 0.01, double maxVal = 10.0);

    /**
     * @brief 构建联邦学习聚合结果
     * @param round 聚合轮次
     * @param participantCount 参与节点数
     * @param epsilon 使用的隐私预算
     * @param clippingBound 梯度裁剪上界
     * @return 聚合结果 JSON
     */
    Json::Value buildAggregationResult(int round,
                                       int participantCount,
                                       double epsilon,
                                       double clippingBound);

    /**
     * @brief 验证管理员配置 JSON 的合法性
     * @param config 待验证的配置 JSON
     * @param[out] errorMsg 验证失败时的错误信息
     * @return 合法返回 true
     */
    bool validateAdminConfig(const Json::Value &config, std::string &errorMsg);
};

} // namespace controllers
} // namespace heartlake
