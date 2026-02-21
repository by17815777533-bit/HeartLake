// @file app_theme.dart
// @brief 光遇风格主题系统 - Sky: Children of the Light 设计语言

import 'dart:ui';
import 'package:flutter/material.dart';

class AppTheme {
  static const _seedColor = Color(0xFFFFAB91); // 光遇暖橙

  // === 保留原有颜色常量（向后兼容） ===
  static const Color primaryColor = Color(0xFFFFAB91);
  static const Color primaryLightColor = Color(0xFFFFCCBC);
  static const Color primaryDarkColor = Color(0xFFFF8A65);
  static const Color darkBlue = Color(0xFF1A73E8);

  static const Color secondaryColor = Color(0xFFB39DDB);
  static const Color accentColor = Color(0xFFFFD54F);

  // 温暖色系
  static const Color warmOrange = Color(0xFFFF8A65);
  static const Color peachPink = Color(0xFFFFAB91);
  static const Color purpleColor = Color(0xFFB39DDB);
  static const Color warmPink = Color(0xFFF48FB1);

  // 背景色
  static const Color backgroundColor = Color(0xFFFFF8F0);
  static const Color skyBlue = Color(0xFF81D4FA);
  static const Color borderCyan = Color(0xFF90CAF9);

  // 文本色
  static const Color textPrimary = Color(0xFF202124);
  static const Color textSecondary = Color(0xFF5F6368);
  static const Color textTertiary = Color(0xFF9AA0A6);

  // 功能色
  static const Color successColor = Color(0xFF66BB6A);
  static const Color warningColor = Color(0xFFFFD54F);
  static const Color errorColor = Color(0xFFEF5350);

  // 深色模式文本
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
    Color(0xFFFFAB91), Color(0xFFFFD54F), Color(0xFFB39DDB),
    Color(0xFFF48FB1), Color(0xFF80DEEA), Color(0xFFFFCC80),
    Color(0xFFCE93D8),
  ];

  // === 光遇专用色 ===
  static const Color candleGlow = Color(0xFFFFD54F);     // 蜡烛光
  static const Color spiritBlue = Color(0xFF80DEEA);     // 精灵蓝
  static const Color cloudPink = Color(0xFFF8BBD0);      // 云粉
  static const Color starPurple = Color(0xFFCE93D8);     // 星紫
  static const Color nightDeep = Color(0xFF0D1B2A);      // 深夜
  static const Color nightSurface = Color(0xFF1A1A2E);   // 夜面

  // 光遇渐变
  static const List<Color> warmGradient = [warmOrange, peachPink, purpleColor];
  static const List<Color> sunsetGradient = [Color(0xFFFFAB91), Color(0xFFFF8A65), Color(0xFFCE93D8)];
  static const List<Color> nightGradient = [Color(0xFF1A1A2E), Color(0xFF16213E), Color(0xFF0D1B2A)];

  // === Light Theme ===
  static final ThemeData lightTheme = ThemeData(
    useMaterial3: true,
    colorScheme: ColorScheme.fromSeed(
      seedColor: _seedColor,
      brightness: Brightness.light,
      primary: warmOrange,
      secondary: purpleColor,
      surface: const Color(0xFFFFF8F0),
      error: errorColor,
    ),
    scaffoldBackgroundColor: const Color(0xFFFFF8F0),
    appBarTheme: const AppBarTheme(
      backgroundColor: Colors.transparent,
      elevation: 0,
      scrolledUnderElevation: 0,
      centerTitle: true,
      titleTextStyle: TextStyle(
        fontSize: 18, fontWeight: FontWeight.w300,
        color: Color(0xFF3E2723), letterSpacing: 2,
      ),
      iconTheme: IconThemeData(color: Color(0xFF5D4037)),
    ),
    cardTheme: CardThemeData(
      color: Colors.white.withValues(alpha: 0.85),
      elevation: 0,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        backgroundColor: warmOrange,
        foregroundColor: Colors.white,
        elevation: 0,
        padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 14),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        textStyle: const TextStyle(fontSize: 15, fontWeight: FontWeight.w500, letterSpacing: 1),
      ),
    ),
    textButtonTheme: TextButtonThemeData(
      style: TextButton.styleFrom(
        foregroundColor: warmOrange,
        textStyle: const TextStyle(fontSize: 14, letterSpacing: 0.5),
      ),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: Colors.white.withValues(alpha: 0.6),
      contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(16),
        borderSide: BorderSide(color: peachPink.withValues(alpha: 0.3)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(16),
        borderSide: BorderSide(color: peachPink.withValues(alpha: 0.2)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(16),
        borderSide: const BorderSide(color: warmOrange, width: 1.5),
      ),
      hintStyle: TextStyle(color: textTertiary.withValues(alpha: 0.6), fontSize: 14),
    ),
    dialogTheme: DialogThemeData(
      backgroundColor: const Color(0xFFFFF8F0),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(24)),
      elevation: 8,
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: const Color(0xFF3E2723).withValues(alpha: 0.9),
      contentTextStyle: const TextStyle(color: Colors.white, fontSize: 14),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      behavior: SnackBarBehavior.floating,
    ),
    bottomSheetTheme: const BottomSheetThemeData(
      backgroundColor: Color(0xFFFFF8F0),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
    ),
    dividerTheme: DividerThemeData(
      color: peachPink.withValues(alpha: 0.15),
      thickness: 0.5,
    ),
  );

  // === Dark Theme ===
  static final ThemeData darkTheme = ThemeData(
    useMaterial3: true,
    colorScheme: ColorScheme.fromSeed(
      seedColor: _seedColor,
      brightness: Brightness.dark,
      primary: warmOrange,
      secondary: purpleColor,
      surface: nightSurface,
      error: errorColor,
    ),
    scaffoldBackgroundColor: nightDeep,
    appBarTheme: const AppBarTheme(
      backgroundColor: Colors.transparent,
      elevation: 0,
      scrolledUnderElevation: 0,
      centerTitle: true,
      titleTextStyle: TextStyle(
        fontSize: 18, fontWeight: FontWeight.w300,
        color: Colors.white, letterSpacing: 2,
      ),
      iconTheme: IconThemeData(color: Colors.white70),
    ),
    cardTheme: CardThemeData(
      color: Colors.white.withValues(alpha: 0.08),
      elevation: 0,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        backgroundColor: warmOrange,
        foregroundColor: Colors.white,
        elevation: 0,
        padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 14),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        textStyle: const TextStyle(fontSize: 15, fontWeight: FontWeight.w500, letterSpacing: 1),
      ),
    ),
    textButtonTheme: TextButtonThemeData(
      style: TextButton.styleFrom(
        foregroundColor: peachPink,
        textStyle: const TextStyle(fontSize: 14, letterSpacing: 0.5),
      ),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: Colors.white.withValues(alpha: 0.08),
      contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(16),
        borderSide: BorderSide(color: warmOrange.withValues(alpha: 0.2)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(16),
        borderSide: BorderSide(color: warmOrange.withValues(alpha: 0.15)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(16),
        borderSide: BorderSide(color: warmOrange.withValues(alpha: 0.6), width: 1.5),
      ),
      hintStyle: TextStyle(color: Colors.white.withValues(alpha: 0.35), fontSize: 14),
    ),
    dialogTheme: DialogThemeData(
      backgroundColor: nightSurface,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(24)),
      elevation: 0,
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: Colors.white.withValues(alpha: 0.12),
      contentTextStyle: const TextStyle(color: Colors.white, fontSize: 14),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      behavior: SnackBarBehavior.floating,
    ),
    bottomSheetTheme: const BottomSheetThemeData(
      backgroundColor: Color(0xFF1A1A2E),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
    ),
    dividerTheme: DividerThemeData(
      color: warmOrange.withValues(alpha: 0.1),
      thickness: 0.5,
    ),
  );
}
