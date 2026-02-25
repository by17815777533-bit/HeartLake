import 'package:flutter/foundation.dart';
import 'package:flutter_test/flutter_test.dart';

/// FriendProvider 依赖 FriendService/TempFriendService/WebSocketManager 单例。
/// 提取核心状态管理逻辑进行测试。

class FriendState with ChangeNotifier {
  List<Map<String, dynamic>> _friends = [];
  List<Map<String, dynamic>> _tempFriends = [];
  List<Map<String, dynamic>> _pendingRequests = [];
  bool _isLoading = false;

  List<Map<String, dynamic>> get friends => List.unmodifiable(_friends);
  List<Map<String, dynamic>> get tempFriends => List.unmodifiable(_tempFriends);
  List<Map<String, dynamic>> get pendingRequests => List.unmodifiable(_pendingRequests);
  bool get isLoading => _isLoading;
  int get pendingCount => _pendingRequests.length;
  int get friendCount => _friends.length;

  Future<void> fetchFriends(Future<Map<String, dynamic>> Function() fetchFn) async {
    _isLoading = true;
    notifyListeners();
    try {
      final result = await fetchFn();
      if (result['success'] == true) {
        _friends = List<Map<String, dynamic>>.from(result['friends'] ?? []);
      }
    } catch (_) {
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  Future<void> fetchTempFriends(Future<Map<String, dynamic>> Function() fetchFn) async {
    try {
      final result = await fetchFn();
      if (result['success'] == true) {
        _tempFriends = List<Map<String, dynamic>>.from(result['temp_friends'] ?? []);
      }
    } catch (_) {}
    notifyListeners();
  }

  Future<void> fetchPendingRequests(Future<Map<String, dynamic>> Function() fetchFn) async {
    try {
      final result = await fetchFn();
      if (result['success'] == true) {
        _pendingRequests = List<Map<String, dynamic>>.from(result['requests'] ?? []);
      }
    } catch (_) {}
    notifyListeners();
  }

  Future<Map<String, dynamic>> acceptRequest(
    String userId,
    Future<Map<String, dynamic>> Function(String) acceptFn,
  ) async {
    try {
      final result = await acceptFn(userId);
      if (result['success'] == true) {
        _pendingRequests.removeWhere((r) => r['user_id'] == userId);
        notifyListeners();
      }
      return result;
    } catch (e) {
      return {'success': false, 'message': '操作失败'};
    }
  }

  Future<Map<String, dynamic>> rejectRequest(
    String userId,
    Future<Map<String, dynamic>> Function(String) rejectFn,
  ) async {
    try {
      final result = await rejectFn(userId);
      if (result['success'] == true) {
        _pendingRequests.removeWhere((r) => r['user_id'] == userId);
        notifyListeners();
      }
      return result;
    } catch (e) {
      return {'success': false, 'message': '操作失败'};
    }
  }

  Future<Map<String, dynamic>> removeFriend(
    String friendId,
    Future<Map<String, dynamic>> Function(String) removeFn,
  ) async {
    try {
      final result = await removeFn(friendId);
      if (result['success'] == true) {
        _friends.removeWhere((f) => f['user_id'] == friendId);
        notifyListeners();
      }
      return result;
    } catch (e) {
      return {'success': false, 'message': '操作失败'};
    }
  }

  Future<Map<String, dynamic>> sendRequest(
    String userId,
    Future<Map<String, dynamic>> Function(String) sendFn,
  ) async {
    try {
      return await sendFn(userId);
    } catch (e) {
      return {'success': false, 'message': '操作失败'};
    }
  }

  Future<Map<String, dynamic>> upgradeToPermanent(
    String tempFriendId,
    Future<Map<String, dynamic>> Function(String) upgradeFn,
    Future<void> Function() refreshFn,
  ) async {
    try {
      final result = await upgradeFn(tempFriendId);
      if (result['success'] == true) {
        _tempFriends.removeWhere((f) => f['temp_friend_id'] == tempFriendId);
        await refreshFn();
      }
      return result;
    } catch (e) {
      return {'success': false, 'message': '操作失败'};
    }
  }

  void handleFriendOnline(Map<String, dynamic> data) {
    final userId = data['user_id']?.toString();
    if (userId == null) return;
    final idx = _friends.indexWhere((f) => f['user_id'] == userId);
    if (idx >= 0) {
      _friends[idx] = {..._friends[idx], 'is_online': true};
      notifyListeners();
    }
  }

  void handleFriendOffline(Map<String, dynamic> data) {
    final userId = data['user_id']?.toString();
    if (userId == null) return;
    final idx = _friends.indexWhere((f) => f['user_id'] == userId);
    if (idx >= 0) {
      _friends[idx] = {..._friends[idx], 'is_online': false};
      notifyListeners();
    }
  }

  void handleFriendRequest(Map<String, dynamic> data) {
    _pendingRequests.insert(0, data);
    notifyListeners();
  }

  void handleFriendAccepted(Map<String, dynamic> data) {
    _pendingRequests.removeWhere((r) => r['user_id'] == data['user_id']);
    notifyListeners();
  }

  void handleFriendRemoved(Map<String, dynamic> data) {
    final userId = data['user_id']?.toString();
    _friends.removeWhere((f) => f['user_id'] == userId);
    notifyListeners();
  }

  void handleTempFriendExpired(Map<String, dynamic> data) {
    final id = data['temp_friend_id']?.toString();
    _tempFriends.removeWhere((f) => f['temp_friend_id'] == id);
    notifyListeners();
  }

  void clear() {
    _friends.clear();
    _tempFriends.clear();
    _pendingRequests.clear();
    notifyListeners();
  }
}

void main() {
  late FriendState state;

  setUp(() {
    state = FriendState();
  });

  group('Initial state', () {
    test('should have empty friends', () => expect(state.friends, isEmpty));
    test('should have empty temp friends', () => expect(state.tempFriends, isEmpty));
    test('should have empty pending requests', () => expect(state.pendingRequests, isEmpty));
    test('should not be loading', () => expect(state.isLoading, false));
    test('should have zero pending count', () => expect(state.pendingCount, 0));
    test('should have zero friend count', () => expect(state.friendCount, 0));
  });

  group('fetchFriends', () {
    test('should load friends successfully', () async {
      await state.fetchFriends(() async => {
        'success': true,
        'friends': [
          {'user_id': 'u1', 'nickname': '小明'},
          {'user_id': 'u2', 'nickname': '小红'},
        ],
      });
      expect(state.friends.length, 2);
      expect(state.friendCount, 2);
      expect(state.isLoading, false);
    });

    test('should handle empty friends', () async {
      await state.fetchFriends(() async => {'success': true, 'friends': []});
      expect(state.friends, isEmpty);
    });

    test('should handle failure silently', () async {
      await state.fetchFriends(() async => {'success': false});
      expect(state.friends, isEmpty);
    });

    test('should handle exception silently', () async {
      await state.fetchFriends(() async => throw Exception('网络错误'));
      expect(state.friends, isEmpty);
      expect(state.isLoading, false);
    });

    test('should handle null friends key', () async {
      await state.fetchFriends(() async => {'success': true});
      expect(state.friends, isEmpty);
    });

    test('should replace previous friends', () async {
      await state.fetchFriends(() async => {
        'success': true,
        'friends': [{'user_id': 'u1'}],
      });
      expect(state.friendCount, 1);

      await state.fetchFriends(() async => {
        'success': true,
        'friends': [{'user_id': 'u2'}, {'user_id': 'u3'}],
      });
      expect(state.friendCount, 2);
      expect(state.friends[0]['user_id'], 'u2');
    });

    test('should notify listeners during loading', () async {
      var notifyCount = 0;
      state.addListener(() => notifyCount++);
      await state.fetchFriends(() async => {'success': true, 'friends': []});
      expect(notifyCount, 2); // loading=true, loading=false
    });
  });

  group('fetchTempFriends', () {
    test('should load temp friends', () async {
      await state.fetchTempFriends(() async => {
        'success': true,
        'temp_friends': [
          {'temp_friend_id': 't1', 'user_id': 'u1'},
        ],
      });
      expect(state.tempFriends.length, 1);
    });

    test('should handle failure', () async {
      await state.fetchTempFriends(() async => {'success': false});
      expect(state.tempFriends, isEmpty);
    });

    test('should handle exception', () async {
      await state.fetchTempFriends(() async => throw Exception('err'));
      expect(state.tempFriends, isEmpty);
    });
  });

  group('fetchPendingRequests', () {
    test('should load pending requests', () async {
      await state.fetchPendingRequests(() async => {
        'success': true,
        'requests': [
          {'user_id': 'u1', 'message': '你好'},
          {'user_id': 'u2', 'message': '加个好友'},
        ],
      });
      expect(state.pendingRequests.length, 2);
      expect(state.pendingCount, 2);
    });

    test('should handle failure', () async {
      await state.fetchPendingRequests(() async => {'success': false});
      expect(state.pendingRequests, isEmpty);
    });
  });

  group('acceptRequest', () {
    setUp(() async {
      await state.fetchPendingRequests(() async => {
        'success': true,
        'requests': [
          {'user_id': 'u1'},
          {'user_id': 'u2'},
        ],
      });
    });

    test('should remove from pending on success', () async {
      final result = await state.acceptRequest('u1', (id) async => {'success': true});
      expect(result['success'], true);
      expect(state.pendingCount, 1);
      expect(state.pendingRequests.every((r) => r['user_id'] != 'u1'), true);
    });

    test('should not remove on failure', () async {
      await state.acceptRequest('u1', (id) async => {'success': false});
      expect(state.pendingCount, 2);
    });

    test('should handle exception', () async {
      final result = await state.acceptRequest('u1', (id) async => throw Exception('err'));
      expect(result['success'], false);
      expect(result['message'], '操作失败');
    });
  });

  group('rejectRequest', () {
    setUp(() async {
      await state.fetchPendingRequests(() async => {
        'success': true,
        'requests': [{'user_id': 'u1'}, {'user_id': 'u2'}],
      });
    });

    test('should remove from pending on success', () async {
      final result = await state.rejectRequest('u2', (id) async => {'success': true});
      expect(result['success'], true);
      expect(state.pendingCount, 1);
    });

    test('should handle exception', () async {
      final result = await state.rejectRequest('u1', (id) async => throw Exception('err'));
      expect(result['success'], false);
    });
  });

  group('removeFriend', () {
    setUp(() async {
      await state.fetchFriends(() async => {
        'success': true,
        'friends': [
          {'user_id': 'u1', 'nickname': '小明'},
          {'user_id': 'u2', 'nickname': '小红'},
        ],
      });
    });

    test('should remove friend on success', () async {
      final result = await state.removeFriend('u1', (id) async => {'success': true});
      expect(result['success'], true);
      expect(state.friendCount, 1);
      expect(state.friends[0]['user_id'], 'u2');
    });

    test('should not remove on failure', () async {
      await state.removeFriend('u1', (id) async => {'success': false});
      expect(state.friendCount, 2);
    });

    test('should handle exception', () async {
      final result = await state.removeFriend('u1', (id) async => throw Exception('err'));
      expect(result['success'], false);
    });
  });

  group('sendRequest', () {
    test('should return result from service', () async {
      final result = await state.sendRequest('u1', (id) async => {'success': true, 'request_id': 'r1'});
      expect(result['success'], true);
      expect(result['request_id'], 'r1');
    });

    test('should handle exception', () async {
      final result = await state.sendRequest('u1', (id) async => throw Exception('err'));
      expect(result['success'], false);
    });
  });

  group('upgradeToPermanent', () {
    setUp(() async {
      await state.fetchTempFriends(() async => {
        'success': true,
        'temp_friends': [
          {'temp_friend_id': 't1', 'user_id': 'u1'},
          {'temp_friend_id': 't2', 'user_id': 'u2'},
        ],
      });
    });

    test('should remove temp friend and refresh on success', () async {
      var refreshCalled = false;
      final result = await state.upgradeToPermanent(
        't1',
        (id) async => {'success': true},
        () async { refreshCalled = true; },
      );
      expect(result['success'], true);
      expect(state.tempFriends.length, 1);
      expect(refreshCalled, true);
    });

    test('should not remove on failure', () async {
      await state.upgradeToPermanent(
        't1',
        (id) async => {'success': false},
        () async {},
      );
      expect(state.tempFriends.length, 2);
    });

    test('should handle exception', () async {
      final result = await state.upgradeToPermanent(
        't1',
        (id) async => throw Exception('err'),
        () async {},
      );
      expect(result['success'], false);
    });
  });

  // ==================== WebSocket 事件处理 ====================
  group('handleFriendOnline', () {
    setUp(() async {
      await state.fetchFriends(() async => {
        'success': true,
        'friends': [
          {'user_id': 'u1', 'is_online': false},
          {'user_id': 'u2', 'is_online': false},
        ],
      });
    });

    test('should set friend online', () {
      state.handleFriendOnline({'user_id': 'u1'});
      expect(state.friends.firstWhere((f) => f['user_id'] == 'u1')['is_online'], true);
      expect(state.friends.firstWhere((f) => f['user_id'] == 'u2')['is_online'], false);
    });

    test('should ignore unknown user', () {
      var notified = false;
      state.addListener(() => notified = true);
      state.handleFriendOnline({'user_id': 'u999'});
      expect(notified, false);
    });

    test('should ignore null user_id', () {
      var notified = false;
      state.addListener(() => notified = true);
      state.handleFriendOnline({});
      expect(notified, false);
    });
  });

  group('handleFriendOffline', () {
    setUp(() async {
      await state.fetchFriends(() async => {
        'success': true,
        'friends': [
          {'user_id': 'u1', 'is_online': true},
        ],
      });
    });

    test('should set friend offline', () {
      state.handleFriendOffline({'user_id': 'u1'});
      expect(state.friends[0]['is_online'], false);
    });

    test('should ignore unknown user', () {
      var notified = false;
      state.addListener(() => notified = true);
      state.handleFriendOffline({'user_id': 'u999'});
      expect(notified, false);
    });
  });

  group('handleFriendRequest', () {
    test('should add to pending requests', () {
      state.handleFriendRequest({'user_id': 'u1', 'message': '你好'});
      expect(state.pendingCount, 1);
      expect(state.pendingRequests[0]['user_id'], 'u1');
    });

    test('should insert at beginning', () {
      state.handleFriendRequest({'user_id': 'u1'});
      state.handleFriendRequest({'user_id': 'u2'});
      expect(state.pendingRequests[0]['user_id'], 'u2');
    });
  });

  group('handleFriendAccepted', () {
    setUp(() {
      state.handleFriendRequest({'user_id': 'u1'});
      state.handleFriendRequest({'user_id': 'u2'});
    });

    test('should remove from pending', () {
      state.handleFriendAccepted({'user_id': 'u1'});
      expect(state.pendingCount, 1);
    });

    test('should ignore unknown user', () {
      state.handleFriendAccepted({'user_id': 'u999'});
      expect(state.pendingCount, 2);
    });
  });

  group('handleFriendRemoved', () {
    setUp(() async {
      await state.fetchFriends(() async => {
        'success': true,
        'friends': [{'user_id': 'u1'}, {'user_id': 'u2'}],
      });
    });

    test('should remove friend', () {
      state.handleFriendRemoved({'user_id': 'u1'});
      expect(state.friendCount, 1);
    });

    test('should ignore null user_id', () {
      state.handleFriendRemoved({});
      expect(state.friendCount, 2);
    });
  });

  group('handleTempFriendExpired', () {
    setUp(() async {
      await state.fetchTempFriends(() async => {
        'success': true,
        'temp_friends': [
          {'temp_friend_id': 't1'},
          {'temp_friend_id': 't2'},
        ],
      });
    });

    test('should remove expired temp friend', () {
      state.handleTempFriendExpired({'temp_friend_id': 't1'});
      expect(state.tempFriends.length, 1);
    });

    test('should ignore unknown id', () {
      state.handleTempFriendExpired({'temp_friend_id': 't999'});
      expect(state.tempFriends.length, 2);
    });
  });

  group('clear', () {
    test('should clear all state', () async {
      await state.fetchFriends(() async => {
        'success': true,
        'friends': [{'user_id': 'u1'}],
      });
      state.handleFriendRequest({'user_id': 'u2'});
      await state.fetchTempFriends(() async => {
        'success': true,
        'temp_friends': [{'temp_friend_id': 't1'}],
      });

      state.clear();

      expect(state.friends, isEmpty);
      expect(state.tempFriends, isEmpty);
      expect(state.pendingRequests, isEmpty);
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
      var c1 = 0, c2 = 0;
      state.addListener(() => c1++);
      state.addListener(() => c2++);
      state.handleFriendRequest({'user_id': 'u1'});
      expect(c1, 1);
      expect(c2, 1);
    });

    test('removed listener should not be notified', () {
      var count = 0;
      void listener() => count++;
      state.addListener(listener);
      state.handleFriendRequest({'user_id': 'u1'});
      expect(count, 1);
      state.removeListener(listener);
      state.handleFriendRequest({'user_id': 'u2'});
      expect(count, 1);
    });
  });
}
