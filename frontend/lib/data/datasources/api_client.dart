/// HTTP 客户端封装
///
/// 基于 Dio 实现的网络请求客户端，提供统一的 HTTP 通信能力。
/// 主要功能包括：
/// - Token 自动注入和刷新：请求时自动添加认证令牌，401 错误时自动刷新
/// - 统一错误处理：捕获网络异常、超时等错误并转换为友好提示
/// - SSL 证书校验：可选的证书指纹验证，防止中间人攻击
library;

import 'dart:io';
import 'package:crypto/crypto.dart';
import 'package:dio/dio.dart';
import 'package:dio/io.dart';
import 'package:flutter/foundation.dart';
import '../../utils/storage_util.dart';
import '../../utils/app_config.dart';
import '../../utils/app_logger.dart';
import '../../utils/error_handler.dart' show ErrorHandler, Result;
import 'cache_service.dart';

/// Token 刷新回调函数类型
typedef TokenRefreshCallback = Future<String?> Function(String? refreshToken);

/// HTTP 客户端单例
///
/// 管理所有 HTTP 请求，自动处理认证、错误和证书校验。
/// 使用单例模式确保全局共享同一个 Dio 实例和 Token 状态。
class ApiClient {
  static final ApiClient _instance = ApiClient._internal();
  factory ApiClient() => _instance;

  late Dio _dio;
  String? _token;
  String? _refreshToken;
  String? _userId;

  /// 防止 Token 刷新时的并发请求
  bool _isRefreshing = false;
  bool _hasTriggeredUnauthorized = false;

  TokenRefreshCallback? _tokenRefreshCallback;
  Function()? _onUnauthorized;
  final CacheService cacheService = CacheService();

  /// SSL 证书指纹列表，用于证书固定验证
  static const List<String> _pinnedCertFingerprints = [
    // 主证书 SHA-256 指纹（heartlake.app）
    'a]4b9c2d1e0f3a5b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b',
    // 备用证书指纹（证书轮换时使用）
    'b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1',
  ];

  /// 是否启用证书固定验证
  static const bool _enableCertPinning = bool.fromEnvironment(
    'ENABLE_CERT_PINNING',
    defaultValue: true,
  );

  ApiClient._internal() {
    _initializeDio();
    _loadToken();
  }

  /// 初始化 Dio 实例并配置基础参数
  void _initializeDio() {
    _dio = Dio(BaseOptions(
      baseUrl: appConfig.apiBaseUrl,
      connectTimeout: appConfig.connectTimeout,
      receiveTimeout: appConfig.receiveTimeout,
      // 抹除 Web 环境下的无意义的响应上传心跳报错噪声
      sendTimeout: kIsWeb ? null : appConfig.sendTimeout,
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      persistentConnection: true,
      responseType: ResponseType.json,
    ));

    // 配置 HTTP 客户端的连接参数
    if (_dio.httpClientAdapter is IOHttpClientAdapter) {
      (_dio.httpClientAdapter as IOHttpClientAdapter).createHttpClient = () {
        final client = HttpClient();
        client.maxConnectionsPerHost = appConfig.maxConnections;
        client.idleTimeout = appConfig.idleTimeout;
        client.connectionTimeout = appConfig.connectTimeout;

        // 调试模式下允许自签名证书，方便本地测试
        if (kDebugMode || !_enableCertPinning) {
          client.badCertificateCallback =
              (X509Certificate cert, String host, int port) => true;
        } else {
          client.badCertificateCallback =
              (X509Certificate cert, String host, int port) => false;
        }

        return client;
      };

      // 生产环境下启用证书指纹验证
      if (!kDebugMode && _enableCertPinning) {
        (_dio.httpClientAdapter as IOHttpClientAdapter).validateCertificate =
            (X509Certificate? cert, String host, int port) {
          if (cert == null) return false;
          final fingerprint = sha256.convert(cert.der).toString();
          return _pinnedCertFingerprints.contains(fingerprint);
        };
      }
    }

    _dio.interceptors.add(InterceptorsWrapper(
      onRequest: _onRequest,
      onResponse: _onResponse,
      onError: _onError,
    ));

    logger.info('API客户端初始化完成: ${appConfig.apiBaseUrl}',
        category: LogCategory.network);
  }

  /// 请求拦截器：自动添加认证 Token 和请求 ID
  Future<void> _onRequest(
    RequestOptions options,
    RequestInterceptorHandler handler,
  ) async {
    _token ??= await StorageUtil.getToken();
    _userId ??= await StorageUtil.getUserId();

    if (_token != null) {
      options.headers['Authorization'] = 'Bearer $_token';
    }
    if (_userId != null) {
      options.headers['X-User-Id'] = _userId;
    }

    options.headers['X-Request-Id'] = _generateRequestId();

    logger.network(options.method, options.uri.toString());

    return handler.next(options);
  }

  /// 响应拦截器：记录成功响应的日志
  void _onResponse(
    Response response,
    ResponseInterceptorHandler handler,
  ) {
    logger.network(
      response.requestOptions.method,
      response.requestOptions.uri.toString(),
      statusCode: response.statusCode,
    );
    return handler.next(response);
  }

  /// 错误拦截器：处理网络错误和 401 认证失败
  Future<void> _onError(
    DioException error,
    ErrorInterceptorHandler handler,
  ) async {
    final statusCode = error.response?.statusCode;
    final requestPath = error.requestOptions.uri.toString();

    logger.network(
      error.requestOptions.method,
      requestPath,
      statusCode: statusCode,
      error: error.message,
    );

    if (error.type == DioExceptionType.connectionTimeout ||
        error.type == DioExceptionType.sendTimeout ||
        error.type == DioExceptionType.receiveTimeout) {
      logger.warning('请求超时: $requestPath', category: LogCategory.network);
      return handler.next(error);
    }

    if (error.type == DioExceptionType.connectionError) {
      logger.warning('连接失败: $requestPath', category: LogCategory.network);
      return handler.next(error);
    }

    // 401 错误时尝试刷新 Token 并重试请求
    if (statusCode == 401) {
      final refreshed = await _handleUnauthorized();
      if (refreshed) {
        try {
          final opts = error.requestOptions;
          opts.headers['Authorization'] = 'Bearer $_token';
          final response = await _dio.fetch(opts);
          return handler.resolve(response);
        } catch (e) {
          // Token刷新后重试仍然失败则顺延
          return handler.next(error);
        }
      } else {
        return handler.next(error);
      }
    }

    return handler.next(error);
  }
}
