/// 用户状态管理
///
/// 管理匿名登录、本地会话恢复、用户信息加载与更新等认证流程。
/// 登录成功后自动建立 WebSocket 连接，登出时断开并清理本地存储。
/// 依赖 [AuthService] 完成认证交互，依赖 [StorageUtil] 做本地持久化。

import 'package:flutter/foundation.dart';
import '../../domain/entities/user.dart';
import '../../data/datasources/auth_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../data/datasources/api_client.dart';
import '../../utils/storage_util.dart';
import '../../di/service_locator.dart';

/// 用户认证与会话状态管理器
///
/// 维护当前登录用户的完整生命周期：匿名登录 -> 会话恢复 -> 资料更新 -> 登出。
/// 登录成功后自动建立 WebSocket 连接，登出时断开并清理本地存储。
class UserProvider with ChangeNotifier {
  final AuthService _authService = sl<AuthService>();
  final WebSocketManager _wsManager = WebSocketManager();
  final ApiClient _apiClient = ApiClient();

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

  /// 释放资源，断开WebSocket连接
  @override
  void dispose() {
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
      if (result['success'] == true) {
        _user = User(userId: result['user_id'], nickname: result['nickname'], isAnonymous: true);
        _isAnonymous = true;
        _wsManager.connect();
        return true;
      }
      return false;
    } catch (e) {
      if (kDebugMode) { debugPrint('匿名登录失败: $e'); }
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
    final userId = await StorageUtil.getUserId();
    final nickname = await StorageUtil.getNickname();
    final token = await StorageUtil.getToken();

    if (userId != null && token != null) {
      // 先临时标记为匿名，refreshProfile 成功后根据实际数据更新
      _user = User(userId: userId, nickname: nickname ?? '用户', isAnonymous: true);
      _isAnonymous = true;
      notifyListeners();
      _wsManager.connect();
      await refreshProfile();
      // refreshProfile 成功后，根据实际用户数据判断是否匿名
      if (_user != null) {
        _isAnonymous = _user!.isAnonymous;
        notifyListeners();
      }
      return true;
    }
    return false;
  }

  /// 刷新用户资料
  ///
  /// 从后端获取最新用户信息并更新本地状态。
  Future<void> refreshProfile() async {
    if (_user == null) return;
    try {
      final response = await _apiClient.get('/users/${_user!.userId}');
      if (response.statusCode == 200 && response.data['code'] == 0) {
        final data = response.data['data'];
        _user = User.fromJson(data);
        notifyListeners();
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('刷新资料失败: $e'); }
    }
  }

  /// 更新昵称
  ///
  /// [nickname] 新昵称
  Future<bool> updateNickname(String nickname) async {
    try {
      final result = await _authService.updateNickname(nickname);
      if (result['success'] == true && _user != null) {
        _user = _user!.copyWith(nickname: nickname);
        await StorageUtil.saveNickname(nickname);
        notifyListeners();
        return true;
      }
      return false;
    } catch (e) {
      if (kDebugMode) { debugPrint('更新昵称失败: $e'); }
      return false;
    }
  }

  /// 更新用户资料
  ///
  /// [nickname] 昵称
  /// [avatarUrl] 头像URL
  /// [bio] 个人简介
  Future<bool> updateProfile({String? nickname, String? avatarUrl, String? bio}) async {
    try {
      final result = await _authService.updateProfile(nickname: nickname, avatarUrl: avatarUrl, bio: bio);
      if (result['success'] == true && _user != null) {
        _user = _user!.copyWith(nickname: nickname, avatarUrl: avatarUrl, bio: bio);
        if (nickname != null) await StorageUtil.saveNickname(nickname);
        notifyListeners();
        return true;
      }
      return false;
    } catch (e) {
      if (kDebugMode) { debugPrint('更新资料失败: $e'); }
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
    _wsManager.disconnect();
    await _authService.logout();
    _user = null;
    _isAnonymous = false;
    notifyListeners();
  }

}
