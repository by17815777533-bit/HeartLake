import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/notification_service.dart';
import 'package:heart_lake/data/datasources/websocket_manager.dart';
import 'package:heart_lake/presentation/providers/notification_provider.dart';

class FakeWebSocketClient implements WebSocketClient {
  final Map<String, List<void Function(Map<String, dynamic>)>> _listeners = {};

  @override
  void on(String eventType, void Function(Map<String, dynamic>) listener) {
    _listeners.putIfAbsent(eventType, () => []).add(listener);
  }

  @override
  void off(String eventType, [void Function(Map<String, dynamic>)? listener]) {
    if (listener == null) {
      _listeners.remove(eventType);
      return;
    }
    _listeners[eventType]?.remove(listener);
  }

  void emit(String eventType, Map<String, dynamic> payload) {
    for (final listener in List<void Function(Map<String, dynamic>)>.from(
      _listeners[eventType] ?? const [],
    )) {
      listener(payload);
    }
  }

  int listenerCount(String eventType) => _listeners[eventType]?.length ?? 0;
}

class FakeNotificationDataSource implements NotificationDataSource {
  Map<String, dynamic> unreadCountResult = {'success': true, 'unread_count': 0};
  Map<String, dynamic> markAsReadResult = {'success': true};
  Map<String, dynamic> markAllAsReadResult = {'success': true};
  final List<Map<String, dynamic>> notificationResponses = [];
  final List<int> requestedPages = [];

  @override
  Future<Map<String, dynamic>> getNotifications({
    int page = 1,
    int pageSize = 20,
  }) async {
    requestedPages.add(page);
    return notificationResponses[page - 1];
  }

  @override
  Future<Map<String, dynamic>> getUnreadCount() async => unreadCountResult;

  @override
  Future<Map<String, dynamic>> markAllAsRead() async => markAllAsReadResult;

  @override
  Future<Map<String, dynamic>> markAsRead(String notificationId) async {
    return markAsReadResult;
  }
}

void main() {
  late FakeNotificationDataSource dataSource;
  late FakeWebSocketClient wsClient;
  late NotificationProvider provider;

  setUp(() {
    dataSource = FakeNotificationDataSource();
    wsClient = FakeWebSocketClient();
    provider = NotificationProvider(
      notificationService: dataSource,
      wsManager: wsClient,
    );
  });

  tearDown(() {
    provider.dispose();
  });

  group('NotificationProvider', () {
    test('registers and unregisters websocket listeners', () {
      final localProvider = NotificationProvider(
        notificationService: dataSource,
        wsManager: wsClient,
      );

      expect(wsClient.listenerCount('new_notification'), 2);

      localProvider.dispose();

      expect(wsClient.listenerCount('new_notification'), 1);
    });

    test('loads unread count and throttles repeated requests', () async {
      dataSource.unreadCountResult = {'success': true, 'unread_count': 7};

      await provider.loadUnreadCount();
      expect(provider.unreadCount, 7);

      dataSource.unreadCountResult = {'success': true, 'unread_count': 99};
      await provider.loadUnreadCount();
      expect(provider.unreadCount, 7);

      await provider.forceRefreshUnreadCount();
      expect(provider.unreadCount, 99);
    });

    test('loads notifications and infers hasMore from page size fallback',
        () async {
      dataSource.notificationResponses.add({
        'success': true,
        'notifications': List.generate(
          20,
          (index) => {
            'notification_id': 'n$index',
            'is_read': index == 0,
          },
        ),
        'unread_count': 19,
      });

      await provider.loadNotifications();

      expect(provider.notifications, hasLength(20));
      expect(provider.hasMore, true);
      expect(provider.unreadCount, 19);
      expect(dataSource.requestedPages, [1]);
    });

    test('supports refresh and list fallback payloads', () async {
      dataSource.notificationResponses.add({
        'success': true,
        'list': <Map<String, dynamic>>[
          {'notificationId': 'first', 'isRead': false},
        ],
        'unread_count': 1,
      });

      await provider.loadNotifications(refresh: true);

      expect(provider.notifications, hasLength(1));
      expect(provider.notifications.first['notification_id'], 'first');
      expect(provider.hasMore, false);
      expect(provider.unreadCount, 1);
    });

    test('marks a notification as read and uses server unread count', () async {
      dataSource.notificationResponses.add({
        'success': true,
        'notifications': [
          {'notification_id': 'n1', 'is_read': false},
        ],
        'unread_count': 4,
      });
      dataSource.markAsReadResult = {'success': true, 'unread_count': 3};

      await provider.loadNotifications();
      await provider.markAsRead('n1');

      expect(provider.unreadCount, 3);
      expect(provider.notifications.first['is_read'], true);
    });

    test('marks all notifications as read locally', () async {
      dataSource.notificationResponses.add({
        'success': true,
        'notifications': [
          {'notification_id': 'n1', 'is_read': false},
          {'notification_id': 'n2', 'is_read': false},
        ],
        'unread_count': 2,
      });

      await provider.loadNotifications();
      await provider.markAllAsRead();

      expect(provider.unreadCount, 0);
      expect(provider.notifications.every((item) => item['is_read'] == true),
          true);
    });

    test('normalizes realtime notification payloads to notification type', () {
      wsClient.emit('new_notification', {
        'type': 'new_notification',
        'notificationId': 'ws_1',
        'notification_type': 'boat',
        'isRead': false,
        'targetId': 'stone_1',
      });

      expect(provider.unreadCount, 1);
      expect(provider.notifications, hasLength(1));
      expect(provider.notifications.first['notification_id'], 'ws_1');
      expect(provider.notifications.first['type'], 'boat');
      expect(provider.notifications.first['target_id'], 'stone_1');
    });

    test('prefers authoritative unread count from websocket payload', () {
      wsClient.emit('new_notification', {
        'notification_id': 'ws_1',
        'notification_type': 'system_notice',
        'is_read': false,
        'unread_count': 6,
      });

      expect(provider.unreadCount, 6);
    });

    test('clear resets provider state', () async {
      dataSource.notificationResponses.add({
        'success': true,
        'notifications': [
          {'notification_id': 'n1', 'is_read': false},
        ],
        'unread_count': 1,
      });

      await provider.loadNotifications();
      provider.clear();

      expect(provider.unreadCount, 0);
      expect(provider.notifications, isEmpty);
      expect(provider.hasMore, true);
      expect(provider.isLoadingNotifications, false);
    });
  });
}
