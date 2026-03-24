/// 应用配置管理
///
/// 单例模式，管理应用的全局配置，包括环境配置、API配置、超时配置、
/// 缓存配置、功能开关等。

library;

import 'dart:io';
import 'package:flutter/foundation.dart';

/// 应用环境枚举
enum AppEnvironment {
  development,
  staging,
  production,
}

/// 应用配置类
///
/// 单例模式，提供全局配置访问。
class AppConfig {
  static final AppConfig _instance = AppConfig._internal();
  factory AppConfig() => _instance;
  AppConfig._internal();

  // ============================================================
  // 环境配置
  // ============================================================

  /// 当前环境
  AppEnvironment _environment = AppEnvironment.development;
  AppEnvironment get environment => _environment;

  /// 是否为开发模式
  bool get isDevelopment => _environment == AppEnvironment.development;

  /// 是否为生产模式
  bool get isProduction => _environment == AppEnvironment.production;

  // ============================================================
  // API配置
  // ============================================================

  /// API基础URL（从环境变量或默认值获取）
  static const String _apiBaseUrlOverride = String.fromEnvironment(
    'API_BASE_URL',
    defaultValue: '',
  );

  /// 公网网关 Origin（例如 https://example.com），用于统一推导 API/WS 地址
  static const String _publicOriginOverride = String.fromEnvironment(
    'PUBLIC_ORIGIN',
    defaultValue: '',
  );

  /// WebSocket 完整地址覆盖（例如 wss://example.com/ws/broadcast）
  static const String _wsUrlOverride = String.fromEnvironment(
    'WS_URL',
    defaultValue: '',
  );

  /// 获取API基础URL
  ///
  /// 根据当前环境返回对应的API地址。
  String get apiBaseUrl {
    final explicitApiBaseUrl = _normalizeApiBaseUrl(_apiBaseUrlOverride);
    if (explicitApiBaseUrl.isNotEmpty) {
      return explicitApiBaseUrl;
    }

    final publicOriginApiBaseUrl = _originToApiBaseUrl(_publicOriginOverride);
    if (publicOriginApiBaseUrl.isNotEmpty) {
      return publicOriginApiBaseUrl;
    }

    final webApiBaseUrl = _getCurrentWebApiUrl();
    if (webApiBaseUrl.isNotEmpty) {
      return webApiBaseUrl;
    }

    switch (_environment) {
      case AppEnvironment.development:
        return _getDevelopmentApiUrl();
      case AppEnvironment.staging:
        return 'https://staging-api.heartlake.app/api';
      case AppEnvironment.production:
        return 'https://api.heartlake.app/api';
    }
  }

  /// 获取开发环境API URL
  ///
  /// 根据平台自动切换合适的地址。
  String _getDevelopmentApiUrl() {
    // 优先使用环境变量
    const envUrl = String.fromEnvironment('DEV_API_URL', defaultValue: '');
    final normalizedEnvUrl = _normalizeApiBaseUrl(envUrl);
    if (normalizedEnvUrl.isNotEmpty) {
      return normalizedEnvUrl;
    }

    // 根据平台智能选择
    try {
      if (Platform.isAndroid) {
        // 模拟器用 10.0.2.2，真机用局域网IP（可通过ANDROID_API_HOST环境变量配置）
        const androidHost = String.fromEnvironment('ANDROID_API_HOST',
            defaultValue: '10.0.2.2');
        return _originToApiBaseUrl('http://$androidHost:8080');
      }
      if (Platform.isIOS) {
        // iOS模拟器可直接访问 localhost，真机需要局域网IP
        const iosHost =
            String.fromEnvironment('IOS_API_HOST', defaultValue: 'localhost');
        return _originToApiBaseUrl('http://$iosHost:8080');
      }
      // Windows / macOS / Linux 桌面端均使用 localhost
      return _originToApiBaseUrl('http://localhost:8080');
    } catch (e) {
      return _originToApiBaseUrl('http://localhost:8080');
    }
  }

  /// WebSocket URL
  String get wsUrl {
    final explicitWsUrl = _normalizeWsUrl(_wsUrlOverride);
    if (explicitWsUrl.isNotEmpty) {
      return explicitWsUrl;
    }

    final publicOriginWsUrl = _originToWsUrl(_publicOriginOverride);
    if (publicOriginWsUrl.isNotEmpty) {
      return publicOriginWsUrl;
    }

    return _apiBaseUrlToWsUrl(apiBaseUrl);
  }

  String _getCurrentWebApiUrl() {
    if (!kIsWeb) return '';
    final base = Uri.base;
    final webOrigin = _getCurrentWebOrigin();
    if (webOrigin.isEmpty) return '';

    final rawHost = base.host.isNotEmpty ? base.host : 'localhost';
    final host = rawHost == '0.0.0.0' ? 'localhost' : rawHost;
    final isPrivateIp = RegExp(
      r'^(127\.0\.0\.1|10\.\d{1,3}\.\d{1,3}\.\d{1,3}|192\.168\.\d{1,3}\.\d{1,3}|172\.(1[6-9]|2\d|3[0-1])\.\d{1,3}\.\d{1,3})$',
    ).hasMatch(host);
    final isLocalHost = host == 'localhost' || isPrivateIp;
    final port = base.hasPort ? base.port : (base.scheme == 'https' ? 443 : 80);

    // Flutter Web 独立调试服务通常运行在随机端口；这时仍保持直连后端 8080 的旧行为。
    if (isLocalHost &&
        port != 80 &&
        port != 443 &&
        port != 3000 &&
        port != 8080) {
      return _originToApiBaseUrl('${base.scheme}://$host:8080');
    }

    return _originToApiBaseUrl(webOrigin);
  }

  String _getCurrentWebOrigin() {
    if (!kIsWeb) return '';
    final base = Uri.base;
    final scheme = base.scheme;
    if (scheme != 'http' && scheme != 'https') {
      return '';
    }
    final origin = base.origin;
    if (origin.isEmpty) {
      return '';
    }
    return origin.replaceAll(RegExp(r'/+$'), '');
  }

  String _normalizeApiBaseUrl(String rawUrl) {
    final normalized = rawUrl.trim();
    if (normalized.isEmpty) {
      return '';
    }
    if (normalized.endsWith('/api')) {
      return normalized;
    }
    if (normalized.endsWith('/api/')) {
      return normalized.substring(0, normalized.length - 1);
    }
    return '${normalized.replaceAll(RegExp(r'/+$'), '')}/api';
  }

  String _normalizeWsUrl(String rawUrl) {
    final normalized = rawUrl.trim();
    if (normalized.isEmpty) {
      return '';
    }
    if (normalized.endsWith('/ws/broadcast')) {
      return normalized;
    }
    if (normalized.endsWith('/ws/broadcast/')) {
      return normalized.substring(0, normalized.length - 1);
    }
    return '${normalized.replaceAll(RegExp(r'/+$'), '')}/ws/broadcast';
  }

  String _originToApiBaseUrl(String rawOrigin) {
    final origin = rawOrigin.trim();
    if (origin.isEmpty) {
      return '';
    }
    final normalizedOrigin = origin
        .replaceAll(RegExp(r'/api/?$'), '')
        .replaceAll(RegExp(r'/+$'), '');
    return '$normalizedOrigin/api';
  }

  String _originToWsUrl(String rawOrigin) {
    final origin = rawOrigin.trim();
    if (origin.isEmpty) {
      return '';
    }
    final normalizedOrigin = origin
        .replaceAll(RegExp(r'/api/?$'), '')
        .replaceAll(RegExp(r'/ws/broadcast/?$'), '')
        .replaceAll(RegExp(r'/+$'), '');
    final wsOrigin = normalizedOrigin
        .replaceFirst('http://', 'ws://')
        .replaceFirst('https://', 'wss://');
    return '$wsOrigin/ws/broadcast';
  }

  String _apiBaseUrlToWsUrl(String rawApiBaseUrl) {
    final normalizedApiBaseUrl = _normalizeApiBaseUrl(rawApiBaseUrl);
    final origin = normalizedApiBaseUrl.replaceAll(RegExp(r'/api/?$'), '');
    final wsOrigin = origin
        .replaceFirst('http://', 'ws://')
        .replaceFirst('https://', 'wss://')
        .replaceAll(RegExp(r'/+$'), '');
    return '$wsOrigin/ws/broadcast';
  }

  // ============================================================
  // 超时配置
  // ============================================================

  /// 连接超时时间。
  ///
  /// 生产环境优先保证低配云机上的失败快速返回，开发环境保留更宽松窗口。
  Duration get connectTimeout =>
      isProduction ? const Duration(seconds: 15) : const Duration(seconds: 30);

  /// 接收超时时间。
  ///
  /// 保留足够的业务处理窗口，同时避免移动端在弱网下长时间挂起。
  Duration get receiveTimeout =>
      isProduction ? const Duration(seconds: 25) : const Duration(seconds: 30);

  /// 发送超时时间。
  Duration get sendTimeout =>
      isProduction ? const Duration(seconds: 20) : const Duration(seconds: 30);

  // ============================================================
  // 连接池配置
  // ============================================================

  /// HTTP最大连接数。
  ///
  /// 低配生产服务器上适度压低并发连接，减少瞬时拥塞。
  int get maxConnections => isProduction ? 4 : 6;

  /// 连接空闲超时
  Duration get idleTimeout =>
      isProduction ? const Duration(seconds: 20) : const Duration(seconds: 30);

  /// 最大重试次数
  int get maxRetries => isProduction ? 1 : 2;

  // ============================================================
  // 缓存配置
  // ============================================================

  /// 石头列表缓存时间
  Duration get stoneCacheDuration => const Duration(minutes: 5);

  /// 用户信息缓存时间
  Duration get userCacheDuration => const Duration(hours: 1);

  /// 最大缓存条目数
  int get maxCacheEntries => 100;

  // ============================================================
  // 分页配置
  // ============================================================

  /// 默认每页数量
  int get defaultPageSize => 20;

  /// 最大每页数量
  int get maxPageSize => 50;

  // ============================================================
  // 内容限制
  // ============================================================

  /// 石头内容最大长度
  int get maxStoneContentLength => 500;

  /// 纸船内容最大长度
  int get maxBoatContentLength => 300;

  /// 聊天消息最大长度
  int get maxChatMessageLength => 1000;

  /// 用户昵称最大长度
  int get maxNicknameLength => 20;

  /// 用户简介最大长度
  int get maxBioLength => 200;

  // ============================================================
  // 功能开关
  // ============================================================

  /// 是否启用AI功能
  bool get enableAI => true;

  /// 是否启用实时推送
  bool get enablePush => true;

  /// 是否启用匿名模式
  bool get enableAnonymous => true;

  /// 是否启用好友系统
  bool get enableFriends => true;

  /// 是否启用涟漪动画
  bool get enableRippleAnimation => true;

  /// 是否启用情绪分析
  bool get enableMoodAnalysis => true;

  // ============================================================
  // 动画配置
  // ============================================================

  /// 涟漪动画持续时间
  Duration get rippleAnimationDuration => const Duration(milliseconds: 2000);

  /// 页面切换动画时间
  Duration get pageTransitionDuration => const Duration(milliseconds: 300);

  /// 卡片动画时间
  Duration get cardAnimationDuration => const Duration(milliseconds: 200);

  // ============================================================
  // 日志配置
  // ============================================================

  /// 是否启用详细日志
  bool get enableVerboseLogging => isDevelopment;

  /// 是否打印网络请求
  bool get logNetworkRequests => isDevelopment;

  /// 是否打印WebSocket消息
  bool get logWebSocketMessages => isDevelopment;

  // ============================================================
  // 方法
  // ============================================================

  /// 初始化配置
  ///
  /// 根据编译模式和运行环境自动判断当前环境。
  ///
  /// [environment] 可选的环境配置，不提供则自动判断
  void initialize({
    AppEnvironment? environment,
  }) {
    if (environment != null) {
      _environment = environment;
    } else if (kIsWeb) {
      // Web场景下优先按访问域名判断环境
      final host = Uri.base.host;
      final isPrivateIp = RegExp(
        r'^(127\.0\.0\.1|0\.0\.0\.0|10\.\d{1,3}\.\d{1,3}\.\d{1,3}|192\.168\.\d{1,3}\.\d{1,3}|172\.(1[6-9]|2\d|3[0-1])\.\d{1,3}\.\d{1,3})$',
      ).hasMatch(host);
      final isLocalHost = host == 'localhost' || isPrivateIp;
      _environment = isLocalHost
          ? AppEnvironment.development
          : (kReleaseMode
              ? AppEnvironment.production
              : (kProfileMode
                  ? AppEnvironment.staging
                  : AppEnvironment.development));
    } else {
      // 非Web端维持原有编译模式策略
      if (kReleaseMode) {
        _environment = AppEnvironment.production;
      } else if (kProfileMode) {
        _environment = AppEnvironment.staging;
      } else {
        _environment = AppEnvironment.development;
      }
    }

    if (kDebugMode) {
      debugPrint('AppConfig initialized');
      debugPrint('Environment: ${_environment.name}');
      debugPrint('API URL: $apiBaseUrl');
      debugPrint('WS URL: $wsUrl');
    }
  }

  /// 打印所有配置
  ///
  /// 调试用，输出当前配置信息。
  void printConfig() {
    if (!kDebugMode) {
      return;
    }
    debugPrint('''
╔══════════════════════════════════════════════════════════════╗
║                    HeartLake 配置信息                         ║
╠══════════════════════════════════════════════════════════════╣
║ 环境: ${_environment.name.padRight(52)}║
║ API: ${apiBaseUrl.padRight(53)}║
║ WS:  ${wsUrl.padRight(53)}║
╠══════════════════════════════════════════════════════════════╣
║ 功能开关:                                                    ║
║   - AI功能: ${enableAI ? '✅' : '❌'}                                                   ║
║   - 实时推送: ${enablePush ? '✅' : '❌'}                                                 ║
║   - 匿名模式: ${enableAnonymous ? '✅' : '❌'}                                                 ║
║   - 好友系统: ${enableFriends ? '✅' : '❌'}                                                 ║
║   - 涟漪动画: ${enableRippleAnimation ? '✅' : '❌'}                                                 ║
║   - 情绪分析: ${enableMoodAnalysis ? '✅' : '❌'}                                                 ║
╚══════════════════════════════════════════════════════════════╝
''');
  }
}

/// 全局配置实例
final appConfig = AppConfig();
