// @file temp_friend_service.dart
// @brief 临时好友服务 - 24小时好友管理
// Created by 王璐瑶

import 'base_service.dart';
class TempFriendService extends BaseService {
  @override
  String get serviceName => 'TempFriendService';

  Future<Map<String, dynamic>> getMyTempFriends() async {
    final response = await get('/temp-friends');
    if (!response.success) return toMap(response);

    return {
      'success': true,
      'temp_friends': response.data?['friends'] ?? [],
      'total': response.data?['total'] ?? 0,
    };
  }

  Future<Map<String, dynamic>> getTempFriendDetail(String tempFriendId) async {
    final response = await get('/temp-friends/$tempFriendId');
    if (!response.success) return toMap(response);

    return {
      'success': true,
      'temp_friend': response.data,
    };
  }

  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId) async {
    final response = await post('/temp-friends/$tempFriendId/upgrade');
    if (!response.success) return toMap(response);

    return {
      'success': true,
      'friendship_id': response.data?['friendship_id'],
    };
  }

  Future<Map<String, dynamic>> deleteTempFriend(String tempFriendId) async {
    return toMap(await delete('/temp-friends/$tempFriendId'));
  }

  /// 创建临时好友
  Future<Map<String, dynamic>> createTempFriend(String userId) async {
    final response = await post('/temp-friends', data: {
      'user_id': userId,
    });
    return toMap(response);
  }

  /// 检查临时好友状态
  Future<Map<String, dynamic>> checkTempFriendStatus(String userId) async {
    final response = await get('/temp-friends/check/$userId');
    return toMap(response);
  }
}
