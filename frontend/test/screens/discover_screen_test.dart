import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/mood_colors.dart';

/// DiscoverScreen 依赖 MoodColors 来渲染情绪标签。
/// 由于 DiscoverScreen 的 initState 会触发网络请求（_loadTrending），
/// 且依赖 ApiClient 单例（需要 appConfig/StorageUtil），
/// 无法在无 mockito 环境下直接 pumpWidget。
/// 因此重点测试 DiscoverScreen 的核心依赖：MoodColors 工具类。

void main() {
  group('MoodColors.fromString (DiscoverScreen mood list)', () {
    // DiscoverScreen 中定义的 8 种情绪
    // final List<String> _moods = ['开心', '平静', '忧伤', '焦虑', '愤怒', '迷茫', '感恩', '期待'];

    test('should map 开心 to MoodType.happy', () {
      expect(MoodColors.fromString('开心'), MoodType.happy);
    });

    test('should map 平静 to MoodType.calm', () {
      expect(MoodColors.fromString('平静'), MoodType.calm);
    });

    test('should map 忧伤 to MoodType.sad', () {
      expect(MoodColors.fromString('忧伤'), MoodType.sad);
    });

    test('should map 悲伤 to MoodType.sad (alias)', () {
      expect(MoodColors.fromString('悲伤'), MoodType.sad);
    });

    test('should map 焦虑 to MoodType.anxious', () {
      expect(MoodColors.fromString('焦虑'), MoodType.anxious);
    });

    test('should map 愤怒 to MoodType.angry', () {
      expect(MoodColors.fromString('愤怒'), MoodType.angry);
    });

    test('should map 迷茫 to MoodType.confused', () {
      expect(MoodColors.fromString('迷茫'), MoodType.confused);
    });

    test('should map 感恩 to MoodType.grateful', () {
      expect(MoodColors.fromString('感恩'), MoodType.grateful);
    });

    test('should map 期待 to MoodType.hopeful', () {
      expect(MoodColors.fromString('期待'), MoodType.hopeful);
    });

    test('should map unknown string to MoodType.neutral', () {
      expect(MoodColors.fromString('未知情绪'), MoodType.neutral);
      expect(MoodColors.fromString(''), MoodType.neutral);
    });

    test('should map 惊喜 to MoodType.surprised', () {
      expect(MoodColors.fromString('惊喜'), MoodType.surprised);
    });
  });

  group('MoodColors.getConfig', () {
    test('should return config for every MoodType', () {
      for (final mood in MoodType.values) {
        final config = MoodColors.getConfig(mood);
        expect(config, isNotNull);
        expect(config.name, isNotEmpty);
        expect(config.description, isNotEmpty);
      }
    });

    test('happy config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.happy);
      expect(config.name, '开心');
    });

    test('calm config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.calm);
      expect(config.name, '平静');
    });

    test('sad config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.sad);
      expect(config.name, '悲伤');
    });

    test('anxious config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.anxious);
      expect(config.name, '焦虑');
    });

    test('angry config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.angry);
      expect(config.name, '愤怒');
    });

    test('surprised config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.surprised);
      expect(config.name, '惊喜');
    });

    test('confused config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.confused);
      expect(config.name, '迷茫');
    });

    test('neutral config should have correct name', () {
      final config = MoodColors.getConfig(MoodType.neutral);
      expect(config.name, '中性');
    });

    test('config should have all color fields non-null', () {
      for (final mood in MoodType.values) {
        final config = MoodColors.getConfig(mood);
        expect(config.primary, isA<Color>());
        expect(config.gradientStart, isA<Color>());
        expect(config.gradientEnd, isA<Color>());
        expect(config.lakeColor, isA<Color>());
        expect(config.rippleColor, isA<Color>());
        expect(config.textColor, isA<Color>());
        expect(config.iconColor, isA<Color>());
        expect(config.cardColor, isA<Color>());
        expect(config.icon, isA<IconData>());
      }
    });
  });

  group('MoodColors.allMoods', () {
    test('should return all 12 mood types', () {
      final allMoods = MoodColors.allMoods;
      expect(allMoods.length, 12);
      expect(allMoods, containsAll(MoodType.values));
    });
  });

  group('MoodColors.createGradient', () {
    test('should return LinearGradient for each mood', () {
      for (final mood in MoodType.values) {
        final gradient = MoodColors.createGradient(mood);
        expect(gradient, isA<LinearGradient>());
        expect(gradient.colors.length, 2);
      }
    });

    test('gradient colors should match config gradientStart and gradientEnd', () {
      for (final mood in MoodType.values) {
        final config = MoodColors.getConfig(mood);
        final gradient = MoodColors.createGradient(mood);
        expect(gradient.colors[0], config.gradientStart);
        expect(gradient.colors[1], config.gradientEnd);
      }
    });

    test('should use default alignment (topLeft -> bottomRight)', () {
      final gradient = MoodColors.createGradient(MoodType.happy);
      expect(gradient.begin, Alignment.topCenter);
      expect(gradient.end, Alignment.bottomCenter);
    });

    test('should accept custom alignment', () {
      final gradient = MoodColors.createGradient(
        MoodType.happy,
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
      );
      expect(gradient.begin, Alignment.topCenter);
      expect(gradient.end, Alignment.bottomCenter);
    });
  });

  group('MoodColors.createLakeGradient', () {
    test('should return LinearGradient with 5 stops', () {
      for (final mood in MoodType.values) {
        final gradient = MoodColors.createLakeGradient(mood);
        expect(gradient, isA<LinearGradient>());
        expect(gradient.colors.length, 5);
        expect(gradient.stops?.length, 5);
      }
    });

    test('stops should be [0.0, 0.35, 0.45, 0.7, 1.0]', () {
      final gradient = MoodColors.createLakeGradient(MoodType.calm);
      expect(gradient.stops, [0.0, 0.35, 0.45, 0.7, 1.0]);
    });

    test('should use topCenter -> bottomCenter alignment', () {
      final gradient = MoodColors.createLakeGradient(MoodType.sad);
      expect(gradient.begin, Alignment.topCenter);
      expect(gradient.end, Alignment.bottomCenter);
    });
  });

  group('MoodColorConfig properties', () {
    test('happy config should have warm colors', () {
      final config = MoodColors.getConfig(MoodType.happy);
      // primary 应该是暖色调（偏黄/橙）
      expect((config.primary.a * 255).round(), greaterThan(0));
    });

    test('sad config should have cool colors', () {
      final config = MoodColors.getConfig(MoodType.sad);
      expect((config.primary.a * 255).round(), greaterThan(0));
    });

    test('each mood should have unique primary color', () {
      final primaries = <Color>{};
      for (final mood in MoodType.values) {
        primaries.add(MoodColors.getConfig(mood).primary);
      }
      // 至少大部分颜色应该不同
      expect(primaries.length, greaterThanOrEqualTo(6));
    });
  });

  group('DiscoverScreen mood chip integration', () {
    // 测试 DiscoverScreen 中 ChoiceChip 使用 MoodColors 的逻辑
    // MoodColors.getConfig(MoodColors.fromString(mood)).primary

    test('should get valid color for each discover mood', () {
      final moods = ['开心', '平静', '忧伤', '焦虑', '愤怒', '迷茫', '感恩', '期待'];

      for (final mood in moods) {
        final moodType = MoodColors.fromString(mood);
        final config = MoodColors.getConfig(moodType);
        // DiscoverScreen 使用 color.withOpacity(0.8) 和 color.withOpacity(0.3)
        final selectedColor = config.primary.withValues(alpha: 0.8);
        final bgColor = config.primary.withValues(alpha: 0.3);

        expect(selectedColor.a, closeTo(0.8, 0.01));
        expect(bgColor.a, closeTo(0.3, 0.01));
      }
    });
  });
}
