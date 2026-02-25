import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/circuit_breaker.dart';

/// StoneService 的核心熔断降级逻辑测试。
/// 由于 StoneService 继承 BaseService（依赖 ApiClient 单例 + Dio），
/// 无法在无网络环境下直接实例化。
/// 因此提取 StoneService 的熔断+缓存降级模式，用纯 CircuitBreaker 验证。

/// 简易内存缓存，模拟 CacheService
class FakeCache {
  final Map<String, dynamic> _store = {};

  void set(String key, dynamic value) {
    _store[key] = value;
  }

  T? get<T>(String key) {
    final v = _store[key];
    return v is T ? v : null;
  }
}

void main() {
  late CircuitBreaker breaker;
  late FakeCache cache;

  setUp(() {
    breaker = CircuitBreaker(
      name: 'stones',
      failureThreshold: 3,
      resetTimeout: const Duration(milliseconds: 200),
      callTimeout: const Duration(milliseconds: 100),
    );
    cache = FakeCache();
  });

  /// 模拟 StoneService.getStones 的熔断+缓存降级模式
  Future<Map<String, dynamic>> getStonesWithBreaker({
    required Future<Map<String, dynamic>> Function() apiCall,
    String cacheKey = 'stones_1_20_latest',
  }) async {
    try {
      final result = await breaker.call(() async {
        final response = await apiCall();
        // 成功时写入缓存
        cache.set(cacheKey, response);
        return response;
      });
      return result;
    } catch (_) {
      // 熔断或异常，降级到缓存
      final cached = cache.get<Map<String, dynamic>>(cacheKey);
      if (cached != null) {
        return {...cached, 'from_cache': true};
      }
      return {'success': false, 'message': '服务暂时不可用，请稍后重试'};
    }
  }

  group('StoneService breaker pattern - normal flow', () {
    test('should return API result when service is healthy', () async {
      final result = await getStonesWithBreaker(
        apiCall: () async => {'success': true, 'stones': ['s1', 's2']},
      );

      expect(result['success'], true);
      expect(result['stones'], ['s1', 's2']);
      expect(result.containsKey('from_cache'), false);
      expect(breaker.state, CircuitState.closed);
    });

    test('should cache successful results', () async {
      await getStonesWithBreaker(
        apiCall: () async => {'success': true, 'stones': ['s1']},
      );

      final cached = cache.get<Map<String, dynamic>>('stones_1_20_latest');
      expect(cached, isNotNull);
      expect(cached!['success'], true);
    });
  });

  group('StoneService breaker pattern - degradation', () {
    test('should fallback to cache when breaker opens', () async {
      // First: successful call populates cache
      await getStonesWithBreaker(
        apiCall: () async => {'success': true, 'stones': ['cached_s1']},
      );

      // Trip the breaker with 3 failures
      for (var i = 0; i < 3; i++) {
        await getStonesWithBreaker(
          apiCall: () async => throw Exception('server down'),
        );
      }

      expect(breaker.state, CircuitState.open);

      // Next call should return cached data
      final result = await getStonesWithBreaker(
        apiCall: () async => throw Exception('should not be called'),
      );

      expect(result['success'], true);
      expect(result['stones'], ['cached_s1']);
      expect(result['from_cache'], true);
    });

    test('should return error when breaker open and no cache', () async {
      // Trip breaker without any prior successful call
      for (var i = 0; i < 3; i++) {
        await getStonesWithBreaker(
          apiCall: () async => throw Exception('fail'),
        );
      }

      final result = await getStonesWithBreaker(
        apiCall: () async => throw Exception('still down'),
      );

      expect(result['success'], false);
      expect(result['message'], contains('服务暂时不可用'));
    });
  });

  group('StoneService breaker pattern - recovery', () {
    test('should recover and fetch fresh data after reset timeout', () async {
      // Populate cache
      await getStonesWithBreaker(
        apiCall: () async => {'success': true, 'stones': ['old']},
      );

      // Trip breaker
      for (var i = 0; i < 3; i++) {
        await getStonesWithBreaker(
          apiCall: () async => throw Exception('fail'),
        );
      }
      expect(breaker.state, CircuitState.open);

      // Wait for reset timeout
      await Future.delayed(const Duration(milliseconds: 250));

      // Should allow probe call (halfOpen)
      final result = await getStonesWithBreaker(
        apiCall: () async => {'success': true, 'stones': ['fresh']},
      );

      expect(result['success'], true);
      expect(result['stones'], ['fresh']);
      expect(result.containsKey('from_cache'), false);
    });
  });

  group('StoneService breaker pattern - timeout', () {
    test('should treat timeout as failure', () async {
      for (var i = 0; i < 3; i++) {
        await getStonesWithBreaker(
          apiCall: () async {
            await Future.delayed(const Duration(milliseconds: 200));
            return {'success': true, 'stones': []};
          },
        );
      }

      expect(breaker.state, CircuitState.open);
    });
  });

  group('StoneService breaker pattern - different cache keys', () {
    test('should use separate cache per page/sort', () async {
      // Cache page 1
      await getStonesWithBreaker(
        apiCall: () async => {'success': true, 'stones': ['p1']},
        cacheKey: 'stones_1_20_latest',
      );

      // Cache page 2
      await getStonesWithBreaker(
        apiCall: () async => {'success': true, 'stones': ['p2']},
        cacheKey: 'stones_2_20_latest',
      );

      // Trip breaker
      for (var i = 0; i < 3; i++) {
        await getStonesWithBreaker(
          apiCall: () async => throw Exception('fail'),
          cacheKey: 'stones_1_20_latest',
        );
      }

      // Page 1 cache
      final r1 = await getStonesWithBreaker(
        apiCall: () async => throw Exception('down'),
        cacheKey: 'stones_1_20_latest',
      );
      expect(r1['stones'], ['p1']);
      expect(r1['from_cache'], true);

      // Page 2 cache
      final r2 = await getStonesWithBreaker(
        apiCall: () async => throw Exception('down'),
        cacheKey: 'stones_2_20_latest',
      );
      expect(r2['stones'], ['p2']);
      expect(r2['from_cache'], true);
    });
  });
}
