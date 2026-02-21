// @file recommendation_service.dart
// @brief 推荐服务 - 封装热门/搜索/情绪发现等推荐API

import 'base_service.dart';

/// 推荐类型枚举
enum RecommendationType {
  similar,
  collaborative,
  emotionCompatible,
  exploration,
  trending,
  personalized,
  random,
}

/// 推荐石头模型
class RecommendedStone {
  final String stoneId;
  final String content;
  final String? authorId;
  final String? authorName;
  final String? moodType;
  final double? emotionScore;
  final int rippleCount;
  final int boatCount;
  final DateTime? createdAt;
  final List<String>? tags;
  final List<String>? mediaUrls;
  final RecommendationType recommendationType;
  final String recommendationReason;
  final double score;

  RecommendedStone({
    required this.stoneId,
    required this.content,
    this.authorId,
    this.authorName,
    this.moodType,
    this.emotionScore,
    this.rippleCount = 0,
    this.boatCount = 0,
    this.createdAt,
    this.tags,
    this.mediaUrls,
    this.recommendationType = RecommendationType.random,
    this.recommendationReason = '',
    this.score = 0.0,
  });

  factory RecommendedStone.fromJson(Map<String, dynamic> json) {
    return RecommendedStone(
      stoneId: json['stone_id']?.toString() ?? '',
      content: json['content']?.toString() ?? '',
      authorId: json['author_id']?.toString(),
      authorName: json['author_name']?.toString() ?? json['nickname']?.toString(),
      moodType: json['mood_type']?.toString(),
      emotionScore: (json['emotion_score'] ?? json['sentiment_score'])?.toDouble(),
      rippleCount: json['ripple_count'] ?? 0,
      boatCount: json['boat_count'] ?? 0,
      createdAt: json['created_at'] != null ? DateTime.tryParse(json['created_at'].toString()) : null,
      tags: (json['tags'] as List?)?.map((e) => e.toString()).toList(),
      mediaUrls: (json['media_urls'] as List?)?.map((e) => e.toString()).toList(),
      recommendationType: _parseType(json['recommendation_type']),
      recommendationReason: json['recommendation_reason']?.toString() ?? '为你推荐',
      score: (json['score'] ?? json['relevance_score'] ?? 0).toDouble(),
    );
  }

  static RecommendationType _parseType(dynamic type) {
    if (type == null) return RecommendationType.random;
    final s = type.toString().toLowerCase();
    if (s.contains('similar')) return RecommendationType.similar;
    if (s.contains('collaborat')) return RecommendationType.collaborative;
    if (s.contains('emotion')) return RecommendationType.emotionCompatible;
    if (s.contains('explor')) return RecommendationType.exploration;
    if (s.contains('trend')) return RecommendationType.trending;
    if (s.contains('personal')) return RecommendationType.personalized;
    return RecommendationType.random;
  }
}

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

class RecommendationService extends BaseService {
  @override
  String get serviceName => 'Recommendation';

  /// 获取热门推荐
  Future<List<Map<String, dynamic>>> getTrending({int limit = 20}) async {
    final resp = await get<dynamic>('/recommendations/trending',
        queryParameters: {'limit': limit});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 搜索石头
  Future<List<Map<String, dynamic>>> search(String query) async {
    final resp = await post<dynamic>('/recommendations/search',
        data: {'query': query});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 按情绪发现
  Future<List<Map<String, dynamic>>> discoverByMood(String mood) async {
    final resp = await get<dynamic>('/recommendations/discover/$mood');
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  List<Map<String, dynamic>> _extractList(dynamic data) {
    if (data is List) {
      return data.cast<Map<String, dynamic>>();
    }
    if (data is Map<String, dynamic>) {
      for (final key in [
        'trending_stones', 'results', 'stones', 'items', 'recommendations', 'data'
      ]) {
        if (data[key] is List) {
          return (data[key] as List).cast<Map<String, dynamic>>();
        }
      }
    }
    return [];
  }
}
