import 'package:flutter_test/flutter_test.dart';

/// NotificationService 继承 BaseService，无法直接实例化。
/// 提取核心响应处理逻辑进行测试。

class NotificationResponseProcessor {
  Map<String, dynamic> processGetNotifications(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取通知失败'};
    if (data is List) {
      final unread = data.where((n) => n is Map && n['is_read'] != true).length;
      return {
        'success': true,
        'notifications': data,
        'items': data,
        'list': data,
        'unread_count': unread,
      };
    }
    if (data is Map<String, dynamic>) {
      final rawNotifications = data['notifications'];
      final rawItems = data['items'];
      final rawList = data['list'];
      final nestedData = data['data'] is Map<String, dynamic>
          ? data['data'] as Map<String, dynamic>
          : null;
      final items = rawNotifications is List
          ? rawNotifications
          : (rawItems is List ? rawItems : (rawList is List ? rawList : []));
      final unreadCount = data['unread_count'] as int? ??
          data['unreadCount'] as int? ??
          nestedData?['unread_count'] as int? ??
          nestedData?['unreadCount'] as int? ??
          0;
      return {
        'success': true,
        'notifications': items,
        'items': items,
        'list': items,
        'unread_count': unreadCount,
      };
    }
    return {
      'success': true,
      'notifications': [],
      'items': [],
      'list': [],
      'unread_count': 0,
    };
  }

  Map<String, dynamic> processGetUnreadCount(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取未读数失败'};
    int unreadCount = 0;
    if (data is Map) {
      unreadCount = data['unread_count'] ?? 0;
    }
    return {'success': true, 'unread_count': unreadCount};
  }

  Map<String, dynamic> processMarkAsRead(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '标记失败'};
    final unreadCount = (data is Map) ? data['unread_count'] : null;
    return {'success': true, 'unread_count': unreadCount};
  }

  Map<String, dynamic> processMarkAllAsRead(bool success) {
    return {'success': success};
  }
}

void main() {
  late NotificationResponseProcessor processor;

  setUp(() {
    processor = NotificationResponseProcessor();
  });

  group('processGetNotifications - List response', () {
    test('should handle list with mixed read/unread', () {
      final data = [
        {'id': 'n1', 'content': '通知1', 'is_read': true},
        {'id': 'n2', 'content': '通知2', 'is_read': false},
        {'id': 'n3', 'content': '通知3', 'is_read': false},
      ];
      final result = processor.processGetNotifications(data, true);
      expect(result['success'], true);
      expect((result['notifications'] as List).length, 3);
      expect(result['unread_count'], 2);
    });

    test('should handle all read notifications', () {
      final data = [
        {'id': 'n1', 'is_read': true},
        {'id': 'n2', 'is_read': true},
      ];
      final result = processor.processGetNotifications(data, true);
      expect(result['unread_count'], 0);
    });

    test('should handle all unread notifications', () {
      final data = [
        {'id': 'n1', 'is_read': false},
        {'id': 'n2'},
      ];
      final result = processor.processGetNotifications(data, true);
      expect(result['unread_count'], 2);
    });

    test('should handle empty list', () {
      final result = processor.processGetNotifications([], true);
      expect(result['success'], true);
      expect(result['notifications'], isEmpty);
      expect(result['unread_count'], 0);
    });

    test('should treat missing is_read as unread', () {
      final data = [
        {'id': 'n1', 'content': '通知'}
      ];
      final result = processor.processGetNotifications(data, true);
      expect(result['unread_count'], 1);
    });

    test('should handle non-map items in list', () {
      final data = [
        {'id': 'n1', 'is_read': false},
        'invalid',
        42,
      ];
      final result = processor.processGetNotifications(data, true);
      expect(result['unread_count'], 1);
    });
  });

  group('processGetNotifications - Map response', () {
    test('should extract from notifications key', () {
      final data = {
        'notifications': [
          {'id': 'n1', 'content': '通知1'},
          {'id': 'n2', 'content': '通知2'},
        ],
        'unread_count': 1,
      };
      final result = processor.processGetNotifications(data, true);
      expect(result['success'], true);
      expect((result['notifications'] as List).length, 2);
      expect(result['unread_count'], 1);
    });

    test('should extract from items key when notifications missing', () {
      final data = {
        'items': [
          {'id': 'n1'}
        ],
        'unread_count': 1,
      };
      final result = processor.processGetNotifications(data, true);
      expect((result['notifications'] as List).length, 1);
    });

    test(
        'should extract from list key when notifications and items are missing',
        () {
      final data = {
        'list': [
          {'id': 'n9'}
        ],
        'unread_count': 1,
      };
      final result = processor.processGetNotifications(data, true);
      expect((result['notifications'] as List).length, 1);
      expect((result['items'] as List).length, 1);
    });

    test('should prefer notifications over items', () {
      final data = {
        'notifications': [
          {'id': 'n1'},
          {'id': 'n2'}
        ],
        'items': [
          {'id': 'n3'}
        ],
        'unread_count': 0,
      };
      final result = processor.processGetNotifications(data, true);
      expect((result['notifications'] as List).length, 2);
    });

    test('should default unread_count to 0', () {
      final data = {
        'notifications': [
          {'id': 'n1'}
        ]
      };
      final result = processor.processGetNotifications(data, true);
      expect(result['unread_count'], 0);
    });

    test('should read nested unread_count aliases', () {
      final data = {
        'items': [
          {'id': 'n1'}
        ],
        'data': {'unreadCount': 4},
      };
      final result = processor.processGetNotifications(data, true);
      expect(result['unread_count'], 4);
    });

    test('should handle empty map', () {
      final result =
          processor.processGetNotifications(<String, dynamic>{}, true);
      expect(result['success'], true);
      expect(result['notifications'], isEmpty);
      expect(result['unread_count'], 0);
    });

    test('should handle map without list values', () {
      final data = {'notifications': 'not a list', 'items': 42};
      final result = processor.processGetNotifications(data, true);
      expect(result['notifications'], isEmpty);
    });
  });

  group('processGetNotifications - other types', () {
    test('should handle null data', () {
      final result = processor.processGetNotifications(null, true);
      expect(result['success'], true);
      expect(result['notifications'], isEmpty);
      expect(result['unread_count'], 0);
    });

    test('should handle string data', () {
      final result = processor.processGetNotifications('invalid', true);
      expect(result['success'], true);
      expect(result['notifications'], isEmpty);
    });

    test('should handle int data', () {
      final result = processor.processGetNotifications(42, true);
      expect(result['success'], true);
      expect(result['notifications'], isEmpty);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetNotifications(null, false);
      expect(result['success'], false);
      expect(result['message'], '获取通知失败');
    });
  });

  group('processGetUnreadCount', () {
    test('should return unread count from map', () {
      final result = processor.processGetUnreadCount({'unread_count': 5}, true);
      expect(result['success'], true);
      expect(result['unread_count'], 5);
    });

    test('should default to 0 when missing', () {
      final result = processor.processGetUnreadCount({}, true);
      expect(result['unread_count'], 0);
    });

    test('should default to 0 for null data', () {
      final result = processor.processGetUnreadCount(null, true);
      expect(result['unread_count'], 0);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetUnreadCount(null, false);
      expect(result['success'], false);
    });

    test('should handle zero unread', () {
      final result = processor.processGetUnreadCount({'unread_count': 0}, true);
      expect(result['unread_count'], 0);
    });

    test('should handle large unread count', () {
      final result =
          processor.processGetUnreadCount({'unread_count': 9999}, true);
      expect(result['unread_count'], 9999);
    });

    test('should handle non-map data', () {
      final result = processor.processGetUnreadCount('invalid', true);
      expect(result['unread_count'], 0);
    });

    test('should handle list data', () {
      final result = processor.processGetUnreadCount([1, 2, 3], true);
      expect(result['unread_count'], 0);
    });
  });

  group('processMarkAsRead', () {
    test('should return success with updated unread count', () {
      final result = processor.processMarkAsRead({'unread_count': 3}, true);
      expect(result['success'], true);
      expect(result['unread_count'], 3);
    });

    test('should handle null unread count', () {
      final result = processor.processMarkAsRead({}, true);
      expect(result['success'], true);
      expect(result['unread_count'], isNull);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processMarkAsRead(null, false);
      expect(result['success'], false);
      expect(result['message'], '标记失败');
    });

    test('should handle non-map data', () {
      final result = processor.processMarkAsRead('ok', true);
      expect(result['success'], true);
      expect(result['unread_count'], isNull);
    });

    test('should handle zero unread after mark', () {
      final result = processor.processMarkAsRead({'unread_count': 0}, true);
      expect(result['unread_count'], 0);
    });

    test('should handle null data on success', () {
      final result = processor.processMarkAsRead(null, true);
      expect(result['success'], true);
      expect(result['unread_count'], isNull);
    });
  });

  group('processMarkAllAsRead', () {
    test('should return success true', () {
      expect(processor.processMarkAllAsRead(true)['success'], true);
    });

    test('should return success false', () {
      expect(processor.processMarkAllAsRead(false)['success'], false);
    });
  });
}
