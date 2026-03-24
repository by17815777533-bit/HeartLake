// 用户数据模型
//
// 对应后端User表，包含基本信息、匿名状态、灯火等级等。
// 时间字段兼容Unix时间戳（秒）和ISO 8601字符串两种格式。

/// 心湖用户实体
///
/// 用户的基本信息和状态。
class User {
  final String userId;
  final String nickname;
  final bool isAnonymous;
  final String status;
  final String? avatarUrl;
  final String? bio;
  final int vipLevel;
  final DateTime? vipExpiresAt;
  final DateTime? createdAt;
  final DateTime? lastActiveAt;

  User({
    required this.userId,
    required this.nickname,
    this.isAnonymous = true,
    this.status = 'active',
    this.avatarUrl,
    this.bio,
    this.vipLevel = 0,
    this.vipExpiresAt,
    this.createdAt,
    this.lastActiveAt,
  });

  /// 是否为VIP用户
  ///
  /// vipLevel大于0且未过期时返回true。
  bool get isVIP =>
      vipLevel > 0 && (vipExpiresAt?.isAfter(DateTime.now()) ?? false);

  /// 复制用户
  ///
  /// 创建一个新的User实例，可选择性地修改部分属性。
  User copyWith({
    String? nickname,
    String? avatarUrl,
    String? bio,
    int? vipLevel,
    DateTime? vipExpiresAt,
  }) {
    return User(
      userId: userId,
      nickname: nickname ?? this.nickname,
      isAnonymous: isAnonymous,
      status: status,
      avatarUrl: avatarUrl == null ? this.avatarUrl : _normalizeOptionalText(avatarUrl),
      bio: bio == null ? this.bio : _normalizeOptionalText(bio),
      vipLevel: vipLevel ?? this.vipLevel,
      vipExpiresAt: vipExpiresAt ?? this.vipExpiresAt,
      createdAt: createdAt,
      lastActiveAt: lastActiveAt,
    );
  }

  factory User.fromJson(Map<String, dynamic> json) {
    return User(
      userId: json['user_id'] ?? '',
      nickname: json['nickname'] ?? '',
      isAnonymous: _parseBool(json['is_anonymous'], defaultValue: true),
      status: json['status'] ?? 'active',
      avatarUrl: _normalizeOptionalText(json['avatar_url']),
      bio: _normalizeOptionalText(json['bio']),
      vipLevel: json['vip_level'] ?? 0,
      vipExpiresAt: _parseDateTime(json['vip_expires_at']),
      createdAt: _parseDateTime(json['created_at']),
      lastActiveAt: _parseDateTime(json['last_active_at']),
    );
  }

  static bool _parseBool(dynamic value, {bool defaultValue = false}) {
    if (value is bool) return value;
    if (value is num) return value != 0;
    if (value is String) {
      final normalized = value.trim().toLowerCase();
      if (normalized == 'true' || normalized == '1' || normalized == 'yes') {
        return true;
      }
      if (normalized == 'false' || normalized == '0' || normalized == 'no') {
        return false;
      }
    }
    return defaultValue;
  }

  static DateTime? _parseDateTime(dynamic date) {
    if (date == null) return null;
    if (date is int) return DateTime.fromMillisecondsSinceEpoch(date * 1000);
    if (date is double) {
      return DateTime.fromMillisecondsSinceEpoch(date.toInt() * 1000);
    }
    if (date is String) {
      final numVal = int.tryParse(date);
      if (numVal != null) {
        return DateTime.fromMillisecondsSinceEpoch(numVal * 1000);
      }
      return DateTime.tryParse(date);
    }
    return null;
  }

  static String? _normalizeOptionalText(dynamic value) {
    final text = value?.toString().trim();
    if (text == null || text.isEmpty) return null;
    return text;
  }

  Map<String, dynamic> toJson() {
    return {
      'user_id': userId,
      'nickname': nickname,
      'is_anonymous': isAnonymous,
      'status': status,
      'avatar_url': avatarUrl,
      'bio': bio,
      'vip_level': vipLevel,
      'vip_expires_at': vipExpiresAt != null
          ? vipExpiresAt!.millisecondsSinceEpoch ~/ 1000
          : null,
      'created_at':
          createdAt != null ? createdAt!.millisecondsSinceEpoch ~/ 1000 : null,
      'last_active_at': lastActiveAt != null
          ? lastActiveAt!.millisecondsSinceEpoch ~/ 1000
          : null,
    };
  }
}
