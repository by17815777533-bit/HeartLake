/**
 * @brief 请求链路追踪中间件 - OpenTelemetry 风格的分布式追踪
 *
 * @details
 * 为每个 HTTP 请求生成或传播分布式追踪上下文：
 * - 读取上游传入的 X-Trace-Id，若不存在则生成新的 128-bit trace ID
 * - 为当前请求生成 64-bit span ID
 * - 将 trace_id / span_id 注入 request attributes，供下游 Controller 使用
 * - 在响应头中返回 X-Trace-Id 和 X-Span-Id，便于客户端关联日志
 * - 记录请求耗时，输出结构化追踪日志
 *
 * @note 应挂载在过滤器链的最前端，确保所有请求都被追踪。
 *       ID 生成使用 thread_local mt19937_64，避免多线程竞争。
 */

#pragma once

#include <drogon/HttpFilter.h>
#include <string>
#include <random>
#include <chrono>

namespace heartlake {
namespace filters {

/**
 * @brief 分布式链路追踪过滤器
 *
 * @details 在 doFilter 中包装下游回调，计算请求处理耗时，
 * 并在响应返回前注入追踪头。追踪数据可与 ELK / Jaeger 等
 * 可观测性平台对接。
 */
class TraceMiddleware : public drogon::HttpFilter<TraceMiddleware> {
public:
    TraceMiddleware() = default;

    /**
     * @brief 执行链路追踪逻辑
     *
     * @param req 入站 HTTP 请求
     * @param fcb 过滤失败回调
     * @param fccb 过滤通过回调 — 包装后注入追踪上下文
     */
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;

    /**
     * @brief 生成 32 字符十六进制 trace ID（128-bit）
     * @return 如 "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6"
     */
    static std::string generateTraceId();

    /**
     * @brief 生成 16 字符十六进制 span ID（64-bit）
     * @return 如 "a1b2c3d4e5f6a7b8"
     */
    static std::string generateSpanId();

private:
    /// 线程局部随机数生成器，避免加锁开销
    static thread_local std::mt19937_64 rng_;
};

} // namespace filters
} // namespace heartlake
