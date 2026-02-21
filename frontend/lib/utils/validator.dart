// @file validator.dart
// @brief 统一验证工具
// Created by 王璐瑶

library;

class ValidationResult {
  final bool isValid;
  final String? errorMessage;

  ValidationResult.valid()
      : isValid = true,
        errorMessage = null;

  ValidationResult.invalid(this.errorMessage) : isValid = false;
}

/// 统一验证器
class Validator {
  /// 验证非空
  static ValidationResult required(String? value, String fieldName) {
    if (value == null || value.trim().isEmpty) {
      return ValidationResult.invalid('$fieldName不能为空');
    }
    return ValidationResult.valid();
  }

  /// 验证最小长度
  static ValidationResult minLength(String? value, int min, String fieldName) {
    if (value == null || value.length < min) {
      return ValidationResult.invalid('$fieldName至少需要$min个字符');
    }
    return ValidationResult.valid();
  }

  /// 验证最大长度
  static ValidationResult maxLength(String? value, int max, String fieldName) {
    if (value != null && value.length > max) {
      return ValidationResult.invalid('$fieldName不能超过$max个字符');
    }
    return ValidationResult.valid();
  }

  /// 验证长度范围
  static ValidationResult lengthRange(
      String? value, int min, int max, String fieldName) {
    final minResult = minLength(value, min, fieldName);
    if (!minResult.isValid) return minResult;

    final maxResult = maxLength(value, max, fieldName);
    if (!maxResult.isValid) return maxResult;

    return ValidationResult.valid();
  }

  /// 验证邮箱格式
  static ValidationResult email(String? value) {
    if (value == null || value.isEmpty) {
      return ValidationResult.invalid('邮箱不能为空');
    }

    final emailRegex = RegExp(r'^[\w-\.]+@([\w-]+\.)+[\w-]{2,4}$');
    if (!emailRegex.hasMatch(value)) {
      return ValidationResult.invalid('邮箱格式不正确');
    }

    return ValidationResult.valid();
  }

  /// 验证密码强度
  static ValidationResult password(String? value) {
    if (value == null || value.isEmpty) {
      return ValidationResult.invalid('密码不能为空');
    }

    if (value.length < 6) {
      return ValidationResult.invalid('密码至少需要6个字符');
    }

    if (value.length > 20) {
      return ValidationResult.invalid('密码不能超过20个字符');
    }

    return ValidationResult.valid();
  }

  /// 验证手机号（中国大陆）
  static ValidationResult phone(String? value) {
    if (value == null || value.isEmpty) {
      return ValidationResult.invalid('手机号不能为空');
    }

    final phoneRegex = RegExp(r'^1[3-9]\d{9}$');
    if (!phoneRegex.hasMatch(value)) {
      return ValidationResult.invalid('手机号格式不正确');
    }

    return ValidationResult.valid();
  }

  /// 验证数字
  static ValidationResult number(String? value, String fieldName) {
    if (value == null || value.isEmpty) {
      return ValidationResult.invalid('$fieldName不能为空');
    }

    if (double.tryParse(value) == null) {
      return ValidationResult.invalid('$fieldName必须是数字');
    }

    return ValidationResult.valid();
  }

  /// 验证数字范围
  static ValidationResult numberRange(
      double? value, double min, double max, String fieldName) {
    if (value == null) {
      return ValidationResult.invalid('$fieldName不能为空');
    }

    if (value < min || value > max) {
      return ValidationResult.invalid('$fieldName必须在$min到$max之间');
    }

    return ValidationResult.valid();
  }

  /// 验证列表长度
  static ValidationResult listLength<T>(
      List<T>? list, int max, String fieldName) {
    if (list != null && list.length > max) {
      return ValidationResult.invalid('$fieldName最多只能选择$max个');
    }
    return ValidationResult.valid();
  }

  /// 验证文件大小（字节）
  static ValidationResult fileSize(int? bytes, int maxMB, String fileName) {
    if (bytes == null) {
      return ValidationResult.invalid('$fileName大小无效');
    }

    final maxBytes = maxMB * 1024 * 1024;
    if (bytes > maxBytes) {
      return ValidationResult.invalid('$fileName不能超过${maxMB}MB');
    }

    return ValidationResult.valid();
  }

  /// 验证URL格式
  static ValidationResult url(String? value) {
    if (value == null || value.isEmpty) {
      return ValidationResult.invalid('URL不能为空');
    }

    final urlRegex = RegExp(
        r'^https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b');
    if (!urlRegex.hasMatch(value)) {
      return ValidationResult.invalid('URL格式不正确');
    }

    return ValidationResult.valid();
  }

  /// 组合多个验证规则
  static ValidationResult combine(List<ValidationResult> results) {
    for (final result in results) {
      if (!result.isValid) return result;
    }
    return ValidationResult.valid();
  }
}

/// 常用验证规则
class ValidationRules {
  // 内容验证
  static ValidationResult content(String? value) {
    return Validator.combine([
      Validator.required(value, '内容'),
      Validator.maxLength(value, 5000, '内容'),
    ]);
  }

  // 昵称验证
  static ValidationResult nickname(String? value) {
    return Validator.combine([
      Validator.required(value, '昵称'),
      Validator.lengthRange(value, 2, 20, '昵称'),
    ]);
  }

  // 标签验证
  static ValidationResult tags(List<String>? tags) {
    return Validator.listLength(tags, 5, '标签');
  }

  // 媒体文件验证
  static ValidationResult mediaFiles(List? files) {
    return Validator.listLength(files, 9, '媒体文件');
  }

  // 视频文件验证
  static ValidationResult videoFiles(List? files) {
    return Validator.listLength(files, 3, '视频');
  }

  // 评论验证
  static ValidationResult comment(String? value) {
    return Validator.combine([
      Validator.required(value, '评论'),
      Validator.lengthRange(value, 1, 500, '评论'),
    ]);
  }

  // 用户ID验证
  static ValidationResult userId(String? value) {
    return Validator.required(value, '用户ID');
  }
}
