// @file notification_provider.dart
// @brief 通知状态管理
// Created by 林子怡

import 'package:flutter/foundation.dart';
import '../../data/datasources/notification_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../utils/storage_util.dart';

/// 通知状态管理Provider
class NotificationProvider with ChangeNotifier {
  final NotificationService _notificationService = NotificationService();
  final WebSocketManager _wsManager = WebSocketManager();
  int _unreadCount = 0;
  bool _isLoading = false;
  DateTime? _lastUpdate;
  bool _wsListenerRegistered = false;

  // P1-1: 保存监听器引用以便精确移除
  late final void Function(Map<String, dynamic>) _onNewNotification;
  late final void Function(Map<String, dynamic>) _onBoatUpdate;

  static const int refreshInterval = 30;

  int get unreadCount => _unreadCount;
  bool get isLoading => _isLoading;
  bool get hasUnread => _unreadCount > 0;

  NotificationProvider() {
    _setupWebSocketListener();
  }

  void _setupWebSocketListener() {
    if (_wsListenerRegistered) return;
    // P1-1: 使用命名引用，dispose 时可精确移除
    _onNewNotification = (data) {
      incrementUnread();
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
      final token = await StorageUtil.getToken();
      if (token != null) {
        final result = await _notificationService.getUnreadCount();
        if (result['success'] == true) {
          _unreadCount = result['unread_count'] ?? 0;
          _lastUpdate = DateTime.now();
        }
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
      final token = await StorageUtil.getToken();
      if (token != null) {
        await _notificationService.markAsRead(notificationId);
        if (_unreadCount > 0) {
          _unreadCount--;
          notifyListeners();
        }
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('标记通知已读失败: $e'); }
    }
  }

  /// 标记所有通知为已读
  Future<void> markAllAsRead() async {
    try {
      final token = await StorageUtil.getToken();
      if (token != null) {
        await _notificationService.markAllAsRead();
        _unreadCount = 0;
        notifyListeners();
      }
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

  void clear() {
    _unreadCount = 0;
    _lastUpdate = null;
    notifyListeners();
  }
}
