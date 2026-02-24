// @file api_client.dart
// @brief API客户端 - 统一HTTP请求处理
// Created by 王璐瑶

library;

import 'dart:io';
import 'package:dio/dio.dart';
import 'package:dio/io.dart';
import '../../utils/storage_util.dart';
import '../../utils/app_config.dart';
import '../../utils/app_logger.dart';
import '../../utils/error_handler.dart' show ErrorHandler, Result;
import 'cache_service.dart';

/// Token刷新回调
typedef TokenRefreshCallback = Future<String?> Function(String? refreshToken);

/// API客户端 - 单例模式
///
/// 提供统一的HTTP请求接口，自动处理：
/// - Token认证
/// - 错误处理
/// - 日志记录
/// - 请求重试
class ApiClient {
  static final ApiClient _instance = ApiClient._internal();
  factory ApiClient() => _instance;

  late Dio _dio;
  String? _token;
  String? _refreshToken;
  String? _userId;
  bool _isRefreshing = false;
  bool _hasTriggeredUnauthorized = false;
  TokenRefreshCallback? _tokenRefreshCallback;
  Function()? _onUnauthorized;
  final CacheService cacheService = CacheService();

  ApiClient._internal() {
    _initializeDio();
    _loadToken();
  }

  /// 初始化Dio实例
  void _initializeDio() {
    _dio = Dio(BaseOptions(
      baseUrl: appConfig.apiBaseUrl,
      connectTimeout: appConfig.connectTimeout,
      receiveTimeout: appConfig.receiveTimeout,
      sendTimeout: appConfig.sendTimeout,
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      // 启用持久连接以提升性能
      persistentConnection: true,
      // 响应数据格式
      responseType: ResponseType.json,
      // P1-3: 移除 validateStatus: true，让 Dio 对非 2xx 正常抛出异常
    ));

    // 配置HTTP客户端适配器以优化性能
    if (_dio.httpClientAdapter is IOHttpClientAdapter) {
      (_dio.httpClientAdapter as IOHttpClientAdapter).createHttpClient = () {
        final client = HttpClient();
        client.maxConnectionsPerHost = appConfig.maxConnections;
        client.idleTimeout = appConfig.idleTimeout;
        client.connectionTimeout = appConfig.connectTimeout;
        // S-3: 生产环境拒绝无效证书，防止中间人攻击
        if (appConfig.isProduction) {
          client.badCertificateCallback =
              (X509Certificate cert, String host, int port) => false;
        }
        return client;
      };
    }

    // 添加请求拦截器
    _dio.interceptors.add(InterceptorsWrapper(
      onRequest: _onRequest,
      onResponse: _onResponse,
      onError: _onError,
    ));

    logger.info('API客户端初始化完成: ${appConfig.apiBaseUrl}',
        category: LogCategory.network);
  }

  /// 请求拦截器
  Future<void> _onRequest(
    RequestOptions options,
    RequestInterceptorHandler handler,
  ) async {
    // 确保Token已加载
    _token ??= await StorageUtil.getToken();
    _userId ??= await StorageUtil.getUserId();

    // 添加认证头
    if (_token != null) {
      options.headers['Authorization'] = 'Bearer $_token';
    }
    if (_userId != null) {
      options.headers['X-User-Id'] = _userId;
    }

    // 添加请求ID用于追踪
    options.headers['X-Request-Id'] = _generateRequestId();

    // 记录请求日志
    logger.network(options.method, options.uri.toString());

    return handler.next(options);
  }

  /// 响应拦截器
  void _onResponse(
    Response response,
    ResponseInterceptorHandler handler,
  ) {
    // 记录成功响应
    logger.network(
      response.requestOptions.method,
      response.requestOptions.uri.toString(),
      statusCode: response.statusCode,
    );

    return handler.next(response);
  }

  /// 错误拦截器 - 统一处理所有HTTP错误码
  Future<void> _onError(
    DioException error,
    ErrorInterceptorHandler handler,
  ) async {
    final statusCode = error.response?.statusCode;
    final requestPath = error.requestOptions.uri.toString();

    // 记录错误
    logger.network(
      error.requestOptions.method,
      requestPath,
      statusCode: statusCode,
      error: error.message,
    );

    // 处理超时错误
    if (error.type == DioExceptionType.connectionTimeout ||
        error.type == DioExceptionType.sendTimeout ||
        error.type == DioExceptionType.receiveTimeout) {
      logger.warning('请求超时: $requestPath', category: LogCategory.network);
      return handler.next(error);
    }

    // 处理连接错误
    if (error.type == DioExceptionType.connectionError) {
      logger.warning('连接失败: $requestPath', category: LogCategory.network);
      return handler.next(error);
    }

    // 处理401未授权错误 - 尝试刷新Token后重试
    if (statusCode == 401) {
      final refreshed = await _handleUnauthorized();
      if (refreshed) {
        // Token刷新成功，用新Token重试原请求
        try {
          final opts = error.requestOptions;
          opts.headers['Authorization'] = 'Bearer $_token';
          final response = await _dio.fetch(opts);
          return handler.resolve(response);
        } catch (retryError) {
          logger.error('Token刷新后重试失败', category: LogCategory.network);
          // 重试也失败了，走正常错误流程
        }
      } else {
        // Token刷新失败，触发未授权回调跳转登录（防重复触发）
        if (!_hasTriggeredUnauthorized) {
          _hasTriggeredUnauthorized = true;
          _onUnauthorized?.call();
        }
      }
      return handler.next(error);
    }

    // 处理403禁止访问
    if (statusCode == 403) {
      logger.warning('权限不足: $requestPath', category: LogCategory.network);
      return handler.next(error);
    }

    // 处理404资源不存在
    if (statusCode == 404) {
      logger.warning('资源不存在: $requestPath', category: LogCategory.network);
      return handler.next(error);
    }

    // 处理500+服务器错误
    if (statusCode != null && statusCode >= 500) {
      logger.error('服务器错误[$statusCode]: $requestPath',
          category: LogCategory.network);
      return handler.next(error);
    }

    return handler.next(error);
  }

  /// 处理未授权错误 - 尝试刷新Token
  Future<bool> _handleUnauthorized() async {
    if (_isRefreshing) return false;
    _isRefreshing = true;

    // 如果没有设置刷新回调，尝试使用内置刷新逻辑
    if (_tokenRefreshCallback == null) {
      return _tryBuiltinTokenRefresh();
    }
    try {
      _refreshToken ??= await StorageUtil.getRefreshToken();
      final newToken = await _tokenRefreshCallback!(_refreshToken);
      if (newToken != null) {
        setToken(newToken);
        logger.auth('Token刷新成功');
        return true;
      }
    } catch (e) {
      logger.auth('Token刷新失败: $e', success: false);
    } finally {
      _isRefreshing = false;
    }
    clearToken();
    return false;
  }

  /// 内置Token刷新 - 直接调用 /auth/refresh 接口
  Future<bool> _tryBuiltinTokenRefresh() async {
    if (_token == null) {
      clearToken();
      return false;
    }
    try {
      _refreshToken ??= await StorageUtil.getRefreshToken();
      final response = await _dio.post('/auth/refresh',
          data: {'refresh_token': _refreshToken});
      final data = response.data;
      if (data is Map<String, dynamic> &&
          data['code'] == 0 &&
          data['data']?['token'] != null) {
        setToken(data['data']['token']);
        logger.auth('Token内置刷新成功');
        return true;
      }
    } catch (e) {
      logger.auth('Token内置刷新失败: $e', success: false);
    } finally {
      _isRefreshing = false;
    }
    clearToken();
    return false;
  }

  /// 生成请求ID
  String _generateRequestId() {
    return '${DateTime.now().millisecondsSinceEpoch}-${_hashCode()}';
  }

  int _hashCode() => identityHashCode(this) % 10000;

  /// 加载保存的Token
  Future<void> _loadToken() async {
    _token = await StorageUtil.getToken();
    _userId = await StorageUtil.getUserId();
  }

  // ============================================================
  // 公开API
  // ============================================================

  /// 设置Token
  void setToken(String token, {String? refreshToken}) {
    _token = token;
    _hasTriggeredUnauthorized = false;
    StorageUtil.saveToken(token);
    if (refreshToken != null) {
      _refreshToken = refreshToken;
      StorageUtil.saveRefreshToken(refreshToken);
    }
    logger.auth('Token已设置');
  }

  /// 设置Token刷新回调
  void setTokenRefreshCallback(TokenRefreshCallback callback) {
    _tokenRefreshCallback = callback;
  }

  /// 设置未授权回调（token过期且刷新失败时触发）
  void setOnUnauthorized(Function() callback) {
    _onUnauthorized = callback;
  }

  /// 设置用户ID
  void setUserId(String userId) {
    _userId = userId;
    StorageUtil.saveUserId(userId);
    logger.auth('用户ID已设置: $userId');
  }

  /// 清除Token
  void clearToken() {
    _token = null;
    _refreshToken = null;
    _userId = null;
    StorageUtil.clearToken();
    StorageUtil.clearRefreshToken();
    logger.auth('Token已清除');
  }

  /// 获取当前Token
  String? get token => _token;

  /// 获取当前用户ID
  String? get userId => _userId;

  /// 是否已登录
  bool get isLoggedIn => _token != null;

  // ============================================================
  // HTTP方法
  // ============================================================

  /// GET请求
  ///
  /// [path] - API路径
  /// [queryParameters] - 查询参数
  /// [useCache] - 是否使用缓存（默认true）
  /// [cacheDuration] - 缓存有效期
  Future<Response> get(
    String path, {
    Map<String, dynamic>? queryParameters,
    bool useCache = true,
    Duration? cacheDuration,
  }) async {
    // 生成缓存键
    final cacheKey = _generateCacheKey('GET', path, queryParameters);

    // 尝试从缓存获取
    if (useCache) {
      final cachedData = cacheService.get<Map<String, dynamic>>(cacheKey);
      if (cachedData != null) {
        logger.debug('从缓存返回: $path', category: LogCategory.network);
        return Response(
          requestOptions: RequestOptions(path: path),
          data: cachedData,
          statusCode: 200,
        );
      }
    }

    // 执行网络请求
    final response = await _dio.get(path, queryParameters: queryParameters);

    // 缓存成功响应
    if (useCache && response.statusCode == 200 && response.data != null) {
      cacheService.set(cacheKey, response.data, ttl: cacheDuration);
    }

    return response;
  }

  /// 生成缓存键（包含用户ID以隔离不同用户的缓存）
  String _generateCacheKey(
      String method, String path, Map<String, dynamic>? params) {
    final userPrefix = _userId != null ? 'u:$_userId:' : '';
    if (params == null || params.isEmpty) {
      return '$userPrefix$method:$path';
    }
    final sortedParams = Map.fromEntries(
      params.entries.toList()..sort((a, b) => a.key.compareTo(b.key)),
    );
    return '$userPrefix$method:$path?${Uri(queryParameters: sortedParams.map((k, v) => MapEntry(k, v.toString()))).query}';
  }

  /// POST请求
  ///
  /// [path] - API路径
  /// [data] - 请求体数据
  Future<Response> post(String path, {dynamic data}) {
    return _dio.post(path, data: data);
  }

  /// PUT请求
  ///
  /// [path] - API路径
  /// [data] - 请求体数据
  Future<Response> put(String path, {dynamic data}) {
    return _dio.put(path, data: data);
  }

  /// PATCH请求
  ///
  /// [path] - API路径
  /// [data] - 请求体数据
  Future<Response> patch(String path, {dynamic data}) {
    return _dio.patch(path, data: data);
  }

  /// DELETE请求
  ///
  /// [path] - API路径
  Future<Response> delete(String path, {dynamic data}) {
    return _dio.delete(path, data: data);
  }

  /// 上传文件
  ///
  /// [path] - API路径
  /// [file] - 文件
  /// [fieldName] - 字段名
  /// [onProgress] - 进度回调
  Future<Response> uploadFile(
    String path, {
    required File file,
    String fieldName = 'file',
    Map<String, dynamic>? extraData,
    void Function(int, int)? onProgress,
  }) async {
    final formData = FormData.fromMap({
      fieldName: await MultipartFile.fromFile(
        file.path,
        filename: file.path.split('/').last,
      ),
      ...?extraData,
    });

    return _dio.post(
      path,
      data: formData,
      onSendProgress: onProgress,
    );
  }

  // ============================================================
  // 带错误处理的请求方法
  // ============================================================

  /// 安全GET请求 - 返回Result类型
  Future<Result<T>> safeGet<T>(
    String path, {
    Map<String, dynamic>? queryParameters,
    T Function(dynamic)? parser,
  }) async {
    try {
      final response = await get(path, queryParameters: queryParameters);
      final data = parser != null ? parser(response.data) : response.data is T ? response.data as T : response.data;
      return Result.success(data);
    } catch (e) {
      return Result.failure(ErrorHandler.handle(e, context: 'GET $path'));
    }
  }

  /// 安全POST请求 - 返回Result类型
  Future<Result<T>> safePost<T>(
    String path, {
    dynamic data,
    T Function(dynamic)? parser,
  }) async {
    try {
      final response = await post(path, data: data);
      final result =
          parser != null ? parser(response.data) : response.data as T;
      return Result.success(result);
    } catch (e) {
      return Result.failure(ErrorHandler.handle(e, context: 'POST $path'));
    }
  }

  /// 获取Dio实例（用于高级用法）
  Dio get dio => _dio;

  /// 更新基础URL
  void updateBaseUrl(String baseUrl) {
    _dio.options.baseUrl = baseUrl;
    logger.info('API基础URL已更新: $baseUrl', category: LogCategory.network);
  }
}
