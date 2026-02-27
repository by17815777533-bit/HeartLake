// 互动服务 - 处理涟漪和纸船

import '../../utils/input_validator.dart';
import '../../di/service_locator.dart';
import 'base_service.dart';
import 'stone_service.dart';
class InteractionService extends BaseService {
  @override
  String get serviceName => 'InteractionService';

  final StoneService _stoneService = sl<StoneService>();

  // 创建涟漪（点赞）
  Future<Map<String, dynamic>> createRipple(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    final response = await post('/stones/$stoneId/ripples');
    if (!response.success) return toMap(response);

    return {
      'success': true,
      'data': response.data,
      'ripple_id': response.data?['ripple_id'],
    };
  }

  // 发送纸船（评论）
  Future<Map<String, dynamic>> createBoat({
    required String stoneId,
    required String content,
    bool isAnonymous = true,
  }) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    InputValidator.requireLength(content, '纸船内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    final response = await post('/stones/$stoneId/boats', data: {
      'content': content,
      if (isAnonymous) 'is_anonymous': true,
    });
    if (!response.success) return toMap(response);

    return {
      'success': true,
      'data': response.data,
      'boat_id': response.data?['boat_id'],
    };
  }

  // 获取石头的纸船列表
  Future<Map<String, dynamic>> getBoats(
    String stoneId, {
    int page = 1,
    int pageSize = 20,
  }) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/stones/$stoneId/boats', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    // API 返回 data 直接是 boats 数组
    final boats = response.data is List ? response.data : (response.data is Map ? (response.data?['boats'] ?? response.data?['items'] ?? []) : []);
    return {
      'success': true,
      'boats': boats,
    };
  }

  // 发起限时会话（基于石头作者）
  Future<Map<String, dynamic>> createConnectionByStone(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    final response = await post('/stones/$stoneId/connections');
    return toMap(response);
  }

  // 发起临时连接（聊天邀请）
  Future<Map<String, dynamic>> createConnection(String targetUserId) async {
    InputValidator.validateUUID(targetUserId, '目标用户ID');
    final response = await post('/connections', data: {
      'target_user_id': targetUserId,
    });
    return toMap(response);
  }

  // 升级为好友
  Future<Map<String, dynamic>> upgradeConnectionToFriend(
      String connectionId) async {
    InputValidator.validateUUID(connectionId, '连接ID');
    final response = await post('/connections/$connectionId/friend');
    return toMap(response);
  }

  // 获取会话消息
  Future<Map<String, dynamic>> getMessages(String connectionId) async {
    InputValidator.validateUUID(connectionId, '连接ID');
    final response = await get('/connections/$connectionId/messages');
    if (!response.success) return toMap(response);

    final messages = response.data is List
        ? response.data
        : (response.data is Map ? (response.data?['items'] ?? response.data?['messages'] ?? []) : []);
    return {
      'success': true,
      'messages': messages,
    };
  }

  // 发送消息
  Future<Map<String, dynamic>> sendMessage({
    required String connectionId,
    required String content,
    String messageType = 'text',
    List<String>? mediaIds,
    int? voiceDuration,
  }) async {
    InputValidator.validateUUID(connectionId, '连接ID');
    InputValidator.requireLength(content, '消息内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    InputValidator.requireInList(messageType, const ['text', 'image', 'voice'], '消息类型');
    if (mediaIds != null) {
      InputValidator.requireListLength(mediaIds, '媒体文件', max: 9);
    }
    final response = await post('/connections/$connectionId/messages', data: {
      'content': content,
      'message_type': messageType,
      if (mediaIds != null && mediaIds.isNotEmpty) 'media_ids': mediaIds,
      if (voiceDuration != null) 'voice_duration': voiceDuration,
    });
    if (!response.success) return toMap(response);

    return {
      'success': true,
      'message':
          response.data is Map ? response.data['message'] : response.data,
    };
  }

  // 获取我的涟漪
  Future<Map<String, dynamic>> getMyRipples({
    int page = 1,
    int pageSize = 20,
  }) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/interactions/my/ripples', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    final ripples = response.data is List
        ? response.data
        : (response.data is Map ? (response.data?['items'] ?? response.data?['ripples'] ?? []) : []);
    return {
      'success': true,
      'ripples': ripples,
    };
  }

  // 获取我的纸船（评论）
  Future<Map<String, dynamic>> getMyBoatsComments({
    int page = 1,
    int pageSize = 20,
  }) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/interactions/my/boats', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    // API 返回 data 直接是 boats 数组
    final myBoats = response.data is List ? response.data : (response.data is Map ? (response.data?['boats'] ?? response.data?['items'] ?? []) : []);
    return {
      'success': true,
      'boats': myBoats,
    };
  }

  // 删除石头
  Future<Map<String, dynamic>> deleteStone(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    return await _stoneService.deleteStone(stoneId);
  }

  // 删除纸船（评论）
  Future<Map<String, dynamic>> deleteBoat(String boatId) async {
    InputValidator.validateUUID(boatId, '纸船ID');
    final response = await delete('/boats/$boatId');
    return toMap(response);
  }

  // 取消涟漪
  Future<Map<String, dynamic>> deleteRipple(String rippleId) async {
    InputValidator.validateUUID(rippleId, '涟漪ID');
    final response = await delete('/ripples/$rippleId');
    return toMap(response);
  }
}
