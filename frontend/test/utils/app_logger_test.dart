import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/app_logger.dart';

void main() {
  late AppLogger testLogger;

  setUp(() {
    testLogger = AppLogger();
    testLogger.setEnabled(true);
    testLogger.setMinLevel(LogLevel.debug);
  });

  // ==================== LogLevel ====================
  group('LogLevel enum', () {
    test('should have 4 values', () {
      expect(LogLevel.values.length, 4);
    });

    test('should have correct order', () {
      expect(LogLevel.debug.index, 0);
      expect(LogLevel.info.index, 1);
      expect(LogLevel.warning.index, 2);
      expect(LogLevel.error.index, 3);
    });

    test('debug should be lowest', () {
      expect(LogLevel.debug.index, lessThan(LogLevel.info.index));
    });

    test('error should be highest', () {
      expect(LogLevel.error.index, greaterThan(LogLevel.warning.index));
    });

    test('should contain all expected values', () {
      expect(LogLevel.values, containsAll([
        LogLevel.debug,
        LogLevel.info,
        LogLevel.warning,
        LogLevel.error,
      ]));
    });
  });

  // ==================== LogCategory ====================
  group('LogCategory enum', () {
    test('should have 8 values', () {
      expect(LogCategory.values.length, 8);
    });

    test('should contain all expected categories', () {
      expect(LogCategory.values, containsAll([
        LogCategory.general,
        LogCategory.network,
        LogCategory.database,
        LogCategory.auth,
        LogCategory.ui,
        LogCategory.websocket,
        LogCategory.ai,
        LogCategory.system,
      ]));
    });
  });

  // ==================== AppLogger 配置 ====================
  group('AppLogger configuration', () {
    test('should be singleton', () {
      final logger1 = AppLogger();
      final logger2 = AppLogger();
      expect(identical(logger1, logger2), true);
    });

    test('setEnabled should not throw', () {
      expect(() => testLogger.setEnabled(true), returnsNormally);
      expect(() => testLogger.setEnabled(false), returnsNormally);
    });

    test('setMinLevel should not throw', () {
      for (final level in LogLevel.values) {
        expect(() => testLogger.setMinLevel(level), returnsNormally);
      }
    });

    test('should accept all log levels', () {
      expect(() => testLogger.setMinLevel(LogLevel.debug), returnsNormally);
      expect(() => testLogger.setMinLevel(LogLevel.info), returnsNormally);
      expect(() => testLogger.setMinLevel(LogLevel.warning), returnsNormally);
      expect(() => testLogger.setMinLevel(LogLevel.error), returnsNormally);
    });
  });

  // ==================== 日志方法 ====================
  group('AppLogger log methods', () {
    test('debug should not throw', () {
      expect(() => testLogger.debug('调试消息'), returnsNormally);
    });

    test('info should not throw', () {
      expect(() => testLogger.info('信息消息'), returnsNormally);
    });

    test('warning should not throw', () {
      expect(() => testLogger.warning('警告消息'), returnsNormally);
    });

    test('error should not throw', () {
      expect(() => testLogger.error('错误消息'), returnsNormally);
    });

    test('debug with category should not throw', () {
      for (final cat in LogCategory.values) {
        expect(() => testLogger.debug('消息', category: cat), returnsNormally);
      }
    });

    test('info with category should not throw', () {
      for (final cat in LogCategory.values) {
        expect(() => testLogger.info('消息', category: cat), returnsNormally);
      }
    });

    test('warning with category should not throw', () {
      for (final cat in LogCategory.values) {
        expect(() => testLogger.warning('消息', category: cat), returnsNormally);
      }
    });

    test('error with category should not throw', () {
      for (final cat in LogCategory.values) {
        expect(() => testLogger.error('消息', category: cat), returnsNormally);
      }
    });

    test('error with error object should not throw', () {
      expect(
        () => testLogger.error('错误', error: Exception('测试异常')),
        returnsNormally,
      );
    });

    test('error with stackTrace should not throw', () {
      expect(
        () => testLogger.error('错误', stackTrace: StackTrace.current),
        returnsNormally,
      );
    });

    test('error with both error and stackTrace should not throw', () {
      expect(
        () => testLogger.error(
          '错误',
          error: Exception('异常'),
          stackTrace: StackTrace.current,
        ),
        returnsNormally,
      );
    });

    test('log with all parameters should not throw', () {
      expect(
        () => testLogger.log(
          '完整日志',
          level: LogLevel.error,
          category: LogCategory.network,
          error: Exception('网络异常'),
          stackTrace: StackTrace.current,
        ),
        returnsNormally,
      );
    });
  });

  // ==================== 专用日志方法 ====================
  group('AppLogger specialized methods', () {
    test('network should not throw', () {
      expect(() => testLogger.network('GET', '/api/stones'), returnsNormally);
    });

    test('network with statusCode should not throw', () {
      expect(() => testLogger.network('POST', '/api/auth', statusCode: 200), returnsNormally);
    });

    test('network with error should not throw', () {
      expect(() => testLogger.network('GET', '/api/users', error: '超时'), returnsNormally);
    });

    test('network with all params should not throw', () {
      expect(
        () => testLogger.network('PUT', '/api/profile', statusCode: 500, error: '服务器错误'),
        returnsNormally,
      );
    });

    test('websocket should not throw', () {
      expect(() => testLogger.websocket('connect'), returnsNormally);
    });

    test('websocket with data should not throw', () {
      expect(() => testLogger.websocket('message', data: '{"type":"ping"}'), returnsNormally);
    });

    test('auth should not throw', () {
      expect(() => testLogger.auth('login'), returnsNormally);
    });

    test('auth with success should not throw', () {
      expect(() => testLogger.auth('login', success: true), returnsNormally);
      expect(() => testLogger.auth('login', success: false), returnsNormally);
    });

    test('ai should not throw', () {
      expect(() => testLogger.ai('analyze'), returnsNormally);
    });

    test('ai with result should not throw', () {
      expect(() => testLogger.ai('classify', result: 'happy: 0.8'), returnsNormally);
    });

    test('ui should not throw', () {
      expect(() => testLogger.ui('navigate to home'), returnsNormally);
    });
  });

  // ==================== 禁用日志 ====================
  group('AppLogger disabled', () {
    test('should not throw when disabled', () {
      testLogger.setEnabled(false);
      expect(() => testLogger.debug('消息'), returnsNormally);
      expect(() => testLogger.info('消息'), returnsNormally);
      expect(() => testLogger.warning('消息'), returnsNormally);
      expect(() => testLogger.error('消息'), returnsNormally);
      expect(() => testLogger.network('GET', '/api'), returnsNormally);
      expect(() => testLogger.websocket('event'), returnsNormally);
      expect(() => testLogger.auth('action'), returnsNormally);
      expect(() => testLogger.ai('action'), returnsNormally);
      expect(() => testLogger.ui('action'), returnsNormally);
    });
  });

  // ==================== 最低级别过滤 ====================
  group('AppLogger min level filtering', () {
    test('setting min level to error should not throw for any call', () {
      testLogger.setMinLevel(LogLevel.error);
      expect(() => testLogger.debug('debug msg'), returnsNormally);
      expect(() => testLogger.info('info msg'), returnsNormally);
      expect(() => testLogger.warning('warn msg'), returnsNormally);
      expect(() => testLogger.error('error msg'), returnsNormally);
    });

    test('setting min level to warning should not throw', () {
      testLogger.setMinLevel(LogLevel.warning);
      expect(() => testLogger.debug('debug'), returnsNormally);
      expect(() => testLogger.info('info'), returnsNormally);
      expect(() => testLogger.warning('warn'), returnsNormally);
    });

    test('setting min level to info should not throw', () {
      testLogger.setMinLevel(LogLevel.info);
      expect(() => testLogger.debug('debug'), returnsNormally);
      expect(() => testLogger.info('info'), returnsNormally);
    });

    test('setting min level to debug should not throw', () {
      testLogger.setMinLevel(LogLevel.debug);
      expect(() => testLogger.debug('debug'), returnsNormally);
    });
  });

  // ==================== 全局 logger 实例 ====================
  group('Global logger instance', () {
    test('should be accessible', () {
      expect(logger, isNotNull);
      expect(logger, isA<AppLogger>());
    });

    test('should be same as factory instance', () {
      expect(identical(logger, AppLogger()), true);
    });

    test('should support all methods', () {
      expect(() => logger.debug('test'), returnsNormally);
      expect(() => logger.info('test'), returnsNormally);
      expect(() => logger.warning('test'), returnsNormally);
      expect(() => logger.error('test'), returnsNormally);
    });
  });

  // ==================== 边界情况 ====================
  group('Edge cases', () {
    test('empty message should not throw', () {
      expect(() => testLogger.debug(''), returnsNormally);
      expect(() => testLogger.info(''), returnsNormally);
    });

    test('very long message should not throw', () {
      final longMsg = 'x' * 10000;
      expect(() => testLogger.info(longMsg), returnsNormally);
    });

    test('unicode message should not throw', () {
      expect(() => testLogger.info('你好世界 🌍 こんにちは'), returnsNormally);
    });

    test('message with newlines should not throw', () {
      expect(() => testLogger.info('line1\nline2\nline3'), returnsNormally);
    });

    test('message with special chars should not throw', () {
      expect(() => testLogger.info('tab\there\r\nend'), returnsNormally);
    });

    test('null error object should not throw', () {
      expect(() => testLogger.error('err', error: null), returnsNormally);
    });

    test('null stackTrace should not throw', () {
      expect(() => testLogger.error('err', stackTrace: null), returnsNormally);
    });
  });
}
