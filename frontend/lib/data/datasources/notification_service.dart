import 'base_service.dart';
import '../../utils/input_validator.dart';
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

  Map<String, dynamic> _applyHasMoreFallback(
    Map<String, dynamic> envelope, {
    required dynamic raw,
    required int pageSize,
    required int itemCount,
  }) {
    bool? explicitHasMore;
    if (raw is Map<String, dynamic>) {
      explicitHasMore = raw['has_more'] as bool? ??
          raw['hasMore'] as bool? ??
          (raw['pagination'] is Map
              ? (raw['pagination']['has_more'] as bool? ??
                  raw['pagination']['hasMore'] as bool?)
              : null) ??
          (raw['data'] is Map
              ? (raw['data']['has_more'] as bool? ??
                  raw['data']['hasMore'] as bool?)
              : null);
    }

    final hasMore = explicitHasMore ?? itemCount >= pageSize;
    final pagination = Map<String, dynamic>.from(
      envelope['pagination'] as Map<String, dynamic>? ?? const {},
    )..['has_more'] = hasMore;

    return {
      ...envelope,
      'has_more': hasMore,
      'hasMore': hasMore,
      'pagination': pagination,
    };
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

    final items = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeNotificationPayload,
      listKeys: const ['notifications'],
    );
    final unreadCount = extractUnreadCount(response.data, items: items);
    final envelope = {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'notifications',
        items: items,
        extra: {
          'unread_count': unreadCount,
          'unreadCount': unreadCount,
        },
      ),
    };
    return _applyHasMoreFallback(
      envelope,
      raw: response.data,
      pageSize: pageSize,
      itemCount: items.length,
    );
  }

  /// 获取未读通知数量
  @override
  Future<Map<String, dynamic>> getUnreadCount() async {
    final response = await get('/notifications/unread-count');
    if (!response.success) return toMap(response);

    final data = response.data;
    final unreadCount = extractUnreadCount(data);
    return {
      ...toMap(response),
      'unread_count': unreadCount,
      'unreadCount': unreadCount,
    };
  }

  /// 标记单条通知为已读
  @override
  Future<Map<String, dynamic>> markAsRead(String notificationId) async {
    InputValidator.validateUUID(notificationId, '通知ID');
    final response = await post('/notifications/$notificationId/read');
    if (!response.success) return toMap(response);

    final data = response.data;
    final normalized = data is Map
        ? normalizeNotificationPayload(
            Map<String, dynamic>.from(data.cast<String, dynamic>()),
          )
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
