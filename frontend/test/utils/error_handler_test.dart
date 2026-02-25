import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/error_handler.dart';

/// ErrorHandler 和 AppError 的扩展测试。

void main() {
  // ==================== AppError 扩展测试 ====================
  group('AppError types coverage', () {
    test('network error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.network, message: 'raw');
      expect(e.userMessage, contains('网络'));
    });

    test('server error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.server, message: 'raw');
      expect(e.userMessage, contains('服务'));
    });

    test('auth error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.auth, message: 'raw');
      expect(e.userMessage, contains('登录'));
    });

    test('validation error should return raw message as userMessage', () {
      final e = AppError(type: AppErrorType.validation, message: '参数格式错误');
      expect(e.userMessage, '参数格式错误');
    });

    test('notFound error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.notFound, message: 'raw');
      expect(e.userMessage, contains('不存在'));
    });

    test('forbidden error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.forbidden, message: 'raw');
      expect(e.userMessage, contains('权限'));
    });

    test('timeout error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.timeout, message: 'raw');
      expect(e.userMessage, contains('超时'));
    });

    test('cancelled error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.cancelled, message: 'raw');
      expect(e.userMessage, contains('取消'));
    });

    test('unknown error should have correct userMessage', () {
      final e = AppError(type: AppErrorType.unknown, message: 'raw');
      expect(e.userMessage, isNotEmpty);
    });
  });

  group('AppError toString', () {
    test('should include type and message', () {
      final e = AppError(type: AppErrorType.server, message: '服务器错误');
      final str = e.toString();
      expect(str, contains('server'));
      expect(str, contains('服务器错误'));
    });

    test('should follow AppError(type): message format', () {
      final e = AppError(type: AppErrorType.server, message: 'err', code: 'ERR_500');
      expect(e.toString(), 'AppError(AppErrorType.server): err');
    });

    test('should include message in toString', () {
      final e = AppError(type: AppErrorType.server, message: '服务器错误500', statusCode: 500);
      expect(e.toString(), contains('服务器错误500'));
    });
  });

  group('AppError equality and properties', () {
    test('should store all fields', () {
      final original = Exception('原始');
      final e = AppError(
        type: AppErrorType.network,
        message: '网络错误',
        code: 'NET_001',
        statusCode: 0,
        originalError: original,
      );
      expect(e.type, AppErrorType.network);
      expect(e.message, '网络错误');
      expect(e.code, 'NET_001');
      expect(e.statusCode, 0);
      expect(e.originalError, original);
    });

    test('should allow null optional fields', () {
      final e = AppError(type: AppErrorType.unknown, message: 'test');
      expect(e.code, isNull);
      expect(e.statusCode, isNull);
      expect(e.originalError, isNull);
    });

    test('should implement Exception', () {
      final e = AppError(type: AppErrorType.unknown, message: 'test');
      expect(e, isA<Exception>());
    });
  });

  // ==================== ErrorHandler.handle ====================
  group('ErrorHandler.handle', () {
    test('should handle generic Exception', () {
      final error = ErrorHandler.handle(Exception('测试异常'));
      expect(error, isA<AppError>());
      expect(error.type, AppErrorType.unknown);
    });

    test('should handle String error', () {
      final error = ErrorHandler.handle('字符串错误');
      expect(error, isA<AppError>());
    });

    test('should handle null error', () {
      final error = ErrorHandler.handle(null);
      expect(error, isA<AppError>());
    });

    test('should handle AppError passthrough', () {
      final original = AppError(type: AppErrorType.auth, message: '认证失败');
      final error = ErrorHandler.handle(original);
      expect(error.type, AppErrorType.auth);
      expect(error.message, '认证失败');
    });

    test('should include context in error', () {
      final error = ErrorHandler.handle(Exception('test'), context: 'GET /api/users');
      expect(error, isA<AppError>());
    });

    test('should handle TypeError', () {
      final error = ErrorHandler.handle(TypeError());
      expect(error, isA<AppError>());
    });

    test('should handle FormatException', () {
      final error = ErrorHandler.handle(const FormatException('bad format'));
      expect(error, isA<AppError>());
    });

    test('should handle StateError', () {
      final error = ErrorHandler.handle(StateError('bad state'));
      expect(error, isA<AppError>());
    });

    test('should handle RangeError', () {
      final error = ErrorHandler.handle(RangeError('out of range'));
      expect(error, isA<AppError>());
    });

    test('should handle ArgumentError', () {
      final error = ErrorHandler.handle(ArgumentError('bad arg'));
      expect(error, isA<AppError>());
    });
  });

  // ==================== Result<T> 扩展测试 ====================
  group('Result success operations', () {
    test('should create success result', () {
      final r = Result.success(42);
      expect(r.isSuccess, true);
      expect(r.isFailure, false);
      expect(r.data, 42);
      expect(r.error, isNull);
    });

    test('should create success with null data', () {
      final r = Result.success(null);
      expect(r.isSuccess, true);
      expect(r.data, isNull);
    });

    test('should create success with string', () {
      final r = Result.success('hello');
      expect(r.dataOrThrow, 'hello');
    });

    test('should create success with list', () {
      final r = Result.success([1, 2, 3]);
      expect(r.dataOrThrow, [1, 2, 3]);
    });

    test('should create success with map', () {
      final r = Result.success({'key': 'value'});
      expect(r.dataOrThrow['key'], 'value');
    });

    test('should create success with complex type', () {
      final r = Result.success({'users': [{'id': 1}, {'id': 2}]});
      expect((r.dataOrThrow['users'] as List).length, 2);
    });
  });

  group('Result failure operations', () {
    test('should create failure result', () {
      final error = AppError(type: AppErrorType.server, message: '错误');
      final r = Result.failure(error);
      expect(r.isSuccess, false);
      expect(r.isFailure, true);
      expect(r.error, error);
    });

    test('dataOrThrow should throw on failure', () {
      final r = Result.failure(AppError(type: AppErrorType.server, message: '错误'));
      expect(() => r.dataOrThrow, throwsA(isA<AppError>()));
    });

    test('dataOr should return default on failure', () {
      final r = Result<int>.failure(AppError(type: AppErrorType.server, message: '错误'));
      expect(r.dataOr(99), 99);
    });
  });

  group('Result.map', () {
    test('should map success value', () {
      final r = Result.success(10);
      final mapped = r.map((v) => v * 2);
      expect(mapped.dataOrThrow, 20);
    });

    test('should map to different type', () {
      final r = Result.success(42);
      final mapped = r.map((v) => v.toString());
      expect(mapped.dataOrThrow, '42');
    });

    test('should not map failure', () {
      final error = AppError(type: AppErrorType.server, message: '错误');
      final r = Result<int>.failure(error);
      final mapped = r.map((v) => v * 2);
      expect(mapped.isFailure, true);
      expect(mapped.error, error);
    });

    test('should chain multiple maps', () {
      final r = Result.success(5);
      final mapped = r.map((v) => v * 2).map((v) => v + 1).map((v) => v.toString());
      expect(mapped.dataOrThrow, '11');
    });
  });

  group('Result.fold', () {
    test('should fold success', () {
      final r = Result.success(42);
      final value = r.fold(
        onSuccess: (d) => 'success: $d',
        onFailure: (e) => 'failure: ${e.message}',
      );
      expect(value, 'success: 42');
    });

    test('should fold failure', () {
      final r = Result<int>.failure(AppError(type: AppErrorType.server, message: '错误'));
      final value = r.fold(
        onSuccess: (d) => 'success: $d',
        onFailure: (e) => 'failure: ${e.message}',
      );
      expect(value, 'failure: 错误');
    });

    test('should fold to bool', () {
      final r = Result.success(100);
      final isLarge = r.fold(onSuccess: (d) => d > 50, onFailure: (_) => false);
      expect(isLarge, true);
    });

    test('should fold to list', () {
      final r = Result.success([1, 2, 3]);
      final len = r.fold(onSuccess: (d) => d.length, onFailure: (_) => 0);
      expect(len, 3);
    });
  });

  group('Result with various types', () {
    test('should work with bool', () {
      final r = Result.success(true);
      expect(r.dataOrThrow, true);
    });

    test('should work with double', () {
      final r = Result.success(3.14);
      expect(r.dataOrThrow, closeTo(3.14, 0.001));
    });

    test('should work with nested map', () {
      final r = Result.success({
        'level1': {
          'level2': {'value': 42}
        }
      });
      expect(r.dataOrThrow['level1']!['level2']!['value'], 42);
    });

    test('should work with empty collections', () {
      expect(Result.success(<int>[]).dataOrThrow, isEmpty);
      expect(Result.success(<String, dynamic>{}).dataOrThrow, isEmpty);
    });
  });
}
