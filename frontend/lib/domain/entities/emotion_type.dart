// @file emotion_type.dart
// @brief 情绪类型统一定义 — 基于 MoodType，消除重复枚举
// Created by 王璐瑶

library;

import 'package:heartlake/utils/mood_colors.dart';

/// 情绪标签辅助工具（桥接后端 emotion_tags 与前端 MoodType）
class EmotionTags {
  /// 将后端 emotion_tags 数组转换为 MoodType 列表
  static List<MoodType> parseFromBackend(List<dynamic>? tags) {
    if (tags == null || tags.isEmpty) return [MoodType.neutral];

    return tags.map((tag) {
      final value = tag.toString();
      try {
        return MoodType.values.firstWhere((m) => m.name == value);
      } catch (_) {
        return MoodType.neutral;
      }
    }).toList();
  }

  /// 将 MoodType 列表转换为后端格式
  static List<String> toBackend(List<MoodType> moods) {
    return moods.map((m) => m.name).toList();
  }

  /// 获取主要情绪（第一个）
  static MoodType getPrimary(List<MoodType> moods) {
    if (moods.isEmpty) return MoodType.neutral;
    return moods.first;
  }

  /// 从情绪分数获取情绪类型
  /// emotion_score: -1.0 到 1.0
  static MoodType fromScore(double score) {
    if (score > 0.6) return MoodType.happy;
    if (score > 0.3) return MoodType.calm;
    if (score > -0.3) return MoodType.neutral;
    if (score > -0.6) return MoodType.anxious;
    return MoodType.sad;
  }
}
