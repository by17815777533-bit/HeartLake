// @file user.dart
// @brief 用户数据模型
// Created by 王璐瑶

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

  bool get isVIP => vipLevel > 0 && (vipExpiresAt?.isAfter(DateTime.now()) ?? false);

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
      avatarUrl: avatarUrl ?? this.avatarUrl,
      bio: bio ?? this.bio,
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
      isAnonymous: json['is_anonymous'] ?? true,
      status: json['status'] ?? 'active',
      avatarUrl: json['avatar_url'],
      bio: json['bio'],
      vipLevel: json['vip_level'] ?? 0,
      vipExpiresAt: _parseDateTime(json['vip_expires_at']),
      createdAt: _parseDateTime(json['created_at']),
      lastActiveAt: _parseDateTime(json['last_active_at']),
    );
  }

  static DateTime? _parseDateTime(dynamic date) {
    if (date == null) return null;
    if (date is int) return DateTime.fromMillisecondsSinceEpoch(date * 1000);
    if (date is double) return DateTime.fromMillisecondsSinceEpoch(date.toInt() * 1000);
    if (date is String) {
      final numVal = int.tryParse(date);
      if (numVal != null) return DateTime.fromMillisecondsSinceEpoch(numVal * 1000);
      return DateTime.tryParse(date);
    }
    return null;
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
      'vip_expires_at': vipExpiresAt != null ? vipExpiresAt!.millisecondsSinceEpoch ~/ 1000 : null,
      'created_at': createdAt != null ? createdAt!.millisecondsSinceEpoch ~/ 1000 : null,
      'last_active_at': lastActiveAt != null ? lastActiveAt!.millisecondsSinceEpoch ~/ 1000 : null,
    };
  }
}
