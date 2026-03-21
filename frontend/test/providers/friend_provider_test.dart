import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/friend_service.dart';
import 'package:heart_lake/data/datasources/temp_friend_service.dart';
import 'package:heart_lake/data/datasources/websocket_manager.dart';
import 'package:heart_lake/presentation/providers/friend_provider.dart';

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
}

class FakeFriendDataSource implements FriendDataSource {
  Map<String, dynamic> friendsResult = {'success': true, 'friends': const []};
  Map<String, dynamic> pendingResult = {'success': true, 'requests': const []};
  Map<String, dynamic> acceptResult = {'success': true};
  Map<String, dynamic> rejectResult = {'success': true};
  Map<String, dynamic> removeResult = {'success': true};
  Map<String, dynamic> sendResult = {'success': true};

  @override
  Future<Map<String, dynamic>> acceptFriendRequest(String userId) async =>
      acceptResult;

  @override
  Future<Map<String, dynamic>> getFriends() async => friendsResult;

  @override
  Future<Map<String, dynamic>> getMessages(String friendId) async =>
      {'success': true, 'messages': const []};

  @override
  Future<Map<String, dynamic>> getPendingRequests() async => pendingResult;

  @override
  Future<Map<String, dynamic>> rejectFriendRequest(String userId) async =>
      rejectResult;

  @override
  Future<Map<String, dynamic>> removeFriend(String friendId) async =>
      removeResult;

  @override
  Future<Map<String, dynamic>> sendFriendRequest({
    required String userId,
    String? message,
  }) async =>
      sendResult;

  @override
  Future<Map<String, dynamic>> sendMessage(
    String friendId,
    String content,
  ) async =>
      {'success': true};
}

class FakeTempFriendDataSource implements TempFriendDataSource {
  Map<String, dynamic> tempFriendsResult = {
    'success': true,
    'temp_friends': const [],
  };
  Map<String, dynamic> upgradeResult = {'success': true};

  @override
  Future<Map<String, dynamic>> checkTempFriendStatus(String userId) async =>
      {'success': true};

  @override
  Future<Map<String, dynamic>> createTempFriend(String userId) async =>
      {'success': true};

  @override
  Future<Map<String, dynamic>> deleteTempFriend(String tempFriendId) async =>
      {'success': true};

  @override
  Future<Map<String, dynamic>> getMyTempFriends() async => tempFriendsResult;

  @override
  Future<Map<String, dynamic>> getTempFriendDetail(String tempFriendId) async =>
      {'success': true};

  @override
  Future<Map<String, dynamic>> upgradeToPermanent(String tempFriendId) async =>
      upgradeResult;
}

void main() {
  late FakeFriendDataSource friendService;
  late FakeTempFriendDataSource tempFriendService;
  late FakeWebSocketClient wsClient;
  late FriendProvider provider;

  setUp(() {
    friendService = FakeFriendDataSource();
    tempFriendService = FakeTempFriendDataSource();
    wsClient = FakeWebSocketClient();
    provider = FriendProvider(
      friendService: friendService,
      tempFriendService: tempFriendService,
      wsManager: wsClient,
    );
  });

  tearDown(() {
    provider.dispose();
  });

  group('FriendProvider', () {
    test('loads friends and supports collection fallbacks', () async {
      friendService.friendsResult = {
        'success': true,
        'items': [
          {'user_id': 'u1', 'nickname': '甲'},
          {'friendId': 'u2', 'nickname': '乙'},
        ],
      };

      await provider.fetchFriends();

      expect(provider.friends, hasLength(2));
      expect(provider.friendCount, 2);
      expect(provider.friends.last['friendId'], 'u2');
      expect(provider.isLoading, false);
    });

    test('loads temp friends and pending requests', () async {
      tempFriendService.tempFriendsResult = {
        'success': true,
        'list': [
          {'temp_friend_id': 'tf1', 'friend_id': 'u1'},
        ],
      };
      friendService.pendingResult = {
        'success': true,
        'requests': [
          {'user_id': 'u9', 'nickname': '待处理'},
        ],
      };

      await provider.fetchTempFriends();
      await provider.fetchPendingRequests();

      expect(provider.tempFriends, hasLength(1));
      expect(provider.pendingRequests, hasLength(1));
      expect(provider.pendingCount, 1);
    });

    test('accept request removes pending item and refreshes friends', () async {
      friendService.pendingResult = {
        'success': true,
        'requests': [
          {'user_id': 'u1', 'nickname': '请求者'},
        ],
      };
      await provider.fetchPendingRequests();

      friendService.friendsResult = {
        'success': true,
        'friends': [
          {'user_id': 'u1', 'nickname': '新好友'},
        ],
      };

      final result = await provider.acceptRequest('u1');

      expect(result['success'], true);
      expect(provider.pendingRequests, isEmpty);
      expect(provider.friends, hasLength(1));
      expect(provider.friends.first['user_id'], 'u1');
    });

    test('reject request removes pending item without refreshing friends',
        () async {
      friendService.pendingResult = {
        'success': true,
        'requests': [
          {'user_id': 'u1'},
        ],
      };
      await provider.fetchPendingRequests();

      final result = await provider.rejectRequest('u1');

      expect(result['success'], true);
      expect(provider.pendingRequests, isEmpty);
      expect(provider.friends, isEmpty);
    });

    test('remove friend updates list locally', () async {
      friendService.friendsResult = {
        'success': true,
        'friends': [
          {'user_id': 'u1'},
          {'user_id': 'u2'},
        ],
      };
      await provider.fetchFriends();

      final result = await provider.removeFriend('u1');

      expect(result['success'], true);
      expect(provider.friends, hasLength(1));
      expect(provider.friends.single['user_id'], 'u2');
    });

    test('upgrade to permanent removes temp friend and refreshes friends',
        () async {
      tempFriendService.tempFriendsResult = {
        'success': true,
        'temp_friends': [
          {'temp_friend_id': 'tf1', 'friend_id': 'u1'},
        ],
      };
      await provider.fetchTempFriends();

      friendService.friendsResult = {
        'success': true,
        'friends': [
          {'user_id': 'u1'},
        ],
      };

      final result = await provider.upgradeToPermanent('tf1');

      expect(result['success'], true);
      expect(provider.tempFriends, isEmpty);
      expect(provider.friends, hasLength(1));
    });

    test('reacts to websocket lifecycle events', () async {
      friendService.friendsResult = {
        'success': true,
        'friends': [
          {'user_id': 'u1', 'is_online': false},
        ],
      };
      tempFriendService.tempFriendsResult = {
        'success': true,
        'temp_friends': [
          {'temp_friend_id': 'tf1', 'friend_id': 'u2'},
        ],
      };
      await provider.fetchFriends();
      await provider.fetchTempFriends();

      wsClient.emit('friend_online', {'user_id': 'u1'});
      expect(provider.friends.first['is_online'], true);

      wsClient.emit('friend_offline', {'user_id': 'u1'});
      expect(provider.friends.first['is_online'], false);

      wsClient.emit('friend_request', {
        'target_user_id': 'u9',
        'avatar': 'https://cdn.example.com/avatar.png',
      });
      expect(provider.pendingRequests, hasLength(1));
      expect(provider.pendingRequests.first['friend_id'], 'u9');

      wsClient.emit('temp_friend_expired', {'temp_friend_id': 'tf1'});
      expect(provider.tempFriends, isEmpty);

      wsClient.emit('friend_removed', {'user_id': 'u1'});
      expect(provider.friends, isEmpty);
    });

    test('clear resets all local state', () async {
      friendService.friendsResult = {
        'success': true,
        'friends': [
          {'user_id': 'u1'},
        ],
      };
      friendService.pendingResult = {
        'success': true,
        'requests': [
          {'user_id': 'u9'},
        ],
      };
      tempFriendService.tempFriendsResult = {
        'success': true,
        'temp_friends': [
          {'temp_friend_id': 'tf1'},
        ],
      };

      await provider.fetchFriends();
      await provider.fetchPendingRequests();
      await provider.fetchTempFriends();

      provider.clear();

      expect(provider.friends, isEmpty);
      expect(provider.pendingRequests, isEmpty);
      expect(provider.tempFriends, isEmpty);
      expect(provider.friendCount, 0);
      expect(provider.pendingCount, 0);
    });
  });
}
