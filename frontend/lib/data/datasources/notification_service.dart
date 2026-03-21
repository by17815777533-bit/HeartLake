import 'base_service.dart';
import '../../utils/input_validator.dart';
import '../../utils/payload_contract.dart';
import 'social_payload_normalizer.dart';

abstract class NotificationDataSource {
  Future<Map<String, dynamic>> getNotifications(
      {int page = 1, int pageSize = 20});
  Future<Map<String, dynamic>> getUnreadCount();
  Future<Map<String, dynamic>> markAsRead(String notificationId);
  Future<Map<String, dynamic>> markAllAsRead();
}

/// 通知服务，负责通知列表查询、未读计数和已读标记
///
/// 兼容后端多种返回格式：纯数组和带 notifications/items/list 键的对象。
class NotificationService extends BaseService
    implements NotificationDataSource {
  @override
  String get serviceName => 'NotificationService';

  List<Map<String, dynamic>> _normalizeNotifications(dynamic rawItems) {
    if (rawItems is! List) return const [];

    return rawItems.whereType<Map>().map((item) {
      final normalized = normalizePayloadContract(
        Map<String, dynamic>.from(item),
      );
      final notificationId = extractNotificationEntityId(normalized);
      if (notificationId != null) {
        normalized['notification_id'] = notificationId;
        normalized['id'] = notificationId;
      }

      final relatedId = normalized['related_id'] ??
          normalized['target_id'] ??
          normalized['stone_id'] ??
          normalized['friend_id'];
      if (relatedId != null) {
        normalized['related_id'] = relatedId;
        normalized['target_id'] = relatedId;
      }

      return normalized;
    }).toList();
  }

  /// 分页获取通知列表，同时返回未读数
  @override
  Future<Map<String, dynamic>> getNotifications(
      {int page = 1, int pageSize = 20}) async {
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
      final items = _normalizeNotifications(data);
      final unreadCount = extractUnreadCount(data, items: items);
      return {
        ...toMap(response),
        ...buildCollectionEnvelope(
          data,
          primaryKey: 'notifications',
          items: items,
          extra: {
            'unread_count': unreadCount,
            'unreadCount': unreadCount,
          },
        ),
      };
    }
    if (data is Map<String, dynamic>) {
      dynamic rawItems = data['notifications'] ??
          data['items'] ??
          data['list'] ??
          data['results'] ??
          data['data'];
      if (rawItems is Map<String, dynamic>) {
        rawItems = rawItems['notifications'] ??
            rawItems['items'] ??
            rawItems['list'] ??
            rawItems['results'] ??
            rawItems['data'];
      }

      final items = _normalizeNotifications(rawItems);
      final unreadCount = extractUnreadCount(data, items: items);
      return {
        ...toMap(response),
        ...buildCollectionEnvelope(
          data,
          primaryKey: 'notifications',
          items: items,
          extra: {
            'unread_count': unreadCount,
            'unreadCount': unreadCount,
          },
        ),
      };
    }
    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        const [],
        primaryKey: 'notifications',
        items: const <Map<String, dynamic>>[],
        extra: const {
          'unread_count': 0,
          'unreadCount': 0,
        },
      ),
    };
  }

  /// 获取未读通知数量
  @override
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
  @override
  Future<Map<String, dynamic>> markAsRead(String notificationId) async {
    InputValidator.validateUUID(notificationId, '通知ID');
    final response = await post('/notifications/$notificationId/read');
    if (!response.success) return toMap(response);

    final data = response.data;
    final normalized = data is Map
        ? normalizePayloadContract(Map<String, dynamic>.from(data))
        : data;
    final unreadCount = (normalized is Map) ? normalized['unread_count'] : null;
    return {...toMap(response), 'unread_count': unreadCount};
  }

  /// 标记所有通知为已读
  @override
  Future<Map<String, dynamic>> markAllAsRead() async {
    final response = await post('/notifications/read-all');
    return toMap(response);
  }
}
