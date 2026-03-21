// 主题状态管理
//
// 持久化用户的亮/暗色偏好到 SharedPreferences，
// 初始化时自动恢复上次选择，切换后立即写入本地存储。
// 无外部依赖，仅依赖 [SharedPreferences] 做本地持久化。

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

/// 主题模式状态管理器，持久化用户的亮/暗色偏好
///
/// 初始化时从 [SharedPreferences] 读取上次选择，
/// 切换后立即写入本地存储，下次启动自动恢复。
class ThemeProvider with ChangeNotifier {
  ThemeMode _themeMode = ThemeMode.light; // 默认浅色模式

  ThemeMode get themeMode => _themeMode;

  ThemeProvider() {
    _loadThemeMode();
  }

  /// 兼容读取 bool 值
  ///
  /// SharedPreferences 在某些平台/版本下可能将 bool 存为 String，
  /// 这里做兼容处理，避免类型转换异常。
  bool _readBoolCompat(SharedPreferences prefs, String key,
      {bool fallback = false}) {
    final value = prefs.get(key);
    if (value is bool) return value;
    if (value is String) {
      final normalized = value.trim().toLowerCase();
      if (normalized == 'true') return true;
      if (normalized == 'false') return false;
    }
    return fallback;
  }

  /// 从本地存储加载主题偏好
  Future<void> _loadThemeMode() async {
    final prefs = await SharedPreferences.getInstance();
    final isDark = _readBoolCompat(prefs, 'isDarkMode');
    _themeMode = isDark ? ThemeMode.dark : ThemeMode.light;
    notifyListeners();
  }

  /// 切换亮/暗色主题并持久化到本地存储
  Future<void> toggleTheme() async {
    _themeMode =
        _themeMode == ThemeMode.light ? ThemeMode.dark : ThemeMode.light;

    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('isDarkMode', _themeMode == ThemeMode.dark);

    notifyListeners();
  }
}
