import 'base_service.dart';
import '../../utils/input_validator.dart';

/// 通知服务，负责通知列表查询、未读计数和已读标记
///
/// 兼容后端两种返回格式：纯数组和带 notifications/items 键的对象。
class NotificationService extends BaseService {
  @override
  String get serviceName => 'NotificationService';

  /// 分页获取通知列表，同时返回未读数
  Future<Map<String, dynamic>> getNotifications({int page = 1, int pageSize = 20}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/notifications', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });
    if (!response.success) return toMap(response);

    final data = response.data;
    // 兼容后端纯数组和对象两种返回格式
    if (data is List) {
      final unread = data.where((n) => n['is_read'] != true).length;
      return {
        'success': true,
        'notifications': data,
        'unread_count': unread,
      };
    }
    if (data is Map<String, dynamic>) {
      final rawNotifications = data['notifications'];
      final rawItems = data['items'];
      final items = rawNotifications is List ? rawNotifications : (rawItems is List ? rawItems : []);
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
    return {...toMap(response), 'unread_count': unreadCount};
  }

  /// 标记单条通知为已读
  Future<Map<String, dynamic>> markAsRead(String notificationId) async {
    InputValidator.validateUUID(notificationId, '通知ID');
    final response = await post('/notifications/$notificationId/read');
    if (!response.success) return toMap(response);

    final data = response.data;
    final unreadCount = (data is Map) ? data['unread_count'] : null;
    return {...toMap(response), 'unread_count': unreadCount};
  }

  /// 标记所有通知为已读
  Future<Map<String, dynamic>> markAllAsRead() async {
    final response = await post('/notifications/read-all');
    return toMap(response);
  }
}
