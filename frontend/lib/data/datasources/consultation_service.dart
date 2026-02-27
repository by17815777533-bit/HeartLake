// 心理咨询服务 - 预约咨询和E2E加密会话
import 'package:cryptography/cryptography.dart';
import 'package:flutter/foundation.dart';
import 'base_service.dart';
import '../../utils/e2e_encryption.dart';
import '../../utils/input_validator.dart';

class ConsultationService extends BaseService {
  @override
  String get serviceName => 'ConsultationService';

  final E2EEncryption _e2e = E2EEncryption();
  SimpleKeyPair? _keyPair;

  /// 创建咨询会话
  Future<Map<String, dynamic>> createSession({String? counselorId}) async {
    if (counselorId != null) {
      InputValidator.validateUUID(counselorId, '咨询师ID');
    }
    final resp = await post('/consultation/session', data: {
      if (counselorId != null) 'counselor_id': counselorId,
    });
    return toMap(resp);
  }

  /// 初始化 E2E 加密：生成密钥对、交换公钥、派生共享密钥
  Future<bool> initE2E(String sessionId) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    try {
      _keyPair = await _e2e.generateKeyPair();
      final myPublicKey = await _e2e.exportPublicKey(_keyPair!);

      final resp = await exchangeKey(
        sessionId: sessionId,
        publicKey: myPublicKey,
      );

      final peerPublicKey = resp['data']?['peer_public_key'] as String?;
      if (peerPublicKey == null || peerPublicKey.isEmpty) return false;

      await _e2e.deriveSharedSecret(_keyPair!, peerPublicKey);
      return true;
    } catch (e) {
      if (kDebugMode) debugPrint('E2E初始化失败: $e');
      return false;
    }
  }

  /// 发送消息（加密就绪时自动加密）
  Future<Map<String, dynamic>> sendMessage({
    required String sessionId,
    required String content,
  }) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    InputValidator.requireLength(content, '消息内容', min: 1, max: 5000);
    content = InputValidator.sanitizeText(content);
    String payload = content;
    bool encrypted = false;

    if (_e2e.isReady) {
      try {
        payload = await _e2e.encrypt(content);
        encrypted = true;
      } catch (e) {
        if (kDebugMode) debugPrint('[ConsultationService] 加密失败，拒绝发送明文: $e');
        return {'success': false, 'message': '消息加密失败，请检查网络后重试'};
      }
    } else {
      // 咨询消息涉及隐私，E2E 未就绪时拒绝发送明文
      if (kDebugMode) debugPrint('[ConsultationService] E2E未就绪，拒绝发送明文');
      return {'success': false, 'message': '安全通道未建立，请稍后再试'};
    }

    final resp = await post('/consultation/message', data: {
      'session_id': sessionId,
      'content': payload,
      'encrypted': encrypted,
    });
    return toMap(resp);
  }

  /// 获取消息历史（加密消息自动解密）
  Future<Map<String, dynamic>> getMessages(String sessionId) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    final resp = await get('/consultation/messages/$sessionId');
    final result = toMap(resp);

    // 解密消息
    if (_e2e.isReady && result['data'] != null) {
      final messages = result['data']['messages'] as List?;
      if (messages != null) {
        for (int i = 0; i < messages.length; i++) {
          final msg = messages[i] as Map<String, dynamic>;
          if (msg['encrypted'] == true) {
            try {
              msg['content'] = await _e2e.decrypt(msg['content'] as String);
            } catch (e) {
              msg['content'] = '[无法解密]';
            }
          }
        }
      }
    }

    return result;
  }

  /// 获取咨询历史会话列表
  Future<Map<String, dynamic>> getSessions() async {
    final resp = await get('/consultation/sessions');
    return toMap(resp);
  }

  /// 密钥交换（E2E加密）
  Future<Map<String, dynamic>> exchangeKey({
    required String sessionId,
    required String publicKey,
  }) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    InputValidator.validateBase64(publicKey, '公钥');
    final resp = await post('/consultation/key-exchange', data: {
      'session_id': sessionId,
      'public_key': publicKey,
    });
    return toMap(resp);
  }

  bool get isE2EReady => _e2e.isReady;

  void dispose() {
    _e2e.dispose();
    _keyPair = null;
  }
}
