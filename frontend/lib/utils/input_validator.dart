// 输入验证工具
//
// 提供前端参数校验，在数据发送到后端之前进行预校验，包括字符串校验、
// UUID格式校验、XSS过滤、文件类型和大小限制等。所有校验失败均抛出
// ValidationException。

/// 验证异常
///
/// 参数不合法时抛出，携带用户友好的错误消息。
class ValidationException implements Exception {
  final String message;
  const ValidationException(this.message);

  @override
  String toString() => message;
}

/// 输入验证工具集
///
/// 提供统一的参数校验方法。
class InputValidator {
  InputValidator._();

  // UUID v4 正则（标准格式 8-4-4-4-12）
  static final _uuidRegex = RegExp(
    r'^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$',
  );

  // 业务ID常见格式：前缀 + UUID（如 stone_xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）
  static final _prefixedUuidRegex = RegExp(
    r'^[A-Za-z][A-Za-z0-9]*_[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$',
  );

  // 通用安全ID格式：字母数字开头，可包含下划线和连字符（如 anonymous_xxx、admin_001）
  static final _safeIdRegex = RegExp(
    r'^[A-Za-z0-9][A-Za-z0-9_-]{0,127}$',
  );

  // HTML标签匹配（含自闭合标签）
  static final _htmlTagRegex = RegExp(r'<[^>]*>', caseSensitive: false);

  // script标签匹配（含内容）
  static final _scriptRegex = RegExp(
    r'<script[^>]*>[\s\S]*?</script>',
    caseSensitive: false,
  );

  // 事件属性匹配（on开头的HTML属性，如 onclick、onerror）
  static final _eventAttrRegex = RegExp(
    r"""\s+on\w+\s*=\s*["'][^"']*["']""",
    caseSensitive: false,
  );

  // URL格式校验
  static final _urlRegex = RegExp(
    r'^https?://[^\s/$.?#].[^\s]*$',
    caseSensitive: false,
  );

  // Base64格式校验（标准或URL安全变体）
  static final _base64Regex = RegExp(
    r'^[A-Za-z0-9+/\-_]+=*$',
  );

  // ISO 8601 日期时间格式
  static final _iso8601Regex = RegExp(
    r'^\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}(:\d{2})?(\.\d+)?(Z|[+-]\d{2}:\d{2})?$',
  );

  /// 校验非空字符串
  ///
  /// 去除首尾空白后判断是否为空。
  ///
  /// [value] 待校验的值
  /// [fieldName] 字段名称，用于错误提示
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

  /// 安全ID格式校验
  ///
  /// 兼容UUID、前缀UUID、业务ID，并防止路径注入攻击。
  ///
  /// [id] 待校验的ID
  /// [fieldName] 字段名称
  static String validateUUID(String? id, String fieldName) {
    if (id == null || id.isEmpty) {
      throw ValidationException('$fieldName不能为空');
    }
    // 防止路径遍历攻击（如 ../、./、/etc/passwd）
    if (id.contains('..') || id.contains('/') || id.contains('\\')) {
      throw ValidationException('$fieldName包含非法字符');
    }
    if (_uuidRegex.hasMatch(id) ||
        _prefixedUuidRegex.hasMatch(id) ||
        _safeIdRegex.hasMatch(id)) {
      return id;
    }
    throw ValidationException('$fieldName格式无效');
  }

  /// XSS过滤
  ///
  /// 去除HTML标签、script标签、事件属性，防止XSS攻击。
  ///
  /// [text] 待过滤的文本
  static String sanitizeText(String text) {
    var sanitized = text;
    // 先移除 script 标签及其内容
    sanitized = sanitized.replaceAll(_scriptRegex, '');
    // 移除事件属性（onclick、onerror 等）
    sanitized = sanitized.replaceAll(_eventAttrRegex, '');
    // 移除所有 HTML 标签
    sanitized = sanitized.replaceAll(_htmlTagRegex, '');
    // 转义残留的尖括号，防止拼接后形成新标签
    sanitized = sanitized.replaceAll('<', '&lt;').replaceAll('>', '&gt;');
    return sanitized;
  }

  /// 文件类型白名单校验
  static String validateFileType(
    String filename,
    List<String> allowedExtensions,
  ) {
    if (filename.isEmpty) {
      throw const ValidationException('文件名不能为空');
    }
    final dotIndex = filename.lastIndexOf('.');
    if (dotIndex < 0 || dotIndex == filename.length - 1) {
      throw const ValidationException('文件缺少扩展名');
    }
    final ext = filename.substring(dotIndex + 1).toLowerCase();
    if (!allowedExtensions.contains(ext)) {
      throw ValidationException('不支持的文件类型: .$ext，允许: ${allowedExtensions.map((e) => '.$e').join(', ')}');
    }
    return filename;
  }

  /// 文件大小限制校验
  static int validateFileSize(int bytes, {int maxMB = 10}) {
    if (bytes <= 0) {
      throw const ValidationException('文件大小无效');
    }
    final maxBytes = maxMB * 1024 * 1024;
    if (bytes > maxBytes) {
      throw ValidationException('文件大小超过${maxMB}MB限制');
    }
    return bytes;
  }

  /// 日期范围校验（年+月）
  static void validateDateRange(int year, int month) {
    requireYear(year);
    requireMonth(month);
  }

  /// URL 格式校验
  static String validateUrl(String? url) {
    if (url == null || url.isEmpty) {
      throw const ValidationException('URL不能为空');
    }
    if (!_urlRegex.hasMatch(url)) {
      throw const ValidationException('URL格式无效');
    }
    return url;
  }

  /// 枚举白名单校验
  static String validateEnum(
    String? value,
    List<String> allowed,
    String fieldName,
  ) {
    if (value == null || !allowed.contains(value)) {
      throw ValidationException('$fieldName不在允许范围内，允许值: ${allowed.join(', ')}');
    }
    return value;
  }

  /// Map key 白名单校验 - 过滤掉不允许的 key
  static Map<String, dynamic> validateMapKeys(
    Map<String, dynamic> map,
    List<String> allowedKeys,
  ) {
    final filtered = <String, dynamic>{};
    for (final key in map.keys) {
      if (allowedKeys.contains(key)) {
        filtered[key] = map[key];
      }
    }
    return filtered;
  }

  /// Base64 格式校验
  static String validateBase64(String? value, String fieldName) {
    if (value == null || value.isEmpty) {
      throw ValidationException('$fieldName不能为空');
    }
    if (!_base64Regex.hasMatch(value)) {
      throw ValidationException('$fieldName不是有效的Base64格式');
    }
    return value;
  }

  /// ISO 8601 日期时间格式校验，且不能是过去时间
  static String validateFutureISO8601(String? value, String fieldName) {
    if (value == null || value.isEmpty) {
      throw ValidationException('$fieldName不能为空');
    }
    if (!_iso8601Regex.hasMatch(value)) {
      throw ValidationException('$fieldName格式无效，需要ISO 8601格式');
    }
    final parsed = DateTime.tryParse(value);
    if (parsed == null) {
      throw ValidationException('$fieldName无法解析为有效日期');
    }
    if (parsed.isBefore(DateTime.now())) {
      throw ValidationException('$fieldName不能是过去的时间');
    }
    return value;
  }

  /// 校验非空 Map
  static Map<String, dynamic> requireNonEmptyMap(
    Map<String, dynamic>? map,
    String fieldName,
  ) {
    if (map == null || map.isEmpty) {
      throw ValidationException('$fieldName不能为空');
    }
    return map;
  }

  /// 校验浮点数范围
  static double requireDoubleRange(
    double value,
    String fieldName, {
    double min = 0.0,
    double max = 1.0,
  }) {
    if (value < min || value > max) {
      throw ValidationException('$fieldName应在$min-$max之间');
    }
    return value;
  }
}
