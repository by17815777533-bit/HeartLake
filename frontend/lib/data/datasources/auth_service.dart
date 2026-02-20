// @file auth_service.dart
// @brief 认证服务 - 符合中国用户习惯
// Created by 王璐瑶

import 'package:flutter/foundation.dart';
import 'base_service.dart';
import 'api_client.dart';
import '../../utils/storage_util.dart';
import 'package:uuid/uuid.dart';

class AuthService extends BaseService {
  @override
  String get serviceName => 'AuthService';

  final ApiClient _apiClient = ApiClient();
  final Uuid _uuid = const Uuid();

  static const int _maxLoginAttempts = 5;
  static const int _lockDurationMinutes = 15;
  static const int _codeCooldownSeconds = 60;

  Future<void> _saveAuthData({
    required String token,
    required String userId,
    String? username,
    String? nickname,
    String? email,
  }) async {
    _apiClient.setToken(token);
    _apiClient.setUserId(userId);
    await Future.wait([
      StorageUtil.saveUserId(userId),
      StorageUtil.saveToken(token),
      if (username != null) StorageUtil.saveUsername(username),
      if (nickname != null) StorageUtil.saveNickname(nickname),
    ]);
  }

  // 检查登录锁定
  Future<Map<String, dynamic>?> _checkLoginLock() async {
    final lockUntil = await StorageUtil.getLoginLockUntil();
    if (lockUntil != null && DateTime.now().isBefore(lockUntil)) {
      final minutes = lockUntil.difference(DateTime.now()).inMinutes + 1;
      return {'success': false, 'message': '登录失败次数过多，请$minutes分钟后再试'};
    }
    return null;
  }

  Future<void> _handleLoginFailure() async {
    final count = await StorageUtil.getLoginFailCount() + 1;
    await StorageUtil.setLoginFailCount(count);
    if (count >= _maxLoginAttempts) {
      await StorageUtil.setLoginLockUntil(DateTime.now().add(const Duration(minutes: _lockDurationMinutes)));
    }
  }

  // 邮箱登录
  Future<Map<String, dynamic>> login({required String email, required String password}) async {
    final lockError = await _checkLoginLock();
    if (lockError != null) return lockError;

    final response = await post('/auth/login/email', data: {'email': email, 'password': password});
    if (!response.success) {
      await _handleLoginFailure();
      final count = await StorageUtil.getLoginFailCount();
      final remaining = _maxLoginAttempts - count;
      return {'success': false, 'message': response.message ?? '登录失败', 'remaining_attempts': remaining > 0 ? remaining : 0};
    }

    await StorageUtil.clearLoginFailCount();
    final data = response.data;
    await _saveAuthData(token: data['token'], userId: data['user_id'], username: data['username'], nickname: data['nickname'], email: email);
    return {'success': true, 'user_id': data['user_id'], 'nickname': data['nickname']};
  }

  // 账号密码登录
  Future<Map<String, dynamic>> loginWithUsername({required String username, required String password}) async {
    final lockError = await _checkLoginLock();
    if (lockError != null) return lockError;

    final response = await post('/auth/login', data: {'username': username, 'password': password});
    if (!response.success) {
      await _handleLoginFailure();
      final count = await StorageUtil.getLoginFailCount();
      final remaining = _maxLoginAttempts - count;
      return {'success': false, 'message': response.message ?? '登录失败', 'remaining_attempts': remaining > 0 ? remaining : 0};
    }

    await StorageUtil.clearLoginFailCount();
    final data = response.data;
    await _saveAuthData(token: data['token'], userId: data['user_id'], username: data['username'], nickname: data['nickname']);
    return {'success': true, 'user_id': data['user_id'], 'username': data['username'], 'nickname': data['nickname']};
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

    final data = response.data;
    await _saveAuthData(token: data['token'], userId: data['user_id'], username: data['username'] ?? data['user_id'], nickname: data['nickname']);
    return {'success': true, 'user_id': data['user_id'], 'nickname': data['nickname'], 'is_anonymous': true};
  }

  // 邮箱注册
  Future<Map<String, dynamic>> registerWithEmail({
    required String email,
    required String password,
    required String verificationCode,
    String? nickname,
  }) async {
    final response = await post('/auth/register/email', data: {
      'email': email,
      'password': password,
      'verification_code': verificationCode,
      if (nickname != null) 'nickname': nickname,
    });
    if (!response.success) return toMap(response);

    final data = response.data;
    await _saveAuthData(token: data['token'], userId: data['user_id'], username: data['username'], nickname: data['nickname'], email: email);
    return {'success': true, 'user_id': data['user_id'], 'username': data['username'], 'nickname': data['nickname']};
  }

  // 发送邮箱验证码
  Future<Map<String, dynamic>> sendEmailVerificationCode(String email) async {
    final lastSent = await StorageUtil.getCodeSentTime('email');
    if (lastSent != null) {
      final elapsed = DateTime.now().difference(lastSent).inSeconds;
      if (elapsed < _codeCooldownSeconds) {
        return {'success': false, 'message': '请${_codeCooldownSeconds - elapsed}秒后再试', 'cooldown': _codeCooldownSeconds - elapsed};
      }
    }
    final response = await post('/auth/email/verification-code', data: {'email': email});
    if (response.success) await StorageUtil.setCodeSentTime('email');
    return toMap(response);
  }

  // 找回密码
  Future<Map<String, dynamic>> resetPassword({required String email, required String code, required String newPassword}) async {
    final response = await post('/auth/reset-password', data: {'email': email, 'verification_code': code, 'new_password': newPassword});
    return toMap(response);
  }

  // 简单注册（开发模式）
  Future<Map<String, dynamic>> registerWithUsername({required String password, String? nickname}) async {
    final codeResult = await sendVerificationCode();
    if (!codeResult['success']) return codeResult;

    final response = await post('/auth/register', data: {
      'password': password,
      'verification_code': codeResult['code'],
      if (nickname != null && nickname.isNotEmpty) 'nickname': nickname,
    });
    if (!response.success) return toMap(response);

    final data = response.data;
    await _saveAuthData(token: data['token'], userId: data['user_id'], username: data['username'], nickname: data['nickname']);
    return {'success': true, 'user_id': data['user_id'], 'username': data['username'], 'nickname': data['nickname']};
  }

  Future<Map<String, dynamic>> sendVerificationCode() async {
    final response = await post('/auth/verification-code', data: {});
    if (!response.success) return toMap(response);
    final result = <String, dynamic>{'success': true};
    if (kDebugMode && response.data['code'] != null) {
      result['code'] = response.data['code'];
    }
    return result;
  }

  Future<Map<String, dynamic>> register({required String password, String? nickname, required String verificationCode}) async {
    final response = await post('/auth/register', data: {
      'password': password,
      'verification_code': verificationCode,
      if (nickname != null) 'nickname': nickname,
    });
    if (!response.success) return toMap(response);

    final data = response.data;
    await _saveAuthData(token: data['token'], userId: data['user_id'], username: data['username'], nickname: data['nickname']);
    return {'success': true, 'user_id': data['user_id'], 'username': data['username'], 'nickname': data['nickname']};
  }

  Future<bool> isLoggedIn() async {
    final token = await StorageUtil.getToken();
    return token != null;
  }

  Future<void> logout() async {
    _apiClient.clearToken();
    await StorageUtil.clearAll();
  }

  // 刷新Token
  Future<Map<String, dynamic>> refreshToken() async {
    final response = await post('/auth/refresh');
    if (!response.success) return toMap(response);

    final data = response.data;
    _apiClient.setToken(data['token']);
    await StorageUtil.saveToken(data['token']);
    return {'success': true};
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

  Future<Map<String, dynamic>> updateNickname(String nickname) async {
    final response = await put('/users/my/nickname', data: {'nickname': nickname});
    if (!response.success) return toMap(response);
    return {'success': true, 'nickname': response.data['nickname']};
  }

  // 修改密码
  Future<Map<String, dynamic>> changePassword({required String oldPassword, required String newPassword}) async {
    final response = await post('/auth/change-password', data: {'old_password': oldPassword, 'new_password': newPassword});
    return toMap(response);
  }

  // 注销账号
  Future<Map<String, dynamic>> deleteAccount({required String password, required String confirmation}) async {
    final response = await post('/auth/delete-account', data: {'password': password, 'confirmation': confirmation});
    if (!response.success) return toMap(response);
    await logout();
    return {'success': true, 'message': '账号已注销'};
  }

  // 绑定邮箱（游客升级）
  Future<Map<String, dynamic>> bindEmail({required String email, required String code}) async {
    final response = await post('/account/bind-email', data: {'email': email, 'verification_code': code});
    return toMap(response);
  }
}
