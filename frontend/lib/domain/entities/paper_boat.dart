// @file paper_boat.dart
// @brief 漂流纸船数据模型
// Created by 王璐瑶

library;

import '../../utils/mood_colors.dart';

/// 漂流状态枚举
enum DriftStatus {
  drifting, // 漂流中
  caught, // 已被捞起
  responded, // 已回复
  released, // 被扔回水中
  expired, // 已过期
}

/// 漂流模式枚举
enum DriftMode {
  random, // 随机漂流
  directed, // 定向漂流
  wish, // 祈愿漂流（节日活动）
}

/// 纸船样式枚举
enum BoatStyle {
  paper, // 普通纸船
  origami, // 折纸船
  lotus, // 荷花船
}

/// 漂流纸船模型
class PaperBoat {
  final String boatId;
  final String senderId;
  final String? receiverId; // 定向漂流时指定
  final String? stoneId; // 关联的石头ID

  final String content;
  final MoodType mood;
  final BoatStyle boatStyle;

  final DriftMode driftMode;
  final DriftStatus driftStatus;

  final DateTime? scheduledDeliveryAt; // 预计送达时间
  final DateTime? actualDeliveryAt; // 实际送达时间

  final bool aiReplied;
  final String? aiReplyContent;
  final DateTime? aiReplyAt;

  final double sentimentScore;

  final String? caughtBy;
  final DateTime? caughtAt;
  final String? responseContent;
  final DateTime? responseAt;

  final DateTime createdAt;
  final DateTime updatedAt;

  // 额外显示信息
  final String? senderNickname;
  final String? catcherNickname;

  PaperBoat({
    required this.boatId,
    required this.senderId,
    this.receiverId,
    this.stoneId,
    required this.content,
    this.mood = MoodType.neutral,
    this.boatStyle = BoatStyle.paper,
    this.driftMode = DriftMode.random,
    this.driftStatus = DriftStatus.drifting,
    this.scheduledDeliveryAt,
    this.actualDeliveryAt,
    this.aiReplied = false,
    this.aiReplyContent,
    this.aiReplyAt,
    this.sentimentScore = 0.0,
    this.caughtBy,
    this.caughtAt,
    this.responseContent,
    this.responseAt,
    required this.createdAt,
    required this.updatedAt,
    this.senderNickname,
    this.catcherNickname,
  });

  factory PaperBoat.fromJson(Map<String, dynamic> json) {
    return PaperBoat(
      boatId: json['boat_id'] ?? '',
      senderId: json['sender_id'] ?? '',
      receiverId: json['receiver_id'],
      stoneId: json['stone_id'],
      content: json['content'] ?? '',
      mood: MoodColors.fromString(json['mood']),
      boatStyle: _parseBoatStyle(json['boat_style']),
      driftMode: _parseDriftMode(json['drift_mode']),
      driftStatus: _parseDriftStatus(json['drift_status']),
      scheduledDeliveryAt: _parseDateTime(json['scheduled_delivery_at']),
      actualDeliveryAt: _parseDateTime(json['actual_delivery_at']),
      aiReplied: json['ai_replied'] ?? false,
      aiReplyContent: json['ai_reply_content'],
      aiReplyAt: _parseDateTime(json['ai_reply_at']),
      sentimentScore: (json['sentiment_score'] ?? 0.0).toDouble(),
      caughtBy: json['caught_by'],
      caughtAt: _parseDateTime(json['caught_at']),
      responseContent: json['response_content'],
      responseAt: _parseDateTime(json['response_at']),
      createdAt: _parseDateTime(json['created_at']) ?? DateTime.now(),
      updatedAt: _parseDateTime(json['updated_at']) ?? DateTime.now(),
      senderNickname: json['sender_nickname'],
      catcherNickname: json['catcher_nickname'],
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'boat_id': boatId,
      'sender_id': senderId,
      'receiver_id': receiverId,
      'stone_id': stoneId,
      'content': content,
      'mood': mood.name,
      'boat_style': boatStyle.name,
      'drift_mode': driftMode.name,
      'drift_status': driftStatus.name,
      'scheduled_delivery_at': scheduledDeliveryAt?.toIso8601String(),
      'actual_delivery_at': actualDeliveryAt?.toIso8601String(),
      'ai_replied': aiReplied,
      'ai_reply_content': aiReplyContent,
      'sentiment_score': sentimentScore,
      'caught_by': caughtBy,
      'response_content': responseContent,
      'created_at': createdAt.toIso8601String(),
    };
  }

  /// 是否可以被回应
  bool get canRespond =>
      driftStatus == DriftStatus.caught && responseContent == null;

  /// 是否可以扔回水中
  bool get canRelease =>
      driftStatus == DriftStatus.caught && responseContent == null;

  /// 是否正在漂流
  bool get isDrifting => driftStatus == DriftStatus.drifting;

  /// 是否有AI回复
  bool get hasAIReply => aiReplied && aiReplyContent != null;

  /// 获取漂流剩余时间（如果有预计送达时间）
  Duration? get remainingDriftTime {
    if (scheduledDeliveryAt == null || driftStatus != DriftStatus.drifting) {
      return null;
    }
    final remaining = scheduledDeliveryAt!.difference(DateTime.now());
    return remaining.isNegative ? Duration.zero : remaining;
  }

  /// 获取状态描述
  String get statusDescription {
    switch (driftStatus) {
      case DriftStatus.drifting:
        if (remainingDriftTime != null) {
          final minutes = remainingDriftTime!.inMinutes;
          return '漂流中，约$minutes分钟后可被捞起';
        }
        return '正在湖面漂流...';
      case DriftStatus.caught:
        return '已被捞起';
      case DriftStatus.responded:
        return '已收到回复';
      case DriftStatus.released:
        return '被扔回水中，继续漂流';
      case DriftStatus.expired:
        return '已过期';
    }
  }

  static BoatStyle _parseBoatStyle(String? style) {
    switch (style) {
      case 'origami':
        return BoatStyle.origami;
      case 'lotus':
        return BoatStyle.lotus;
      default:
        return BoatStyle.paper;
    }
  }

  static DriftMode _parseDriftMode(String? mode) {
    switch (mode) {
      case 'directed':
        return DriftMode.directed;
      case 'wish':
        return DriftMode.wish;
      default:
        return DriftMode.random;
    }
  }

  static DriftStatus _parseDriftStatus(String? status) {
    switch (status) {
      case 'drifting':
        return DriftStatus.drifting;
      case 'caught':
        return DriftStatus.caught;
      case 'responded':
        return DriftStatus.responded;
      case 'released':
        return DriftStatus.released;
      case 'expired':
        return DriftStatus.expired;
      default:
        return DriftStatus.drifting;
    }
  }

  static DateTime? _parseDateTime(dynamic date) {
    if (date == null) return null;
    if (date is int) return DateTime.fromMillisecondsSinceEpoch(date * 1000);
    if (date is String) {
      final numVal = int.tryParse(date);
      if (numVal != null) {
        return DateTime.fromMillisecondsSinceEpoch(numVal * 1000);
      }
      return DateTime.tryParse(date);
    }
    return null;
  }
}
