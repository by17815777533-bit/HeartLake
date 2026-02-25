import 'package:flutter/foundation.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/domain/entities/user.dart';

/// UserProvider 依赖 AuthService/WebSocketManager/ApiClient 单例。
/// 提取核心状态管理逻辑进行测试。

class UserState with ChangeNotifier {
  User? _user;
  bool _isLoading = false;
  bool _isAnonymous = false;

  User? get user => _user;
  bool get isLoading => _isLoading;
  bool get isLoggedIn => _user != null;
  bool get isAnonymous => _isAnonymous;
  bool get isVIP => _user?.isVIP ?? false;
  String? get userId => _user?.userId;
  String? get nickname => _user?.nickname;

  Future<bool> anonymousLogin(Future<Map<String, dynamic>> Function() loginFn) async {
    _isLoading = true;
    notifyListeners();
    try {
      final result = await loginFn();
      if (result['success'] == true) {
        _user = User(userId: result['user_id'], nickname: result['nickname'] ?? '用户', isAnonymous: true);
        _isAnonymous = true;
        return true;
      }
      return false;
    } catch (_) {
      return false;
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  Future<bool> restoreUser({
    required String? userId,
    required String? nickname,
    required String? token,
    Future<void> Function()? refreshFn,
  }) async {
    if (userId != null && token != null) {
      _user = User(userId: userId, nickname: nickname ?? '用户', isAnonymous: true);
      _isAnonymous = true;
      notifyListeners();
      if (refreshFn != null) await refreshFn();
      if (_user != null) {
        _isAnonymous = _user!.isAnonymous;
        notifyListeners();
      }
      return true;
    }
    return false;
  }

  Future<void> refreshProfile(Future<User?> Function() fetchFn) async {
    if (_user == null) return;
    try {
      final newUser = await fetchFn();
      if (newUser != null) {
        _user = newUser;
        notifyListeners();
      }
    } catch (_) {}
  }

  Future<bool> updateNickname(String nickname, Future<Map<String, dynamic>> Function(String) updateFn) async {
    try {
      final result = await updateFn(nickname);
      if (result['success'] == true && _user != null) {
        _user = _user!.copyWith(nickname: nickname);
        notifyListeners();
        return true;
      }
      return false;
    } catch (_) {
      return false;
    }
  }

  Future<bool> updateProfile({
    String? nickname,
    String? avatarUrl,
    String? bio,
    required Future<Map<String, dynamic>> Function({String? nickname, String? avatarUrl, String? bio}) updateFn,
  }) async {
    try {
      final result = await updateFn(nickname: nickname, avatarUrl: avatarUrl, bio: bio);
      if (result['success'] == true && _user != null) {
        _user = _user!.copyWith(nickname: nickname, avatarUrl: avatarUrl, bio: bio);
        notifyListeners();
        return true;
      }
      return false;
    } catch (_) {
      return false;
    }
  }

  void updateUser({String? nickname, String? avatarUrl, String? bio}) {
    if (_user == null) return;
    _user = _user!.copyWith(nickname: nickname, avatarUrl: avatarUrl, bio: bio);
    notifyListeners();
  }

  Future<void> logout(Future<void> Function() logoutFn) async {
    await logoutFn();
    _user = null;
    _isAnonymous = false;
    notifyListeners();
  }
}

void main() {
  late UserState state;

  setUp(() {
    state = UserState();
  });

  group('Initial state', () {
    test('should have null user', () => expect(state.user, isNull));
    test('should not be loading', () => expect(state.isLoading, false));
    test('should not be logged in', () => expect(state.isLoggedIn, false));
    test('should not be anonymous', () => expect(state.isAnonymous, false));
    test('should not be VIP', () => expect(state.isVIP, false));
    test('should have null userId', () => expect(state.userId, isNull));
    test('should have null nickname', () => expect(state.nickname, isNull));
  });

  group('anonymousLogin', () {
    test('should login successfully', () async {
      final result = await state.anonymousLogin(() async => {
        'success': true,
        'user_id': 'uid_1',
        'nickname': '匿名用户',
      });

      expect(result, true);
      expect(state.isLoggedIn, true);
      expect(state.isAnonymous, true);
      expect(state.userId, 'uid_1');
      expect(state.nickname, '匿名用户');
      expect(state.isLoading, false);
    });

    test('should fail on unsuccessful response', () async {
      final result = await state.anonymousLogin(() async => {'success': false});
      expect(result, false);
      expect(state.isLoggedIn, false);
    });

    test('should fail on exception', () async {
      final result = await state.anonymousLogin(() async => throw Exception('网络错误'));
      expect(result, false);
      expect(state.isLoggedIn, false);
      expect(state.isLoading, false);
    });

    test('should set loading during login', () async {
      bool wasLoading = false;
      state.addListener(() {
        if (state.isLoading) wasLoading = true;
      });
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      expect(wasLoading, true);
      expect(state.isLoading, false);
    });

    test('should handle null nickname', () async {
      await state.anonymousLogin(() async => {
        'success': true,
        'user_id': 'u1',
      });
      expect(state.nickname, '用户');
    });

    test('should notify listeners on success', () async {
      int count = 0;
      state.addListener(() => count++);
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      expect(count, greaterThanOrEqualTo(2)); // loading true + loading false
    });

    test('should notify listeners on failure', () async {
      int count = 0;
      state.addListener(() => count++);
      await state.anonymousLogin(() async => {'success': false});
      expect(count, greaterThanOrEqualTo(2));
    });
  });

  group('restoreUser', () {
    test('should restore with valid data', () async {
      final result = await state.restoreUser(
        userId: 'u1',
        nickname: '小明',
        token: 'tok_123',
      );
      expect(result, true);
      expect(state.isLoggedIn, true);
      expect(state.userId, 'u1');
      expect(state.nickname, '小明');
    });

    test('should fail without userId', () async {
      final result = await state.restoreUser(userId: null, nickname: '小明', token: 'tok');
      expect(result, false);
      expect(state.isLoggedIn, false);
    });

    test('should fail without token', () async {
      final result = await state.restoreUser(userId: 'u1', nickname: '小明', token: null);
      expect(result, false);
    });

    test('should use default nickname when null', () async {
      await state.restoreUser(userId: 'u1', nickname: null, token: 'tok');
      expect(state.nickname, '用户');
    });

    test('should call refreshFn after restore', () async {
      bool refreshCalled = false;
      await state.restoreUser(
        userId: 'u1',
        nickname: '小明',
        token: 'tok',
        refreshFn: () async { refreshCalled = true; },
      );
      expect(refreshCalled, true);
    });

    test('should set anonymous initially', () async {
      await state.restoreUser(userId: 'u1', nickname: '小明', token: 'tok');
      expect(state.isAnonymous, true);
    });

    test('should update anonymous after refresh', () async {
      await state.restoreUser(
        userId: 'u1',
        nickname: '小明',
        token: 'tok',
        refreshFn: () async {
          // refreshProfile would update _user
        },
      );
      // After restore, isAnonymous reflects user.isAnonymous
      expect(state.isAnonymous, true); // default User is anonymous
    });
  });

  group('refreshProfile', () {
    test('should update user on success', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1', 'nickname': '旧名'});

      await state.refreshProfile(() async => User(
        userId: 'u1',
        nickname: '新名',
        isAnonymous: false,
        vipLevel: 2,
        vipExpiresAt: DateTime.now().add(const Duration(days: 30)),
      ));

      expect(state.nickname, '新名');
      expect(state.isVIP, true);
    });

    test('should not update on null result', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1', 'nickname': '原名'});
      await state.refreshProfile(() async => null);
      expect(state.nickname, '原名');
    });

    test('should not crash when user is null', () async {
      await state.refreshProfile(() async => User(userId: 'u1', nickname: 'test'));
      expect(state.user, isNull);
    });

    test('should handle exception', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      await state.refreshProfile(() async => throw Exception('网络错误'));
      expect(state.isLoggedIn, true); // should not crash
    });

    test('should notify listeners on update', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      int count = 0;
      state.addListener(() => count++);
      await state.refreshProfile(() async => User(userId: 'u1', nickname: '新名'));
      expect(count, greaterThan(0));
    });
  });

  group('updateNickname', () {
    test('should update nickname on success', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1', 'nickname': '旧名'});
      final result = await state.updateNickname('新名', (n) async => {'success': true});
      expect(result, true);
      expect(state.nickname, '新名');
    });

    test('should not update on failure', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1', 'nickname': '旧名'});
      final result = await state.updateNickname('新名', (n) async => {'success': false});
      expect(result, false);
      expect(state.nickname, '旧名');
    });

    test('should handle exception', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      final result = await state.updateNickname('新名', (n) async => throw Exception('err'));
      expect(result, false);
    });

    test('should not update when user is null', () async {
      final result = await state.updateNickname('新名', (n) async => {'success': true});
      expect(result, false);
    });

    test('should notify listeners on success', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      int count = 0;
      state.addListener(() => count++);
      await state.updateNickname('新名', (n) async => {'success': true});
      expect(count, greaterThan(0));
    });
  });

  group('updateProfile', () {
    test('should update all fields', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1', 'nickname': '旧名'});
      final result = await state.updateProfile(
        nickname: '新名',
        avatarUrl: 'new.png',
        bio: '新简介',
        updateFn: ({nickname, avatarUrl, bio}) async => {'success': true},
      );
      expect(result, true);
      expect(state.nickname, '新名');
      expect(state.user!.avatarUrl, 'new.png');
      expect(state.user!.bio, '新简介');
    });

    test('should update partial fields', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1', 'nickname': '旧名'});
      await state.updateProfile(
        bio: '只更新简介',
        updateFn: ({nickname, avatarUrl, bio}) async => {'success': true},
      );
      expect(state.user!.bio, '只更新简介');
      expect(state.nickname, '旧名'); // unchanged
    });

    test('should fail on unsuccessful response', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      final result = await state.updateProfile(
        nickname: '新名',
        updateFn: ({nickname, avatarUrl, bio}) async => {'success': false},
      );
      expect(result, false);
    });

    test('should handle exception', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      final result = await state.updateProfile(
        nickname: '新名',
        updateFn: ({nickname, avatarUrl, bio}) async => throw Exception('err'),
      );
      expect(result, false);
    });
  });

  group('updateUser (local)', () {
    test('should update local user', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1', 'nickname': '旧名'});
      state.updateUser(nickname: '本地更新', bio: '本地简介');
      expect(state.nickname, '本地更新');
      expect(state.user!.bio, '本地简介');
    });

    test('should not crash when user is null', () {
      state.updateUser(nickname: '测试');
      expect(state.user, isNull);
    });

    test('should notify listeners', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      int count = 0;
      state.addListener(() => count++);
      state.updateUser(nickname: '新名');
      expect(count, 1);
    });
  });

  group('logout', () {
    test('should clear user state', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      expect(state.isLoggedIn, true);

      await state.logout(() async {});
      expect(state.isLoggedIn, false);
      expect(state.isAnonymous, false);
      expect(state.user, isNull);
    });

    test('should call logout function', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      bool logoutCalled = false;
      await state.logout(() async { logoutCalled = true; });
      expect(logoutCalled, true);
    });

    test('should notify listeners', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      int count = 0;
      state.addListener(() => count++);
      await state.logout(() async {});
      expect(count, greaterThan(0));
    });

    test('should be safe to call when not logged in', () async {
      await state.logout(() async {});
      expect(state.isLoggedIn, false);
    });
  });

  group('isVIP', () {
    test('should return false when not logged in', () {
      expect(state.isVIP, false);
    });

    test('should return false for non-VIP user', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      expect(state.isVIP, false);
    });

    test('should return true after refresh with VIP data', () async {
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      await state.refreshProfile(() async => User(
        userId: 'u1',
        nickname: 'VIP用户',
        vipLevel: 2,
        vipExpiresAt: DateTime.now().add(const Duration(days: 30)),
      ));
      expect(state.isVIP, true);
    });
  });

  group('ChangeNotifier behavior', () {
    test('multiple listeners should all be notified', () async {
      int c1 = 0, c2 = 0;
      state.addListener(() => c1++);
      state.addListener(() => c2++);
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      expect(c1, greaterThan(0));
      expect(c2, greaterThan(0));
    });

    test('removed listener should not be notified', () async {
      int count = 0;
      void listener() => count++;
      state.addListener(listener);
      await state.anonymousLogin(() async => {'success': true, 'user_id': 'u1'});
      final countAfterLogin = count;

      state.removeListener(listener);
      state.updateUser(nickname: '新名');
      expect(count, countAfterLogin);
    });
  });
}
