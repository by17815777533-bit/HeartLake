/// 好友状态管理
///
/// 统一管理正式好友、临时好友和好友请求三类列表的状态。
/// 通过 WebSocket 监听好友上下线、请求、接受、删除、临时好友过期等实时事件。
/// 依赖 [FriendService] 和 [TempFriendService] 完成后端交互。

import 'package:flutter/foundation.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/temp_friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';

/// 好友系统状态管理器
///
/// 维护三类列表：正式好友、临时好友（纸船互动产生的 24h 限时好友）、待处理好友请求。
/// 通过 WebSocket 监听 6 种实时事件自动同步列表，无需手动刷新。
class FriendProvider with ChangeNotifier {
  final FriendService _friendService = sl<FriendService>();
  final TempFriendService _tempFriendService = sl<TempFriendService>();
  final WebSocketManager _wsManager = WebSocketManager();

  List<Map<String, dynamic>> _friends = [];
  List<Map<String, dynamic>> _tempFriends = [];
  List<Map<String, dynamic>> _pendingRequests = [];
  bool _isLoading = false;
  bool _wsRegistered = false;

  // WebSocket 事件监听器引用，dispose 时需逐个移除
  late void Function(Map<String, dynamic>) _onFriendOnline;
  late void Function(Map<String, dynamic>) _onFriendOffline;
  late void Function(Map<String, dynamic>) _onFriendRequest;
  late void Function(Map<String, dynamic>) _onFriendAccepted;
  late void Function(Map<String, dynamic>) _onFriendRemoved;
  late void Function(Map<String, dynamic>) _onTempFriendExpired;

  List<Map<String, dynamic>> get friends => List.unmodifiable(_friends);
  List<Map<String, dynamic>> get tempFriends => List.unmodifiable(_tempFriends);
  List<Map<String, dynamic>> get pendingRequests => List.unmodifiable(_pendingRequests);
  bool get isLoading => _isLoading;
  int get pendingCount => _pendingRequests.length;
  int get friendCount => _friends.length;

  FriendProvider() {
    _setupWebSocket();
  }

  /// 注册 WebSocket 事件监听器，监听好友相关的 6 种实时事件
  void _setupWebSocket() {
    if (_wsRegistered) return;

    _onFriendOnline = (data) {
      final userId = data['user_id']?.toString();
      if (userId == null) return;
      final idx = _friends.indexWhere((f) => f['user_id'] == userId);
      if (idx >= 0) {
        _friends[idx] = {..._friends[idx], 'is_online': true};
        notifyListeners();
      }
    };

    _onFriendOffline = (data) {
      final userId = data['user_id']?.toString();
      if (userId == null) return;
      final idx = _friends.indexWhere((f) => f['user_id'] == userId);
      if (idx >= 0) {
        _friends[idx] = {..._friends[idx], 'is_online': false};
        notifyListeners();
      }
    };

    _onFriendRequest = (data) {
      _pendingRequests.insert(0, data);
      notifyListeners();
    };

    _onFriendAccepted = (data) {
      _pendingRequests.removeWhere((r) => r['user_id'] == data['user_id']);
      fetchFriends();
    };

    _onFriendRemoved = (data) {
      final userId = data['user_id']?.toString();
      _friends.removeWhere((f) => f['user_id'] == userId);
      notifyListeners();
    };

    _onTempFriendExpired = (data) {
      final id = data['temp_friend_id']?.toString();
      _tempFriends.removeWhere((f) => f['temp_friend_id'] == id);
      notifyListeners();
    };

    _wsManager.on('friend_online', _onFriendOnline);
    _wsManager.on('friend_offline', _onFriendOffline);
    _wsManager.on('friend_request', _onFriendRequest);
    _wsManager.on('friend_accepted', _onFriendAccepted);
    _wsManager.on('friend_removed', _onFriendRemoved);
    _wsManager.on('temp_friend_expired', _onTempFriendExpired);
    _wsRegistered = true;
  }

  /// 释放资源，移除所有 WebSocket 监听器
  @override
  void dispose() {
    if (_wsRegistered) {
      _wsManager.off('friend_online', _onFriendOnline);
      _wsManager.off('friend_offline', _onFriendOffline);
      _wsManager.off('friend_request', _onFriendRequest);
      _wsManager.off('friend_accepted', _onFriendAccepted);
      _wsManager.off('friend_removed', _onFriendRemoved);
      _wsManager.off('temp_friend_expired', _onTempFriendExpired);
    }
    super.dispose();
  }

  // ==================== 好友列表 ====================

  /// 从后端拉取正式好友列表
  Future<void> fetchFriends() async {
    _isLoading = true;
    notifyListeners();
    try {
      final result = await _friendService.getFriends();
      if (result['success'] == true) {
        _friends = List<Map<String, dynamic>>.from(result['friends'] ?? []);
      }
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 获取好友列表失败: $e');
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  // ==================== 临时好友 ====================

  /// 从后端拉取临时好友列表（纸船互动产生的 24h 限时好友）
  Future<void> fetchTempFriends() async {
    try {
      final result = await _tempFriendService.getMyTempFriends();
      if (result['success'] == true) {
        _tempFriends = List<Map<String, dynamic>>.from(result['temp_friends'] ?? []);
        notifyListeners();
      }
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 获取临时好友失败: $e');
    }
  }

  // ==================== 好友请求 ====================

  /// 拉取待处理的好友请求列表
  Future<void> fetchPendingRequests() async {
    try {
      final result = await _friendService.getPendingRequests();
      if (result['success'] == true) {
        _pendingRequests = List<Map<String, dynamic>>.from(result['requests'] ?? []);
        notifyListeners();
      }
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 获取好友请求失败: $e');
    }
  }

  /// 接受好友请求，成功后刷新好友列表
  Future<Map<String, dynamic>> acceptRequest(String userId) async {
    try {
      final result = await _friendService.acceptFriendRequest(userId);
      if (result['success'] == true) {
        _pendingRequests.removeWhere((r) => r['user_id'] == userId);
        await fetchFriends();
      }
      return result;
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 接受好友请求失败: $e');
      return {'success': false, 'message': '操作失败'};
    }
  }

  /// 拒绝好友请求
  Future<Map<String, dynamic>> rejectRequest(String userId) async {
    try {
      final result = await _friendService.rejectFriendRequest(userId);
      if (result['success'] == true) {
        _pendingRequests.removeWhere((r) => r['user_id'] == userId);
        notifyListeners();
      }
      return result;
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 拒绝好友请求失败: $e');
      return {'success': false, 'message': '操作失败'};
    }
  }

  /// 删除好友关系
  Future<Map<String, dynamic>> removeFriend(String friendId) async {
    try {
      final result = await _friendService.removeFriend(friendId);
      if (result['success'] == true) {
        _friends.removeWhere((f) => f['user_id'] == friendId);
        notifyListeners();
      }
      return result;
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 删除好友失败: $e');
      return {'success': false, 'message': '操作失败'};
    }
  }

  /// 向目标用户发送好友请求
  Future<Map<String, dynamic>> sendRequest(String userId, {String? message}) async {
    try {
      return await _friendService.sendFriendRequest(userId: userId, message: message);
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 发送好友请求失败: $e');
      return {'success': false, 'message': '操作失败'};
    }
  }

  // ==================== 临时好友操作 ====================

  /// 将临时好友升级为正式好友
  ///
  /// 升级成功后从临时好友列表移除，并刷新正式好友列表。
  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId) async {
    try {
      final result = await _tempFriendService.upgradeToPermanent(tempFriendId);
      if (result['success'] == true) {
        _tempFriends.removeWhere((f) => f['temp_friend_id'] == tempFriendId);
        await fetchFriends();
      }
      return result;
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 升级好友失败: $e');
      return {'success': false, 'message': '操作失败'};
    }
  }

  /// 清空所有好友相关状态（退出登录时调用）
  void clear() {
    _friends.clear();
    _tempFriends.clear();
    _pendingRequests.clear();
    notifyListeners();
  }
}
