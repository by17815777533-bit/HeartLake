import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/recommendation_response_parser.dart';

void main() {
  group('RecommendationResponseParser.extractList', () {
    test('extracts direct list payloads and filters invalid items', () {
      final result = RecommendationResponseParser.extractList([
        {'stone_id': 's1'},
        'invalid',
        {'stone_id': 's2'},
      ]);

      expect(result, hasLength(2));
      expect(result.first['stone_id'], 's1');
      expect(result.last['stone_id'], 's2');
    });

    test('extracts nested data payloads with semantic recommendation keys', () {
      final result = RecommendationResponseParser.extractList({
        'code': 0,
        'data': {
          'trending_stones': [
            {'stone_id': 'nested_1'},
            {'stone_id': 'nested_2'},
          ],
        },
      });

      expect(result, hasLength(2));
      expect(result.first['stone_id'], 'nested_1');
    });

    test('honors configured key priority when payload is inconsistent', () {
      final result = RecommendationResponseParser.extractList({
        'trending_stones': [
          {'stone_id': 'preferred'}
        ],
        'results': [
          {'stone_id': 'fallback'}
        ],
      });

      expect(result, hasLength(1));
      expect(result.single['stone_id'], 'preferred');
    });

    test('can target custom keys for specialized recommendation sources', () {
      final result = RecommendationResponseParser.extractList(
        {
          'matches': [
            {'stone_id': 'match_1'}
          ],
          'recommendations': [
            {'stone_id': 'rec_1'}
          ],
        },
        keys: const ['matches', 'recommendations'],
      );

      expect(result, hasLength(1));
      expect(result.single['stone_id'], 'match_1');
    });

    test('returns empty list for unsupported payloads', () {
      expect(
        RecommendationResponseParser.extractList({
          'data': {'meta': 'no list here'},
        }),
        isEmpty,
      );
      expect(RecommendationResponseParser.extractList('invalid'), isEmpty);
    });
  });
}
