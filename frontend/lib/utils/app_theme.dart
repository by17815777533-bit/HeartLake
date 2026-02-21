// @file app_theme.dart
// @brief 光遇风格主题系统 - Sky: Children of the Light 设计语言

import 'dart:ui';
import 'package:flutter/material.dart';

class AppTheme {
  // === 光遇核心色彩系统 ===
  static const Color primaryColor = Color(0xFFF2CC8F); // 金色主色
  static const Color primaryLightColor = Color(0xFFF5DDB5); // 浅金
  static const Color primaryDarkColor = Color(0xFFE8A87C); // 琥珀色
  static const Color darkBlue = Color(0xFF141432); // 星空蓝

  static const Color secondaryColor = Color(0xFF7B68AE); // 暮光紫
  static const Color accentColor = Color(0xFFF2CC8F); // 金色

  // 光遇暖色系
  static const Color warmOrange = Color(0xFFE8A87C); // 琥珀色
  static const Color peachPink = Color(0xFFF2CC8F); // 金色
  static const Color purpleColor = Color(0xFF7B68AE); // 暮光紫
  static const Color warmPink = Color(0xFFE07A5F); // 日落色

  // 背景色
  static const Color backgroundColor = Color(0xFFFFF8F0); // 暖白
  static const Color skyBlue = Color(0xFF222255); // 深蓝面
  static const Color borderCyan = Color(0xFF7B68AE); // 暮光紫边框

  // 文本色
  static const Color textPrimary = Color(0xFF3D2B1F); // 深棕
  static const Color textSecondary = Color(0xFF6B5B4F); // 中棕
  static const Color textTertiary = Color(0xFF9A8A7E); // 淡棕

  // 功能色
  static const Color successColor = Color(0xFF7BAE7B); // 柔和绿
  static const Color warningColor = Color(0xFFF2CC8F); // 金色警告
  static const Color errorColor = Color(0xFFE07A5F); // 日落红

  // 深色模式文本
  static const Color darkTextPrimary = Color(0xFFF0E6D3); // 暖白
  static const Color darkTextSecondary = Color(0xFFB8A99A); // 淡金

  // 业务专用色 - 湖水（光遇风格）
  static const Color lakeSurface = Color(0xFF7B68AE); // 暮光紫
  static const Color lakeMiddle = Color(0xFF4A2C8A); // 深紫
  static const Color lakeDeep = Color(0xFF141432); // 星空深蓝
  static const Color lakeBackground = Color(0xFF1A1A3E); // 深蓝

  // 业务专用色 - 石头（光遇蜡烛光）
  static const Color lightStone = Color(0xFFFFF5EB); // 暖白
  static const Color mediumStone = Color(0xFFF2CC8F); // 金色
  static const Color heavyStone = Color(0xFFE8A87C); // 琥珀

  // 缤纷色板（光遇风格）
  static const List<Color> rainbowColors = [
    Color(0xFFF2CC8F), Color(0xFFE8A87C), Color(0xFFE07A5F),
    Color(0xFF7B68AE), Color(0xFF4A2C8A), Color(0xFFF0E6D3),
    Color(0xFFB8A99A),
  ];

  // === 光遇专用色 ===
  static const Color candleGlow = Color(0xFFF2CC8F); // 蜡烛金光
  static const Color spiritBlue = Color(0xFF7B68AE); // 暮光紫
  static const Color cloudPink = Color(0xFFE07A5F); // 日落色
  static const Color starPurple = Color(0xFF4A2C8A); // 深紫
  static const Color nightDeep = Color(0xFF0F0F2A); // 星空深蓝
  static const Color nightSurface = Color(0xFF1A1A3E); // 深蓝卡片

  // 光遇渐变
  static const List<Color> warmGradient = [
    Color(0xFFF2CC8F), Color(0xFFE8A87C), Color(0xFFE07A5F),
  ];
  static const List<Color> sunsetGradient = [
    Color(0xFFE07A5F), Color(0xFFE8A87C), Color(0xFF7B68AE),
  ];
  static const List<Color> nightGradient = [
    Color(0xFF0F0F2A), Color(0xFF1A1A3E), Color(0xFF222255),
  ];

  // === Light Theme ===
  static final ThemeData lightTheme = ThemeData(
    useMaterial3: true,
    colorScheme: ColorScheme.fromSeed(
      seedColor: primaryColor,
      brightness: Brightness.light,
      primary: warmOrange,
      secondary: purpleColor,
      surface: const Color(0xFFFFF8F0),
      error: errorColor,
    ),
    scaffoldBackgroundColor: const Color(0xFFFFF8F0),
    appBarTheme: AppBarTheme(
      backgroundColor: const Color(0xFFFFF8F0).withValues(alpha: 0.85),
      elevation: 0,
      scrolledUnderElevation: 0,
      centerTitle: true,
      titleTextStyle: const TextStyle(
        fontSize: 18, fontWeight: FontWeight.w600,
        color: Color(0xFF3D2B1F), letterSpacing: 1.5,
      ),
      iconTheme: const IconThemeData(color: Color(0xFF6B5B4F)),
    ),
    cardTheme: CardThemeData(
      color: Colors.white,
      elevation: 2,
      shadowColor: const Color(0xFFF2CC8F).withValues(alpha: 0.2),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        backgroundColor: warmOrange,
        foregroundColor: Colors.white,
        elevation: 0,
        padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 14),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        textStyle: const TextStyle(fontSize: 15, fontWeight: FontWeight.w600, letterSpacing: 0.5),
      ),
    ),
    textButtonTheme: TextButtonThemeData(
      style: TextButton.styleFrom(
        foregroundColor: warmOrange,
        textStyle: const TextStyle(fontSize: 14, fontWeight: FontWeight.w500),
      ),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: const Color(0xFFFFF5EB),
      contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: primaryColor.withValues(alpha: 0.3)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: primaryColor.withValues(alpha: 0.2)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: const BorderSide(color: Color(0xFFF2CC8F), width: 1.5),
      ),
      hintStyle: TextStyle(color: textTertiary.withValues(alpha: 0.6), fontSize: 14),
    ),
    dialogTheme: DialogThemeData(
      backgroundColor: const Color(0xFFFFF8F0),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      elevation: 8,
      shadowColor: const Color(0xFFF2CC8F).withValues(alpha: 0.15),
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: const Color(0xFF3D2B1F).withValues(alpha: 0.9),
      contentTextStyle: const TextStyle(color: Color(0xFFF0E6D3), fontSize: 14),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      behavior: SnackBarBehavior.floating,
    ),
    bottomSheetTheme: const BottomSheetThemeData(
      backgroundColor: Color(0xFFFFF8F0),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
    ),
    bottomNavigationBarTheme: BottomNavigationBarThemeData(
      backgroundColor: const Color(0xFFFFF8F0).withValues(alpha: 0.9),
      selectedItemColor: warmOrange,
      unselectedItemColor: textTertiary,
      elevation: 0,
      type: BottomNavigationBarType.fixed,
    ),
    dividerTheme: DividerThemeData(
      color: primaryColor.withValues(alpha: 0.12),
      thickness: 0.5,
    ),
  );

  // === Dark Theme (默认主题) ===
  static final ThemeData darkTheme = ThemeData(
    useMaterial3: true,
    colorScheme: ColorScheme.fromSeed(
      seedColor: primaryColor,
      brightness: Brightness.dark,
      primary: primaryColor,
      secondary: purpleColor,
      surface: nightSurface,
      error: errorColor,
    ),
    scaffoldBackgroundColor: nightDeep,
    appBarTheme: AppBarTheme(
      backgroundColor: nightDeep.withValues(alpha: 0.7),
      elevation: 0,
      scrolledUnderElevation: 0,
      centerTitle: true,
      titleTextStyle: const TextStyle(
        fontSize: 18, fontWeight: FontWeight.w600,
        color: Color(0xFFF0E6D3), letterSpacing: 1.5,
      ),
      iconTheme: const IconThemeData(color: Color(0xFFB8A99A)),
    ),
    cardTheme: CardThemeData(
      color: const Color(0xFF1A1A3E),
      elevation: 4,
      shadowColor: const Color(0xFFF2CC8F).withValues(alpha: 0.08),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        backgroundColor: primaryColor,
        foregroundColor: const Color(0xFF0F0F2A),
        elevation: 0,
        padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 14),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        textStyle: const TextStyle(fontSize: 15, fontWeight: FontWeight.w600, letterSpacing: 0.5),
      ),
    ),
    textButtonTheme: TextButtonThemeData(
      style: TextButton.styleFrom(
        foregroundColor: primaryColor,
        textStyle: const TextStyle(fontSize: 14, fontWeight: FontWeight.w500),
      ),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: const Color(0xFF222255).withValues(alpha: 0.5),
      contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: primaryColor.withValues(alpha: 0.2)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: primaryColor.withValues(alpha: 0.15)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: primaryColor.withValues(alpha: 0.7), width: 1.5),
      ),
      hintStyle: TextStyle(color: const Color(0xFF7A6F63).withValues(alpha: 0.7), fontSize: 14),
    ),
    dialogTheme: DialogThemeData(
      backgroundColor: nightSurface,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      elevation: 0,
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: const Color(0xFF222255).withValues(alpha: 0.9),
      contentTextStyle: const TextStyle(color: Color(0xFFF0E6D3), fontSize: 14),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      behavior: SnackBarBehavior.floating,
    ),
    bottomSheetTheme: const BottomSheetThemeData(
      backgroundColor: Color(0xFF1A1A3E),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
    ),
    bottomNavigationBarTheme: BottomNavigationBarThemeData(
      backgroundColor: nightDeep.withValues(alpha: 0.85),
      selectedItemColor: primaryColor,
      unselectedItemColor: const Color(0xFF7A6F63),
      elevation: 0,
      type: BottomNavigationBarType.fixed,
    ),
    dividerTheme: DividerThemeData(
      color: primaryColor.withValues(alpha: 0.08),
      thickness: 0.5,
    ),
  );
}
