// @file auth_service.dart
// @brief 认证服务 - 匿名登录 + 关键词恢复
// Created by 王璐瑶

import 'base_service.dart';
import 'api_client.dart';
import '../../utils/storage_util.dart';
import 'package:uuid/uuid.dart';

class AuthService extends BaseService {
  @override
  String get serviceName => 'AuthService';

  final ApiClient _apiClient = ApiClient();
  final Uuid _uuid = const Uuid();

  Future<void> _saveAuthData({
    required String token,
    required String userId,
    String? nickname,
  }) async {
    _apiClient.setToken(token);
    _apiClient.setUserId(userId);
    await Future.wait([
      StorageUtil.saveUserId(userId),
      StorageUtil.saveToken(token),
      if (nickname != null) StorageUtil.saveNickname(nickname),
    ]);
  }

  // 匿名登录（游客模式）
  Future<Map<String, dynamic>> anonymousLogin() async {
    String? deviceId = await StorageUtil.getDeviceId();
    if (deviceId == null) {
      deviceId = _uuid.v4();
      await StorageUtil.saveDeviceId(deviceId);
    }

    final response = await post('/auth/anonymous', data: {'device_id': deviceId});
    if (!response.success) return toMap(response);

    final data = response.data as Map<String, dynamic>? ?? {};
    await _saveAuthData(token: data['token'], userId: data['user_id'], nickname: data['nickname']);
    return {
      'success': true,
      'user_id': data['user_id'],
      'nickname': data['nickname'],
      'recovery_key': data['recovery_key'],
    };
  }

  // 关键词恢复账号
  Future<Map<String, dynamic>> recoverWithKey(String recoveryKey) async {
    final response = await post('/auth/recover', data: {'recovery_key': recoveryKey});
    if (!response.success) return toMap(response);

    final data = response.data as Map<String, dynamic>? ?? {};
    await _saveAuthData(
      token: data['token'],
      userId: data['user_id'],
      nickname: data['nickname'],
    );
    return {'success': true, 'user_id': data['user_id'], 'nickname': data['nickname']};
  }

  Future<bool> isLoggedIn() async {
    final token = await StorageUtil.getToken();
    return token != null;
  }

  Future<void> logout() async {
    _apiClient.clearToken();
    await StorageUtil.clearAll();
  }

  // 更新个人资料
  Future<Map<String, dynamic>> updateProfile({String? avatarUrl, String? bio, String? nickname}) async {
    final Map<String, dynamic> data = {};
    if (avatarUrl != null) data['avatar_url'] = avatarUrl;
    if (bio != null) data['bio'] = bio;
    if (nickname != null) data['nickname'] = nickname;
    if (data.isEmpty) return {'success': true};

    final response = await put('/users/my/profile', data: data);
    if (!response.success) return toMap(response);

    final responseData = response.data;
    if (responseData['nickname'] != null) {
      await StorageUtil.saveNickname(responseData['nickname']);
    }
    return {'success': true, 'user_id': responseData['user_id'], 'nickname': responseData['nickname'], 'avatar_url': responseData['avatar_url'], 'bio': responseData['bio']};
  }

  // Token 刷新
  Future<Map<String, dynamic>> refreshToken() async {
    final token = await StorageUtil.getToken();
    if (token == null) return {'success': false, 'code': 401};

    final response = await post('/auth/refresh', data: {'token': token});
    if (!response.success) return toMap(response);

    final data = response.data;
    if (data != null && data['token'] != null) {
      await _saveAuthData(token: data['token'], userId: data['user_id'] ?? await StorageUtil.getUserId() ?? '');
    }
    return {'success': true};
  }

  Future<Map<String, dynamic>> updateNickname(String nickname) async {
    final response = await put('/users/my/nickname', data: {'nickname': nickname});
    if (!response.success) return toMap(response);
    return {'success': true, 'nickname': response.data['nickname']};
  }
}
