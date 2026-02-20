/**
 * @file HealthController.h
 * @brief 健康检查控制器 - 服务状态监控
 * @author 白洋
 * @date 2025-02-15
 * @copyright Copyright (c) 2025 HeartLake. All rights reserved.
 */

#pragma once

#include <drogon/HttpController.h>
#include <chrono>

using namespace drogon;

namespace heartlake {
namespace controllers {

/**
 * @brief 健康检查HTTP控制器
 *
 * 提供服务健康状态检查：
 * - 基础健康检查（状态、版本、时间戳）
 * - 详细健康检查（数据库、Redis、运行时间、内存）
 */
class HealthController : public drogon::HttpController<HealthController> {
public:
  METHOD_LIST_BEGIN

  /// 基础健康检查
  ADD_METHOD_TO(HealthController::health, "/api/health", Get);

  /// 详细健康检查（含数据库、Redis、系统信息）
  ADD_METHOD_TO(HealthController::healthDetailed, "/api/health/detailed", Get);

  METHOD_LIST_END

  void health(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

  void healthDetailed(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

private:
  /// 服务启动时间
  static const std::chrono::steady_clock::time_point startTime_;

  /// 获取当前内存使用量（MB），从 /proc/self/status 读取 VmRSS
  static double getMemoryUsageMB();
};

} // namespace controllers
} // namespace heartlake
