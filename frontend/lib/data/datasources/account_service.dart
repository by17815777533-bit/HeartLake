// 账号管理服务
//
// 提供用户账号全生命周期管理能力，包括账号信息查询、隐私设置、
// 黑名单管理、登录设备审计、个人资料编辑和数据导出等功能。
//
// 所有写入操作均通过InputValidator做前置校验，确保只有白名单内的字段
// 才能提交到后端，防止非法字段注入和XSS攻击。

import '../../utils/input_validator.dart';
import 'base_service.dart';
import 'social_payload_normalizer.dart';

/// 账号管理服务
///
/// 提供账号信息查询、隐私设置、黑名单管理等功能。
class AccountService extends BaseService {
  @override
  String get serviceName => 'AccountService';

  Map<String, dynamic> _normalizePayload(Map<String, dynamic> payload) {
    final normalized = Map<String, dynamic>.from(payload);
    final rawData = normalized['data'];
    if (rawData is Map) {
      final data = Map<String, dynamic>.from(rawData.cast<String, dynamic>());
      final avatar = data['avatar'];
      if ((data['avatar_url'] == null ||
              data['avatar_url'].toString().isEmpty) &&
          avatar != null) {
        data['avatar_url'] = avatar;
      }
      final blockedUsers = data['blocked_users'];
      if (data['items'] == null && blockedUsers is List) {
        data['items'] = blockedUsers;
      }
      normalized['data'] = data;
    }
    return normalized;
  }

  Map<String, dynamic> _normalizeDevicePayload(Map raw) {
    final item = Map<String, dynamic>.from(raw.cast<String, dynamic>());
    final sessionId = item['session_id'] ?? item['sessionId'] ?? item['id'];
    if (sessionId != null) {
      item['session_id'] = sessionId.toString();
      item['sessionId'] = sessionId.toString();
      item['id'] = sessionId.toString();
    }

    final createdAt = item['created_at'] ?? item['createdAt'];
    if (createdAt != null) {
      item['created_at'] = createdAt;
      item['createdAt'] = createdAt;
    }

    final lastActiveAt = item['last_active_at'] ?? item['lastActiveAt'];
    if (lastActiveAt != null) {
      item['last_active_at'] = lastActiveAt;
      item['lastActiveAt'] = lastActiveAt;
    }

    return item;
  }

  Map<String, dynamic> _normalizeLoginLogPayload(Map raw) {
    final item = Map<String, dynamic>.from(raw.cast<String, dynamic>());
    final logId = item['log_id'] ?? item['logId'] ?? item['id'];
    if (logId != null) {
      item['log_id'] = logId.toString();
      item['logId'] = logId.toString();
      item['id'] = logId.toString();
    }

    final loginTime =
        item['login_time'] ?? item['loginTime'] ?? item['created_at'];
    if (loginTime != null) {
      item['login_time'] = loginTime;
      item['loginTime'] = loginTime;
      item['created_at'] = item['created_at'] ?? loginTime;
      item['createdAt'] = item['createdAt'] ?? loginTime;
    }

    return item;
  }

  Map<String, dynamic> _normalizeSecurityEventPayload(Map raw) {
    final item = Map<String, dynamic>.from(raw.cast<String, dynamic>());
    final eventId = item['event_id'] ?? item['eventId'] ?? item['id'];
    if (eventId != null) {
      item['event_id'] = eventId.toString();
      item['eventId'] = eventId.toString();
      item['id'] = eventId.toString();
    }

    final createdAt = item['created_at'] ?? item['createdAt'];
    if (createdAt != null) {
      item['created_at'] = createdAt;
      item['createdAt'] = createdAt;
    }

    return item;
  }

  /// 隐私设置白名单，只有这些 key 才会被提交到后端
  static const _allowedPrivacyKeys = [
    // 后端当前生效字段
    'profile_visibility',
    'show_online_status',
    'allow_friend_request',
    'allow_message_from_stranger',
    // 兼容历史/前端扩展字段（后端可忽略未知字段）
    'allow_stranger_boat',
    'show_mood_history',
    'allow_resonance_match',
    'show_profile_to_stranger',
    'show_emotion_heatmap',
    'allow_stranger_message',
    'show_stone_count',
    'data_collection',
    'personalized_recommendation',
    'anonymous_analytics',
  ];

  /// 个人资料白名单，限制可修改的字段范围
  static const _allowedProfileKeys = [
    'nickname',
    'bio',
    'avatar',
    'avatar_url',
    'gender',
    'birthday',
    'location',
    'email',
  ];

  /// 获取账号信息
  ///
  /// 返回当前用户的基本账号信息。
  Future<Map<String, dynamic>> getAccountInfo() async {
    final response = await get('/account/info');
    return _normalizePayload(toMap(response));
  }

  /// 获取账号统计
  ///
  /// 返回账号的统计数据，如发布石头数、好友数等。
  Future<Map<String, dynamic>> getAccountStats() async {
    final response = await get('/account/stats');
    return toMap(response);
  }

  /// 获取登录设备列表
  ///
  /// 返回当前账号的所有登录设备信息。
  Future<Map<String, dynamic>> getDevices(
      {int page = 1, int pageSize = 20}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/account/devices', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    final devices = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeDevicePayload,
      listKeys: const ['devices'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'devices',
        items: devices,
      ),
    };
  }

  /// 获取登录日志
  ///
  /// 返回当前账号的历史登录记录，用于安全审计。
  ///
  /// [page] 页码，从1开始
  /// [pageSize] 每页数量，默认20条
  Future<Map<String, dynamic>> getLoginLogs(
      {int page = 1, int pageSize = 20}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/account/login-logs', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    final logs = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeLoginLogPayload,
      listKeys: const ['logs'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'logs',
        items: logs,
      ),
    };
  }

  /// 获取隐私设置
  ///
  /// 返回当前用户的隐私设置配置。
  Future<Map<String, dynamic>> getPrivacySettings() async {
    final response = await get('/account/privacy');
    return toMap(response);
  }

  /// 更新隐私设置
  ///
  /// 仅接受白名单内的字段，非法key会被自动剔除。
  ///
  /// [settings] 隐私设置Map，只有白名单内的字段会被提交
  Future<Map<String, dynamic>> updatePrivacySettings(
      Map<String, dynamic> settings) async {
    settings = InputValidator.validateMapKeys(settings, _allowedPrivacyKeys);
    final response = await put('/account/privacy', data: settings);
    return toMap(response);
  }

  /// 获取黑名单列表
  ///
  /// [page] 页码，从1开始
  /// [pageSize] 每页数量，默认20条
  Future<Map<String, dynamic>> getBlockedUsers(
      {int page = 1, int pageSize = 20}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/account/blocked-users', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) {
      return _normalizePayload(toMap(response));
    }

    final blockedUsers = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeFriendPayload,
      listKeys: const ['blocked_users'],
    );

    return {
      ..._normalizePayload(toMap(response)),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'blocked_users',
        items: blockedUsers,
      ),
    };
  }

  /// 拉黑用户
  ///
  /// [userId] 要拉黑的用户ID
  Future<Map<String, dynamic>> blockUser(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/account/block/$userId');
    return toMap(response);
  }

  /// 取消拉黑
  ///
  /// [userId] 要取消拉黑的用户ID
  Future<Map<String, dynamic>> unblockUser(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await delete('/account/unblock/$userId');
    return toMap(response);
  }

  /// 导出数据
  ///
  /// 创建数据导出任务，返回任务ID用于查询导出状态。
  Future<Map<String, dynamic>> exportData() async {
    final response = await post('/account/export');
    return toMap(response);
  }

  /// 移除登录设备
  ///
  /// [sessionId] 会话ID
  Future<Map<String, dynamic>> removeDevice(String sessionId) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    final response = await delete('/account/devices/$sessionId');
    return toMap(response);
  }

  /// 获取安全事件
  ///
  /// 返回账号的安全事件记录。
  Future<Map<String, dynamic>> getSecurityEvents(
      {int page = 1, int pageSize = 20}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/account/security-events', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    final events = extractNormalizedList(
      response.data,
      itemNormalizer: _normalizeSecurityEventPayload,
      listKeys: const ['events'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'events',
        items: events,
      ),
    };
  }

  /// 获取数据导出状态
  ///
  /// [taskId] 导出任务ID
  Future<Map<String, dynamic>> getExportStatus(String taskId) async {
    InputValidator.validateUUID(taskId, '任务ID');
    final response = await get('/account/export/$taskId');
    return toMap(response);
  }

  /// 上传头像
  ///
  /// 支持jpg/jpeg/png/webp格式，最大5MB。
  ///
  /// [avatarData] 头像数据，可以是File、Uint8List或MultipartFile
  /// [filename] 文件名，用于格式校验
  /// [fileSize] 文件大小，用于大小校验
  Future<Map<String, dynamic>> uploadAvatar(dynamic avatarData,
      {String? filename, int? fileSize}) async {
    if (filename != null) {
      InputValidator.validateFileType(
          filename, const ['jpg', 'jpeg', 'png', 'webp']);
    }
    if (fileSize != null) {
      InputValidator.validateFileSize(fileSize, maxMB: 5);
    }
    final response = await post('/account/avatar', data: avatarData);
    return toMap(response);
  }

  /// 更新个人资料
  ///
  /// 仅接受白名单内的字段。昵称会做XSS过滤和长度校验（2-20字），
  /// 签名最长200字。
  ///
  /// [profile] 个人资料Map，只有白名单内的字段会被提交
  Future<Map<String, dynamic>> updateProfile(
      Map<String, dynamic> profile) async {
    profile = InputValidator.validateMapKeys(profile, _allowedProfileKeys);
    if (profile.containsKey('avatar') && !profile.containsKey('avatar_url')) {
      profile['avatar_url'] = profile['avatar'];
    }
    profile.remove('avatar');
    // 校验昵称长度
    if (profile.containsKey('nickname') && profile['nickname'] is String) {
      final sanitized =
          InputValidator.sanitizeText(profile['nickname'] as String);
      profile['nickname'] =
          InputValidator.requireLength(sanitized, '昵称', min: 2, max: 20);
    }
    // 校验个性签名长度
    if (profile.containsKey('bio') && profile['bio'] is String) {
      InputValidator.requireLength(profile['bio'] as String, '个性签名', max: 200);
      profile['bio'] = InputValidator.sanitizeText(profile['bio'] as String);
    }
    final response = await put('/account/profile', data: profile);
    return toMap(response);
  }

  /// 停用账号
  ///
  /// 临时停用账号，可以恢复。
  Future<Map<String, dynamic>> deactivateAccount() async {
    final response = await post('/account/deactivate');
    return toMap(response);
  }

  /// 永久删除账号
  ///
  /// 永久删除账号及所有数据，不可恢复。
  Future<Map<String, dynamic>> deleteAccountPermanently() async {
    final response = await post('/account/delete-permanent', data: {
      'confirmation': 'DELETE',
    });
    return toMap(response);
  }
}
