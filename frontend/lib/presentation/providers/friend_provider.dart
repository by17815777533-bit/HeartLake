// @file friend_provider.dart
// @brief 好友状态管理 - 好友列表、临时好友、好友请求
// Created by 林子怡

import 'package:flutter/foundation.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/temp_friend_service.dart';
import '../../data/datasources/websocket_manager.dart';

/// 好友状态管理Provider
class FriendProvider with ChangeNotifier {
  final FriendService _friendService = FriendService();
  final TempFriendService _tempFriendService = TempFriendService();
  final WebSocketManager _wsManager = WebSocketManager();

  List<Map<String, dynamic>> _friends = [];
  List<Map<String, dynamic>> _tempFriends = [];
  List<Map<String, dynamic>> _pendingRequests = [];
  bool _isLoading = false;
  bool _wsRegistered = false;

  void Function(Map<String, dynamic>)? _onFriendOnline;
  void Function(Map<String, dynamic>)? _onFriendOffline;
  void Function(Map<String, dynamic>)? _onFriendRequest;
  void Function(Map<String, dynamic>)? _onFriendAccepted;
  void Function(Map<String, dynamic>)? _onFriendRemoved;
  void Function(Map<String, dynamic>)? _onTempFriendExpired;

  List<Map<String, dynamic>> get friends => List.unmodifiable(_friends);
  List<Map<String, dynamic>> get tempFriends => List.unmodifiable(_tempFriends);
  List<Map<String, dynamic>> get pendingRequests => List.unmodifiable(_pendingRequests);
  bool get isLoading => _isLoading;
  int get pendingCount => _pendingRequests.length;
  int get friendCount => _friends.length;

  FriendProvider() {
    _setupWebSocket();
  }

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

  Future<Map<String, dynamic>> sendRequest(String userId, {String? message}) async {
    try {
      return await _friendService.sendFriendRequest(userId: userId, message: message);
    } catch (e) {
      if (kDebugMode) debugPrint('[FriendProvider] 发送好友请求失败: $e');
      return {'success': false, 'message': '操作失败'};
    }
  }

  // ==================== 临时好友操作 ====================

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

  void clear() {
    _friends.clear();
    _tempFriends.clear();
    _pendingRequests.clear();
    notifyListeners();
  }
}
