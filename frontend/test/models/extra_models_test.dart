import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/domain/entities/api_response.dart';
import 'package:heart_lake/domain/entities/emotion_trend_point.dart';
import 'package:heart_lake/domain/entities/recommended_stone.dart';
import 'package:heart_lake/domain/entities/emotion_type.dart';
import 'package:heart_lake/utils/mood_colors.dart';

void main() {
  // ==================== ApiResponse ====================
  group('ApiResponse construction', () {
    test('should create with required fields', () {
      final resp = ApiResponse(code: 0, message: '操作成功');
      expect(resp.code, 0);
      expect(resp.message, '操作成功');
      expect(resp.success, true);
      expect(resp.data, isNull);
    });

    test('should be success when code is 0', () {
      expect(ApiResponse(code: 0, message: 'ok').success, true);
    });

    test('should not be success when code is non-zero', () {
      expect(ApiResponse(code: 100001, message: 'err').success, false);
      expect(ApiResponse(code: 500, message: 'err').success, false);
      expect(ApiResponse(code: -1, message: 'err').success, false);
    });

    test('should store data', () {
      final resp = ApiResponse(code: 0, message: 'ok', data: {'key': 'value'});
      expect(resp.data, {'key': 'value'});
    });
  });

  group('ApiResponse.fromJson', () {
    test('should parse complete JSON', () {
      final json = {'code': 0, 'message': '成功', 'data': {'user_id': 'u1'}};
      final resp = ApiResponse.fromJson(json, null);
      expect(resp.code, 0);
      expect(resp.message, '成功');
      expect(resp.success, true);
    });

    test('should parse with fromJsonT converter', () {
      final json = {'code': 0, 'message': 'ok', 'data': {'name': '小明'}};
      final resp = ApiResponse<String>.fromJson(json, (d) => (d as Map)['name'] as String);
      expect(resp.data, '小明');
    });

    test('should handle missing code', () {
      final resp = ApiResponse.fromJson({'message': 'err'}, null);
      expect(resp.code, 500);
      expect(resp.success, false);
    });

    test('should handle missing message', () {
      final resp = ApiResponse.fromJson({'code': 0}, null);
      expect(resp.message, '');
    });

    test('should handle null data without converter', () {
      final resp = ApiResponse.fromJson({'code': 0, 'message': 'ok'}, null);
      expect(resp.data, isNull);
    });

    test('should handle null data with converter', () {
      final resp = ApiResponse.fromJson({'code': 0, 'message': 'ok'}, (d) => d.toString());
      expect(resp.data, isNull);
    });

    test('should handle empty JSON', () {
      final resp = ApiResponse.fromJson({}, null);
      expect(resp.code, 500);
      expect(resp.message, '');
    });
  });

  group('ApiResponse.success factory', () {
    test('should create success response', () {
      final resp = ApiResponse.success({'id': 1});
      expect(resp.code, 0);
      expect(resp.success, true);
      expect(resp.data, {'id': 1});
      expect(resp.message, '操作成功');
    });

    test('should allow custom message', () {
      final resp = ApiResponse.success(null, '创建成功');
      expect(resp.message, '创建成功');
    });

    test('should allow null data', () {
      final resp = ApiResponse.success(null);
      expect(resp.data, isNull);
      expect(resp.success, true);
    });
  });

  group('ApiResponse.error factory', () {
    test('should create error response', () {
      final resp = ApiResponse.error('服务器错误');
      expect(resp.code, 500);
      expect(resp.success, false);
      expect(resp.message, '服务器错误');
    });

    test('should allow custom code', () {
      final resp = ApiResponse.error('未授权', 401);
      expect(resp.code, 401);
    });
  });

  // ==================== PagedResponse ====================
  group('PagedResponse construction', () {
    test('should create with required fields', () {
      final resp = PagedResponse(items: [1, 2, 3], total: 10, page: 1, pageSize: 20);
      expect(resp.items, [1, 2, 3]);
      expect(resp.total, 10);
      expect(resp.page, 1);
      expect(resp.pageSize, 20);
    });

    test('hasMore should be true when more pages exist', () {
      final resp = PagedResponse(items: [1], total: 100, page: 1, pageSize: 20);
      expect(resp.hasMore, true);
    });

    test('hasMore should be false when on last page', () {
      final resp = PagedResponse(items: [1], total: 20, page: 1, pageSize: 20);
      expect(resp.hasMore, false);
    });

    test('hasMore should be false when total is 0', () {
      final resp = PagedResponse(items: [], total: 0, page: 1, pageSize: 20);
      expect(resp.hasMore, false);
    });

    test('hasMore edge case: exactly at boundary', () {
      final resp = PagedResponse(items: [1], total: 40, page: 2, pageSize: 20);
      expect(resp.hasMore, false);
    });

    test('hasMore edge case: past boundary', () {
      final resp = PagedResponse(items: [], total: 30, page: 2, pageSize: 20);
      expect(resp.hasMore, false);
    });
  });

  group('PagedResponse.fromJson', () {
    test('should parse complete JSON', () {
      final json = {
        'items': [
          {'id': 1, 'name': 'a'},
          {'id': 2, 'name': 'b'},
        ],
        'total': 50,
        'page': 1,
        'page_size': 20,
      };
      final resp = PagedResponse<Map<String, dynamic>>.fromJson(json, (e) => e);
      expect(resp.items.length, 2);
      expect(resp.total, 50);
      expect(resp.page, 1);
      expect(resp.pageSize, 20);
    });

    test('should handle missing items', () {
      final resp = PagedResponse<Map<String, dynamic>>.fromJson({}, (e) => e);
      expect(resp.items, isEmpty);
    });

    test('should handle missing total', () {
      final resp = PagedResponse<Map<String, dynamic>>.fromJson({'items': []}, (e) => e);
      expect(resp.total, 0);
    });

    test('should handle missing page', () {
      final resp = PagedResponse<Map<String, dynamic>>.fromJson({'items': []}, (e) => e);
      expect(resp.page, 1);
    });

    test('should handle missing page_size', () {
      final resp = PagedResponse<Map<String, dynamic>>.fromJson({'items': []}, (e) => e);
      expect(resp.pageSize, 20);
    });

    test('should apply fromJsonT converter', () {
      final json = {
        'items': [{'value': 42}],
        'total': 1,
        'page': 1,
        'page_size': 10,
      };
      final resp = PagedResponse<int>.fromJson(json, (e) => e['value'] as int);
      expect(resp.items, [42]);
    });
  });

  // ==================== EmotionTrendPoint ====================
  group('EmotionTrendPoint.fromJson', () {
    test('should parse complete JSON', () {
      final json = {
        'date': '2026-01-15',
        'emotion_score': 0.75,
        'mood': 'happy',
        'stone_count': 5,
        'interaction_count': 12,
      };
      final point = EmotionTrendPoint.fromJson(json);
      expect(point.date.year, 2026);
      expect(point.date.month, 1);
      expect(point.date.day, 15);
      expect(point.emotionScore, 0.75);
      expect(point.mood, 'happy');
      expect(point.stoneCount, 5);
      expect(point.interactionCount, 12);
    });

    test('should handle score key alias', () {
      final point = EmotionTrendPoint.fromJson({'date': '2026-01-01', 'score': 0.5});
      expect(point.emotionScore, 0.5);
    });

    test('should handle mood_type key alias', () {
      final point = EmotionTrendPoint.fromJson({'date': '2026-01-01', 'mood_type': 'sad'});
      expect(point.mood, 'sad');
    });

    test('should default stone_count to 0', () {
      final point = EmotionTrendPoint.fromJson({'date': '2026-01-01'});
      expect(point.stoneCount, 0);
    });

    test('should default interaction_count to 0', () {
      final point = EmotionTrendPoint.fromJson({'date': '2026-01-01'});
      expect(point.interactionCount, 0);
    });

    test('should default mood to neutral', () {
      final point = EmotionTrendPoint.fromJson({'date': '2026-01-01'});
      expect(point.mood, 'neutral');
    });

    test('should default score to 0', () {
      final point = EmotionTrendPoint.fromJson({'date': '2026-01-01'});
      expect(point.emotionScore, 0.0);
    });

    test('should handle invalid date', () {
      final point = EmotionTrendPoint.fromJson({'date': 'invalid'});
      expect(point.date, isA<DateTime>());
    });

    test('should handle null date', () {
      final point = EmotionTrendPoint.fromJson({});
      expect(point.date, isA<DateTime>());
    });

    test('should handle int score', () {
      final point = EmotionTrendPoint.fromJson({'date': '2026-01-01', 'emotion_score': 1});
      expect(point.emotionScore, 1.0);
    });
  });

  // ==================== RecommendedStone ====================
  group('RecommendedStone.fromJson', () {
    test('should parse complete JSON', () {
      final json = {
        'stone_id': 'rs1',
        'content': '推荐内容',
        'author_id': 'a1',
        'author_name': '作者',
        'mood_type': 'happy',
        'emotion_score': 0.8,
        'ripple_count': 10,
        'boat_count': 3,
        'created_at': '2026-01-15T10:00:00Z',
        'tags': ['标签1', '标签2'],
        'media_urls': ['http://img.com/1.png'],
        'recommendation_type': 'similar',
        'recommendation_reason': '与你的情绪相似',
        'score': 0.95,
      };
      final stone = RecommendedStone.fromJson(json);
      expect(stone.stoneId, 'rs1');
      expect(stone.content, '推荐内容');
      expect(stone.authorId, 'a1');
      expect(stone.authorName, '作者');
      expect(stone.moodType, 'happy');
      expect(stone.emotionScore, 0.8);
      expect(stone.rippleCount, 10);
      expect(stone.boatCount, 3);
      expect(stone.tags, ['标签1', '标签2']);
      expect(stone.mediaUrls, ['http://img.com/1.png']);
      expect(stone.recommendationType, RecommendationType.similar);
      expect(stone.recommendationReason, '与你的情绪相似');
      expect(stone.score, 0.95);
    });

    test('should handle empty JSON', () {
      final stone = RecommendedStone.fromJson({});
      expect(stone.stoneId, '');
      expect(stone.content, '');
      expect(stone.authorId, isNull);
      expect(stone.rippleCount, 0);
      expect(stone.boatCount, 0);
      expect(stone.recommendationType, RecommendationType.random);
      expect(stone.recommendationReason, '为你推荐');
      expect(stone.score, 0.0);
    });

    test('should use nickname as author_name fallback', () {
      final stone = RecommendedStone.fromJson({'stone_id': 's1', 'content': 'c', 'nickname': '昵称'});
      expect(stone.authorName, '昵称');
    });

    test('should prefer author_name over nickname', () {
      final stone = RecommendedStone.fromJson({
        'stone_id': 's1',
        'content': 'c',
        'author_name': '作者名',
        'nickname': '昵称',
      });
      expect(stone.authorName, '作者名');
    });

    test('should use sentiment_score as emotion_score fallback', () {
      final stone = RecommendedStone.fromJson({
        'stone_id': 's1',
        'content': 'c',
        'sentiment_score': 0.6,
      });
      expect(stone.emotionScore, 0.6);
    });

    test('should use relevance_score as score fallback', () {
      final stone = RecommendedStone.fromJson({
        'stone_id': 's1',
        'content': 'c',
        'relevance_score': 0.88,
      });
      expect(stone.score, 0.88);
    });

    test('should parse created_at', () {
      final stone = RecommendedStone.fromJson({
        'stone_id': 's1',
        'content': 'c',
        'created_at': '2026-02-20T12:00:00Z',
      });
      expect(stone.createdAt?.year, 2026);
    });

    test('should handle null created_at', () {
      final stone = RecommendedStone.fromJson({'stone_id': 's1', 'content': 'c'});
      expect(stone.createdAt, isNull);
    });

    test('should handle null tags', () {
      final stone = RecommendedStone.fromJson({'stone_id': 's1', 'content': 'c'});
      expect(stone.tags, isNull);
    });

    test('should handle null media_urls', () {
      final stone = RecommendedStone.fromJson({'stone_id': 's1', 'content': 'c'});
      expect(stone.mediaUrls, isNull);
    });
  });

  group('RecommendationType parsing', () {
    test('should parse similar', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'similar'});
      expect(s.recommendationType, RecommendationType.similar);
    });

    test('should parse collaborative', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'collaborative'});
      expect(s.recommendationType, RecommendationType.collaborative);
    });

    test('should parse emotion_compatible', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'emotion_compatible'});
      expect(s.recommendationType, RecommendationType.emotionCompatible);
    });

    test('should parse exploration', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'exploration'});
      expect(s.recommendationType, RecommendationType.exploration);
    });

    test('should parse trending', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'trending'});
      expect(s.recommendationType, RecommendationType.trending);
    });

    test('should parse personalized', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'personalized'});
      expect(s.recommendationType, RecommendationType.personalized);
    });

    test('should default to random for unknown', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'unknown'});
      expect(s.recommendationType, RecommendationType.random);
    });

    test('should default to random for null', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': ''});
      expect(s.recommendationType, RecommendationType.random);
    });

    test('should be case insensitive', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'SIMILAR'});
      expect(s.recommendationType, RecommendationType.similar);
    });

    test('should match partial strings', () {
      final s = RecommendedStone.fromJson({'stone_id': '', 'content': '', 'recommendation_type': 'collaborative_filtering'});
      expect(s.recommendationType, RecommendationType.collaborative);
    });
  });

  group('RecommendationType enum', () {
    test('should have 7 values', () {
      expect(RecommendationType.values.length, 7);
    });

    test('should contain all types', () {
      expect(RecommendationType.values, containsAll([
        RecommendationType.similar,
        RecommendationType.collaborative,
        RecommendationType.emotionCompatible,
        RecommendationType.exploration,
        RecommendationType.trending,
        RecommendationType.personalized,
        RecommendationType.random,
      ]));
    });
  });

  // ==================== EmotionTags ====================
  group('EmotionTags.parseFromBackend', () {
    test('should parse valid mood names', () {
      final moods = EmotionTags.parseFromBackend(['happy', 'sad', 'calm']);
      expect(moods.length, 3);
      expect(moods[0], MoodType.happy);
      expect(moods[1], MoodType.sad);
      expect(moods[2], MoodType.calm);
    });

    test('should return neutral for null input', () {
      final moods = EmotionTags.parseFromBackend(null);
      expect(moods, [MoodType.neutral]);
    });

    test('should return neutral for empty input', () {
      final moods = EmotionTags.parseFromBackend([]);
      expect(moods, [MoodType.neutral]);
    });

    test('should handle unknown mood names', () {
      final moods = EmotionTags.parseFromBackend(['unknown_mood']);
      expect(moods, [MoodType.neutral]);
    });

    test('should handle mixed valid and invalid', () {
      final moods = EmotionTags.parseFromBackend(['happy', 'invalid', 'sad']);
      expect(moods.length, 3);
      expect(moods[0], MoodType.happy);
      expect(moods[1], MoodType.neutral);
      expect(moods[2], MoodType.sad);
    });
  });

  group('EmotionTags.toBackend', () {
    test('should convert moods to strings', () {
      final result = EmotionTags.toBackend([MoodType.happy, MoodType.sad]);
      expect(result, ['happy', 'sad']);
    });

    test('should handle empty list', () {
      expect(EmotionTags.toBackend([]), isEmpty);
    });

    test('should handle single mood', () {
      expect(EmotionTags.toBackend([MoodType.calm]), ['calm']);
    });
  });

  group('EmotionTags.getPrimary', () {
    test('should return first mood', () {
      expect(EmotionTags.getPrimary([MoodType.happy, MoodType.sad]), MoodType.happy);
    });

    test('should return neutral for empty list', () {
      expect(EmotionTags.getPrimary([]), MoodType.neutral);
    });
  });

  group('EmotionTags.fromScore', () {
    test('should return happy for high score', () {
      expect(EmotionTags.fromScore(0.8), MoodType.happy);
    });

    test('should return calm for moderate positive', () {
      expect(EmotionTags.fromScore(0.4), MoodType.calm);
    });

    test('should return neutral for near zero', () {
      expect(EmotionTags.fromScore(0.0), MoodType.neutral);
    });

    test('should return anxious for moderate negative', () {
      expect(EmotionTags.fromScore(-0.4), MoodType.anxious);
    });

    test('should return sad for very negative', () {
      expect(EmotionTags.fromScore(-0.8), MoodType.sad);
    });

    test('should handle boundary 0.6', () {
      expect(EmotionTags.fromScore(0.61), MoodType.happy);
    });

    test('should handle boundary 0.3', () {
      expect(EmotionTags.fromScore(0.31), MoodType.calm);
    });

    test('should handle boundary -0.3', () {
      expect(EmotionTags.fromScore(-0.31), MoodType.anxious);
    });

    test('should handle boundary -0.6', () {
      expect(EmotionTags.fromScore(-0.61), MoodType.sad);
    });

    test('should handle exact 1.0', () {
      expect(EmotionTags.fromScore(1.0), MoodType.happy);
    });

    test('should handle exact -1.0', () {
      expect(EmotionTags.fromScore(-1.0), MoodType.sad);
    });
  });
}
