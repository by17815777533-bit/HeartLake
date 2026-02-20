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
}
