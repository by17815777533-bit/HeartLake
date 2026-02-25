import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/error_codes.dart';

void main() {
  // ==================== ErrorCodes 常量 ====================
  group('ErrorCodes constants', () {
    test('success should be 200', () {
      expect(ErrorCodes.success, 200);
    });

    test('common errors should be in 100xxx range', () {
      expect(ErrorCodes.invalidRequest, 100001);
      expect(ErrorCodes.invalidParameter, 100002);
      expect(ErrorCodes.missingParameter, 100003);
      expect(ErrorCodes.internalError, 100500);
      expect(ErrorCodes.serviceUnavailable, 100503);
    });

    test('auth errors should be in 200xxx range', () {
      expect(ErrorCodes.unauthorized, 200001);
      expect(ErrorCodes.tokenExpired, 200002);
      expect(ErrorCodes.tokenInvalid, 200003);
      expect(ErrorCodes.permissionDenied, 200004);
      expect(ErrorCodes.loginRequired, 200005);
    });

    test('user errors should be in 300xxx range', () {
      expect(ErrorCodes.userNotFound, 300001);
      expect(ErrorCodes.userAlreadyExists, 300002);
      expect(ErrorCodes.emailAlreadyExists, 300003);
      expect(ErrorCodes.invalidEmail, 300004);
      expect(ErrorCodes.invalidPassword, 300005);
      expect(ErrorCodes.passwordTooWeak, 300006);
      expect(ErrorCodes.verificationCodeInvalid, 300007);
      expect(ErrorCodes.verificationCodeExpired, 300008);
      expect(ErrorCodes.emailNotVerified, 300009);
      expect(ErrorCodes.userSuspended, 300010);
      expect(ErrorCodes.userDeleted, 300011);
    });

    test('content errors should be in 400xxx range', () {
      expect(ErrorCodes.contentNotFound, 400001);
      expect(ErrorCodes.contentDeleted, 400002);
      expect(ErrorCodes.contentTooLong, 400003);
      expect(ErrorCodes.contentEmpty, 400004);
      expect(ErrorCodes.contentSensitive, 400005);
      expect(ErrorCodes.contentModerationFailed, 400006);
      expect(ErrorCodes.rateLimitExceeded, 400007);
      expect(ErrorCodes.duplicateContent, 400008);
    });

    test('friend errors should be in 410xxx range', () {
      expect(ErrorCodes.friendNotFound, 410001);
      expect(ErrorCodes.alreadyFriends, 410002);
      expect(ErrorCodes.friendRequestExists, 410003);
      expect(ErrorCodes.cannotAddSelf, 410004);
      expect(ErrorCodes.friendLimitExceeded, 410005);
    });

    test('AI errors should be in 500xxx range', () {
      expect(ErrorCodes.aiServiceError, 500001);
      expect(ErrorCodes.aiApiError, 500002);
      expect(ErrorCodes.aiTimeout, 500003);
      expect(ErrorCodes.aiQuotaExceeded, 500004);
      expect(ErrorCodes.aiInvalidResponse, 500005);
    });

    test('database errors should be in 600xxx range', () {
      expect(ErrorCodes.databaseError, 600001);
      expect(ErrorCodes.databaseConnectionFailed, 600002);
      expect(ErrorCodes.duplicateKey, 600003);
      expect(ErrorCodes.foreignKeyViolation, 600004);
    });

    test('network errors should be in 700xxx range', () {
      expect(ErrorCodes.networkError, 700001);
      expect(ErrorCodes.requestTimeout, 700002);
      expect(ErrorCodes.upstreamError, 700003);
    });
  });

  // ==================== friendlyMessage ====================
  group('ErrorCodes.friendlyMessage', () {
    test('success should return 操作成功', () {
      expect(ErrorCodes.friendlyMessage(200), '操作成功');
    });

    test('auth errors should return login messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.unauthorized), contains('重新登录'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.loginRequired), contains('重新登录'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.tokenExpired), contains('过期'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.tokenInvalid), contains('无效'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.permissionDenied), contains('无法访问'));
    });

    test('user errors should return user messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.userNotFound), contains('找不到'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.userAlreadyExists), isNotEmpty);
      expect(ErrorCodes.friendlyMessage(ErrorCodes.invalidEmail), contains('邮箱'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.invalidPassword), contains('密码'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.passwordTooWeak), contains('密码'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.verificationCodeInvalid), contains('验证码'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.verificationCodeExpired), contains('验证码'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.emailNotVerified), contains('验证'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.userSuspended), contains('停用'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.userDeleted), contains('删除'));
    });

    test('content errors should return content messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.contentNotFound), contains('沉入湖底'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.contentDeleted), contains('删除'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.contentTooLong), contains('太长'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.contentEmpty), contains('不能为空'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.contentSensitive), contains('敏感'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.contentModerationFailed), contains('审核'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.rateLimitExceeded), contains('频繁'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.duplicateContent), contains('已经存在'));
    });

    test('friend errors should return friend messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.friendNotFound), contains('找不到'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.alreadyFriends), contains('已经是好友'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.friendRequestExists), contains('已发送'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.cannotAddSelf), contains('自己'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.friendLimitExceeded), contains('上限'));
    });

    test('AI errors should return AI messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.aiServiceError), contains('湖神'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.aiTimeout), contains('太久'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.aiQuotaExceeded), contains('用完'));
    });

    test('database errors should return db messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.databaseError), contains('无法访问'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.duplicateKey), contains('已存在'));
    });

    test('network errors should return network messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.networkError), contains('连接'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.requestTimeout), contains('平静'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.upstreamError), contains('湖神'));
    });

    test('common errors should return generic messages', () {
      expect(ErrorCodes.friendlyMessage(ErrorCodes.invalidRequest), contains('参数'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.internalError), contains('湖神'));
      expect(ErrorCodes.friendlyMessage(ErrorCodes.serviceUnavailable), contains('湖神'));
    });

    test('unknown code should return default message', () {
      expect(ErrorCodes.friendlyMessage(999999), contains('小波澜'));
      expect(ErrorCodes.friendlyMessage(0), contains('小波澜'));
      expect(ErrorCodes.friendlyMessage(-1), contains('小波澜'));
    });

    test('null code should return default message', () {
      expect(ErrorCodes.friendlyMessage(null), contains('小波澜'));
    });

    test('all messages should be non-empty', () {
      final codes = [
        200, 100001, 100002, 100003, 100500, 100503,
        200001, 200002, 200003, 200004, 200005,
        300001, 300002, 300003, 300004, 300005, 300006, 300007, 300008, 300009, 300010, 300011,
        400001, 400002, 400003, 400004, 400005, 400006, 400007, 400008,
        410001, 410002, 410003, 410004, 410005,
        500001, 500002, 500003, 500004, 500005,
        600001, 600002, 600003, 600004,
        700001, 700002, 700003,
      ];
      for (final code in codes) {
        expect(ErrorCodes.friendlyMessage(code), isNotEmpty, reason: 'Code $code should have message');
      }
    });

    test('all messages should end with ~', () {
      final codes = [200001, 300001, 400001, 410001, 500001, 600001, 700001];
      for (final code in codes) {
        expect(ErrorCodes.friendlyMessage(code).endsWith('~'), true, reason: 'Code $code');
      }
    });
  });

  // ==================== requiresReLogin ====================
  group('ErrorCodes.requiresReLogin', () {
    test('should return true for auth codes', () {
      expect(ErrorCodes.requiresReLogin(ErrorCodes.unauthorized), true);
      expect(ErrorCodes.requiresReLogin(ErrorCodes.tokenExpired), true);
      expect(ErrorCodes.requiresReLogin(ErrorCodes.tokenInvalid), true);
      expect(ErrorCodes.requiresReLogin(ErrorCodes.loginRequired), true);
    });

    test('should return false for non-auth codes', () {
      expect(ErrorCodes.requiresReLogin(ErrorCodes.permissionDenied), false);
      expect(ErrorCodes.requiresReLogin(ErrorCodes.userNotFound), false);
      expect(ErrorCodes.requiresReLogin(ErrorCodes.contentNotFound), false);
      expect(ErrorCodes.requiresReLogin(ErrorCodes.networkError), false);
      expect(ErrorCodes.requiresReLogin(ErrorCodes.success), false);
    });

    test('should return false for null', () {
      expect(ErrorCodes.requiresReLogin(null), false);
    });

    test('should return false for unknown codes', () {
      expect(ErrorCodes.requiresReLogin(0), false);
      expect(ErrorCodes.requiresReLogin(999999), false);
      expect(ErrorCodes.requiresReLogin(-1), false);
    });
  });
}
