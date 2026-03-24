// 石头数据模型
//
// 代表用户投入心湖的情感内容载体，包含基本信息、互动数据、
// AI分析结果和媒体文件等。

import 'package:flutter/foundation.dart';
import '../../utils/payload_contract.dart';

/// 石头模型
///
/// 用户投入心湖的情感内容载体。
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

  // 推荐与共鸣算法相关字段
  final String? recommendationAlgorithm;
  final String? recommendationReason;
  final double? recommendationScore;
  final double? semanticScore;
  final double? trajectoryScore;
  final double? temporalScore;
  final double? diversityScore;

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
    this.recommendationAlgorithm,
    this.recommendationReason,
    this.recommendationScore,
    this.semanticScore,
    this.trajectoryScore,
    this.temporalScore,
    this.diversityScore,
    this.mediaIds,
    this.hasMedia = false,
    this.hasRippled = false,
  });

  factory Stone.fromJson(Map<String, dynamic> json) {
    try {
      final normalized = normalizePayloadContract(json);

      // 解析 media_ids
      List<String>? mediaIds;
      if (normalized['media_ids'] != null) {
        if (normalized['media_ids'] is List) {
          mediaIds = List<String>.from(normalized['media_ids']);
        } else if (normalized['media_ids'] is String) {
          // 处理 PostgreSQL 数组格式 "{id1,id2}"
          final str = normalized['media_ids'] as String;
          if (str.startsWith('{') && str.endsWith('}')) {
            final inner = str.substring(1, str.length - 1);
            if (inner.isNotEmpty) {
              mediaIds = inner.split(',').map((e) => e.trim()).toList();
            }
          }
        }
      }

      final author = normalized['author'];
      final legacyUser = normalized['user'];
      final authorMap = author is Map
          ? normalizePayloadContract(author)
          : legacyUser is Map
              ? normalizePayloadContract(legacyUser)
              : null;

      return Stone(
        stoneId: normalized['stone_id'] ?? normalized['id'] ?? '',
        userId: normalized['user_id'] ?? normalized['author_id'] ?? '',
        content: normalized['content'] ?? normalized['stone_content'] ?? '',
        stoneType: normalized['stone_type'] ?? 'medium',
        stoneColor: normalized['stone_color'] ?? '#7A92A3',
        isAnonymous: parseBool(normalized['is_anonymous'], defaultValue: true),
        status: normalized['status'] ?? 'published',
        viewCount: normalized['view_count'] is int
            ? normalized['view_count']
            : int.tryParse(normalized['view_count']?.toString() ?? '0') ?? 0,
        rippleCount: normalized['ripple_count'] is int
            ? normalized['ripple_count']
            : int.tryParse(normalized['ripple_count']?.toString() ?? '0') ?? 0,
        boatCount: normalized['boat_count'] is int
            ? normalized['boat_count']
            : int.tryParse(normalized['boat_count']?.toString() ?? '0') ?? 0,
        tags: normalized['tags'] is List
            ? (normalized['tags'] as List).map((e) => e.toString()).toList()
            : [],
        createdAt: _parseDate(normalized['created_at']),
        // 兼容推荐API的平铺 author_name 和标准API的嵌套 author.nickname
        authorNickname: authorMap?['nickname'] ??
            normalized['author_name'] ??
            normalized['nickname'],
        // AI 分析字段 - 兼容 emotion_score 和 sentiment_score
        moodType: normalized['mood_type'],
        sentimentScore: _parseDouble(
            normalized['sentiment_score'] ?? normalized['emotion_score']),
        aiTags: normalized['ai_tags'] != null
            ? List<String>.from(normalized['ai_tags'])
            : null,
        recommendationAlgorithm:
            normalized['algorithm'] ?? normalized['recommendation_algorithm'],
        recommendationReason:
            normalized['recommendation_reason'] ?? normalized['reason'],
        recommendationScore:
            _parseDouble(normalized['score'] ?? normalized['relevance_score']),
        semanticScore: _parseDouble(normalized['semantic_score']),
        trajectoryScore: _parseDouble(normalized['trajectory_score']),
        temporalScore: _parseDouble(normalized['temporal_score']),
        diversityScore: _parseDouble(normalized['diversity_score']),
        // 媒体字段
        mediaIds: mediaIds,
        hasMedia: parseBool(normalized['has_media']) ||
            (mediaIds != null && mediaIds.isNotEmpty),
        hasRippled: parseBool(normalized['has_rippled']),
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

  factory Stone.fromPayload(Map<String, dynamic> payload) {
    return Stone.fromJson(extractStonePayload(payload));
  }

  factory Stone.fromBoatReference(Map<String, dynamic> payload) {
    final normalized = normalizePayloadContract(payload);
    return Stone.fromJson({
      'stone_id': normalized['stone_id'],
      'user_id': normalized['stone_user_id'] ?? normalized['user_id'],
      'content': normalized['stone_content'] ?? normalized['content'],
      'stone_type': normalized['stone_type'] ?? 'medium',
      'stone_color': normalized['stone_color'] ?? '#7A92A3',
      'created_at': normalized['stone_created_at'] ?? normalized['created_at'],
      'mood_type': normalized['stone_mood_type'] ?? normalized['mood_type'],
      'ripple_count': normalized['stone_ripple_count'],
      'boat_count': normalized['stone_boat_count'],
      'author_name': normalized['author_name'] ??
          normalized['sender_name'] ??
          normalized['nickname'],
    });
  }

  static bool parseBool(dynamic value, {bool defaultValue = false}) {
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
      'algorithm': recommendationAlgorithm,
      'recommendation_reason': recommendationReason,
      'score': recommendationScore,
      'semantic_score': semanticScore,
      'trajectory_score': trajectoryScore,
      'temporal_score': temporalScore,
      'diversity_score': diversityScore,
      'media_ids': mediaIds,
      'has_media': hasMedia,
      'has_rippled': hasRippled,
    };
  }

  /// 复制石头
  ///
  /// 创建一个新的Stone实例，可选择性地修改部分属性。
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
    String? recommendationAlgorithm,
    String? recommendationReason,
    double? recommendationScore,
    double? semanticScore,
    double? trajectoryScore,
    double? temporalScore,
    double? diversityScore,
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
      recommendationAlgorithm:
          recommendationAlgorithm ?? this.recommendationAlgorithm,
      recommendationReason: recommendationReason ?? this.recommendationReason,
      recommendationScore: recommendationScore ?? this.recommendationScore,
      semanticScore: semanticScore ?? this.semanticScore,
      trajectoryScore: trajectoryScore ?? this.trajectoryScore,
      temporalScore: temporalScore ?? this.temporalScore,
      diversityScore: diversityScore ?? this.diversityScore,
      mediaIds: mediaIds ?? this.mediaIds,
      hasMedia: hasMedia ?? this.hasMedia,
      hasRippled: hasRippled ?? this.hasRippled,
    );
  }
}
