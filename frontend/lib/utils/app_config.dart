// 应用配置管理

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
/// 单例模式，全局配置管理
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

  /// 获取API基础URL
  String get apiBaseUrl {
    if (_apiBaseUrlOverride.isNotEmpty) {
      return _apiBaseUrlOverride;
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

  /// 获取开发环境API URL（根据平台自动切换）
  String _getDevelopmentApiUrl() {
    if (kIsWeb) {
      final scheme = Uri.base.scheme.isNotEmpty ? Uri.base.scheme : 'http';
      final rawHost = Uri.base.host.isNotEmpty ? Uri.base.host : 'localhost';
      final host = rawHost == '0.0.0.0' ? 'localhost' : rawHost;
      return '$scheme://$host:8080/api';
    }

    // 优先使用环境变量
    const envUrl = String.fromEnvironment('DEV_API_URL', defaultValue: '');
    if (envUrl.isNotEmpty) {
      return envUrl;
    }

    // 根据平台智能选择
    try {
      if (Platform.isAndroid) {
        // 模拟器用 10.0.2.2，真机用局域网IP（可通过ANDROID_API_HOST环境变量配置）
        const androidHost = String.fromEnvironment('ANDROID_API_HOST', defaultValue: '10.0.2.2');
        return 'http://$androidHost:8080/api';
      }
      if (Platform.isIOS) {
        // iOS模拟器可直接访问 localhost，真机需要局域网IP
        const iosHost = String.fromEnvironment('IOS_API_HOST', defaultValue: 'localhost');
        return 'http://$iosHost:8080/api';
      }
      // Windows / macOS / Linux 桌面端均使用 localhost
      return 'http://localhost:8080/api';
    } catch (e) {
      return 'http://localhost:8080/api';
    }
  }

  /// WebSocket 地址
  String get wsUrl {
    final baseUrl = apiBaseUrl;
    return '${baseUrl.replaceFirst('http://', 'ws://').replaceFirst('https://', 'wss://').replaceFirst('/api', '').replaceFirst(RegExp(r'/+$'), '')}/ws/broadcast';
  }

  // ============================================================
  // 超时配置
  // ============================================================

  /// 连接超时时间（优化为10秒，C++后端响应快）
  Duration get connectTimeout => const Duration(seconds: 30);

  /// 接收超时时间（优化为30秒）
  Duration get receiveTimeout => const Duration(seconds: 30);

  /// 发送超时时间（优化为30秒）
  Duration get sendTimeout => const Duration(seconds: 30);

  // ============================================================
  // 连接池配置
  // ============================================================

  /// HTTP最大连接数
  int get maxConnections => 5;

  /// 连接空闲超时
  Duration get idleTimeout => const Duration(seconds: 30);

  /// 最大重试次数
  int get maxRetries => 2;

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
  void initialize({
    AppEnvironment? environment,
  }) {
    if (environment != null) {
      _environment = environment;
    } else if (kIsWeb) {
      // Web 场景下优先按访问域名判断环境，避免本地 --release 被误判为生产环境
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
      // 非 Web 端维持原有编译模式策略
      if (kReleaseMode) {
        _environment = AppEnvironment.production;
      } else if (kProfileMode) {
        _environment = AppEnvironment.staging;
      } else {
        _environment = AppEnvironment.development;
      }
    }

    debugPrint('🔧 AppConfig initialized');
    debugPrint('   Environment: ${_environment.name}');
    debugPrint('   API URL: $apiBaseUrl');
    debugPrint('   WS URL: $wsUrl');
  }

  /// 打印所有配置（调试用）
  void printConfig() {
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
