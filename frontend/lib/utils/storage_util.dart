// 本地存储工具类

import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:flutter/foundation.dart';
import 'dart:convert';

class StorageUtil {
  static const String _tokenKey = 'user_token';
  static const String _refreshTokenKey = 'refresh_token';
  static const String _userIdKey = 'user_id';
  static const String _deviceIdKey = 'device_id';
  static const String _usernameKey = 'username';
  static const String _nicknameKey = 'nickname';

  static const FlutterSecureStorage _secureStorage = FlutterSecureStorage();
  static SharedPreferences? _prefs;
  static bool _webSecureStorageRepairAttempted = false;
  static String? _runtimeToken;
  static String? _runtimeRefreshToken;
  static String? _runtimeUserId;

  /// 获取缓存的SharedPreferences实例，避免重复异步调用
  static Future<SharedPreferences> get _instance async {
    _prefs ??= await SharedPreferences.getInstance();
    return _prefs!;
  }

  static void _setRuntimeByKey(String key, String value) {
    if (key == _tokenKey) _runtimeToken = value;
    if (key == _refreshTokenKey) _runtimeRefreshToken = value;
    if (key == _userIdKey) _runtimeUserId = value;
  }

  static void _clearRuntimeByKey(String key) {
    if (key == _tokenKey) _runtimeToken = null;
    if (key == _refreshTokenKey) _runtimeRefreshToken = null;
    if (key == _userIdKey) _runtimeUserId = null;
  }

  static String? _runtimeByKey(String key) {
    if (key == _tokenKey) return _runtimeToken;
    if (key == _refreshTokenKey) return _runtimeRefreshToken;
    if (key == _userIdKey) return _runtimeUserId;
    return null;
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
    if (kDebugMode) {
      debugPrint('secure storage repair triggered [$operation]: $error');
    }
    try {
      await _secureStorage.deleteAll();
      if (kDebugMode) {
        debugPrint('secure storage repair finished');
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('secure storage repair failed: $e');
      }
    }
  }

  static Future<void> _secureWrite(String key, String value) async {
    _setRuntimeByKey(key, value);
    try {
      await _secureStorage.write(key: key, value: value);
      await _cleanupLegacyWebPlaintextKey(key);
    } catch (e) {
      await _repairWebSecureStorageIfNeeded(operation: 'write:$key', error: e);
      try {
        await _secureStorage.write(key: key, value: value);
        await _cleanupLegacyWebPlaintextKey(key);
      } catch (e2) {
        if (kDebugMode) {
          debugPrint('secure write failed [$key]: $e2');
        }
      }
    }
  }

  static Future<String?> _secureRead(String key) async {
    try {
      final value = await _secureStorage.read(key: key);
      if (value != null && value.isNotEmpty) {
        _setRuntimeByKey(key, value);
      }
      await _cleanupLegacyWebPlaintextKey(key);
      return value ?? _runtimeByKey(key);
    } catch (e) {
      await _repairWebSecureStorageIfNeeded(operation: 'read:$key', error: e);
      try {
        final retryValue = await _secureStorage.read(key: key);
        if (retryValue != null && retryValue.isNotEmpty) {
          _setRuntimeByKey(key, retryValue);
        }
        await _cleanupLegacyWebPlaintextKey(key);
        return retryValue ?? _runtimeByKey(key);
      } catch (e2) {
        if (kDebugMode) {
          debugPrint('secure read failed [$key]: $e2');
        }
        if (kIsWeb) {
          final prefs = await _instance;
          final legacy = prefs.getString(key);
          if (legacy != null && legacy.isNotEmpty) {
            try {
              await _secureStorage.write(key: key, value: legacy);
              await prefs.remove(key);
              _setRuntimeByKey(key, legacy);
              return legacy;
            } catch (_) {
              return _runtimeByKey(key);
            }
          }
        }
        return _runtimeByKey(key);
      }
    }
  }

  static Future<void> _secureDelete(String key) async {
    try {
      await _secureStorage.delete(key: key);
    } catch (e) {
      await _repairWebSecureStorageIfNeeded(operation: 'delete:$key', error: e);
      try {
        await _secureStorage.delete(key: key);
      } catch (e2) {
        if (kDebugMode) {
          debugPrint('secure delete failed [$key]: $e2');
        }
      }
    } finally {
      _clearRuntimeByKey(key);
      await _cleanupLegacyWebPlaintextKey(key);
    }
  }

  // 保存 Token (安全存储)
  static Future<void> saveToken(String token) async {
    await _secureWrite(_tokenKey, token);
  }

  // 获取 Token (安全存储)
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

  // 保存用户名
  static Future<void> saveUsername(String username) async {
    final prefs = await _instance;
    await prefs.setString(_usernameKey, username);
  }

  // 获取用户名
  static Future<String?> getUsername() async {
    final prefs = await _instance;
    return prefs.getString(_usernameKey);
  }

  // 保存昵称
  static Future<void> saveNickname(String nickname) async {
    final prefs = await _instance;
    await prefs.setString(_nicknameKey, nickname);
  }

  // 获取昵称
  static Future<String?> getNickname() async {
    final prefs = await _instance;
    return prefs.getString(_nicknameKey);
  }

  // 保存设备ID
  static Future<void> saveDeviceId(String deviceId) async {
    final prefs = await _instance;
    await prefs.setString(_deviceIdKey, deviceId);
  }

  // 获取设备ID
  static Future<String?> getDeviceId() async {
    final prefs = await _instance;
    return prefs.getString(_deviceIdKey);
  }

  // 保存对象
  static Future<void> saveObject(String key, dynamic object) async {
    final prefs = await _instance;
    await prefs.setString(key, jsonEncode(object));
  }

  // 获取对象
  static Future<dynamic> getObject(String key) async {
    final prefs = await _instance;
    final jsonString = prefs.getString(key);
    if (jsonString == null) return null;
    return jsonDecode(jsonString);
  }

  // 清除所有数据（保留引导页标记和设备ID）
  static Future<void> clearAll() async {
    final prefs = await _instance;
    final onboardingDoneString = prefs.getString('onboarding_done');
    final onboardingDoneBool = prefs.getBool('onboarding_done');
    final deviceId = prefs.getString(_deviceIdKey);
    await prefs.clear();
    // 清除安全存储中的 token
    try {
      await _secureStorage.deleteAll();
    } catch (e) {
      await _repairWebSecureStorageIfNeeded(
          operation: 'deleteAll', error: e);
      if (kDebugMode) {
        debugPrint('clear secure storage failed: $e');
      }
    }
    if (onboardingDoneString != null) {
      await prefs.setString('onboarding_done', onboardingDoneString);
    } else if (onboardingDoneBool != null) {
      await prefs.setBool('onboarding_done', onboardingDoneBool);
    }
    if (deviceId != null) {
      await prefs.setString(_deviceIdKey, deviceId);
    }
    _runtimeToken = null;
    _runtimeRefreshToken = null;
    _runtimeUserId = null;
  }

  // 通用字符串保存方法
  static Future<void> saveString(String key, String value) async {
    final prefs = await _instance;
    await prefs.setString(key, value);
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
    await prefs.remove(key);
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
    await prefs.setInt('login_fail_count', count);
  }

  static Future<void> clearLoginFailCount() async {
    final prefs = await _instance;
    await prefs.remove('login_fail_count');
    await prefs.remove('login_lock_until');
  }

  // 登录锁定时间
  static Future<DateTime?> getLoginLockUntil() async {
    final prefs = await _instance;
    final ms = prefs.getInt('login_lock_until');
    return ms != null ? DateTime.fromMillisecondsSinceEpoch(ms) : null;
  }

  static Future<void> setLoginLockUntil(DateTime time) async {
    final prefs = await _instance;
    await prefs.setInt('login_lock_until', time.millisecondsSinceEpoch);
  }

  // 验证码发送时间（按类型区分）
  static Future<DateTime?> getCodeSentTime(String type) async {
    final prefs = await _instance;
    final ms = prefs.getInt('code_sent_$type');
    return ms != null ? DateTime.fromMillisecondsSinceEpoch(ms) : null;
  }

  static Future<void> setCodeSentTime(String type) async {
    final prefs = await _instance;
    await prefs.setInt('code_sent_$type', DateTime.now().millisecondsSinceEpoch);
  }
}
