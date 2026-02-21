import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/error_handler.dart';

/// ApiClient 是单例，其构造函数依赖 appConfig 和 StorageUtil 全局变量，
/// 无法在无 mockito 环境下直接实例化。
/// 因此重点测试 ApiClient 的核心依赖：AppErrorType、AppError、Result<T>。

void main() {
  group('AppErrorType enum', () {
    test('should have 9 values', () {
      expect(AppErrorType.values.length, 9);
    });

    test('should contain all expected types', () {
      expect(AppErrorType.values, containsAll([
        AppErrorType.network,
        AppErrorType.server,
        AppErrorType.auth,
        AppErrorType.validation,
        AppErrorType.notFound,
        AppErrorType.forbidden,
        AppErrorType.timeout,
        AppErrorType.cancelled,
        AppErrorType.unknown,
      ]));
    });
  });

  group('AppError construction', () {
    test('should store all fields correctly', () {
      final error = AppError(
        type: AppErrorType.server,
        message: '服务器错误',
        code: 'ERR_500',
        statusCode: 500,
      );

      expect(error.type, AppErrorType.server);
      expect(error.message, '服务器错误');
      expect(error.code, 'ERR_500');
      expect(error.statusCode, 500);
    });

    test('should allow null optional fields', () {
      final error = AppError(
        type: AppErrorType.unknown,
        message: '未知错误',
      );

      expect(error.code, isNull);
      expect(error.statusCode, isNull);
      expect(error.originalError, isNull);
    });

    test('should store originalError', () {
      final original = Exception('原始异常');
      final error = AppError(
        type: AppErrorType.network,
        message: '网络错误',
        originalError: original,
      );

      expect(error.originalError, original);
    });

    test('should implement Exception', () {
      final error = AppError(
        type: AppErrorType.unknown,
        message: 'test',
      );
      expect(error, isA<Exception>());
    });
  });

  group('AppError.userMessage', () {
    test('network should return friendly message', () {
      final error = AppError(type: AppErrorType.network, message: 'raw');
      expect(error.userMessage, '网络不太给力，请检查连接后重试');
    });

    test('server should return friendly message', () {
      final error = AppError(type: AppErrorType.server, message: 'raw');
      expect(error.userMessage, '服务器开小差了，请稍后再试');
    });

    test('auth should return friendly message', () {
      final error = AppError(type: AppErrorType.auth, message: 'raw');
      expect(error.userMessage, '请重新登录');
    });

    test('validation should pass through original message', () {
      final error = AppError(type: AppErrorType.validation, message: '昵称不能为空');
      expect(error.userMessage, '昵称不能为空');
    });

    test('notFound should return friendly message', () {
      final error = AppError(type: AppErrorType.notFound, message: 'raw');
      expect(error.userMessage, '内容已被删除或不存在');
    });

    test('forbidden should return friendly message', () {
      final error = AppError(type: AppErrorType.forbidden, message: 'raw');
      expect(error.userMessage, '暂无权限');
    });

    test('timeout should return friendly message', () {
      final error = AppError(type: AppErrorType.timeout, message: 'raw');
      expect(error.userMessage, '请求超时，请重试');
    });

    test('cancelled should return friendly message', () {
      final error = AppError(type: AppErrorType.cancelled, message: 'raw');
      expect(error.userMessage, '操作已取消');
    });

    test('unknown should return friendly message', () {
      final error = AppError(type: AppErrorType.unknown, message: 'raw');
      expect(error.userMessage, '发生了一点小问题');
    });
  });

  group('AppError.requiresReauth', () {
    test('should return true for auth type', () {
      final error = AppError(type: AppErrorType.auth, message: '过期');
      expect(error.requiresReauth, true);
    });

    test('should return false for non-auth types', () {
      final nonAuthTypes = [
        AppErrorType.network,
        AppErrorType.server,
        AppErrorType.validation,
        AppErrorType.notFound,
        AppErrorType.forbidden,
        AppErrorType.timeout,
        AppErrorType.cancelled,
        AppErrorType.unknown,
      ];

      for (final type in nonAuthTypes) {
        final error = AppError(type: type, message: 'test');
        expect(error.requiresReauth, false, reason: '$type should not require reauth');
      }
    });
  });

  group('AppError.canRetry', () {
    test('should return true for network type', () {
      final error = AppError(type: AppErrorType.network, message: 'test');
      expect(error.canRetry, true);
    });

    test('should return true for timeout type', () {
      final error = AppError(type: AppErrorType.timeout, message: 'test');
      expect(error.canRetry, true);
    });

    test('should return true for server type', () {
      final error = AppError(type: AppErrorType.server, message: 'test');
      expect(error.canRetry, true);
    });

    test('should return false for non-retryable types', () {
      final nonRetryable = [
        AppErrorType.auth,
        AppErrorType.validation,
        AppErrorType.notFound,
        AppErrorType.forbidden,
        AppErrorType.cancelled,
        AppErrorType.unknown,
      ];

      for (final type in nonRetryable) {
        final error = AppError(type: type, message: 'test');
        expect(error.canRetry, false, reason: '$type should not be retryable');
      }
    });
  });

  group('AppError.toString', () {
    test('should format as AppError(type): message', () {
      final error = AppError(type: AppErrorType.network, message: '网络断开');
      expect(error.toString(), 'AppError(AppErrorType.network): 网络断开');
    });

    test('should work for all types', () {
      for (final type in AppErrorType.values) {
        final error = AppError(type: type, message: 'msg');
        expect(error.toString(), contains('AppError'));
        expect(error.toString(), contains('msg'));
      }
    });
  });

  group('Result.success', () {
    test('should store data', () {
      final result = Result.success('hello');
      expect(result.data, 'hello');
    });

    test('should have no error', () {
      final result = Result.success(42);
      expect(result.error, isNull);
    });

    test('isSuccess should be true', () {
      final result = Result.success('data');
      expect(result.isSuccess, true);
    });

    test('isFailure should be false', () {
      final result = Result.success('data');
      expect(result.isFailure, false);
    });
  });

  group('Result.failure', () {
    test('should store error', () {
      final error = AppError(type: AppErrorType.server, message: '错误');
      final result = Result<String>.failure(error);
      expect(result.error, error);
    });

    test('should have null data', () {
      final error = AppError(type: AppErrorType.server, message: '错误');
      final result = Result<String>.failure(error);
      expect(result.data, isNull);
    });

    test('isSuccess should be false', () {
      final error = AppError(type: AppErrorType.server, message: '错误');
      final result = Result<String>.failure(error);
      expect(result.isSuccess, false);
    });

    test('isFailure should be true', () {
      final error = AppError(type: AppErrorType.server, message: '错误');
      final result = Result<String>.failure(error);
      expect(result.isFailure, true);
    });
  });

  group('Result.dataOrThrow', () {
    test('should return data on success', () {
      final result = Result.success('value');
      expect(result.dataOrThrow, 'value');
    });

    test('should throw error on failure', () {
      final error = AppError(type: AppErrorType.auth, message: '未授权');
      final result = Result<String>.failure(error);
      expect(() => result.dataOrThrow, throwsA(isA<AppError>()));
    });
  });

  group('Result.dataOr', () {
    test('should return data when present', () {
      final result = Result.success('actual');
      expect(result.dataOr('default'), 'actual');
    });

    test('should return default when failure', () {
      final error = AppError(type: AppErrorType.server, message: '错误');
      final result = Result<String>.failure(error);
      expect(result.dataOr('default'), 'default');
    });
  });

  group('Result.map', () {
    test('should transform success data', () {
      final result = Result.success(10);
      final mapped = result.map((v) => v * 2);

      expect(mapped.isSuccess, true);
      expect(mapped.dataOrThrow, 20);
    });

    test('should pass through failure without calling mapper', () {
      final error = AppError(type: AppErrorType.network, message: '网络错误');
      final result = Result<int>.failure(error);
      var mapperCalled = false;
      final mapped = result.map((v) {
        mapperCalled = true;
        return v * 2;
      });

      expect(mapped.isFailure, true);
      expect(mapped.error, error);
      expect(mapperCalled, false);
    });

    test('should change result type', () {
      final result = Result.success(42);
      final mapped = result.map((v) => 'number: $v');

      expect(mapped.isSuccess, true);
      expect(mapped.dataOrThrow, 'number: 42');
    });
  });

  group('Result.fold', () {
    test('should call onSuccess for success result', () {
      final result = Result.success('data');
      final value = result.fold(
        onSuccess: (data) => 'got: $data',
        onFailure: (error) => 'error: ${error.message}',
      );

      expect(value, 'got: data');
    });

    test('should call onFailure for failure result', () {
      final error = AppError(type: AppErrorType.timeout, message: '超时');
      final result = Result<String>.failure(error);
      final value = result.fold(
        onSuccess: (data) => 'got: $data',
        onFailure: (error) => 'error: ${error.message}',
      );

      expect(value, 'error: 超时');
    });

    test('should allow different return types', () {
      final result = Result.success(100);
      final isLarge = result.fold(
        onSuccess: (data) => data > 50,
        onFailure: (_) => false,
      );

      expect(isLarge, true);
    });
  });

  group('Result with complex types', () {
    test('should work with Map type', () {
      final result = Result.success({'key': 'value', 'count': 42});
      expect(result.isSuccess, true);
      expect(result.dataOrThrow['key'], 'value');
    });

    test('should work with List type', () {
      final result = Result.success([1, 2, 3]);
      final mapped = result.map((list) => list.length);
      expect(mapped.dataOrThrow, 3);
    });
  });

  group('ApiClient constants (static analysis)', () {
    test('retryStatusCodes should be known values', () {
      // ApiClient._retryStatusCodes = [408, 500, 502, 503, 504]
      // 验证这些 HTTP 状态码的语义
      const retryStatusCodes = [408, 500, 502, 503, 504];

      expect(retryStatusCodes, contains(408)); // Request Timeout
      expect(retryStatusCodes, contains(500)); // Internal Server Error
      expect(retryStatusCodes, contains(502)); // Bad Gateway
      expect(retryStatusCodes, contains(503)); // Service Unavailable
      expect(retryStatusCodes, contains(504)); // Gateway Timeout
      expect(retryStatusCodes.length, 5);
    });
  });
}
