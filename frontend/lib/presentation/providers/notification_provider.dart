// 通知状态管理
//
// 管理未读计数、通知列表分页加载，并通过 WebSocket 监听
// 实时通知事件和纸船事件更新未读角标。
// 依赖 [NotificationService] 完成后端交互，依赖 [WebSocketManager] 接收推送。

import 'package:flutter/foundation.dart';
import '../../data/datasources/notification_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/payload_contract.dart';

/// 通知中心状态管理器
///
/// 维护未读计数和通知列表两部分状态。
/// 通过 WebSocket 监听 `new_notification` 和 `boat_update` 事件实时递增未读数。
/// 未读计数有节流保护，[refreshInterval] 秒内不会重复请求后端。
class NotificationProvider with ChangeNotifier {
  final NotificationService _notificationService = sl<NotificationService>();
  final WebSocketManager _wsManager = WebSocketManager();
  int _unreadCount = 0;
  bool _isLoading = false;

  /// 上次拉取未读数的时间，用于节流
  DateTime? _lastUpdate;
  bool _wsListenerRegistered = false;

  // 通知列表管理
  List<Map<String, dynamic>> _notifications = [];
  final Map<String, int> _notificationIndexById = {};
  bool _isLoadingNotifications = false;
  bool _hasMore = true;
  int _currentPage = 1;
  static const int _pageSize = 20;

  // WebSocket 监听器引用，dispose 时精确移除防止内存泄漏
  late final void Function(Map<String, dynamic>) _onNewNotification;
  late final void Function(Map<String, dynamic>) _onBoatUpdate;

  /// 未读数拉取的最小间隔（秒），防止频繁请求
  static const int refreshInterval = 30;

  int get unreadCount => _unreadCount;
  bool get isLoading => _isLoading;
  bool get hasUnread => _unreadCount > 0;
  List<Map<String, dynamic>> get notifications =>
      List.unmodifiable(_notifications);
  bool get isLoadingNotifications => _isLoadingNotifications;
  bool get hasMore => _hasMore;

  NotificationProvider() {
    _setupWebSocketListener();
  }

  /// 注册 WebSocket 事件监听器
  void _setupWebSocketListener() {
    if (_wsListenerRegistered) return;
    _onNewNotification = (data) {
      if (_upsertNotification(data, insertAtHead: true)) {
        _unreadCount++;
      }
      notifyListeners();
    };
    _onBoatUpdate = (data) {
      incrementUnread();
    };
    _wsManager.on('new_notification', _onNewNotification);
    _wsManager.on('boat_update', _onBoatUpdate);
    _wsListenerRegistered = true;
  }

  void _rebuildNotificationIndex() {
    _notificationIndexById.clear();
    for (var i = 0; i < _notifications.length; i++) {
      final notificationId = _notificationIdOf(_notifications[i]);
      if (notificationId != null) {
        _notificationIndexById[notificationId] = i;
      }
    }
  }

  String? _notificationIdOf(Map<String, dynamic> notification) {
    return extractNotificationEntityId(notification);
  }

  bool _upsertNotification(
    Map<String, dynamic> notification, {
    bool insertAtHead = false,
  }) {
    final normalized = normalizePayloadContract(notification);
    final notificationId = _notificationIdOf(normalized);
    if (notificationId != null) {
      normalized['notification_id'] = notificationId;
      normalized['id'] = notificationId;
    }

    final existingIndex =
        notificationId == null ? null : _notificationIndexById[notificationId];
    final wasUnread = existingIndex != null &&
        _notifications[existingIndex]['is_read'] != true;
    final shouldCountUnread = normalized['is_read'] != true;
    if (existingIndex != null) {
      _notifications[existingIndex] = {
        ..._notifications[existingIndex],
        ...normalized,
      };
    } else if (insertAtHead) {
      _notifications.insert(0, normalized);
    } else {
      _notifications.add(normalized);
    }

    _rebuildNotificationIndex();
    return shouldCountUnread && !wasUnread;
  }

  bool _markNotificationReadLocal(String notificationId) {
    final index = _notificationIndexById[notificationId];
    if (index == null) return false;
    if (_notifications[index]['is_read'] == true) return false;
    _notifications[index] = {
      ..._notifications[index],
      'is_read': true,
    };
    return true;
  }

  /// 释放资源，移除 WebSocket 监听器
  @override
  void dispose() {
    // 精确移除监听器，防止内存泄漏
    if (_wsListenerRegistered) {
      _wsManager.off('new_notification', _onNewNotification);
      _wsManager.off('boat_update', _onBoatUpdate);
    }
    super.dispose();
  }

  /// 加载未读通知数量（有节流保护，[refreshInterval] 秒内不重复请求）
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
      if (kDebugMode) {
        debugPrint('加载未读数量失败: $e');
      }
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  /// 标记单个通知为已读，同时同步本地列表状态
  ///
  /// 先乐观递减本地未读数，再以服务端返回的实际未读数为准。
  Future<void> markAsRead(String notificationId) async {
    try {
      final result = await _notificationService.markAsRead(notificationId);
      final changed = _markNotificationReadLocal(notificationId);
      if (result['unread_count'] != null) {
        _unreadCount = result['unread_count'] as int;
      } else if (changed && _unreadCount > 0) {
        _unreadCount--;
      }
      notifyListeners();
    } catch (e) {
      if (kDebugMode) {
        debugPrint('标记通知已读失败: $e');
      }
    }
  }

  /// 标记所有通知为已读，批量更新本地列表状态
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
      if (kDebugMode) {
        debugPrint('标记所有通知已读失败: $e');
      }
    }
  }

  /// 未读数 +1（WebSocket 推送时调用）
  void incrementUnread() {
    _unreadCount++;
    notifyListeners();
  }

  /// 未读数 -1
  void decrementUnread() {
    if (_unreadCount > 0) {
      _unreadCount--;
      notifyListeners();
    }
  }

  /// 直接设置未读数（服务端返回精确值时使用）
  void setUnreadCount(int count) {
    if (_unreadCount != count) {
      _unreadCount = count;
      notifyListeners();
    }
  }

  /// 强制刷新未读数（忽略节流间隔）
  Future<void> forceRefreshUnreadCount() async {
    _lastUpdate = null;
    await loadUnreadCount();
  }

  /// 加载通知列表（支持分页和下拉刷新）
  ///
  /// [refresh] 为 true 时重置分页状态从第一页开始加载。
  Future<void> loadNotifications({bool refresh = false}) async {
    if (_isLoadingNotifications) return;

    if (refresh) {
      _currentPage = 1;
      _notifications = [];
      _notificationIndexById.clear();
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
        for (final item in newItems) {
          _upsertNotification(item);
        }
        _hasMore = newItems.length >= _pageSize;
        _currentPage++;
        // 同步未读数
        if (result['unread_count'] != null) {
          _unreadCount = result['unread_count'] as int;
        }
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('加载通知列表失败: $e');
      }
    } finally {
      _isLoadingNotifications = false;
      notifyListeners();
    }
  }

  /// 清空所有通知状态（退出登录时调用）
  void clear() {
    _unreadCount = 0;
    _lastUpdate = null;
    _notifications = [];
    _notificationIndexById.clear();
    _currentPage = 1;
    _hasMore = true;
    _isLoadingNotifications = false;
    notifyListeners();
  }
}
