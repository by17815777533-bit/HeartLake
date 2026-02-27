// VIP服务 - 会员功能管理

import 'base_service.dart';
import '../../utils/input_validator.dart';

class VIPService extends BaseService {
  @override
  String get serviceName => 'VIPService';

  /// 获取用户VIP状态
  Future<Map<String, dynamic>> getVIPStatus() async {
    final response = await get('/vip/status');
    return toMap(response);
  }

  /// 获取VIP权益列表
  Future<Map<String, dynamic>> getPrivileges() async {
    final response = await get('/vip/privileges');
    return toMap(response);
  }

  /// 检查是否有免费心理咨询额度
  Future<bool> hasFreeCounselingQuota() async {
    final response = await get('/vip/counseling/check');
    if (response.success) {
      return response.data?['has_quota'] ?? false;
    }
    return false;
  }

  /// 预约心理咨询
  Future<Map<String, dynamic>> bookCounseling({
    required String appointmentTime,
    bool isFreeVIP = false,
  }) async {
    InputValidator.validateFutureISO8601(appointmentTime, '预约时间');
    final response = await post('/vip/counseling/book', data: {
      'appointment_time': appointmentTime,
      'is_free_vip': isFreeVIP,
    });
    return toMap(response);
  }

  /// 获取AI评论频率（小时）
  Future<double> getAICommentFrequency() async {
    final response = await get('/vip/ai-comment-frequency');
    if (response.success) {
      return (response.data?['frequency_hours'] ?? 2.0).toDouble();
    }
    return 2.0;
  }

  /// 检查用户是否是VIP
  Future<bool> isVIP() async {
    final result = await getVIPStatus();
    if (result['success'] == true) {
      final data = result['data'];
      return (data is Map) ? (data['is_vip'] ?? false) : false;
    }
    return false;
  }

  /// 获取VIP剩余天数
  Future<int> getVIPDaysLeft() async {
    final result = await getVIPStatus();
    if (result['success'] == true) {
      final data = result['data'];
      return (data is Map) ? (data['days_left'] ?? 0) : 0;
    }
    return 0;
  }
}
