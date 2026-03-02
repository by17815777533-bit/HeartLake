/// 情绪标签工具类
///
/// 负责后端emotion_tags与前端MoodType之间的转换。

library;

import 'package:heart_lake/utils/mood_colors.dart';

/// 情绪标签工具类
///
/// 提供情绪标签的转换和解析功能。
class EmotionTags {
  /// 解析后端情绪标签
  ///
  /// 将后端emotion_tags数组转换为MoodType列表。
  ///
  /// [tags] 后端情绪标签数组
  static List<MoodType> parseFromBackend(List<dynamic>? tags) {
    if (tags == null || tags.isEmpty) return [MoodType.neutral];

    return tags.map((tag) => MoodColors.fromString(tag.toString())).toList();
  }

  /// 转换为后端格式
  ///
  /// 将MoodType列表转换为后端格式的字符串数组。
  ///
  /// [moods] 情绪类型列表
  static List<String> toBackend(List<MoodType> moods) {
    return moods.map((m) => m.name).toList();
  }

  /// 获取主要情绪
  ///
  /// 返回列表中的第一个情绪，如果列表为空则返回neutral。
  ///
  /// [moods] 情绪类型列表
  static MoodType getPrimary(List<MoodType> moods) {
    if (moods.isEmpty) return MoodType.neutral;
    return moods.first;
  }

  /// 从情绪分数获取情绪类型
  ///
  /// 根据情绪分数（-1.0到1.0）映射到对应的情绪类型。
  ///
  /// [score] 情绪分数，范围-1.0到1.0
  static MoodType fromScore(double score) {
    if (score > 0.6) return MoodType.happy;
    if (score > 0.3) return MoodType.calm;
    if (score > -0.3) return MoodType.neutral;
    if (score > -0.6) return MoodType.anxious;
    return MoodType.sad;
  }
}
