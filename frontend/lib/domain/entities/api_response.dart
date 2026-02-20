// @file api_response.dart
// @brief 通用API响应模型
// Created by 王璐瑶

/// 通用 API 响应模型
class ApiResponse<T> {
  final int code;
  final String message;
  final T? data;
  final bool success;

  ApiResponse({
    required this.code,
    required this.message,
    this.data,
  }) : success = code == 0;

  factory ApiResponse.fromJson(
    Map<String, dynamic> json,
    T Function(dynamic)? fromJsonT,
  ) {
    return ApiResponse(
      code: json['code'] as int? ?? 500,
      message: json['message'] as String? ?? '',
      data: json['data'] != null && fromJsonT != null
          ? fromJsonT(json['data'])
          : json['data'] as T?,
    );
  }

  factory ApiResponse.success(T? data, [String message = '操作成功']) {
    return ApiResponse(code: 0, message: message, data: data);
  }

  factory ApiResponse.error(String message, [int code = 500]) {
    return ApiResponse(code: code, message: message);
  }
}

/// 分页响应模型
class PagedResponse<T> {
  final List<T> items;
  final int total;
  final int page;
  final int pageSize;

  PagedResponse({
    required this.items,
    required this.total,
    required this.page,
    required this.pageSize,
  });

  bool get hasMore => page * pageSize < total;

  factory PagedResponse.fromJson(
    Map<String, dynamic> json,
    T Function(Map<String, dynamic>) fromJsonT,
  ) {
    final itemsList = json['items'] as List? ?? [];
    return PagedResponse(
      items: itemsList.map((e) => fromJsonT(e as Map<String, dynamic>)).toList(),
      total: json['total'] as int? ?? 0,
      page: json['page'] as int? ?? 1,
      pageSize: json['page_size'] as int? ?? 20,
    );
  }
}
