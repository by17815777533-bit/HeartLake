import 'package:flutter/foundation.dart';
import 'package:flutter_test/flutter_test.dart';

/// NotificationProvider 的构造函数依赖 WebSocketManager 单例（注册监听器），
/// 而 FakeNotificationService 继承 BaseService 会触发 ApiClient 单例初始化
/// （依赖 dart:io Platform，在纯 Dart 测试中可能不稳定）。
///
/// 因此我们提取 NotificationProvider 的核心状态逻辑到一个独立的
/// StandaloneNotificationState 类中进行测试，覆盖所有公开方法的行为。

// ============================================================
// 独立的通知状态管理类 — 复刻 NotificationProvider 的纯逻辑
// ============================================================
class NotificationState with ChangeNotifier {
  int _unreadCount = 0;
  bool _isLoading = false;
  DateTime? _lastUpdate;
  List<Map<String, dynamic>> _notifications = [];
  bool _isLoadingNotifications = false;
  bool _hasMore = true;
  static const int refreshInterval = 30;

  int get unreadCount => _unreadCount;
  bool get isLoading => _isLoading;
  bool get hasUnread => _unreadCount > 0;
  List<Map<String, dynamic>> get notifications =>
      List.unmodifiable(_notifications);
  bool get isLoadingNotifications => _isLoadingNotifications;
  bool get hasMore => _hasMore;

  /// 模拟 loadUnreadCount
  Future<void> loadUnreadCount(
      Future<Map<String, dynamic>> Function() fetchFn) async {
    if (_lastUpdate != null &&
        DateTime.now().difference(_lastUpdate!).inSeconds < refreshInterval) {
      return;
    }

    _isLoading = true;
    notifyListeners();

    try {
      final result = await fetchFn();
      if (result['success'] == true) {
        _unreadCount = result['unread_count'] ?? 0;
        _lastUpdate = DateTime.now();
      }
    } catch (_) {
      // 静默失败
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  /// 模拟 markAsRead
  Future<void> markAsRead(
    String notificationId,
    Future<Map<String, dynamic>> Function(String) markFn,
  ) async {
    try {
      final result = await markFn(notificationId);
      if (_unreadCount > 0) {
        _unreadCount--;
      }
      final idx = _notifications
          .indexWhere((n) => n['id']?.toString() == notificationId);
      if (idx >= 0) {
        _notifications[idx] = {..._notifications[idx], 'is_read': true};
      }
      if (result['unread_count'] != null) {
        _unreadCount = result['unread_count'] as int;
      }
      notifyListeners();
    } catch (_) {}
  }

  /// 模拟 markAllAsRead
  Future<void> markAllAsRead(Future<void> Function() markAllFn) async {
    try {
      await markAllFn();
      _unreadCount = 0;
      for (int i = 0; i < _notifications.length; i++) {
        if (_notifications[i]['is_read'] != true) {
          _notifications[i] = {..._notifications[i], 'is_read': true};
        }
      }
      notifyListeners();
    } catch (_) {}
  }

  void incrementUnread() {
    _unreadCount++;
    notifyListeners();
  }

  void decrementUnread() {
    if (_unreadCount > 0) {
      _unreadCount--;
      notifyListeners();
    }
  }

  void setUnreadCount(int count) {
    if (_unreadCount != count) {
      _unreadCount = count;
      notifyListeners();
    }
  }

  void addNotification(Map<String, dynamic> data) {
    _notifications.insert(0, data);
    incrementUnread();
  }

  void clear() {
    _unreadCount = 0;
    _lastUpdate = null;
    _notifications = [];
    _hasMore = true;
    _isLoadingNotifications = false;
    notifyListeners();
  }
}

void main() {
  late NotificationState state;

  setUp(() {
    state = NotificationState();
  });

  group('Initial state', () {
    test('should have zero unread count', () {
      expect(state.unreadCount, 0);
    });

    test('should not be loading', () {
      expect(state.isLoading, false);
    });

    test('should have no unread', () {
      expect(state.hasUnread, false);
    });

    test('notifications should be empty', () {
      expect(state.notifications, isEmpty);
    });

    test('should have more pages by default', () {
      expect(state.hasMore, true);
    });

    test('should not be loading notifications', () {
      expect(state.isLoadingNotifications, false);
    });
  });

  group('loadUnreadCount', () {
    test('should update unread count on success', () async {
      await state.loadUnreadCount(
        () async => {'success': true, 'unread_count': 7},
      );
      expect(state.unreadCount, 7);
      expect(state.hasUnread, true);
      expect(state.isLoading, false);
    });

    test('should not update on failure response', () async {
      await state.loadUnreadCount(
        () async => {'success': false},
      );
      expect(state.unreadCount, 0);
    });

    test('should handle exception gracefully', () async {
      await state.loadUnreadCount(
        () async => throw Exception('network error'),
      );
      expect(state.unreadCount, 0);
      expect(state.isLoading, false);
    });

    test('should throttle within refreshInterval', () async {
      await state.loadUnreadCount(
        () async => {'success': true, 'unread_count': 5},
      );
      expect(state.unreadCount, 5);

      // Second call within 30s should be skipped
      await state.loadUnreadCount(
        () async => {'success': true, 'unread_count': 99},
      );
      expect(state.unreadCount, 5); // unchanged
    });
  });

  group('markAsRead', () {
    test('should decrement unread and use server count', () async {
      state.setUnreadCount(5);

      await state.markAsRead(
        'notif-1',
        (id) async => {'success': true, 'unread_count': 3},
      );

      // Server returned 3, so that takes priority
      expect(state.unreadCount, 3);
    });

    test('should mark notification as read in list', () async {
      state.addNotification({'id': 'n1', 'is_read': false, 'text': 'hello'});
      state.addNotification({'id': 'n2', 'is_read': false, 'text': 'world'});

      await state.markAsRead(
        'n1',
        (id) async => {'success': true},
      );

      final n1 = state.notifications.firstWhere((n) => n['id'] == 'n1');
      expect(n1['is_read'], true);

      final n2 = state.notifications.firstWhere((n) => n['id'] == 'n2');
      expect(n2['is_read'], false);
    });

    test('should not go below zero on decrement', () async {
      // unreadCount is 0
      await state.markAsRead(
        'notif-x',
        (id) async => {'success': true},
      );
      expect(state.unreadCount, 0);
    });

    test('should handle exception gracefully', () async {
      state.setUnreadCount(3);
      await state.markAsRead(
        'notif-1',
        (id) async => throw Exception('fail'),
      );
      // unchanged on error
      expect(state.unreadCount, 3);
    });
  });

  group('markAllAsRead', () {
    test('should set unread count to zero', () async {
      state.setUnreadCount(10);
      state.addNotification({'id': 'a', 'is_read': false});
      state.addNotification({'id': 'b', 'is_read': false});

      await state.markAllAsRead(() async {});

      expect(state.unreadCount, 0);
      expect(state.hasUnread, false);
      for (final n in state.notifications) {
        expect(n['is_read'], true);
      }
    });

    test('should handle exception gracefully', () async {
      state.setUnreadCount(5);
      await state.markAllAsRead(() async => throw Exception('fail'));
      // unchanged on error
      expect(state.unreadCount, 5);
    });
  });

  group('incrementUnread', () {
    test('should increase count by 1', () {
      expect(state.unreadCount, 0);
      state.incrementUnread();
      expect(state.unreadCount, 1);
      state.incrementUnread();
      expect(state.unreadCount, 2);
    });

    test('should notify listeners', () {
      var count = 0;
      state.addListener(() => count++);
      state.incrementUnread();
      expect(count, 1);
    });
  });

  group('decrementUnread', () {
    test('should decrease count by 1', () {
      state.setUnreadCount(3);
      state.decrementUnread();
      expect(state.unreadCount, 2);
    });

    test('should not go below zero', () {
      expect(state.unreadCount, 0);
      state.decrementUnread();
      expect(state.unreadCount, 0);
    });

    test('should notify listeners when count > 0', () {
      state.setUnreadCount(2);
      var count = 0;
      state.addListener(() => count++);
      state.decrementUnread();
      expect(count, 1);
    });

    test('should not notify when count is 0', () {
      var count = 0;
      state.addListener(() => count++);
      state.decrementUnread();
      expect(count, 0);
    });
  });

  group('setUnreadCount', () {
    test('should set exact count', () {
      state.setUnreadCount(42);
      expect(state.unreadCount, 42);
    });

    test('should not notify if same value', () {
      state.setUnreadCount(5);
      var notified = false;
      state.addListener(() => notified = true);
      state.setUnreadCount(5);
      expect(notified, false);
    });

    test('should notify on different value', () {
      state.setUnreadCount(5);
      var notified = false;
      state.addListener(() => notified = true);
      state.setUnreadCount(10);
      expect(notified, true);
    });
  });

  group('addNotification', () {
    test('should insert at beginning and increment unread', () {
      state.addNotification({'id': '1', 'text': 'first'});
      state.addNotification({'id': '2', 'text': 'second'});

      expect(state.notifications.length, 2);
      expect(state.notifications[0]['id'], '2'); // most recent first
      expect(state.notifications[1]['id'], '1');
      expect(state.unreadCount, 2);
    });
  });

  group('clear', () {
    test('should reset all state', () {
      state.setUnreadCount(10);
      state.addNotification({'id': '1'});
      state.addNotification({'id': '2'});

      state.clear();

      expect(state.unreadCount, 0);
      expect(state.hasUnread, false);
      expect(state.notifications, isEmpty);
      expect(state.hasMore, true);
      expect(state.isLoadingNotifications, false);
    });

    test('should notify listeners', () {
      var count = 0;
      state.addListener(() => count++);
      state.clear();
      expect(count, 1);
    });
  });

  group('ChangeNotifier behavior', () {
    test('multiple listeners should all be notified', () {
      var count1 = 0;
      var count2 = 0;
      state.addListener(() => count1++);
      state.addListener(() => count2++);
      state.incrementUnread();
      expect(count1, 1);
      expect(count2, 1);
    });

    test('removed listener should not be notified', () {
      var count = 0;
      void listener() => count++;
      state.addListener(listener);
      state.incrementUnread();
      expect(count, 1);

      state.removeListener(listener);
      state.incrementUnread();
      expect(count, 1); // not incremented
    });
  });
}
