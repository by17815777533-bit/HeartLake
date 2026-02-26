// @file ai_recommendation_service.dart
// @brief AI 推荐服务 - 封装所有 AI 推荐相关 API
// Created by 白洋

library;

import '../../utils/input_validator.dart';
import 'base_service.dart';

/// AI 推荐服务
///
/// 封装后端 AI 推荐引擎的所有 API：
/// - 相似石头推荐 (HNSW 向量搜索)
/// - 个性化推荐 (协同过滤 + 内容推荐)
/// - 高级共鸣推荐 (情绪轨迹 DTW + 多维度融合)
/// - 情绪趋势分析
class AIRecommendationService extends BaseService {
  @override
  String get serviceName => 'AIRecommendation';

  /// 获取相似石头推荐 (基于 HNSW 向量相似度)
  ///
  /// 后端路由: GET /api/recommendations/similar-stones/{stoneId}
  /// 来源: VectorSearchController
  Future<List<Map<String, dynamic>>> getSimilarStones(String stoneId, {int limit = 5}) async {
    InputValidator.requireNonEmpty(stoneId, '石头ID');
    InputValidator.requirePositive(limit, '推荐数量');
    final resp = await get<dynamic>('/recommendations/similar-stones/$stoneId',
        queryParameters: {'limit': limit});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 获取个性化推荐 (协同过滤 + 内容推荐 + 探索)
  Future<List<Map<String, dynamic>>> getPersonalizedRecommendations({int limit = 10}) async {
    InputValidator.requirePositive(limit, '推荐数量');
    final resp = await get<dynamic>('/recommendations/stones',
        queryParameters: {'limit': limit});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 获取高级共鸣推荐 (情绪轨迹 DTW + 语义相似度 + 时间衰减 + 多样性)
  Future<List<Map<String, dynamic>>> getAdvancedRecommendations({int limit = 10}) async {
    InputValidator.requirePositive(limit, '推荐数量');
    final resp = await get<dynamic>('/recommendations/advanced',
        queryParameters: {'limit': limit});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 获取情绪趋势数据
  Future<Map<String, dynamic>> getEmotionTrends() async {
    final resp = await get<dynamic>('/recommendations/emotion-trends');
    if (resp.success && resp.data is Map<String, dynamic>) {
      return resp.data as Map<String, dynamic>;
    }
    return {};
  }

  /// 记录用户交互 (用于推荐引擎在线学习)
  Future<bool> trackInteraction({
    required String stoneId,
    required String interactionType,
    double reward = 1.0,
  }) async {
    InputValidator.requireNonEmpty(stoneId, '石头ID');
    InputValidator.requireInList(interactionType, const [
      'view', 'ripple', 'boat', 'share', 'connection',
    ], '交互类型');
    InputValidator.requireDoubleRange(reward, '奖励值', min: 0.0, max: 1.0);
    final resp = await post<dynamic>('/recommendations/track', data: {
      'stone_id': stoneId,
      'interaction_type': interactionType,
      'reward': reward,
    });
    return resp.success;
  }

  List<Map<String, dynamic>> _extractList(dynamic data) {
    if (data is List) {
      return data.whereType<Map<String, dynamic>>().toList();
    }
    if (data is Map<String, dynamic>) {
      for (final key in ['stones', 'items', 'recommendations', 'data']) {
        if (data[key] is List) {
          return (data[key] as List).whereType<Map<String, dynamic>>().toList();
        }
      }
    }
    return [];
  }
}
