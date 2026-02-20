// @file friend_service.dart
// @brief 好友服务 - 处理好友请求和关系管理
// Created by 王璐瑶

import 'base_service.dart';

class FriendService extends BaseService {
  @override
  String get serviceName => 'FriendService';

  // 发送好友请求
  Future<Map<String, dynamic>> sendFriendRequest({
    required String userId,
    String? message,
  }) async {
    final response = await post('/friends/request', data: {
      'user_id': userId,
      if (message != null) 'message': message,
    });

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'request_id': response.data?['request_id'],
      'data': response.data,
    };
  }

  // 接受好友请求
  Future<Map<String, dynamic>> acceptFriendRequest(String userId) async {
    final response = await post('/friends/accept/$userId');
    return toMap(response);
  }

  // 拒绝好友请求
  Future<Map<String, dynamic>> rejectFriendRequest(String userId) async {
    final response = await post('/friends/reject/$userId');
    return toMap(response);
  }

  // 删除好友
  Future<Map<String, dynamic>> removeFriend(String friendId) async {
    final response = await delete('/friends/$friendId');
    return toMap(response);
  }

  // 获取好友列表
  Future<Map<String, dynamic>> getFriends() async {
    final response = await get('/friends');

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'friends': response.data?['friends'],
      'total': response.data?['total'],
    };
  }

  // 获取待处理的好友请求
  Future<Map<String, dynamic>> getPendingRequests() async {
    final response = await get('/friends/requests/pending');

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'requests': response.data?['requests'],
      'total': response.data?['total'],
    };
  }

  // 获取与好友的聊天记录
  Future<Map<String, dynamic>> getMessages(String friendId) async {
    final response = await get('/friends/$friendId/messages');

    if (!response.success) return toMap(response);

    return {
      'success': true,
      'messages': response.data?['messages'] ?? [],
    };
  }

  // 发送消息给好友
  Future<Map<String, dynamic>> sendMessage(String friendId, String content) async {
    final response = await post('/friends/$friendId/messages', data: {
      'content': content,
    });

    return toMap(response);
  }
}
