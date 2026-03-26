// 用户状态管理
//
// 管理匿名登录、本地会话恢复、用户信息加载与更新等认证流程。
// 登录成功后自动建立 WebSocket 连接，登出时断开并清理本地存储。
// 依赖 [AuthDataSource] 与 [RealtimeClient] 完成会话交互。

import 'dart:async';

import 'package:flutter/foundation.dart';
import '../../data/datasources/account_service.dart';
import '../../domain/entities/user.dart';
import '../../data/datasources/auth_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/storage_util.dart';

/// 用户认证与会话状态管理器
///
/// 维护当前登录用户的完整生命周期：匿名登录 -> 会话恢复 -> 资料更新 -> 登出。
/// 登录成功后自动建立 WebSocket 连接，登出时断开并清理本地存储。
class UserProvider with ChangeNotifier {
  final AuthDataSource _authService;
  final AccountService _accountService;
  final RealtimeClient _wsManager;
  StreamSubscription<StoredAuthSession?>? _authStateSubscription;

  User? _user;
  bool _isLoading = false;
  bool _isAnonymous = false;
  String? _errorMessage;

  // 只读访问器
  User? get user => _user;
  bool get isLoading => _isLoading;
  bool get isLoggedIn => _user != null;
  bool get isAnonymous => _isAnonymous;
  bool get isVIP => _user?.isVIP ?? false;
  String? get userId => _user?.userId;
  String? get nickname => _user?.nickname;
  String? get errorMessage => _errorMessage;

  UserProvider({
    AuthDataSource? authService,
    AccountService? accountService,
    RealtimeClient? wsManager,
  })  : _authService = authService ?? sl<AuthService>(),
        _accountService = accountService ?? sl<AccountService>(),
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
    final isAnonymous = session.isAnonymous ?? true;
    final nickname = session.nickname?.trim() ?? '';
    _user = User(
      userId: session.userId,
      nickname: nickname,
      isAnonymous: isAnonymous,
    );
    _isAnonymous = isAnonymous;
    _errorMessage = null;
    notifyListeners();
    unawaited(_wsManager.connect());
    if (refreshProfileAfter) {
      unawaited(refreshProfile());
    }
  }

  void _reportProviderError(
    Object error,
    StackTrace stackTrace,
    String context,
  ) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stackTrace,
        library: 'heartlake',
        context: ErrorDescription(context),
      ),
    );
  }

  void _setErrorMessage(String? message, {bool notify = false}) {
    _errorMessage = message;
    if (notify) {
      notifyListeners();
    }
  }

  String _extractErrorMessage(Object error, String fallback) {
    final raw = error
        .toString()
        .replaceFirst(RegExp(r'^Bad state:\s*'), '')
        .replaceFirst(RegExp(r'^Exception:\s*'), '');
    final message = raw.trim();
    return message.isEmpty ? fallback : message;
  }

  Future<bool> _ensureLocalUserLoaded() async {
    if (_user != null) return true;

    final session = await _authService.getStoredSession();
    if (session == null) {
      _setErrorMessage('登录状态已失效', notify: true);
      return false;
    }

    _applyStoredSession(session);
    return true;
  }

  void _clearLocalSession() {
    _wsManager.disconnect();
    _user = null;
    _isAnonymous = false;
    _errorMessage = null;
    notifyListeners();
  }

  Future<bool> _reloadProfileFromServer() async {
    if (_user == null) {
      _setErrorMessage('用户状态缺失', notify: true);
      return false;
    }

    final result = await _authService.getUserProfile(_user!.userId);
    if (result['success'] != true) {
      final message = result['message']?.toString() ?? '刷新资料失败';
      _setErrorMessage(message, notify: true);
      _reportProviderError(
        StateError(message),
        StackTrace.current,
        'UserProvider._reloadProfileFromServer',
      );
      return false;
    }

    final userPayload = result['user'];
    if (userPayload is! Map) {
      const message = '用户资料响应缺少 user 对象';
      _setErrorMessage(message, notify: true);
      _reportProviderError(
        StateError(message),
        StackTrace.current,
        'UserProvider._reloadProfileFromServer',
      );
      return false;
    }

    _user = User.fromJson(
      Map<String, dynamic>.from(userPayload.cast<String, dynamic>()),
    );
    if (_user!.userId.trim().isEmpty) {
      const message = '用户资料响应缺少 user_id';
      _setErrorMessage(message, notify: true);
      _reportProviderError(
        StateError(message),
        StackTrace.current,
        'UserProvider._reloadProfileFromServer',
      );
      return false;
    }
    final nickname = _user?.nickname.trim();
    if (nickname != null && nickname.isNotEmpty) {
      await StorageUtil.saveNickname(nickname);
    }
    _isAnonymous = _user?.isAnonymous ?? false;
    _errorMessage = null;
    notifyListeners();
    return true;
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
    _errorMessage = null;
    notifyListeners();

    try {
      final result = await _authService.anonymousLogin();
      if (result['success'] == true) {
        _errorMessage = null;
        return true;
      }

      final message = result['message']?.toString() ?? '匿名登录失败';
      _setErrorMessage(message);
      _reportProviderError(
        StateError(message),
        StackTrace.current,
        'UserProvider.anonymousLogin',
      );
      return false;
    } catch (error, stackTrace) {
      _setErrorMessage('匿名登录失败，请稍后重试');
      _reportProviderError(
        error,
        stackTrace,
        'UserProvider.anonymousLogin',
      );
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
    final hasLocalUser = await _ensureLocalUserLoaded();
    if (!hasLocalUser || _user == null) return;

    try {
      await _reloadProfileFromServer();
    } catch (error, stackTrace) {
      _setErrorMessage('刷新资料失败，请稍后重试', notify: true);
      _reportProviderError(
        error,
        stackTrace,
        'UserProvider.refreshProfile',
      );
    }
  }

  /// 更新昵称
  ///
  /// [nickname] 新昵称
  Future<bool> updateNickname(String nickname) async {
    final hasLocalUser = await _ensureLocalUserLoaded();
    if (!hasLocalUser || _user == null) return false;

    try {
      await _accountService.updateProfile({
        'nickname': nickname,
      });
      return await _reloadProfileFromServer();
    } catch (error, stackTrace) {
      _setErrorMessage(
        _extractErrorMessage(error, '更新昵称失败，请稍后重试'),
        notify: true,
      );
      _reportProviderError(
        error,
        stackTrace,
        'UserProvider.updateNickname',
      );
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
    final hasLocalUser = await _ensureLocalUserLoaded();
    if (!hasLocalUser || _user == null) return false;

    try {
      final payload = <String, dynamic>{};
      if (nickname != null) {
        payload['nickname'] = nickname;
      }
      if (avatarUrl != null) {
        payload['avatar_url'] = avatarUrl;
      }
      if (bio != null) {
        payload['bio'] = bio;
      }
      if (payload.isEmpty) {
        return true;
      }

      await _accountService.updateProfile(payload);
      return await _reloadProfileFromServer();
    } catch (error, stackTrace) {
      _setErrorMessage(
        _extractErrorMessage(error, '更新资料失败，请稍后重试'),
        notify: true,
      );
      _reportProviderError(
        error,
        stackTrace,
        'UserProvider.updateProfile',
      );
      return false;
    }
  }

  /// 仅更新本地用户信息（不调用后端，用于其他页面编辑后同步状态）
  void updateUser({String? nickname, String? avatarUrl, String? bio}) {
    if (_user == null) return;
    _user = _user!.copyWith(
      nickname: nickname ?? _user!.nickname,
      avatarUrl: avatarUrl ?? _user!.avatarUrl,
      bio: bio ?? _user!.bio,
    );
    _errorMessage = null;
    notifyListeners();
  }

  void clearError() {
    if (_errorMessage == null) return;
    _errorMessage = null;
    notifyListeners();
  }

  /// 登出
  ///
  /// 断开WebSocket连接，清除本地凭证和用户状态。
  Future<void> logout() async {
    await _authService.logout();
  }
}
