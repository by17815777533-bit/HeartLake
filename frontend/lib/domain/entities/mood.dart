// @file mood.dart
// @brief 情绪类型定义 - 统一使用 mood_colors.dart 的 MoodType
// Created by 王璐瑶

library;

import 'package:flutter/material.dart';
import '../../utils/mood_colors.dart';

// 重新导出统一的 MoodType
export '../../utils/mood_colors.dart' show MoodType;

/// 情绪配置类 - 用于选择器等场景
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

  /// 从 MoodColorConfig 创建
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
        colors: [primaryColor.withOpacity(0.8), primaryColor.withOpacity(0.4)],
      );

  BoxDecoration get glowDecoration => BoxDecoration(
        boxShadow: [
          BoxShadow(color: glowColor.withOpacity(0.6), blurRadius: 20, spreadRadius: 5),
          BoxShadow(color: glowColor.withOpacity(0.3), blurRadius: 40, spreadRadius: 10),
        ],
      );
}

/// 情绪配置映射 - 桥接到 MoodColors
class MoodConfigs {
  static MoodConfig getConfig(MoodType type) {
    final colorConfig = MoodColors.getConfig(type);
    return MoodConfig.fromColorConfig(type, colorConfig);
  }

  static MoodType fromString(String? moodStr) => MoodColors.fromString(moodStr);

  static String moodToString(MoodType type) => type.name;

  static MoodType fromSentimentScore(double score) => MoodColors.fromSentimentScore(score);

  /// 获取所有可选情绪（完整8种）
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
