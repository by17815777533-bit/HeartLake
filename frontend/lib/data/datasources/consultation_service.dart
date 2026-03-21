// 心理咨询服务 - 预约咨询和E2E加密会话
import 'package:cryptography/cryptography.dart';
import 'package:flutter/foundation.dart';
import 'base_service.dart';
import '../../utils/e2e_encryption.dart';
import '../../utils/input_validator.dart';

class _ConsultationCryptoContext {
  _ConsultationCryptoContext({
    required this.e2e,
    required this.keyPair,
  });

  final E2EEncryption e2e;
  final SimpleKeyPair keyPair;
}

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

  final Map<String, _ConsultationCryptoContext> _contexts = {};

  _ConsultationCryptoContext? _contextOf(String sessionId) {
    return _contexts[sessionId];
  }

  Future<_ConsultationCryptoContext> _ensureContext(String sessionId) async {
    final existing = _contexts[sessionId];
    if (existing != null) {
      return existing;
    }

    final e2e = E2EEncryption();
    final keyPair = await e2e.generateKeyPair();
    final created = _ConsultationCryptoContext(
      e2e: e2e,
      keyPair: keyPair,
    );
    _contexts[sessionId] = created;
    return created;
  }

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
      final context = await _ensureContext(sessionId);
      final myPublicKey = await context.e2e.exportPublicKey(context.keyPair);

      final resp = await exchangeKey(
        sessionId: sessionId,
        clientPublicKey: myPublicKey,
      );

      final data = resp['data'] as Map<String, dynamic>? ?? const {};
      final serverPublicKey = data['server_public_key']?.toString() ?? '';
      final salt = data['salt']?.toString() ?? '';
      if (serverPublicKey.isEmpty || salt.isEmpty) {
        return false;
      }

      await context.e2e.deriveSessionKeyFromSeed(serverPublicKey, salt);
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
    final context = _contextOf(sessionId);

    if (context?.e2e.isReady == true) {
      try {
        final encrypted = await context!.e2e.encryptEnvelope(content);
        final resp = await post('/consultation/message', data: {
          'session_id': sessionId,
          'encrypted': encrypted,
        });
        return toMap(resp);
      } catch (e) {
        if (kDebugMode) debugPrint('[ConsultationService] 加密失败，拒绝发送明文: $e');
        return {'success': false, 'message': '消息加密失败，请检查网络后重试'};
      }
    }

    // 咨询消息涉及隐私，E2E 未就绪时拒绝发送明文
    if (kDebugMode) debugPrint('[ConsultationService] E2E未就绪，拒绝发送明文');
    return {'success': false, 'message': '安全通道未建立，请稍后再试'};
  }

  /// 获取消息历史
  ///
  /// 自动解密加密消息，兼容两种格式：
  /// - 旧版：`encrypted=true` + `content` 为密文字符串
  /// - 新版：`encrypted` 为 `{ciphertext, iv, tag}` 信封对象
  /// 解密失败的消息内容会被替换为 `[无法解密]`。
  Future<Map<String, dynamic>> getMessages(
    String sessionId, {
    int page = 1,
    int pageSize = 50,
  }) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final resp = await get(
      '/consultation/messages/$sessionId',
      queryParameters: {
        'page': page,
        'page_size': pageSize,
      },
      useCache: false,
    );
    final result = toMap(resp);

    final rawData = result['data'];
    final envelope = rawData is Map<String, dynamic>
        ? Map<String, dynamic>.from(rawData)
        : <String, dynamic>{
            'messages': rawData is List ? rawData : const <dynamic>[],
          };
    final List<dynamic> messages = (envelope['messages'] ??
            envelope['items'] ??
            envelope['list']) as List? ??
        [];
    final context = _contextOf(sessionId);

    final normalizedMessages = await Future.wait(
      messages.map((item) async {
        if (item is! Map) return <String, dynamic>{};
        final msg = Map<String, dynamic>.from(item);
        final encryptedField = msg['encrypted'];
        try {
          if (context?.e2e.isReady == true && encryptedField is Map) {
            final env = Map<String, dynamic>.from(encryptedField);
            final cipher = env['ciphertext']?.toString() ?? '';
            final iv = env['iv']?.toString() ?? '';
            final tag = env['tag']?.toString() ?? '';
            if (cipher.isNotEmpty && iv.isNotEmpty && tag.isNotEmpty) {
              msg['content'] = await context!.e2e.decryptEnvelope(
                ciphertextBase64: cipher,
                ivBase64: iv,
                tagBase64: tag,
              );
              msg['decryption_status'] = 'ready';
            } else {
              msg['content'] = '[加密消息]';
              msg['decryption_status'] = 'invalid_payload';
            }
          } else if (encryptedField == true && msg['content'] is String) {
            msg['content'] = msg['content'];
            msg['decryption_status'] = 'legacy_payload';
          } else if (encryptedField is Map) {
            msg['content'] = '[加密消息]';
            msg['decryption_status'] = 'pending_key_exchange';
          } else {
            msg['decryption_status'] = 'plain';
          }
        } catch (e) {
          msg['content'] = '[无法解密]';
          msg['decryption_status'] = 'failed';
          if (kDebugMode) {
            debugPrint('[ConsultationService] 解密失败($sessionId): $e');
          }
        }
        msg['created_at'] = msg['created_at'] ?? msg['time'];
        msg['sender_type'] = msg['sender_type'] ?? msg['sender'];
        return msg;
      }),
    );

    result['data'] = {
      ...envelope,
      'messages': normalizedMessages,
      'items': normalizedMessages,
      'list': normalizedMessages,
    };

    return result;
  }

  /// 获取咨询历史会话列表
  Future<Map<String, dynamic>> getSessions({
    int page = 1,
    int pageSize = 20,
  }) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final resp = await get(
      '/consultation/sessions',
      queryParameters: {
        'page': page,
        'page_size': pageSize,
      },
      useCache: false,
    );
    return toMap(resp);
  }

  /// 密钥交换
  ///
  /// 将本地公钥发送到服务端，换取对端公钥用于 ECDH 派生。
  Future<Map<String, dynamic>> exchangeKey({
    required String sessionId,
    required String clientPublicKey,
  }) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    InputValidator.validateBase64(clientPublicKey, '公钥');
    final resp = await post('/consultation/key-exchange', data: {
      'session_id': sessionId,
      'client_public_key': clientPublicKey,
    });
    return toMap(resp);
  }

  /// E2E 加密通道是否已就绪
  bool get isE2EReady => _contexts.values.any((context) => context.e2e.isReady);

  /// 释放加密资源，清除密钥对
  void dispose() {
    for (final context in _contexts.values) {
      context.e2e.dispose();
    }
    _contexts.clear();
  }
}
