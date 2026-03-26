// 石头数据模型
//
// 代表用户投入心湖的情感内容载体，包含基本信息、互动数据、
// AI分析结果和媒体文件等。

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
      final author = normalized['author'];
      final legacyUser = normalized['user'];
      final authorMap = author is Map
          ? normalizePayloadContract(author)
          : legacyUser is Map
              ? normalizePayloadContract(legacyUser)
              : null;
      final mediaIds = _parseMediaIds(normalized['media_ids']);

      return Stone(
        stoneId: _requireNonEmptyString(
          normalized['stone_id'] ?? normalized['id'],
          field: 'stone_id',
        ),
        userId: _requireNonEmptyString(
          normalized['user_id'] ?? normalized['author_id'],
          field: 'user_id',
        ),
        content: _requireNonEmptyString(
          normalized['content'] ?? normalized['stone_content'],
          field: 'content',
        ),
        stoneType: _requireNonEmptyString(
          normalized['stone_type'],
          field: 'stone_type',
        ),
        stoneColor: _requireNonEmptyString(
          normalized['stone_color'],
          field: 'stone_color',
        ),
        isAnonymous: _parseBoolField(
          normalized['is_anonymous'],
          field: 'is_anonymous',
          defaultValue: true,
        ),
        status: _optionalNonEmptyString(normalized['status']) ?? 'published',
        viewCount: _parseIntField(
          normalized['view_count'],
          field: 'view_count',
          defaultValue: 0,
        ),
        rippleCount: _parseIntField(
          normalized['ripple_count'],
          field: 'ripple_count',
          defaultValue: 0,
        ),
        boatCount: _parseIntField(
          normalized['boat_count'],
          field: 'boat_count',
          defaultValue: 0,
        ),
        tags: _parseStringList(
          normalized['tags'],
          field: 'tags',
          defaultValue: const [],
        ),
        createdAt: _parseRequiredDate(
          normalized['created_at'],
          field: 'created_at',
        ),
        // 兼容推荐API的平铺 author_name 和标准API的嵌套 author.nickname
        authorNickname: _optionalNonEmptyString(authorMap?['nickname']) ??
            _optionalNonEmptyString(normalized['author_name']) ??
            _optionalNonEmptyString(normalized['nickname']),
        // AI 分析字段 - 兼容 emotion_score 和 sentiment_score
        moodType: _optionalNonEmptyString(normalized['mood_type']),
        sentimentScore: _parseOptionalDouble(
            normalized['sentiment_score'] ?? normalized['emotion_score']),
        aiTags:
            _parseOptionalStringList(normalized['ai_tags'], field: 'ai_tags'),
        recommendationAlgorithm:
            _optionalNonEmptyString(normalized['algorithm']) ??
                _optionalNonEmptyString(normalized['recommendation_algorithm']),
        recommendationReason:
            _optionalNonEmptyString(normalized['recommendation_reason']) ??
                _optionalNonEmptyString(normalized['reason']),
        recommendationScore: _parseOptionalDouble(
          normalized['score'] ?? normalized['relevance_score'],
        ),
        semanticScore: _parseOptionalDouble(normalized['semantic_score']),
        trajectoryScore: _parseOptionalDouble(normalized['trajectory_score']),
        temporalScore: _parseOptionalDouble(normalized['temporal_score']),
        diversityScore: _parseOptionalDouble(normalized['diversity_score']),
        // 媒体字段
        mediaIds: mediaIds,
        hasMedia: _parseBoolField(
              normalized['has_media'],
              field: 'has_media',
              defaultValue: false,
            ) ||
            (mediaIds != null && mediaIds.isNotEmpty),
        hasRippled: _parseBoolField(
          normalized['has_rippled'],
          field: 'has_rippled',
          defaultValue: false,
        ),
      );
    } catch (error, stackTrace) {
      Error.throwWithStackTrace(
        FormatException(
          'Stone JSON 解析失败: $error, keys=${json.keys.toList()}',
        ),
        stackTrace,
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
      'stone_type': normalized['stone_type'],
      'stone_color': normalized['stone_color'],
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

  static String _requireNonEmptyString(
    dynamic value, {
    required String field,
  }) {
    final candidate = _optionalNonEmptyString(value);
    if (candidate != null) return candidate;
    throw FormatException('Stone payload 缺少必填字段: $field');
  }

  static String? _optionalNonEmptyString(dynamic value) {
    if (value == null) return null;
    final candidate = value.toString().trim();
    if (candidate.isEmpty) return null;
    return candidate;
  }

  static bool _parseBoolField(
    dynamic value, {
    required String field,
    required bool defaultValue,
  }) {
    if (value == null) return defaultValue;
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
    throw FormatException('Stone payload 字段 $field 不是合法布尔值: $value');
  }

  static int _parseIntField(
    dynamic value, {
    required String field,
    required int defaultValue,
  }) {
    if (value == null) return defaultValue;
    if (value is int) return value;
    if (value is num) return value.toInt();
    if (value is String) {
      final parsed = int.tryParse(value.trim());
      if (parsed != null) return parsed;
    }
    throw FormatException('Stone payload 字段 $field 不是合法整数: $value');
  }

  static List<String> _parseStringList(
    dynamic value, {
    required String field,
    required List<String> defaultValue,
  }) {
    if (value == null) return defaultValue;
    if (value is! List) {
      throw FormatException('Stone payload 字段 $field 不是列表: $value');
    }
    return value
        .map((item) => _requireNonEmptyString(item, field: field))
        .toList();
  }

  static List<String>? _parseOptionalStringList(
    dynamic value, {
    required String field,
  }) {
    if (value == null) return null;
    return _parseStringList(value, field: field, defaultValue: const []);
  }

  static double? _parseOptionalDouble(dynamic value) {
    if (value == null) return null;
    if (value is num) return value.toDouble();
    if (value is String) {
      final parsed = double.tryParse(value.trim());
      if (parsed != null) return parsed;
    }
    throw FormatException('Stone payload 包含非法数字: $value');
  }

  static DateTime _parseRequiredDate(
    dynamic date, {
    required String field,
  }) {
    if (date == null) {
      throw FormatException('Stone payload 缺少必填字段: $field');
    }
    if (date is int) {
      return _parseUnixTimestamp(date);
    }
    if (date is num) {
      return _parseUnixTimestamp(date.toInt());
    }
    if (date is String) {
      final trimmed = date.trim();
      if (trimmed.isEmpty) {
        throw FormatException('Stone payload 字段 $field 不能为空字符串');
      }
      final numVal = int.tryParse(trimmed);
      if (numVal != null) {
        return _parseUnixTimestamp(numVal);
      }
      final parsedDate = DateTime.tryParse(trimmed);
      if (parsedDate != null) {
        return parsedDate;
      }
      throw FormatException('Stone payload 字段 $field 不是合法时间: $date');
    }
    throw FormatException('Stone payload 字段 $field 不是合法时间: $date');
  }

  static DateTime _parseUnixTimestamp(int value) {
    final isMilliseconds = value.abs() >= 100000000000;
    return DateTime.fromMillisecondsSinceEpoch(
      isMilliseconds ? value : value * 1000,
    );
  }

  static List<String>? _parseMediaIds(dynamic value) {
    if (value == null) return null;
    if (value is List) {
      return value
          .map((item) => _requireNonEmptyString(item, field: 'media_ids'))
          .toList();
    }
    if (value is String) {
      final normalized = value.trim();
      if (normalized.isEmpty || normalized == '{}') {
        return const [];
      }
      if (normalized.startsWith('{') && normalized.endsWith('}')) {
        final inner = normalized.substring(1, normalized.length - 1).trim();
        if (inner.isEmpty) return const [];
        return inner
            .split(',')
            .map((item) => _requireNonEmptyString(item, field: 'media_ids'))
            .toList();
      }
    }
    throw FormatException('Stone payload 字段 media_ids 格式非法: $value');
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
