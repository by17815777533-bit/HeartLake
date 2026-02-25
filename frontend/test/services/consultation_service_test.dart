import 'package:flutter_test/flutter_test.dart';

/// ConsultationService 继承 BaseService，无法直接实例化。
/// 提取核心逻辑进行测试：会话创建、消息处理、E2E加密状态管理。

class ConsultationResponseProcessor {
  Map<String, dynamic> processCreateSession(dynamic data, bool success, {String? counselorId}) {
    if (!success) return {'success': false, 'message': data is Map ? (data['message'] ?? '创建失败') : '创建失败'};
    return {'success': true, 'data': data, 'code': 200, 'message': null};
  }

  Map<String, dynamic> processSendMessage({
    required dynamic data,
    required bool success,
    required String content,
    required String sessionId,
    required bool isE2EReady,
  }) {
    if (sessionId.isEmpty) return {'success': false, 'message': '会话ID不能为空'};
    if (content.isEmpty) return {'success': false, 'message': '消息不能为空'};
    if (!success) return {'success': false, 'message': '发送失败'};
    return {
      'success': true,
      'data': data,
      'encrypted': isE2EReady,
    };
  }

  Map<String, dynamic> processGetMessages({
    required dynamic data,
    required bool success,
    required bool isE2EReady,
  }) {
    if (!success) return {'success': false, 'message': '获取消息失败'};
    final result = {'success': true, 'data': data, 'code': 200, 'message': null};

    if (isE2EReady && data != null) {
      final messages = data is Map ? data['messages'] as List? : null;
      if (messages != null) {
        for (int i = 0; i < messages.length; i++) {
          final msg = messages[i] as Map<String, dynamic>;
          if (msg['encrypted'] == true) {
            msg['content'] = '[已解密] ${msg['content']}';
          }
        }
      }
    }
    return result;
  }

  Map<String, dynamic> processGetSessions(dynamic data, bool success) {
    if (!success) return {'success': false, 'message': '获取会话列表失败'};
    return {'success': true, 'data': data, 'code': 200, 'message': null};
  }

  Map<String, dynamic> processExchangeKey({
    required dynamic data,
    required bool success,
    required String sessionId,
    required String publicKey,
  }) {
    if (sessionId.isEmpty) return {'success': false, 'message': '会话ID不能为空'};
    if (publicKey.isEmpty) return {'success': false, 'message': '公钥不能为空'};
    if (!success) return {'success': false, 'message': '密钥交换失败'};
    return {'success': true, 'data': data, 'code': 200, 'message': null};
  }
}

/// E2E加密状态管理器 - 复刻 ConsultationService 的加密状态逻辑
class E2EStateManager {
  bool _isReady = false;
  String? _peerPublicKey;

  bool get isReady => _isReady;
  String? get peerPublicKey => _peerPublicKey;

  bool initE2E(String? peerPublicKey) {
    if (peerPublicKey == null || peerPublicKey.isEmpty) return false;
    _peerPublicKey = peerPublicKey;
    _isReady = true;
    return true;
  }

  void dispose() {
    _isReady = false;
    _peerPublicKey = null;
  }

  String processOutgoingMessage(String content) {
    if (_isReady) return '[encrypted]$content';
    return content;
  }

  String processIncomingMessage(String content, bool encrypted) {
    if (encrypted && _isReady) return content.replaceFirst('[encrypted]', '');
    if (encrypted && !_isReady) return '[无法解密]';
    return content;
  }
}

void main() {
  late ConsultationResponseProcessor processor;
  late E2EStateManager e2eManager;

  setUp(() {
    processor = ConsultationResponseProcessor();
    e2eManager = E2EStateManager();
  });

  // ==================== 会话创建 ====================
  group('processCreateSession', () {
    test('should return success on valid response', () {
      final result = processor.processCreateSession({'session_id': 's1'}, true);
      expect(result['success'], true);
      expect(result['data']['session_id'], 's1');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processCreateSession({'message': '创建失败'}, false);
      expect(result['success'], false);
    });

    test('should handle null data on success', () {
      final result = processor.processCreateSession(null, true);
      expect(result['success'], true);
      expect(result['data'], isNull);
    });

    test('should handle counselorId parameter', () {
      final result = processor.processCreateSession(
        {'session_id': 's2', 'counselor_id': 'c1'},
        true,
        counselorId: 'c1',
      );
      expect(result['success'], true);
    });

    test('should handle null counselorId', () {
      final result = processor.processCreateSession({'session_id': 's3'}, true);
      expect(result['success'], true);
    });

    test('should use default message on failure without message', () {
      final result = processor.processCreateSession(null, false);
      expect(result['message'], '创建失败');
    });

    test('should use server message on failure', () {
      final result = processor.processCreateSession({'message': '咨询师不在线'}, false);
      expect(result['message'], '咨询师不在线');
    });

    test('should include code 200 on success', () {
      final result = processor.processCreateSession({}, true);
      expect(result['code'], 200);
    });
  });

  // ==================== 发送消息 ====================
  group('processSendMessage', () {
    test('should return success with encrypted flag when E2E ready', () {
      final result = processor.processSendMessage(
        data: {'message_id': 'm1'},
        success: true,
        content: '你好',
        sessionId: 's1',
        isE2EReady: true,
      );
      expect(result['success'], true);
      expect(result['encrypted'], true);
    });

    test('should return success without encryption when E2E not ready', () {
      final result = processor.processSendMessage(
        data: {'message_id': 'm2'},
        success: true,
        content: '你好',
        sessionId: 's1',
        isE2EReady: false,
      );
      expect(result['success'], true);
      expect(result['encrypted'], false);
    });

    test('should fail on empty content', () {
      final result = processor.processSendMessage(
        data: null,
        success: true,
        content: '',
        sessionId: 's1',
        isE2EReady: false,
      );
      expect(result['success'], false);
      expect(result['message'], '消息不能为空');
    });

    test('should fail on empty sessionId', () {
      final result = processor.processSendMessage(
        data: null,
        success: true,
        content: '你好',
        sessionId: '',
        isE2EReady: false,
      );
      expect(result['success'], false);
      expect(result['message'], '会话ID不能为空');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processSendMessage(
        data: null,
        success: false,
        content: '你好',
        sessionId: 's1',
        isE2EReady: false,
      );
      expect(result['success'], false);
    });

    test('should handle long content', () {
      final longContent = 'a' * 10000;
      final result = processor.processSendMessage(
        data: {'message_id': 'm3'},
        success: true,
        content: longContent,
        sessionId: 's1',
        isE2EReady: false,
      );
      expect(result['success'], true);
    });

    test('should handle special characters in content', () {
      final result = processor.processSendMessage(
        data: {'message_id': 'm4'},
        success: true,
        content: '你好！@#\$%^&*()_+{}|:"<>?',
        sessionId: 's1',
        isE2EReady: false,
      );
      expect(result['success'], true);
    });

    test('should handle unicode content', () {
      final result = processor.processSendMessage(
        data: {'message_id': 'm5'},
        success: true,
        content: '🌊💙🪨',
        sessionId: 's1',
        isE2EReady: true,
      );
      expect(result['success'], true);
      expect(result['encrypted'], true);
    });
  });

  // ==================== 获取消息 ====================
  group('processGetMessages', () {
    test('should return messages on success', () {
      final result = processor.processGetMessages(
        data: {
          'messages': [
            {'content': '你好', 'encrypted': false},
            {'content': '回复', 'encrypted': false},
          ]
        },
        success: true,
        isE2EReady: false,
      );
      expect(result['success'], true);
    });

    test('should decrypt encrypted messages when E2E ready', () {
      final result = processor.processGetMessages(
        data: {
          'messages': [
            {'content': '加密内容', 'encrypted': true},
            {'content': '明文内容', 'encrypted': false},
          ]
        },
        success: true,
        isE2EReady: true,
      );
      expect(result['success'], true);
      final messages = (result['data'] as Map)['messages'] as List;
      expect(messages[0]['content'], contains('已解密'));
      expect(messages[1]['content'], '明文内容');
    });

    test('should not decrypt when E2E not ready', () {
      final result = processor.processGetMessages(
        data: {
          'messages': [
            {'content': '加密内容', 'encrypted': true},
          ]
        },
        success: true,
        isE2EReady: false,
      );
      expect(result['success'], true);
      final messages = (result['data'] as Map)['messages'] as List;
      expect(messages[0]['content'], '加密内容');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetMessages(data: null, success: false, isE2EReady: false);
      expect(result['success'], false);
    });

    test('should handle null data on success', () {
      final result = processor.processGetMessages(data: null, success: true, isE2EReady: false);
      expect(result['success'], true);
    });

    test('should handle empty messages list', () {
      final result = processor.processGetMessages(
        data: {'messages': []},
        success: true,
        isE2EReady: true,
      );
      expect(result['success'], true);
    });

    test('should handle null messages key', () {
      final result = processor.processGetMessages(
        data: {'other': 'data'},
        success: true,
        isE2EReady: true,
      );
      expect(result['success'], true);
    });

    test('should handle mixed encrypted and plain messages', () {
      final result = processor.processGetMessages(
        data: {
          'messages': [
            {'content': 'msg1', 'encrypted': true},
            {'content': 'msg2', 'encrypted': false},
            {'content': 'msg3', 'encrypted': true},
          ]
        },
        success: true,
        isE2EReady: true,
      );
      final messages = (result['data'] as Map)['messages'] as List;
      expect(messages[0]['content'], contains('已解密'));
      expect(messages[1]['content'], 'msg2');
      expect(messages[2]['content'], contains('已解密'));
    });
  });

  // ==================== 获取会话列表 ====================
  group('processGetSessions', () {
    test('should return sessions on success', () {
      final result = processor.processGetSessions([
        {'session_id': 's1', 'status': 'active'},
        {'session_id': 's2', 'status': 'closed'},
      ], true);
      expect(result['success'], true);
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processGetSessions(null, false);
      expect(result['success'], false);
    });

    test('should handle empty sessions', () {
      final result = processor.processGetSessions([], true);
      expect(result['success'], true);
    });

    test('should handle null data', () {
      final result = processor.processGetSessions(null, true);
      expect(result['success'], true);
      expect(result['data'], isNull);
    });
  });

  // ==================== 密钥交换 ====================
  group('processExchangeKey', () {
    test('should return success with peer key', () {
      final result = processor.processExchangeKey(
        data: {'peer_public_key': 'pk_abc'},
        success: true,
        sessionId: 's1',
        publicKey: 'my_pk',
      );
      expect(result['success'], true);
    });

    test('should fail on empty sessionId', () {
      final result = processor.processExchangeKey(
        data: null,
        success: true,
        sessionId: '',
        publicKey: 'my_pk',
      );
      expect(result['success'], false);
      expect(result['message'], '会话ID不能为空');
    });

    test('should fail on empty publicKey', () {
      final result = processor.processExchangeKey(
        data: null,
        success: true,
        sessionId: 's1',
        publicKey: '',
      );
      expect(result['success'], false);
      expect(result['message'], '公钥不能为空');
    });

    test('should fail on unsuccessful response', () {
      final result = processor.processExchangeKey(
        data: null,
        success: false,
        sessionId: 's1',
        publicKey: 'my_pk',
      );
      expect(result['success'], false);
    });

    test('should handle null data on success', () {
      final result = processor.processExchangeKey(
        data: null,
        success: true,
        sessionId: 's1',
        publicKey: 'my_pk',
      );
      expect(result['success'], true);
    });
  });

  // ==================== E2E 状态管理 ====================
  group('E2EStateManager', () {
    test('should start as not ready', () {
      expect(e2eManager.isReady, false);
      expect(e2eManager.peerPublicKey, isNull);
    });

    test('should become ready after initE2E with valid key', () {
      final result = e2eManager.initE2E('peer_pk_123');
      expect(result, true);
      expect(e2eManager.isReady, true);
      expect(e2eManager.peerPublicKey, 'peer_pk_123');
    });

    test('should fail initE2E with null key', () {
      final result = e2eManager.initE2E(null);
      expect(result, false);
      expect(e2eManager.isReady, false);
    });

    test('should fail initE2E with empty key', () {
      final result = e2eManager.initE2E('');
      expect(result, false);
      expect(e2eManager.isReady, false);
    });

    test('should reset on dispose', () {
      e2eManager.initE2E('peer_pk');
      expect(e2eManager.isReady, true);
      e2eManager.dispose();
      expect(e2eManager.isReady, false);
      expect(e2eManager.peerPublicKey, isNull);
    });

    test('should encrypt outgoing message when ready', () {
      e2eManager.initE2E('peer_pk');
      final encrypted = e2eManager.processOutgoingMessage('你好');
      expect(encrypted, '[encrypted]你好');
    });

    test('should not encrypt outgoing message when not ready', () {
      final plain = e2eManager.processOutgoingMessage('你好');
      expect(plain, '你好');
    });

    test('should decrypt incoming encrypted message when ready', () {
      e2eManager.initE2E('peer_pk');
      final decrypted = e2eManager.processIncomingMessage('[encrypted]你好', true);
      expect(decrypted, '你好');
    });

    test('should return [无法解密] when not ready for encrypted message', () {
      final result = e2eManager.processIncomingMessage('[encrypted]你好', true);
      expect(result, '[无法解密]');
    });

    test('should return plain text for non-encrypted message', () {
      final result = e2eManager.processIncomingMessage('你好', false);
      expect(result, '你好');
    });

    test('should handle re-initialization', () {
      e2eManager.initE2E('key1');
      expect(e2eManager.peerPublicKey, 'key1');
      e2eManager.initE2E('key2');
      expect(e2eManager.peerPublicKey, 'key2');
    });

    test('should handle dispose and re-init', () {
      e2eManager.initE2E('key1');
      e2eManager.dispose();
      e2eManager.initE2E('key2');
      expect(e2eManager.isReady, true);
      expect(e2eManager.peerPublicKey, 'key2');
    });

    test('should handle empty message encryption', () {
      e2eManager.initE2E('key');
      final encrypted = e2eManager.processOutgoingMessage('');
      expect(encrypted, '[encrypted]');
    });

    test('should handle long message encryption', () {
      e2eManager.initE2E('key');
      final longMsg = 'a' * 10000;
      final encrypted = e2eManager.processOutgoingMessage(longMsg);
      expect(encrypted, '[encrypted]$longMsg');
    });

    test('should handle special characters in encryption', () {
      e2eManager.initE2E('key');
      const msg = '你好！@#\$%^&*()';
      final encrypted = e2eManager.processOutgoingMessage(msg);
      expect(encrypted, '[encrypted]$msg');
    });
  });
}
