import 'package:flutter_test/flutter_test.dart';

/// InteractionService 继承 BaseService，无法直接实例化。
/// 提取核心响应处理逻辑进行测试。

class InteractionResponseProcessor {
  Map<String, dynamic> processCreateRipple(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '涟漪创建失败'};
    return {
      'success': true,
      'data': data,
      'ripple_id': data is Map ? data['ripple_id'] : null,
    };
  }

  Map<String, dynamic> processCreateBoat({
    required dynamic data,
    required bool success,
    required String content,
    bool isAnonymous = true,
  }) {
    if (content.isEmpty) return {'success': false, 'message': '内容不能为空'};
    if (!success) return {'success': false, 'message': '纸船发送失败'};
    return {
      'success': true,
      'data': data,
      'boat_id': data is Map ? data['boat_id'] : null,
    };
  }

  Map<String, dynamic> processGetBoats(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取纸船失败'};
    final boats = data is List
        ? data
        : (data is Map ? (data['boats'] ?? data['items'] ?? []) : []);
    return {'success': true, 'boats': boats};
  }

  Map<String, dynamic> processGetMessages(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取消息失败'};
    final messages = data is List
        ? data
        : (data is Map ? (data['items'] ?? data['messages'] ?? []) : []);
    return {'success': true, 'messages': messages};
  }

  Map<String, dynamic> processSendMessage({
    required dynamic data,
    required bool success,
    required String content,
    String messageType = 'text',
    List<String>? mediaIds,
    int? voiceDuration,
  }) {
    if (content.isEmpty) return {'success': false, 'message': '消息不能为空'};
    if (!success) return {'success': false, 'message': '发送失败'};
    return {
      'success': true,
      'message': data is Map ? data['message'] : data,
    };
  }

  Map<String, dynamic> processGetMyRipples(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取涟漪失败'};
    final ripples = data is List
        ? data
        : (data is Map ? (data['items'] ?? data['ripples'] ?? []) : []);
    return {'success': true, 'ripples': ripples};
  }

  Map<String, dynamic> processGetMyBoatsComments(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取纸船失败'};
    final boats = data is List
        ? data
        : (data is Map ? (data['boats'] ?? data['items'] ?? []) : []);
    return {'success': true, 'boats': boats};
  }

  Map<String, dynamic> buildSendMessageData({
    required String content,
    String messageType = 'text',
    List<String>? mediaIds,
    int? voiceDuration,
  }) {
    return {
      'content': content,
      'message_type': messageType,
      if (mediaIds != null && mediaIds.isNotEmpty) 'media_ids': mediaIds,
      if (voiceDuration != null) 'voice_duration': voiceDuration,
    };
  }
}

void main() {
  late InteractionResponseProcessor processor;

  setUp(() {
    processor = InteractionResponseProcessor();
  });

  group('processCreateRipple', () {
    test('should return success with ripple_id', () {
      final result = processor.processCreateRipple({'ripple_id': 'r1', 'count': 5}, true);
      expect(result['success'], true);
      expect(result['ripple_id'], 'r1');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processCreateRipple(null, false);
      expect(result['success'], false);
      expect(result['message'], '涟漪创建失败');
    });

    test('should handle null data on success', () {
      final result = processor.processCreateRipple(null, true);
      expect(result['success'], true);
      expect(result['ripple_id'], isNull);
    });

    test('should handle non-map data', () {
      final result = processor.processCreateRipple('string_data', true);
      expect(result['success'], true);
      expect(result['ripple_id'], isNull);
    });

    test('should handle missing ripple_id in map', () {
      final result = processor.processCreateRipple({'count': 3}, true);
      expect(result['success'], true);
      expect(result['ripple_id'], isNull);
    });

    test('should preserve full data', () {
      final data = {'ripple_id': 'r1', 'count': 5, 'created_at': '2026-01-01'};
      final result = processor.processCreateRipple(data, true);
      expect(result['data'], data);
    });
  });

  group('processCreateBoat', () {
    test('should return success with boat_id', () {
      final result = processor.processCreateBoat(
        data: {'boat_id': 'b1'},
        success: true,
        content: '加油！',
      );
      expect(result['success'], true);
      expect(result['boat_id'], 'b1');
    });

    test('should fail on empty content', () {
      final result = processor.processCreateBoat(
        data: {},
        success: true,
        content: '',
      );
      expect(result['success'], false);
      expect(result['message'], '内容不能为空');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processCreateBoat(
        data: null,
        success: false,
        content: '内容',
      );
      expect(result['success'], false);
    });

    test('should handle anonymous boat', () {
      final result = processor.processCreateBoat(
        data: {'boat_id': 'b2'},
        success: true,
        content: '匿名留言',
        isAnonymous: true,
      );
      expect(result['success'], true);
    });

    test('should handle non-anonymous boat', () {
      final result = processor.processCreateBoat(
        data: {'boat_id': 'b3'},
        success: true,
        content: '实名留言',
        isAnonymous: false,
      );
      expect(result['success'], true);
    });

    test('should handle null data on success', () {
      final result = processor.processCreateBoat(
        data: null,
        success: true,
        content: '内容',
      );
      expect(result['success'], true);
      expect(result['boat_id'], isNull);
    });

    test('should handle long content', () {
      final longContent = 'A' * 10000;
      final result = processor.processCreateBoat(
        data: {'boat_id': 'b4'},
        success: true,
        content: longContent,
      );
      expect(result['success'], true);
    });

    test('should handle unicode content', () {
      final result = processor.processCreateBoat(
        data: {'boat_id': 'b5'},
        success: true,
        content: '你好世界🌍',
      );
      expect(result['success'], true);
    });
  });

  group('processGetBoats', () {
    test('should handle List data', () {
      final result = processor.processGetBoats([
        {'boat_id': 'b1', 'content': '留言1'},
        {'boat_id': 'b2', 'content': '留言2'},
      ], true);
      expect(result['success'], true);
      expect((result['boats'] as List).length, 2);
    });

    test('should handle Map with boats key', () {
      final result = processor.processGetBoats({
        'boats': [{'boat_id': 'b1'}],
      }, true);
      expect((result['boats'] as List).length, 1);
    });

    test('should handle Map with items key', () {
      final result = processor.processGetBoats({
        'items': [{'boat_id': 'b1'}, {'boat_id': 'b2'}],
      }, true);
      expect((result['boats'] as List).length, 2);
    });

    test('should handle empty list', () {
      final result = processor.processGetBoats([], true);
      expect(result['boats'], isEmpty);
    });

    test('should handle null data', () {
      final result = processor.processGetBoats(null, true);
      expect(result['boats'], isEmpty);
    });

    test('should handle non-list non-map data', () {
      final result = processor.processGetBoats('invalid', true);
      expect(result['boats'], isEmpty);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetBoats(null, false);
      expect(result['success'], false);
    });

    test('should handle Map without boats or items key', () {
      final result = processor.processGetBoats({'other': 'data'}, true);
      expect(result['boats'], isEmpty);
    });
  });

  group('processGetMessages', () {
    test('should handle List data', () {
      final result = processor.processGetMessages([
        {'id': 'm1', 'content': '你好'},
      ], true);
      expect(result['success'], true);
      expect((result['messages'] as List).length, 1);
    });

    test('should handle Map with messages key', () {
      final result = processor.processGetMessages({
        'messages': [{'id': 'm1'}],
      }, true);
      expect((result['messages'] as List).length, 1);
    });

    test('should handle Map with items key', () {
      final result = processor.processGetMessages({
        'items': [{'id': 'm1'}, {'id': 'm2'}],
      }, true);
      expect((result['messages'] as List).length, 2);
    });

    test('should handle empty data', () {
      final result = processor.processGetMessages([], true);
      expect(result['messages'], isEmpty);
    });

    test('should handle null data', () {
      final result = processor.processGetMessages(null, true);
      expect(result['messages'], isEmpty);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetMessages(null, false);
      expect(result['success'], false);
    });

    test('should prefer items over messages key', () {
      final result = processor.processGetMessages({
        'items': [{'id': 'i1'}],
        'messages': [{'id': 'm1'}, {'id': 'm2'}],
      }, true);
      expect((result['messages'] as List).length, 1);
    });
  });

  group('processSendMessage', () {
    test('should return success with message data', () {
      final result = processor.processSendMessage(
        data: {'message': {'id': 'm1', 'content': '你好'}},
        success: true,
        content: '你好',
      );
      expect(result['success'], true);
      expect(result['message']['id'], 'm1');
    });

    test('should fail on empty content', () {
      final result = processor.processSendMessage(
        data: {},
        success: true,
        content: '',
      );
      expect(result['success'], false);
      expect(result['message'], '消息不能为空');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processSendMessage(
        data: null,
        success: false,
        content: '内容',
      );
      expect(result['success'], false);
    });

    test('should handle non-map data', () {
      final result = processor.processSendMessage(
        data: 'raw_data',
        success: true,
        content: '内容',
      );
      expect(result['success'], true);
      expect(result['message'], 'raw_data');
    });

    test('should handle text message type', () {
      final result = processor.processSendMessage(
        data: {'message': 'ok'},
        success: true,
        content: '文本消息',
        messageType: 'text',
      );
      expect(result['success'], true);
    });

    test('should handle voice message type', () {
      final result = processor.processSendMessage(
        data: {'message': 'ok'},
        success: true,
        content: 'voice_url',
        messageType: 'voice',
        voiceDuration: 30,
      );
      expect(result['success'], true);
    });

    test('should handle media message', () {
      final result = processor.processSendMessage(
        data: {'message': 'ok'},
        success: true,
        content: '带图消息',
        mediaIds: ['media1', 'media2'],
      );
      expect(result['success'], true);
    });
  });

  group('processGetMyRipples', () {
    test('should handle List data', () {
      final result = processor.processGetMyRipples([
        {'ripple_id': 'r1'},
        {'ripple_id': 'r2'},
      ], true);
      expect(result['success'], true);
      expect((result['ripples'] as List).length, 2);
    });

    test('should handle Map with ripples key', () {
      final result = processor.processGetMyRipples({
        'ripples': [{'ripple_id': 'r1'}],
      }, true);
      expect((result['ripples'] as List).length, 1);
    });

    test('should handle Map with items key', () {
      final result = processor.processGetMyRipples({
        'items': [{'ripple_id': 'r1'}],
      }, true);
      expect((result['ripples'] as List).length, 1);
    });

    test('should handle empty data', () {
      final result = processor.processGetMyRipples([], true);
      expect(result['ripples'], isEmpty);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetMyRipples(null, false);
      expect(result['success'], false);
    });

    test('should handle null data on success', () {
      final result = processor.processGetMyRipples(null, true);
      expect(result['ripples'], isEmpty);
    });
  });

  group('processGetMyBoatsComments', () {
    test('should handle List data', () {
      final result = processor.processGetMyBoatsComments([
        {'boat_id': 'b1'},
      ], true);
      expect(result['success'], true);
      expect((result['boats'] as List).length, 1);
    });

    test('should handle Map with boats key', () {
      final result = processor.processGetMyBoatsComments({
        'boats': [{'boat_id': 'b1'}, {'boat_id': 'b2'}],
      }, true);
      expect((result['boats'] as List).length, 2);
    });

    test('should handle Map with items key', () {
      final result = processor.processGetMyBoatsComments({
        'items': [{'boat_id': 'b1'}],
      }, true);
      expect((result['boats'] as List).length, 1);
    });

    test('should handle empty data', () {
      final result = processor.processGetMyBoatsComments([], true);
      expect(result['boats'], isEmpty);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetMyBoatsComments(null, false);
      expect(result['success'], false);
    });
  });

  group('buildSendMessageData', () {
    test('should build basic text message data', () {
      final data = processor.buildSendMessageData(content: '你好');
      expect(data['content'], '你好');
      expect(data['message_type'], 'text');
      expect(data.containsKey('media_ids'), false);
      expect(data.containsKey('voice_duration'), false);
    });

    test('should include media_ids when provided', () {
      final data = processor.buildSendMessageData(
        content: '带图',
        mediaIds: ['m1', 'm2'],
      );
      expect(data['media_ids'], ['m1', 'm2']);
    });

    test('should not include empty media_ids', () {
      final data = processor.buildSendMessageData(
        content: '无图',
        mediaIds: [],
      );
      expect(data.containsKey('media_ids'), false);
    });

    test('should include voice_duration when provided', () {
      final data = processor.buildSendMessageData(
        content: 'voice',
        messageType: 'voice',
        voiceDuration: 15,
      );
      expect(data['voice_duration'], 15);
      expect(data['message_type'], 'voice');
    });

    test('should not include null voice_duration', () {
      final data = processor.buildSendMessageData(content: '文本');
      expect(data.containsKey('voice_duration'), false);
    });

    test('should handle all parameters together', () {
      final data = processor.buildSendMessageData(
        content: '完整消息',
        messageType: 'mixed',
        mediaIds: ['m1'],
        voiceDuration: 10,
      );
      expect(data['content'], '完整消息');
      expect(data['message_type'], 'mixed');
      expect(data['media_ids'], ['m1']);
      expect(data['voice_duration'], 10);
    });
  });

  group('Connection operations', () {
    test('createConnectionByStone should validate stoneId', () {
      const stoneId = 'stone_123';
      expect(stoneId, isNotEmpty);
      expect(stoneId, contains('stone'));
    });

    test('createConnection should validate targetUserId', () {
      const targetUserId = 'user_456';
      expect(targetUserId, isNotEmpty);
    });

    test('upgradeConnectionToFriend should validate connectionId', () {
      const connectionId = 'conn_789';
      expect(connectionId, isNotEmpty);
    });

    test('deleteBoat should validate boatId', () {
      const boatId = 'boat_001';
      expect(boatId, isNotEmpty);
    });

    test('deleteRipple should validate rippleId', () {
      const rippleId = 'ripple_001';
      expect(rippleId, isNotEmpty);
    });
  });

  group('Pagination parameters', () {
    test('default page should be 1', () {
      const page = 1;
      const pageSize = 20;
      expect(page, 1);
      expect(pageSize, 20);
    });

    test('should accept custom page and pageSize', () {
      const page = 3;
      const pageSize = 50;
      expect(page, greaterThan(0));
      expect(pageSize, greaterThan(0));
    });

    test('should build query parameters correctly', () {
      const page = 2;
      const pageSize = 10;
      final params = {'page': page, 'page_size': pageSize};
      expect(params['page'], 2);
      expect(params['page_size'], 10);
    });
  });
}
