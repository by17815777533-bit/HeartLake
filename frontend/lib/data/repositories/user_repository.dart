// @file user_repository.dart
// @brief 用户数据仓库
// Created by 王璐瑶

import '../../domain/entities/user.dart';
import '../datasources/user_service.dart';
import 'base_repository.dart';

/// User 数据仓库
class UserRepository extends BaseRepository {
  final UserService _userService = UserService();

  Future<User?> getCurrentUser() async {
    const cacheKey = 'current_user';

    final cached = cacheService.get<User>(cacheKey);
    if (cached != null) return cached;

    final result = await _userService.getCurrentUser();
    if (result['success'] == true && result['user'] != null) {
      final user = User.fromJson(result['user'] as Map<String, dynamic>);
      cacheService.set(cacheKey, user, ttl: const Duration(minutes: 10));
      return user;
    }
    return null;
  }

  Future<User?> getUserById(String userId) async {
    final cacheKey = 'user_$userId';

    final cached = cacheService.get<User>(cacheKey);
    if (cached != null) return cached;

    final result = await _userService.getUserProfile(userId);
    if (result['success'] == true && result['user'] != null) {
      final user = User.fromJson(result['user'] as Map<String, dynamic>);
      cacheService.set(cacheKey, user, ttl: const Duration(minutes: 5));
      return user;
    }
    return null;
  }

  Future<bool> updateProfile({String? nickname, String? bio, String? avatarUrl}) async {
    final result = await _userService.updateProfile(
      nickname: nickname,
      bio: bio,
      avatarUrl: avatarUrl,
    );
    if (result['success'] == true) {
      cacheService.remove('current_user');
      return true;
    }
    return false;
  }

  void clearUserCache() {
    cacheService.remove('current_user');
    invalidateCache('user_');
  }
}
