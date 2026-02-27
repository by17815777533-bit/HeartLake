// 账号服务 - 账号管理功能

import '../../utils/input_validator.dart';
import 'base_service.dart';

class AccountService extends BaseService {
  @override
  String get serviceName => 'AccountService';

  // 隐私设置允许的 key
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

  // 个人资料允许的 key
  static const _allowedProfileKeys = [
    'nickname',
    'bio',
    'avatar',
    'gender',
    'birthday',
    'location',
  ];

  /// 获取账号信息
  Future<Map<String, dynamic>> getAccountInfo() async {
    final response = await get('/account/info');
    return toMap(response);
  }

  /// 获取账号统计
  Future<Map<String, dynamic>> getAccountStats() async {
    final response = await get('/account/stats');
    return toMap(response);
  }

  /// 获取登录设备列表
  Future<Map<String, dynamic>> getDevices() async {
    final response = await get('/account/devices');
    return toMap(response);
  }

  /// 获取登录日志
  Future<Map<String, dynamic>> getLoginLogs(
      {int page = 1, int pageSize = 20}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/account/login-logs', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 获取隐私设置
  Future<Map<String, dynamic>> getPrivacySettings() async {
    final response = await get('/account/privacy');
    return toMap(response);
  }

  /// 更新隐私设置
  Future<Map<String, dynamic>> updatePrivacySettings(
      Map<String, dynamic> settings) async {
    settings = InputValidator.validateMapKeys(settings, _allowedPrivacyKeys);
    final response = await put('/account/privacy', data: settings);
    return toMap(response);
  }

  /// 获取黑名单
  Future<Map<String, dynamic>> getBlockedUsers(
      {int page = 1, int pageSize = 20}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/account/blocked-users', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 拉黑用户
  Future<Map<String, dynamic>> blockUser(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await post('/account/block/$userId');
    return toMap(response);
  }

  /// 取消拉黑
  Future<Map<String, dynamic>> unblockUser(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await delete('/account/unblock/$userId');
    return toMap(response);
  }

  /// 导出数据
  Future<Map<String, dynamic>> exportData() async {
    final response = await post('/account/export');
    return toMap(response);
  }

  /// 移除登录设备
  Future<Map<String, dynamic>> removeDevice(String sessionId) async {
    InputValidator.validateUUID(sessionId, '会话ID');
    final response = await delete('/account/devices/$sessionId');
    return toMap(response);
  }

  /// 获取安全事件
  Future<Map<String, dynamic>> getSecurityEvents() async {
    final response = await get('/account/security-events');
    return toMap(response);
  }

  /// 获取数据导出状态
  Future<Map<String, dynamic>> getExportStatus(String taskId) async {
    InputValidator.validateUUID(taskId, '任务ID');
    final response = await get('/account/export/$taskId');
    return toMap(response);
  }

  /// 上传头像
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
  Future<Map<String, dynamic>> updateProfile(
      Map<String, dynamic> profile) async {
    profile = InputValidator.validateMapKeys(profile, _allowedProfileKeys);
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
  Future<Map<String, dynamic>> deactivateAccount() async {
    final response = await post('/account/deactivate');
    return toMap(response);
  }

  /// 永久删除账号
  Future<Map<String, dynamic>> deleteAccountPermanently() async {
    final response = await post('/account/delete-permanent');
    return toMap(response);
  }
}
