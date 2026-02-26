// @file temp_friend_service.dart
// @brief 临时好友服务 - 24小时好友管理
// Created by 王璐瑶

import 'base_service.dart';
import '../../utils/input_validator.dart';

class TempFriendService extends BaseService {
  @override
  String get serviceName => 'TempFriendService';

  Future<Map<String, dynamic>> getMyTempFriends() async {
    final response = await get('/temp-friends');
    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'temp_friends': response.data?['friends'] ?? [],
      'total': response.data?['total'] ?? 0,
    };
  }

  Future<Map<String, dynamic>> getTempFriendDetail(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    final response = await get('/temp-friends/$tempFriendId');
    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'temp_friend': response.data,
    };
  }

  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    final response = await post('/temp-friends/$tempFriendId/upgrade');
    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'friendship_id': response.data?['friendship_id'],
    };
  }

  Future<Map<String, dynamic>> deleteTempFriend(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    return toMap(await delete('/temp-friends/$tempFriendId'));
  }

  /// 创建临时好友
  Future<Map<String, dynamic>> createTempFriend(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/temp-friends', data: {
      'user_id': userId,
    });
    return toMap(response);
  }

  /// 检查临时好友状态
  Future<Map<String, dynamic>> checkTempFriendStatus(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await get('/temp-friends/check/$userId');
    return toMap(response);
  }
}
