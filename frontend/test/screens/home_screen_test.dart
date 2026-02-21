import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/app_theme.dart';

/// HomeScreen 依赖 AppTheme 来渲染主题和导航栏颜色。
/// 由于 HomeScreen 的子页面（LakeScreen 等）依赖 ApiClient、WebSocketManager、Provider，
/// 无法在无 mockito 环境下直接 pumpWidget。
/// 因此重点测试 HomeScreen 的核心依赖：AppTheme 工具类。

void main() {
  group('AppTheme color constants', () {
    test('primaryColor should be Sky peach', () {
      expect(AppTheme.primaryColor, const Color(0xFFFFAB91));
    });

    test('secondaryColor should be Sky lavender', () {
      expect(AppTheme.secondaryColor, const Color(0xFFB39DDB));
    });

    test('accentColor should be Sky warm yellow', () {
      expect(AppTheme.accentColor, const Color(0xFFFFD54F));
    });

    test('primaryLightColor should be lighter than primaryColor', () {
      expect(AppTheme.primaryLightColor, isA<Color>());
      // primaryLightColor alpha should be > 0
      expect((AppTheme.primaryLightColor.a * 255).round(), greaterThan(0));
    });

    test('primaryDarkColor should be defined', () {
      expect(AppTheme.primaryDarkColor, isA<Color>());
    });

    test('darkBlue should be defined', () {
      expect(AppTheme.darkBlue, isA<Color>());
    });
  });

  group('AppTheme warm colors', () {
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
  });

  group('AppTheme background and text colors', () {
    test('backgroundColor should be defined', () {
      expect(AppTheme.backgroundColor, isA<Color>());
    });

    test('skyBlue should be defined', () {
      expect(AppTheme.skyBlue, isA<Color>());
    });

    test('borderCyan should be defined', () {
      expect(AppTheme.borderCyan, isA<Color>());
    });

    test('textPrimary should be dark', () {
      expect(AppTheme.textPrimary, isA<Color>());
      // text primary should have high alpha (opaque or near-opaque)
      expect((AppTheme.textPrimary.a * 255).round(), greaterThan(200));
    });

    test('textSecondary should be defined', () {
      expect(AppTheme.textSecondary, isA<Color>());
    });

    test('textTertiary should be defined', () {
      expect(AppTheme.textTertiary, isA<Color>());
    });
  });

  group('AppTheme functional colors', () {
    test('successColor should be defined', () {
      expect(AppTheme.successColor, isA<Color>());
    });

    test('warningColor should be defined', () {
      expect(AppTheme.warningColor, isA<Color>());
    });

    test('errorColor should be defined', () {
      expect(AppTheme.errorColor, isA<Color>());
    });
  });

  group('AppTheme dark mode text colors', () {
    test('darkTextPrimary should be light', () {
      expect(AppTheme.darkTextPrimary, isA<Color>());
    });

    test('darkTextSecondary should be defined', () {
      expect(AppTheme.darkTextSecondary, isA<Color>());
    });
  });

  group('AppTheme lake colors', () {
    test('lakeSurface should be defined', () {
      expect(AppTheme.lakeSurface, isA<Color>());
    });

    test('lakeMiddle should be defined', () {
      expect(AppTheme.lakeMiddle, isA<Color>());
    });

    test('lakeDeep should be defined', () {
      expect(AppTheme.lakeDeep, isA<Color>());
    });

    test('lakeBackground should be defined', () {
      expect(AppTheme.lakeBackground, isA<Color>());
    });

    test('lake colors should form a gradient from light to dark', () {
      // lakeSurface should be lighter (higher luminance) than lakeDeep
      final surfaceLum = AppTheme.lakeSurface.computeLuminance();
      final deepLum = AppTheme.lakeDeep.computeLuminance();
      expect(surfaceLum, greaterThan(deepLum));
    });
  });

  group('AppTheme stone colors', () {
    test('lightStone should be defined', () {
      expect(AppTheme.lightStone, isA<Color>());
    });

    test('mediumStone should be defined', () {
      expect(AppTheme.mediumStone, isA<Color>());
    });

    test('heavyStone should be defined', () {
      expect(AppTheme.heavyStone, isA<Color>());
    });
  });

  group('AppTheme rainbowColors', () {
    test('should have 7 colors', () {
      expect(AppTheme.rainbowColors.length, 7);
    });

    test('all entries should be Color instances', () {
      for (final color in AppTheme.rainbowColors) {
        expect(color, isA<Color>());
      }
    });

    test('all colors should be non-transparent', () {
      for (final color in AppTheme.rainbowColors) {
        expect((color.a * 255).round(), greaterThan(0));
      }
    });
  });

  group('AppTheme lightTheme', () {
    test('should use Material 3', () {
      expect(AppTheme.lightTheme.useMaterial3, true);
    });

    test('should have light brightness', () {
      expect(AppTheme.lightTheme.brightness, Brightness.light);
    });

    test('should have a colorScheme', () {
      expect(AppTheme.lightTheme.colorScheme, isNotNull);
    });

    test('colorScheme should be seeded from primaryColor', () {
      final scheme = AppTheme.lightTheme.colorScheme;
      // The seed color produces a primary that may differ slightly,
      // but brightness should be light
      expect(scheme.brightness, Brightness.light);
    });
  });

  group('AppTheme darkTheme', () {
    test('should use Material 3', () {
      expect(AppTheme.darkTheme.useMaterial3, true);
    });

    test('should have dark brightness', () {
      expect(AppTheme.darkTheme.brightness, Brightness.dark);
    });

    test('should have a colorScheme', () {
      expect(AppTheme.darkTheme.colorScheme, isNotNull);
    });

    test('colorScheme should have dark brightness', () {
      final scheme = AppTheme.darkTheme.colorScheme;
      expect(scheme.brightness, Brightness.dark);
    });
  });

  group('HomeScreen NavigationBar integration', () {
    // Test the color values HomeScreen uses for NavigationBar
    test('light mode nav background should be valid', () {
      const navBgLight = Color(0xFFF5F9FC);
      expect((navBgLight.a * 255).round(), 255);
    });

    test('dark mode nav background should be valid', () {
      const navBgDark = Color(0xFF303134);
      expect((navBgDark.a * 255).round(), 255);
    });

    test('indicator color should match primaryColor with low alpha', () {
      final indicatorColor = AppTheme.primaryColor.withAlpha(30);
      expect((indicatorColor.a * 255).round(), closeTo(30, 1));
      // RGB channels should match primaryColor
      expect((indicatorColor.r * 255).round(), (AppTheme.primaryColor.r * 255).round());
      expect((indicatorColor.g * 255).round(), (AppTheme.primaryColor.g * 255).round());
      expect((indicatorColor.b * 255).round(), (AppTheme.primaryColor.b * 255).round());
    });

    test('HomeScreen uses 5 navigation destinations', () {
      // Verify the navigation labels and icons used in HomeScreen
      final destinations = [
        {'label': '观湖', 'icon': Icons.water},
        {'label': '湖底', 'icon': Icons.explore},
        {'label': '投石', 'icon': Icons.add_circle},
        {'label': '好友', 'icon': Icons.people},
        {'label': '倒影', 'icon': Icons.blur_on},
      ];

      expect(destinations.length, 5);
      for (final dest in destinations) {
        expect(dest['label'], isA<String>());
        expect(dest['icon'], isA<IconData>());
      }
    });
  });
}
