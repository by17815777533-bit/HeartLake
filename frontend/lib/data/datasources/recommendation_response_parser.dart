const List<String> recommendationResponseKeys = <String>[
  'trending_stones',
  'results',
  'stones',
  'items',
  'recommendations',
  'data',
];

/// 推荐接口响应解析器。
///
/// 统一兼容后端多种列表返回格式，避免各个推荐数据源重复维护相同的提取逻辑。
class RecommendationResponseParser {
  RecommendationResponseParser._();

  static List<Map<String, dynamic>> extractList(
    dynamic data, {
    List<String> keys = recommendationResponseKeys,
  }) {
    if (data is List) {
      return data.whereType<Map<String, dynamic>>().toList();
    }

    if (data is Map<String, dynamic>) {
      for (final key in keys) {
        if (data[key] is List) {
          return (data[key] as List).whereType<Map<String, dynamic>>().toList();
        }
      }
    }

    return [];
  }
}
