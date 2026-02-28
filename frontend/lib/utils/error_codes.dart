/// 业务错误码映射
///
/// 与后端ErrorCode.h对齐，提供错误码常量和用户友好的错误消息转换。
/// 错误码分段规则：100xxx通用错误、200xxx认证错误、300xxx用户错误、
/// 400xxx内容错误、410xxx好友错误、500xxx AI错误、600xxx数据库错误、
/// 700xxx网络错误。

/// 业务错误码常量
///
/// 与后端ErrorCode.h一一对应。
class ErrorCodes {
  ErrorCodes._();

  static const int success = 200;

  // 通用错误 100xxx
  static const int invalidRequest = 100001;
  static const int invalidParameter = 100002;
  static const int missingParameter = 100003;
  static const int internalError = 100500;
  static const int serviceUnavailable = 100503;

  // 认证错误 200xxx
  static const int unauthorized = 200001;
  static const int tokenExpired = 200002;
  static const int tokenInvalid = 200003;
  static const int permissionDenied = 200004;
  static const int loginRequired = 200005;

  // 用户错误 300xxx
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

  // 内容错误 400xxx
  static const int contentNotFound = 400001;
  static const int contentDeleted = 400002;
  static const int contentTooLong = 400003;
  static const int contentEmpty = 400004;
  static const int contentSensitive = 400005;
  static const int contentModerationFailed = 400006;
  static const int rateLimitExceeded = 400007;
  static const int duplicateContent = 400008;

  // 好友错误 410xxx
  static const int friendNotFound = 410001;
  static const int alreadyFriends = 410002;
  static const int friendRequestExists = 410003;
  static const int cannotAddSelf = 410004;
  static const int friendLimitExceeded = 410005;

  // AI 错误 500xxx
  static const int aiServiceError = 500001;
  static const int aiApiError = 500002;
  static const int aiTimeout = 500003;
  static const int aiQuotaExceeded = 500004;
  static const int aiInvalidResponse = 500005;

  // 数据库错误 600xxx
  static const int databaseError = 600001;
  static const int databaseConnectionFailed = 600002;
  static const int duplicateKey = 600003;
  static const int foreignKeyViolation = 600004;

  // 网络错误 700xxx
  static const int networkError = 700001;
  static const int requestTimeout = 700002;
  static const int upstreamError = 700003;

  /// 错误码转用户友好消息
  ///
  /// 将错误码转为心湖风格的用户提示语。
  ///
  /// [code] 错误码
  static String friendlyMessage(int? code) {
    switch (code) {
      case success:
        return '操作成功';
      // 认证
      case unauthorized:
      case loginRequired:
        return '需要重新登录才能继续哦~';
      case tokenExpired:
        return '登录已过期，请重新登录~';
      case tokenInvalid:
        return '登录凭证无效，请重新登录~';
      case permissionDenied:
        return '暂时无法访问这片湖域~';
      // 用户
      case userNotFound:
        return '找不到这位湖友~';
      case userAlreadyExists:
      case emailAlreadyExists:
        return '这个身份已经在湖边了~';
      case invalidEmail:
        return '邮箱格式不太对哦~';
      case invalidPassword:
        return '密码不正确~';
      case passwordTooWeak:
        return '密码太简单了，再想一个更安全的吧~';
      case verificationCodeInvalid:
        return '验证码不正确~';
      case verificationCodeExpired:
        return '验证码已过期，请重新获取~';
      case emailNotVerified:
        return '请先验证邮箱~';
      case userSuspended:
        return '账号已被停用~';
      case userDeleted:
        return '账号已被删除~';
      // 内容
      case contentNotFound:
        return '这颗石头似乎已经沉入湖底了~';
      case contentDeleted:
        return '内容已被删除~';
      case contentTooLong:
        return '内容太长了，精简一下吧~';
      case contentEmpty:
        return '内容不能为空哦~';
      case contentSensitive:
        return '内容包含敏感词，请修改后再试~';
      case contentModerationFailed:
        return '内容审核未通过~';
      case rateLimitExceeded:
        return '操作太频繁了，休息一下吧~';
      case duplicateContent:
        return '这条内容已经存在了~';
      // 好友
      case friendNotFound:
        return '找不到这位好友~';
      case alreadyFriends:
        return '你们已经是好友了~';
      case friendRequestExists:
        return '好友请求已发送，耐心等待回复吧~';
      case cannotAddSelf:
        return '不能添加自己为好友哦~';
      case friendLimitExceeded:
        return '好友数量已达上限~';
      // AI
      case aiServiceError:
      case aiApiError:
      case aiInvalidResponse:
        return '湖神暂时无法回应，请稍后再试~';
      case aiTimeout:
        return '湖神思考太久了，请稍后再试~';
      case aiQuotaExceeded:
        return '今日与湖神的对话次数已用完~';
      // 数据库
      case databaseError:
      case databaseConnectionFailed:
        return '湖底数据暂时无法访问~';
      case duplicateKey:
        return '数据已存在~';
      // 网络
      case networkError:
        return '与心湖的连接暂时中断~';
      case requestTimeout:
        return '湖面有些平静，请稍后再试~';
      case upstreamError:
        return '湖神正在休息，请稍后再来~';
      // 通用
      case invalidRequest:
      case invalidParameter:
      case missingParameter:
        return '请求参数有误，请检查后再试~';
      case internalError:
      case serviceUnavailable:
        return '湖神正在休息，请稍后再来~';
      default:
        return '遇到了一点小波澜，请稍后再试~';
    }
  }

  /// 是否需要重新登录
  ///
  /// 判断错误码是否表示需要重新登录。
  ///
  /// [code] 错误码
  static bool requiresReLogin(int? code) {
    return code == unauthorized ||
        code == tokenExpired ||
        code == tokenInvalid ||
        code == loginRequired;
  }
}
