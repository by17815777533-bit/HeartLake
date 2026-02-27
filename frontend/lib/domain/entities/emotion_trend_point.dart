// 情绪趋势数据点模型

/// 情绪趋势数据点
class EmotionTrendPoint {
  final DateTime date;
  final double emotionScore;
  final String mood;
  final int stoneCount;
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
