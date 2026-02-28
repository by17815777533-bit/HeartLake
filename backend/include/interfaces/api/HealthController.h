/**
 * 健康检查控制器 - 服务状态与依赖组件可用性监控
 *
 * 提供两级健康检查：
 * - 基础检查：快速返回服务存活状态，适合负载均衡器探活
 * - 详细检查：深度探测 PostgreSQL、Redis 连接状态及系统资源，
 *   适合运维监控面板
 *
 * 两个端点均无需认证，可匿名访问。
 */

#pragma once

#include <drogon/HttpController.h>
#include <chrono>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 服务健康检查 HTTP 控制器
 *
 * @details 路由表：
 * - GET /api/health          — 基础健康检查（状态、版本、时间戳）
 * - GET /api/health/detailed — 详细健康检查（数据库、Redis、运行时间、内存）
 *
 * 基础检查响应示例：
 * @code
 * { "status": "ok", "version": "1.0.0", "timestamp": "..." }
 * @endcode
 *
 * 详细检查额外包含 database_connected、redis_connected、
 * uptime_seconds、memory_usage_mb 等字段。
 */
class HealthController : public drogon::HttpController<HealthController> {
public:
  METHOD_LIST_BEGIN

  /// 基础健康检查 — 返回服务状态、版本号和当前时间戳
  ADD_METHOD_TO(HealthController::health, "/api/health", Get);

  /// 详细健康检查 — 额外探测数据库、Redis 连接及系统资源
  ADD_METHOD_TO(HealthController::healthDetailed, "/api/health/detailed", Get);

  METHOD_LIST_END

  /**
   * @brief 基础健康检查
   * @param req HTTP 请求
   * @param callback 响应回调
   * @return JSON: status / version / timestamp
   */
  void health(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

  /**
   * @brief 详细健康检查
   * @param req HTTP 请求
   * @param callback 响应回调
   * @return JSON: 基础字段 + database / redis / uptime / memory
   */
  void healthDetailed(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

private:
  /// 进程启动时间点，用于计算 uptime
  static const std::chrono::steady_clock::time_point startTime_;

  /// 从 /proc/self/status 读取 VmRSS，返回当前进程物理内存占用（MB）
  static double getMemoryUsageMB();
};

} // namespace controllers
} // namespace heartlake
