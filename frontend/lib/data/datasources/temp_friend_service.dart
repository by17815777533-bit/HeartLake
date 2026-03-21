import 'base_service.dart';
import '../../utils/input_validator.dart';
import 'social_payload_normalizer.dart';

/// 临时好友服务，管理24小时限时好友关系的创建、升级和删除
class TempFriendService extends BaseService {
  @override
  String get serviceName => 'TempFriendService';

  Map<String, dynamic> _normalizeTempFriend(Map raw) {
    final item = normalizeFriendPayload(raw);
    final friendId = item['friend_id'] ?? item['friend_user_id'];
    if (friendId != null) {
      item['friend_id'] = friendId;
      item['friend_user_id'] = friendId;
    }
    return item;
  }

  /// 获取当前用户的所有临时好友
  Future<Map<String, dynamic>> getMyTempFriends() async {
    final response = await get('/temp-friends');
    if (!response.success) return toMap(response);

    final tempFriends = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeTempFriend,
      listKeys: const ['temp_friends', 'friends', 'items', 'list'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'temp_friends',
        items: tempFriends,
      ),
    };
  }

  /// 获取指定临时好友的详细信息
  Future<Map<String, dynamic>> getTempFriendDetail(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    final response = await get('/temp-friends/$tempFriendId');
    if (!response.success) return toMap(response);

    final detail = response.data is Map
        ? _normalizeTempFriend(response.data as Map)
        : response.data;

    return {
      ...toMap(response),
      'temp_friend': detail,
    };
  }

  /// 将临时好友升级为永久好友关系
  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    final response = await post('/temp-friends/$tempFriendId/upgrade');
    if (!response.success) return toMap(response);

    return {
      ...toMap(response),
      'friendship_id': response.data?['friendship_id'],
    };
  }

  /// 删除指定的临时好友关系
  Future<Map<String, dynamic>> deleteTempFriend(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    return toMap(await delete('/temp-friends/$tempFriendId'));
  }

  /// 创建临时好友
  Future<Map<String, dynamic>> createTempFriend(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/temp-friends', data: {
      'target_user_id': userId,
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
