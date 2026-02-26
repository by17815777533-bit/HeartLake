// @file auth_service.dart
// @brief 认证服务 - 匿名登录 + 关键词恢复
// Created by 王璐瑶

import '../../utils/input_validator.dart';
import 'base_service.dart';
import 'api_client.dart';
import 'package:uuid/uuid.dart';
import '../../utils/storage_util.dart';

class AuthService extends BaseService {
  @override
  String get serviceName => 'AuthService';

  Future<void> _saveAuthData({
    required String token,
    required String userId,
    String? nickname,
  }) async {
    ApiClient().setToken(token);
    ApiClient().setUserId(userId);
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
      deviceId = const Uuid().v4();
      await StorageUtil.saveDeviceId(deviceId);
    }

    final response = await post('/auth/anonymous', data: {'device_id': deviceId});
    if (!response.success) return toMap(response);

    final data = response.data as Map<String, dynamic>? ?? {};
    final token = data['token'];
    final userId = data['user_id'];
    if (token == null || userId == null) {
      return {'success': false, 'message': '服务器返回数据不完整'};
    }
    await _saveAuthData(token: token, userId: userId, nickname: data['nickname']);
    return {
      'success': true,
      'user_id': userId,
      'nickname': data['nickname'],
      'recovery_key': data['recovery_key'],
    };
  }

  // 关键词恢复账号
  Future<Map<String, dynamic>> recoverWithKey(String recoveryKey) async {
    InputValidator.requireLength(recoveryKey, '恢复密钥', min: 8, max: 512);
    final response = await post('/auth/recover', data: {'recovery_key': recoveryKey});
    if (!response.success) return toMap(response);

    final data = response.data as Map<String, dynamic>? ?? {};
    final token = data['token'];
    final userId = data['user_id'];
    if (token == null || userId == null) {
      return {'success': false, 'message': '服务器返回数据不完整'};
    }
    await _saveAuthData(
      token: token,
      userId: userId,
      nickname: data['nickname'],
    );
    return {'success': true, 'user_id': userId, 'nickname': data['nickname']};
  }

  Future<bool> isLoggedIn() async {
    final token = await StorageUtil.getToken();
    return token != null;
  }

  Future<void> logout() async {
    ApiClient().clearToken();
    await StorageUtil.clearAll();
  }

  // 更新个人资料
  Future<Map<String, dynamic>> updateProfile({String? avatarUrl, String? bio, String? nickname}) async {
    final Map<String, dynamic> data = {};
    if (avatarUrl != null) {
      InputValidator.requireLength(avatarUrl, '头像URL', max: 500);
      data['avatar_url'] = avatarUrl;
    }
    if (bio != null) {
      InputValidator.requireLength(bio, '个人简介', max: 200);
      data['bio'] = bio;
    }
    if (nickname != null) {
      InputValidator.requireLength(nickname, '昵称', min: 2, max: 20);
      data['nickname'] = nickname;
    }
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
    final refreshTk = await StorageUtil.getRefreshToken();
    if (refreshTk == null) return {'success': false, 'code': 401};

    final response = await post('/auth/refresh', data: {'refresh_token': refreshTk});
    if (!response.success) return toMap(response);

    final data = response.data;
    if (data != null && data['token'] != null) {
      await _saveAuthData(token: data['token'], userId: data['user_id'] ?? await StorageUtil.getUserId() ?? '');
    }
    return {'success': true};
  }

  Future<Map<String, dynamic>> updateNickname(String nickname) async {
    InputValidator.requireLength(nickname, '昵称', min: 2, max: 20);
    final response = await put('/users/my/nickname', data: {'nickname': nickname});
    if (!response.success) return toMap(response);
    return {'success': true, 'nickname': response.data['nickname']};
  }
}
