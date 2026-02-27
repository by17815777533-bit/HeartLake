// 石头数据模型

import 'package:flutter/foundation.dart';

/// 石头模型
///
/// 代表用户投入心湖的情感内容载体，包含：
/// - 基本信息：内容、类型、颜色
/// - 互动数据：涟漪数、纸船数
/// - AI分析：情绪类型、情感分数、AI标签
/// - 媒体文件：图片、视频等
class Stone {
  final String stoneId;
  final String userId;
  final String content;
  final String stoneType; // light, medium, heavy - 石头重量
  final String stoneColor;
  final bool isAnonymous;
  final String status;
  final int viewCount;
  final int rippleCount;
  final int boatCount;
  final List<String> tags;
  final DateTime createdAt;
  final String? authorNickname;

  // AI 分析相关字段
  final String?
      moodType; // 情绪类型: happy, sad, calm, anxious, angry, surprised, confused
  final double? sentimentScore; // 情感分数: -1.0 (消极) 到 1.0 (积极)
  final List<String>? aiTags; // AI 生成的标签

  // 媒体文件相关字段
  final List<String>? mediaIds; // 关联的媒体文件ID列表
  final bool hasMedia; // 是否包含媒体文件
  final bool hasRippled; // 当前用户是否已涟漪

  Stone({
    required this.stoneId,
    required this.userId,
    required this.content,
    required this.stoneType,
    required this.stoneColor,
    this.isAnonymous = true,
    this.status = 'published',
    this.viewCount = 0,
    this.rippleCount = 0,
    this.boatCount = 0,
    this.tags = const [],
    required this.createdAt,
    this.authorNickname,
    this.moodType,
    this.sentimentScore,
    this.aiTags,
    this.mediaIds,
    this.hasMedia = false,
    this.hasRippled = false,
  });

  factory Stone.fromJson(Map<String, dynamic> json) {
    try {
      // 解析 media_ids
      List<String>? mediaIds;
      if (json['media_ids'] != null) {
        if (json['media_ids'] is List) {
          mediaIds = List<String>.from(json['media_ids']);
        } else if (json['media_ids'] is String) {
          // 处理 PostgreSQL 数组格式 "{id1,id2}"
          final str = json['media_ids'] as String;
          if (str.startsWith('{') && str.endsWith('}')) {
            final inner = str.substring(1, str.length - 1);
            if (inner.isNotEmpty) {
              mediaIds = inner.split(',').map((e) => e.trim()).toList();
            }
          }
        }
      }

      return Stone(
        stoneId: json['stone_id'] ?? '',
        userId: json['user_id'] ?? '',
        content: json['content'] ?? '',
        stoneType: json['stone_type'] ?? 'medium',
        stoneColor: json['stone_color'] ?? '#7A92A3',
        isAnonymous: json['is_anonymous'] ?? true,
        status: json['status'] ?? 'published',
        viewCount: json['view_count'] is int
            ? json['view_count']
            : int.tryParse(json['view_count']?.toString() ?? '0') ?? 0,
        rippleCount: json['ripple_count'] is int
            ? json['ripple_count']
            : int.tryParse(json['ripple_count']?.toString() ?? '0') ?? 0,
        boatCount: json['boat_count'] is int
            ? json['boat_count']
            : int.tryParse(json['boat_count']?.toString() ?? '0') ?? 0,
        tags: json['tags'] is List ? (json['tags'] as List).map((e) => e.toString()).toList() : [],
        createdAt: _parseDate(json['created_at']),
        // 兼容推荐API的平铺 author_name 和标准API的嵌套 author.nickname
        authorNickname: json['author']?['nickname'] ?? json['author_name'] ?? json['nickname'],
        // AI 分析字段 - 兼容 emotion_score 和 sentiment_score
        moodType: json['mood_type'],
        sentimentScore: _parseDouble(json['sentiment_score'] ?? json['emotion_score']),
        aiTags:
            json['ai_tags'] != null ? List<String>.from(json['ai_tags']) : null,
        // 媒体字段
        mediaIds: mediaIds,
        hasMedia: json['has_media'] == true ||
            (mediaIds != null && mediaIds.isNotEmpty),
        hasRippled: json['has_rippled'] == true,
      );
    } catch (e, stackTrace) {
      // 记录详细错误信息，包含原始JSON便于排查
      debugPrint('Stone.fromJson 解析失败: $e');
      debugPrint('  原始数据: ${json.keys.toList()}');
      debugPrint('  堆栈: $stackTrace');
      throw FormatException(
        'Stone JSON 解析失败: $e, keys=${json.keys.toList()}',
      );
    }
  }

  static double? _parseDouble(dynamic value) {
    if (value == null) return null;
    if (value is num) return value.toDouble();
    if (value is String) return double.tryParse(value);
    return null;
  }

  static DateTime _parseDate(dynamic date) {
    if (date == null) return DateTime.now();
    if (date is int) return DateTime.fromMillisecondsSinceEpoch(date * 1000);
    if (date is String) {
      // 先尝试按数字时间戳字符串解析
      final numVal = int.tryParse(date);
      if (numVal != null) {
        return DateTime.fromMillisecondsSinceEpoch(numVal * 1000);
      }
      // 再尝试按 ISO 时间字符串解析
      try {
        return DateTime.parse(date);
      } catch (_) {}
    }
    return DateTime.now();
  }

  Map<String, dynamic> toJson() {
    return {
      'stone_id': stoneId,
      'user_id': userId,
      'content': content,
      'stone_type': stoneType,
      'stone_color': stoneColor,
      'is_anonymous': isAnonymous,
      'status': status,
      'view_count': viewCount,
      'ripple_count': rippleCount,
      'boat_count': boatCount,
      'tags': tags,
      'created_at': createdAt.millisecondsSinceEpoch ~/ 1000,
      'mood_type': moodType,
      'sentiment_score': sentimentScore,
      'ai_tags': aiTags,
      'media_ids': mediaIds,
      'has_media': hasMedia,
      'has_rippled': hasRippled,
    };
  }

  /// 复制并修改石头属性
  Stone copyWith({
    String? stoneId,
    String? userId,
    String? content,
    String? stoneType,
    String? stoneColor,
    bool? isAnonymous,
    String? status,
    int? viewCount,
    int? rippleCount,
    int? boatCount,
    List<String>? tags,
    DateTime? createdAt,
    String? authorNickname,
    String? moodType,
    double? sentimentScore,
    List<String>? aiTags,
    List<String>? mediaIds,
    bool? hasMedia,
    bool? hasRippled,
  }) {
    return Stone(
      stoneId: stoneId ?? this.stoneId,
      userId: userId ?? this.userId,
      content: content ?? this.content,
      stoneType: stoneType ?? this.stoneType,
      stoneColor: stoneColor ?? this.stoneColor,
      isAnonymous: isAnonymous ?? this.isAnonymous,
      status: status ?? this.status,
      viewCount: viewCount ?? this.viewCount,
      rippleCount: rippleCount ?? this.rippleCount,
      boatCount: boatCount ?? this.boatCount,
      tags: tags ?? this.tags,
      createdAt: createdAt ?? this.createdAt,
      authorNickname: authorNickname ?? this.authorNickname,
      moodType: moodType ?? this.moodType,
      sentimentScore: sentimentScore ?? this.sentimentScore,
      aiTags: aiTags ?? this.aiTags,
      mediaIds: mediaIds ?? this.mediaIds,
      hasMedia: hasMedia ?? this.hasMedia,
      hasRippled: hasRippled ?? this.hasRippled,
    );
  }
}
