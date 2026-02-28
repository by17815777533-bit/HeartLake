/// 情绪趋势数据点
///
/// 记录某一天的情绪分数、类型和互动统计，用于情绪日历、趋势折线图等
/// 可视化组件。兼容后端emotion_score/score和mood/mood_type两种字段名。
class EmotionTrendPoint {
  /// 日期
  final DateTime date;

  /// 情绪分数
  ///
  /// 当日综合情绪分数，范围-1.0（消极）到1.0（积极）。
  final double emotionScore;

  /// 情绪标签
  ///
  /// 当日主要情绪类型。
  final String mood;

  /// 投石数量
  ///
  /// 当日发布的石头数量。
  final int stoneCount;

  /// 互动次数
  ///
  /// 当日互动次数（涟漪+纸船）。
  final int interactionCount;

  EmotionTrendPoint({
    required this.date,
    required this.emotionScore,
    required this.mood,
    this.stoneCount = 0,
    this.interactionCount = 0,
  });

  factory EmotionTrendPoint.fromJson(Map<String, dynamic> json) {
    return EmotionTrendPoint(
      date: DateTime.tryParse(json['date']?.toString() ?? '') ?? DateTime.now(),
      emotionScore: (json['emotion_score'] ?? json['score'] ?? 0).toDouble(),
      mood: json['mood']?.toString() ?? json['mood_type']?.toString() ?? 'neutral',
      stoneCount: json['stone_count'] ?? 0,
      interactionCount: json['interaction_count'] ?? 0,
    );
  }
}
