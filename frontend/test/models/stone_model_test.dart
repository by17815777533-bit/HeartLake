import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/domain/entities/stone.dart';

void main() {
  group('Stone.fromJson', () {
    test('should parse complete JSON correctly', () {
      final json = {
        'stone_id': 'stone-001',
        'user_id': 'user-001',
        'content': '今天心情不错',
        'stone_type': 'heavy',
        'stone_color': '#FF5733',
        'is_anonymous': false,
        'status': 'published',
        'view_count': 42,
        'ripple_count': 10,
        'boat_count': 3,
        'tags': ['日记', '心情'],
        'created_at': '2025-06-15T10:30:00Z',
        'mood_type': 'happy',
        'sentiment_score': 0.85,
        'ai_tags': ['积极', '乐观'],
        'media_ids': ['media-1', 'media-2'],
        'has_media': true,
        'author': {'nickname': '小明'},
      };

      final stone = Stone.fromJson(json);

      expect(stone.stoneId, 'stone-001');
      expect(stone.userId, 'user-001');
      expect(stone.content, '今天心情不错');
      expect(stone.stoneType, 'heavy');
      expect(stone.stoneColor, '#FF5733');
      expect(stone.isAnonymous, false);
      expect(stone.status, 'published');
      expect(stone.viewCount, 42);
      expect(stone.rippleCount, 10);
      expect(stone.boatCount, 3);
      expect(stone.tags, ['日记', '心情']);
      expect(stone.moodType, 'happy');
      expect(stone.sentimentScore, 0.85);
      expect(stone.aiTags, ['积极', '乐观']);
      expect(stone.mediaIds, ['media-1', 'media-2']);
      expect(stone.hasMedia, true);
      expect(stone.authorNickname, '小明');
    });

    test('should handle null/missing fields with defaults', () {
      final json = <String, dynamic>{};

      final stone = Stone.fromJson(json);

      expect(stone.stoneId, '');
      expect(stone.userId, '');
      expect(stone.content, '');
      expect(stone.stoneType, 'medium');
      expect(stone.stoneColor, '#7A92A3');
      expect(stone.isAnonymous, true);
      expect(stone.status, 'published');
      expect(stone.viewCount, 0);
      expect(stone.rippleCount, 0);
      expect(stone.boatCount, 0);
      expect(stone.tags, isEmpty);
      expect(stone.moodType, isNull);
      expect(stone.sentimentScore, isNull);
      expect(stone.aiTags, isNull);
      expect(stone.mediaIds, isNull);
      expect(stone.hasMedia, false);
      expect(stone.authorNickname, isNull);
    });

    test('should parse counts from string values', () {
      final json = {
        'stone_id': 's1',
        'user_id': 'u1',
        'content': 'test',
        'view_count': '100',
        'ripple_count': '25',
        'boat_count': '7',
      };

      final stone = Stone.fromJson(json);

      expect(stone.viewCount, 100);
      expect(stone.rippleCount, 25);
      expect(stone.boatCount, 7);
    });

    test('should parse counts from invalid strings as 0', () {
      final json = {
        'stone_id': 's1',
        'user_id': 'u1',
        'content': 'test',
        'view_count': 'abc',
        'ripple_count': '',
        'boat_count': null,
      };

      final stone = Stone.fromJson(json);

      expect(stone.viewCount, 0);
      expect(stone.rippleCount, 0);
      expect(stone.boatCount, 0);
    });

    test('should parse PostgreSQL array format for media_ids', () {
      final json = {
        'stone_id': 's1',
        'user_id': 'u1',
        'content': 'test',
        'media_ids': '{media-a,media-b,media-c}',
      };

      final stone = Stone.fromJson(json);

      expect(stone.mediaIds, ['media-a', 'media-b', 'media-c']);
      expect(stone.hasMedia, true);
    });

    test('should handle empty PostgreSQL array format', () {
      final json = {
        'stone_id': 's1',
        'user_id': 'u1',
        'content': 'test',
        'media_ids': '{}',
      };

      final stone = Stone.fromJson(json);

      expect(stone.mediaIds, isNull);
    });

    test('should set hasMedia true when mediaIds present even if has_media is false', () {
      final json = {
        'stone_id': 's1',
        'user_id': 'u1',
        'content': 'test',
        'media_ids': ['m1'],
        'has_media': false,
      };

      final stone = Stone.fromJson(json);

      expect(stone.hasMedia, true);
    });

    group('date parsing', () {
      test('should parse ISO 8601 string', () {
        final json = {
          'stone_id': 's1',
          'user_id': 'u1',
          'content': 'test',
          'created_at': '2025-01-15T08:00:00Z',
        };

        final stone = Stone.fromJson(json);

        expect(stone.createdAt.year, 2025);
        expect(stone.createdAt.month, 1);
        expect(stone.createdAt.day, 15);
      });

      test('should parse unix timestamp as int', () {
        final json = {
          'stone_id': 's1',
          'user_id': 'u1',
          'content': 'test',
          'created_at': 1705305600, // 2024-01-15 08:00:00 UTC
        };

        final stone = Stone.fromJson(json);
        final expected = DateTime.fromMillisecondsSinceEpoch(1705305600 * 1000);

        expect(stone.createdAt, expected);
      });

      test('should parse unix timestamp as string', () {
        final json = {
          'stone_id': 's1',
          'user_id': 'u1',
          'content': 'test',
          'created_at': '1705305600',
        };

        final stone = Stone.fromJson(json);
        final expected = DateTime.fromMillisecondsSinceEpoch(1705305600 * 1000);

        expect(stone.createdAt, expected);
      });

      test('should default to now when created_at is null', () {
        final before = DateTime.now();
        final stone = Stone.fromJson({
          'stone_id': 's1',
          'user_id': 'u1',
          'content': 'test',
        });
        final after = DateTime.now();

        expect(stone.createdAt.isAfter(before.subtract(const Duration(seconds: 1))), true);
        expect(stone.createdAt.isBefore(after.add(const Duration(seconds: 1))), true);
      });
    });
  });

  group('Stone.toJson', () {
    test('should serialize all fields correctly', () {
      final createdAt = DateTime(2025, 6, 15, 10, 30);
      final stone = Stone(
        stoneId: 'stone-001',
        userId: 'user-001',
        content: '测试内容',
        stoneType: 'heavy',
        stoneColor: '#FF5733',
        isAnonymous: false,
        status: 'published',
        viewCount: 42,
        rippleCount: 10,
        boatCount: 3,
        tags: ['tag1'],
        createdAt: createdAt,
        moodType: 'happy',
        sentimentScore: 0.85,
        aiTags: ['积极'],
        mediaIds: ['m1'],
        hasMedia: true,
      );

      final json = stone.toJson();

      expect(json['stone_id'], 'stone-001');
      expect(json['user_id'], 'user-001');
      expect(json['content'], '测试内容');
      expect(json['stone_type'], 'heavy');
      expect(json['stone_color'], '#FF5733');
      expect(json['is_anonymous'], false);
      expect(json['status'], 'published');
      expect(json['view_count'], 42);
      expect(json['ripple_count'], 10);
      expect(json['boat_count'], 3);
      expect(json['tags'], ['tag1']);
      expect(json['created_at'], createdAt.millisecondsSinceEpoch ~/ 1000);
      expect(json['mood_type'], 'happy');
      expect(json['sentiment_score'], 0.85);
      expect(json['ai_tags'], ['积极']);
      expect(json['media_ids'], ['m1']);
      expect(json['has_media'], true);
    });

    test('should serialize null optional fields as null', () {
      final stone = Stone(
        stoneId: 's1',
        userId: 'u1',
        content: 'test',
        stoneType: 'medium',
        stoneColor: '#000',
        createdAt: DateTime.now(),
      );

      final json = stone.toJson();

      expect(json['mood_type'], isNull);
      expect(json['sentiment_score'], isNull);
      expect(json['ai_tags'], isNull);
      expect(json['media_ids'], isNull);
    });
  });

  group('Stone.copyWith', () {
    late Stone original;

    setUp(() {
      original = Stone(
        stoneId: 'stone-001',
        userId: 'user-001',
        content: '原始内容',
        stoneType: 'medium',
        stoneColor: '#7A92A3',
        isAnonymous: true,
        rippleCount: 5,
        boatCount: 2,
        createdAt: DateTime(2025, 1, 1),
      );
    });

    test('should return identical stone when no params given', () {
      final copy = original.copyWith();

      expect(copy.stoneId, original.stoneId);
      expect(copy.userId, original.userId);
      expect(copy.content, original.content);
      expect(copy.stoneType, original.stoneType);
      expect(copy.rippleCount, original.rippleCount);
      expect(copy.boatCount, original.boatCount);
    });

    test('should update only specified fields', () {
      final copy = original.copyWith(
        content: '修改后的内容',
        rippleCount: 10,
        moodType: 'sad',
      );

      expect(copy.content, '修改后的内容');
      expect(copy.rippleCount, 10);
      expect(copy.moodType, 'sad');
      // unchanged
      expect(copy.stoneId, original.stoneId);
      expect(copy.userId, original.userId);
      expect(copy.boatCount, original.boatCount);
    });

    test('should update media fields', () {
      final copy = original.copyWith(
        mediaIds: ['m1', 'm2'],
        hasMedia: true,
      );

      expect(copy.mediaIds, ['m1', 'm2']);
      expect(copy.hasMedia, true);
    });
  });

  group('Stone fromJson -> toJson roundtrip', () {
    test('should preserve data through serialization roundtrip', () {
      final json = {
        'stone_id': 'rt-001',
        'user_id': 'user-rt',
        'content': '往返测试',
        'stone_type': 'light',
        'stone_color': '#AABBCC',
        'is_anonymous': false,
        'status': 'published',
        'view_count': 5,
        'ripple_count': 3,
        'boat_count': 1,
        'tags': ['test'],
        'created_at': 1700000000,
        'mood_type': 'calm',
        'sentiment_score': 0.5,
        'ai_tags': ['平静'],
        'media_ids': ['m1'],
        'has_media': true,
      };

      final stone = Stone.fromJson(json);
      final output = stone.toJson();

      expect(output['stone_id'], json['stone_id']);
      expect(output['content'], json['content']);
      expect(output['stone_type'], json['stone_type']);
      expect(output['mood_type'], json['mood_type']);
      expect(output['sentiment_score'], json['sentiment_score']);
      expect(output['created_at'], json['created_at']);
    });
  });
}
