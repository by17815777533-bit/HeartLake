// @file user_service.dart
// @brief 用户服务 - 用户信息查询和管理
// Created by 王璐瑶

import 'base_service.dart';
class UserService extends BaseService {
  @override
  String get serviceName => 'UserService';

  /// 搜索用户
  Future<Map<String, dynamic>> searchUsers(String query) async {
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
    final response = await get('/users/$userId');

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'user': response.data,
    };
  }

  /// 获取用户信息 (别名)
  Future<Map<String, dynamic>> getUserProfile(String userId) async {
    return getUserInfo(userId);
  }

  /// 获取当前用户信息
  Future<Map<String, dynamic>> getCurrentUser() async {
    final response = await get('/account/info');

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'user': response.data,
    };
  }

  /// 更新用户资料
  Future<Map<String, dynamic>> updateProfile({
    String? nickname,
    String? bio,
    String? avatarUrl,
  }) async {
    final data = <String, dynamic>{};
    if (nickname != null) data['nickname'] = nickname;
    if (bio != null) data['bio'] = bio;
    if (avatarUrl != null) data['avatar_url'] = avatarUrl;

    final response = await put('/account/profile', data: data);
    return toMap(response);
  }
}
