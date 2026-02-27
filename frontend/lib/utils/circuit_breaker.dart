// 熔断器模式 - 防止级联故障，支持自动恢复

library;

import 'dart:async';

/// 熔断器状态
enum CircuitState {
  /// 正常通行
  closed,

  /// 熔断打开，拒绝所有请求
  open,

  /// 半开，允许少量探测请求
  halfOpen,
}

/// 熔断器打开时抛出的异常
class CircuitBreakerOpenException implements Exception {
  final String endpoint;
  final Duration retryAfter;

  CircuitBreakerOpenException(this.endpoint, this.retryAfter);

  @override
  String toString() =>
      'CircuitBreakerOpenException: $endpoint 熔断中，${retryAfter.inSeconds}秒后重试';
}

/// 熔断器
///
/// 每个端点独立的熔断器实例，支持：
/// - 失败计数与阈值
/// - 自动恢复（open -> halfOpen -> closed）
/// - 调用超时
/// - 可选的降级回调
class CircuitBreaker {
  final String name;
  final int failureThreshold;
  final Duration resetTimeout;
  final Duration callTimeout;

  CircuitState _state = CircuitState.closed;
  int _failureCount = 0;
  DateTime? _lastFailureTime;
  int _halfOpenSuccessCount = 0;
  static const int _halfOpenSuccessThreshold = 2;

  CircuitState get state => _state;
  int get failureCount => _failureCount;

  CircuitBreaker({
    required this.name,
    this.failureThreshold = 5,
    this.resetTimeout = const Duration(seconds: 30),
    this.callTimeout = const Duration(seconds: 10),
  });

  /// 通过熔断器执行调用
  ///
  /// [action] 实际的异步操作
  /// [fallback] 熔断打开时的降级回调（可选）
  Future<T> call<T>(
    Future<T> Function() action, {
    Future<T> Function()? fallback,
  }) async {
    if (_state == CircuitState.open) {
      if (_shouldAttemptReset()) {
        _state = CircuitState.halfOpen;
        _halfOpenSuccessCount = 0;
      } else {
        if (fallback != null) return fallback();
        throw CircuitBreakerOpenException(name, _remainingResetTime());
      }
    }

    try {
      final result = await action().timeout(callTimeout);
      _onSuccess();
      return result;
    } on TimeoutException {
      _onFailure();
      if (fallback != null && _state == CircuitState.open) return fallback();
      rethrow;
    } catch (e) {
      _onFailure();
      if (fallback != null && _state == CircuitState.open) return fallback();
      rethrow;
    }
  }

  void _onSuccess() {
    if (_state == CircuitState.halfOpen) {
      _halfOpenSuccessCount++;
      if (_halfOpenSuccessCount >= _halfOpenSuccessThreshold) {
        _reset();
      }
    } else {
      _failureCount = 0;
    }
  }

  void _onFailure() {
    _failureCount++;
    _lastFailureTime = DateTime.now();
    if (_failureCount >= failureThreshold) {
      _state = CircuitState.open;
    }
  }

  bool _shouldAttemptReset() {
    if (_lastFailureTime == null) return true;
    return DateTime.now().difference(_lastFailureTime!) >= resetTimeout;
  }

  Duration _remainingResetTime() {
    if (_lastFailureTime == null) return Duration.zero;
    final elapsed = DateTime.now().difference(_lastFailureTime!);
    final remaining = resetTimeout - elapsed;
    return remaining.isNegative ? Duration.zero : remaining;
  }

  void _reset() {
    _state = CircuitState.closed;
    _failureCount = 0;
    _halfOpenSuccessCount = 0;
    _lastFailureTime = null;
  }

  /// 手动重置熔断器
  void reset() => _reset();
}
