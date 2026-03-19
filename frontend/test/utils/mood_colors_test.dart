import 'package:flutter_test/flutter_test.dart';
import 'package:flutter/material.dart';
import 'package:heart_lake/utils/mood_colors.dart';

/// MoodColors 扩展测试 - 覆盖所有情绪类型、颜色属性、转换方法。

void main() {
  // ==================== MoodType 枚举 ====================
  group('MoodType enum', () {
    test('should have 8 values', () {
      expect(MoodType.values.length, 8);
    });

    test('should contain all expected types', () {
      expect(MoodType.values, containsAll([
        MoodType.happy, MoodType.calm, MoodType.sad, MoodType.anxious,
        MoodType.angry, MoodType.surprised, MoodType.confused, MoodType.neutral,
      ]));
    });

    test('each type should have a unique name', () {
      final names = MoodType.values.map((m) => m.name).toSet();
      expect(names.length, MoodType.values.length);
    });
  });

  // ==================== MoodColorConfig 属性 ====================
  group('MoodColorConfig properties', () {
    test('every mood should have a valid config', () {
      for (final mood in MoodType.values) {
        final config = MoodColors.getConfig(mood);
        expect(config, isNotNull);
        expect(config.primary, isA<Color>());
        expect(config.gradientStart, isA<Color>());
        expect(config.gradientEnd, isA<Color>());
        expect(config.lakeColor, isA<Color>());
        expect(config.rippleColor, isA<Color>());
        expect(config.textColor, isA<Color>());
        expect(config.iconColor, isA<Color>());
        expect(config.cardColor, isA<Color>());
      }
    });

    test('every mood should have a non-empty name', () {
      for (final mood in MoodType.values) {
        expect(MoodColors.getConfig(mood).name, isNotEmpty);
      }
    });

    test('every mood should have a non-empty description', () {
      for (final mood in MoodType.values) {
        expect(MoodColors.getConfig(mood).description, isNotEmpty);
      }
    });

    test('every mood should have an icon', () {
      for (final mood in MoodType.values) {
        expect(MoodColors.getConfig(mood).icon, isA<IconData>());
      }
    });

    test('primary colors should be opaque', () {
      for (final mood in MoodType.values) {
        final config = MoodColors.getConfig(mood);
        expect((config.primary.a * 255).round(), 255);
      }
    });

    test('text colors should be opaque', () {
      for (final mood in MoodType.values) {
        final config = MoodColors.getConfig(mood);
        expect((config.textColor.a * 255).round(), 255);
      }
    });
  });

  // ==================== 具体情绪名称 ====================
  group('MoodColors specific names', () {
    test('happy should be 开心', () {
      expect(MoodColors.getConfig(MoodType.happy).name, '开心');
    });

    test('calm should be 平静', () {
      expect(MoodColors.getConfig(MoodType.calm).name, '平静');
    });

    test('sad should be 悲伤', () {
      expect(MoodColors.getConfig(MoodType.sad).name, '悲伤');
    });

    test('anxious should be 焦虑', () {
      expect(MoodColors.getConfig(MoodType.anxious).name, '焦虑');
    });

    test('angry should be 愤怒', () {
      expect(MoodColors.getConfig(MoodType.angry).name, '愤怒');
    });

    test('surprised should be 惊喜', () {
      expect(MoodColors.getConfig(MoodType.surprised).name, '惊喜');
    });

    test('confused should be 迷茫', () {
      expect(MoodColors.getConfig(MoodType.confused).name, '迷茫');
    });

    test('neutral should be 中性', () {
      expect(MoodColors.getConfig(MoodType.neutral).name, '中性');
    });
  });

  // ==================== fromString ====================
  group('MoodColors.fromString', () {
    test('should map Chinese names to MoodType', () {
      expect(MoodColors.fromString('开心'), MoodType.happy);
      expect(MoodColors.fromString('平静'), MoodType.calm);
      expect(MoodColors.fromString('悲伤'), MoodType.sad);
      expect(MoodColors.fromString('焦虑'), MoodType.anxious);
      expect(MoodColors.fromString('愤怒'), MoodType.angry);
      expect(MoodColors.fromString('惊喜'), MoodType.surprised);
      expect(MoodColors.fromString('迷茫'), MoodType.confused);
    });

    test('should map English names to MoodType', () {
      expect(MoodColors.fromString('happy'), MoodType.happy);
      expect(MoodColors.fromString('calm'), MoodType.calm);
      expect(MoodColors.fromString('sad'), MoodType.sad);
      expect(MoodColors.fromString('anxious'), MoodType.anxious);
      expect(MoodColors.fromString('angry'), MoodType.angry);
      expect(MoodColors.fromString('surprised'), MoodType.surprised);
      expect(MoodColors.fromString('confused'), MoodType.confused);
      expect(MoodColors.fromString('neutral'), MoodType.neutral);
    });

    test('should map aliases', () {
      expect(MoodColors.fromString('忧伤'), MoodType.sad);
      expect(MoodColors.fromString('期待'), MoodType.calm);
      expect(MoodColors.fromString('感恩'), MoodType.happy);
      expect(MoodColors.fromString('孤独'), MoodType.sad);
      expect(MoodColors.fromString('平和'), MoodType.calm);
    });

    test('should return neutral for unknown strings', () {
      expect(MoodColors.fromString('未知'), MoodType.neutral);
      expect(MoodColors.fromString(''), MoodType.neutral);
      expect(MoodColors.fromString('xyz'), MoodType.neutral);
    });

    test('should return neutral for null', () {
      expect(MoodColors.fromString(null), MoodType.neutral);
    });
  });

  // ==================== fromSentimentScore ====================
  group('MoodColors.fromSentimentScore', () {
    test('very positive score should return happy', () {
      expect(MoodColors.fromSentimentScore(0.9), MoodType.happy);
      expect(MoodColors.fromSentimentScore(0.7), MoodType.happy);
    });

    test('positive score should return calm', () {
      expect(MoodColors.fromSentimentScore(0.4), MoodType.calm);
    });

    test('neutral score should return neutral', () {
      expect(MoodColors.fromSentimentScore(0.0), MoodType.neutral);
      expect(MoodColors.fromSentimentScore(0.1), MoodType.neutral);
      expect(MoodColors.fromSentimentScore(0.29), MoodType.neutral);
    });

    test('slightly negative score should return confused', () {
      expect(MoodColors.fromSentimentScore(-0.1), MoodType.confused);
      expect(MoodColors.fromSentimentScore(-0.2), MoodType.confused);
    });

    test('moderately negative score should return sad', () {
      expect(MoodColors.fromSentimentScore(-0.4), MoodType.sad);
      expect(MoodColors.fromSentimentScore(-0.5), MoodType.sad);
    });

    test('very negative score should return angry', () {
      expect(MoodColors.fromSentimentScore(-0.8), MoodType.angry);
      expect(MoodColors.fromSentimentScore(-1.0), MoodType.angry);
    });

    test('boundary values', () {
      // >= 0.6 -> happy
      expect(MoodColors.fromSentimentScore(0.6), MoodType.happy);
      // >= 0.3 -> calm
      expect(MoodColors.fromSentimentScore(0.3), MoodType.calm);
      // >= 0.0 -> neutral
      expect(MoodColors.fromSentimentScore(0.0), MoodType.neutral);
      // >= -0.3 -> confused
      expect(MoodColors.fromSentimentScore(-0.3), MoodType.confused);
      // >= -0.6 -> sad
      expect(MoodColors.fromSentimentScore(-0.6), MoodType.sad);
      // < -0.6 -> angry
      expect(MoodColors.fromSentimentScore(-0.61), MoodType.angry);
    });
  });

  // ==================== allMoods ====================
  group('MoodColors.allMoods', () {
    test('should return all mood types', () {
      expect(MoodColors.allMoods, MoodType.values);
    });

    test('should have correct length', () {
      expect(MoodColors.allMoods.length, 8);
    });
  });

  // ==================== createGradient ====================
  group('MoodColors.createGradient', () {
    test('should create gradient for every mood', () {
      for (final mood in MoodType.values) {
        final gradient = MoodColors.createGradient(mood);
        expect(gradient, isA<LinearGradient>());
        expect(gradient.colors.length, 2);
      }
    });

    test('should use default alignment', () {
      final gradient = MoodColors.createGradient(MoodType.happy);
      expect(gradient.begin, Alignment.topCenter);
      expect(gradient.end, Alignment.bottomCenter);
    });

    test('should accept custom alignment', () {
      final gradient = MoodColors.createGradient(
        MoodType.happy,
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
      );
      expect(gradient.begin, Alignment.topLeft);
      expect(gradient.end, Alignment.bottomRight);
    });

    test('gradient colors should match config', () {
      for (final mood in MoodType.values) {
        final config = MoodColors.getConfig(mood);
        final gradient = MoodColors.createGradient(mood);
        expect(gradient.colors[0], config.gradientStart);
        expect(gradient.colors[1], config.gradientEnd);
      }
    });
  });

  // ==================== createLakeGradient ====================
  group('MoodColors.createLakeGradient', () {
    test('should create lake gradient for every mood', () {
      for (final mood in MoodType.values) {
        final gradient = MoodColors.createLakeGradient(mood);
        expect(gradient, isA<LinearGradient>());
        expect(gradient.colors.length, 5);
        expect(gradient.stops!.length, 5);
      }
    });

    test('should have correct stops', () {
      final gradient = MoodColors.createLakeGradient(MoodType.calm);
      expect(gradient.stops, [0.0, 0.35, 0.45, 0.7, 1.0]);
    });

    test('should use top-to-bottom alignment', () {
      final gradient = MoodColors.createLakeGradient(MoodType.sad);
      expect(gradient.begin, Alignment.topCenter);
      expect(gradient.end, Alignment.bottomCenter);
    });
  });

  // ==================== 颜色唯一性 ====================
  group('MoodColors uniqueness', () {
    test('most moods should have unique primary colors', () {
      final primaries = <Color>{};
      for (final mood in MoodType.values) {
        primaries.add(MoodColors.getConfig(mood).primary);
      }
      expect(primaries.length, greaterThanOrEqualTo(8));
    });

    test('most moods should have unique names', () {
      final names = <String>{};
      for (final mood in MoodType.values) {
        names.add(MoodColors.getConfig(mood).name);
      }
      expect(names.length, MoodType.values.length);
    });
  });
}
