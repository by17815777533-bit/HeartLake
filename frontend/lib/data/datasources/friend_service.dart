// 好友关系管理服务
//
// 前端仅保留当前仍在使用的自动好友/临时好友能力，不再维护手动申请流。

import '../../utils/input_validator.dart';
import '../../utils/payload_contract.dart';
import 'base_service.dart';
import 'social_payload_normalizer.dart';

abstract class FriendDataSource {
  Future<Map<String, dynamic>> removeFriend(String friendId);
  Future<Map<String, dynamic>> getFriends();
  Future<Map<String, dynamic>> getMessages(String friendId);
  Future<Map<String, dynamic>> sendMessage(String friendId, String content);
}

/// 好友关系管理服务
///
/// 提供好友关系管理和聊天功能。
class FriendService extends BaseService implements FriendDataSource {
  @override
  String get serviceName => 'FriendService';

  Map<String, dynamic> _normalizeFriendActionPayload(
    dynamic raw, {
    String? peerId,
    String? requestId,
    String? content,
  }) {
    final payload = raw is Map
        ? normalizeFriendPayload(
            normalizePayloadContract(
              Map<String, dynamic>.from(raw.cast<String, dynamic>()),
            ),
          )
        : <String, dynamic>{};

    final resolvedPeerId = peerId ??
        payload['friend_id']?.toString() ??
        payload['friend_user_id']?.toString() ??
        payload['user_id']?.toString() ??
        payload['peer_id']?.toString();
    if (resolvedPeerId != null && resolvedPeerId.isNotEmpty) {
      payload['friend_id'] = resolvedPeerId;
      payload['friendId'] = resolvedPeerId;
      payload['friend_user_id'] = resolvedPeerId;
      payload['friendUserId'] = resolvedPeerId;
      payload['user_id'] = resolvedPeerId;
      payload['userId'] = resolvedPeerId;
      payload['peer_id'] = resolvedPeerId;
      payload['peerId'] = resolvedPeerId;
    }

    final resolvedRequestId = requestId ??
        payload['request_id']?.toString() ??
        payload['requestId']?.toString();
    if (resolvedRequestId != null && resolvedRequestId.isNotEmpty) {
      payload['request_id'] = resolvedRequestId;
      payload['requestId'] = resolvedRequestId;
    }

    if (content != null && content.isNotEmpty && payload['content'] == null) {
      payload['content'] = content;
    }

    return payload;
  }

  /// 删除好友
  ///
  /// [friendId] 好友ID
  @override
  Future<Map<String, dynamic>> removeFriend(String friendId) async {
    InputValidator.validateUUID(friendId, '好友ID');
    final response = await delete('/friends/$friendId');
    return {
      ...toMap(response),
      'data': _normalizeFriendActionPayload(response.data, peerId: friendId),
    };
  }

  /// 获取好友列表
  ///
  /// 返回当前用户的好友列表和总数。
  @override
  Future<Map<String, dynamic>> getFriends() async {
    final response = await get('/friends');

    if (!response.success) return toMap(response);

    final friends = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeFriendPayload,
      listKeys: const ['friends', 'items', 'list'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'friends',
        items: friends,
        requireExplicitTotal: true,
      ),
    };
  }

  /// 获取聊天记录
  ///
  /// 获取与指定好友的聊天记录，兼容多种返回格式。
  ///
  /// [friendId] 好友ID
  @override
  Future<Map<String, dynamic>> getMessages(String friendId) async {
    InputValidator.validateUUID(friendId, '好友ID');
    final response = await get('/friends/$friendId/messages');

    if (!response.success) return toMap(response);

    // 兼容后端两种返回格式：直接数组 或 { "messages": [...] }
    final messages = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeMessagePayload,
      listKeys: const ['messages', 'items'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'messages',
        items: messages,
        requireExplicitTotal: true,
      ),
    };
  }

  /// 发送聊天消息
  ///
  /// [friendId] 好友ID
  /// [content] 消息内容，1-2000字符
  @override
  Future<Map<String, dynamic>> sendMessage(
      String friendId, String content) async {
    InputValidator.validateUUID(friendId, '好友ID');
    InputValidator.requireLength(content, '消息内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    final response = await post('/friends/$friendId/messages', data: {
      'content': content,
    });
    if (!response.success) return toMap(response);
    if (response.data is! Map) {
      throw StateError('Friend message success payload is not a map');
    }
    final payload = normalizeMessagePayload(response.data as Map);
    if ((payload['message_id']?.toString().isEmpty ?? true) &&
        (payload['id']?.toString().isEmpty ?? true)) {
      throw StateError('Friend message success payload is missing message_id');
    }
    return {
      ...toMap(response),
      'data': payload,
      'message': response.message,
    };
  }
}
