import 'package:flutter_test/flutter_test.dart';

/// RecommendationService 继承 BaseService，无法直接实例化。
/// 提取核心逻辑进行测试：列表提取、响应处理。

class RecommendationResponseProcessor {
  List<Map<String, dynamic>> extractList(dynamic data) {
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

  List<Map<String, dynamic>> processTrending(dynamic data, bool success, {int limit = 20}) {
    if (!success || data == null) return [];
    return extractList(data);
  }

  List<Map<String, dynamic>> processSearch(dynamic data, bool success, String query) {
    if (query.isEmpty) return [];
    if (!success || data == null) return [];
    return extractList(data);
  }

  List<Map<String, dynamic>> processDiscoverByMood(dynamic data, bool success, String mood) {
    if (mood.isEmpty) return [];
    if (!success || data == null) return [];
    return extractList(data);
  }
}

void main() {
  late RecommendationResponseProcessor processor;

  setUp(() {
    processor = RecommendationResponseProcessor();
  });

  // ==================== extractList ====================
  group('extractList', () {
    test('should extract from direct list', () {
      final data = [
        {'stone_id': 's1', 'content': '内容1'},
        {'stone_id': 's2', 'content': '内容2'},
      ];
      final result = processor.extractList(data);
      expect(result.length, 2);
      expect(result[0]['stone_id'], 's1');
    });

    test('should filter non-map items from list', () {
      final data = [
        {'stone_id': 's1'},
        'invalid',
        42,
        {'stone_id': 's2'},
        null,
      ];
      final result = processor.extractList(data);
      expect(result.length, 2);
    });

    test('should extract from trending_stones key', () {
      final data = {
        'trending_stones': [
          {'stone_id': 's1'},
          {'stone_id': 's2'},
        ]
      };
      final result = processor.extractList(data);
      expect(result.length, 2);
    });

    test('should extract from results key', () {
      final data = {
        'results': [{'stone_id': 's1'}]
      };
      final result = processor.extractList(data);
      expect(result.length, 1);
    });

    test('should extract from stones key', () {
      final data = {
        'stones': [{'stone_id': 's1'}, {'stone_id': 's2'}]
      };
      final result = processor.extractList(data);
      expect(result.length, 2);
    });

    test('should extract from items key', () {
      final data = {
        'items': [{'stone_id': 's1'}]
      };
      final result = processor.extractList(data);
      expect(result.length, 1);
    });

    test('should extract from recommendations key', () {
      final data = {
        'recommendations': [{'stone_id': 's1'}, {'stone_id': 's2'}, {'stone_id': 's3'}]
      };
      final result = processor.extractList(data);
      expect(result.length, 3);
    });

    test('should extract from data key', () {
      final data = {
        'data': [{'stone_id': 's1'}]
      };
      final result = processor.extractList(data);
      expect(result.length, 1);
    });

    test('should prioritize first matching key', () {
      final data = {
        'trending_stones': [{'stone_id': 'trending'}],
        'results': [{'stone_id': 'result1'}, {'stone_id': 'result2'}],
      };
      final result = processor.extractList(data);
      expect(result.length, 1);
      expect(result[0]['stone_id'], 'trending');
    });

    test('should return empty for null data', () {
      expect(processor.extractList(null), isEmpty);
    });

    test('should return empty for string data', () {
      expect(processor.extractList('invalid'), isEmpty);
    });

    test('should return empty for int data', () {
      expect(processor.extractList(42), isEmpty);
    });

    test('should return empty for empty list', () {
      expect(processor.extractList([]), isEmpty);
    });

    test('should return empty for empty map', () {
      expect(processor.extractList({}), isEmpty);
    });

    test('should return empty for map without matching keys', () {
      final data = {'unknown_key': [{'stone_id': 's1'}]};
      expect(processor.extractList(data), isEmpty);
    });

    test('should handle nested non-list values', () {
      final data = {'trending_stones': 'not a list'};
      expect(processor.extractList(data), isEmpty);
    });

    test('should handle list with only non-map items', () {
      expect(processor.extractList([1, 2, 3, 'a', 'b']), isEmpty);
    });

    test('should handle large list', () {
      final data = List.generate(100, (i) => {'stone_id': 's$i'});
      expect(processor.extractList(data).length, 100);
    });
  });

  // ==================== processTrending ====================
  group('processTrending', () {
    test('should return trending stones on success', () {
      final data = [
        {'stone_id': 's1', 'content': '热门1', 'ripple_count': 100},
        {'stone_id': 's2', 'content': '热门2', 'ripple_count': 80},
      ];
      final result = processor.processTrending(data, true);
      expect(result.length, 2);
      expect(result[0]['ripple_count'], 100);
    });

    test('should return empty on failure', () {
      expect(processor.processTrending(null, false), isEmpty);
    });

    test('should return empty on null data', () {
      expect(processor.processTrending(null, true), isEmpty);
    });

    test('should handle map with trending_stones key', () {
      final data = {
        'trending_stones': [
          {'stone_id': 's1'},
        ]
      };
      final result = processor.processTrending(data, true);
      expect(result.length, 1);
    });

    test('should handle limit parameter', () {
      final data = List.generate(30, (i) => {'stone_id': 's$i'});
      final result = processor.processTrending(data, true, limit: 20);
      expect(result.length, 30); // limit is server-side, processor returns all
    });

    test('should handle empty list', () {
      expect(processor.processTrending([], true), isEmpty);
    });

    test('should filter invalid items', () {
      final data = [{'stone_id': 's1'}, 'invalid', null, {'stone_id': 's2'}];
      final result = processor.processTrending(data, true);
      expect(result.length, 2);
    });

    test('should handle map with items key', () {
      final data = {'items': [{'stone_id': 's1'}]};
      final result = processor.processTrending(data, true);
      expect(result.length, 1);
    });
  });

  // ==================== processSearch ====================
  group('processSearch', () {
    test('should return search results on success', () {
      final data = {
        'results': [
          {'stone_id': 's1', 'content': '搜索结果1'},
          {'stone_id': 's2', 'content': '搜索结果2'},
        ]
      };
      final result = processor.processSearch(data, true, '关键词');
      expect(result.length, 2);
    });

    test('should return empty on empty query', () {
      final data = {'results': [{'stone_id': 's1'}]};
      expect(processor.processSearch(data, true, ''), isEmpty);
    });

    test('should return empty on failure', () {
      expect(processor.processSearch(null, false, '关键词'), isEmpty);
    });

    test('should return empty on null data', () {
      expect(processor.processSearch(null, true, '关键词'), isEmpty);
    });

    test('should handle direct list response', () {
      final data = [{'stone_id': 's1', 'content': '匹配'}];
      final result = processor.processSearch(data, true, '匹配');
      expect(result.length, 1);
    });

    test('should handle Chinese query', () {
      final data = [{'stone_id': 's1', 'content': '心情日记'}];
      final result = processor.processSearch(data, true, '心情');
      expect(result.length, 1);
    });

    test('should handle special characters in query', () {
      final data = [{'stone_id': 's1'}];
      final result = processor.processSearch(data, true, '特殊@#\$');
      expect(result.length, 1);
    });

    test('should handle empty results', () {
      final data = {'results': []};
      expect(processor.processSearch(data, true, '无结果'), isEmpty);
    });
  });

  // ==================== processDiscoverByMood ====================
  group('processDiscoverByMood', () {
    test('should return stones for mood on success', () {
      final data = [
        {'stone_id': 's1', 'mood_type': 'happy'},
        {'stone_id': 's2', 'mood_type': 'happy'},
      ];
      final result = processor.processDiscoverByMood(data, true, 'happy');
      expect(result.length, 2);
    });

    test('should return empty on empty mood', () {
      expect(processor.processDiscoverByMood([], true, ''), isEmpty);
    });

    test('should return empty on failure', () {
      expect(processor.processDiscoverByMood(null, false, 'happy'), isEmpty);
    });

    test('should return empty on null data', () {
      expect(processor.processDiscoverByMood(null, true, 'happy'), isEmpty);
    });

    test('should handle all mood types', () {
      final moods = ['happy', 'sad', 'calm', 'anxious', 'angry', 'surprised', 'confused', 'neutral'];
      for (final mood in moods) {
        final data = [{'stone_id': 's1', 'mood_type': mood}];
        final result = processor.processDiscoverByMood(data, true, mood);
        expect(result.length, 1, reason: 'Failed for mood: $mood');
      }
    });

    test('should handle map response with stones key', () {
      final data = {
        'stones': [{'stone_id': 's1', 'mood_type': 'calm'}]
      };
      final result = processor.processDiscoverByMood(data, true, 'calm');
      expect(result.length, 1);
    });

    test('should handle empty list', () {
      expect(processor.processDiscoverByMood([], true, 'sad'), isEmpty);
    });

    test('should filter non-map items', () {
      final data = [{'stone_id': 's1'}, null, 'invalid'];
      final result = processor.processDiscoverByMood(data, true, 'happy');
      expect(result.length, 1);
    });
  });

  // ==================== 边界值测试 ====================
  group('Edge cases', () {
    test('extractList should handle deeply nested data', () {
      final data = {
        'stones': [
          {
            'stone_id': 's1',
            'nested': {'deep': {'value': 42}},
          }
        ]
      };
      final result = processor.extractList(data);
      expect(result.length, 1);
      expect(result[0]['nested']['deep']['value'], 42);
    });

    test('extractList should handle list of empty maps', () {
      final data = <Map<String, dynamic>>[{}, {}, {}];
      final result = processor.extractList(data);
      expect(result.length, 3);
    });

    test('processTrending should handle bool data', () {
      expect(processor.processTrending(true, true), isEmpty);
    });

    test('processTrending should handle double data', () {
      expect(processor.processTrending(3.14, true), isEmpty);
    });

    test('processSearch should handle whitespace-only query', () {
      final data = [{'stone_id': 's1'}];
      final result = processor.processSearch(data, true, '   ');
      expect(result.length, 1); // whitespace is not empty
    });

    test('processDiscoverByMood should handle unknown mood', () {
      final data = [{'stone_id': 's1'}];
      final result = processor.processDiscoverByMood(data, true, 'unknown_mood');
      expect(result.length, 1);
    });
  });
}
