// @file resilient_api_client.dart
// @brief 弹性API客户端 - 为每个端点提供独立熔断器 + 缓存降级

library;

import 'dart:async';
import 'package:flutter/foundation.dart';
import '../../utils/circuit_breaker.dart';
import 'api_client.dart';
import 'cache_service.dart';

/// 弹性API客户端
///
/// 在 ApiClient 之上封装：
/// - 每个端点独立的熔断器
/// - 熔断时自动降级到缓存
class ResilientApiClient {
  final ApiClient _client = ApiClient();
  final CacheService _cache = CacheService();
  final Map<String, CircuitBreaker> _breakers = {};

  /// 获取或创建端点对应的熔断器
  CircuitBreaker _breakerFor(String endpoint) {
    return _breakers.putIfAbsent(
      endpoint,
      () => CircuitBreaker(
        name: endpoint,
        failureThreshold: 3,
        resetTimeout: const Duration(seconds: 30),
        callTimeout: const Duration(seconds: 15),
      ),
    );
  }

  /// 带熔断器的GET请求，熔断时降级到缓存
  Future<dynamic> get(
    String path, {
    Map<String, dynamic>? queryParameters,
    String? cacheKey,
    Duration cacheTtl = const Duration(minutes: 5),
  }) async {
    final breaker = _breakerFor(path);
    final key = cacheKey ?? 'resilient_$path';

    try {
      final response = await breaker.call(
        () => _client.get(path, queryParameters: queryParameters),
        fallback: () async {
          final cached = _cache.get<dynamic>(key);
          if (cached != null) {
            if (kDebugMode) {
              debugPrint('[ResilientApiClient] 熔断降级到缓存: $path');
            }
            return cached;
          }
          throw CircuitBreakerOpenException(path, breaker.state == CircuitState.open
              ? const Duration(seconds: 30)
              : Duration.zero);
        },
      );

      // 成功时更新缓存
      final data = response is Map ? response : response?.data;
      if (data != null) {
        _cache.set(key, data, ttl: cacheTtl);
      }
      return response;
    } on CircuitBreakerOpenException {
      // 熔断且无缓存
      rethrow;
    } catch (e) {
      // 非熔断异常，尝试缓存降级
      final cached = _cache.get<dynamic>(key);
      if (cached != null) {
        if (kDebugMode) {
          debugPrint('[ResilientApiClient] 请求失败，降级到缓存: $path');
        }
        return cached;
      }
      rethrow;
    }
  }

  /// 带熔断器的POST请求（写操作不降级到缓存）
  Future<dynamic> post(
    String path, {
    dynamic data,
  }) async {
    final breaker = _breakerFor(path);
    final response = await breaker.call(
      () => _client.post(path, data: data),
    );
    return response;
  }

  /// 获取端点熔断器状态
  CircuitState stateOf(String endpoint) {
    return _breakers[endpoint]?.state ?? CircuitState.closed;
  }

  /// 重置指定端点的熔断器
  void resetBreaker(String endpoint) {
    _breakers[endpoint]?.reset();
  }

  /// 重置所有熔断器
  void resetAll() {
    for (final breaker in _breakers.values) {
      breaker.reset();
    }
  }
}
