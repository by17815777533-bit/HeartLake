import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/social_payload_normalizer.dart';

void main() {
  group('extractNormalizedList', () {
    test('falls back to generic list key even when semantic key is absent', () {
      final items = extractNormalizedList(
        {
          'list': [
            {'boat_id': 'b1', 'content': 'hello'},
          ],
        },
        itemNormalizer: (raw) => Map<String, dynamic>.from(raw),
        listKeys: const ['boats'],
      );

      expect(items, hasLength(1));
      expect(items.first['boat_id'], 'b1');
    });

    test('can read nested data payloads', () {
      final items = extractNormalizedList(
        {
          'code': 0,
          'data': {
            'items': [
              {'id': 'nested_1'},
            ],
          },
        },
        itemNormalizer: (raw) => Map<String, dynamic>.from(raw),
        listKeys: const ['users'],
      );

      expect(items, hasLength(1));
      expect(items.first['id'], 'nested_1');
    });
  });

  group('extractPaginationPayload', () {
    test('reads pagination fields from nested payload', () {
      final pagination = extractPaginationPayload({
        'data': {
          'total': 42,
          'page': 2,
          'page_size': 10,
          'total_pages': 5,
          'has_more': true,
        },
      });

      expect(pagination['total'], 42);
      expect(pagination['page'], 2);
      expect(pagination['page_size'], 10);
      expect(pagination['total_pages'], 5);
      expect(pagination['has_more'], true);
    });

    test('falls back to item count when total is missing', () {
      final pagination = extractPaginationPayload({}, itemCount: 3);

      expect(pagination['total'], 3);
      expect(pagination['page'], 1);
      expect(pagination['has_more'], false);
    });
  });
}
