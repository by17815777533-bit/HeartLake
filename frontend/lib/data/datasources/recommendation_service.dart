import 'base_service.dart';
import 'recommendation_response_parser.dart';
import '../../utils/input_validator.dart';
import '../../utils/mood_colors.dart';

export '../../domain/entities/recommended_stone.dart';
export '../../domain/entities/emotion_trend_point.dart';

/// 推荐服务，提供热门石头、关键词搜索和按情绪发现等推荐能力
class RecommendationService extends BaseService {
  @override
  String get serviceName => '共鸣服务';

  /// 允许传入的情绪类型
  static const _allowedMoods = MoodColors.supportedMoodKeys;

  String _resolveServiceError(
      ServiceResponse<dynamic> response, String action) {
    final message = response.message?.trim();
    if (message != null && message.isNotEmpty) {
      return message;
    }
    return '$action失败';
  }

  bool _containsListCandidate(dynamic value) {
    if (value is List) return true;
    if (value is! Map) return false;
    final source = Map<String, dynamic>.from(value.cast<String, dynamic>());
    for (final key in recommendationResponseKeys) {
      final nested = source[key];
      if (nested is List) return true;
      if (nested is Map && _containsListCandidate(nested)) return true;
    }
    return false;
  }

  void _requireRecommendationCollection(dynamic data, String action) {
    if (_containsListCandidate(data)) {
      return;
    }
    throw StateError('$action响应缺少推荐集合');
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
    _requireRecommendationCollection(response.data, action);
    return RecommendationResponseParser.extractList(response.data);
  }

  /// 获取热门推荐
  Future<List<Map<String, dynamic>>> getTrending({int limit = 20}) async {
    InputValidator.requirePageSize(limit);
    final resp = await get<dynamic>('/recommendations/trending',
        queryParameters: {'limit': limit});
    return _extractRecommendationList(resp, '获取热门推荐');
  }

  /// 搜索石头
  Future<List<Map<String, dynamic>>> search(String query) async {
    InputValidator.requireLength(query, '想找的心声', min: 1, max: 200);
    query = InputValidator.sanitizeText(query);
    final resp =
        await post<dynamic>('/recommendations/search', data: {'query': query});
    return _extractRecommendationList(resp, '搜索石头');
  }

  /// 按情绪发现
  Future<List<Map<String, dynamic>>> discoverByMood(String mood) async {
    InputValidator.validateEnum(mood, _allowedMoods, '情绪类型');
    final resp = await get<dynamic>('/recommendations/discover/$mood');
    return _extractRecommendationList(resp, '按情绪发现内容');
  }
}
