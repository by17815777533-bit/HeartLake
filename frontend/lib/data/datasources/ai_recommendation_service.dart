/// AI推荐服务
///
/// 封装后端AI推荐引擎的所有接口，提供多种推荐策略：
/// - 相似石头推荐：基于HNSW向量搜索的语义相似度匹配
/// - 个性化推荐：协同过滤与内容推荐的混合策略
/// - 高级共鸣推荐：情绪轨迹DTW距离计算与多维度融合
/// - 情绪趋势分析：用户情绪变化的时序数据
library;

import '../../utils/input_validator.dart';
import 'base_service.dart';

/// AI推荐服务
///
/// 封装后端AI推荐引擎的所有接口，提供多种推荐策略。
class AIRecommendationService extends BaseService {
  @override
  String get serviceName => '湖神陪伴服务';

  /// 获取相似石头推荐
  ///
  /// 基于HNSW向量相似度搜索，返回与指定石头语义相近的其他石头。
  ///
  /// [stoneId] 目标石头ID
  /// [limit] 推荐数量，默认5条
  ///
  /// 返回相似石头列表，按相似度降序排列。
  Future<List<Map<String, dynamic>>> getSimilarStones(String stoneId,
      {int limit = 5}) async {
    InputValidator.requireNonEmpty(stoneId, '石头ID');
    InputValidator.requirePositive(limit, '推荐数量');
    final resp = await get<dynamic>('/recommendations/similar-stones/$stoneId',
        queryParameters: {'limit': limit});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 获取个性化推荐
  ///
  /// 结合协同过滤、内容推荐和探索策略，为用户推荐可能感兴趣的石头。
  ///
  /// [limit] 推荐数量，默认10条
  ///
  /// 返回个性化推荐石头列表。
  Future<List<Map<String, dynamic>>> getPersonalizedRecommendations(
      {int limit = 10}) async {
    InputValidator.requirePositive(limit, '推荐数量');
    final resp = await get<dynamic>('/recommendations/stones',
        queryParameters: {'limit': limit});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 获取高级共鸣推荐
  ///
  /// 基于情绪轨迹DTW距离、语义相似度、时间衰减和多样性的综合推荐算法。
  ///
  /// [limit] 推荐数量，默认10条
  ///
  /// 返回高级共鸣推荐石头列表。
  Future<List<Map<String, dynamic>>> getAdvancedRecommendations(
      {int limit = 10}) async {
    InputValidator.requirePositive(limit, '推荐数量');
    final resp = await get<dynamic>('/recommendations/advanced',
        queryParameters: {'limit': limit});
    if (resp.success && resp.data != null) {
      return _extractList(resp.data);
    }
    return [];
  }

  /// 获取情绪趋势数据
  ///
  /// 返回用户的情绪变化时序数据，用于情绪趋势可视化。
  ///
  /// 返回包含情绪趋势点的Map。
  Future<Map<String, dynamic>> getEmotionTrends() async {
    final resp =
        await get<dynamic>('/recommendations/emotion-trends', useCache: false);
    if (resp.success && resp.data is Map<String, dynamic>) {
      return resp.data as Map<String, dynamic>;
    }
    return {};
  }

  /// 记录用户交互
  ///
  /// 用于推荐引擎的在线学习，根据用户行为调整推荐策略。
  ///
  /// [stoneId] 石头ID
  /// [interactionType] 交互类型：view/ripple/boat/share/connection
  /// [reward] 奖励值，范围0.0-1.0，默认1.0
  ///
  /// 返回是否记录成功。
  Future<bool> trackInteraction({
    required String stoneId,
    required String interactionType,
    double reward = 1.0,
  }) async {
    InputValidator.requireNonEmpty(stoneId, '石头ID');
    InputValidator.requireInList(
        interactionType,
        const [
          'view',
          'ripple',
          'boat',
          'share',
          'connection',
        ],
        '交互类型');
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
