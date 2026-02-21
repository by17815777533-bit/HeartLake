// @file consultation_service.dart
// @brief 心理咨询服务 - 预约咨询和E2E加密会话
import 'base_service.dart';

class ConsultationService extends BaseService {
  @override
  String get serviceName => 'ConsultationService';

  /// 创建咨询会话
  Future<Map<String, dynamic>> createSession({String? counselorId}) async {
    final resp = await post('/consultation/session', data: {
      if (counselorId != null) 'counselor_id': counselorId,
    });
    return toMap(resp);
  }

  /// 发送消息
  Future<Map<String, dynamic>> sendMessage({
    required String sessionId,
    required String content,
  }) async {
    final resp = await post('/consultation/message', data: {
      'session_id': sessionId,
      'content': content,
    });
    return toMap(resp);
  }

  /// 获取消息历史
  Future<Map<String, dynamic>> getMessages(String sessionId) async {
    final resp = await get('/consultation/messages/$sessionId');
    return toMap(resp);
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
    final resp = await post('/consultation/key-exchange', data: {
      'session_id': sessionId,
      'public_key': publicKey,
    });
    return toMap(resp);
  }
}
