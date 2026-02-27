// 通知状态管理

import 'package:flutter/foundation.dart';
import '../../data/datasources/notification_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';

/// 通知状态管理Provider
class NotificationProvider with ChangeNotifier {
  final NotificationService _notificationService = sl<NotificationService>();
  final WebSocketManager _wsManager = WebSocketManager();
  int _unreadCount = 0;
  bool _isLoading = false;
  DateTime? _lastUpdate;
  bool _wsListenerRegistered = false;

  // 通知列表管理
  List<Map<String, dynamic>> _notifications = [];
  bool _isLoadingNotifications = false;
  bool _hasMore = true;
  int _currentPage = 1;
  static const int _pageSize = 20;

  // P1-1: 保存监听器引用以便精确移除
  late final void Function(Map<String, dynamic>) _onNewNotification;
  late final void Function(Map<String, dynamic>) _onBoatUpdate;

  static const int refreshInterval = 30;

  int get unreadCount => _unreadCount;
  bool get isLoading => _isLoading;
  bool get hasUnread => _unreadCount > 0;
  List<Map<String, dynamic>> get notifications => List.unmodifiable(_notifications);
  bool get isLoadingNotifications => _isLoadingNotifications;
  bool get hasMore => _hasMore;

  NotificationProvider() {
    _setupWebSocketListener();
  }

  void _setupWebSocketListener() {
    if (_wsListenerRegistered) return;
    // P1-1: 使用命名引用，dispose 时可精确移除
    _onNewNotification = (data) {
      incrementUnread();
      // 实时推送的通知插入列表头部
      _notifications.insert(0, data);
      notifyListeners();
    };
    _onBoatUpdate = (data) {
      incrementUnread();
    };
    _wsManager.on('new_notification', _onNewNotification);
    _wsManager.on('boat_update', _onBoatUpdate);
    _wsListenerRegistered = true;
  }

  @override
  void dispose() {
    // P1-1: 精确移除 WS 监听器，防止内存泄漏
    if (_wsListenerRegistered) {
      _wsManager.off('new_notification', _onNewNotification);
      _wsManager.off('boat_update', _onBoatUpdate);
    }
    super.dispose();
  }

  /// 加载未读通知数量
  Future<void> loadUnreadCount() async {
    if (_lastUpdate != null &&
        DateTime.now().difference(_lastUpdate!).inSeconds < refreshInterval) {
      return;
    }

    _isLoading = true;
    notifyListeners();

    try {
      final result = await _notificationService.getUnreadCount();
      if (result['success'] == true) {
        _unreadCount = result['unread_count'] ?? 0;
        _lastUpdate = DateTime.now();
      }
    } catch (e) {
      // P2-3: debugPrint 包裹在 kDebugMode 检查中
      if (kDebugMode) { debugPrint('加载未读数量失败: $e'); }
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  /// 标记单个通知为已读
  Future<void> markAsRead(String notificationId) async {
    try {
      final result = await _notificationService.markAsRead(notificationId);
      if (_unreadCount > 0) {
        _unreadCount--;
      }
      // 同步列表中的已读状态
      final idx = _notifications.indexWhere((n) => n['id']?.toString() == notificationId);
      if (idx >= 0) {
        _notifications[idx] = {..._notifications[idx], 'is_read': true};
      }
      // 服务端返回的未读数优先
      if (result['unread_count'] != null) {
        _unreadCount = result['unread_count'] as int;
      }
      notifyListeners();
    } catch (e) {
      if (kDebugMode) { debugPrint('标记通知已读失败: $e'); }
    }
  }

  /// 标记所有通知为已读
  Future<void> markAllAsRead() async {
    try {
      await _notificationService.markAllAsRead();
      _unreadCount = 0;
      // 同步列表中所有通知的已读状态
      for (int i = 0; i < _notifications.length; i++) {
        if (_notifications[i]['is_read'] != true) {
          _notifications[i] = {..._notifications[i], 'is_read': true};
        }
      }
      notifyListeners();
    } catch (e) {
      if (kDebugMode) { debugPrint('标记所有通知已读失败: $e'); }
    }
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

  Future<void> forceRefreshUnreadCount() async {
    _lastUpdate = null;
    await loadUnreadCount();
  }

  /// 加载通知列表（支持分页和刷新）
  Future<void> loadNotifications({bool refresh = false}) async {
    if (_isLoadingNotifications) return;

    if (refresh) {
      _currentPage = 1;
      _notifications = [];
      _hasMore = true;
    }

    if (!_hasMore) return;

    _isLoadingNotifications = true;
    notifyListeners();

    try {
      final result = await _notificationService.getNotifications(
        page: _currentPage,
        pageSize: _pageSize,
      );
      if (result['success'] == true) {
        final items = result['notifications'] as List? ?? [];
        final newItems = items.whereType<Map<String, dynamic>>().toList();
        _notifications.addAll(newItems);
        _hasMore = newItems.length >= _pageSize;
        _currentPage++;
        // 同步未读数
        if (result['unread_count'] != null) {
          _unreadCount = result['unread_count'] as int;
        }
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('加载通知列表失败: $e'); }
    } finally {
      _isLoadingNotifications = false;
      notifyListeners();
    }
  }

  void clear() {
    _unreadCount = 0;
    _lastUpdate = null;
    _notifications = [];
    _currentPage = 1;
    _hasMore = true;
    _isLoadingNotifications = false;
    notifyListeners();
  }
}
