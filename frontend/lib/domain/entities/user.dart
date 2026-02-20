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
      vipExpiresAt: json['vip_expires_at'] != null
          ? DateTime.fromMillisecondsSinceEpoch(json['vip_expires_at'] * 1000)
          : null,
      createdAt: json['created_at'] != null
          ? DateTime.fromMillisecondsSinceEpoch(json['created_at'] * 1000)
          : null,
      lastActiveAt: json['last_active_at'] != null
          ? DateTime.fromMillisecondsSinceEpoch(json['last_active_at'] * 1000)
          : null,
    );
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
      'vip_expires_at': vipExpiresAt?.millisecondsSinceEpoch,
      'created_at': createdAt?.millisecondsSinceEpoch,
      'last_active_at': lastActiveAt?.millisecondsSinceEpoch,
    };
  }
}
