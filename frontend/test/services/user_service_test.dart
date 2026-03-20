import 'package:heart_lake/data/datasources/user_service.dart';
import 'package:flutter_test/flutter_test.dart';

/// UserService 继承 BaseService，无法直接实例化。
/// 提取核心响应处理逻辑进行测试。

class UserResponseProcessor {
  Map<String, dynamic> processSearchUsers(dynamic data, bool success) {
    if (!success) {
      return {'success': false, 'message': data?['message'] ?? '搜索失败'};
    }
    final users = UserPayloadNormalizer.extractUserList(data);
    final total =
        UserPayloadNormalizer.extractTotal(data, fallback: users.length);
    return {
      'success': true,
      'users': users,
      'total': total,
    };
  }

  Map<String, dynamic> processGetUserInfo(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取用户信息失败'};
    return {'success': true, 'user': UserPayloadNormalizer.normalizeUser(data)};
  }

  Map<String, dynamic> processGetUserStats(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取统计失败'};
    return {'success': true, 'data': data};
  }

  Map<String, dynamic> processEmotionHeatmap(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取热力图失败'};
    return {'success': true, 'data': data};
  }

  Map<String, dynamic> processEmotionCalendar(
      dynamic data, bool success, int year, int month) {
    if (year < 2020 || year > 2100) {
      return {'success': false, 'message': '年份无效'};
    }
    if (month < 1 || month > 12) {
      return {'success': false, 'message': '月份无效'};
    }
    if (!success) {
      return {'success': false, 'message': '获取日历失败'};
    }
    return {'success': true, 'data': data};
  }

  Map<String, dynamic> processMyBoats(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取纸船失败'};
    return {'success': true, 'data': data};
  }

  Map<String, dynamic> processUploadFile(
      int? statusCode, Map<String, dynamic>? data) {
    final payload = UserPayloadNormalizer.normalizeUploadedFile(data?['data']);
    if (statusCode == 200 && data?['code'] == 0) {
      return {'success': true, 'data': payload ?? <String, dynamic>{}};
    }
    return {
      'success': false,
      'message': data?['message'] ?? '上传失败',
      'data': payload,
    };
  }
}

void main() {
  late UserResponseProcessor processor;

  setUp(() {
    processor = UserResponseProcessor();
  });

  group('processSearchUsers', () {
    test('should return users on success', () {
      final result = processor.processSearchUsers({
        'users': [
          {'user_id': 'u1', 'nickname': '小明'},
          {'user_id': 'u2', 'nickname': '小红'},
        ],
        'total': 2,
      }, true);

      expect(result['success'], true);
      expect((result['users'] as List).length, 2);
      expect(result['total'], 2);
    });

    test('should return empty list when no users', () {
      final result =
          processor.processSearchUsers({'users': [], 'total': 0}, true);
      expect(result['success'], true);
      expect(result['users'], isEmpty);
      expect(result['total'], 0);
    });

    test('should handle null data on success', () {
      final result = processor.processSearchUsers(null, true);
      expect(result['success'], true);
      expect(result['users'], isEmpty);
      expect(result['total'], 0);
    });

    test('should handle missing users key', () {
      final result = processor.processSearchUsers({'total': 5}, true);
      expect(result['success'], true);
      expect(result['users'], isEmpty);
    });

    test('should handle missing total key', () {
      final result = processor.processSearchUsers({
        'users': [
          {'user_id': 'u1'}
        ]
      }, true);
      expect(result['total'], 1);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processSearchUsers({'message': '搜索超时'}, false);
      expect(result['success'], false);
      expect(result['message'], '搜索超时');
    });

    test('should use default message on failure without message', () {
      final result = processor.processSearchUsers({}, false);
      expect(result['success'], false);
      expect(result['message'], '搜索失败');
    });

    test('should handle large user list', () {
      final users =
          List.generate(100, (i) => {'user_id': 'u$i', 'nickname': '用户$i'});
      final result =
          processor.processSearchUsers({'users': users, 'total': 100}, true);
      expect(result['success'], true);
      expect((result['users'] as List).length, 100);
      expect(result['total'], 100);
    });

    test('should preserve user data structure', () {
      final result = processor.processSearchUsers({
        'users': [
          {
            'user_id': 'u1',
            'nickname': '小明',
            'avatar_url': 'http://img.com/1.png',
            'bio': '你好'
          },
        ],
        'total': 1,
      }, true);

      final user = (result['users'] as List).first;
      expect(user['user_id'], 'u1');
      expect(user['nickname'], '小明');
      expect(user['avatar_url'], 'http://img.com/1.png');
      expect(user['bio'], '你好');
    });

    test('should handle null message on failure', () {
      final result = processor.processSearchUsers(null, false);
      expect(result['success'], false);
      expect(result['message'], '搜索失败');
    });
  });

  group('processGetUserInfo', () {
    test('should return user data on success', () {
      final userData = {'user_id': 'u1', 'nickname': '小明', 'bio': '热爱生活'};
      final result = processor.processGetUserInfo(userData, true);
      expect(result['success'], true);
      expect(result['user']['user_id'], 'u1');
      expect(result['user']['userId'], 'u1');
      expect(result['user']['nickname'], '小明');
      expect(result['user']['bio'], '热爱生活');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetUserInfo(null, false);
      expect(result['success'], false);
    });

    test('should handle null data on success', () {
      final result = processor.processGetUserInfo(null, true);
      expect(result['success'], true);
      expect(result['user'], isNull);
    });

    test('should handle complex user data', () {
      final userData = {
        'userId': 'u1',
        'name': '小明',
        'vip_level': 2,
        'stats': {'stones': 10, 'friends': 5},
      };
      final result = processor.processGetUserInfo(userData, true);
      expect(result['user']['user_id'], 'u1');
      expect(result['user']['userId'], 'u1');
      expect(result['user']['nickname'], '小明');
      expect(result['user']['stats']['stones'], 10);
    });

    test('should handle empty map data', () {
      final result = processor.processGetUserInfo({}, true);
      expect(result['success'], true);
      expect(result['user'], isA<Map>());
    });
  });

  group('processGetUserStats', () {
    test('should return stats on success', () {
      final stats = {'stone_count': 10, 'ripple_count': 50, 'friend_count': 3};
      final result = processor.processGetUserStats(stats, true);
      expect(result['success'], true);
      expect(result['data'], stats);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetUserStats(null, false);
      expect(result['success'], false);
      expect(result['message'], '获取统计失败');
    });

    test('should handle null stats data', () {
      final result = processor.processGetUserStats(null, true);
      expect(result['success'], true);
      expect(result['data'], isNull);
    });

    test('should handle empty stats', () {
      final result = processor.processGetUserStats({}, true);
      expect(result['success'], true);
    });

    test('should preserve numeric values', () {
      final stats = {'stone_count': 999, 'avg_score': 0.85};
      final result = processor.processGetUserStats(stats, true);
      expect(result['data']['stone_count'], 999);
      expect(result['data']['avg_score'], 0.85);
    });
  });

  group('processEmotionHeatmap', () {
    test('should return heatmap data on success', () {
      final heatmap = {
        'data': [
          {'date': '2026-01-01', 'score': 0.8},
          {'date': '2026-01-02', 'score': 0.3},
        ]
      };
      final result = processor.processEmotionHeatmap(heatmap, true);
      expect(result['success'], true);
      expect(result['data'], heatmap);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processEmotionHeatmap(null, false);
      expect(result['success'], false);
    });

    test('should handle null data', () {
      final result = processor.processEmotionHeatmap(null, true);
      expect(result['success'], true);
      expect(result['data'], isNull);
    });

    test('should handle empty heatmap', () {
      final result = processor.processEmotionHeatmap({'data': []}, true);
      expect(result['success'], true);
    });
  });

  group('processEmotionCalendar', () {
    test('should return calendar data for valid date', () {
      final calendar = {
        'days': [
          {'day': 1, 'mood': 'happy'}
        ]
      };
      final result = processor.processEmotionCalendar(calendar, true, 2026, 2);
      expect(result['success'], true);
      expect(result['data'], calendar);
    });

    test('should fail for invalid year too low', () {
      final result = processor.processEmotionCalendar({}, true, 2019, 1);
      expect(result['success'], false);
      expect(result['message'], '年份无效');
    });

    test('should fail for invalid year too high', () {
      final result = processor.processEmotionCalendar({}, true, 2101, 1);
      expect(result['success'], false);
    });

    test('should fail for invalid month 0', () {
      final result = processor.processEmotionCalendar({}, true, 2026, 0);
      expect(result['success'], false);
      expect(result['message'], '月份无效');
    });

    test('should fail for invalid month 13', () {
      final result = processor.processEmotionCalendar({}, true, 2026, 13);
      expect(result['success'], false);
    });

    test('should accept month 1', () {
      final result = processor.processEmotionCalendar({}, true, 2026, 1);
      expect(result['success'], true);
    });

    test('should accept month 12', () {
      final result = processor.processEmotionCalendar({}, true, 2026, 12);
      expect(result['success'], true);
    });

    test('should accept year 2020', () {
      final result = processor.processEmotionCalendar({}, true, 2020, 6);
      expect(result['success'], true);
    });

    test('should accept year 2100', () {
      final result = processor.processEmotionCalendar({}, true, 2100, 6);
      expect(result['success'], true);
    });

    test('should fail on unsuccessful response with valid date', () {
      final result = processor.processEmotionCalendar({}, false, 2026, 2);
      expect(result['success'], false);
      expect(result['message'], '获取日历失败');
    });
  });

  group('processMyBoats', () {
    test('should return boats on success', () {
      final boats = {
        'boats': [
          {'boat_id': 'b1', 'content': '加油'}
        ]
      };
      final result = processor.processMyBoats(boats, true);
      expect(result['success'], true);
      expect(result['data'], boats);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processMyBoats(null, false);
      expect(result['success'], false);
      expect(result['message'], '获取纸船失败');
    });

    test('should handle empty boats', () {
      final result = processor.processMyBoats({'boats': []}, true);
      expect(result['success'], true);
    });

    test('should handle null data on success', () {
      final result = processor.processMyBoats(null, true);
      expect(result['success'], true);
      expect(result['data'], isNull);
    });
  });

  group('processUploadFile', () {
    test('should succeed with status 200 and code 0', () {
      final result = processor.processUploadFile(200, {
        'code': 0,
        'data': {'url': 'http://img.com/avatar.png'},
      });
      expect(result['success'], true);
      expect(result['data']['url'], 'http://img.com/avatar.png');
    });

    test('should fail with non-200 status', () {
      final result = processor.processUploadFile(500, {'code': 0, 'data': {}});
      expect(result['success'], false);
    });

    test('should fail with non-0 code', () {
      final result =
          processor.processUploadFile(200, {'code': 1, 'message': '文件太大'});
      expect(result['success'], false);
      expect(result['message'], '文件太大');
    });

    test('should use default message when no message provided', () {
      final result = processor.processUploadFile(200, {'code': 1});
      expect(result['success'], false);
      expect(result['message'], '上传失败');
    });

    test('should handle null data', () {
      final result = processor.processUploadFile(200, null);
      expect(result['success'], false);
    });

    test('should handle null status code', () {
      final result = processor.processUploadFile(null, {'code': 0, 'data': {}});
      expect(result['success'], false);
    });

    test('should handle 404 status', () {
      final result = processor.processUploadFile(404, {'message': '接口不存在'});
      expect(result['success'], false);
    });

    test('should handle 413 payload too large', () {
      final result = processor.processUploadFile(413, {'message': '文件超过限制'});
      expect(result['success'], false);
      expect(result['message'], '文件超过限制');
    });

    test('should normalize file_url to url', () {
      final result = processor.processUploadFile(200, {
        'code': 0,
        'data': {'file_url': 'http://img.com/avatar.png'},
      });
      expect(result['success'], true);
      expect(result['data']['url'], 'http://img.com/avatar.png');
      expect(result['data']['fileUrl'], 'http://img.com/avatar.png');
    });
  });

  // ==================== ServiceResponse.fromResponse 模拟 ====================
  group('ServiceResponse fromResponse pattern', () {
    Map<String, dynamic> simulateFromResponse(
        Map<String, dynamic> responseData) {
      final code = responseData['code'] as int?;
      final success = code == 0;
      return {
        'success': success,
        'data': success ? responseData['data'] : null,
        'message': responseData['message'],
        'code': code,
      };
    }

    test('should parse success response (code=0)', () {
      final result = simulateFromResponse({
        'code': 0,
        'data': {'user_id': 'u1'},
        'message': '操作成功',
      });
      expect(result['success'], true);
      expect(result['data']['user_id'], 'u1');
    });

    test('should parse error response (code!=0)', () {
      final result = simulateFromResponse({
        'code': 400001,
        'message': '内容不存在',
      });
      expect(result['success'], false);
      expect(result['data'], isNull);
      expect(result['code'], 400001);
    });

    test('should handle null code', () {
      final result = simulateFromResponse({'message': 'test'});
      expect(result['success'], false);
    });

    test('should handle code 200 as failure (only 0 is success)', () {
      final result = simulateFromResponse({'code': 200, 'data': {}});
      expect(result['success'], false);
    });

    test('should handle negative code', () {
      final result = simulateFromResponse({'code': -1, 'message': '未知错误'});
      expect(result['success'], false);
    });
  });

  // ==================== toMap 转换逻辑 ====================
  group('toMap conversion pattern', () {
    Map<String, dynamic> toMap(
        bool success, int? code, String? message, dynamic data) {
      return {
        'success': success,
        'code': code,
        'message': message,
        'data': data,
      };
    }

    test('should convert success response', () {
      final map = toMap(true, 200, '操作成功', {'id': '123'});
      expect(map['success'], true);
      expect(map['code'], 200);
      expect(map['message'], '操作成功');
      expect(map['data']['id'], '123');
    });

    test('should convert error response', () {
      final map = toMap(false, 500, '服务器错误', null);
      expect(map['success'], false);
      expect(map['code'], 500);
      expect(map['data'], isNull);
    });

    test('should handle all null optional fields', () {
      final map = toMap(false, null, null, null);
      expect(map['code'], isNull);
      expect(map['message'], isNull);
      expect(map['data'], isNull);
    });

    test('should preserve complex data', () {
      final data = {
        'users': [
          {'id': 'u1'},
          {'id': 'u2'}
        ],
        'total': 2,
      };
      final map = toMap(true, 0, 'ok', data);
      expect((map['data']['users'] as List).length, 2);
    });
  });
}
