// @file guardian_service.dart
// @brief 守望者服务 - 灯火转赠与激励系统

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
    InputValidator.requireNonEmpty(toUserId, '目标用户ID');
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
  Future<Map<String, dynamic>> chat(Map<String, dynamic> data) async {
    final response = await post('/guardian/chat', data: data);
    return toMap(response);
  }
}
