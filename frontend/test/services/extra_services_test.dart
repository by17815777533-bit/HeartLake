import 'package:flutter_test/flutter_test.dart';

/// TempFriendService / ReportService / GuardianService / VIPService / AccountService
/// 等服务的响应处理逻辑测试。

class TempFriendResponseProcessor {
  Map<String, dynamic> processGetMyTempFriends(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取临时好友失败'};
    return {
      'success': true,
      'temp_friends': data is Map ? (data['friends'] ?? []) : [],
      'total': data is Map ? (data['total'] ?? 0) : 0,
    };
  }

  Map<String, dynamic> processGetDetail(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取详情失败'};
    return {'success': true, 'temp_friend': data};
  }

  Map<String, dynamic> processUpgrade(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '升级失败'};
    return {
      'success': true,
      'friendship_id': data is Map ? data['friendship_id'] : null,
    };
  }

  Map<String, dynamic> processDelete(bool success) {
    return {'success': success};
  }

  Map<String, dynamic> processCreate(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '创建失败'};
    return {'success': true, 'data': data};
  }

  Map<String, dynamic> processCheckStatus(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '检查失败'};
    return {'success': true, 'data': data};
  }
}

class ReportResponseProcessor {
  Map<String, dynamic> processCreateReport({
    required dynamic data,
    required bool success,
    required String targetType,
    required String targetId,
    required String reason,
    String? description,
  }) {
    if (targetType.isEmpty) return {'success': false, 'message': '举报类型不能为空'};
    if (targetId.isEmpty) return {'success': false, 'message': '举报目标不能为空'};
    if (reason.isEmpty) return {'success': false, 'message': '举报原因不能为空'};
    if (!success) return {'success': false, 'message': '举报失败'};
    return {
      'success': true,
      'report_id': data is Map ? data['report_id'] : null,
    };
  }

  Map<String, dynamic> processGetMyReports(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取举报列表失败'};
    return {
      'success': true,
      'reports': data is Map ? data['reports'] : null,
      'total': data is Map ? data['total'] : null,
    };
  }
}

class GuardianResponseProcessor {
  Map<String, dynamic> processGetStats(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取统计失败'};
    return {'success': true, 'data': data};
  }

  Map<String, dynamic> processTransferLamp(dynamic data, bool success, String toUserId) {
    if (toUserId.isEmpty) return {'success': false, 'message': '目标用户不能为空'};
    if (!success) return {'success': false, 'message': '转赠失败'};
    return {'success': true, 'data': data};
  }

  Map<String, dynamic> processChat(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '对话失败'};
    return {'success': true, 'data': data};
  }
}

class VIPResponseProcessor {
  Map<String, dynamic> processGetStatus(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取VIP状态失败'};
    return {'success': true, 'data': data};
  }

  bool processIsVIP(Map<String, dynamic> result) {
    if (result['success'] != true) return false;
    final data = result['data'];
    return (data is Map) ? (data['is_vip'] ?? false) : false;
  }

  int processGetDaysLeft(Map<String, dynamic> result) {
    if (result['success'] != true) return 0;
    final data = result['data'];
    return (data is Map) ? (data['days_left'] ?? 0) : 0;
  }

  bool processHasFreeCounseling(dynamic data, bool success) {
    if (!success) return false;
    return data is Map ? (data['has_quota'] ?? false) : false;
  }

  double processGetAICommentFrequency(dynamic data, bool success) {
    if (!success) return 2.0;
    return data is Map ? ((data['frequency_hours'] ?? 2.0) as num).toDouble() : 2.0;
  }
}

class AccountResponseProcessor {
  Map<String, dynamic> processSimple(dynamic data, bool success, String errorMsg) {
    if (!success) return {'success': false, 'message': errorMsg};
    return {'success': true, 'data': data};
  }
}

void main() {
  // ==================== TempFriendService ====================
  group('TempFriendResponseProcessor', () {
    late TempFriendResponseProcessor processor;
    setUp(() => processor = TempFriendResponseProcessor());

    group('processGetMyTempFriends', () {
      test('should return temp friends on success', () {
        final result = processor.processGetMyTempFriends({
          'friends': [{'user_id': 'u1'}, {'user_id': 'u2'}],
          'total': 2,
        }, true);
        expect(result['success'], true);
        expect((result['temp_friends'] as List).length, 2);
        expect(result['total'], 2);
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processGetMyTempFriends(null, false);
        expect(result['success'], false);
      });

      test('should handle null data', () {
        final result = processor.processGetMyTempFriends(null, true);
        expect(result['temp_friends'], isEmpty);
        expect(result['total'], 0);
      });

      test('should handle empty friends', () {
        final result = processor.processGetMyTempFriends({'friends': [], 'total': 0}, true);
        expect(result['temp_friends'], isEmpty);
      });

      test('should handle missing friends key', () {
        final result = processor.processGetMyTempFriends({'total': 3}, true);
        expect(result['temp_friends'], isEmpty);
      });

      test('should handle missing total key', () {
        final result = processor.processGetMyTempFriends({'friends': [{'id': '1'}]}, true);
        expect(result['total'], 0);
      });
    });

    group('processGetDetail', () {
      test('should return detail on success', () {
        final result = processor.processGetDetail({'user_id': 'u1', 'expires_at': '2026-03-01'}, true);
        expect(result['success'], true);
        expect(result['temp_friend']['user_id'], 'u1');
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processGetDetail(null, false);
        expect(result['success'], false);
      });
    });

    group('processUpgrade', () {
      test('should return friendship_id on success', () {
        final result = processor.processUpgrade({'friendship_id': 'f1'}, true);
        expect(result['success'], true);
        expect(result['friendship_id'], 'f1');
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processUpgrade(null, false);
        expect(result['success'], false);
      });

      test('should handle null data on success', () {
        final result = processor.processUpgrade(null, true);
        expect(result['friendship_id'], isNull);
      });
    });

    group('processDelete', () {
      test('should return success true', () {
        expect(processor.processDelete(true)['success'], true);
      });
      test('should return success false', () {
        expect(processor.processDelete(false)['success'], false);
      });
    });

    group('processCreate', () {
      test('should return success on valid response', () {
        final result = processor.processCreate({'temp_friend_id': 'tf1'}, true);
        expect(result['success'], true);
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processCreate(null, false);
        expect(result['success'], false);
      });
    });

    group('processCheckStatus', () {
      test('should return status on success', () {
        final result = processor.processCheckStatus({'is_temp_friend': true, 'expires_at': '2026-03-01'}, true);
        expect(result['success'], true);
        expect(result['data']['is_temp_friend'], true);
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processCheckStatus(null, false);
        expect(result['success'], false);
      });
    });
  });

  // ==================== ReportService ====================
  group('ReportResponseProcessor', () {
    late ReportResponseProcessor processor;
    setUp(() => processor = ReportResponseProcessor());

    group('processCreateReport', () {
      test('should return success with report_id', () {
        final result = processor.processCreateReport(
          data: {'report_id': 'rpt_1'},
          success: true,
          targetType: 'stone',
          targetId: 's1',
          reason: 'spam',
        );
        expect(result['success'], true);
        expect(result['report_id'], 'rpt_1');
      });

      test('should fail on empty targetType', () {
        final result = processor.processCreateReport(
          data: null, success: true, targetType: '', targetId: 's1', reason: 'spam',
        );
        expect(result['success'], false);
        expect(result['message'], '举报类型不能为空');
      });

      test('should fail on empty targetId', () {
        final result = processor.processCreateReport(
          data: null, success: true, targetType: 'stone', targetId: '', reason: 'spam',
        );
        expect(result['success'], false);
        expect(result['message'], '举报目标不能为空');
      });

      test('should fail on empty reason', () {
        final result = processor.processCreateReport(
          data: null, success: true, targetType: 'stone', targetId: 's1', reason: '',
        );
        expect(result['success'], false);
        expect(result['message'], '举报原因不能为空');
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processCreateReport(
          data: null, success: false, targetType: 'stone', targetId: 's1', reason: 'spam',
        );
        expect(result['success'], false);
      });

      test('should handle description parameter', () {
        final result = processor.processCreateReport(
          data: {'report_id': 'rpt_2'},
          success: true,
          targetType: 'stone',
          targetId: 's1',
          reason: 'spam',
          description: '垃圾内容',
        );
        expect(result['success'], true);
      });

      test('should handle null data on success', () {
        final result = processor.processCreateReport(
          data: null, success: true, targetType: 'stone', targetId: 's1', reason: 'spam',
        );
        expect(result['report_id'], isNull);
      });

      test('should handle non-map data', () {
        final result = processor.processCreateReport(
          data: 'string', success: true, targetType: 'stone', targetId: 's1', reason: 'spam',
        );
        expect(result['report_id'], isNull);
      });
    });

    group('processGetMyReports', () {
      test('should return reports on success', () {
        final result = processor.processGetMyReports({
          'reports': [{'report_id': 'r1'}, {'report_id': 'r2'}],
          'total': 2,
        }, true);
        expect(result['success'], true);
        expect((result['reports'] as List).length, 2);
        expect(result['total'], 2);
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processGetMyReports(null, false);
        expect(result['success'], false);
      });

      test('should handle null data', () {
        final result = processor.processGetMyReports(null, true);
        expect(result['reports'], isNull);
      });

      test('should handle empty reports', () {
        final result = processor.processGetMyReports({'reports': [], 'total': 0}, true);
        expect(result['reports'], isEmpty);
      });
    });
  });

  // ==================== GuardianService ====================
  group('GuardianResponseProcessor', () {
    late GuardianResponseProcessor processor;
    setUp(() => processor = GuardianResponseProcessor());

    group('processGetStats', () {
      test('should return stats on success', () {
        final result = processor.processGetStats({'lamps': 5, 'transfers': 3}, true);
        expect(result['success'], true);
        expect(result['data']['lamps'], 5);
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processGetStats(null, false);
        expect(result['success'], false);
      });
    });

    group('processTransferLamp', () {
      test('should return success on valid transfer', () {
        final result = processor.processTransferLamp({'lamp_id': 'l1'}, true, 'u1');
        expect(result['success'], true);
      });

      test('should fail on empty toUserId', () {
        final result = processor.processTransferLamp(null, true, '');
        expect(result['success'], false);
        expect(result['message'], '目标用户不能为空');
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processTransferLamp(null, false, 'u1');
        expect(result['success'], false);
      });
    });

    group('processChat', () {
      test('should return chat response on success', () {
        final result = processor.processChat({'reply': '你好'}, true);
        expect(result['success'], true);
        expect(result['data']['reply'], '你好');
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processChat(null, false);
        expect(result['success'], false);
      });
    });
  });

  // ==================== VIPService ====================
  group('VIPResponseProcessor', () {
    late VIPResponseProcessor processor;
    setUp(() => processor = VIPResponseProcessor());

    group('processGetStatus', () {
      test('should return VIP status on success', () {
        final result = processor.processGetStatus({'is_vip': true, 'days_left': 30}, true);
        expect(result['success'], true);
        expect(result['data']['is_vip'], true);
      });

      test('should fail on unsuccessful response', () {
        final result = processor.processGetStatus(null, false);
        expect(result['success'], false);
      });
    });

    group('processIsVIP', () {
      test('should return true for VIP user', () {
        expect(processor.processIsVIP({'success': true, 'data': {'is_vip': true}}), true);
      });

      test('should return false for non-VIP user', () {
        expect(processor.processIsVIP({'success': true, 'data': {'is_vip': false}}), false);
      });

      test('should return false on failure', () {
        expect(processor.processIsVIP({'success': false}), false);
      });

      test('should return false when data is null', () {
        expect(processor.processIsVIP({'success': true, 'data': null}), false);
      });

      test('should return false when is_vip missing', () {
        expect(processor.processIsVIP({'success': true, 'data': {}}), false);
      });

      test('should return false when data is not map', () {
        expect(processor.processIsVIP({'success': true, 'data': 'string'}), false);
      });
    });

    group('processGetDaysLeft', () {
      test('should return days left', () {
        expect(processor.processGetDaysLeft({'success': true, 'data': {'days_left': 30}}), 30);
      });

      test('should return 0 on failure', () {
        expect(processor.processGetDaysLeft({'success': false}), 0);
      });

      test('should return 0 when missing', () {
        expect(processor.processGetDaysLeft({'success': true, 'data': {}}), 0);
      });

      test('should return 0 when data is not map', () {
        expect(processor.processGetDaysLeft({'success': true, 'data': null}), 0);
      });
    });

    group('processHasFreeCounseling', () {
      test('should return true when has quota', () {
        expect(processor.processHasFreeCounseling({'has_quota': true}, true), true);
      });

      test('should return false when no quota', () {
        expect(processor.processHasFreeCounseling({'has_quota': false}, true), false);
      });

      test('should return false on failure', () {
        expect(processor.processHasFreeCounseling(null, false), false);
      });

      test('should return false when missing', () {
        expect(processor.processHasFreeCounseling({}, true), false);
      });

      test('should return false when data is not map', () {
        expect(processor.processHasFreeCounseling('string', true), false);
      });
    });

    group('processGetAICommentFrequency', () {
      test('should return frequency hours', () {
        expect(processor.processGetAICommentFrequency({'frequency_hours': 1.5}, true), 1.5);
      });

      test('should return default 2.0 on failure', () {
        expect(processor.processGetAICommentFrequency(null, false), 2.0);
      });

      test('should return default 2.0 when missing', () {
        expect(processor.processGetAICommentFrequency({}, true), 2.0);
      });

      test('should handle int frequency', () {
        expect(processor.processGetAICommentFrequency({'frequency_hours': 3}, true), 3.0);
      });

      test('should return default when data is not map', () {
        expect(processor.processGetAICommentFrequency('string', true), 2.0);
      });
    });
  });

  // ==================== AccountService ====================
  group('AccountResponseProcessor', () {
    late AccountResponseProcessor processor;
    setUp(() => processor = AccountResponseProcessor());

    test('should return success with data', () {
      final result = processor.processSimple({'info': 'test'}, true, '获取失败');
      expect(result['success'], true);
      expect(result['data']['info'], 'test');
    });

    test('should fail with error message', () {
      final result = processor.processSimple(null, false, '获取账号信息失败');
      expect(result['success'], false);
      expect(result['message'], '获取账号信息失败');
    });

    test('should handle null data on success', () {
      final result = processor.processSimple(null, true, '');
      expect(result['success'], true);
      expect(result['data'], isNull);
    });

    test('getAccountInfo scenario', () {
      final result = processor.processSimple({
        'user_id': 'u1', 'nickname': '小明', 'email': '[email]',
      }, true, '获取账号信息失败');
      expect(result['success'], true);
    });

    test('getAccountStats scenario', () {
      final result = processor.processSimple({
        'stones_count': 10, 'friends_count': 5, 'ripples_count': 20,
      }, true, '获取统计失败');
      expect(result['data']['stones_count'], 10);
    });

    test('getDevices scenario', () {
      final result = processor.processSimple({
        'devices': [{'device_id': 'd1', 'platform': 'android'}],
      }, true, '获取设备失败');
      expect((result['data']['devices'] as List).length, 1);
    });

    test('getLoginLogs scenario', () {
      final result = processor.processSimple({
        'logs': [{'time': '2026-01-01', 'ip': '1.2.3.4'}],
      }, true, '获取日志失败');
      expect(result['success'], true);
    });

    test('getPrivacySettings scenario', () {
      final result = processor.processSimple({
        'show_online': true, 'allow_search': false,
      }, true, '获取隐私设置失败');
      expect(result['data']['show_online'], true);
    });

    test('updatePrivacySettings scenario', () {
      final result = processor.processSimple({'updated': true}, true, '更新失败');
      expect(result['success'], true);
    });

    test('getBlockedUsers scenario', () {
      final result = processor.processSimple({
        'users': [{'user_id': 'u1'}], 'total': 1,
      }, true, '获取黑名单失败');
      expect(result['success'], true);
    });

    test('blockUser scenario', () {
      final result = processor.processSimple({'blocked': true}, true, '拉黑失败');
      expect(result['success'], true);
    });

    test('unblockUser scenario', () {
      final result = processor.processSimple({'unblocked': true}, true, '取消拉黑失败');
      expect(result['success'], true);
    });

    test('exportData scenario', () {
      final result = processor.processSimple({'task_id': 'task_1'}, true, '导出失败');
      expect(result['data']['task_id'], 'task_1');
    });

    test('removeDevice scenario', () {
      final result = processor.processSimple(null, true, '移除设备失败');
      expect(result['success'], true);
    });

    test('getSecurityEvents scenario', () {
      final result = processor.processSimple({
        'events': [{'type': 'login', 'time': '2026-01-01'}],
      }, true, '获取安全事件失败');
      expect(result['success'], true);
    });

    test('getExportStatus scenario', () {
      final result = processor.processSimple({
        'status': 'completed', 'download_url': 'http://example.com/data.zip',
      }, true, '获取导出状态失败');
      expect(result['data']['status'], 'completed');
    });

    test('deactivateAccount scenario', () {
      final result = processor.processSimple({'deactivated': true}, true, '停用失败');
      expect(result['success'], true);
    });

    test('deleteAccountPermanently scenario', () {
      final result = processor.processSimple({'deleted': true}, true, '删除失败');
      expect(result['success'], true);
    });

    test('uploadAvatar scenario', () {
      final result = processor.processSimple({'avatar_url': 'http://img.com/new.png'}, true, '上传失败');
      expect(result['data']['avatar_url'], 'http://img.com/new.png');
    });

    test('updateProfile scenario', () {
      final result = processor.processSimple({
        'nickname': '新昵称', 'bio': '新简介',
      }, true, '更新资料失败');
      expect(result['data']['nickname'], '新昵称');
    });
  });
}
