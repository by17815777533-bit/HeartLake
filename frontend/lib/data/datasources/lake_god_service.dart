// @file lake_god_service.dart
// @brief 湖神咨询服务
import 'base_service.dart';

class LakeGodService extends BaseService {
  @override
  String get serviceName => 'LakeGodService';

  static const String _lakeGodId = 'lake_god_ai';
  String? _sessionId;

  Future<Map<String, dynamic>> startSession() async {
    final resp = await post('/consultation/session', data: {'counselor_id': _lakeGodId});
    if (resp.success && resp.data != null) {
      _sessionId = resp.data['session_id'];
    }
    return toMap(resp);
  }

  Future<Map<String, dynamic>> sendMessage(String content) async {
    if (_sessionId == null) {
      final start = await startSession();
      if (start['success'] != true) return start;
    }
    final resp = await post('/consultation/message', data: {
      'session_id': _sessionId,
      'content': content,
    });
    return toMap(resp);
  }

  Future<Map<String, dynamic>> getMessages() async {
    if (_sessionId == null) return {'success': true, 'messages': []};
    final resp = await get('/consultation/messages/$_sessionId');
    return toMap(resp);
  }
}
