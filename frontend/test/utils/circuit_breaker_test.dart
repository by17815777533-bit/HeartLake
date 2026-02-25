import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/circuit_breaker.dart';

void main() {
  late CircuitBreaker breaker;

  setUp(() {
    breaker = CircuitBreaker(
      name: 'test',
      failureThreshold: 3,
      resetTimeout: const Duration(milliseconds: 200),
      callTimeout: const Duration(milliseconds: 100),
    );
  });

  group('CircuitBreaker initial state', () {
    test('should start in closed state', () {
      expect(breaker.state, CircuitState.closed);
      expect(breaker.failureCount, 0);
    });
  });

  group('Closed state - normal calls', () {
    test('should execute action and return result when closed', () async {
      final result = await breaker.call(() async => 42);
      expect(result, 42);
      expect(breaker.state, CircuitState.closed);
      expect(breaker.failureCount, 0);
    });

    test('should reset failure count on success', () async {
      // Cause 1 failure (below threshold)
      try {
        await breaker.call(() async => throw Exception('fail'));
      } catch (_) {}
      expect(breaker.failureCount, 1);

      // Success should reset
      await breaker.call(() async => 'ok');
      expect(breaker.failureCount, 0);
    });

    test('should propagate exceptions from action', () async {
      expect(
        () => breaker.call(() async => throw StateError('boom')),
        throwsA(isA<StateError>()),
      );
    });
  });

  group('Closed -> Open transition', () {
    test('should open after failureThreshold consecutive failures', () async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail $i'));
        } catch (_) {}
      }

      expect(breaker.state, CircuitState.open);
      expect(breaker.failureCount, 3);
    });

    test('should open on timeout failures', () async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async {
            await Future.delayed(const Duration(milliseconds: 200));
            return 'too slow';
          });
        } catch (_) {}
      }

      expect(breaker.state, CircuitState.open);
    });
  });

  group('Open state - fast fail', () {
    Future<void> tripBreaker() async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }
    }

    test('should throw CircuitBreakerOpenException when open', () async {
      await tripBreaker();
      expect(breaker.state, CircuitState.open);

      expect(
        () => breaker.call(() async => 'should not run'),
        throwsA(isA<CircuitBreakerOpenException>()),
      );
    });

    test('CircuitBreakerOpenException should contain endpoint name', () async {
      await tripBreaker();

      try {
        await breaker.call(() async => 'nope');
        fail('should have thrown');
      } on CircuitBreakerOpenException catch (e) {
        expect(e.endpoint, 'test');
        expect(e.toString(), contains('test'));
        expect(e.toString(), contains('熔断中'));
      }
    });
  });

  group('Open -> HalfOpen transition (timeout)', () {
    Future<void> tripBreaker() async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }
    }

    test('should transition to halfOpen after resetTimeout', () async {
      await tripBreaker();
      expect(breaker.state, CircuitState.open);

      // Wait for resetTimeout to elapse
      await Future.delayed(const Duration(milliseconds: 250));

      // Next call should be allowed (transitions to halfOpen internally)
      final result = await breaker.call(() async => 'probe');
      // After 1 success in halfOpen, still halfOpen (need 2 successes)
      expect(breaker.state, CircuitState.halfOpen);
      expect(result, 'probe');
    });
  });

  group('HalfOpen -> Closed (success recovery)', () {
    Future<void> tripAndWait() async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }
      await Future.delayed(const Duration(milliseconds: 250));
    }

    test('should recover to closed after 2 consecutive successes in halfOpen',
        () async {
      await tripAndWait();

      // First success in halfOpen
      await breaker.call(() async => 'ok1');
      expect(breaker.state, CircuitState.halfOpen);

      // Second success -> should close
      await breaker.call(() async => 'ok2');
      expect(breaker.state, CircuitState.closed);
      expect(breaker.failureCount, 0);
    });
  });

  group('HalfOpen -> Open (failure)', () {
    Future<void> tripAndWait() async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }
      await Future.delayed(const Duration(milliseconds: 250));
    }

    test('should go back to open on failure in halfOpen', () async {
      await tripAndWait();

      // First call transitions to halfOpen, then fails
      try {
        await breaker.call(() async => throw Exception('halfOpen fail'));
      } catch (_) {}

      // failureCount is now 4 (3 from trip + 1 from halfOpen fail)
      // which is >= threshold, so state goes back to open
      expect(breaker.state, CircuitState.open);
    });
  });

  group('Fallback (degradation)', () {
    Future<void> tripBreaker() async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }
    }

    test('should call fallback when open and not yet reset', () async {
      await tripBreaker();
      expect(breaker.state, CircuitState.open);

      final result = await breaker.call(
        () async => 'should not run',
        fallback: () async => 'fallback value',
      );

      expect(result, 'fallback value');
    });

    test('should call fallback when action fails and breaker opens', () async {
      // Cause 2 failures first
      for (var i = 0; i < 2; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }
      expect(breaker.state, CircuitState.closed);

      // 3rd failure opens the breaker, fallback should be called
      final result = await breaker.call(
        () async => throw Exception('3rd fail'),
        fallback: () async => 'degraded',
      );

      expect(result, 'degraded');
      expect(breaker.state, CircuitState.open);
    });

    test('should call fallback on timeout when breaker opens', () async {
      for (var i = 0; i < 2; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }

      final result = await breaker.call(
        () async {
          await Future.delayed(const Duration(milliseconds: 200));
          return 'too slow';
        },
        fallback: () async => 'timeout fallback',
      );

      expect(result, 'timeout fallback');
      expect(breaker.state, CircuitState.open);
    });
  });

  group('reset() method', () {
    test('should reset breaker to initial state', () async {
      // Trip the breaker
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }
      expect(breaker.state, CircuitState.open);

      breaker.reset();

      expect(breaker.state, CircuitState.closed);
      expect(breaker.failureCount, 0);
    });

    test('should allow calls again after reset', () async {
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }

      breaker.reset();

      final result = await breaker.call(() async => 'recovered');
      expect(result, 'recovered');
      expect(breaker.state, CircuitState.closed);
    });
  });

  group('CircuitBreakerOpenException', () {
    test('should store endpoint and retryAfter', () {
      final ex = CircuitBreakerOpenException(
        'my-endpoint',
        const Duration(seconds: 15),
      );
      expect(ex.endpoint, 'my-endpoint');
      expect(ex.retryAfter, const Duration(seconds: 15));
    });

    test('toString should include endpoint and seconds', () {
      final ex = CircuitBreakerOpenException(
        'stones',
        const Duration(seconds: 30),
      );
      final str = ex.toString();
      expect(str, contains('stones'));
      expect(str, contains('30'));
    });
  });

  group('Edge cases', () {
    test('multiple breakers should be independent', () async {
      final breaker2 = CircuitBreaker(
        name: 'other',
        failureThreshold: 3,
        resetTimeout: const Duration(milliseconds: 200),
        callTimeout: const Duration(milliseconds: 100),
      );

      // Trip breaker1
      for (var i = 0; i < 3; i++) {
        try {
          await breaker.call(() async => throw Exception('fail'));
        } catch (_) {}
      }

      expect(breaker.state, CircuitState.open);
      expect(breaker2.state, CircuitState.closed);

      // breaker2 should still work
      final result = await breaker2.call(() async => 'ok');
      expect(result, 'ok');
    });

    test('should handle async action correctly', () async {
      final result = await breaker.call(() async {
        await Future.delayed(const Duration(milliseconds: 10));
        return 'async result';
      });
      expect(result, 'async result');
    });
  });
}
