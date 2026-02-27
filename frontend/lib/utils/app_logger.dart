// 统一日志工具

library;

import 'package:flutter/foundation.dart';
import 'app_config.dart';

/// 日志级别
enum LogLevel {
  debug,
  info,
  warning,
  error,
}

/// 日志分类
enum LogCategory {
  general,
  network,
  database,
  auth,
  ui,
  websocket,
  ai,
  system,
}

/// 日志工具类
class AppLogger {
  static final AppLogger _instance = AppLogger._internal();
  factory AppLogger() => _instance;
  AppLogger._internal();

  /// 日志图标映射
  static const Map<LogLevel, String> _levelIcons = {
    LogLevel.debug: '🔍',
    LogLevel.info: '📝',
    LogLevel.warning: '⚠️',
    LogLevel.error: '❌',
  };

  /// 分类图标映射
  static const Map<LogCategory, String> _categoryIcons = {
    LogCategory.general: '📋',
    LogCategory.network: '🌐',
    LogCategory.database: '💾',
    LogCategory.auth: '🔐',
    LogCategory.ui: '🎨',
    LogCategory.websocket: '🔌',
    LogCategory.ai: '🤖',
    LogCategory.system: '⚙️',
  };

  /// 是否启用日志
  bool _enabled = true;

  /// 最低日志级别
  LogLevel _minLevel = LogLevel.debug;

  /// 设置是否启用日志
  void setEnabled(bool enabled) {
    _enabled = enabled;
  }

  /// 设置最低日志级别
  void setMinLevel(LogLevel level) {
    _minLevel = level;
  }

  /// 通用日志方法
  void log(
    String message, {
    LogLevel level = LogLevel.info,
    LogCategory category = LogCategory.general,
    Object? error,
    StackTrace? stackTrace,
  }) {
    if (!_enabled) return;
    if (level.index < _minLevel.index) return;
    if (!appConfig.enableVerboseLogging && level == LogLevel.debug) return;

    final levelIcon = _levelIcons[level] ?? '📝';
    final categoryIcon = _categoryIcons[category] ?? '📋';
    final timestamp = DateTime.now().toString().substring(11, 23);

    final buffer = StringBuffer();
    buffer.write('[$timestamp] $levelIcon $categoryIcon ');
    buffer.write(message);

    if (error != null) {
      buffer.write('\n   Error: $error');
    }

    if (stackTrace != null && level == LogLevel.error) {
      buffer.write('\n   StackTrace: $stackTrace');
    }

    debugPrint(buffer.toString());
  }

  // ============================================================
  // 快捷方法
  // ============================================================

  /// 调试日志
  void debug(String message, {LogCategory category = LogCategory.general}) {
    log(message, level: LogLevel.debug, category: category);
  }

  /// 信息日志
  void info(String message, {LogCategory category = LogCategory.general}) {
    log(message, level: LogLevel.info, category: category);
  }

  /// 警告日志
  void warning(String message, {LogCategory category = LogCategory.general}) {
    log(message, level: LogLevel.warning, category: category);
  }

  /// 错误日志
  void error(
    String message, {
    LogCategory category = LogCategory.general,
    Object? error,
    StackTrace? stackTrace,
  }) {
    log(
      message,
      level: LogLevel.error,
      category: category,
      error: error,
      stackTrace: stackTrace,
    );
  }

  // ============================================================
  // 专用日志方法
  // ============================================================

  /// 网络请求日志
  void network(String method, String url, {int? statusCode, String? error}) {
    if (!appConfig.logNetworkRequests) return;

    final displayUrl = _humanizeUrl(url);
    final status = statusCode != null ? '[$statusCode]' : '';
    final errorMsg = error != null ? ' - $error' : '';
    log('$method $displayUrl $status$errorMsg', category: LogCategory.network);
  }

  String _humanizeUrl(String url) {
    return url
        .replaceAll('/vip/', '/light/')
        .replaceAll('/vip?', '/light?')
        .replaceAll('/vip', '/light');
  }

  /// WebSocket日志
  void websocket(String event, {String? data}) {
    if (!appConfig.logWebSocketMessages) return;

    final dataStr = data != null ? ': $data' : '';
    log('WS $event$dataStr', category: LogCategory.websocket);
  }

  /// 认证日志
  void auth(String action, {bool success = true}) {
    final status = success ? '✅' : '❌';
    log('$status $action', category: LogCategory.auth);
  }

  /// AI日志
  void ai(String action, {String? result}) {
    final resultStr = result != null ? ': $result' : '';
    log('AI $action$resultStr', category: LogCategory.ai);
  }

  /// UI日志
  void ui(String action) {
    log(action, category: LogCategory.ui, level: LogLevel.debug);
  }
}

/// 全局日志实例
final logger = AppLogger();
