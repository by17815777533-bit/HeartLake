/// 情绪配置模型
///
/// 封装单种情绪的颜色、图标、描述等视觉属性，用于情绪选择器和展示组件。

library;

import 'package:flutter/material.dart';
import '../../utils/mood_colors.dart';

export '../../utils/mood_colors.dart' show MoodType;

/// 情绪配置
///
/// 封装单种情绪的视觉属性。
class MoodConfig {
  final MoodType type;
  final String label;
  final Color primaryColor;
  final Color glowColor;
  final Color backgroundColor;
  final String description;
  final IconData icon;

  const MoodConfig({
    required this.type,
    required this.label,
    required this.primaryColor,
    required this.glowColor,
    required this.backgroundColor,
    required this.description,
    required this.icon,
  });

  /// 从颜色配置构造
  ///
  /// 桥接MoodColorConfig到MoodConfig。
  ///
  /// [type] 情绪类型
  /// [config] 颜色配置
  factory MoodConfig.fromColorConfig(MoodType type, MoodColorConfig config) {
    return MoodConfig(
      type: type,
      label: config.name,
      primaryColor: config.primary,
      glowColor: config.rippleColor,
      backgroundColor: config.cardColor,
      description: config.description,
      icon: config.icon,
    );
  }

  LinearGradient get gradient => LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [primaryColor.withValues(alpha: 0.8), primaryColor.withValues(alpha: 0.4)],
      );

  BoxDecoration get glowDecoration => BoxDecoration(
        boxShadow: [
          BoxShadow(color: glowColor.withValues(alpha: 0.6), blurRadius: 20, spreadRadius: 5),
          BoxShadow(color: glowColor.withValues(alpha: 0.3), blurRadius: 40, spreadRadius: 10),
        ],
      );
}

/// 情绪配置查询入口
///
/// 桥接MoodColors提供统一的配置访问接口。
class MoodConfigs {
  static MoodConfig getConfig(MoodType type) {
    final colorConfig = MoodColors.getConfig(type);
    return MoodConfig.fromColorConfig(type, colorConfig);
  }

  static MoodType fromString(String? moodStr) => MoodColors.fromString(moodStr);

  static String moodToString(MoodType type) => type.name;

  static MoodType fromSentimentScore(double score) => MoodColors.fromSentimentScore(score);

  /// 获取可选情绪列表
  ///
  /// 返回全部8种可选情绪配置，按正面到负面排列。
  static List<MoodConfig> get selectableMoods {
    const types = [
      MoodType.happy,
      MoodType.calm,
      MoodType.neutral,
      MoodType.surprised,
      MoodType.confused,
      MoodType.anxious,
      MoodType.sad,
      MoodType.angry,
    ];
    return types.map((t) => getConfig(t)).toList();
  }
}
