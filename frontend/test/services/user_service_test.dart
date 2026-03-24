import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/user_service.dart';

void main() {
  group('UserPayloadNormalizer.normalizeUser', () {
    test('normalizes user aliases into a stable contract', () {
      final user = UserPayloadNormalizer.normalizeUser({
        'userId': 'u1',
        'name': '小明',
        'avatar': 'https://cdn.example.com/a.png',
        'createdAt': '2026-03-01T08:00:00Z',
        'lastSeenAt': '2026-03-02T08:00:00Z',
        'vipLevel': 2,
        'vipExpiresAt': '2026-04-01T00:00:00Z',
      });

      expect(user, isNotNull);
      expect(user!['user_id'], 'u1');
      expect(user['userId'], 'u1');
      expect(user['nickname'], '小明');
      expect(user['avatar_url'], 'https://cdn.example.com/a.png');
      expect(user['avatarUrl'], 'https://cdn.example.com/a.png');
      expect(user['created_at'], '2026-03-01T08:00:00Z');
      expect(user['last_active_at'], '2026-03-02T08:00:00Z');
      expect(user['vip_level'], 2);
      expect(user['vip_expires_at'], '2026-04-01T00:00:00Z');
    });

    test('extracts nested user/profile payloads', () {
      final user = UserPayloadNormalizer.normalizeUser({
        'data': {
          'profile': {
            'id': 'u2',
            'display_name': '旅人',
          },
        },
      });

      expect(user, isNotNull);
      expect(user!['user_id'], 'u2');
      expect(user['nickname'], '旅人');
    });

    test('normalizes blank avatar and bio to null', () {
      final user = UserPayloadNormalizer.normalizeUser({
        'user_id': 'u3',
        'nickname': '空值用户',
        'avatar_url': '   ',
        'bio': '',
      });

      expect(user, isNotNull);
      expect(user!['avatar_url'], isNull);
      expect(user['avatarUrl'], isNull);
      expect(user['bio'], isNull);
    });
  });

  group('UserPayloadNormalizer.extractUserList', () {
    test('extracts users from nested data payloads', () {
      final users = UserPayloadNormalizer.extractUserList({
        'data': {
          'users': [
            {'user_id': 'u1'},
            {'userId': 'u2'},
          ],
        },
      });

      expect(users, hasLength(2));
      expect(users.first['user_id'], 'u1');
      expect(users.last['user_id'], 'u2');
    });

    test('supports results key and filters invalid items', () {
      final users = UserPayloadNormalizer.extractUserList({
        'results': [
          {'id': 'u1', 'name': '甲'},
          'invalid',
          {'user_id': 'u2', 'nickname': '乙'},
        ],
      });

      expect(users, hasLength(2));
      expect(users.first['user_id'], 'u1');
      expect(users.first['nickname'], '甲');
      expect(users.last['user_id'], 'u2');
    });
  });

  group('UserPayloadNormalizer.extractTotal', () {
    test('reads total aliases from multiple payload styles', () {
      expect(UserPayloadNormalizer.extractTotal({'total': 5}), 5);
      expect(UserPayloadNormalizer.extractTotal({'count': 6}), 6);
      expect(UserPayloadNormalizer.extractTotal({'total_count': 7}), 7);
      expect(
        UserPayloadNormalizer.extractTotal({
          'pagination': {'total': 8},
        }),
        8,
      );
      expect(
        UserPayloadNormalizer.extractTotal({
          'meta': {'total': 9},
        }),
        9,
      );
    });

    test('falls back to extracted list size when totals are absent', () {
      final total = UserPayloadNormalizer.extractTotal({
        'users': [
          {'user_id': 'u1'},
          {'user_id': 'u2'},
          {'user_id': 'u3'},
        ],
      });

      expect(total, 3);
    });
  });

  group('UserPayloadNormalizer.buildUserCollection', () {
    test('builds a shared collection envelope with corrected totals', () {
      final collection = UserPayloadNormalizer.buildUserCollection({
        'data': {
          'users': [
            {'user_id': 'u1'},
            {'user_id': 'u2'},
          ],
          'count': 9,
          'page': 2,
          'page_size': 2,
        },
      });

      expect(collection['users'], hasLength(2));
      expect(collection['items'], hasLength(2));
      expect(collection['list'], hasLength(2));
      expect(collection['total'], 9);
      expect(collection['page'], 2);
      expect(collection['page_size'], 2);
      expect(collection['total_pages'], 5);
      expect(collection['has_more'], true);
      expect(collection['pagination']['total'], 9);
    });
  });
}
