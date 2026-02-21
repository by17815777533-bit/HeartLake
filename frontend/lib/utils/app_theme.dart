// @file app_theme.dart
// @brief 心湖主题系统 - 蓝色湖面风格

import 'package:flutter/material.dart';

class AppTheme {
  static const Color seedColor = Color(0xFF4285F4);

  // 核心色
  static const Color primaryColor = Color(0xFF4285F4);
  static const Color secondaryColor = Color(0xFF34A853);
  static const Color tertiaryColor = Color(0xFF80DEEA);
  static const Color accentColor = Color(0xFFFBBC04);

  // 功能色
  static const Color successColor = Color(0xFF34A853);
  static const Color warningColor = Color(0xFFFBBC04);
  static const Color errorColor = Color(0xFFEA4335);

  // 文本色（亮色）
  static const Color textPrimary = Color(0xFF202124);
  static const Color textSecondary = Color(0xFF5F6368);
  static const Color textTertiary = Color(0xFF9AA0A6);

  // 文本色（暗色）
  static const Color darkTextPrimary = Color(0xFFE8EAED);
  static const Color darkTextSecondary = Color(0xFF9AA0A6);

  // 湖面业务色
  static const Color lakeSurface = Color(0xFF64B5F6);
  static const Color lakeMiddle = Color(0xFF1E88E5);
  static const Color lakeDeep = Color(0xFF0D47A1);
  static const Color lakeBackground = Color(0xFF1565C0);

  // 石头业务色
  static const Color lightStone = Color(0xFFFFF8E1);
  static const Color mediumStone = Color(0xFFFFE082);
  static const Color heavyStone = Color(0xFFFFB300);

  // 风格别名（向后兼容）
  static const Color warmOrange = Color(0xFFFF8A65);
  static const Color peachPink = Color(0xFFFFAB91);
  static const Color purpleColor = Color(0xFFB39DDB);
  static const Color warmPink = Color(0xFFF48FB1);
  static const Color candleGlow = Color(0xFF8AB4F8);
  static const Color spiritBlue = Color(0xFF4285F4);
  static const Color cloudPink = Color(0xFFE3F2FD);
  static const Color starPurple = Color(0xFF1A73E8);
  static const Color nightDeep = Color(0xFF0D1B2A);
  static const Color nightSurface = Color(0xFF1B2838);
  static const Color backgroundColor = Color(0xFFF8F9FA);
  static const Color skyBlue = Color(0xFF81D4FA);
  static const Color borderCyan = Color(0xFF90CAF9);
  static const Color darkBlue = Color(0xFF1A73E8);
  static const Color primaryLightColor = Color(0xFF8AB4F8);
  static const Color primaryDarkColor = Color(0xFF1A73E8);

  static const List<Color> rainbowColors = [
    Color(0xFF5C9CE6),
    Color(0xFF81C784),
    Color(0xFFFFB74D),
    Color(0xFFF8BBD9),
    Color(0xFFB39DDB),
    Color(0xFF80DEEA),
    Color(0xFFFFCC80),
  ];

  static const List<Color> warmGradient = [
    Color(0xFF8AB4F8),
    Color(0xFF4285F4),
    Color(0xFF1A73E8),
  ];

  static const List<Color> sunsetGradient = [
    Color(0xFFFF8A65),
    Color(0xFFB39DDB),
    Color(0xFF64B5F6),
  ];

  static const List<Color> nightGradient = [
    Color(0xFF0D1B2A),
    Color(0xFF1B2838),
    Color(0xFF2A3F5F),
  ];

  // 兼容层：为仍在迁移中的页面提供一个可用的全局配色对象。
  static const ColorScheme fallbackColorScheme = ColorScheme.light(
    primary: primaryColor,
    onPrimary: Color(0xFFFFFFFF),
    secondary: secondaryColor,
    onSecondary: Color(0xFFFFFFFF),
    tertiary: accentColor,
    onTertiary: Color(0xFF202124),
    error: errorColor,
    onError: Color(0xFFFFFFFF),
    surface: backgroundColor,
    onSurface: textPrimary,
    surfaceContainerHighest: Color(0xFFE8F0FE),
    onSurfaceVariant: textSecondary,
    outline: Color(0xFFDADCE0),
  );

  static final ThemeData lightTheme = ThemeData(
    useMaterial3: true,
    brightness: Brightness.light,
    colorScheme: ColorScheme.fromSeed(
      seedColor: seedColor,
      brightness: Brightness.light,
      primary: primaryColor,
      secondary: secondaryColor,
      tertiary: accentColor,
      surface: backgroundColor,
      error: errorColor,
    ).copyWith(
      onPrimary: const Color(0xFFFFFFFF),
      onSecondary: const Color(0xFFFFFFFF),
      onSurface: textPrimary,
      onSurfaceVariant: textSecondary,
      onError: const Color(0xFFFFFFFF),
      outline: const Color(0xFFDADCE0),
      surfaceContainerHighest: const Color(0xFFE8F0FE),
    ),
    scaffoldBackgroundColor: backgroundColor,
    appBarTheme: AppBarTheme(
      backgroundColor: backgroundColor.withValues(alpha: 0.92),
      foregroundColor: textPrimary,
      elevation: 0,
      scrolledUnderElevation: 0,
      centerTitle: true,
      titleTextStyle: const TextStyle(
        fontSize: 18,
        fontWeight: FontWeight.w600,
        color: textPrimary,
        letterSpacing: 0.8,
      ),
      iconTheme: const IconThemeData(color: textSecondary),
    ),
    cardTheme: CardThemeData(
      color: Colors.white.withValues(alpha: 0.85),
      elevation: 0,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(18)),
      shadowColor: Colors.black.withValues(alpha: 0.04),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        backgroundColor: primaryColor,
        foregroundColor: Colors.white,
        elevation: 0,
        padding: const EdgeInsets.symmetric(horizontal: 28, vertical: 14),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
        textStyle: const TextStyle(
          fontSize: 15,
          fontWeight: FontWeight.w600,
          letterSpacing: 0.5,
        ),
      ),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: Colors.white.withValues(alpha: 0.75),
      contentPadding: const EdgeInsets.symmetric(horizontal: 18, vertical: 14),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: primaryColor.withValues(alpha: 0.18)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: borderCyan.withValues(alpha: 0.3)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: primaryColor.withValues(alpha: 0.75), width: 1.4),
      ),
      hintStyle: TextStyle(color: textTertiary.withValues(alpha: 0.8)),
    ),
    navigationBarTheme: NavigationBarThemeData(
      backgroundColor: const Color(0xFFF5F9FC),
      indicatorColor: primaryColor.withValues(alpha: 0.2),
      elevation: 0,
      labelTextStyle: WidgetStateProperty.resolveWith((states) {
        return TextStyle(
          fontSize: 12,
          fontWeight: states.contains(WidgetState.selected)
              ? FontWeight.w600
              : FontWeight.w500,
          color: states.contains(WidgetState.selected)
              ? textPrimary
              : textTertiary,
        );
      }),
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: const Color(0xFF323232).withValues(alpha: 0.92),
      contentTextStyle: const TextStyle(color: Color(0xFFE8EAED)),
      behavior: SnackBarBehavior.floating,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    ),
    dividerTheme: DividerThemeData(
      color: borderCyan.withValues(alpha: 0.18),
      thickness: 0.6,
    ),
  );

  static final ThemeData darkTheme = ThemeData(
    useMaterial3: true,
    brightness: Brightness.dark,
    colorScheme: ColorScheme.fromSeed(
      seedColor: seedColor,
      brightness: Brightness.dark,
      primary: primaryLightColor,
      secondary: const Color(0xFF81C784),
      tertiary: const Color(0xFFFFE082),
      surface: nightSurface,
      error: const Color(0xFFFFB4AB),
    ).copyWith(
      onPrimary: const Color(0xFF0D1B2A),
      onSecondary: const Color(0xFFFFFFFF),
      onSurface: darkTextPrimary,
      onSurfaceVariant: darkTextSecondary,
      onError: const Color(0xFF0D1B2A),
      outline: primaryLightColor.withValues(alpha: 0.3),
      surfaceContainerHighest: const Color(0xFF2A3F5F),
    ),
    scaffoldBackgroundColor: nightDeep,
    appBarTheme: AppBarTheme(
      backgroundColor: nightDeep.withValues(alpha: 0.82),
      foregroundColor: darkTextPrimary,
      elevation: 0,
      scrolledUnderElevation: 0,
      centerTitle: true,
      titleTextStyle: const TextStyle(
        fontSize: 18,
        fontWeight: FontWeight.w600,
        color: darkTextPrimary,
        letterSpacing: 0.8,
      ),
      iconTheme: const IconThemeData(color: darkTextSecondary),
    ),
    cardTheme: CardThemeData(
      color: const Color(0xFF1B2838).withValues(alpha: 0.75),
      elevation: 0,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(18)),
      shadowColor: primaryLightColor.withValues(alpha: 0.08),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        backgroundColor: primaryLightColor,
        foregroundColor: const Color(0xFF0D1B2A),
        elevation: 0,
        padding: const EdgeInsets.symmetric(horizontal: 28, vertical: 14),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
        textStyle: const TextStyle(
          fontSize: 15,
          fontWeight: FontWeight.w600,
          letterSpacing: 0.5,
        ),
      ),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: const Color(0xFF0D1B2A).withValues(alpha: 0.68),
      contentPadding: const EdgeInsets.symmetric(horizontal: 18, vertical: 14),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: primaryLightColor.withValues(alpha: 0.15)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: primaryLightColor.withValues(alpha: 0.16)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: primaryLightColor.withValues(alpha: 0.55), width: 1.4),
      ),
      hintStyle: TextStyle(color: darkTextSecondary.withValues(alpha: 0.7)),
    ),
    navigationBarTheme: NavigationBarThemeData(
      backgroundColor: const Color(0xFF1B2838),
      indicatorColor: primaryLightColor.withValues(alpha: 0.18),
      elevation: 0,
      labelTextStyle: WidgetStateProperty.resolveWith((states) {
        return TextStyle(
          fontSize: 12,
          fontWeight: states.contains(WidgetState.selected)
              ? FontWeight.w600
              : FontWeight.w500,
          color: states.contains(WidgetState.selected)
              ? darkTextPrimary
              : darkTextSecondary,
        );
      }),
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: const Color(0xFF1B2838).withValues(alpha: 0.95),
      contentTextStyle: const TextStyle(color: Color(0xFFE8EAED)),
      behavior: SnackBarBehavior.floating,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    ),
    dividerTheme: DividerThemeData(
      color: primaryLightColor.withValues(alpha: 0.1),
      thickness: 0.6,
    ),
  );
}

// 兼容旧页面中的 `colorScheme.*` 直接访问，避免迁移阶段大面积编译失败。
const ColorScheme colorScheme = AppTheme.fallbackColorScheme;
