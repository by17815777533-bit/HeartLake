/**
 * 健康检查控制器实现
 * @author 白洋
 * @date 2025-02-15
 * @copyright Copyright (c) 2025 HeartLake. All rights reserved.
 */

#include "interfaces/api/HealthController.h"
#include <drogon/drogon.h>
#include <fstream>
#include <sstream>
#include <string>

namespace heartlake {
namespace controllers {

/// 记录服务启动时间
const std::chrono::steady_clock::time_point HealthController::startTime_ =
    std::chrono::steady_clock::now();

/**
 * 基础健康检查
 * 返回服务状态、版本号和当前时间戳
 */
void HealthController::health(
    const HttpRequestPtr &/*req*/,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  Json::Value result;
  result["status"] = "ok";
  result["version"] = "3.0";
  result["timestamp"] = trantor::Date::now().toFormattedString(false);

  auto resp = HttpResponse::newHttpJsonResponse(result);
  resp->setStatusCode(k200OK);
  callback(resp);
}

/**
 * 详细健康检查
 * 包含数据库状态、Redis状态、运行时间和内存使用
 */
void HealthController::healthDetailed(
    const HttpRequestPtr &/*req*/,
    std::function<void(const HttpResponsePtr &)> &&callback) {

  // 使用 shared_ptr 在异步回调间共享结果
  auto result = std::make_shared<Json::Value>();
  (*result)["status"] = "ok";
  (*result)["version"] = "3.0";
  (*result)["timestamp"] = trantor::Date::now().toFormattedString(false);

  // 计算运行时间（秒）
  auto now = std::chrono::steady_clock::now();
  auto uptimeSeconds =
      std::chrono::duration_cast<std::chrono::seconds>(now - startTime_)
          .count();
  (*result)["uptime_seconds"] = static_cast<Json::Int64>(uptimeSeconds);

  // 获取内存使用量
  (*result)["memory_usage_mb"] = getMemoryUsageMB();

  // 检查数据库连接
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr &)>>(
          std::move(callback));

  try {
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT 1",
        [result, callbackPtr](const drogon::orm::Result &) {
          // 数据库正常
          (*result)["db_status"] = "ok";

          // 检查 Redis 连接
          try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [result, callbackPtr](const drogon::nosql::RedisResult &) {
                  // Redis 正常
                  (*result)["redis_status"] = "ok";

                  auto resp = HttpResponse::newHttpJsonResponse(*result);
                  resp->setStatusCode(k200OK);
                  (*callbackPtr)(resp);
                },
                [result, callbackPtr](const std::exception &) {
                  // Redis 异常
                  (*result)["redis_status"] = "error";
                  (*result)["redis_error"] = "connection_failed";
                  (*result)["status"] = "degraded";

                  auto resp = HttpResponse::newHttpJsonResponse(*result);
                  resp->setStatusCode(k200OK);
                  (*callbackPtr)(resp);
                },
                "PING");
          } catch (const std::exception &e) {
            // Redis 客户端获取失败
            (*result)["redis_status"] = "unavailable";
            (*result)["redis_error"] = "connection_failed";
            (*result)["status"] = "degraded";

            auto resp = HttpResponse::newHttpJsonResponse(*result);
            resp->setStatusCode(k200OK);
            (*callbackPtr)(resp);
          }
        },
        [result, callbackPtr](const drogon::orm::DrogonDbException &/*e*/) {
          // 数据库异常
          (*result)["db_status"] = "error";
          (*result)["db_error"] = "connection_failed";
          (*result)["status"] = "degraded";

          // 即使数据库挂了也尝试检查 Redis
          try {
            auto redisClient = drogon::app().getRedisClient("default");
            redisClient->execCommandAsync(
                [result, callbackPtr](const drogon::nosql::RedisResult &) {
                  (*result)["redis_status"] = "ok";

                  auto resp = HttpResponse::newHttpJsonResponse(*result);
                  resp->setStatusCode(k200OK);
                  (*callbackPtr)(resp);
                },
                [result, callbackPtr](const std::exception &) {
                  (*result)["redis_status"] = "error";
                  (*result)["redis_error"] = "connection_failed";

                  auto resp = HttpResponse::newHttpJsonResponse(*result);
                  resp->setStatusCode(k200OK);
                  (*callbackPtr)(resp);
                },
                "PING");
          } catch (const std::exception &ex) {
            (*result)["redis_status"] = "unavailable";
            (*result)["redis_error"] = "connection_failed";

            auto resp = HttpResponse::newHttpJsonResponse(*result);
            resp->setStatusCode(k200OK);
            (*callbackPtr)(resp);
          }
        });
  } catch (const std::exception &e) {
    // 数据库客户端获取失败
    (*result)["db_status"] = "unavailable";
    (*result)["db_error"] = "connection_failed";
    (*result)["status"] = "degraded";

    // 仍然尝试检查 Redis
    try {
      auto redisClient = drogon::app().getRedisClient("default");
      redisClient->execCommandAsync(
          [result, callbackPtr](const drogon::nosql::RedisResult &) {
            (*result)["redis_status"] = "ok";

            auto resp = HttpResponse::newHttpJsonResponse(*result);
            resp->setStatusCode(k200OK);
            (*callbackPtr)(resp);
          },
          [result, callbackPtr](const std::exception &) {
            (*result)["redis_status"] = "error";
            (*result)["redis_error"] = "connection_failed";

            auto resp = HttpResponse::newHttpJsonResponse(*result);
            resp->setStatusCode(k200OK);
            (*callbackPtr)(resp);
          },
          "PING");
    } catch (const std::exception &ex) {
      (*result)["redis_status"] = "unavailable";
      (*result)["redis_error"] = "connection_failed";

      auto resp = HttpResponse::newHttpJsonResponse(*result);
      resp->setStatusCode(k200OK);
      (*callbackPtr)(resp);
    }
  }
}

/**
 * 从 /proc/self/status 读取 VmRSS 获取内存使用量
 * @return 内存使用量（MB），读取失败返回 -1.0
 */
double HealthController::getMemoryUsageMB() {
  std::ifstream statusFile("/proc/self/status");
  if (!statusFile.is_open()) {
    return -1.0;
  }

  std::string line;
  while (std::getline(statusFile, line)) {
    if (line.find("VmRSS:") == 0) {
      std::istringstream iss(line);
      std::string label;
      long valueKB = 0;
      iss >> label >> valueKB;
      // VmRSS 单位是 kB，转换为 MB
      return static_cast<double>(valueKB) / 1024.0;
    }
  }

  return -1.0;
}

} // namespace controllers
} // namespace heartlake
