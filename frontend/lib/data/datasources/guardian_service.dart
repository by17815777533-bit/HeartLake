// @file guardian_service.dart
// @brief 守望者服务 - 灯火转赠与激励系统

import 'base_service.dart';

class GuardianService extends BaseService {
  @override
  String get serviceName => 'GuardianService';

  Future<Map<String, dynamic>> getStats() async {
    final response = await get('/guardian/stats');
    return toMap(response);
  }

  Future<Map<String, dynamic>> transferLamp(String toUserId) async {
    final response = await post('/guardian/transfer-lamp', data: {
      'to_user_id': toUserId,
    });
    return toMap(response);
  }

  Future<Map<String, dynamic>> getEmotionInsights() async {
    final response = await get('/guardian/insights');
    return toMap(response);
  }

  /// 获取守望者信息
  Future<Map<String, dynamic>> getGuardianInfo() async {
    final response = await get('/guardian');
    return toMap(response);
  }

  /// 守望者对话
  Future<Map<String, dynamic>> chat(String content) async {
    final response = await post('/guardian/chat', data: {
      'content': content,
    });
    return toMap(response);
  }
}
