import 'base_service.dart';
import '../../utils/input_validator.dart';
import 'social_payload_normalizer.dart';

abstract class TempFriendDataSource {
  Future<Map<String, dynamic>> getMyTempFriends();
  Future<Map<String, dynamic>> getTempFriendDetail(String tempFriendId);
  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId);
  Future<Map<String, dynamic>> deleteTempFriend(String tempFriendId);
  Future<Map<String, dynamic>> createTempFriend(String userId);
  Future<Map<String, dynamic>> checkTempFriendStatus(String userId);
}

/// 临时好友服务，管理24小时限时好友关系的创建、升级和删除
class TempFriendService extends BaseService implements TempFriendDataSource {
  @override
  String get serviceName => 'TempFriendService';

  Map<String, dynamic> _normalizeTempFriend(Map raw) {
    final item = normalizeFriendPayload(raw);
    final tempFriendId =
        item['temp_friend_id'] ?? item['tempFriendId'] ?? item['id'];
    if (tempFriendId != null) {
      item['temp_friend_id'] = tempFriendId.toString();
      item['tempFriendId'] = tempFriendId.toString();
      item['id'] = tempFriendId.toString();
    }

    final friendId = item['friend_id'] ?? item['friend_user_id'];
    if (friendId != null) {
      item['friend_id'] = friendId;
      item['friendId'] = friendId;
      item['friend_user_id'] = friendId;
      item['friendUserId'] = friendId;
    }

    final expiresAt = item['expires_at'] ?? item['expiresAt'];
    if (expiresAt != null) {
      item['expires_at'] = expiresAt;
      item['expiresAt'] = expiresAt;
    }

    final createdAt = item['created_at'] ?? item['createdAt'];
    if (createdAt != null) {
      item['created_at'] = createdAt;
      item['createdAt'] = createdAt;
    }

    final friendshipId = item['friendship_id'] ?? item['friendshipId'];
    if (friendshipId != null) {
      item['friendship_id'] = friendshipId.toString();
      item['friendshipId'] = friendshipId.toString();
    }

    final upgradedToFriend = item['upgraded_to_friend'] ?? item['upgradedToFriend'];
    if (upgradedToFriend != null) {
      item['upgraded_to_friend'] = upgradedToFriend;
      item['upgradedToFriend'] = upgradedToFriend;
    }

    return item;
  }

  /// 获取当前用户的所有临时好友
  @override
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
        requireExplicitTotal: true,
      ),
    };
  }

  /// 获取指定临时好友的详细信息
  @override
  Future<Map<String, dynamic>> getTempFriendDetail(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    final response = await get('/temp-friends/$tempFriendId');
    if (!response.success) return toMap(response);

    final detail = response.data is Map
        ? _normalizeTempFriend(response.data as Map)
        : response.data;

    return {
      ...toMap(response),
      'data': detail,
      'temp_friend': detail,
    };
  }

  /// 将临时好友升级为永久好友关系
  @override
  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    final response = await post('/temp-friends/$tempFriendId/upgrade');
    if (!response.success) return toMap(response);

    final payload = response.data is Map
        ? _normalizeTempFriend(
            Map<String, dynamic>.from((response.data as Map).cast<String, dynamic>()),
          )
        : const <String, dynamic>{};
    return {
      ...toMap(response),
      'data': payload,
      'friendship_id': payload['friendship_id'],
      'friendshipId': payload['friendshipId'],
    };
  }

  /// 删除指定的临时好友关系
  @override
  Future<Map<String, dynamic>> deleteTempFriend(String tempFriendId) async {
    InputValidator.validateUUID(tempFriendId, '临时好友ID');
    return toMap(await delete('/temp-friends/$tempFriendId'));
  }

  /// 创建临时好友
  @override
  Future<Map<String, dynamic>> createTempFriend(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/temp-friends', data: {
      'target_user_id': userId,
    });
    if (!response.success) return toMap(response);

    final payload = response.data is Map
        ? _normalizeTempFriend(
            Map<String, dynamic>.from((response.data as Map).cast<String, dynamic>()),
          )
        : const <String, dynamic>{};
    return {
      ...toMap(response),
      'data': payload,
      'temp_friend_id': payload['temp_friend_id'],
      'tempFriendId': payload['tempFriendId'],
    };
  }

  /// 检查临时好友状态
  @override
  Future<Map<String, dynamic>> checkTempFriendStatus(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await get('/temp-friends/check/$userId');
    if (!response.success) return toMap(response);

    final payload = response.data is Map
        ? _normalizeTempFriend(
            Map<String, dynamic>.from((response.data as Map).cast<String, dynamic>()),
          )
        : const <String, dynamic>{};
    return {
      ...toMap(response),
      'data': payload,
      'is_temp_friend': payload['is_temp_friend'],
      'temp_friend_id': payload['temp_friend_id'],
      'tempFriendId': payload['tempFriendId'],
    };
  }
}
