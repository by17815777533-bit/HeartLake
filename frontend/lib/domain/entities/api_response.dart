// 通用API响应模型
//
// 后端统一返回{code, message, data}结构，code为0表示成功。
// 泛型参数T对应data字段的业务类型，通过fromJsonT反序列化。

/// 通用API响应模型
///
/// 封装后端统一响应格式。
class ApiResponse<T> {
  /// 业务状态码
  ///
  /// 0表示成功，非0为具体错误码。
  final int code;

  /// 提示信息
  final String message;

  /// 业务数据
  final T? data;

  /// 是否成功
  ///
  /// code为0时为true。
  final bool success;

  ApiResponse({
    required this.code,
    required this.message,
    this.data,
  }) : success = code == 0;

  /// 从JSON反序列化
  ///
  /// [json] JSON数据
  /// [fromJsonT] data字段的反序列化函数
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
///
/// 封装列表数据和分页元信息，后端返回{items, total, page, page_size}结构。
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
