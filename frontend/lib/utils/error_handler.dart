// @file error_handler.dart
// @brief 统一错误处理
// Created by 王璐瑶

library;

import 'package:dio/dio.dart';
import 'app_logger.dart';

/// 应用错误类型
enum AppErrorType {
  /// 网络错误
  network,

  /// 服务器错误
  server,

  /// 认证错误
  auth,

  /// 参数错误
  validation,

  /// 未找到
  notFound,

  /// 权限不足
  forbidden,

  /// 超时
  timeout,

  /// 取消
  cancelled,

  /// 未知错误
  unknown,
}

/// 应用错误类
class AppError implements Exception {
  /// 错误类型
  final AppErrorType type;

  /// 错误消息
  final String message;

  /// 错误代码
  final String? code;

  /// 原始错误
  final dynamic originalError;

  /// HTTP状态码
  final int? statusCode;

  AppError({
    required this.type,
    required this.message,
    this.code,
    this.originalError,
    this.statusCode,
  });

  /// 从Dio异常创建AppError
  factory AppError.fromDioException(DioException e) {
    switch (e.type) {
      case DioExceptionType.connectionTimeout:
      case DioExceptionType.sendTimeout:
      case DioExceptionType.receiveTimeout:
        return AppError(
          type: AppErrorType.timeout,
          message: '请求超时，请检查网络连接',
          originalError: e,
        );

      case DioExceptionType.connectionError:
        return AppError(
          type: AppErrorType.network,
          message: '网络连接失败，请检查网络设置',
          originalError: e,
        );

      case DioExceptionType.cancel:
        return AppError(
          type: AppErrorType.cancelled,
          message: '请求已取消',
          originalError: e,
        );

      case DioExceptionType.badResponse:
        return _handleBadResponse(e);

      case DioExceptionType.badCertificate:
        return AppError(
          type: AppErrorType.network,
          message: '证书验证失败',
          originalError: e,
        );

      default:
        return AppError(
          type: AppErrorType.unknown,
          message: e.message ?? '未知网络错误',
          originalError: e,
        );
    }
  }

  /// 处理错误响应
  static AppError _handleBadResponse(DioException e) {
    final response = e.response;
    final statusCode = response?.statusCode ?? 0;

    // 尝试从响应体获取错误信息
    String message = '服务器错误';
    String? code;

    if (response?.data is Map) {
      final data = response!.data as Map;
      message = data['message'] ?? data['error'] ?? '服务器错误';
      code = data['code']?.toString();
    }

    switch (statusCode) {
      case 400:
        return AppError(
          type: AppErrorType.validation,
          message: message,
          code: code,
          statusCode: statusCode,
          originalError: e,
        );

      case 401:
        return AppError(
          type: AppErrorType.auth,
          message: '登录已过期，请重新登录',
          code: code,
          statusCode: statusCode,
          originalError: e,
        );

      case 403:
        return AppError(
          type: AppErrorType.forbidden,
          message: '没有权限执行此操作',
          code: code,
          statusCode: statusCode,
          originalError: e,
        );

      case 404:
        return AppError(
          type: AppErrorType.notFound,
          message: '请求的资源不存在',
          code: code,
          statusCode: statusCode,
          originalError: e,
        );

      case 500:
      case 502:
      case 503:
        return AppError(
          type: AppErrorType.server,
          message: '服务器繁忙，请稍后重试',
          code: code,
          statusCode: statusCode,
          originalError: e,
        );

      default:
        return AppError(
          type: AppErrorType.server,
          message: message,
          code: code,
          statusCode: statusCode,
          originalError: e,
        );
    }
  }

  /// 获取用户友好的错误消息
  String get userMessage {
    switch (type) {
      case AppErrorType.network:
        return '网络不太给力，请检查连接后重试';
      case AppErrorType.server:
        return '服务器开小差了，请稍后再试';
      case AppErrorType.auth:
        return '请重新登录';
      case AppErrorType.validation:
        return message;
      case AppErrorType.notFound:
        return '内容已被删除或不存在';
      case AppErrorType.forbidden:
        return '暂无权限';
      case AppErrorType.timeout:
        return '请求超时，请重试';
      case AppErrorType.cancelled:
        return '操作已取消';
      case AppErrorType.unknown:
        return '发生了一点小问题';
    }
  }

  /// 是否需要重新登录
  bool get requiresReauth => type == AppErrorType.auth;

  /// 是否可以重试
  bool get canRetry =>
      type == AppErrorType.network ||
      type == AppErrorType.timeout ||
      type == AppErrorType.server;

  @override
  String toString() => 'AppError($type): $message';
}

/// 错误处理工具类
class ErrorHandler {
  /// 处理并记录错误
  static AppError handle(dynamic error, {String? context}) {
    AppError appError;

    if (error is AppError) {
      appError = error;
    } else if (error is DioException) {
      appError = AppError.fromDioException(error);
    } else {
      appError = AppError(
        type: AppErrorType.unknown,
        message: error.toString(),
        originalError: error,
      );
    }

    // 记录错误日志
    logger.error(
      '${context ?? 'Error'}: ${appError.message}',
      error: appError.originalError,
    );

    return appError;
  }

  /// 安全执行异步操作
  static Future<T?> safeAsync<T>(
    Future<T> Function() operation, {
    String? context,
    T? defaultValue,
    void Function(AppError)? onError,
  }) async {
    try {
      return await operation();
    } catch (e) {
      final error = handle(e, context: context);
      onError?.call(error);
      return defaultValue;
    }
  }

  /// 安全执行同步操作
  static T? safeSync<T>(
    T Function() operation, {
    String? context,
    T? defaultValue,
    void Function(AppError)? onError,
  }) {
    try {
      return operation();
    } catch (e) {
      final error = handle(e, context: context);
      onError?.call(error);
      return defaultValue;
    }
  }
}

/// Result类型 - 用于返回成功或失败
class Result<T> {
  final T? data;
  final AppError? error;

  const Result._({this.data, this.error});

  /// 成功结果
  factory Result.success(T data) => Result._(data: data);

  /// 失败结果
  factory Result.failure(AppError error) => Result._(error: error);

  /// 是否成功
  bool get isSuccess => error == null;

  /// 是否失败
  bool get isFailure => error != null;

  /// 获取数据，失败时抛出异常
  T get dataOrThrow {
    if (error != null) throw error!;
    return data as T;
  }

  /// 获取数据，失败时返回默认值
  T dataOr(T defaultValue) => data ?? defaultValue;

  /// 映射成功数据
  Result<R> map<R>(R Function(T) mapper) {
    if (isSuccess) {
      return Result.success(mapper(data as T));
    }
    return Result.failure(error!);
  }

  /// 处理结果
  R fold<R>({
    required R Function(T data) onSuccess,
    required R Function(AppError error) onFailure,
  }) {
    if (isSuccess) {
      return onSuccess(data as T);
    }
    return onFailure(error!);
  }
}
