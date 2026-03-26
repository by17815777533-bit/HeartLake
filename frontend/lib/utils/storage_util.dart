// 本地持久化存储工具
//
// 分层管理敏感数据和普通数据。敏感数据（token、userId）使用
// FlutterSecureStorage加密存储，普通数据（昵称、设备ID、偏好设置）
// 使用SharedPreferences。
//
// Web平台下SecureStorage可能因浏览器限制失败，此时会尝试一次
// 修复并迁移历史明文数据；修复失败会显式抛出，而不是静默降级。

import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:flutter/foundation.dart';
import 'dart:convert';

class StorageOperationException implements Exception {
  final String operation;
  final Object cause;

  const StorageOperationException(this.operation, this.cause);

  @override
  String toString() => 'StorageOperationException($operation): $cause';
}

/// 本地存储工具
///
/// 提供统一的本地数据存储接口。
class StorageUtil {
  static const String _tokenKey = 'user_token';
  static const String _refreshTokenKey = 'refresh_token';
  static const String _userIdKey = 'user_id';
  static const String _sessionIdKey = 'session_id';
  static const String _deviceIdKey = 'device_id';
  static const String _usernameKey = 'username';
  static const String _nicknameKey = 'nickname';
  static const String _isAnonymousKey = 'is_anonymous';

  static const FlutterSecureStorage _secureStorage = FlutterSecureStorage();
  static SharedPreferences? _prefs;
  static bool _webSecureStorageRepairAttempted = false;

  static bool? _readBoolCompat(SharedPreferences prefs, String key) {
    final value = prefs.get(key);
    if (value is bool) return value;
    if (value is String) {
      final normalized = value.trim().toLowerCase();
      if (normalized == 'true') return true;
      if (normalized == 'false') return false;
    }
    return null;
  }

  /// 获取SharedPreferences实例
  ///
  /// 缓存实例，避免重复异步调用。
  static Future<SharedPreferences> get _instance async {
    _prefs ??= await SharedPreferences.getInstance();
    return _prefs!;
  }

  static Never _throwStorageFailure(
      String operation, Object error, StackTrace stackTrace) {
    Error.throwWithStackTrace(
      StorageOperationException(operation, error),
      stackTrace,
    );
  }

  static Future<void> _ensurePrefsMutation(
      Future<bool> mutation, String operation) async {
    final success = await mutation;
    if (!success) {
      throw StorageOperationException(
          operation, 'shared preferences returned false');
    }
  }

  static Future<void> _cleanupLegacyWebPlaintextKey(String key) async {
    if (!kIsWeb) return;
    final prefs = await _instance;
    if (prefs.containsKey(key)) {
      await prefs.remove(key);
    }
  }

  static Future<void> _repairWebSecureStorageIfNeeded({
    required String operation,
    required Object error,
  }) async {
    if (!kIsWeb || _webSecureStorageRepairAttempted) return;
    _webSecureStorageRepairAttempted = true;
    await _secureStorage.deleteAll();
  }

  static Future<T> _runSecureStorageOperation<T>({
    required String operation,
    required Future<T> Function() action,
  }) async {
    try {
      return await action();
    } catch (error) {
      try {
        await _repairWebSecureStorageIfNeeded(
          operation: operation,
          error: error,
        );
        return await action();
      } catch (retryError, retryStackTrace) {
        _throwStorageFailure(operation, retryError, retryStackTrace);
      }
    }
  }

  static Future<void> _secureWrite(String key, String value) async {
    await _runSecureStorageOperation<void>(
      operation: 'write:$key',
      action: () => _secureStorage.write(key: key, value: value),
    );
    await _cleanupLegacyWebPlaintextKey(key);
  }

  static Future<String?> _secureRead(String key) async {
    final value = await _runSecureStorageOperation<String?>(
      operation: 'read:$key',
      action: () => _secureStorage.read(key: key),
    );
    if (value != null && value.isNotEmpty) {
      await _cleanupLegacyWebPlaintextKey(key);
      return value;
    }
    if (kIsWeb) {
      final prefs = await _instance;
      final legacy = prefs.getString(key);
      if (legacy != null && legacy.isNotEmpty) {
        await _secureWrite(key, legacy);
        return legacy;
      }
    }
    return value;
  }

  static Future<void> _secureDelete(String key) async {
    await _runSecureStorageOperation<void>(
      operation: 'delete:$key',
      action: () => _secureStorage.delete(key: key),
    );
    await _cleanupLegacyWebPlaintextKey(key);
  }

  /// 保存Token
  ///
  /// 使用安全存储保存访问令牌。
  static Future<void> saveToken(String token) async {
    await _secureWrite(_tokenKey, token);
  }

  /// 获取Token
  ///
  /// 从安全存储读取访问令牌。
  static Future<String?> getToken() async {
    return await _secureRead(_tokenKey);
  }

  // 清除 Token (安全存储)
  static Future<void> clearToken() async {
    await _secureDelete(_tokenKey);
  }

  // 保存 RefreshToken (安全存储)
  static Future<void> saveRefreshToken(String token) async {
    await _secureWrite(_refreshTokenKey, token);
  }

  // 获取 RefreshToken (安全存储)
  static Future<String?> getRefreshToken() async {
    return await _secureRead(_refreshTokenKey);
  }

  // 清除 RefreshToken (安全存储)
  static Future<void> clearRefreshToken() async {
    await _secureDelete(_refreshTokenKey);
  }

  // 保存用户ID (安全存储)
  static Future<void> saveUserId(String userId) async {
    await _secureWrite(_userIdKey, userId);
  }

  // 获取用户ID (安全存储)
  static Future<String?> getUserId() async {
    return await _secureRead(_userIdKey);
  }

  // 保存会话ID (安全存储)
  static Future<void> saveSessionId(String sessionId) async {
    await _secureWrite(_sessionIdKey, sessionId);
  }

  // 获取会话ID (安全存储)
  static Future<String?> getSessionId() async {
    return await _secureRead(_sessionIdKey);
  }

  // 清除会话ID (安全存储)
  static Future<void> clearSessionId() async {
    await _secureDelete(_sessionIdKey);
  }

  // 保存用户名
  static Future<void> saveUsername(String username) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setString(_usernameKey, username),
      'setString:$_usernameKey',
    );
  }

  // 获取用户名
  static Future<String?> getUsername() async {
    final prefs = await _instance;
    return prefs.getString(_usernameKey);
  }

  // 保存昵称
  static Future<void> saveNickname(String nickname) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setString(_nicknameKey, nickname),
      'setString:$_nicknameKey',
    );
  }

  // 获取昵称
  static Future<String?> getNickname() async {
    final prefs = await _instance;
    return prefs.getString(_nicknameKey);
  }

  // 保存匿名状态
  static Future<void> saveIsAnonymous(bool isAnonymous) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setBool(_isAnonymousKey, isAnonymous),
      'setBool:$_isAnonymousKey',
    );
  }

  // 获取匿名状态
  static Future<bool?> getIsAnonymous() async {
    final prefs = await _instance;
    return _readBoolCompat(prefs, _isAnonymousKey);
  }

  // 保存设备ID
  static Future<void> saveDeviceId(String deviceId) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setString(_deviceIdKey, deviceId),
      'setString:$_deviceIdKey',
    );
  }

  // 获取设备ID
  static Future<String?> getDeviceId() async {
    final prefs = await _instance;
    return prefs.getString(_deviceIdKey);
  }

  // 保存对象
  static Future<void> saveObject(String key, dynamic object) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setString(key, jsonEncode(object)),
      'setString:$key',
    );
  }

  // 获取对象
  static Future<dynamic> getObject(String key) async {
    final prefs = await _instance;
    final jsonString = prefs.getString(key);
    if (jsonString == null) return null;
    return jsonDecode(jsonString);
  }

  /// 清除所有数据
  ///
  /// 清除所有存储数据，但保留引导页标记和设备ID。
  static Future<void> clearAll() async {
    final prefs = await _instance;
    final onboardingDoneBool = _readBoolCompat(prefs, 'onboarding_done');
    final deviceId = prefs.getString(_deviceIdKey);
    await _runSecureStorageOperation<void>(
      operation: 'deleteAll',
      action: () => _secureStorage.deleteAll(),
    );
    await _ensurePrefsMutation(prefs.clear(), 'clear preferences');
    if (onboardingDoneBool != null) {
      await _ensurePrefsMutation(
        prefs.setBool('onboarding_done', onboardingDoneBool),
        'setBool:onboarding_done',
      );
    }
    if (deviceId != null) {
      await _ensurePrefsMutation(
        prefs.setString(_deviceIdKey, deviceId),
        'setString:$_deviceIdKey',
      );
    }
  }

  // 通用字符串保存方法
  static Future<void> saveString(String key, String value) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setString(key, value),
      'setString:$key',
    );
  }

  // 通用字符串设置方法（别名）
  static Future<void> setString(String key, String value) async {
    await saveString(key, value);
  }

  // 通用字符串获取方法
  static Future<String?> getString(String key) async {
    final prefs = await _instance;
    return prefs.getString(key);
  }

  // 通用删除方法
  static Future<void> removeString(String key) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(prefs.remove(key), 'remove:$key');
  }

  // 删除 Token（别名）
  static Future<void> removeToken() async {
    await clearToken();
  }

  // 登录失败次数
  static Future<int> getLoginFailCount() async {
    final prefs = await _instance;
    return prefs.getInt('login_fail_count') ?? 0;
  }

  static Future<void> setLoginFailCount(int count) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setInt('login_fail_count', count),
      'setInt:login_fail_count',
    );
  }

  static Future<void> clearLoginFailCount() async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.remove('login_fail_count'),
      'remove:login_fail_count',
    );
    await _ensurePrefsMutation(
      prefs.remove('login_lock_until'),
      'remove:login_lock_until',
    );
  }

  // 登录锁定时间
  static Future<DateTime?> getLoginLockUntil() async {
    final prefs = await _instance;
    final ms = prefs.getInt('login_lock_until');
    return ms != null ? DateTime.fromMillisecondsSinceEpoch(ms) : null;
  }

  static Future<void> setLoginLockUntil(DateTime time) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setInt('login_lock_until', time.millisecondsSinceEpoch),
      'setInt:login_lock_until',
    );
  }

  // 验证码发送时间（按类型区分）
  static Future<DateTime?> getCodeSentTime(String type) async {
    final prefs = await _instance;
    final ms = prefs.getInt('code_sent_$type');
    return ms != null ? DateTime.fromMillisecondsSinceEpoch(ms) : null;
  }

  static Future<void> setCodeSentTime(String type) async {
    final prefs = await _instance;
    await _ensurePrefsMutation(
      prefs.setInt('code_sent_$type', DateTime.now().millisecondsSinceEpoch),
      'setInt:code_sent_$type',
    );
  }
}
