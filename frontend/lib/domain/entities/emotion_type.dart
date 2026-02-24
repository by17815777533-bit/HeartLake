// @file emotion_type.dart
// @brief 情绪类型统一定义
// Created by 王璐瑶

library;

/// 情绪类型枚举（与后端对应）
class EmotionType {
  // 前端显示名称
  final String label;
  // 后端存储值
  final String value;
  // 对应的MoodType
  final String moodType;

  const EmotionType(this.label, this.value, this.moodType);

  // 7种核心情绪（与后端emotion_tags对应）
  static const EmotionType happy = EmotionType('开心', 'happy', 'happy');
  static const EmotionType calm = EmotionType('平静', 'calm', 'calm');
  static const EmotionType sad = EmotionType('悲伤', 'sad', 'sad');
  static const EmotionType anxious = EmotionType('焦虑', 'anxious', 'anxious');
  static const EmotionType angry = EmotionType('愤怒', 'angry', 'angry');
  static const EmotionType surprised =
      EmotionType('惊喜', 'surprised', 'surprised');
  static const EmotionType confused = EmotionType('迷茫', 'confused', 'confused');
  static const EmotionType neutral = EmotionType('中性', 'neutral', 'neutral');
  static const EmotionType hopeful = EmotionType('希望', 'hopeful', 'hopeful');
  static const EmotionType grateful = EmotionType('感恩', 'grateful', 'grateful');
  static const EmotionType lonely = EmotionType('孤独', 'lonely', 'lonely');
  static const EmotionType peaceful = EmotionType('平和', 'peaceful', 'peaceful');

  // 所有情绪类型列表
  static const List<EmotionType> all = [
    happy,
    calm,
    sad,
    anxious,
    angry,
    surprised,
    confused,
    neutral,
    hopeful,
    grateful,
    lonely,
    peaceful,
  ];

  /// 从后端值获取情绪类型
  static EmotionType fromValue(String value) {
    return all.firstWhere(
      (e) => e.value == value,
      orElse: () => neutral,
    );
  }

  /// 从标签获取情绪类型
  static EmotionType fromLabel(String label) {
    return all.firstWhere(
      (e) => e.label == label,
      orElse: () => neutral,
    );
  }

  /// 从情绪分数获取情绪类型
  /// emotion_score: -1.0 到 1.0
  static EmotionType fromScore(double score) {
    if (score > 0.6) return happy;
    if (score > 0.3) return calm;
    if (score > -0.3) return neutral;
    if (score > -0.6) return anxious;
    return sad;
  }

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is EmotionType &&
          runtimeType == other.runtimeType &&
          value == other.value;

  @override
  int get hashCode => value.hashCode;

  @override
  String toString() => 'EmotionType($label, $value)';
}

/// 情绪标签辅助工具
class EmotionTags {
  /// 将后端emotion_tags数组转换为EmotionType列表
  static List<EmotionType> parseFromBackend(List<dynamic>? tags) {
    if (tags == null || tags.isEmpty) return [EmotionType.neutral];

    return tags.map((tag) => EmotionType.fromValue(tag.toString())).toList();
  }

  /// 将EmotionType列表转换为后端格式
  static List<String> toBackend(List<EmotionType> emotions) {
    return emotions.map((e) => e.value).toList();
  }

  /// 获取主要情绪（第一个）
  static EmotionType getPrimary(List<EmotionType> emotions) {
    if (emotions.isEmpty) return EmotionType.neutral;
    return emotions.first;
  }
}
