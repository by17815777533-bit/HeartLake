// @file user_provider.dart
// @brief 用户状态管理 - 符合中国用户习惯
// Created by 林子怡

import 'package:flutter/foundation.dart';
import '../../domain/entities/user.dart';
import '../../data/datasources/auth_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../data/datasources/api_client.dart';
import '../../utils/storage_util.dart';

class UserProvider with ChangeNotifier {
  final AuthService _authService = AuthService();
  final WebSocketManager _wsManager = WebSocketManager();
  final ApiClient _apiClient = ApiClient();

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

  // P1-2: 添加 dispose 方法，断开 WS 连接
  @override
  void dispose() {
    _wsManager.disconnect();
    super.dispose();
  }

  // 匿名登录（游客模式）
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

  // P1-2: 恢复用户状态，修复匿名状态判断逻辑
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

  // 刷新用户资料
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

  // 更新昵称
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

  // 更新资料
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

  // 更新本地用户信息
  void updateUser({String? nickname, String? avatarUrl, String? bio}) {
    if (_user == null) return;
    _user = _user!.copyWith(nickname: nickname, avatarUrl: avatarUrl, bio: bio);
    notifyListeners();
  }

  // 登出
  Future<void> logout() async {
    _wsManager.disconnect();
    await _authService.logout();
    _user = null;
    _isAnonymous = false;
    notifyListeners();
  }

}
