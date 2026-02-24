// @file recommendation_service.dart
// @brief 推荐服务 - 封装热门/搜索/情绪发现等推荐API

import 'base_service.dart';

export '../../domain/entities/recommended_stone.dart';
export '../../domain/entities/emotion_trend_point.dart';

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
      return data.whereType<Map<String, dynamic>>().toList();
    }
    if (data is Map<String, dynamic>) {
      for (final key in [
        'trending_stones', 'results', 'stones', 'items', 'recommendations', 'data'
      ]) {
        if (data[key] is List) {
          return (data[key] as List).whereType<Map<String, dynamic>>().toList();
        }
      }
    }
    return [];
  }
}
