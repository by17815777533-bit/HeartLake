/// HTTP 客户端封装
///
/// 基于 Dio 实现的全局网络请求客户端，提供统一的 HTTP 通信能力。
/// 依赖 [StorageUtil] 持久化 Token，依赖 [CacheService] 做 GET 请求缓存。
///
/// 主要功能：
/// - Token 自动注入和刷新：请求拦截器自动添加认证令牌，401 时尝试 refreshToken 换新
/// - 统一错误处理：捕获网络异常、超时等错误并记录日志
/// - SSL 证书固定：生产环境下校验证书 SHA-256 指纹，防止中间人攻击
library;

import 'dart:async';
import 'dart:io';
import 'package:crypto/crypto.dart';
import 'package:dio/dio.dart';
import 'package:dio/io.dart';
import 'package:flutter/foundation.dart';
import '../../utils/storage_util.dart';
import '../../utils/app_config.dart';
import '../../utils/app_logger.dart';
import 'cache_service.dart';

/// Token 刷新回调函数类型
typedef TokenRefreshCallback = Future<String?> Function(String? refreshToken);

/// HTTP 客户端单例
///
/// 通过工厂构造函数保证全局唯一实例，所有业务 Service 共享同一个 Dio
/// 和 Token 状态。内部通过拦截器链完成认证注入、日志记录和错误处理。
class ApiClient {
  static final ApiClient _instance = ApiClient._internal();
  factory ApiClient() => _instance;

  late Dio _dio;
  String? _token;
  String? _refreshToken;
  String? _userId;

  Completer<bool>? _refreshCompleter;

  /// 标记是否已触发过未授权回调，避免重复跳转登录页
  bool _hasTriggeredUnauthorized = false;

  /// Token 刷新回调，由外部（如 AuthService）注入
  TokenRefreshCallback? _tokenRefreshCallback;

  /// 认证失效回调，通常用于跳转登录页
  Function()? _onUnauthorized;

  /// 客户端级 GET 缓存，减少重复请求
  final CacheService cacheService = CacheService();

  /// 证书固定开关（默认关闭，只有显式提供指纹后才建议开启）
  static const bool _enableCertPinning = bool.fromEnvironment(
    'ENABLE_CERT_PINNING',
    defaultValue: false,
  );

  /// 逗号分隔的 SHA-256 指纹列表（小写十六进制）
  static const String _pinnedCertFingerprintsRaw = String.fromEnvironment(
    'CERT_SHA256_PINS',
    defaultValue: '',
  );

  static final Set<String> _pinnedCertFingerprints = _parsePinnedFingerprints();

  static Set<String> _parsePinnedFingerprints() {
    return _pinnedCertFingerprintsRaw
        .split(',')
        .map((item) => item.trim().toLowerCase())
        .where((item) => item.isNotEmpty)
        .toSet();
  }

  bool get _shouldUseCertificatePinning =>
      !kIsWeb && _enableCertPinning && _pinnedCertFingerprints.isNotEmpty;

  ApiClient._internal() {
    _initializeDio();
    _loadToken();
  }

  /// 初始化 Dio 实例，配置超时、Header、连接池、SSL 和拦截器
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

        // 仅在调试模式下允许自签名证书，生产环境保留系统默认证书校验。
        if (kDebugMode) {
          client.badCertificateCallback =
              (X509Certificate cert, String host, int port) => true;
        }

        return client;
      };

      // 仅在显式提供证书指纹时启用 Pinning，避免线上证书轮换导致客户端彻底失联。
      if (_shouldUseCertificatePinning) {
        (_dio.httpClientAdapter as IOHttpClientAdapter).validateCertificate =
            (X509Certificate? cert, String host, int port) {
          if (cert == null) return false;
          final fingerprint = sha256.convert(cert.der).toString().toLowerCase();
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
    if (statusCode == 401 && !_isRefreshRequest(error.requestOptions)) {
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

  bool _isRefreshRequest(RequestOptions options) {
    final normalizedPath = options.path.toLowerCase();
    return normalizedPath.endsWith('/auth/refresh') ||
        normalizedPath == '/auth/refresh';
  }

  /// 从本地存储加载已持久化的认证令牌
  Future<void> _loadToken() async {
    _token = await StorageUtil.getToken();
    _refreshToken = await StorageUtil.getRefreshToken();
    _userId = await StorageUtil.getUserId();
  }

  /// 生成唯一请求标识，用于链路追踪
  String _generateRequestId() {
    return '${DateTime.now().millisecondsSinceEpoch}-${_token?.hashCode ?? 0}';
  }

  /// 处理 401 未授权响应，尝试通过 refreshToken 换取新令牌
  ///
  /// 使用 _isRefreshing 标志防止并发刷新。
  /// 刷新成功返回 true，调用方可用新 Token 重试原请求；
  /// 刷新失败触发 onUnauthorized 回调（通常跳转登录页）。
  Future<bool> _handleUnauthorized() async {
    final existingCompleter = _refreshCompleter;
    if (existingCompleter != null) {
      return existingCompleter.future;
    }

    final completer = Completer<bool>();
    _refreshCompleter = completer;
    try {
      if (_tokenRefreshCallback != null) {
        final newToken = await _tokenRefreshCallback!(_refreshToken);
        if (newToken != null && newToken.isNotEmpty) {
          _token = newToken;
          _hasTriggeredUnauthorized = false;
          completer.complete(true);
          return true;
        }
      }
      if (!_hasTriggeredUnauthorized) {
        _hasTriggeredUnauthorized = true;
        _onUnauthorized?.call();
      }
      completer.complete(false);
      return false;
    } catch (error, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'api_client',
          context: ErrorDescription('while refreshing unauthorized session'),
        ),
      );
      if (!_hasTriggeredUnauthorized) {
        _hasTriggeredUnauthorized = true;
        _onUnauthorized?.call();
      }
      if (!completer.isCompleted) {
        completer.complete(false);
      }
      return false;
    } finally {
      _refreshCompleter = null;
    }
  }

  /// 设置 Token 刷新回调和未授权回调
  ///
  /// 通常在 App 初始化时调用一次，注入 AuthService 的刷新逻辑
  /// 和路由层的登录跳转逻辑。
  void configureAuth({
    TokenRefreshCallback? tokenRefreshCallback,
    Function()? onUnauthorized,
  }) {
    _tokenRefreshCallback = tokenRefreshCallback;
    _onUnauthorized = onUnauthorized;
  }

  /// 兼容旧调用：仅设置未授权回调
  void setOnUnauthorized(Function() onUnauthorized) {
    _onUnauthorized = onUnauthorized;
  }

  /// 更新认证令牌并持久化到本地存储
  ///
  /// 登录成功后由 AuthService 调用，同时重置未授权标记。
  Future<void> setToken(String token,
      {String? refreshToken, String? userId, String? sessionId}) async {
    _token = token;
    _hasTriggeredUnauthorized = false;
    await StorageUtil.saveToken(token);
    if (refreshToken != null) {
      _refreshToken = refreshToken;
      await StorageUtil.saveRefreshToken(refreshToken);
    }
    if (userId != null) {
      _userId = userId;
      await StorageUtil.saveUserId(userId);
    }
    if (sessionId != null) {
      await StorageUtil.saveSessionId(sessionId);
    }
  }

  /// 兼容旧调用：单独设置 userId
  Future<void> setUserId(String userId) async {
    _userId = userId;
    await StorageUtil.saveUserId(userId);
  }

  /// 清除认证状态并清空本地存储（退出登录时调用）
  Future<void> clearToken() async {
    _token = null;
    _refreshToken = null;
    _userId = null;
    _hasTriggeredUnauthorized = false;
    await StorageUtil.clearAll();
  }

  /// 发起 GET 请求，支持客户端内存缓存
  ///
  /// [useCache] 为 true 时，优先返回缓存数据；缓存未命中再发起网络请求。
  /// [cacheDuration] 可自定义缓存 TTL，默认使用 [CacheService] 的全局配置。
  Future<Response> get(
    String path, {
    Map<String, dynamic>? queryParameters,
    bool useCache = true,
    Duration? cacheDuration,
  }) async {
    if (useCache) {
      final cacheKey = 'GET:$path:${queryParameters?.toString() ?? ''}';
      final cached = cacheService.get(cacheKey);
      if (cached != null) {
        return Response(
          requestOptions: RequestOptions(path: path),
          data: cached,
          statusCode: 200,
        );
      }
      final response = await _dio.get(path, queryParameters: queryParameters);
      cacheService.set(
        cacheKey,
        response.data,
        ttl: cacheDuration,
      );
      return response;
    }
    return _dio.get(path, queryParameters: queryParameters);
  }

  /// 发起 POST 请求
  Future<Response> post(String path, {dynamic data}) {
    return _dio.post(path, data: data);
  }

  /// 发起 PUT 请求
  Future<Response> put(String path, {dynamic data}) {
    return _dio.put(path, data: data);
  }

  /// 发起 DELETE 请求
  Future<Response> delete(String path, {dynamic data}) {
    return _dio.delete(path, data: data);
  }

  /// 上传文件（multipart/form-data），支持进度回调
  Future<Response> upload(
    String path, {
    required FormData data,
    void Function(int, int)? onSendProgress,
  }) {
    return _dio.post(path, data: data, onSendProgress: onSendProgress);
  }

  /// 兼容旧调用：动态文件上传
  Future<Response> uploadFile(
    String path, {
    required dynamic file,
    String? filename,
    void Function(int, int)? onSendProgress,
  }) async {
    MultipartFile multipart;
    if (file is MultipartFile) {
      multipart = file;
    } else if (file is File) {
      multipart = await MultipartFile.fromFile(
        file.path,
        filename: filename ?? file.path.split('/').last,
      );
    } else if (file is List<int>) {
      multipart = MultipartFile.fromBytes(
        file,
        filename: filename ?? 'upload.bin',
      );
    } else if (file is Uint8List) {
      multipart = MultipartFile.fromBytes(
        file,
        filename: filename ?? 'upload.bin',
      );
    } else {
      throw ArgumentError('Unsupported upload file type: ${file.runtimeType}');
    }

    return upload(
      path,
      data: FormData.fromMap({'file': multipart}),
      onSendProgress: onSendProgress,
    );
  }

  /// 获取底层 Dio 实例（仅用于需要自定义配置的特殊场景）
  Dio get dio => _dio;

  /// 当前访问令牌（内存态）
  String? get token => _token;

  /// 当前刷新令牌（内存态）
  String? get refreshToken => _refreshToken;

  /// 当前用户 ID（内存态）
  String? get userId => _userId;
}
