// @file account_service.dart
// @brief 账号服务 - 账号管理功能
// Created by 王璐瑶

import 'base_service.dart';

class AccountService extends BaseService {
  @override
  String get serviceName => 'AccountService';

  /// 获取账号信息
  Future<Map<String, dynamic>> getAccountInfo() async {
    final response = await get('/account/info');
    return toMap(response);
  }

  /// 更新头像
  Future<Map<String, dynamic>> updateAvatar(String avatarUrl) async {
    final response = await post('/account/avatar', data: {
      'avatar_url': avatarUrl,
    });
    return toMap(response);
  }

  /// 更新资料
  Future<Map<String, dynamic>> updateProfile({
    String? nickname,
    String? bio,
    String? gender,
    String? birthday,
  }) async {
    final data = <String, dynamic>{};
    if (nickname != null) data['nickname'] = nickname;
    if (bio != null) data['bio'] = bio;
    if (gender != null) data['gender'] = gender;
    if (birthday != null) data['birthday'] = birthday;
    final response = await put('/account/profile', data: data);
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
  Future<Map<String, dynamic>> getLoginLogs({int page = 1, int pageSize = 20}) async {
    final response = await get('/account/login-logs', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 修改密码
  Future<Map<String, dynamic>> changePassword({
    required String oldPassword,
    required String newPassword,
  }) async {
    final response = await post('/account/change-password', data: {
      'old_password': oldPassword,
      'new_password': newPassword,
    });
    return toMap(response);
  }

  /// 绑定邮箱
  Future<Map<String, dynamic>> bindEmail(String email, String code) async {
    final response = await post('/account/bind-email', data: {
      'email': email,
      'verification_code': code,
    });
    return toMap(response);
  }

  /// 解绑邮箱
  Future<Map<String, dynamic>> unbindEmail(String code) async {
    final response = await post('/account/unbind-email', data: {
      'code': code,
    });
    return toMap(response);
  }

  /// 获取隐私设置
  Future<Map<String, dynamic>> getPrivacySettings() async {
    final response = await get('/account/privacy');
    return toMap(response);
  }

  /// 更新隐私设置
  Future<Map<String, dynamic>> updatePrivacySettings(Map<String, dynamic> settings) async {
    final response = await put('/account/privacy', data: settings);
    return toMap(response);
  }

  /// 获取黑名单
  Future<Map<String, dynamic>> getBlockedUsers({int page = 1, int pageSize = 20}) async {
    final response = await get('/account/blocked-users', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    return toMap(response);
  }

  /// 拉黑用户
  Future<Map<String, dynamic>> blockUser(String userId) async {
    final response = await post('/account/block/$userId');
    return toMap(response);
  }

  /// 取消拉黑
  Future<Map<String, dynamic>> unblockUser(String userId) async {
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
    final response = await get('/account/export/$taskId');
    return toMap(response);
  }

  /// 永久删除账号
  Future<Map<String, dynamic>> deleteAccountPermanently(String password) async {
    final response = await post('/account/delete-permanent', data: {
      'password': password,
    });
    return toMap(response);
  }

  /// 注销账号
  Future<Map<String, dynamic>> deactivateAccount(String password) async {
    final response = await post('/account/deactivate', data: {
      'password': password,
    });
    return toMap(response);
  }
}
