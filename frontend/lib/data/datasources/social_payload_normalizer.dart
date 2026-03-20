import 'dart:collection';

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
  return LinkedHashSet<String>.from([...listKeys, 'items', 'list']).toList();
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
        final value = data[key];
        if (value is! List) continue;
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
  final source = sources.isNotEmpty ? sources.first : <String, dynamic>{};

  Map<String, dynamic> pagination = <String, dynamic>{};
  for (final candidate in sources) {
    final value = candidate['pagination'];
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

  final total = _toInt(read(const ['total'])) ?? itemCount;
  final page = _toInt(read(const ['page'])) ?? 1;
  final pageSize = _toInt(read(const ['page_size', 'pageSize'])) ?? itemCount;
  final totalPages = _toInt(read(const ['total_pages', 'totalPages'])) ??
      (pageSize > 0 ? (total + pageSize - 1) ~/ pageSize : 0);
  final hasMore = read(const ['has_more']) == true ||
      (pageSize > 0 && page * pageSize < total);

  return {
    'total': total,
    'page': page,
    'page_size': pageSize,
    'pageSize': pageSize,
    'total_pages': totalPages,
    'totalPages': totalPages,
    'has_more': hasMore,
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
