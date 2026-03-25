bool _hasMeaningfulValue(dynamic value) {
  if (value == null) return false;
  if (value is String) return value.trim().isNotEmpty;
  return true;
}

dynamic _normalizePayloadValue(dynamic value) {
  if (value is Map) {
    return normalizePayloadContract(value);
  }
  if (value is List) {
    return value.map(_normalizePayloadValue).toList();
  }
  return value;
}

void _mirrorAlias(
  Map<String, dynamic> data,
  String canonicalKey,
  List<String> aliasKeys,
) {
  dynamic value;
  if (_hasMeaningfulValue(data[canonicalKey])) {
    value = data[canonicalKey];
  } else {
    for (final aliasKey in aliasKeys) {
      if (_hasMeaningfulValue(data[aliasKey])) {
        value = data[aliasKey];
        break;
      }
    }
  }

  if (!_hasMeaningfulValue(value)) return;

  data[canonicalKey] = value;
  for (final aliasKey in aliasKeys) {
    data[aliasKey] = value;
  }
}

Map<String, dynamic> normalizePayloadContract(Map raw) {
  final normalized = <String, dynamic>{};
  raw.forEach((key, value) {
    normalized[key.toString()] = _normalizePayloadValue(value);
  });

  _mirrorAlias(normalized, 'user_id', const ['userId']);
  _mirrorAlias(normalized, 'author_id', const ['authorId']);
  _mirrorAlias(normalized, 'triggered_by', const ['triggeredBy']);
  _mirrorAlias(normalized, 'created_at', const ['createdAt']);
  _mirrorAlias(normalized, 'stone_id', const ['stoneId']);
  _mirrorAlias(normalized, 'stone_user_id', const ['stoneUserId']);
  _mirrorAlias(normalized, 'stone_owner_id', const ['stoneOwnerId']);
  _mirrorAlias(normalized, 'stone_content', const ['stoneContent']);
  _mirrorAlias(normalized, 'stone_created_at', const ['stoneCreatedAt']);
  _mirrorAlias(normalized, 'stone_mood_type', const ['stoneMoodType']);
  _mirrorAlias(normalized, 'boat_id', const ['boatId']);
  _mirrorAlias(normalized, 'ripple_id', const ['rippleId']);
  _mirrorAlias(normalized, 'message_id', const ['messageId']);
  _mirrorAlias(normalized, 'request_id', const ['requestId']);
  _mirrorAlias(normalized, 'connection_id', const ['connectionId']);
  _mirrorAlias(normalized, 'session_id', const ['sessionId']);
  _mirrorAlias(normalized, 'friendship_id', const ['friendshipId']);
  _mirrorAlias(normalized, 'notification_id', const ['notificationId', 'id']);
  _mirrorAlias(normalized, 'blocked_user_id', const ['blockedUserId']);
  _mirrorAlias(normalized, 'resource_id', const ['resourceId']);
  _mirrorAlias(normalized, 'appointment_id', const ['appointmentId']);
  _mirrorAlias(normalized, 'avatar_url', const ['avatarUrl']);
  _mirrorAlias(normalized, 'friend_id', const ['friendId']);
  _mirrorAlias(normalized, 'friend_user_id', const ['friendUserId']);
  _mirrorAlias(normalized, 'target_user_id', const ['targetUserId']);
  _mirrorAlias(normalized, 'from_user_id', const ['fromUserId']);
  _mirrorAlias(normalized, 'to_user_id', const ['toUserId']);
  _mirrorAlias(normalized, 'target_id', const ['targetId']);
  _mirrorAlias(normalized, 'related_id', const ['relatedId']);
  _mirrorAlias(normalized, 'peer_id', const ['peerId']);
  _mirrorAlias(normalized, 'sender_id', const ['senderId']);
  _mirrorAlias(normalized, 'sender_name', const ['senderName']);
  _mirrorAlias(normalized, 'target_name', const ['targetName']);
  _mirrorAlias(normalized, 'author_name', const ['authorName']);
  _mirrorAlias(normalized, 'stone_type', const ['stoneType']);
  _mirrorAlias(normalized, 'stone_color', const ['stoneColor']);
  _mirrorAlias(normalized, 'mood_type', const ['moodType']);
  _mirrorAlias(normalized, 'expires_at', const ['expiresAt']);
  _mirrorAlias(normalized, 'boat_count', const ['boatCount']);
  _mirrorAlias(normalized, 'ripple_count', const ['rippleCount']);
  _mirrorAlias(normalized, 'view_count', const ['viewCount']);
  _mirrorAlias(normalized, 'is_anonymous', const ['isAnonymous']);
  _mirrorAlias(normalized, 'has_rippled', const ['hasRippled']);
  _mirrorAlias(normalized, 'is_read', const ['isRead']);
  _mirrorAlias(normalized, 'days_left', const ['daysLeft']);
  _mirrorAlias(normalized, 'has_quota', const ['hasQuota']);
  _mirrorAlias(normalized, 'frequency_hours', const ['frequencyHours']);
  _mirrorAlias(normalized, 'frequency_minutes', const ['frequencyMinutes']);
  _mirrorAlias(normalized, 'remaining_budget', const ['remainingBudget']);
  _mirrorAlias(normalized, 'total_budget', const ['totalBudget']);
  _mirrorAlias(normalized, 'query_count', const ['queryCount']);
  _mirrorAlias(normalized, 'dominant_mood', const ['dominantMood']);
  _mirrorAlias(normalized, 'sample_count', const ['sampleCount']);
  _mirrorAlias(normalized, 'avg_score', const ['avgScore']);
  _mirrorAlias(normalized, 'can_transfer_lamp', const ['canTransferLamp']);
  _mirrorAlias(normalized, 'is_guardian', const ['isGuardian']);
  _mirrorAlias(normalized, 'is_lamp_keeper', const ['isLampKeeper']);
  _mirrorAlias(normalized, 'resonance_points', const ['resonancePoints']);
  _mirrorAlias(normalized, 'quality_ripples', const ['qualityRipples']);
  _mirrorAlias(normalized, 'warm_boats', const ['warmBoats']);
  _mirrorAlias(normalized, 'is_vip', const ['isVip']);

  return normalized;
}

String? _firstNonEmptyString(List<dynamic> values) {
  for (final value in values) {
    final candidate = value?.toString().trim();
    if (candidate != null && candidate.isNotEmpty) {
      return candidate;
    }
  }
  return null;
}

String? extractPayloadUserId(Map<String, dynamic> payload) {
  final normalized = normalizePayloadContract(payload);
  final stone = normalized['stone'];
  final boat = normalized['boat'];
  final ripple = normalized['ripple'];
  final stoneMap = stone is Map ? normalizePayloadContract(stone) : null;
  final boatMap = boat is Map ? normalizePayloadContract(boat) : null;
  final rippleMap = ripple is Map ? normalizePayloadContract(ripple) : null;
  return _firstNonEmptyString([
    normalized['triggered_by'],
    normalized['sender_id'],
    normalized['user_id'],
    boatMap?['sender_id'],
    boatMap?['user_id'],
    rippleMap?['sender_id'],
    rippleMap?['user_id'],
    stoneMap?['user_id'],
    stoneMap?['author_id'],
  ]);
}

String? extractFriendEntityId(Map<String, dynamic> payload) {
  final normalized = normalizePayloadContract(payload);
  return _firstNonEmptyString([
    normalized['user_id'],
    normalized['friend_id'],
    normalized['friend_user_id'],
    normalized['peer_id'],
    normalized['target_user_id'],
    normalized['from_user_id'],
    normalized['to_user_id'],
  ]);
}

String? extractStoneEntityId(Map<String, dynamic> payload) {
  final normalized = normalizePayloadContract(payload);
  final stone = normalized['stone'];
  final boat = normalized['boat'];
  final ripple = normalized['ripple'];
  final stoneMap = stone is Map ? normalizePayloadContract(stone) : null;
  final boatMap = boat is Map ? normalizePayloadContract(boat) : null;
  final rippleMap = ripple is Map ? normalizePayloadContract(ripple) : null;
  return _firstNonEmptyString([
    normalized['stone_id'],
    stoneMap?['stone_id'],
    boatMap?['stone_id'],
    rippleMap?['stone_id'],
  ]);
}

Map<String, dynamic> extractStonePayload(Map<String, dynamic> payload) {
  final normalized = normalizePayloadContract(payload);
  final stone = normalized['stone'];
  if (stone is Map) {
    return normalizePayloadContract(stone);
  }
  return normalized;
}

int? extractPayloadInt(Map<String, dynamic> payload, List<String> keys) {
  final normalized = normalizePayloadContract(payload);
  for (final key in keys) {
    final value = normalized[key];
    if (value is int) return value;
    if (value is num) return value.toInt();
    if (value is String) {
      final parsed = int.tryParse(value.trim());
      if (parsed != null) return parsed;
    }
  }
  return null;
}

int? extractBoatCount(Map<String, dynamic> payload) {
  return extractPayloadInt(payload, const ['boat_count', 'boatCount']);
}

int? extractRippleCount(Map<String, dynamic> payload) {
  return extractPayloadInt(payload, const ['ripple_count', 'rippleCount']);
}

String? extractBoatEntityId(Map<String, dynamic> payload) {
  final normalized = normalizePayloadContract(payload);
  final boat = normalized['boat'];
  final boatMap = boat is Map ? normalizePayloadContract(boat) : null;
  return _firstNonEmptyString([
    normalized['boat_id'],
    boatMap?['boat_id'],
  ]);
}

String? extractRippleEntityId(Map<String, dynamic> payload) {
  final normalized = normalizePayloadContract(payload);
  final ripple = normalized['ripple'];
  final rippleMap = ripple is Map ? normalizePayloadContract(ripple) : null;
  return _firstNonEmptyString([
    normalized['ripple_id'],
    rippleMap?['ripple_id'],
  ]);
}

String? extractNotificationEntityId(Map<String, dynamic> payload) {
  final normalized = normalizePayloadContract(payload);
  final notification = normalized['notification'];
  final nestedData = normalized['data'];
  final notificationMap =
      notification is Map ? normalizePayloadContract(notification) : null;
  final dataMap =
      nestedData is Map ? normalizePayloadContract(nestedData) : null;
  return _firstNonEmptyString([
    normalized['notification_id'],
    normalized['id'],
    notificationMap?['notification_id'],
    notificationMap?['id'],
    dataMap?['notification_id'],
    dataMap?['id'],
  ]);
}
