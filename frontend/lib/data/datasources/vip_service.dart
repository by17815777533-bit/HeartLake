// @file vip_service.dart
// @brief VIP服务 - 会员功能管理
// Created by 王璐瑶

import 'base_service.dart';

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
    try {
      final response = await get('/vip/counseling/check');
      if (response.success) {
        return response.data?['has_quota'] ?? false;
      }
      return false;
    } catch (e) {
      return false;
    }
  }

  /// 预约心理咨询
  Future<Map<String, dynamic>> bookCounseling({
    required String appointmentTime,
    bool isFreeVIP = false,
  }) async {
    final response = await post('/vip/counseling/book', data: {
      'appointment_time': appointmentTime,
      'is_free_vip': isFreeVIP,
    });
    return toMap(response);
  }

  /// 获取AI评论频率（小时）
  Future<double> getAICommentFrequency() async {
    try {
      final response = await get('/vip/ai-comment-frequency');
      if (response.success) {
        return (response.data?['frequency_hours'] ?? 2.0).toDouble();
      }
      return 2.0;
    } catch (e) {
      return 2.0;
    }
  }

  /// 检查用户是否是VIP
  Future<bool> isVIP() async {
    try {
      final response = await get('/vip/status');
      if (response.success) {
        return response.data?['is_vip'] ?? false;
      }
      return false;
    } catch (e) {
      return false;
    }
  }

  /// 获取VIP剩余天数
  Future<int> getVIPDaysLeft() async {
    try {
      final response = await get('/vip/status');
      if (response.success) {
        return response.data?['days_left'] ?? 0;
      }
      return 0;
    } catch (e) {
      return 0;
    }
  }
}
