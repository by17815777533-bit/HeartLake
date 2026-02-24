// @file error_codes.dart
// @brief 后端错误码映射 - 与 ErrorCode.cpp 的 messageZhMap 保持同步
// Created by Claude

/// 错误码工具类
///
/// 将后端6位数字错误码映射为中文用户友好消息，
/// 与 backend/src/utils/ErrorCode.cpp 的 messageZhMap 一一对应。
class ErrorCodes {
  ErrorCodes._();

  // 通用错误 (1xxxxx)
  static const int invalidRequest = 100001;
  static const int invalidParameter = 100002;
  static const int missingParameter = 100003;
  static const int internalError = 100500;
  static const int serviceUnavailable = 100503;

  // 认证错误 (2xxxxx)
  static const int unauthorized = 200001;
  static const int tokenExpired = 200002;
  static const int tokenInvalid = 200003;
  static const int permissionDenied = 200004;
  static const int loginRequired = 200005;

  // 用户错误 (3xxxxx)
  static const int userNotFound = 300001;
  static const int userAlreadyExists = 300002;
  static const int emailAlreadyExists = 300003;
  static const int invalidEmail = 300004;
  static const int invalidPassword = 300005;
  static const int passwordTooWeak = 300006;
  static const int verificationCodeInvalid = 300007;
  static const int verificationCodeExpired = 300008;
  static const int emailNotVerified = 300009;
  static const int userSuspended = 300010;
  static const int userDeleted = 300011;

  // 内容错误 (4xxxxx)
  static const int contentNotFound = 400001;
  static const int contentDeleted = 400002;
  static const int contentTooLong = 400003;
  static const int contentEmpty = 400004;
  static const int contentSensitive = 400005;
  static const int contentModerationFailed = 400006;
  static const int rateLimitExceeded = 400007;
  static const int duplicateContent = 400008;

  // 好友错误 (41xxxx)
  static const int friendNotFound = 410001;
  static const int alreadyFriends = 410002;
  static const int friendRequestExists = 410003;
  static const int cannotAddSelf = 410004;
  static const int friendLimitExceeded = 410005;

  // AI服务错误 (5xxxxx)
  static const int aiServiceError = 500001;
  static const int aiApiError = 500002;
  static const int aiTimeout = 500003;
  static const int aiQuotaExceeded = 500004;
  static const int aiInvalidResponse = 500005;

  // 数据库错误 (6xxxxx)
  static const int databaseError = 600001;
  static const int databaseConnectionFailed = 600002;
  static const int duplicateKey = 600003;
  static const int foreignKeyViolation = 600004;

  // 网络错误 (7xxxxx)
  static const int networkError = 700001;
  static const int requestTimeout = 700002;
  static const int upstreamError = 700003;

  /// 错误码 → 中文消息映射（与 ErrorCode.cpp messageZhMap 同步）
  static const Map<int, String> _messages = {
    invalidRequest: '无效的请求',
    invalidParameter: '参数无效',
    missingParameter: '缺少必需参数',
    internalError: '服务器内部错误',
    serviceUnavailable: '服务暂时不可用',
    unauthorized: '未授权',
    tokenExpired: '登录已过期，请重新登录',
    tokenInvalid: '无效的登录凭证',
    permissionDenied: '权限不足',
    loginRequired: '请先登录',
    userNotFound: '用户不存在',
    userAlreadyExists: '用户已存在',
    emailAlreadyExists: '邮箱已被注册',
    invalidEmail: '邮箱格式不正确',
    invalidPassword: '密码不正确',
    passwordTooWeak: '密码强度不够',
    verificationCodeInvalid: '验证码错误',
    verificationCodeExpired: '验证码已过期',
    emailNotVerified: '邮箱未验证',
    userSuspended: '账户已被停用',
    userDeleted: '账户已删除',
    contentNotFound: '内容不存在',
    contentDeleted: '内容已被删除',
    contentTooLong: '内容过长',
    contentEmpty: '内容不能为空',
    contentSensitive: '内容包含敏感信息',
    contentModerationFailed: '内容审核未通过',
    rateLimitExceeded: '操作过于频繁，请稍后再试',
    duplicateContent: '内容重复',
    friendNotFound: '好友不存在',
    alreadyFriends: '已经是好友了',
    friendRequestExists: '好友请求已存在',
    cannotAddSelf: '不能添加自己为好友',
    friendLimitExceeded: '好友数量已达上限',
    aiServiceError: 'AI服务暂时不可用',
    aiApiError: 'AI服务响应异常',
    aiTimeout: 'AI服务响应超时',
    aiQuotaExceeded: 'AI服务配额已用完',
    aiInvalidResponse: 'AI服务返回无效',
    databaseError: '数据库错误',
    databaseConnectionFailed: '数据库连接失败',
    duplicateKey: '数据已存在',
    foreignKeyViolation: '数据关联错误',
    networkError: '网络错误',
    requestTimeout: '请求超时',
    upstreamError: '上游服务错误',
  };

  /// 根据错误码获取中文消息
  ///
  /// 优先返回服务端消息 [serverMessage]（如果非空且非通用），
  /// 否则从本地映射查找，最后返回默认消息。
  static String getMessage(int? code, {String? serverMessage}) {
    // 服务端消息非空且有实际内容时优先使用
    if (serverMessage != null && serverMessage.trim().isNotEmpty) {
      return serverMessage;
    }
    // 从本地映射查找
    if (code != null && _messages.containsKey(code)) {
      return _messages[code]!;
    }
    // 默认消息
    return '遇到了一点小波澜，请稍后再试~';
  }

  /// 判断错误码是否属于认证类错误
  static bool isAuthError(int? code) {
    return code != null && code >= 200001 && code <= 200005;
  }

  /// 判断错误码是否属于限流错误
  static bool isRateLimitError(int? code) {
    return code == rateLimitExceeded;
  }
}
