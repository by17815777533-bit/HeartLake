// @file base_service.dart
// @brief 统一服务基类 - 提供HTTP请求方法和错误处理
// Created by 王璐瑶

library;

import 'package:dio/dio.dart';
import 'package:meta/meta.dart';
import 'api_client.dart';
import '../../utils/app_logger.dart';
import '../../utils/error_codes.dart';

/// 统一的响应格式
class ServiceResponse<T> {
  final bool success;
  final T? data;
  final String? message;
  final int? code;

  ServiceResponse({
    required this.success,
    this.data,
    this.message,
    this.code,
  });

  factory ServiceResponse.success(T data, {String? message}) {
    return ServiceResponse(
      success: true,
      data: data,
      message: message,
      code: 200,
    );
  }

  factory ServiceResponse.error(String message, {int? code, T? data}) {
    return ServiceResponse(
      success: false,
      message: message,
      code: code ?? 500,
      data: data,
    );
  }

  /// 从API响应创建
  factory ServiceResponse.fromResponse(Response response) {
    final data = response.data;

    if (data is Map<String, dynamic>) {
      final code = data['code'] as int?;
      final success = code == 0;

      return ServiceResponse(
        success: success,
        data: success ? (data['data'] is T ? data['data'] as T : null) : null,
        message: data['message'] as String?,
        code: code,
      );
    }

    return ServiceResponse.error('响应格式错误');
  }
}

/// 服务基类
///
/// 所有业务Service继承此类，自动获得：
/// - 统一的HTTP请求方法
/// - 统一的错误处理（由ApiClient拦截器统一处理HTTP错误码）
/// - 统一的日志记录
abstract class BaseService {
  final ApiClient _client = ApiClient();
  final AppLogger _logger = AppLogger();

  /// 子类可访问的ApiClient实例（用于高级用法如进度回调）
  @protected
  ApiClient get client => _client;

  /// Service名称（用于日志）
  String get serviceName;

  /// 从DioException提取用户友好的错误消息
  String _friendlyError(dynamic e) {
    if (e is DioException) {
      // 超时类错误
      if (e.type == DioExceptionType.connectionTimeout ||
          e.type == DioExceptionType.sendTimeout ||
          e.type == DioExceptionType.receiveTimeout) {
        return '湖面有些平静，请稍后再试~';
      }
      // 连接错误
      if (e.type == DioExceptionType.connectionError) {
        return '与心湖的连接暂时中断，请检查网络后再试~';
      }
      // HTTP响应错误 - 优先使用业务错误码
      if (e.response?.data is Map) {
        final respData = e.response!.data as Map;
        final businessCode = respData['code'] as int?;
        if (businessCode != null && businessCode != 0) {
          return ErrorCodes.friendlyMessage(businessCode);
        }
        final msg = respData['message'];
        if (msg != null && msg.toString().isNotEmpty) {
          return msg.toString();
        }
      }
      // 按HTTP状态码返回友好消息
      switch (e.response?.statusCode) {
        case 401:
          return '需要重新登录才能继续哦~';
        case 403:
          return '暂时无法访问这片湖域~';
        case 404:
          return '这颗石头似乎已经沉入湖底了~';
      }
      if (e.response?.statusCode != null && e.response!.statusCode! >= 500) {
        return '湖神正在休息，请稍后再来~';
      }
    }
    return '遇到了一点小波澜，请稍后再试~';
  }

  /// 从DioException响应体提取data字段（用于传递服务端附加数据，如心理援助信息）
  dynamic _extractData(DioException e) {
    if (e.response?.data is Map) {
      return (e.response!.data as Map<String, dynamic>)['data'];
    }
    return null;
  }

  /// GET请求
  Future<ServiceResponse<T>> get<T>(
    String path, {
    Map<String, dynamic>? queryParameters,
  }) async {
    try {
      _logger.debug('[$serviceName] GET $path');
      final response =
          await _client.get(path, queryParameters: queryParameters);
      return ServiceResponse.fromResponse(response);
    } on DioException catch (e) {
      _logger.error('[$serviceName] GET失败: $e');
      return ServiceResponse.error(_friendlyError(e), code: e.response?.statusCode, data: _extractData(e));
    } catch (e) {
      _logger.error('[$serviceName] GET失败: $e');
      return ServiceResponse.error(_friendlyError(e));
    }
  }

  /// POST请求
  Future<ServiceResponse<T>> post<T>(
    String path, {
    dynamic data,
  }) async {
    try {
      _logger.debug('[$serviceName] POST $path');
      final response = await _client.post(path, data: data);
      return ServiceResponse.fromResponse(response);
    } on DioException catch (e) {
      _logger.error('[$serviceName] POST失败: $e');
      return ServiceResponse.error(_friendlyError(e), code: e.response?.statusCode, data: _extractData(e));
    } catch (e) {
      _logger.error('[$serviceName] POST失败: $e');
      return ServiceResponse.error(_friendlyError(e));
    }
  }

  /// PUT请求
  Future<ServiceResponse<T>> put<T>(
    String path, {
    dynamic data,
  }) async {
    try {
      _logger.debug('[$serviceName] PUT $path');
      final response = await _client.put(path, data: data);
      return ServiceResponse.fromResponse(response);
    } on DioException catch (e) {
      _logger.error('[$serviceName] PUT失败: $e');
      return ServiceResponse.error(_friendlyError(e), code: e.response?.statusCode, data: _extractData(e));
    } catch (e) {
      _logger.error('[$serviceName] PUT失败: $e');
      return ServiceResponse.error(_friendlyError(e));
    }
  }

  /// DELETE请求
  Future<ServiceResponse<T>> delete<T>(
    String path, {
    dynamic data,
  }) async {
    try {
      _logger.debug('[$serviceName] DELETE $path');
      final response = await _client.delete(path, data: data);
      return ServiceResponse.fromResponse(response);
    } on DioException catch (e) {
      _logger.error('[$serviceName] DELETE失败: $e');
      return ServiceResponse.error(_friendlyError(e), code: e.response?.statusCode, data: _extractData(e));
    } catch (e) {
      _logger.error('[$serviceName] DELETE失败: $e');
      return ServiceResponse.error(_friendlyError(e));
    }
  }

  /// 转换为旧的Map格式（兼容现有代码）
  Map<String, dynamic> toMap<T>(ServiceResponse<T> response) {
    return {
      'success': response.success,
      'code': response.code,
      'message': response.message,
      'data': response.data,
    };
  }
}
