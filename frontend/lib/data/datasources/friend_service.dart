/// 好友关系管理服务
///
/// 处理好友请求的发送、接受、拒绝以及聊天消息收发。

import '../../utils/input_validator.dart';
import 'base_service.dart';
import 'social_payload_normalizer.dart';

/// 好友关系管理服务
///
/// 提供好友关系管理和聊天功能。
class FriendService extends BaseService {
  @override
  String get serviceName => 'FriendService';

  /// 发送好友请求
  ///
  /// 向指定用户发送好友请求，可附带一条附言。
  ///
  /// [userId] 目标用户ID
  /// [message] 附言，最长200字符
  Future<Map<String, dynamic>> sendFriendRequest({
    required String userId,
    String? message,
  }) async {
    InputValidator.validateUUID(userId, '用户ID');
    if (message != null) {
      InputValidator.requireLength(message, '附言', max: 200);
      message = InputValidator.sanitizeText(message);
    }
    final response = await post('/friends/request', data: {
      'target_user_id': userId,
      if (message != null) 'message': message,
    });

    if (!response.success) return toMap(response);

    final payload = response.data is Map
        ? normalizeFriendPayload(response.data as Map)
        : response.data;

    return {
      ...toMap(response),
      'data': payload,
      'request_id':
          response.data is Map ? (response.data as Map)['request_id'] : null,
    };
  }

  /// 接受好友请求
  ///
  /// [userId] 发送请求的用户ID
  Future<Map<String, dynamic>> acceptFriendRequest(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/friends/accept/$userId');
    return toMap(response);
  }

  /// 拒绝好友请求
  ///
  /// [userId] 发送请求的用户ID
  Future<Map<String, dynamic>> rejectFriendRequest(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/friends/reject/$userId');
    return toMap(response);
  }

  /// 删除好友
  ///
  /// [friendId] 好友ID
  Future<Map<String, dynamic>> removeFriend(String friendId) async {
    InputValidator.validateUUID(friendId, '好友ID');
    final response = await delete('/friends/$friendId');
    return toMap(response);
  }

  /// 获取好友列表
  ///
  /// 返回当前用户的好友列表和总数。
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
      ),
    };
  }

  /// 获取待处理的好友请求
  ///
  /// 返回尚未处理的好友请求列表。
  Future<Map<String, dynamic>> getPendingRequests() async {
    final response = await get('/friends/requests/pending');

    if (!response.success) return toMap(response);

    final requests = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeFriendPayload,
      listKeys: const ['requests', 'items', 'list'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'requests',
        items: requests,
      ),
    };
  }

  /// 获取聊天记录
  ///
  /// 获取与指定好友的聊天记录，兼容多种返回格式。
  ///
  /// [friendId] 好友ID
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
      ),
    };
  }

  /// 发送聊天消息
  ///
  /// [friendId] 好友ID
  /// [content] 消息内容，1-2000字符
  Future<Map<String, dynamic>> sendMessage(
      String friendId, String content) async {
    InputValidator.validateUUID(friendId, '好友ID');
    InputValidator.requireLength(content, '消息内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    final response = await post('/friends/$friendId/messages', data: {
      'content': content,
    });
    if (!response.success) return toMap(response);

    final payload = response.data is Map
        ? normalizeMessagePayload(response.data as Map)
        : response.data;
    return {
      ...toMap(response),
      'data': payload,
      'message': payload,
    };
  }
}
