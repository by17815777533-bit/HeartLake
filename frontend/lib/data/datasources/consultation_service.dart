// 心理咨询服务 - 预约咨询和E2E加密会话
import 'package:cryptography/cryptography.dart';
import 'package:flutter/foundation.dart';
import 'base_service.dart';
import '../../utils/e2e_encryption.dart';
import '../../utils/input_validator.dart';

/// 心理咨询服务
///
/// 封装咨询会话的完整生命周期，核心特性：
/// - 会话创建与历史查询
/// - X25519 密钥交换 + AES-GCM 端到端加密（E2E）
/// - 消息收发自动加密/解密，E2E 未就绪时拒绝发送明文
/// - 兼容两种加密信封格式（旧版 encrypted=true、新版 envelope 对象）
///
/// 隐私保护策略：咨询消息涉及用户心理隐私，
/// 当 E2E 通道未建立时会直接拒绝发送，而非降级为明文传输。
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

  /// 初始化 E2E 加密通道
  ///
  /// 流程：生成本地 X25519 密钥对 -> 将公钥发送给服务端 ->
  /// 接收对端公钥 -> 派生 ECDH 共享密钥。
  /// 成功返回 true，任何环节失败返回 false。
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

  /// 发送咨询消息
  ///
  /// E2E 就绪时自动加密后发送；未就绪时拒绝发送明文，
  /// 返回 `{success: false, message: '安全通道未建立...'}`。
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

  /// 获取消息历史
  ///
  /// 自动解密加密消息，兼容两种格式：
  /// - 旧版：`encrypted=true` + `content` 为密文字符串
  /// - 新版：`encrypted` 为 `{ciphertext, iv, tag}` 信封对象
  /// 解密失败的消息内容会被替换为 `[无法解密]`。
  Future<Map<String, dynamic>> getMessages(String sessionId) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    final resp = await get('/consultation/messages/$sessionId');
    final result = toMap(resp);

    final rawData = result['data'];
    final List<dynamic> messages = rawData is List
        ? rawData
        : (rawData is Map<String, dynamic> ? (rawData['messages'] ?? []) : []);

    // 解密消息（兼容两种格式：旧版 encrypted=true+content，新版 encrypted={ciphertext,iv,tag}）
    if (_e2e.isReady) {
      for (final item in messages) {
        if (item is! Map) continue;
        final msg = Map<String, dynamic>.from(item);
        final encryptedField = msg['encrypted'];
        try {
          if (encryptedField == true && msg['content'] is String) {
            msg['content'] = await _e2e.decrypt(msg['content'] as String);
          } else if (encryptedField is Map) {
            final env = Map<String, dynamic>.from(encryptedField);
            final cipher = env['ciphertext']?.toString() ?? '';
            final iv = env['iv']?.toString() ?? '';
            final tag = env['tag']?.toString() ?? '';
            if (cipher.isNotEmpty && iv.isNotEmpty && tag.isNotEmpty) {
              msg['content'] = await _e2e.decryptEnvelope(
                ciphertextBase64: cipher,
                ivBase64: iv,
                tagBase64: tag,
              );
            }
          }
        } catch (_) {
          msg['content'] = '[无法解密]';
        }
        msg['created_at'] = msg['created_at'] ?? msg['time'];
        msg['sender_type'] = msg['sender_type'] ?? msg['sender'];
        item
          ..clear()
          ..addAll(msg);
      }
    }

    result['data'] = {'messages': messages};

    return result;
  }

  /// 获取咨询历史会话列表
  Future<Map<String, dynamic>> getSessions() async {
    final resp = await get('/consultation/sessions');
    return toMap(resp);
  }

  /// 密钥交换
  ///
  /// 将本地公钥发送到服务端，换取对端公钥用于 ECDH 派生。
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

  /// E2E 加密通道是否已就绪
  bool get isE2EReady => _e2e.isReady;

  /// 释放加密资源，清除密钥对
  void dispose() {
    _e2e.dispose();
    _keyPair = null;
  }
}
