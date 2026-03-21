// 用户状态管理
//
// 管理匿名登录、本地会话恢复、用户信息加载与更新等认证流程。
// 登录成功后自动建立 WebSocket 连接，登出时断开并清理本地存储。
// 依赖 [AuthDataSource] 与 [RealtimeClient] 完成会话交互。

import 'dart:async';

import 'package:flutter/foundation.dart';
import '../../domain/entities/user.dart';
import '../../data/datasources/auth_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';

/// 用户认证与会话状态管理器
///
/// 维护当前登录用户的完整生命周期：匿名登录 -> 会话恢复 -> 资料更新 -> 登出。
/// 登录成功后自动建立 WebSocket 连接，登出时断开并清理本地存储。
class UserProvider with ChangeNotifier {
  final AuthDataSource _authService;
  final RealtimeClient _wsManager;
  StreamSubscription<StoredAuthSession?>? _authStateSubscription;

  User? _user;
  bool _isLoading = false;
  bool _isAnonymous = false;

  // 只读访问器
  User? get user => _user;
  bool get isLoading => _isLoading;
  bool get isLoggedIn => _user != null;
  bool get isAnonymous => _isAnonymous;
  bool get isVIP => _user?.isVIP ?? false;
  String? get userId => _user?.userId;
  String? get nickname => _user?.nickname;

  UserProvider({
    AuthDataSource? authService,
    RealtimeClient? wsManager,
  })  : _authService = authService ?? sl<AuthService>(),
        _wsManager = wsManager ?? WebSocketManager() {
    _authStateSubscription = _authService.authStateChanges.listen(
      (session) {
        if (session == null) {
          _clearLocalSession();
          return;
        }
        _applyStoredSession(session, refreshProfileAfter: true);
      },
    );
  }

  void _applyStoredSession(
    StoredAuthSession session, {
    bool refreshProfileAfter = false,
  }) {
    _user = User(
      userId: session.userId,
      nickname: session.nickname ?? '用户',
      isAnonymous: true,
    );
    _isAnonymous = true;
    notifyListeners();
    unawaited(_wsManager.connect());
    if (refreshProfileAfter) {
      unawaited(refreshProfile());
    }
  }

  void _clearLocalSession() {
    _wsManager.disconnect();
    _user = null;
    _isAnonymous = false;
    notifyListeners();
  }

  /// 释放资源，断开WebSocket连接
  @override
  void dispose() {
    _authStateSubscription?.cancel();
    _wsManager.disconnect();
    super.dispose();
  }

  /// 匿名登录（游客模式）
  ///
  /// 登录成功后自动建立WebSocket连接。
  Future<bool> anonymousLogin() async {
    _isLoading = true;
    notifyListeners();

    try {
      final result = await _authService.anonymousLogin();
      return result['success'] == true;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('匿名登录失败: $e');
      }
      return false;
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  /// 从本地存储恢复用户状态
  ///
  /// 先用本地缓存构建临时User，再通过refreshProfile获取最新数据。
  Future<bool> restoreUser() async {
    final session = await _authService.getStoredSession();
    if (session == null) return false;

    _applyStoredSession(session, refreshProfileAfter: true);
    return true;
  }

  /// 刷新用户资料
  ///
  /// 从后端获取最新用户信息并更新本地状态。
  Future<void> refreshProfile() async {
    if (_user == null) return;
    try {
      final result = await _authService.getUserProfile(_user!.userId);
      if (result['success'] == true && result['user'] is Map<String, dynamic>) {
        _user = User.fromJson(result['user'] as Map<String, dynamic>);
        _isAnonymous = _user?.isAnonymous ?? false;
        notifyListeners();
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('刷新资料失败: $e');
      }
    }
  }

  /// 更新昵称
  ///
  /// [nickname] 新昵称
  Future<bool> updateNickname(String nickname) async {
    try {
      final result = await _authService.updateNickname(nickname);
      if (result['success'] == true && _user != null) {
        _user = _user!.copyWith(
          nickname: result['nickname']?.toString() ?? _user!.nickname,
        );
        notifyListeners();
        return true;
      }
      return false;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('更新昵称失败: $e');
      }
      return false;
    }
  }

  /// 更新用户资料
  ///
  /// [nickname] 昵称
  /// [avatarUrl] 头像URL
  /// [bio] 个人简介
  Future<bool> updateProfile(
      {String? nickname, String? avatarUrl, String? bio}) async {
    try {
      final result = await _authService.updateProfile(
          nickname: nickname, avatarUrl: avatarUrl, bio: bio);
      if (result['success'] == true && _user != null) {
        _user = _user!.copyWith(
          nickname: result['nickname']?.toString() ?? _user!.nickname,
          avatarUrl: result['avatar_url']?.toString() ?? _user!.avatarUrl,
          bio: result['bio']?.toString() ?? _user!.bio,
        );
        notifyListeners();
        return true;
      }
      return false;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('更新资料失败: $e');
      }
      return false;
    }
  }

  /// 仅更新本地用户信息（不调用后端，用于其他页面编辑后同步状态）
  void updateUser({String? nickname, String? avatarUrl, String? bio}) {
    if (_user == null) return;
    _user = _user!.copyWith(nickname: nickname, avatarUrl: avatarUrl, bio: bio);
    notifyListeners();
  }

  /// 登出
  ///
  /// 断开WebSocket连接，清除本地凭证和用户状态。
  Future<void> logout() async {
    await _authService.logout();
  }
}
