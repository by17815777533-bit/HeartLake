// 互动服务
//
// 处理石头的涟漪（点赞）和纸船（评论）等社交互动操作，
// 以及限时会话、消息收发和内容删除功能。
// 对示例石头（showcase_stone_ 前缀）做只读保护，禁止互动操作。
// 依赖 BaseService 提供的 HTTP 方法，依赖 StoneService 代理石头删除。

import '../../utils/input_validator.dart';
import '../../di/service_locator.dart';
import 'base_service.dart';
import 'social_payload_normalizer.dart';
import 'stone_service.dart';

abstract class InteractionDataSource {
  Future<Map<String, dynamic>> createRipple(String stoneId);
  Future<Map<String, dynamic>> createBoat({
    required String stoneId,
    required String content,
    bool isAnonymous = true,
  });
  Future<Map<String, dynamic>> getBoats(
    String stoneId, {
    int page = 1,
    int pageSize = 20,
  });
  Future<Map<String, dynamic>> getMyRipples({
    int page = 1,
    int pageSize = 20,
  });
  Future<Map<String, dynamic>> getMyBoatsComments({
    int page = 1,
    int pageSize = 20,
  });
  Future<Map<String, dynamic>> createConnectionByStone(String stoneId);
  Future<Map<String, dynamic>> deleteStone(String stoneId);
  Future<Map<String, dynamic>> deleteBoat(String boatId);
  Future<Map<String, dynamic>> deleteRipple(String rippleId);
}

/// 互动服务
///
/// 封装涟漪、纸船、限时会话等社交互动接口。
/// 所有写操作会先校验输入参数，并对示例石头做只读拦截。
class InteractionService extends BaseService implements InteractionDataSource {
  @override
  String get serviceName => 'InteractionService';

  final StoneService _stoneService = sl<StoneService>();

  /// 判断是否为只读的示例石头
  bool _isReadonlyShowcaseStone(String stoneId) {
    return stoneId.startsWith('showcase_stone_');
  }

  /// 将示例石头的 404 错误转换为更友好的用户提示
  ///
  /// 示例石头在后端不存在实体，请求会返回 404，
  /// 这里统一包装为引导用户刷新列表的提示文案。
  Map<String, dynamic> _friendlyStoneNotFound(
      Map<String, dynamic> raw, String stoneId) {
    final message = raw['message']?.toString() ?? '';
    if (stoneId.startsWith('showcase_stone_') && message.contains('石头不存在')) {
      return {
        ...raw,
        'message': '石头不存在或已被移除，请刷新列表后重试',
      };
    }
    return raw;
  }

  /// 创建涟漪（点赞）
  ///
  /// [stoneId] 目标石头ID，示例石头会被拒绝操作
  @override
  Future<Map<String, dynamic>> createRipple(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    if (_isReadonlyShowcaseStone(stoneId)) {
      return {
        'success': false,
        'code': 'SHOWCASE_READ_ONLY',
        'message': '示例石头仅供浏览，不能进行涟漪操作',
      };
    }
    final response = await post('/stones/$stoneId/ripples');
    if (!response.success) {
      return _friendlyStoneNotFound(toMap(response), stoneId);
    }

    final payload = response.data is Map<String, dynamic>
        ? response.data as Map<String, dynamic>
        : <String, dynamic>{};

    return {
      'success': true,
      'data': response.data,
      'message': response.message,
      'ripple_id': payload['ripple_id'],
      'ripple_count': payload['ripple_count'],
      'already_rippled': payload['already_rippled'] == true,
    };
  }

  /// 发送纸船（评论）
  ///
  /// [stoneId] 目标石头ID
  /// [content] 纸船内容，1-2000字符
  /// [isAnonymous] 是否匿名发送，默认true
  @override
  Future<Map<String, dynamic>> createBoat({
    required String stoneId,
    required String content,
    bool isAnonymous = true,
  }) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    if (_isReadonlyShowcaseStone(stoneId)) {
      return {
        'success': false,
        'code': 'SHOWCASE_READ_ONLY',
        'message': '示例石头仅供浏览，不能发送纸船',
      };
    }
    InputValidator.requireLength(content, '纸船内容', min: 1, max: 2000);
    content = InputValidator.sanitizeText(content);
    final response = await post('/stones/$stoneId/boats', data: {
      'content': content,
      if (isAnonymous) 'is_anonymous': true,
    });
    if (!response.success) {
      return _friendlyStoneNotFound(toMap(response), stoneId);
    }

    return {
      'success': true,
      'data': response.data,
      'boat_id': response.data?['boat_id'],
    };
  }

  /// 获取石头的纸船列表
  ///
  /// [stoneId] 目标石头ID
  /// [page] 页码，从1开始
  /// [pageSize] 每页数量，默认20条
  @override
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
    if (!response.success) {
      return _friendlyStoneNotFound(toMap(response), stoneId);
    }

    final boats = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeBoatPayload,
      listKeys: const ['boats', 'items'],
    );
    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'boats',
        items: boats,
      ),
    };
  }

  /// 发起限时会话（基于石头作者建立临时连接）
  ///
  /// 用户通过石头发起与作者的限时聊天，后端会创建一个有过期时间的 connection。
  @override
  Future<Map<String, dynamic>> createConnectionByStone(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    final response = await post('/stones/$stoneId/connections');
    return toMap(response);
  }

  /// 发起临时连接（直接向目标用户发起聊天邀请）
  ///
  /// 与 [createConnectionByStone] 不同，这里直接指定目标用户而非通过石头。
  Future<Map<String, dynamic>> createConnection(String targetUserId) async {
    InputValidator.validateUUID(targetUserId, '目标用户ID');
    final response = await post('/connections', data: {
      'target_user_id': targetUserId,
    });
    return toMap(response);
  }

  /// 将限时连接升级为正式好友关系
  ///
  /// 双方在限时会话期间可选择升级为永久好友。
  Future<Map<String, dynamic>> upgradeConnectionToFriend(
      String connectionId) async {
    InputValidator.validateUUID(connectionId, '连接ID');
    final response = await post('/connections/$connectionId/friend');
    return toMap(response);
  }

  /// 获取会话消息列表
  ///
  /// 返回指定连接下的所有聊天消息，兼容后端返回 List 或 Map 两种格式。
  Future<Map<String, dynamic>> getMessages(String connectionId) async {
    InputValidator.validateUUID(connectionId, '连接ID');
    final response = await get('/connections/$connectionId/messages');
    if (!response.success) return toMap(response);

    final messages = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeMessagePayload,
      listKeys: const ['items', 'messages'],
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

  /// 发送消息到指定会话
  ///
  /// 支持文本、图片、语音三种消息类型。
  /// 文本内容会经过 [InputValidator.sanitizeText] 过滤 XSS 风险字符。
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
    InputValidator.requireInList(
        messageType, const ['text', 'image', 'voice'], '消息类型');
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

    final payload = response.data is Map
        ? normalizeMessagePayload(response.data as Map)
        : response.data;

    return {
      'success': true,
      'message': payload,
      'data': payload,
    };
  }

  /// 获取当前用户的涟漪列表
  ///
  /// 返回用户产生过涟漪的石头信息，兼容后端返回 List 或 Map 两种格式。
  @override
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

    final ripples = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeRipplePayload,
      listKeys: const ['items', 'ripples'],
    );
    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'ripples',
        items: ripples,
      ),
    };
  }

  /// 获取当前用户发送的纸船（评论）列表
  ///
  /// 返回用户发出的所有纸船及其关联石头信息。
  @override
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

    final myBoats = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeBoatPayload,
      listKeys: const ['boats', 'items'],
    );
    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'boats',
        items: myBoats,
      ),
    };
  }

  /// 删除石头（代理到 [StoneService]）
  @override
  Future<Map<String, dynamic>> deleteStone(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    return await _stoneService.deleteStone(stoneId);
  }

  /// 删除纸船（评论）
  ///
  /// [boatId] 纸船ID
  @override
  Future<Map<String, dynamic>> deleteBoat(String boatId) async {
    InputValidator.validateUUID(boatId, '纸船ID');
    final response = await delete('/boats/$boatId');
    return toMap(response);
  }

  /// 取消涟漪
  ///
  /// [rippleId] 涟漪ID
  @override
  Future<Map<String, dynamic>> deleteRipple(String rippleId) async {
    InputValidator.validateUUID(rippleId, '涟漪ID');
    final response = await delete('/ripples/$rippleId');
    return toMap(response);
  }
}
