import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/domain/entities/stone.dart';
import 'package:heart_lake/domain/entities/api_response.dart';

void main() {
  group('Stone Model', () {
    test('fromJson parses correctly', () {
      final json = {
        'stone_id': 'test_123',
        'user_id': 'user_456',
        'content': 'Test content',
        'ripple_count': 5,
        'boat_count': 3,
        'tags': ['test', 'flutter'],
        'created_at': '2026-02-09T10:00:00Z',
      };

      final stone = Stone.fromJson(json);

      expect(stone.stoneId, 'test_123');
      expect(stone.userId, 'user_456');
      expect(stone.content, 'Test content');
      expect(stone.rippleCount, 5);
      expect(stone.boatCount, 3);
      expect(stone.tags, ['test', 'flutter']);
    });

    test('toJson serializes correctly', () {
      final stone = Stone(
        stoneId: 'test_123',
        userId: 'user_456',
        content: 'Test content',
        stoneType: 'light',
        stoneColor: 'blue',
        rippleCount: 5,
        boatCount: 3,
        tags: ['test'],
        createdAt: DateTime.parse('2026-02-09T10:00:00Z'),
      );

      final json = stone.toJson();

      expect(json['stone_id'], 'test_123');
      expect(json['content'], 'Test content');
    });
  });

  group('ApiResponse', () {
    test('success response', () {
      final response = ApiResponse<String>.success('data', 'OK');

      expect(response.success, true);
      expect(response.code, 200);
      expect(response.data, 'data');
    });

    test('error response', () {
      final response = ApiResponse<String>.error('Failed', 500);

      expect(response.success, false);
      expect(response.code, 500);
      expect(response.message, 'Failed');
    });

    test('fromJson parses correctly', () {
      final json = {
        'code': 200,
        'message': 'Success',
        'data': {'name': 'test'},
      };

      final response = ApiResponse<Map<String, dynamic>>.fromJson(json, null);

      expect(response.success, true);
      expect(response.data?['name'], 'test');
    });
  });

  group('PagedResponse', () {
    test('hasMore returns true when more pages exist', () {
      final response = PagedResponse<String>(
        items: ['a', 'b'],
        total: 10,
        page: 1,
        pageSize: 2,
      );

      expect(response.hasMore, true);
    });

    test('hasMore returns false on last page', () {
      final response = PagedResponse<String>(
        items: ['a', 'b'],
        total: 4,
        page: 2,
        pageSize: 2,
      );

      expect(response.hasMore, false);
    });
  });
}
