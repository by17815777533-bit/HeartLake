// 好友服务 - 处理好友请求和关系管理

import '../../utils/input_validator.dart';
import 'base_service.dart';

class FriendService extends BaseService {
  @override
  String get serviceName => 'FriendService';

  // 发送好友请求
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
      'user_id': userId,
      if (message != null) 'message': message,
    });

    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'request_id': response.data?['request_id'],
    };
  }

  // 接受好友请求
  Future<Map<String, dynamic>> acceptFriendRequest(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/friends/accept/$userId');
    return toMap(response);
  }

  // 拒绝好友请求
  Future<Map<String, dynamic>> rejectFriendRequest(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/friends/reject/$userId');
    return toMap(response);
  }

  // 删除好友
  Future<Map<String, dynamic>> removeFriend(String friendId) async {
    InputValidator.validateUUID(friendId, '好友ID');
    final response = await delete('/friends/$friendId');
    return toMap(response);
  }

  // 获取好友列表
  Future<Map<String, dynamic>> getFriends() async {
    final response = await get('/friends');

    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'friends': response.data?['friends'],
      'total': response.data?['total'],
    };
  }

  // 获取待处理的好友请求
  Future<Map<String, dynamic>> getPendingRequests() async {
    final response = await get('/friends/requests/pending');

    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'requests': response.data?['requests'],
      'total': response.data?['total'],
    };
  }

  // 获取与好友的聊天记录
  // 后端返回 { "code": 0, "data": [ {id, sender_id, receiver_id, content, created_at}, ... ] }
  // data 直接是数组，不是 { "messages": [...] }
  Future<Map<String, dynamic>> getMessages(String friendId) async {
    InputValidator.validateUUID(friendId, '好友ID');
    final response = await get('/friends/$friendId/messages');

    if (!response.success) return toMap(response);

    // response.data 可能是 List（后端直接返回数组）或 Map
    final rawData = response.data;
    List messages;
    if (rawData is List) {
      messages = rawData;
    } else if (rawData is Map) {
      messages = rawData['messages'] ?? [];
    } else {
      messages = [];
    }

    return {
      'success': true,
      'messages': messages,
    };
  }

  // 发送消息给好友
  Future<Map<String, dynamic>> sendMessage(String friendId, String content) async {
    InputValidator.validateUUID(friendId, '好友ID');
    InputValidator.requireLength(content, '消息内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    final response = await post('/friends/$friendId/messages', data: {
      'content': content,
    });

    return toMap(response);
  }
}
