// 账号管理服务
//
// 提供用户账号全生命周期管理能力，包括账号信息查询、隐私设置、
// 黑名单管理、登录设备审计、个人资料编辑和数据导出等功能。
//
// 所有写入操作均通过InputValidator做前置校验，确保只有白名单内的字段
// 才能提交到后端，防止非法字段注入和XSS攻击。

import '../../utils/input_validator.dart';
import '../../utils/app_config.dart';
import '../../utils/payload_contract.dart';
import 'base_service.dart';
import 'social_payload_normalizer.dart';

/// 账号管理服务
///
/// 提供账号信息查询、隐私设置、黑名单管理等功能。
class AccountService extends BaseService {
  @override
  String get serviceName => 'AccountService';

  bool _hasValue(dynamic value) {
    if (value == null) return false;
    if (value is String) return value.trim().isNotEmpty;
    return true;
  }

  String? _resolveMediaUrl(dynamic value) {
    final raw = value?.toString().trim();
    if (raw == null || raw.isEmpty) return null;

    final parsed = Uri.tryParse(raw);
    if (parsed != null && parsed.hasScheme) {
      return raw;
    }

    final apiBaseUri = Uri.tryParse(appConfig.apiBaseUrl);
    if (apiBaseUri == null || apiBaseUri.host.isEmpty) {
      return raw;
    }

    final origin = Uri(
      scheme: apiBaseUri.scheme,
      host: apiBaseUri.host,
      port: apiBaseUri.hasPort ? apiBaseUri.port : null,
    );
    final normalizedPath = raw.startsWith('/') ? raw : '/$raw';
    return origin.resolve(normalizedPath).toString();
  }

  String? _extractUploadedUrl(dynamic payload) {
    if (payload is! Map) return null;

    final source = Map<String, dynamic>.from(payload.cast<String, dynamic>());
    final data = source['data'] is Map
        ? Map<String, dynamic>.from(
            (source['data'] as Map).cast<String, dynamic>(),
          )
        : source;

    return _resolveMediaUrl(
      data['url'] ??
          data['file_url'] ??
          data['avatar_url'] ??
          data['media_url'] ??
          data['relative_url'] ??
          data['path'],
    );
  }

  Map<String, dynamic> _normalizeDataMap(Map<String, dynamic> payload) {
    final data = normalizePayloadContract(payload);
    final avatar = data['avatar'];
    if ((data['avatar_url'] == null || data['avatar_url'].toString().isEmpty) &&
        avatar != null) {
      data['avatar_url'] = avatar;
    }
    final resolvedAvatarUrl = _resolveMediaUrl(data['avatar_url']);
    if (resolvedAvatarUrl != null) {
      data['avatar_url'] = resolvedAvatarUrl;
      data['avatarUrl'] = resolvedAvatarUrl;
    }
    final blockedUsers = data['blocked_users'];
    if (data['items'] == null && blockedUsers is List) {
      data['items'] = blockedUsers;
    }
    return data;
  }

  Map<String, dynamic> _normalizePayload(Map<String, dynamic> payload) {
    final normalized = Map<String, dynamic>.from(payload);
    final rawData = normalized['data'];
    if (rawData is Map) {
      normalized['data'] = _normalizeDataMap(
        Map<String, dynamic>.from(rawData.cast<String, dynamic>()),
      );
    }
    return normalized;
  }

  String _resolveServiceError(
    ServiceResponse<dynamic> response,
    String action,
  ) {
    final message = response.message?.trim();
    if (message != null && message.isNotEmpty) {
      return message;
    }
    return '$action失败';
  }

  void _ensureSuccess(ServiceResponse<dynamic> response, String action) {
    if (!response.success) {
      throw StateError(_resolveServiceError(response, action));
    }
  }

  dynamic _requireResponseData(
      ServiceResponse<dynamic> response, String action) {
    _ensureSuccess(response, action);
    if (response.data == null) {
      throw StateError('$action响应缺少 data');
    }
    return response.data;
  }

  Map<String, dynamic> _requireResponseDataMap(
    ServiceResponse<dynamic> response,
    String action,
  ) {
    final rawData = _requireResponseData(response, action);
    if (rawData is! Map) {
      throw StateError('$action响应 data 不是对象');
    }
    return _normalizeDataMap(
      Map<String, dynamic>.from(rawData.cast<String, dynamic>()),
    );
  }

  dynamic _requireCollectionSource(
    ServiceResponse<dynamic> response,
    String action,
  ) {
    final rawData = _requireResponseData(response, action);
    if (rawData is Map || rawData is List) {
      return rawData;
    }
    throw StateError('$action响应 data 不是有效集合');
  }

  Map<String, dynamic> _normalizeSuccessResponse(
    ServiceResponse<dynamic> response,
    String action, {
    bool requireDataMap = false,
  }) {
    _ensureSuccess(response, action);
    if (requireDataMap) {
      return {
        ...toMap(response),
        'data': _requireResponseDataMap(response, action),
      };
    }
    if (response.data is Map) {
      return {
        ...toMap(response),
        'data': _normalizeDataMap(
          Map<String, dynamic>.from(
              (response.data as Map).cast<String, dynamic>()),
        ),
      };
    }
    return toMap(response);
  }

  Map<String, dynamic> _normalizeResponseWithRequiredData(
    ServiceResponse<dynamic> response,
    String action, {
    required List<String> requiredKeys,
    Map<String, dynamic> Function(Map<String, dynamic>)? dataNormalizer,
  }) {
    _ensureSuccess(response, action);
    var data = _requireResponseDataMap(response, action);
    if (dataNormalizer != null) {
      data = dataNormalizer(data);
    }
    for (final key in requiredKeys) {
      if (!_hasValue(data[key])) {
        throw StateError('$action响应缺少 $key');
      }
    }
    return {
      ...toMap(response),
      'data': data,
    };
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

  Map<String, dynamic> _normalizeExportTaskPayload(Map raw) {
    final item = Map<String, dynamic>.from(raw.cast<String, dynamic>());
    final taskId = item['task_id'] ?? item['taskId'] ?? item['id'];
    if (taskId != null) {
      item['task_id'] = taskId.toString();
      item['taskId'] = taskId.toString();
      item['id'] = taskId.toString();
    }

    final createdAt = item['created_at'] ?? item['createdAt'];
    if (createdAt != null) {
      item['created_at'] = createdAt;
      item['createdAt'] = createdAt;
    }

    final completedAt = item['completed_at'] ?? item['completedAt'];
    if (completedAt != null) {
      item['completed_at'] = completedAt;
      item['completedAt'] = completedAt;
    }

    final downloadUrl = item['download_url'] ?? item['downloadUrl'];
    if (downloadUrl != null) {
      item['download_url'] = downloadUrl;
      item['downloadUrl'] = downloadUrl;
    }

    return item;
  }

  /// 隐私设置白名单，仅保留当前后端真实支持的字段。
  static const _allowedPrivacyKeys = [
    'profile_visibility',
    'show_online_status',
    'allow_message_from_stranger',
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
    return _normalizeSuccessResponse(response, '获取账号信息', requireDataMap: true);
  }

  /// 获取账号统计
  ///
  /// 返回账号的统计数据，如发布石头数、好友数等。
  Future<Map<String, dynamic>> getAccountStats() async {
    final response = await get('/account/stats');
    return _normalizeSuccessResponse(response, '获取账号统计', requireDataMap: true);
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
    final rawData = _requireCollectionSource(response, '获取登录设备列表');

    final devices = extractNormalizedList(
      rawData,
      itemNormalizer: _normalizeDevicePayload,
      listKeys: const ['devices'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        rawData,
        primaryKey: 'devices',
        items: devices,
        requireExplicitTotal: true,
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
    final rawData = _requireCollectionSource(response, '获取登录日志');

    final logs = extractNormalizedList(
      rawData,
      itemNormalizer: _normalizeLoginLogPayload,
      listKeys: const ['logs'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        rawData,
        primaryKey: 'logs',
        items: logs,
        requireExplicitTotal: true,
      ),
    };
  }

  /// 获取隐私设置
  ///
  /// 返回当前用户的隐私设置配置。
  Future<Map<String, dynamic>> getPrivacySettings() async {
    final response = await get('/account/privacy');
    return _normalizeSuccessResponse(response, '获取隐私设置', requireDataMap: true);
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
    return _normalizeSuccessResponse(
      response,
      '更新隐私设置',
      requireDataMap: true,
    );
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
    final rawData = _requireCollectionSource(response, '获取黑名单列表');

    final blockedUsers = extractNormalizedList(
      rawData,
      itemNormalizer: normalizeFriendPayload,
      listKeys: const ['blocked_users'],
    );

    return {
      ..._normalizePayload(toMap(response)),
      ...buildCollectionEnvelope(
        rawData,
        primaryKey: 'blocked_users',
        items: blockedUsers,
        requireExplicitTotal: true,
      ),
    };
  }

  /// 拉黑用户
  ///
  /// [userId] 要拉黑的用户ID
  Future<Map<String, dynamic>> blockUser(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/account/block/$userId');
    return _normalizeResponseWithRequiredData(
      response,
      '拉黑用户',
      requiredKeys: const ['blocked_user_id', 'status'],
    );
  }

  /// 取消拉黑
  ///
  /// [userId] 要取消拉黑的用户ID
  Future<Map<String, dynamic>> unblockUser(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await delete('/account/unblock/$userId');
    return _normalizeResponseWithRequiredData(
      response,
      '取消拉黑用户',
      requiredKeys: const ['blocked_user_id', 'status'],
    );
  }

  /// 导出数据
  ///
  /// 创建数据导出任务，返回任务ID用于查询导出状态。
  Future<Map<String, dynamic>> exportData() async {
    final response = await post('/account/export');
    final payload = _normalizeExportTaskPayload(
      _requireResponseDataMap(response, '创建数据导出任务'),
    );
    if (!_hasValue(payload['task_id']) || !_hasValue(payload['status'])) {
      throw StateError('创建数据导出任务响应缺少 task_id 或 status');
    }
    return {
      ...toMap(response),
      'data': payload,
      'task_id': payload['task_id'],
      'taskId': payload['taskId'],
    };
  }

  /// 移除登录设备
  ///
  /// [sessionId] 会话ID
  Future<Map<String, dynamic>> removeDevice(String sessionId) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    final response = await delete('/account/devices/$sessionId');
    return _normalizeResponseWithRequiredData(
      response,
      '移除登录设备',
      requiredKeys: const ['session_id', 'status'],
    );
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
    final rawData = _requireCollectionSource(response, '获取安全事件');

    final events = extractNormalizedList(
      rawData,
      itemNormalizer: _normalizeSecurityEventPayload,
      listKeys: const ['events'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        rawData,
        primaryKey: 'events',
        items: events,
        requireExplicitTotal: true,
      ),
    };
  }

  /// 获取数据导出状态
  ///
  /// [taskId] 导出任务ID
  Future<Map<String, dynamic>> getExportStatus(String taskId) async {
    InputValidator.validateUUID(taskId, '任务ID');
    final response = await get('/account/export/$taskId');
    final payload = _normalizeExportTaskPayload(
      _requireResponseDataMap(response, '获取导出任务状态'),
    );
    if (!_hasValue(payload['task_id']) || !_hasValue(payload['status'])) {
      throw StateError('获取导出任务状态响应缺少 task_id 或 status');
    }
    return {
      ...toMap(response),
      'data': payload,
      'task_id': payload['task_id'],
      'taskId': payload['taskId'],
      'status': payload['status'],
      'download_url': payload['download_url'],
      'downloadUrl': payload['downloadUrl'],
    };
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
    if (avatarData is String) {
      final response = await post('/account/avatar', data: {
        'avatar_url': avatarData,
      });
      return _normalizeResponseWithRequiredData(
        response,
        '更新头像',
        requiredKeys: const ['avatar_url'],
      );
    }

    final uploadResponse = await client.uploadFile('/media/upload',
        file: avatarData, filename: filename);
    final uploadPayload = uploadResponse.data;
    final avatarUrl = _extractUploadedUrl(uploadPayload);
    final success = uploadResponse.statusCode == 200 &&
        uploadPayload is Map &&
        uploadPayload['code'] == 0 &&
        avatarUrl != null &&
        avatarUrl.isNotEmpty;
    if (!success) {
      final message = uploadPayload is Map
          ? uploadPayload['message']?.toString().trim() ?? '头像上传失败'
          : '头像上传失败';
      throw StateError(message);
    }

    final response = await post('/account/avatar', data: {
      'avatar_url': avatarUrl,
    });
    return _normalizeResponseWithRequiredData(
      response,
      '更新头像',
      requiredKeys: const ['avatar_url'],
    );
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
    return _normalizeSuccessResponse(
      response,
      '更新个人资料',
      requireDataMap: true,
    );
  }

  /// 停用账号
  ///
  /// 临时停用账号，可以恢复。
  Future<Map<String, dynamic>> deactivateAccount() async {
    final response = await post('/account/deactivate', data: {
      'confirmation': 'DEACTIVATE',
    });
    return _normalizeResponseWithRequiredData(
      response,
      '停用账号',
      requiredKeys: const ['status', 'recovery_window_days'],
    );
  }

  /// 永久删除账号
  ///
  /// 永久删除账号及所有数据，不可恢复。
  Future<Map<String, dynamic>> deleteAccountPermanently() async {
    final response = await post('/account/delete-permanent', data: {
      'confirmation': 'DELETE',
    });
    return _normalizeResponseWithRequiredData(
      response,
      '永久删除账号',
      requiredKeys: const ['status', 'deleted'],
    );
  }
}
