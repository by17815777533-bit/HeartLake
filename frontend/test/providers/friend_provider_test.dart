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
  Map<String, dynamic> removeResult = {'success': true};

  @override
  Future<Map<String, dynamic>> getFriends() async => friendsResult;

  @override
  Future<Map<String, dynamic>> getMessages(String friendId) async =>
      {'success': true, 'messages': const []};

  @override
  Future<Map<String, dynamic>> removeFriend(String friendId) async =>
      removeResult;

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

    test('loads temp friends', () async {
      tempFriendService.tempFriendsResult = {
        'success': true,
        'list': [
          {'temp_friend_id': 'tf1', 'friend_id': 'u1'},
        ],
      };

      await provider.fetchTempFriends();

      expect(provider.tempFriends, hasLength(1));
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

      friendService.friendsResult = {
        'success': true,
        'friends': [
          {'user_id': 'u1', 'is_online': false},
          {'user_id': 'u3', 'is_online': true},
        ],
      };
      wsClient.emit('friend_accepted', {'friend_id': 'u3'});
      await Future<void>.delayed(Duration.zero);
      expect(provider.friends, hasLength(2));

      wsClient.emit('temp_friend_expired', {'temp_friend_id': 'tf1'});
      expect(provider.tempFriends, isEmpty);

      wsClient.emit('friend_removed', {'user_id': 'u1'});
      expect(provider.friends.any((item) => item['user_id'] == 'u1'), isFalse);
    });

    test('clear resets all local state', () async {
      friendService.friendsResult = {
        'success': true,
        'friends': [
          {'user_id': 'u1'},
        ],
      };
      tempFriendService.tempFriendsResult = {
        'success': true,
        'temp_friends': [
          {'temp_friend_id': 'tf1'},
        ],
      };

      await provider.fetchFriends();
      await provider.fetchTempFriends();

      provider.clear();

      expect(provider.friends, isEmpty);
      expect(provider.tempFriends, isEmpty);
      expect(provider.friendCount, 0);
    });
  });
}
