// @file user_service.dart
// @brief 用户服务 - 用户信息查询和管理
// Created by 王璐瑶

import 'base_service.dart';
import '../../utils/input_validator.dart';
class UserService extends BaseService {
  @override
  String get serviceName => 'UserService';

  /// 搜索用户
  Future<Map<String, dynamic>> searchUsers(String query) async {
    InputValidator.requireLength(query, '搜索关键词', min: 1, max: 50);
    final response = await get('/users/search', queryParameters: {'q': query});

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'users': response.data?['users'] ?? [],
      'total': response.data?['total'] ?? 0,
    };
  }

  /// 获取用户信息
  Future<Map<String, dynamic>> getUserInfo(String userId) async {
    InputValidator.requireNonEmpty(userId, '用户ID');
    final response = await get('/users/$userId');

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'user': response.data,
    };
  }

  /// 获取用户统计
  Future<Map<String, dynamic>> getUserStats(String userId) async {
    InputValidator.requireNonEmpty(userId, '用户ID');
    final response = await get('/users/$userId/stats');
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 获取情绪热力图
  Future<Map<String, dynamic>> getEmotionHeatmap() async {
    final response = await get('/users/my/emotion-heatmap');
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 获取情绪日历
  Future<Map<String, dynamic>> getEmotionCalendar(int year, int month) async {
    InputValidator.requireYear(year);
    InputValidator.requireMonth(month);
    final response = await get('/users/my/emotion-calendar', queryParameters: {'year': year, 'month': month});
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 获取我收到的纸船
  Future<Map<String, dynamic>> getMyBoats({int page = 1, int pageSize = 100}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/users/my/boats', queryParameters: {'page': page, 'page_size': pageSize});
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 上传文件（头像等）
  Future<Map<String, dynamic>> uploadFile(dynamic file) async {
    try {
      final response = await client.uploadFile('/media/upload', file: file);
      if (response.statusCode == 200 && response.data['code'] == 0) {
        return {'success': true, 'data': response.data['data']};
      }
      return {'success': false, 'message': response.data['message'] ?? '上传失败'};
    } catch (e) {
      return {'success': false, 'message': '上传失败: $e'};
    }
  }

}
