import 'package:flutter_test/flutter_test.dart';

/// FriendService 继承 BaseService，无法直接实例化。
/// 提取核心响应处理逻辑进行测试。

class FriendResponseProcessor {
  Map<String, dynamic> processSendRequest(dynamic data, bool success,
      {String? message}) {
    if (!success)
      return {
        'success': false,
        'message': data is Map ? (data['message'] ?? '发送失败') : '发送失败'
      };
    return {
      'success': true,
      'request_id': data is Map ? data['request_id'] : null,
    };
  }

  Map<String, dynamic> processAcceptRequest(bool success) {
    return {'success': success};
  }

  Map<String, dynamic> processRejectRequest(bool success) {
    return {'success': success};
  }

  Map<String, dynamic> processRemoveFriend(bool success) {
    return {'success': success};
  }

  Map<String, dynamic> processGetFriends(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取好友列表失败'};
    final friends =
        data is Map ? (data['friends'] ?? data['items'] ?? data['list']) : null;
    final total = data is Map
        ? data['total'] ?? (friends is List ? friends.length : null)
        : null;
    return {
      'success': true,
      'friends': friends,
      'items': friends,
      'list': friends,
      'total': total,
    };
  }

  Map<String, dynamic> processGetPendingRequests(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取请求列表失败'};
    final requests = data is Map
        ? (data['requests'] ?? data['items'] ?? data['list'])
        : null;
    final total = data is Map
        ? data['total'] ?? (requests is List ? requests.length : null)
        : null;
    return {
      'success': true,
      'requests': requests,
      'items': requests,
      'list': requests,
      'total': total,
    };
  }

  Map<String, dynamic> processGetMessages(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取消息失败'};
    List messages;
    if (data is List) {
      messages = data;
    } else if (data is Map) {
      messages = (data['messages'] as List?) ?? [];
    } else {
      messages = [];
    }
    return {'success': true, 'messages': messages};
  }

  Map<String, dynamic> processSendMessage(bool success) {
    return {'success': success};
  }
}

void main() {
  late FriendResponseProcessor processor;

  setUp(() {
    processor = FriendResponseProcessor();
  });

  group('processSendRequest', () {
    test('should return success with request_id', () {
      final result =
          processor.processSendRequest({'request_id': 'req_1'}, true);
      expect(result['success'], true);
      expect(result['request_id'], 'req_1');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processSendRequest({'message': '不能添加自己'}, false);
      expect(result['success'], false);
      expect(result['message'], '不能添加自己');
    });

    test('should use default message on failure', () {
      final result = processor.processSendRequest(null, false);
      expect(result['success'], false);
      expect(result['message'], '发送失败');
    });

    test('should handle null data on success', () {
      final result = processor.processSendRequest(null, true);
      expect(result['success'], true);
      expect(result['request_id'], isNull);
    });

    test('should handle missing request_id', () {
      final result = processor.processSendRequest({'other': 'data'}, true);
      expect(result['success'], true);
      expect(result['request_id'], isNull);
    });

    test('should handle empty map data', () {
      final result = processor.processSendRequest({}, true);
      expect(result['success'], true);
    });

    test('should handle non-map data on failure', () {
      final result = processor.processSendRequest('error', false);
      expect(result['success'], false);
      expect(result['message'], '发送失败');
    });

    test('should handle map with message on failure', () {
      final result =
          processor.processSendRequest({'message': '好友数量已达上限'}, false);
      expect(result['message'], '好友数量已达上限');
    });
  });

  group('processAcceptRequest', () {
    test('should return success true', () {
      expect(processor.processAcceptRequest(true)['success'], true);
    });

    test('should return success false', () {
      expect(processor.processAcceptRequest(false)['success'], false);
    });
  });

  group('processRejectRequest', () {
    test('should return success true', () {
      expect(processor.processRejectRequest(true)['success'], true);
    });

    test('should return success false', () {
      expect(processor.processRejectRequest(false)['success'], false);
    });
  });

  group('processRemoveFriend', () {
    test('should return success true', () {
      expect(processor.processRemoveFriend(true)['success'], true);
    });

    test('should return success false', () {
      expect(processor.processRemoveFriend(false)['success'], false);
    });
  });

  group('processGetFriends', () {
    test('should return friends list on success', () {
      final result = processor.processGetFriends({
        'friends': [
          {'user_id': 'u1', 'nickname': '小明'},
          {'user_id': 'u2', 'nickname': '小红'},
        ],
        'total': 2,
      }, true);

      expect(result['success'], true);
      expect((result['friends'] as List).length, 2);
      expect(result['total'], 2);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetFriends(null, false);
      expect(result['success'], false);
      expect(result['message'], '获取好友列表失败');
    });

    test('should handle empty friends list', () {
      final result =
          processor.processGetFriends({'friends': [], 'total': 0}, true);
      expect(result['success'], true);
      expect(result['friends'], isEmpty);
      expect(result['total'], 0);
    });

    test('should fall back to items when friends key is missing', () {
      final result = processor.processGetFriends({
        'items': [
          {'user_id': 'u9', 'nickname': '候补'},
        ],
      }, true);
      expect((result['friends'] as List).length, 1);
      expect(result['total'], 1);
    });

    test('should handle null data on success', () {
      final result = processor.processGetFriends(null, true);
      expect(result['success'], true);
      expect(result['friends'], isNull);
    });

    test('should handle missing friends key', () {
      final result = processor.processGetFriends({'total': 5}, true);
      expect(result['success'], true);
      expect(result['friends'], isNull);
    });

    test('should handle large friends list', () {
      final friends = List.generate(50, (i) => {'user_id': 'u$i'});
      final result =
          processor.processGetFriends({'friends': friends, 'total': 50}, true);
      expect((result['friends'] as List).length, 50);
    });

    test('should preserve friend data structure', () {
      final result = processor.processGetFriends({
        'friends': [
          {
            'user_id': 'u1',
            'nickname': '小明',
            'is_online': true,
            'avatar_url': 'http://img.com/1.png'
          },
        ],
        'total': 1,
      }, true);

      final friend = (result['friends'] as List).first;
      expect(friend['is_online'], true);
      expect(friend['avatar_url'], 'http://img.com/1.png');
    });
  });

  group('processGetPendingRequests', () {
    test('should return requests on success', () {
      final result = processor.processGetPendingRequests({
        'requests': [
          {'user_id': 'u1', 'message': '你好'},
        ],
        'total': 1,
      }, true);

      expect(result['success'], true);
      expect((result['requests'] as List).length, 1);
      expect(result['total'], 1);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetPendingRequests(null, false);
      expect(result['success'], false);
    });

    test('should handle empty requests', () {
      final result = processor
          .processGetPendingRequests({'requests': [], 'total': 0}, true);
      expect(result['requests'], isEmpty);
    });

    test('should handle null data on success', () {
      final result = processor.processGetPendingRequests(null, true);
      expect(result['success'], true);
      expect(result['requests'], isNull);
    });

    test('should handle missing total', () {
      final result =
          processor.processGetPendingRequests({'requests': []}, true);
      expect(result['total'], 0);
    });

    test('should preserve request data', () {
      final result = processor.processGetPendingRequests({
        'requests': [
          {'user_id': 'u1', 'message': '想和你做朋友', 'created_at': '2026-01-01'},
        ],
        'total': 1,
      }, true);

      final req = (result['requests'] as List).first;
      expect(req['message'], '想和你做朋友');
    });

    test('should fall back to items when requests key is missing', () {
      final result = processor.processGetPendingRequests({
        'items': [
          {'user_id': 'u5', 'message': '兼容列表键'},
        ],
      }, true);
      expect((result['requests'] as List).length, 1);
      expect(result['total'], 1);
    });
  });

  group('processGetMessages', () {
    test('should handle List data (backend returns array directly)', () {
      final messages = [
        {'id': 'm1', 'content': '你好', 'sender_id': 'u1'},
        {'id': 'm2', 'content': '嗨', 'sender_id': 'u2'},
      ];
      final result = processor.processGetMessages(messages, true);
      expect(result['success'], true);
      expect((result['messages'] as List).length, 2);
    });

    test('should handle Map data with messages key', () {
      final result = processor.processGetMessages({
        'messages': [
          {'id': 'm1', 'content': '你好'},
        ],
      }, true);
      expect(result['success'], true);
      expect((result['messages'] as List).length, 1);
    });

    test('should handle Map data without messages key', () {
      final result = processor.processGetMessages({'other': 'data'}, true);
      expect(result['success'], true);
      expect(result['messages'], isEmpty);
    });

    test('should handle null data on success', () {
      final result = processor.processGetMessages(null, true);
      expect(result['success'], true);
      expect(result['messages'], isEmpty);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetMessages(null, false);
      expect(result['success'], false);
      expect(result['message'], '获取消息失败');
    });

    test('should handle empty list', () {
      final result = processor.processGetMessages([], true);
      expect(result['messages'], isEmpty);
    });

    test('should handle empty map', () {
      final result = processor.processGetMessages({}, true);
      expect(result['messages'], isEmpty);
    });

    test('should handle non-list non-map data', () {
      final result = processor.processGetMessages('invalid', true);
      expect(result['messages'], isEmpty);
    });

    test('should handle large message list', () {
      final messages =
          List.generate(200, (i) => {'id': 'm$i', 'content': '消息$i'});
      final result = processor.processGetMessages(messages, true);
      expect((result['messages'] as List).length, 200);
    });

    test('should preserve message structure from list', () {
      final messages = [
        {
          'id': 'm1',
          'sender_id': 'u1',
          'receiver_id': 'u2',
          'content': '你好',
          'created_at': '2026-01-01'
        },
      ];
      final result = processor.processGetMessages(messages, true);
      final msg = (result['messages'] as List).first;
      expect(msg['sender_id'], 'u1');
      expect(msg['receiver_id'], 'u2');
    });
  });

  group('processSendMessage', () {
    test('should return success true', () {
      expect(processor.processSendMessage(true)['success'], true);
    });

    test('should return success false', () {
      expect(processor.processSendMessage(false)['success'], false);
    });
  });

  // ==================== TempFriendService 逻辑测试 ====================
  group('TempFriend response processing', () {
    Map<String, dynamic> processGetTempFriends(dynamic data, bool success) {
      if (!success) return {'success': false, 'message': '获取临时好友失败'};
      final friends = data is Map
          ? (data['temp_friends'] ??
              data['friends'] ??
              data['items'] ??
              data['list'] ??
              [])
          : [];
      return {
        'success': true,
        'temp_friends': friends,
        'total': data is Map
            ? (data['total'] ?? (friends is List ? friends.length : 0))
            : 0,
      };
    }

    Map<String, dynamic> processUpgrade(dynamic data, bool success) {
      if (!success) return {'success': false, 'message': '升级失败'};
      return {
        'success': true,
        'friendship_id': data is Map ? data['friendship_id'] : null,
      };
    }

    Map<String, dynamic> processCheckStatus(dynamic data, bool success) {
      if (!success) return {'success': false};
      return {'success': true, 'data': data};
    }

    test('getTempFriends should return list on success', () {
      final result = processGetTempFriends({
        'temp_friends': [
          {'temp_friend_id': 'tf1', 'nickname': '临时好友'}
        ],
        'total': 1,
      }, true);
      expect(result['success'], true);
      expect((result['temp_friends'] as List).length, 1);
    });

    test('getTempFriends should fall back to items', () {
      final result = processGetTempFriends({
        'items': [
          {'temp_friend_id': 'tf2', 'nickname': '兼容好友'}
        ],
      }, true);
      expect((result['temp_friends'] as List).length, 1);
      expect(result['total'], 1);
    });

    test('getTempFriends should handle empty list', () {
      final result = processGetTempFriends({'friends': [], 'total': 0}, true);
      expect(result['temp_friends'], isEmpty);
    });

    test('getTempFriends should fail on error', () {
      final result = processGetTempFriends(null, false);
      expect(result['success'], false);
    });

    test('getTempFriends should handle null data', () {
      final result = processGetTempFriends(null, true);
      expect(result['temp_friends'], isEmpty);
    });

    test('upgrade should return friendship_id on success', () {
      final result = processUpgrade({'friendship_id': 'f1'}, true);
      expect(result['success'], true);
      expect(result['friendship_id'], 'f1');
    });

    test('upgrade should fail on error', () {
      final result = processUpgrade(null, false);
      expect(result['success'], false);
    });

    test('upgrade should handle missing friendship_id', () {
      final result = processUpgrade({}, true);
      expect(result['friendship_id'], isNull);
    });

    test('checkStatus should return data on success', () {
      final result = processCheckStatus(
          {'is_temp_friend': true, 'expires_at': '2026-01-02'}, true);
      expect(result['success'], true);
      expect(result['data']['is_temp_friend'], true);
    });

    test('checkStatus should fail on error', () {
      final result = processCheckStatus(null, false);
      expect(result['success'], false);
    });
  });
}
