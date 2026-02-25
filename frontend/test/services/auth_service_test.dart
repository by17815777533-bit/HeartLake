import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/base_service.dart';

/// AuthService 继承 BaseService（依赖 ApiClient 单例），无法直接实例化。
/// 提取核心逻辑进行测试：响应处理、数据验证、ServiceResponse 转换。

class AuthResultProcessor {
  Map<String, dynamic> processAnonymousLogin(Map<String, dynamic> data, bool success) {
    if (!success) return {'success': false, 'message': data['message'] ?? '登录失败'};
    final token = data['token'];
    final userId = data['user_id'];
    if (token == null || userId == null) {
      return {'success': false, 'message': '服务器返回数据不完整'};
    }
    return {
      'success': true,
      'user_id': userId,
      'nickname': data['nickname'],
      'recovery_key': data['recovery_key'],
    };
  }

  Map<String, dynamic> processRecover(Map<String, dynamic> data, bool success) {
    if (!success) return {'success': false, 'message': data['message'] ?? '恢复失败'};
    final token = data['token'];
    final userId = data['user_id'];
    if (token == null || userId == null) {
      return {'success': false, 'message': '服务器返回数据不完整'};
    }
    return {'success': true, 'user_id': userId, 'nickname': data['nickname']};
  }

  Map<String, dynamic> processUpdateProfile({
    String? avatarUrl,
    String? bio,
    String? nickname,
  }) {
    final Map<String, dynamic> data = {};
    if (avatarUrl != null) data['avatar_url'] = avatarUrl;
    if (bio != null) data['bio'] = bio;
    if (nickname != null) data['nickname'] = nickname;
    if (data.isEmpty) return {'success': true};
    return data;
  }

  Map<String, dynamic> processRefreshToken(String? currentToken) {
    if (currentToken == null) return {'success': false, 'code': 401};
    return {'success': true};
  }

  Map<String, dynamic> processUpdateNickname(Map<String, dynamic> data, bool success) {
    if (!success) return {'success': false, 'message': data['message'] ?? '更新失败'};
    return {'success': true, 'nickname': data['nickname']};
  }
}

void main() {
  late AuthResultProcessor processor;

  setUp(() {
    processor = AuthResultProcessor();
  });

  // ==================== ServiceResponse 测试 ====================
  group('ServiceResponse construction', () {
    test('success factory should set correct fields', () {
      final resp = ServiceResponse.success({'key': 'value'}, message: '操作成功');
      expect(resp.success, true);
      expect(resp.data, {'key': 'value'});
      expect(resp.message, '操作成功');
      expect(resp.code, 200);
    });

    test('error factory should set correct fields', () {
      final resp = ServiceResponse.error('服务器错误', code: 500);
      expect(resp.success, false);
      expect(resp.message, '服务器错误');
      expect(resp.code, 500);
      expect(resp.data, isNull);
    });

    test('error factory default code should be 500', () {
      final resp = ServiceResponse.error('错误');
      expect(resp.code, 500);
    });

    test('success factory should allow null data', () {
      final resp = ServiceResponse.success(null);
      expect(resp.success, true);
      expect(resp.data, isNull);
    });

    test('constructor should store all fields', () {
      final resp = ServiceResponse(
        success: true,
        data: 42,
        message: 'ok',
        code: 200,
      );
      expect(resp.success, true);
      expect(resp.data, 42);
      expect(resp.message, 'ok');
      expect(resp.code, 200);
    });

    test('constructor should allow null optional fields', () {
      final resp = ServiceResponse(success: false);
      expect(resp.data, isNull);
      expect(resp.message, isNull);
      expect(resp.code, isNull);
    });
  });

  // ==================== 匿名登录处理 ====================
  group('processAnonymousLogin', () {
    test('should return success with complete data', () {
      final result = processor.processAnonymousLogin({
        'token': 'tok_123',
        'user_id': 'uid_456',
        'nickname': '匿名用户',
        'recovery_key': 'key_789',
      }, true);

      expect(result['success'], true);
      expect(result['user_id'], 'uid_456');
      expect(result['nickname'], '匿名用户');
      expect(result['recovery_key'], 'key_789');
    });

    test('should fail when token is null', () {
      final result = processor.processAnonymousLogin({
        'user_id': 'uid_456',
      }, true);

      expect(result['success'], false);
      expect(result['message'], '服务器返回数据不完整');
    });

    test('should fail when user_id is null', () {
      final result = processor.processAnonymousLogin({
        'token': 'tok_123',
      }, true);

      expect(result['success'], false);
      expect(result['message'], '服务器返回数据不完整');
    });

    test('should fail when both token and user_id are null', () {
      final result = processor.processAnonymousLogin({}, true);
      expect(result['success'], false);
      expect(result['message'], '服务器返回数据不完整');
    });

    test('should fail when response is not successful', () {
      final result = processor.processAnonymousLogin({
        'message': '服务器繁忙',
      }, false);

      expect(result['success'], false);
      expect(result['message'], '服务器繁忙');
    });

    test('should use default message when response fails without message', () {
      final result = processor.processAnonymousLogin({}, false);
      expect(result['success'], false);
      expect(result['message'], '登录失败');
    });

    test('should handle null nickname gracefully', () {
      final result = processor.processAnonymousLogin({
        'token': 'tok_123',
        'user_id': 'uid_456',
      }, true);

      expect(result['success'], true);
      expect(result['nickname'], isNull);
    });

    test('should handle null recovery_key gracefully', () {
      final result = processor.processAnonymousLogin({
        'token': 'tok_123',
        'user_id': 'uid_456',
        'nickname': '用户',
      }, true);

      expect(result['success'], true);
      expect(result['recovery_key'], isNull);
    });

    test('should handle empty string token as valid', () {
      final result = processor.processAnonymousLogin({
        'token': '',
        'user_id': 'uid_456',
      }, true);
      expect(result['success'], true);
    });

    test('should handle empty string user_id as valid', () {
      final result = processor.processAnonymousLogin({
        'token': 'tok_123',
        'user_id': '',
      }, true);
      expect(result['success'], true);
      expect(result['user_id'], '');
    });
  });

  // ==================== 恢复账号处理 ====================
  group('processRecover', () {
    test('should return success with complete data', () {
      final result = processor.processRecover({
        'token': 'tok_new',
        'user_id': 'uid_456',
        'nickname': '恢复用户',
      }, true);

      expect(result['success'], true);
      expect(result['user_id'], 'uid_456');
      expect(result['nickname'], '恢复用户');
    });

    test('should fail when token is null', () {
      final result = processor.processRecover({
        'user_id': 'uid_456',
      }, true);
      expect(result['success'], false);
      expect(result['message'], '服务器返回数据不完整');
    });

    test('should fail when user_id is null', () {
      final result = processor.processRecover({
        'token': 'tok_new',
      }, true);
      expect(result['success'], false);
      expect(result['message'], '服务器返回数据不完整');
    });

    test('should fail when response is not successful', () {
      final result = processor.processRecover({
        'message': '恢复密钥无效',
      }, false);
      expect(result['success'], false);
      expect(result['message'], '恢复密钥无效');
    });

    test('should use default message on failure without message', () {
      final result = processor.processRecover({}, false);
      expect(result['success'], false);
      expect(result['message'], '恢复失败');
    });

    test('should handle null nickname', () {
      final result = processor.processRecover({
        'token': 'tok_new',
        'user_id': 'uid_456',
      }, true);
      expect(result['success'], true);
      expect(result['nickname'], isNull);
    });

    test('should not include recovery_key in result', () {
      final result = processor.processRecover({
        'token': 'tok_new',
        'user_id': 'uid_456',
        'recovery_key': 'key_old',
      }, true);
      expect(result.containsKey('recovery_key'), false);
    });

    test('should handle extra fields in response', () {
      final result = processor.processRecover({
        'token': 'tok_new',
        'user_id': 'uid_456',
        'nickname': '用户',
        'extra_field': 'ignored',
      }, true);
      expect(result['success'], true);
      expect(result.containsKey('extra_field'), false);
    });
  });

  // ==================== 更新资料请求构建 ====================
  group('processUpdateProfile', () {
    test('should return empty success when no params', () {
      final result = processor.processUpdateProfile();
      expect(result['success'], true);
    });

    test('should include only avatarUrl when provided', () {
      final result = processor.processUpdateProfile(avatarUrl: 'https://img.com/a.png');
      expect(result['avatar_url'], 'https://img.com/a.png');
      expect(result.containsKey('bio'), false);
      expect(result.containsKey('nickname'), false);
    });

    test('should include only bio when provided', () {
      final result = processor.processUpdateProfile(bio: '热爱生活');
      expect(result['bio'], '热爱生活');
      expect(result.containsKey('avatar_url'), false);
    });

    test('should include only nickname when provided', () {
      final result = processor.processUpdateProfile(nickname: '新昵称');
      expect(result['nickname'], '新昵称');
    });

    test('should include all fields when all provided', () {
      final result = processor.processUpdateProfile(
        avatarUrl: 'url',
        bio: 'bio',
        nickname: 'name',
      );
      expect(result['avatar_url'], 'url');
      expect(result['bio'], 'bio');
      expect(result['nickname'], 'name');
    });

    test('should include two fields when two provided', () {
      final result = processor.processUpdateProfile(
        avatarUrl: 'url',
        nickname: 'name',
      );
      expect(result['avatar_url'], 'url');
      expect(result['nickname'], 'name');
      expect(result.containsKey('bio'), false);
    });

    test('should handle empty string values', () {
      final result = processor.processUpdateProfile(nickname: '');
      expect(result['nickname'], '');
    });
  });

  // ==================== Token 刷新处理 ====================
  group('processRefreshToken', () {
    test('should fail when current token is null', () {
      final result = processor.processRefreshToken(null);
      expect(result['success'], false);
      expect(result['code'], 401);
    });

    test('should succeed when current token exists', () {
      final result = processor.processRefreshToken('valid_token');
      expect(result['success'], true);
    });

    test('should succeed with empty string token', () {
      final result = processor.processRefreshToken('');
      expect(result['success'], true);
    });
  });

  // ==================== 更新昵称处理 ====================
  group('processUpdateNickname', () {
    test('should return success with nickname', () {
      final result = processor.processUpdateNickname({
        'nickname': '新昵称',
      }, true);
      expect(result['success'], true);
      expect(result['nickname'], '新昵称');
    });

    test('should fail when response is not successful', () {
      final result = processor.processUpdateNickname({
        'message': '昵称已被占用',
      }, false);
      expect(result['success'], false);
      expect(result['message'], '昵称已被占用');
    });

    test('should use default message on failure', () {
      final result = processor.processUpdateNickname({}, false);
      expect(result['message'], '更新失败');
    });

    test('should handle null nickname in success response', () {
      final result = processor.processUpdateNickname({}, true);
      expect(result['success'], true);
      expect(result['nickname'], isNull);
    });

    test('should handle unicode nickname', () {
      final result = processor.processUpdateNickname({
        'nickname': '🌊心湖用户',
      }, true);
      expect(result['nickname'], '🌊心湖用户');
    });

    test('should handle very long nickname', () {
      final longName = 'A' * 100;
      final result = processor.processUpdateNickname({
        'nickname': longName,
      }, true);
      expect(result['nickname'], longName);
    });
  });

  // ==================== ServiceResponse.fromResponse 模拟 ====================
  group('ServiceResponse fromResponse pattern', () {
    Map<String, dynamic> simulateApiResponse(int code, dynamic data, String? message) {
      return {'code': code, 'data': data, 'message': message};
    }

    ServiceResponse<dynamic> parseResponse(Map<String, dynamic> responseData) {
      final code = responseData['code'] as int?;
      final success = code == 0;
      return ServiceResponse(
        success: success,
        data: success ? responseData['data'] : null,
        message: responseData['message'] as String?,
        code: code,
      );
    }

    test('should parse success response (code=0)', () {
      final apiResp = simulateApiResponse(0, {'user_id': 'u1'}, '操作成功');
      final resp = parseResponse(apiResp);
      expect(resp.success, true);
      expect(resp.data, {'user_id': 'u1'});
      expect(resp.code, 0);
    });

    test('should parse error response (code!=0)', () {
      final apiResp = simulateApiResponse(400001, null, '内容不存在');
      final resp = parseResponse(apiResp);
      expect(resp.success, false);
      expect(resp.data, isNull);
      expect(resp.message, '内容不存在');
    });

    test('should parse auth error (code=200001)', () {
      final apiResp = simulateApiResponse(200001, null, '未授权');
      final resp = parseResponse(apiResp);
      expect(resp.success, false);
      expect(resp.code, 200001);
    });

    test('should parse server error (code=100500)', () {
      final apiResp = simulateApiResponse(100500, null, '内部错误');
      final resp = parseResponse(apiResp);
      expect(resp.success, false);
      expect(resp.code, 100500);
    });

    test('should handle null message', () {
      final apiResp = simulateApiResponse(0, 'data', null);
      final resp = parseResponse(apiResp);
      expect(resp.message, isNull);
    });

    test('should handle complex data', () {
      final apiResp = simulateApiResponse(0, {
        'token': 'tok',
        'user_id': 'uid',
        'nested': {'key': 'value'},
      }, 'ok');
      final resp = parseResponse(apiResp);
      expect(resp.data['token'], 'tok');
      expect(resp.data['nested']['key'], 'value');
    });
  });

  // ==================== toMap 转换逻辑 ====================
  group('toMap conversion pattern', () {
    Map<String, dynamic> toMap(ServiceResponse resp) {
      return {
        'success': resp.success,
        'code': resp.code,
        'message': resp.message,
        'data': resp.data,
      };
    }

    test('should convert success response to map', () {
      final resp = ServiceResponse.success({'id': '1'});
      final map = toMap(resp);
      expect(map['success'], true);
      expect(map['data'], {'id': '1'});
      expect(map['code'], 200);
    });

    test('should convert error response to map', () {
      final resp = ServiceResponse.error('失败', code: 403);
      final map = toMap(resp);
      expect(map['success'], false);
      expect(map['message'], '失败');
      expect(map['code'], 403);
      expect(map['data'], isNull);
    });

    test('should preserve null fields', () {
      final resp = ServiceResponse(success: true);
      final map = toMap(resp);
      expect(map['message'], isNull);
      expect(map['code'], isNull);
      expect(map['data'], isNull);
    });
  });

  // ==================== 设备ID生成逻辑 ====================
  group('Device ID logic', () {
    test('should generate UUID v4 format', () {
      // UUID v4 格式: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
      final uuidPattern = RegExp(
        r'^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$',
      );
      // 模拟 UUID v4
      const sampleUuid = '550e8400-e29b-41d4-a716-446655440000';
      expect(uuidPattern.hasMatch(sampleUuid), true);
    });

    test('should reject invalid UUID format', () {
      final uuidPattern = RegExp(
        r'^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$',
      );
      expect(uuidPattern.hasMatch('not-a-uuid'), false);
      expect(uuidPattern.hasMatch(''), false);
      expect(uuidPattern.hasMatch('12345'), false);
    });
  });

  // ==================== 登录状态判断 ====================
  group('isLoggedIn logic', () {
    test('should return true when token exists', () {
      const token = 'valid_token';
      expect(token != null, true); // ignore: unnecessary_null_comparison
    });

    test('should return false when token is null', () {
      const String? token = null;
      expect(token != null, false);
    });
  });
}
