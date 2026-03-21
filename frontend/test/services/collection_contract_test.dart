import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/social_payload_normalizer.dart';

void main() {
  group('extractNormalizedList', () {
    test('reads nested semantic lists and normalizes boat aliases', () {
      final boats = extractNormalizedList(
        {
          'code': 0,
          'data': {
            'boats': [
              {
                'boatId': 'boat_1',
                'stoneId': 'stone_1',
                'senderId': 'user_1',
                'createdAt': '2026-03-01T08:00:00Z',
              },
            ],
          },
        },
        itemNormalizer: normalizeBoatPayload,
        listKeys: const ['boats'],
      );

      expect(boats, hasLength(1));
      expect(boats.single['boat_id'], 'boat_1');
      expect(boats.single['stone_id'], 'stone_1');
      expect(boats.single['sender_id'], 'user_1');
      expect(boats.single['created_at'], '2026-03-01T08:00:00Z');
    });

    test('honors ordered semantic keys when payload contains conflicting lists',
        () {
      final messages = extractNormalizedList(
        {
          'items': [
            {'message_id': 'item_1'}
          ],
          'messages': [
            {'message_id': 'message_1'}
          ],
        },
        itemNormalizer: normalizeMessagePayload,
        listKeys: const ['items', 'messages'],
      );

      expect(messages, hasLength(1));
      expect(messages.single['message_id'], 'item_1');
    });
  });

  group('buildCollectionEnvelope', () {
    test('mirrors semantic key, generic aliases, and pagination metadata', () {
      final raw = {
        'data': {
          'notifications': [
            {'notification_id': 'n1', 'is_read': false}
          ],
          'total': 21,
          'page': 2,
          'page_size': 10,
          'total_pages': 3,
          'has_more': true,
        },
      };

      final items = extractNormalizedList(
        raw,
        itemNormalizer: (item) => Map<String, dynamic>.from(item),
        listKeys: const ['notifications'],
      );
      final envelope = buildCollectionEnvelope(
        raw,
        primaryKey: 'notifications',
        items: items,
        extra: const {'unread_count': 4, 'unreadCount': 4},
      );

      expect(envelope['notifications'], hasLength(1));
      expect(envelope['items'], hasLength(1));
      expect(envelope['list'], hasLength(1));
      expect(envelope['total'], 21);
      expect(envelope['page'], 2);
      expect(envelope['pageSize'], 10);
      expect(envelope['totalPages'], 3);
      expect(envelope['has_more'], true);
      expect(envelope['pagination']['total'], 21);
      expect(envelope['unread_count'], 4);
      expect(envelope['unreadCount'], 4);
    });

    test('falls back to item count when pagination metadata is missing', () {
      final raw = [
        {'ripple_id': 'r1'},
        {'ripple_id': 'r2'},
      ];
      final items = extractNormalizedList(
        raw,
        itemNormalizer: normalizeRipplePayload,
        listKeys: const ['ripples'],
      );
      final envelope = buildCollectionEnvelope(
        raw,
        primaryKey: 'ripples',
        items: items,
      );

      expect(envelope['ripples'], hasLength(2));
      expect(envelope['total'], 2);
      expect(envelope['page'], 1);
      expect(envelope['page_size'], 2);
      expect(envelope['total_pages'], 1);
      expect(envelope['has_more'], false);
    });
  });

  group('extractUnreadCount', () {
    test('prefers explicit unread count from nested alias fields', () {
      final unread = extractUnreadCount(
        {
          'items': [
            {'notification_id': 'n1', 'is_read': false},
            {'notification_id': 'n2', 'is_read': false},
          ],
          'data': {'unreadCount': 7},
        },
        items: const [
          {'notification_id': 'n1', 'is_read': false},
          {'notification_id': 'n2', 'is_read': false},
        ],
      );

      expect(unread, 7);
    });

    test('derives unread count from raw list when metadata is absent', () {
      final items = const [
        {'notification_id': 'n1', 'is_read': true},
        {'notification_id': 'n2', 'is_read': false},
        {'notification_id': 'n3'},
      ];

      final unread = extractUnreadCount(items, items: items);
      expect(unread, 2);
    });
  });

  group('normalizeFriendPayload', () {
    test(
        'mirrors friend identifiers and avatar aliases for friend-like payloads',
        () {
      final normalized = normalizeFriendPayload({
        'target_user_id': 'user_42',
        'avatar': 'https://cdn.example.com/avatar.png',
        'createdAt': '2026-03-20T10:00:00Z',
      });

      expect(normalized['friend_id'], 'user_42');
      expect(normalized['friend_user_id'], 'user_42');
      expect(normalized['user_id'], 'user_42');
      expect(normalized['avatar_url'], 'https://cdn.example.com/avatar.png');
      expect(normalized['created_at'], '2026-03-20T10:00:00Z');
    });
  });
}
