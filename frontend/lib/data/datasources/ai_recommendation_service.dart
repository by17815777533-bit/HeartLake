/// AI推荐服务
///
/// 封装后端AI推荐引擎的所有接口，提供多种推荐策略：
/// - 相似石头推荐：基于HNSW向量搜索的语义相似度匹配
/// - 个性化推荐：协同过滤与内容推荐的混合策略
/// - 高级共鸣推荐：情绪轨迹DTW距离计算与多维度融合
/// - 情绪趋势分析：用户情绪变化的时序数据
library;

import '../../utils/input_validator.dart';
import '../../utils/payload_contract.dart';
import 'base_service.dart';
import 'recommendation_response_parser.dart';
import 'social_payload_normalizer.dart';

/// AI推荐服务
///
/// 封装后端AI推荐引擎的所有接口，提供多种推荐策略。
class AIRecommendationService extends BaseService {
  @override
  String get serviceName => '湖神陪伴服务';

  static const _trendListKeys = ['trends', 'items', 'list', 'results'];

  String _resolveServiceError(
      ServiceResponse<dynamic> response, String action) {
    final message = response.message?.trim();
    if (message != null && message.isNotEmpty) {
      return message;
    }
    return '$action失败';
  }

  List<Map<String, dynamic>> _extractRecommendationList(
    ServiceResponse<dynamic> response,
    String action,
  ) {
    if (!response.success) {
      throw StateError(_resolveServiceError(response, action));
    }
    if (response.data == null) {
      throw StateError('$action响应缺少 data');
    }
    return RecommendationResponseParser.extractList(response.data);
  }

  int? _toInt(dynamic value) {
    if (value is int) return value;
    if (value is num) return value.toInt();
    if (value is String) return int.tryParse(value.trim());
    return null;
  }

  double? _toDouble(dynamic value) {
    if (value is double) return value;
    if (value is num) return value.toDouble();
    if (value is String) return double.tryParse(value.trim());
    return null;
  }

  Map<String, dynamic> _normalizeEmotionTrendRow(Map<String, dynamic> item) {
    final normalized = normalizePayloadContract(item);

    final date =
        normalized['date'] ?? normalized['day'] ?? normalized['created_at'];
    if (date != null) {
      normalized['date'] = date.toString();
      normalized['day'] = date.toString();
    }

    final mood = (normalized['mood'] ?? normalized['mood_type'] ?? 'neutral')
        .toString()
        .toLowerCase();
    normalized['mood'] = mood;
    normalized['mood_type'] = mood;

    final stoneCount = _toInt(normalized['stone_count'] ?? normalized['count']);
    if (stoneCount != null) {
      normalized['stone_count'] = stoneCount;
      normalized['count'] = stoneCount;
    }

    final emotionScore =
        _toDouble(normalized['emotion_score'] ?? normalized['score']);
    if (emotionScore != null) {
      normalized['emotion_score'] = emotionScore;
      normalized['score'] = emotionScore;
    }

    return normalized;
  }

  Map<String, dynamic> _normalizeEmotionTrendsPayload(
    dynamic raw, {
    required int requestedDays,
  }) {
    if (raw is! Map) {
      throw StateError('Emotion trends payload is not a map');
    }

    final source = Map<String, dynamic>.from(raw);
    final nestedData = source['data'] is Map
        ? Map<String, dynamic>.from(source['data'] as Map)
        : source;
    final hasTrendList = _trendListKeys.any(source.containsKey) ||
        _trendListKeys.any(nestedData.containsKey);
    if (!hasTrendList) {
      throw StateError('Emotion trends payload is missing trends collection');
    }
    final trends = extractNormalizedList(
      raw,
      itemNormalizer: _normalizeEmotionTrendRow,
      listKeys: _trendListKeys,
    );
    final periodDays =
        _toInt(nestedData['period_days'] ?? source['period_days']) ??
            requestedDays;

    return buildCollectionEnvelope(
      raw,
      primaryKey: 'trends',
      items: trends,
      extra: {
        'period_days': periodDays,
      },
    );
  }

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
    return _extractRecommendationList(resp, '获取相似石头推荐');
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
    return _extractRecommendationList(resp, '获取个性化推荐');
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
    return _extractRecommendationList(resp, '获取高级共鸣推荐');
  }

  /// 获取情绪趋势数据
  ///
  /// 返回用户的情绪变化时序数据，用于情绪趋势可视化。
  ///
  /// 返回包含情绪趋势点的Map。
  Future<Map<String, dynamic>> getEmotionTrends({int days = 30}) async {
    if (days < 1 || days > 365) {
      throw const ValidationException('情绪趋势天数应在1-365之间');
    }
    final resp = await get<dynamic>(
      '/recommendations/emotion-trends',
      queryParameters: {'days': days},
      useCache: false,
    );
    if (!resp.success) return toMap(resp);

    final payload = _normalizeEmotionTrendsPayload(
      resp.data,
      requestedDays: days,
    );
    return {
      ...toMap(resp),
      'data': payload,
      ...payload,
    };
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
}
