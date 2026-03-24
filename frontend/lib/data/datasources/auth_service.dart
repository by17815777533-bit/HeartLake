// 认证服务
//
// 提供匿名登录和账号恢复功能，支持设备ID绑定和恢复密钥机制。
// 所有认证数据会同步到本地存储和内存缓存。

import 'dart:async';

import '../../utils/input_validator.dart';
import 'base_service.dart';
import 'api_client.dart';
import 'user_service.dart' show UserPayloadNormalizer;
import 'package:uuid/uuid.dart';
import '../../utils/storage_util.dart';

class StoredAuthSession {
  final String userId;
  final String token;
  final String? sessionId;
  final String? nickname;
  final bool? isAnonymous;

  const StoredAuthSession({
    required this.userId,
    required this.token,
    this.sessionId,
    this.nickname,
    this.isAnonymous,
  });
}

abstract class AuthDataSource {
  Stream<StoredAuthSession?> get authStateChanges;
  Future<Map<String, dynamic>> anonymousLogin();
  Future<StoredAuthSession?> getStoredSession();
  Future<Map<String, dynamic>> getUserProfile(String userId);
  Future<void> logout();
}

/// 认证服务
///
/// 提供匿名登录和账号恢复功能。
class AuthService extends BaseService implements AuthDataSource {
  final StreamController<StoredAuthSession?> _authStateController =
      StreamController<StoredAuthSession?>.broadcast();

  @override
  String get serviceName => 'AuthService';

  @override
  Stream<StoredAuthSession?> get authStateChanges =>
      _authStateController.stream;

  /// 保存认证数据
  ///
  /// 将认证数据同步写入ApiClient内存态和本地持久化存储。
  ///
  /// [token] 访问令牌
  /// [userId] 用户ID
  /// [nickname] 用户昵称（可选）
  Future<void> _saveAuthData({
    required String token,
    required String userId,
    String? refreshToken,
    String? sessionId,
    String? nickname,
    bool? isAnonymous,
  }) async {
    await Future.wait([
      ApiClient().setToken(
        token,
        refreshToken: refreshToken,
        userId: userId,
        sessionId: sessionId,
      ),
      if (nickname != null) StorageUtil.saveNickname(nickname),
      if (isAnonymous != null) StorageUtil.saveIsAnonymous(isAnonymous),
    ]);
    _authStateController.add(
      StoredAuthSession(
        userId: userId,
        token: token,
        sessionId: sessionId,
        nickname: nickname,
        isAnonymous: isAnonymous,
      ),
    );
  }

  bool? _parseOptionalBool(dynamic value) {
    if (value is bool) return value;
    if (value is num) return value != 0;
    if (value is String) {
      final normalized = value.trim().toLowerCase();
      if (normalized == 'true' || normalized == '1' || normalized == 'yes') {
        return true;
      }
      if (normalized == 'false' || normalized == '0' || normalized == 'no') {
        return false;
      }
    }
    return null;
  }

  /// 匿名登录
  ///
  /// 首次使用时自动生成设备ID并持久化到本地存储。
  ///
  /// 返回包含token、用户ID和恢复密钥的Map：
  /// - success: 是否成功
  /// - user_id: 用户ID
  /// - nickname: 用户昵称
  /// - recovery_key: 恢复密钥
  /// - is_new_user: 是否新用户
  @override
  Future<Map<String, dynamic>> anonymousLogin() async {
    String? deviceId = await StorageUtil.getDeviceId();
    if (deviceId == null) {
      deviceId = const Uuid().v4();
      await StorageUtil.saveDeviceId(deviceId);
    }

    final response =
        await post('/auth/anonymous', data: {'device_id': deviceId});
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
      refreshToken: data['refresh_token'],
      sessionId: data['session_id'],
      nickname: data['nickname'],
      isAnonymous: _parseOptionalBool(data['is_anonymous']),
    );
    return {
      'success': true,
      'user_id': userId,
      'nickname': data['nickname'],
      'recovery_key': data['recovery_key'],
      'session_id': data['session_id'],
      'refresh_token': data['refresh_token'],
      'refresh_expires_at': data['refresh_expires_at'],
      'is_new_user': data['is_new_user'] == true,
    };
  }

  /// 通过恢复密钥找回账号
  ///
  /// [recoveryKey] 恢复密钥，长度8-512字符
  ///
  /// 返回包含token和用户信息的Map。
  Future<Map<String, dynamic>> recoverWithKey(String recoveryKey) async {
    InputValidator.requireLength(recoveryKey, '恢复密钥', min: 8, max: 512);
    String? deviceId = await StorageUtil.getDeviceId();
    if (deviceId == null) {
      deviceId = const Uuid().v4();
      await StorageUtil.saveDeviceId(deviceId);
    }

    final response = await post('/auth/recover', data: {
      'recovery_key': recoveryKey,
      'device_id': deviceId,
    });
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
      refreshToken: data['refresh_token'],
      sessionId: data['session_id'],
      nickname: data['nickname'],
      isAnonymous: _parseOptionalBool(data['is_anonymous']),
    );
    return {
      'success': true,
      'user_id': userId,
      'nickname': data['nickname'],
      'session_id': data['session_id'],
      'refresh_token': data['refresh_token'],
      'refresh_expires_at': data['refresh_expires_at'],
      'is_new_user': data['is_new_user'] == true,
    };
  }

  /// 检查是否已登录
  ///
  /// 检查本地是否存有有效token。
  Future<bool> isLoggedIn() async {
    final token = await StorageUtil.getToken();
    return token != null;
  }

  /// 退出登录
  ///
  /// 清除所有本地凭证并标记为主动退出，下次启动进入登录页。
  @override
  Future<void> logout() async {
    await ApiClient().clearToken();
    await StorageUtil.clearAll();
    // 标记主动退出，下次启动进入登录页而非自动匿名登录
    await StorageUtil.saveString('manual_logout', 'true');
    _authStateController.add(null);
  }

  /// 刷新访问令牌
  ///
  /// 使用refresh_token刷新访问令牌。
  Future<Map<String, dynamic>> refreshToken({String? refreshToken}) async {
    final refreshTk = refreshToken ?? await StorageUtil.getRefreshToken();
    final payload = (refreshTk != null && refreshTk.isNotEmpty)
        ? {'refresh_token': refreshTk}
        : null;

    final response = await post('/auth/refresh', data: payload);
    if (!response.success) return toMap(response);

    final data = response.data;
    if (data != null && data['token'] != null) {
      final resolvedUserId =
          (data['user_id'] as String?) ?? await StorageUtil.getUserId() ?? '';
      await _saveAuthData(
        token: data['token'],
        userId: resolvedUserId,
        refreshToken: data['refresh_token'] ?? refreshTk,
        sessionId: data['session_id'] as String?,
        isAnonymous: await StorageUtil.getIsAnonymous(),
      );
    }
    return {
      'success': true,
      'token': data?['token'],
      'refresh_token': data?['refresh_token'] ?? refreshTk,
      'refresh_expires_at': data?['refresh_expires_at'],
      'user_id': data?['user_id'],
      'session_id': data?['session_id'],
      'expires_at': data?['expires_at'],
    };
  }

  Future<String?> refreshAccessToken(String? refreshTokenValue) async {
    final result = await refreshToken(refreshToken: refreshTokenValue);
    if (result['success'] == true) {
      return result['token'] as String?;
    }
    return null;
  }

  @override
  Future<StoredAuthSession?> getStoredSession() async {
    final userId = await StorageUtil.getUserId();
    final token = await StorageUtil.getToken();
    if (userId == null || token == null) {
      return null;
    }
    return StoredAuthSession(
      userId: userId,
      token: token,
      sessionId: await StorageUtil.getSessionId(),
      nickname: await StorageUtil.getNickname(),
      isAnonymous: await StorageUtil.getIsAnonymous(),
    );
  }

  @override
  Future<Map<String, dynamic>> getUserProfile(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final currentUserId = await StorageUtil.getUserId();
    final response = currentUserId == userId
        ? await get('/account/info')
        : await get('/users/$userId');
    if (!response.success) {
      return toMap(response);
    }
    final user = UserPayloadNormalizer.normalizeUser(response.data);
    if (user == null) {
      return {'success': false, 'message': '服务器返回数据不完整'};
    }
    if (currentUserId == userId) {
      user['user_id'] = user['user_id'] ?? currentUserId;
      user['userId'] = user['userId'] ?? currentUserId;

      final storedIsAnonymous = await StorageUtil.getIsAnonymous();
      if (!user.containsKey('is_anonymous') && storedIsAnonymous != null) {
        user['is_anonymous'] = storedIsAnonymous;
        user['isAnonymous'] = storedIsAnonymous;
      }
    }
    return {
      'success': true,
      'user': user,
    };
  }
}
