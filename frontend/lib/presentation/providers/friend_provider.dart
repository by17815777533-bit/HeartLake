// 好友状态管理
//
// 仅维护当前仍在使用的两类关系：正式好友和临时好友。
// 手动好友申请已下线，不再保留待处理请求的本地影子状态。

import 'dart:async';

import 'package:flutter/foundation.dart';

import '../../data/datasources/friend_service.dart';
import '../../data/datasources/social_payload_normalizer.dart';
import '../../data/datasources/temp_friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';

class FriendProvider with ChangeNotifier {
  final FriendDataSource _friendService;
  final TempFriendDataSource _tempFriendService;
  final WebSocketClient _wsManager;

  List<Map<String, dynamic>> _friends = [];
  List<Map<String, dynamic>> _tempFriends = [];
  final Map<String, int> _friendIndexByUserId = {};
  final Map<String, int> _tempFriendIndexById = {};
  bool _isLoading = false;
  bool _wsRegistered = false;

  late void Function(Map<String, dynamic>) _onFriendOnline;
  late void Function(Map<String, dynamic>) _onFriendOffline;
  late void Function(Map<String, dynamic>) _onFriendAccepted;
  late void Function(Map<String, dynamic>) _onFriendRemoved;
  late void Function(Map<String, dynamic>) _onTempFriendCreated;
  late void Function(Map<String, dynamic>) _onTempFriendExpired;

  List<Map<String, dynamic>> get friends => List.unmodifiable(_friends);
  List<Map<String, dynamic>> get tempFriends => List.unmodifiable(_tempFriends);
  bool get isLoading => _isLoading;
  int get friendCount => _friends.length;

  FriendProvider({
    FriendDataSource? friendService,
    TempFriendDataSource? tempFriendService,
    WebSocketClient? wsManager,
  })  : _friendService = friendService ?? sl<FriendService>(),
        _tempFriendService = tempFriendService ?? sl<TempFriendService>(),
        _wsManager = wsManager ?? WebSocketManager() {
    _setupWebSocket();
  }

  String? _friendUserId(Map<String, dynamic> item) {
    final candidate = item['user_id'] ??
        item['friend_id'] ??
        item['friend_user_id'] ??
        item['userId'] ??
        item['friendId'] ??
        item['friendUserId'] ??
        item['peer_id'] ??
        item['from_user_id'] ??
        item['to_user_id'] ??
        item['target_user_id'];
    final userId = candidate?.toString();
    if (userId == null || userId.isEmpty) {
      return null;
    }
    return userId;
  }

  String? _tempFriendId(Map<String, dynamic> item) =>
      item['temp_friend_id']?.toString();

  void _rebuildFriendIndex() {
    _friendIndexByUserId.clear();
    for (var i = 0; i < _friends.length; i++) {
      final userId = _friendUserId(_friends[i]);
      if (userId != null) {
        _friendIndexByUserId[userId] = i;
      }
    }
  }

  void _rebuildTempFriendIndex() {
    _tempFriendIndexById.clear();
    for (var i = 0; i < _tempFriends.length; i++) {
      final tempFriendId = _tempFriendId(_tempFriends[i]);
      if (tempFriendId != null && tempFriendId.isNotEmpty) {
        _tempFriendIndexById[tempFriendId] = i;
      }
    }
  }

  bool _removeFriendByUserId(String userId) {
    final index = _friendIndexByUserId[userId];
    if (index == null) return false;
    _friends.removeAt(index);
    _rebuildFriendIndex();
    return true;
  }

  bool _removeTempFriendById(String tempFriendId) {
    final index = _tempFriendIndexById[tempFriendId];
    if (index == null) return false;
    _tempFriends.removeAt(index);
    _rebuildTempFriendIndex();
    return true;
  }

  void _setupWebSocket() {
    if (_wsRegistered) return;

    _onFriendOnline = (data) {
      final userId = _friendUserId(data);
      if (userId == null) return;
      final index = _friendIndexByUserId[userId];
      if (index == null) return;
      _friends[index] = {..._friends[index], 'is_online': true};
      notifyListeners();
    };

    _onFriendOffline = (data) {
      final userId = _friendUserId(data);
      if (userId == null) return;
      final index = _friendIndexByUserId[userId];
      if (index == null) return;
      _friends[index] = {..._friends[index], 'is_online': false};
      notifyListeners();
    };

    _onFriendAccepted = (_) {
      unawaited(fetchFriends());
    };

    _onFriendRemoved = (data) {
      final userId = _friendUserId(data);
      if (userId == null) return;
      if (_removeFriendByUserId(userId)) {
        notifyListeners();
      }
    };

    _onTempFriendCreated = (_) {
      unawaited(fetchTempFriends());
    };

    _onTempFriendExpired = (data) {
      final id =
          data['temp_friend_id']?.toString() ?? data['tempFriendId']?.toString();
      if (id == null) return;
      if (_removeTempFriendById(id)) {
        notifyListeners();
      } else {
        unawaited(fetchTempFriends());
      }
    };

    _wsManager.on('friend_online', _onFriendOnline);
    _wsManager.on('friend_offline', _onFriendOffline);
    _wsManager.on('friend_accepted', _onFriendAccepted);
    _wsManager.on('friend_removed', _onFriendRemoved);
    _wsManager.on('temp_friend_created', _onTempFriendCreated);
    _wsManager.on('temp_friend_expired', _onTempFriendExpired);
    _wsRegistered = true;
  }

  @override
  void dispose() {
    if (_wsRegistered) {
      _wsManager.off('friend_online', _onFriendOnline);
      _wsManager.off('friend_offline', _onFriendOffline);
      _wsManager.off('friend_accepted', _onFriendAccepted);
      _wsManager.off('friend_removed', _onFriendRemoved);
      _wsManager.off('temp_friend_created', _onTempFriendCreated);
      _wsManager.off('temp_friend_expired', _onTempFriendExpired);
    }
    super.dispose();
  }

  Future<Map<String, dynamic>> fetchFriends() async {
    _isLoading = true;
    notifyListeners();
    try {
      final result = await _friendService.getFriends();
      if (result['success'] == true) {
        _friends = extractNormalizedList(
          result,
          itemNormalizer: normalizeFriendPayload,
          listKeys: const ['friends'],
        );
        _rebuildFriendIndex();
      }
      return result;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[FriendProvider] 获取好友列表失败: $e');
      }
      return {'success': false, 'message': '获取好友列表失败'};
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  Future<Map<String, dynamic>> fetchTempFriends() async {
    try {
      final result = await _tempFriendService.getMyTempFriends();
      if (result['success'] == true) {
        _tempFriends = extractNormalizedList(
          result,
          itemNormalizer: (item) => Map<String, dynamic>.from(item),
          listKeys: const ['temp_friends', 'friends'],
        );
        _rebuildTempFriendIndex();
        notifyListeners();
      }
      return result;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[FriendProvider] 获取临时好友失败: $e');
      }
      return {'success': false, 'message': '获取临时好友失败'};
    }
  }

  Future<Map<String, dynamic>> removeFriend(String friendId) async {
    try {
      final result = await _friendService.removeFriend(friendId);
      if (result['success'] == true) {
        _removeFriendByUserId(friendId);
        notifyListeners();
      }
      return result;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[FriendProvider] 删除好友失败: $e');
      }
      return {'success': false, 'message': '操作失败'};
    }
  }

  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId) async {
    try {
      final result = await _tempFriendService.upgradeToPermanent(tempFriendId);
      if (result['success'] == true) {
        _removeTempFriendById(tempFriendId);
        await fetchFriends();
      }
      return result;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[FriendProvider] 升级好友失败: $e');
      }
      return {'success': false, 'message': '操作失败'};
    }
  }

  Future<Map<String, dynamic>> deleteTempFriend(String tempFriendId) async {
    try {
      final result = await _tempFriendService.deleteTempFriend(tempFriendId);
      if (result['success'] == true) {
        _removeTempFriendById(tempFriendId);
        notifyListeners();
      }
      return result;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[FriendProvider] 删除临时好友失败: $e');
      }
      return {'success': false, 'message': '操作失败'};
    }
  }

  void clear() {
    _friends.clear();
    _tempFriends.clear();
    _friendIndexByUserId.clear();
    _tempFriendIndexById.clear();
    notifyListeners();
  }
}
