// @file recommendation_service.dart
// @brief 推荐服务 - 情绪趋势和推荐数据
// Created by 王璐瑶

library;

import 'base_service.dart';

/// 推荐服务
class RecommendationService extends BaseService {
  @override
  String get serviceName => 'RecommendationService';

  /// 获取推荐石头
  Future<Map<String, dynamic>> getRecommendedStones({int page = 1, int pageSize = 20}) async {
    final response = await get('/recommendations/stones', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);
    return {
      'success': true,
      'stones': response.data?['recommendations'] ?? [],
    };
  }

  /// 获取情绪趋势
  Future<Map<String, dynamic>> getEmotionTrends({int days = 7}) async {
    final response = await get('/recommendations/emotion-trends', queryParameters: {
      'days': days,
    });
    if (!response.success) return toMap(response);
    return {
      'success': true,
      'trends': response.data ?? [],
    };
  }

  /// 获取高级推荐（多算法融合：UCB、Thompson Sampling、MMR）
  Future<Map<String, dynamic>> getAdvancedRecommendations({int limit = 20}) async {
    final response = await get('/recommendations/advanced', queryParameters: {
      'limit': limit.toString(),
    });
    return toMap(response);
  }

  /// 记录用户交互（用于推荐系统学习）
  Future<Map<String, dynamic>> trackInteraction({
    required String itemId,
    required String interactionType,
    double? reward,
  }) async {
    final response = await post('/recommendations/track', data: {
      'item_id': itemId,
      'interaction_type': interactionType,
      if (reward != null) 'reward': reward,
    });
    return toMap(response);
  }

  /// 按情绪发现内容
  Future<Map<String, dynamic>> discoverByMood(String mood, {int page = 1, int pageSize = 20}) async {
    final response = await get('/recommendations/discover/$mood', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 获取热门趋势内容
  Future<Map<String, dynamic>> getTrendingContent({int page = 1, int pageSize = 20}) async {
    final response = await get('/recommendations/trending', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 搜索推荐内容
  Future<Map<String, dynamic>> searchRecommendations(String query, {int page = 1, int pageSize = 20}) async {
    final response = await get('/recommendations/search', queryParameters: {
      'q': query,
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }
}

/// 情绪趋势数据点
class EmotionTrendPoint {
  final DateTime date;
  final double emotionScore;
  final String mood;
  final int stoneCount;
  final int interactionCount;

  const EmotionTrendPoint({
    required this.date,
    required this.emotionScore,
    required this.mood,
    this.stoneCount = 0,
    this.interactionCount = 0,
  });
}

/// 推荐类型
enum RecommendationType {
  similar,
  trending,
  personalized,
  random,
  collaborative,
  emotionCompatible,
  exploration,
}

/// 推荐石头
class RecommendedStone {
  final String stoneId;
  final String? authorId;
  final String? authorName;
  final String content;
  final String moodType;
  final double emotionScore;
  final int rippleCount;
  final int boatCount;
  final DateTime createdAt;
  final String recommendationReason;
  final RecommendationType recommendationType;
  final List<String>? tags;
  final List<String>? mediaUrls;

  const RecommendedStone({
    required this.stoneId,
    this.authorId,
    this.authorName,
    required this.content,
    required this.moodType,
    required this.emotionScore,
    required this.rippleCount,
    required this.boatCount,
    required this.createdAt,
    required this.recommendationReason,
    required this.recommendationType,
    this.tags,
    this.mediaUrls,
  });
}

