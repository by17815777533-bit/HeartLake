// @file storage_util.dart
// @brief 本地存储工具类
// Created by 王璐瑶

import 'package:shared_preferences/shared_preferences.dart';
import 'dart:convert';

class StorageUtil {
  static const String _tokenKey = 'user_token';
  static const String _refreshTokenKey = 'refresh_token';
  static const String _userIdKey = 'user_id';
  static const String _deviceIdKey = 'device_id';
  static const String _usernameKey = 'username';
  static const String _nicknameKey = 'nickname';

  static SharedPreferences? _prefs;

  /// 获取缓存的SharedPreferences实例，避免重复异步调用
  static Future<SharedPreferences> get _instance async {
    _prefs ??= await SharedPreferences.getInstance();
    return _prefs!;
  }

  // 保存 Token
  static Future<void> saveToken(String token) async {
    final prefs = await _instance;
    await prefs.setString(_tokenKey, token);
  }

  // 获取 Token
  static Future<String?> getToken() async {
    final prefs = await _instance;
    return prefs.getString(_tokenKey);
  }

  // 清除 Token
  static Future<void> clearToken() async {
    final prefs = await _instance;
    await prefs.remove(_tokenKey);
  }

  // 保存 RefreshToken
  static Future<void> saveRefreshToken(String token) async {
    final prefs = await _instance;
    await prefs.setString(_refreshTokenKey, token);
  }

  // 获取 RefreshToken
  static Future<String?> getRefreshToken() async {
    final prefs = await _instance;
    return prefs.getString(_refreshTokenKey);
  }

  // 清除 RefreshToken
  static Future<void> clearRefreshToken() async {
    final prefs = await _instance;
    await prefs.remove(_refreshTokenKey);
  }

  // 保存用户ID
  static Future<void> saveUserId(String userId) async {
    final prefs = await _instance;
    await prefs.setString(_userIdKey, userId);
  }

  // 获取用户ID
  static Future<String?> getUserId() async {
    final prefs = await _instance;
    return prefs.getString(_userIdKey);
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

  // 清除所有数据（保留引导页标记）
  static Future<void> clearAll() async {
    final prefs = await _instance;
    final onboardingDone = prefs.getString('onboarding_done');
    await prefs.clear();
    if (onboardingDone != null) {
      await prefs.setString('onboarding_done', onboardingDone);
    }
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
