// @file app_theme.dart
// @brief 心湖主题系统 - 光遇暖色氛围

import 'package:flutter/material.dart';

class AppTheme {
  static const Color seedColor = Color(0xFFFFAB91);

  // 核心色
  static const Color primaryColor = Color(0xFFFFAB91);
  static const Color secondaryColor = Color(0xFFB39DDB);
  static const Color tertiaryColor = Color(0xFF7EC8E3);
  static const Color accentColor = Color(0xFFFFD54F);

  // 功能色
  static const Color successColor = Color(0xFF81B29A);
  static const Color warningColor = Color(0xFFE8A87C);
  static const Color errorColor = Color(0xFFE07A5F);

  // 文本色（亮色）
  static const Color textPrimary = Color(0xFF3D2B1F);
  static const Color textSecondary = Color(0xFF6B5B4F);
  static const Color textTertiary = Color(0xFF9A8A7E);

  // 文本色（暗色）
  static const Color darkTextPrimary = Color(0xFFF0E6D3);
  static const Color darkTextSecondary = Color(0xFFB8A99A);

  // 湖面业务色
  static const Color lakeSurface = Color(0xFF7BA3C4);
  static const Color lakeMiddle = Color(0xFF4D7298);
  static const Color lakeDeep = Color(0xFF1E3A5F);
  static const Color lakeBackground = Color(0xFF142847);

  // 石头业务色
  static const Color lightStone = Color(0xFFFFF5EB);
  static const Color mediumStone = Color(0xFFF2CC8F);
  static const Color heavyStone = Color(0xFFE8A87C);

  // 光遇风格别名（向后兼容）
  static const Color warmOrange = Color(0xFFE8A87C);
  static const Color peachPink = Color(0xFFF2CC8F);
  static const Color purpleColor = Color(0xFF7B68AE);
  static const Color warmPink = Color(0xFFE07A5F);
  static const Color candleGlow = Color(0xFFF2CC8F);
  static const Color spiritBlue = Color(0xFF7B68AE);
  static const Color cloudPink = Color(0xFFEFD3C6);
  static const Color starPurple = Color(0xFF4A2C8A);
  static const Color nightDeep = Color(0xFF0F0F2A);
  static const Color nightSurface = Color(0xFF1A1A3E);
  static const Color backgroundColor = Color(0xFFFFF8F0);
  static const Color skyBlue = Color(0xFF222255);
  static const Color borderCyan = Color(0xFF7B68AE);
  static const Color darkBlue = Color(0xFF141432);
  static const Color primaryLightColor = Color(0xFFFFCCBC);
  static const Color primaryDarkColor = Color(0xFFE08B73);

  static const List<Color> rainbowColors = [
    Color(0xFFFFD54F),
    Color(0xFFFFAB91),
    Color(0xFFE07A5F),
    Color(0xFFB39DDB),
    Color(0xFF7EC8E3),
    Color(0xFF81B29A),
    Color(0xFF7B68AE),
  ];

  static const List<Color> warmGradient = [
    Color(0xFFF2CC8F),
    Color(0xFFE8A87C),
    Color(0xFFE07A5F),
  ];

  static const List<Color> sunsetGradient = [
    Color(0xFFE07A5F),
    Color(0xFFB39DDB),
    Color(0xFF7EC8E3),
  ];

  static const List<Color> nightGradient = [
    Color(0xFF0F0F2A),
    Color(0xFF1A1A3E),
    Color(0xFF2D3A66),
  ];

  // 兼容层：为仍在迁移中的页面提供一个可用的全局配色对象。
  // 后续页面应优先使用 Theme.of(context).colorScheme。
  static const ColorScheme fallbackColorScheme = ColorScheme.light(
    primary: primaryColor,
    onPrimary: Color(0xFFFFFFFF),
    secondary: secondaryColor,
    onSecondary: Color(0xFFFFFFFF),
    tertiary: accentColor,
    onTertiary: Color(0xFF2A1808),
    error: errorColor,
    onError: Color(0xFFFFFFFF),
    surface: backgroundColor,
    onSurface: textPrimary,
    surfaceContainerHighest: Color(0xFFF7ECE0),
    onSurfaceVariant: textSecondary,
    outline: Color(0xFFD7C2AE),
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
      outline: const Color(0xFFD7C2AE),
      surfaceContainerHighest: const Color(0xFFF7ECE0),
    ),
    fontFamily: 'Inter',
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
        borderSide: BorderSide(color: secondaryColor.withValues(alpha: 0.2)),
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
      backgroundColor: const Color(0xFF4A3A2D).withValues(alpha: 0.92),
      contentTextStyle: const TextStyle(color: Color(0xFFFFF3E8)),
      behavior: SnackBarBehavior.floating,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    ),
    dividerTheme: DividerThemeData(
      color: secondaryColor.withValues(alpha: 0.18),
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
      secondary: const Color(0xFFC9B8E8),
      tertiary: const Color(0xFFFFE082),
      surface: nightSurface,
      error: const Color(0xFFFFB4AB),
    ).copyWith(
      onPrimary: const Color(0xFF2A1808),
      onSecondary: const Color(0xFFFFFFFF),
      onSurface: darkTextPrimary,
      onSurfaceVariant: darkTextSecondary,
      onError: const Color(0xFF2A1808),
      outline: candleGlow.withValues(alpha: 0.3),
      surfaceContainerHighest: const Color(0xFF2D2D52),
    ),
    fontFamily: 'Inter',
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
      color: const Color(0xFF1A1A3E).withValues(alpha: 0.75),
      elevation: 0,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(18)),
      shadowColor: const Color(0xFFF2CC8F).withValues(alpha: 0.08),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        backgroundColor: peachPink,
        foregroundColor: const Color(0xFF2A1808),
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
      fillColor: const Color(0xFF121235).withValues(alpha: 0.68),
      contentPadding: const EdgeInsets.symmetric(horizontal: 18, vertical: 14),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: candleGlow.withValues(alpha: 0.15)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: candleGlow.withValues(alpha: 0.16)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: candleGlow.withValues(alpha: 0.55), width: 1.4),
      ),
      hintStyle: TextStyle(color: darkTextSecondary.withValues(alpha: 0.7)),
    ),
    navigationBarTheme: NavigationBarThemeData(
      backgroundColor: const Color(0xFF303134),
      indicatorColor: peachPink.withValues(alpha: 0.18),
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
      backgroundColor: const Color(0xFF171733).withValues(alpha: 0.95),
      contentTextStyle: const TextStyle(color: Color(0xFFF0E6D3)),
      behavior: SnackBarBehavior.floating,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    ),
    dividerTheme: DividerThemeData(
      color: candleGlow.withValues(alpha: 0.1),
      thickness: 0.6,
    ),
  );
}

// 兼容旧页面中的 `colorScheme.*` 直接访问，避免迁移阶段大面积编译失败。
const ColorScheme colorScheme = AppTheme.fallbackColorScheme;
