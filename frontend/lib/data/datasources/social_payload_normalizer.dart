import '../../utils/payload_contract.dart';

Map<String, dynamic> _asMap(Map raw) =>
    Map<String, dynamic>.from(raw.cast<String, dynamic>());

Iterable<Map<String, dynamic>> _candidateMaps(dynamic raw) sync* {
  if (raw is! Map) return;
  final source = _asMap(raw);
  yield source;

  final nested = source['data'];
  if (nested is Map) {
    yield _asMap(nested);
  }
}

List<String> _mergeListKeys(List<String> listKeys) {
  return {...listKeys, 'items', 'list', 'results'}.toList();
}

dynamic _firstValue(Map<String, dynamic> data, List<String> keys) {
  for (final key in keys) {
    final value = data[key];
    if (value == null) continue;
    if (value is String && value.trim().isEmpty) continue;
    return value;
  }
  return null;
}

int? _toInt(dynamic value) {
  if (value is int) return value;
  if (value is num) return value.toInt();
  if (value is String) return int.tryParse(value.trim());
  return null;
}

bool? _toBool(dynamic value) {
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
  return null;
}

List<dynamic>? _extractNestedList(dynamic value) {
  if (value is List) {
    return value;
  }
  if (value is! Map) {
    return null;
  }

  final source = _asMap(value);
  for (final key in const ['items', 'list', 'results', 'data']) {
    final nested = _extractNestedList(source[key]);
    if (nested != null) {
      return nested;
    }
  }
  return null;
}

List<Map<String, dynamic>> extractNormalizedList(
  dynamic raw, {
  required Map<String, dynamic> Function(Map<String, dynamic>) itemNormalizer,
  List<String> listKeys = const ['items'],
}) {
  if (raw is List) {
    return raw
        .whereType<Map>()
        .map((item) => itemNormalizer(_asMap(item)))
        .toList();
  }
  if (raw is Map) {
    final mergedKeys = _mergeListKeys(listKeys);
    for (final data in _candidateMaps(raw)) {
      for (final key in mergedKeys) {
        final value = _extractNestedList(data[key]);
        if (value == null) continue;
        return value
            .whereType<Map>()
            .map((item) => itemNormalizer(_asMap(item)))
            .toList();
      }
    }
  }
  return <Map<String, dynamic>>[];
}

Map<String, dynamic> extractPaginationPayload(
  dynamic raw, {
  int itemCount = 0,
}) {
  final sources = raw is Map
      ? _candidateMaps(raw).toList()
      : const <Map<String, dynamic>>[];

  Map<String, dynamic> pagination = <String, dynamic>{};
  for (final candidate in sources) {
    final value = candidate['pagination'] ?? candidate['meta'];
    if (value is Map) {
      pagination = _asMap(value);
      break;
    }
  }

  dynamic read(List<String> keys) {
    for (final candidate in sources) {
      final value = _firstValue(candidate, keys);
      if (value != null) return value;
    }
    return _firstValue(pagination, keys);
  }

  final rawTotal = _toInt(read(const ['total', 'count', 'total_count']));
  final rawPage = _toInt(read(const ['page', 'page_index']));
  final rawPageSize =
      _toInt(read(const ['page_size', 'pageSize', 'per_page', 'limit']));
  final rawTotalPages =
      _toInt(read(const ['total_pages', 'totalPages', 'page_count']));
  final rawHasMore = _toBool(read(const ['has_more', 'hasMore']));

  final total = rawTotal ?? itemCount;
  final page = rawPage ?? 1;
  final pageSize = rawPageSize ?? itemCount;
  final totalPages =
      rawTotalPages ?? (pageSize > 0 ? (total + pageSize - 1) ~/ pageSize : 0);
  final hasMore = rawHasMore ?? (pageSize > 0 && page * pageSize < total);

  return {
    'total': total,
    'page': page,
    'page_size': pageSize,
    'pageSize': pageSize,
    'total_pages': totalPages,
    'totalPages': totalPages,
    'has_more': hasMore,
    'hasMore': hasMore,
    '_has_explicit_total': rawTotal != null,
    '_has_explicit_total_pages': rawTotalPages != null,
    '_has_explicit_has_more': rawHasMore != null,
  };
}

int extractUnreadCount(
  dynamic raw, {
  List<Map<String, dynamic>> items = const <Map<String, dynamic>>[],
}) {
  if (raw is List) {
    return items.where((item) => item['is_read'] != true).length;
  }

  if (raw is Map) {
    for (final candidate in _candidateMaps(raw)) {
      final unread = _toInt(_firstValue(candidate, const [
        'unread_count',
        'unreadCount',
      ]));
      if (unread != null) {
        return unread;
      }
    }
  }

  return items.where((item) => item['is_read'] != true).length;
}

Map<String, dynamic> buildCollectionEnvelope<T>(
  dynamic raw, {
  required String primaryKey,
  required List<T> items,
  int? totalOverride,
  bool requireExplicitTotal = false,
  Map<String, dynamic> extra = const <String, dynamic>{},
}) {
  final basePagination = extractPaginationPayload(raw, itemCount: items.length);
  if (requireExplicitTotal && basePagination['_has_explicit_total'] != true) {
    throw StateError('$primaryKey 响应缺少 total');
  }
  final total = totalOverride ?? basePagination['total'] as int;
  final page = basePagination['page'] as int;
  final pageSize = basePagination['page_size'] as int;
  final hasExplicitTotalPages =
      basePagination['_has_explicit_total_pages'] == true;
  final hasExplicitHasMore = basePagination['_has_explicit_has_more'] == true;
  final totalPages = hasExplicitTotalPages
      ? basePagination['total_pages'] as int
      : pageSize > 0
          ? (total + pageSize - 1) ~/ pageSize
          : 0;
  final hasMore = hasExplicitHasMore
      ? basePagination['has_more'] == true
      : pageSize > 0 && page * pageSize < total;
  final pagination = {
    ...basePagination,
    'total': total,
    'page': page,
    'page_size': pageSize,
    'pageSize': pageSize,
    'total_pages': totalPages,
    'totalPages': totalPages,
    'has_more': hasMore,
    'hasMore': hasMore,
  };
  pagination.remove('_has_explicit_total');
  pagination.remove('_has_explicit_total_pages');
  pagination.remove('_has_explicit_has_more');
  return {
    'success': true,
    primaryKey: items,
    'items': items,
    'list': items,
    'total': total,
    'page': page,
    'page_size': pageSize,
    'pageSize': pageSize,
    'total_pages': totalPages,
    'totalPages': totalPages,
    'has_more': hasMore,
    'hasMore': hasMore,
    'pagination': pagination,
    ...extra,
  };
}

Map<String, dynamic> normalizeFriendPayload(Map raw) {
  final item = _asMap(raw);

  final peerId = _firstValue(item, const [
    'friend_id',
    'friend_user_id',
    'user_id',
    'userId',
    'friendId',
    'friendUserId',
    'peer_id',
    'target_user_id',
    'to_user_id',
    'from_user_id',
    'requester_id',
  ]);
  if (peerId != null) {
    final normalizedId = peerId.toString();
    item['friend_id'] = normalizedId;
    item['friendId'] = normalizedId;
    item['friend_user_id'] = normalizedId;
    item['friendUserId'] = normalizedId;
    item['user_id'] = normalizedId;
    item['userId'] = normalizedId;
    item['peer_id'] = normalizedId;
  }

  final avatar = _firstValue(item, const [
    'avatar_url',
    'avatarUrl',
    'avatar',
    'friend_avatar',
    'image',
  ]);
  if (avatar != null) {
    item['avatar_url'] = avatar;
    item['avatarUrl'] = avatar;
    item['friend_avatar'] = avatar;
  }

  final createdAt = _firstValue(item, const ['created_at', 'createdAt']);
  if (createdAt != null) {
    item['created_at'] = createdAt;
    item['createdAt'] = createdAt;
  }

  return item;
}

Map<String, dynamic> normalizeConnectionPayload(Map raw) {
  final item = _asMap(raw);

  final connectionId =
      _firstValue(item, const ['connection_id', 'connectionId', 'id']);
  if (connectionId != null) {
    final normalizedId = connectionId.toString();
    item['connection_id'] = normalizedId;
    item['connectionId'] = normalizedId;
    item['id'] = normalizedId;
  }

  final userId = _firstValue(item, const ['user_id', 'userId']);
  if (userId != null) {
    final normalizedUserId = userId.toString();
    item['user_id'] = normalizedUserId;
    item['userId'] = normalizedUserId;
  }

  final targetUserId =
      _firstValue(item, const ['target_user_id', 'targetUserId']);
  if (targetUserId != null) {
    final normalizedTargetUserId = targetUserId.toString();
    item['target_user_id'] = normalizedTargetUserId;
    item['targetUserId'] = normalizedTargetUserId;
  }

  final stoneId = _firstValue(item, const ['stone_id', 'stoneId']);
  if (stoneId != null) {
    final normalizedStoneId = stoneId.toString();
    item['stone_id'] = normalizedStoneId;
    item['stoneId'] = normalizedStoneId;
  }

  final friendshipId =
      _firstValue(item, const ['friendship_id', 'friendshipId']);
  if (friendshipId != null) {
    final normalizedFriendshipId = friendshipId.toString();
    item['friendship_id'] = normalizedFriendshipId;
    item['friendshipId'] = normalizedFriendshipId;
  }

  final friendId = _firstValue(item, const ['friend_id', 'friendId']);
  if (friendId != null) {
    final normalizedFriendId = friendId.toString();
    item['friend_id'] = normalizedFriendId;
    item['friendId'] = normalizedFriendId;
  }

  final createdAt = _firstValue(item, const ['created_at', 'createdAt']);
  if (createdAt != null) {
    item['created_at'] = createdAt;
    item['createdAt'] = createdAt;
  }

  final expiresAt = _firstValue(item, const ['expires_at', 'expiresAt']);
  if (expiresAt != null) {
    item['expires_at'] = expiresAt;
    item['expiresAt'] = expiresAt;
  }

  return item;
}

Map<String, dynamic> normalizeBoatPayload(Map raw) {
  final item = _asMap(raw);

  final boatId = _firstValue(item, const ['boat_id', 'boatId', 'id']);
  if (boatId != null) {
    final normalizedId = boatId.toString();
    item['boat_id'] = normalizedId;
    item['boatId'] = normalizedId;
    item['id'] = normalizedId;
  }

  final stoneId = _firstValue(item, const ['stone_id', 'stoneId']);
  if (stoneId != null) {
    final normalizedStoneId = stoneId.toString();
    item['stone_id'] = normalizedStoneId;
    item['stoneId'] = normalizedStoneId;
  }

  final senderId =
      _firstValue(item, const ['sender_id', 'senderId', 'user_id', 'userId']);
  if (senderId != null) {
    final normalizedSender = senderId.toString();
    item['sender_id'] = normalizedSender;
    item['senderId'] = normalizedSender;
  }

  final isAiReply = _toBool(
        _firstValue(item, const ['is_ai_reply', 'isAiReply', 'is_ai']),
      ) ==
      true;
  if (isAiReply) {
    item['is_ai_reply'] = true;
    item['isAiReply'] = true;
  }

  final receiverId = _firstValue(
    item,
    const ['receiver_id', 'receiverId', 'target_user_id', 'targetUserId'],
  );
  if (receiverId != null) {
    final normalizedReceiver = receiverId.toString();
    item['receiver_id'] = normalizedReceiver;
    item['receiverId'] = normalizedReceiver;
  }

  final createdAt = _firstValue(item, const ['created_at', 'createdAt']);
  if (createdAt != null) {
    item['created_at'] = createdAt;
    item['createdAt'] = createdAt;
  }

  final color =
      _firstValue(item, const ['boat_color', 'boatColor', 'boat_style']);
  if (color != null) {
    item['boat_color'] = color;
    item['boatColor'] = color;
  }

  final stoneMoodType = _firstValue(
    item,
    const ['stone_mood_type', 'stoneMoodType', 'mood_type', 'moodType'],
  );
  if (stoneMoodType != null) {
    item['stone_mood_type'] = stoneMoodType;
    item['stoneMoodType'] = stoneMoodType;
  }

  final sender = item['sender'];
  if (sender is Map) {
    item['sender'] = normalizeFriendPayload(sender);
  }
  final author = item['author'];
  if (author is Map) {
    item['author'] = normalizeFriendPayload(author);
  }
  final receiver = item['receiver'];
  if (receiver is Map) {
    item['receiver'] = normalizeFriendPayload(receiver);
  }

  final senderMap =
      item['sender'] is Map ? _asMap(item['sender'] as Map) : null;
  final authorMap =
      item['author'] is Map ? _asMap(item['author'] as Map) : null;
  final senderName = _firstValue(item, const ['sender_name', 'senderName']) ??
      senderMap?['nickname'] ??
      authorMap?['nickname'] ??
      item['nickname'];
  if (senderName != null) {
    item['sender_name'] = senderName.toString();
    item['senderName'] = senderName.toString();
  }

  final senderKey = _firstValue(item, const ['sender_id', 'senderId'])
      ?.toString()
      .toLowerCase();
  final isLakeGodSender =
      isAiReply || senderKey == 'ai_lakegod' || senderKey == 'lake_god';
  if (isLakeGodSender) {
    item['is_ai_reply'] = true;
    item['isAiReply'] = true;
    item['sender_name'] = '湖神';
    item['senderName'] = '湖神';
    item['agent_name'] = item['agent_name'] ?? '湖神';
    item['agentName'] = item['agentName'] ?? '湖神';
    final aiAuthor = {
      ...?authorMap,
      'nickname': '湖神',
      'is_anonymous': false,
      'isAnonymous': false,
      'is_ai_reply': true,
      'isAiReply': true,
      'agent_name': '湖神',
      'agentName': '湖神',
    };
    item['author'] = aiAuthor;
    item['sender'] = {
      ...?senderMap,
      ...aiAuthor,
    };
  }

  return item;
}

Map<String, dynamic> normalizeRipplePayload(Map raw) {
  final item = _asMap(raw);

  final rippleId = _firstValue(item, const ['ripple_id', 'rippleId', 'id']);
  if (rippleId != null) {
    final normalizedId = rippleId.toString();
    item['ripple_id'] = normalizedId;
    item['rippleId'] = normalizedId;
    item['id'] = normalizedId;
  }

  final stoneId = _firstValue(item, const ['stone_id', 'stoneId']);
  if (stoneId != null) {
    final normalizedStoneId = stoneId.toString();
    item['stone_id'] = normalizedStoneId;
    item['stoneId'] = normalizedStoneId;
  }

  final userId =
      _firstValue(item, const ['user_id', 'userId', 'stone_user_id']);
  if (userId != null) {
    final normalizedUserId = userId.toString();
    item['user_id'] = normalizedUserId;
    item['userId'] = normalizedUserId;
    item['stone_user_id'] = normalizedUserId;
  }

  final createdAt = _firstValue(item, const ['created_at', 'createdAt']);
  if (createdAt != null) {
    item['created_at'] = createdAt;
    item['createdAt'] = createdAt;
  }

  final content =
      _firstValue(item, const ['stone_content', 'stoneContent', 'content']);
  if (content != null) {
    item['stone_content'] = content;
    item['stoneContent'] = content;
    item['content'] = content;
  }

  final moodType = _firstValue(
    item,
    const ['stone_mood_type', 'stoneMoodType', 'mood_type', 'moodType'],
  );
  if (moodType != null) {
    item['stone_mood_type'] = moodType;
    item['stoneMoodType'] = moodType;
    item['mood_type'] = moodType;
    item['moodType'] = moodType;
  }

  return item;
}

Map<String, dynamic> normalizeMessagePayload(Map raw) {
  final item = _asMap(raw);

  final messageId = _firstValue(item, const ['message_id', 'messageId', 'id']);
  if (messageId != null) {
    final normalizedId = messageId.toString();
    item['message_id'] = normalizedId;
    item['messageId'] = normalizedId;
  }

  final senderId =
      _firstValue(item, const ['sender_id', 'senderId', 'user_id', 'userId']);
  if (senderId != null) {
    final normalizedSender = senderId.toString();
    item['sender_id'] = normalizedSender;
    item['senderId'] = normalizedSender;
  }

  final createdAt =
      _firstValue(item, const ['created_at', 'createdAt', 'time']);
  if (createdAt != null) {
    item['created_at'] = createdAt;
    item['createdAt'] = createdAt;
  }

  return item;
}

Map<String, dynamic> normalizeNotificationPayload(Map raw) {
  final normalized = normalizePayloadContract(_asMap(raw));
  final nestedData = normalized['data'];
  final item = nestedData is Map
      ? {
          ...normalized,
          ...normalizePayloadContract(_asMap(nestedData)),
        }
      : normalized;

  final eventType = normalized['type']?.toString();
  final notificationType =
      _firstValue(item, const ['notification_type', 'type'])?.toString();
  if (notificationType != null && notificationType.isNotEmpty) {
    if (eventType != null &&
        eventType.isNotEmpty &&
        eventType != notificationType) {
      item['event_type'] = eventType;
    }
    item['type'] = notificationType;
    item['notification_type'] = notificationType;
  }

  final notificationId = _firstValue(
    item,
    const ['notification_id', 'notificationId', 'id'],
  );
  if (notificationId != null) {
    final normalizedId = notificationId.toString();
    item['notification_id'] = normalizedId;
    item['notificationId'] = normalizedId;
    item['id'] = normalizedId;
  }

  final relatedId = _firstValue(item, const [
    'related_id',
    'relatedId',
    'target_id',
    'targetId',
    'stone_id',
    'stoneId',
    'friend_id',
    'friendId',
  ]);
  if (relatedId != null) {
    final normalizedRelatedId = relatedId.toString();
    item['related_id'] = normalizedRelatedId;
    item['relatedId'] = normalizedRelatedId;
  }

  final stoneId = _firstValue(item, const ['stone_id', 'stoneId']);
  if (stoneId != null &&
      notificationType != null &&
      const {'ripple', 'boat', 'ai_reply'}.contains(notificationType)) {
    final normalizedStoneId = stoneId.toString();
    item['target_id'] = normalizedStoneId;
    item['targetId'] = normalizedStoneId;
    item['stone_id'] = normalizedStoneId;
    item['stoneId'] = normalizedStoneId;
  } else if (relatedId != null) {
    final normalizedRelatedId = relatedId.toString();
    item['target_id'] = normalizedRelatedId;
    item['targetId'] = normalizedRelatedId;
  }

  final isRead = _toBool(_firstValue(item, const ['is_read', 'isRead']));
  if (isRead != null) {
    item['is_read'] = isRead;
    item['isRead'] = isRead;
  }

  return item;
}

Map<String, dynamic> normalizeConsultationSessionPayload(Map raw) {
  final item = _asMap(raw);

  final sessionId = _firstValue(item, const ['session_id', 'sessionId', 'id']);
  if (sessionId != null) {
    final normalizedSessionId = sessionId.toString();
    item['session_id'] = normalizedSessionId;
    item['sessionId'] = normalizedSessionId;
    item['id'] = normalizedSessionId;
  }

  final counterpartId = _firstValue(
    item,
    const [
      'counterpart_id',
      'counterpartId',
      'participant_id',
      'participantId'
    ],
  );
  if (counterpartId != null) {
    final normalizedCounterpartId = counterpartId.toString();
    item['counterpart_id'] = normalizedCounterpartId;
    item['counterpartId'] = normalizedCounterpartId;
    item['participant_id'] = normalizedCounterpartId;
    item['participantId'] = normalizedCounterpartId;
    item['counselor_id'] = item['counselor_id'] ?? normalizedCounterpartId;
    item['counselorId'] = item['counselorId'] ?? normalizedCounterpartId;
  }

  final updatedAt = _firstValue(item, const ['updated_at', 'updatedAt']);
  if (updatedAt != null) {
    item['updated_at'] = updatedAt;
    item['updatedAt'] = updatedAt;
  }

  final lastMessage = _firstValue(item, const ['last_message', 'lastMessage']);
  if (lastMessage != null) {
    item['last_message'] = lastMessage;
    item['lastMessage'] = lastMessage;
  }

  final counselorName =
      _firstValue(item, const ['counselor_name', 'counselorName']);
  if (counselorName != null) {
    item['counselor_name'] = counselorName;
    item['counselorName'] = counselorName;
  }

  final counselorAvatar = _firstValue(
    item,
    const [
      'counselor_avatar_url',
      'counselorAvatarUrl',
      'avatar_url',
      'avatarUrl'
    ],
  );
  if (counselorAvatar != null) {
    item['counselor_avatar_url'] = counselorAvatar;
    item['counselorAvatarUrl'] = counselorAvatar;
    item['avatar_url'] = counselorAvatar;
    item['avatarUrl'] = counselorAvatar;
  }

  return item;
}
