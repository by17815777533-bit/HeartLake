// @file input_validator.dart
// @brief 输入验证工具 - 统一的参数校验，防止无效数据到达后端
// Created by 王璐瑶

/// 验证异常 - 参数不合法时抛出
class ValidationException implements Exception {
  final String message;
  const ValidationException(this.message);

  @override
  String toString() => message;
}

/// 输入验证工具集
class InputValidator {
  InputValidator._();

  // UUID v4 正则（标准格式 8-4-4-4-12）
  static final _uuidRegex = RegExp(
    r'^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$',
  );

  /// 校验非空字符串，去除首尾空白后判断
  static String requireNonEmpty(String? value, String fieldName) {
    if (value == null || value.trim().isEmpty) {
      throw ValidationException('$fieldName不能为空');
    }
    return value.trim();
  }

  /// 校验字符串长度范围
  static String requireLength(
    String value,
    String fieldName, {
    int min = 1,
    int max = 5000,
  }) {
    final trimmed = value.trim();
    if (trimmed.length < min) {
      throw ValidationException('$fieldName至少需要$min个字符');
    }
    if (trimmed.length > max) {
      throw ValidationException('$fieldName不能超过$max个字符');
    }
    return trimmed;
  }

  /// 校验UUID格式
  static String requireUUID(String? value, String fieldName) {
    if (value == null || !_uuidRegex.hasMatch(value)) {
      throw ValidationException('$fieldName格式无效');
    }
    return value;
  }

  /// 校验值在白名单内
  static String requireInList(
    String? value,
    List<String> allowed,
    String fieldName,
  ) {
    if (value == null || !allowed.contains(value)) {
      throw ValidationException('$fieldName不在允许范围内');
    }
    return value;
  }

  /// 校验可选值（非null时必须在白名单内）
  static String? optionalInList(
    String? value,
    List<String> allowed,
    String fieldName,
  ) {
    if (value == null) return null;
    return requireInList(value, allowed, fieldName);
  }

  /// 校验分页参数
  static int requirePage(int page) {
    if (page < 1 || page > 10000) {
      throw const ValidationException('页码超出有效范围');
    }
    return page;
  }

  /// 校验每页条数
  static int requirePageSize(int pageSize, {int max = 100}) {
    if (pageSize < 1 || pageSize > max) {
      throw ValidationException('每页条数应在1-$max之间');
    }
    return pageSize;
  }

  /// 校验列表长度
  static List<T> requireListLength<T>(
    List<T> list,
    String fieldName, {
    int max = 20,
  }) {
    if (list.length > max) {
      throw ValidationException('$fieldName最多$max项');
    }
    return list;
  }

  /// 校验年份范围
  static int requireYear(int year) {
    if (year < 2020 || year > 2100) {
      throw const ValidationException('年份超出有效范围');
    }
    return year;
  }

  /// 校验月份范围
  static int requireMonth(int month) {
    if (month < 1 || month > 12) {
      throw const ValidationException('月份应在1-12之间');
    }
    return month;
  }

  /// 校验正整数
  static int requirePositive(int value, String fieldName) {
    if (value <= 0) {
      throw ValidationException('$fieldName必须为正整数');
    }
    return value;
  }
}
