// @file notification_service.dart
// @brief 通知服务层
// Created by 林子怡

import 'base_service.dart';

class NotificationService extends BaseService {
  @override
  String get serviceName => 'NotificationService';

  /// 获取通知列表
  Future<Map<String, dynamic>> getNotifications({int page = 1, int pageSize = 100}) async {
    final response = await get('/notifications', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    final data = response.data;
    // 后端返回纯数组 或 {notifications: [...], unread_count: N}
    if (data is List) {
      final unread = data.where((n) => n['is_read'] != true).length;
      return {
        'success': true,
        'notifications': data,
        'unread_count': unread,
      };
    }
    if (data is Map<String, dynamic>) {
      final items = data['notifications'] as List? ?? data['items'] as List? ?? [];
      final unreadCount = data['unread_count'] as int? ?? 0;
      return {
        'success': true,
        'notifications': items,
        'unread_count': unreadCount,
      };
    }
    return {'success': true, 'notifications': [], 'unread_count': 0};
  }

  /// 获取未读通知数量
  Future<Map<String, dynamic>> getUnreadCount() async {
    final response = await get('/notifications/unread-count');
    if (!response.success) return toMap(response);

    final data = response.data;
    int unreadCount = 0;
    if (data is Map) {
      unreadCount = data['unread_count'] ?? 0;
    }
    return {'success': true, 'unread_count': unreadCount};
  }

  /// 标记单条通知为已读
  Future<Map<String, dynamic>> markAsRead(String notificationId) async {
    final response = await post('/notifications/$notificationId/read');
    if (!response.success) return toMap(response);

    final data = response.data;
    final unreadCount = (data is Map) ? data['unread_count'] : null;
    return {'success': true, 'unread_count': unreadCount};
  }

  /// 标记所有通知为已读
  Future<Map<String, dynamic>> markAllAsRead() async {
    final response = await post('/notifications/read-all');
    return toMap(response);
  }
}
