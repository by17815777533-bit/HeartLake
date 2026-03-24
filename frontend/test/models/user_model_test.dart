import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/domain/entities/user.dart';

void main() {
  group('User.fromJson', () {
    test('should parse complete JSON correctly', () {
      final json = {
        'user_id': 'user-001',
        'nickname': '小明',
        'is_anonymous': false,
        'status': 'active',
        'avatar_url': 'https://example.com/avatar.png',
        'bio': '热爱生活',
        'vip_level': 2,
        'vip_expires_at': 1750000000, // unix seconds
        'created_at': 1700000000,
        'last_active_at': 1705000000,
      };

      final user = User.fromJson(json);

      expect(user.userId, 'user-001');
      expect(user.nickname, '小明');
      expect(user.isAnonymous, false);
      expect(user.status, 'active');
      expect(user.avatarUrl, 'https://example.com/avatar.png');
      expect(user.bio, '热爱生活');
      expect(user.vipLevel, 2);
      expect(user.vipExpiresAt,
          DateTime.fromMillisecondsSinceEpoch(1750000000 * 1000));
      expect(user.createdAt,
          DateTime.fromMillisecondsSinceEpoch(1700000000 * 1000));
      expect(user.lastActiveAt,
          DateTime.fromMillisecondsSinceEpoch(1705000000 * 1000));
    });

    test('should handle null/missing fields with defaults', () {
      final json = <String, dynamic>{};

      final user = User.fromJson(json);

      expect(user.userId, '');
      expect(user.nickname, '');
      expect(user.isAnonymous, true);
      expect(user.status, 'active');
      expect(user.avatarUrl, isNull);
      expect(user.bio, isNull);
      expect(user.vipLevel, 0);
      expect(user.vipExpiresAt, isNull);
      expect(user.createdAt, isNull);
      expect(user.lastActiveAt, isNull);
    });

    test('should normalize blank avatar and bio to null', () {
      final user = User.fromJson({
        'user_id': 'u1',
        'nickname': '空值用户',
        'avatar_url': '   ',
        'bio': '',
      });

      expect(user.avatarUrl, isNull);
      expect(user.bio, isNull);
    });

    test('should parse timestamps as unix seconds * 1000', () {
      final json = {
        'user_id': 'u1',
        'nickname': 'test',
        'created_at': 1700000000,
      };

      final user = User.fromJson(json);

      // fromJson multiplies by 1000 to convert unix seconds to milliseconds
      expect(user.createdAt!.millisecondsSinceEpoch, 1700000000 * 1000);
    });
  });

  group('User.toJson', () {
    test('should serialize all fields correctly', () {
      final vipExpires = DateTime(2026, 6, 15);
      final createdAt = DateTime(2025, 1, 1);
      final lastActive = DateTime(2025, 6, 1);

      final user = User(
        userId: 'user-001',
        nickname: '小明',
        isAnonymous: false,
        status: 'active',
        avatarUrl: 'https://example.com/avatar.png',
        bio: '热爱生活',
        vipLevel: 2,
        vipExpiresAt: vipExpires,
        createdAt: createdAt,
        lastActiveAt: lastActive,
      );

      final json = user.toJson();

      expect(json['user_id'], 'user-001');
      expect(json['nickname'], '小明');
      expect(json['is_anonymous'], false);
      expect(json['status'], 'active');
      expect(json['avatar_url'], 'https://example.com/avatar.png');
      expect(json['bio'], '热爱生活');
      expect(json['vip_level'], 2);
      // toJson outputs unix seconds (matching backend format)
      expect(json['vip_expires_at'], vipExpires.millisecondsSinceEpoch ~/ 1000);
      expect(json['created_at'], createdAt.millisecondsSinceEpoch ~/ 1000);
      expect(json['last_active_at'], lastActive.millisecondsSinceEpoch ~/ 1000);
    });

    test('should serialize null optional fields as null', () {
      final user = User(
        userId: 'u1',
        nickname: 'test',
      );

      final json = user.toJson();

      expect(json['avatar_url'], isNull);
      expect(json['bio'], isNull);
      expect(json['vip_expires_at'], isNull);
      expect(json['created_at'], isNull);
      expect(json['last_active_at'], isNull);
    });
  });

  group('User.isVIP', () {
    test('should return true when vipLevel > 0 and not expired', () {
      final user = User(
        userId: 'u1',
        nickname: 'vip',
        vipLevel: 1,
        vipExpiresAt: DateTime.now().add(const Duration(days: 30)),
      );

      expect(user.isVIP, true);
    });

    test('should return false when vipLevel is 0', () {
      final user = User(
        userId: 'u1',
        nickname: 'free',
        vipLevel: 0,
        vipExpiresAt: DateTime.now().add(const Duration(days: 30)),
      );

      expect(user.isVIP, false);
    });

    test('should return false when vip has expired', () {
      final user = User(
        userId: 'u1',
        nickname: 'expired',
        vipLevel: 2,
        vipExpiresAt: DateTime.now().subtract(const Duration(days: 1)),
      );

      expect(user.isVIP, false);
    });

    test('should return false when vipExpiresAt is null', () {
      final user = User(
        userId: 'u1',
        nickname: 'no-expiry',
        vipLevel: 1,
      );

      expect(user.isVIP, false);
    });
  });

  group('User.copyWith', () {
    late User original;

    setUp(() {
      original = User(
        userId: 'user-001',
        nickname: '原始昵称',
        isAnonymous: false,
        status: 'active',
        avatarUrl: 'old.png',
        bio: '旧简介',
        vipLevel: 1,
        vipExpiresAt: DateTime(2026, 1, 1),
      );
    });

    test('should return identical user when no params given', () {
      final copy = original.copyWith();

      expect(copy.userId, original.userId);
      expect(copy.nickname, original.nickname);
      expect(copy.avatarUrl, original.avatarUrl);
      expect(copy.bio, original.bio);
      expect(copy.vipLevel, original.vipLevel);
    });

    test('should update only specified fields', () {
      final copy = original.copyWith(
        nickname: '新昵称',
        bio: '新简介',
      );

      expect(copy.nickname, '新昵称');
      expect(copy.bio, '新简介');
      // unchanged
      expect(copy.userId, original.userId);
      expect(copy.avatarUrl, original.avatarUrl);
      expect(copy.vipLevel, original.vipLevel);
    });

    test('should update vip fields', () {
      final newExpiry = DateTime(2027, 6, 1);
      final copy = original.copyWith(
        vipLevel: 3,
        vipExpiresAt: newExpiry,
      );

      expect(copy.vipLevel, 3);
      expect(copy.vipExpiresAt, newExpiry);
    });

    test('should clear optional text fields when empty string is provided', () {
      final copy = original.copyWith(
        avatarUrl: '   ',
        bio: '',
      );

      expect(copy.avatarUrl, isNull);
      expect(copy.bio, isNull);
    });
  });
}
