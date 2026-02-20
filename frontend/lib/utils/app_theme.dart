// @file app_theme.dart
// @brief Material Design 3 主题配置

import 'package:flutter/material.dart';

class AppTheme {
  static const _seedColor = Color(0xFF4285F4);

  // Google 蓝色系
  static const Color primaryColor = Color(0xFF4285F4);
  static const Color primaryLightColor = Color(0xFF8AB4F8);
  static const Color primaryDarkColor = Color(0xFF1A73E8);
  static const Color darkBlue = Color(0xFF1A73E8);

  // 辅助色
  static const Color secondaryColor = Color(0xFF34A853);
  static const Color accentColor = Color(0xFFFBBC04);

  // 温暖色系
  static const Color warmOrange = Color(0xFFFF8A65);
  static const Color peachPink = Color(0xFFFFAB91);
  static const Color purpleColor = Color(0xFFB39DDB);
  static const Color warmPink = Color(0xFFF48FB1);

  // 背景色
  static const Color backgroundColor = Color(0xFFF8F9FA);
  static const Color skyBlue = Color(0xFF81D4FA);
  static const Color borderCyan = Color(0xFF90CAF9);

  // 文本色
  static const Color textPrimary = Color(0xFF202124);
  static const Color textSecondary = Color(0xFF5F6368);
  static const Color textTertiary = Color(0xFF9AA0A6);

  // 功能色
  static const Color successColor = Color(0xFF34A853);
  static const Color warningColor = Color(0xFFFBBC04);
  static const Color errorColor = Color(0xFFEA4335);

  // 深色模式
  static const Color darkTextPrimary = Color(0xFFE8EAED);
  static const Color darkTextSecondary = Color(0xFF9AA0A6);

  // 业务专用色 - 湖水
  static const Color lakeSurface = Color(0xFF64B5F6);
  static const Color lakeMiddle = Color(0xFF1E88E5);
  static const Color lakeDeep = Color(0xFF0D47A1);
  static const Color lakeBackground = Color(0xFF1565C0);

  // 业务专用色 - 石头
  static const Color lightStone = Color(0xFFFFF8E1);
  static const Color mediumStone = Color(0xFFFFE082);
  static const Color heavyStone = Color(0xFFFFB300);

  // 缤纷色板
  static const List<Color> rainbowColors = [
    Color(0xFF5C9CE6),
    Color(0xFF81C784),
    Color(0xFFFFB74D),
    Color(0xFFF8BBD9),
    Color(0xFFB39DDB),
    Color(0xFF80DEEA),
    Color(0xFFFFCC80),
  ];

  static final ThemeData lightTheme = ThemeData(
    useMaterial3: true,
    colorScheme: ColorScheme.fromSeed(
      seedColor: _seedColor,
      brightness: Brightness.light,
    ),
  );

  static final ThemeData darkTheme = ThemeData(
    useMaterial3: true,
    colorScheme: ColorScheme.fromSeed(
      seedColor: _seedColor,
      brightness: Brightness.dark,
    ),
  );
}
