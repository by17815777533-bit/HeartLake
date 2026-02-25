import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/app_theme.dart';
import 'package:flutter/material.dart';

void main() {
  // ==================== 核心色 ====================
  group('AppTheme core colors', () {
    test('primaryColor should be correct', () {
      expect(AppTheme.primaryColor, const Color(0xFF4285F4));
    });

    test('secondaryColor should be correct', () {
      expect(AppTheme.secondaryColor, const Color(0xFF34A853));
    });

    test('tertiaryColor should be correct', () {
      expect(AppTheme.tertiaryColor, const Color(0xFF80DEEA));
    });

    test('accentColor should be correct', () {
      expect(AppTheme.accentColor, const Color(0xFFFBBC04));
    });

    test('seedColor should match primaryColor', () {
      expect(AppTheme.seedColor, AppTheme.primaryColor);
    });
  });

  // ==================== 功能色 ====================
  group('AppTheme functional colors', () {
    test('successColor should be green', () {
      expect(AppTheme.successColor, const Color(0xFF34A853));
    });

    test('warningColor should be yellow', () {
      expect(AppTheme.warningColor, const Color(0xFFFBBC04));
    });

    test('errorColor should be red', () {
      expect(AppTheme.errorColor, const Color(0xFFEA4335));
    });

    test('successColor should match secondaryColor', () {
      expect(AppTheme.successColor, AppTheme.secondaryColor);
    });

    test('warningColor should match accentColor', () {
      expect(AppTheme.warningColor, AppTheme.accentColor);
    });
  });

  // ==================== 文本色 ====================
  group('AppTheme text colors', () {
    test('textPrimary should be dark', () {
      expect(AppTheme.textPrimary, const Color(0xFF202124));
      expect((AppTheme.textPrimary.a * 255).round(), 255);
    });

    test('textSecondary should be medium', () {
      expect(AppTheme.textSecondary, const Color(0xFF5F6368));
    });

    test('textTertiary should be light', () {
      expect(AppTheme.textTertiary, const Color(0xFF9AA0A6));
    });

    test('darkTextPrimary should be light for dark mode', () {
      expect(AppTheme.darkTextPrimary, const Color(0xFFE8EAED));
    });

    test('darkTextSecondary should be medium for dark mode', () {
      expect(AppTheme.darkTextSecondary, const Color(0xFF9AA0A6));
    });

    test('textPrimary should be darker than textSecondary', () {
      // Lower luminance = darker
      expect(
        AppTheme.textPrimary.computeLuminance(),
        lessThan(AppTheme.textSecondary.computeLuminance()),
      );
    });

    test('textSecondary should be darker than textTertiary', () {
      expect(
        AppTheme.textSecondary.computeLuminance(),
        lessThan(AppTheme.textTertiary.computeLuminance()),
      );
    });
  });

  // ==================== 湖面业务色 ====================
  group('AppTheme lake colors', () {
    test('lakeSurface should be defined', () {
      expect(AppTheme.lakeSurface, const Color(0xFF64B5F6));
    });

    test('lakeMiddle should be defined', () {
      expect(AppTheme.lakeMiddle, const Color(0xFF1E88E5));
    });

    test('lakeDeep should be defined', () {
      expect(AppTheme.lakeDeep, const Color(0xFF0D47A1));
    });

    test('lakeBackground should be defined', () {
      expect(AppTheme.lakeBackground, const Color(0xFF1565C0));
    });

    test('lake colors should get progressively darker', () {
      expect(
        AppTheme.lakeSurface.computeLuminance(),
        greaterThan(AppTheme.lakeMiddle.computeLuminance()),
      );
      expect(
        AppTheme.lakeMiddle.computeLuminance(),
        greaterThan(AppTheme.lakeDeep.computeLuminance()),
      );
    });
  });

  // ==================== 石头业务色 ====================
  group('AppTheme stone colors', () {
    test('lightStone should be lightest', () {
      expect(AppTheme.lightStone, const Color(0xFFFFF8E1));
    });

    test('mediumStone should be medium', () {
      expect(AppTheme.mediumStone, const Color(0xFFFFE082));
    });

    test('heavyStone should be darkest', () {
      expect(AppTheme.heavyStone, const Color(0xFFFFB300));
    });

    test('stone colors should get progressively darker', () {
      expect(
        AppTheme.lightStone.computeLuminance(),
        greaterThan(AppTheme.mediumStone.computeLuminance()),
      );
      expect(
        AppTheme.mediumStone.computeLuminance(),
        greaterThan(AppTheme.heavyStone.computeLuminance()),
      );
    });
  });

  // ==================== VIP色 ====================
  group('AppTheme VIP colors', () {
    test('vipGold should be defined', () {
      expect(AppTheme.vipGold, const Color(0xFFFFD54F));
    });

    test('vipGoldDark should be defined', () {
      expect(AppTheme.vipGoldDark, const Color(0xFFFF8F00));
    });

    test('vipGold should be lighter than vipGoldDark', () {
      expect(
        AppTheme.vipGold.computeLuminance(),
        greaterThan(AppTheme.vipGoldDark.computeLuminance()),
      );
    });
  });

  // ==================== 别名色 ====================
  group('AppTheme alias colors', () {
    test('warmOrange should be defined', () {
      expect(AppTheme.warmOrange, isA<Color>());
    });

    test('peachPink should be defined', () {
      expect(AppTheme.peachPink, isA<Color>());
    });

    test('purpleColor should be defined', () {
      expect(AppTheme.purpleColor, isA<Color>());
    });

    test('warmPink should be defined', () {
      expect(AppTheme.warmPink, isA<Color>());
    });

    test('candleGlow should be defined', () {
      expect(AppTheme.candleGlow, isA<Color>());
    });

    test('spiritBlue should match primaryColor', () {
      expect(AppTheme.spiritBlue, AppTheme.primaryColor);
    });

    test('cloudPink should be defined', () {
      expect(AppTheme.cloudPink, isA<Color>());
    });

    test('starPurple should be defined', () {
      expect(AppTheme.starPurple, isA<Color>());
    });

    test('nightDeep should be very dark', () {
      expect(AppTheme.nightDeep.computeLuminance(), lessThan(0.05));
    });

    test('nightSurface should be dark', () {
      expect(AppTheme.nightSurface.computeLuminance(), lessThan(0.1));
    });

    test('backgroundColor should be light', () {
      expect(AppTheme.backgroundColor.computeLuminance(), greaterThan(0.8));
    });

    test('skyBlue should be defined', () {
      expect(AppTheme.skyBlue, isA<Color>());
    });

    test('gentlePurple should be defined', () {
      expect(AppTheme.gentlePurple, isA<Color>());
    });

    test('borderCyan should be defined', () {
      expect(AppTheme.borderCyan, isA<Color>());
    });

    test('darkBlue should be defined', () {
      expect(AppTheme.darkBlue, isA<Color>());
    });

    test('primaryLightColor should be lighter than primaryColor', () {
      expect(
        AppTheme.primaryLightColor.computeLuminance(),
        greaterThan(AppTheme.primaryColor.computeLuminance()),
      );
    });

    test('primaryDarkColor should be darker than primaryColor', () {
      expect(
        AppTheme.primaryDarkColor.computeLuminance(),
        lessThanOrEqualTo(AppTheme.primaryColor.computeLuminance()),
      );
    });
  });

  // ==================== 渐变色列表 ====================
  group('AppTheme gradient lists', () {
    test('rainbowColors should have 7 colors', () {
      expect(AppTheme.rainbowColors.length, 7);
    });

    test('rainbowColors should all be opaque', () {
      for (final c in AppTheme.rainbowColors) {
        expect((c.a * 255).round(), 255);
      }
    });

    test('warmGradient should have 3 colors', () {
      expect(AppTheme.warmGradient.length, 3);
    });

    test('sunsetGradient should have 3 colors', () {
      expect(AppTheme.sunsetGradient.length, 3);
    });

    test('all gradient colors should be unique', () {
      expect(AppTheme.rainbowColors.toSet().length, 7);
    });
  });

  // ==================== ThemeData ====================
  group('AppTheme lightTheme', () {
    test('should be a valid ThemeData', () {
      expect(AppTheme.lightTheme, isA<ThemeData>());
    });

    test('should use Material 3', () {
      expect(AppTheme.lightTheme.useMaterial3, true);
    });

    test('should have correct brightness', () {
      expect(AppTheme.lightTheme.brightness, Brightness.light);
    });

    test('should have scaffold background color', () {
      expect(AppTheme.lightTheme.scaffoldBackgroundColor, isA<Color>());
    });

    test('should have app bar theme', () {
      expect(AppTheme.lightTheme.appBarTheme, isNotNull);
    });

    test('should have card theme', () {
      expect(AppTheme.lightTheme.cardTheme, isNotNull);
    });

    test('should have navigation bar theme', () {
      expect(AppTheme.lightTheme.navigationBarTheme, isNotNull);
    });

    test('should have snack bar theme', () {
      expect(AppTheme.lightTheme.snackBarTheme, isNotNull);
    });

    test('should have divider theme', () {
      expect(AppTheme.lightTheme.dividerTheme, isNotNull);
    });

    test('should have input decoration theme', () {
      expect(AppTheme.lightTheme.inputDecorationTheme, isNotNull);
    });

    test('should have color scheme', () {
      expect(AppTheme.lightTheme.colorScheme, isNotNull);
    });
  });

  group('AppTheme darkTheme', () {
    test('should be a valid ThemeData', () {
      expect(AppTheme.darkTheme, isA<ThemeData>());
    });

    test('should use Material 3', () {
      expect(AppTheme.darkTheme.useMaterial3, true);
    });

    test('should have correct brightness', () {
      expect(AppTheme.darkTheme.brightness, Brightness.dark);
    });

    test('should have scaffold background color', () {
      expect(AppTheme.darkTheme.scaffoldBackgroundColor, isA<Color>());
    });

    test('should have app bar theme', () {
      expect(AppTheme.darkTheme.appBarTheme, isNotNull);
    });

    test('should have card theme', () {
      expect(AppTheme.darkTheme.cardTheme, isNotNull);
    });

    test('should have navigation bar theme', () {
      expect(AppTheme.darkTheme.navigationBarTheme, isNotNull);
    });

    test('should have snack bar theme', () {
      expect(AppTheme.darkTheme.snackBarTheme, isNotNull);
    });

    test('should have divider theme', () {
      expect(AppTheme.darkTheme.dividerTheme, isNotNull);
    });

    test('should have input decoration theme', () {
      expect(AppTheme.darkTheme.inputDecorationTheme, isNotNull);
    });

    test('dark scaffold should be darker than light scaffold', () {
      expect(
        AppTheme.darkTheme.scaffoldBackgroundColor.computeLuminance(),
        lessThan(AppTheme.lightTheme.scaffoldBackgroundColor.computeLuminance()),
      );
    });
  });

  group('AppTheme fallbackColorScheme', () {
    test('should be a valid ColorScheme', () {
      expect(AppTheme.fallbackColorScheme, isA<ColorScheme>());
    });

    test('should have light brightness', () {
      expect(AppTheme.fallbackColorScheme.brightness, Brightness.light);
    });

    test('primary should match primaryColor', () {
      expect(AppTheme.fallbackColorScheme.primary, AppTheme.primaryColor);
    });

    test('secondary should match secondaryColor', () {
      expect(AppTheme.fallbackColorScheme.secondary, AppTheme.secondaryColor);
    });

    test('error should match errorColor', () {
      expect(AppTheme.fallbackColorScheme.error, AppTheme.errorColor);
    });
  });

  // ==================== 全局 colorScheme 兼容 ====================
  group('Global colorScheme compatibility', () {
    test('should be same as fallbackColorScheme', () {
      expect(colorScheme, AppTheme.fallbackColorScheme);
    });

    test('should have primary color', () {
      expect(colorScheme.primary, isA<Color>());
    });

    test('should have secondary color', () {
      expect(colorScheme.secondary, isA<Color>());
    });

    test('should have error color', () {
      expect(colorScheme.error, isA<Color>());
    });

    test('should have surface color', () {
      expect(colorScheme.surface, isA<Color>());
    });
  });
}
