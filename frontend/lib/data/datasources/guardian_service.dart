// 守望者服务 - 灯火转赠与激励系统

import '../../utils/input_validator.dart';
import 'base_service.dart';

class GuardianService extends BaseService {
  @override
  String get serviceName => 'GuardianService';

  Future<Map<String, dynamic>> getStats() async {
    final response = await get('/guardian/stats');
    return toMap(response);
  }

  Future<Map<String, dynamic>> transferLamp(String toUserId) async {
    InputValidator.validateUUID(toUserId, '目标用户ID');
    final response = await post('/guardian/transfer-lamp', data: {
      'to_user_id': toUserId,
    });
    return toMap(response);
  }

  Future<Map<String, dynamic>> getEmotionInsights() async {
    final response = await get('/guardian/insights');
    return toMap(response);
  }

  /// 守望者对话
  Future<Map<String, dynamic>> chat(String content) async {
    InputValidator.requireLength(content, '对话内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    final response = await post('/guardian/chat', data: {
      'content': content,
    });
    return toMap(response);
  }
}
