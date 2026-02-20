// @file base_repository.dart
// @brief 基础Repository抽象类
// Created by 王璐瑶

import '../datasources/api_client.dart';
import '../datasources/cache_service.dart';

/// 基础 Repository 抽象类
abstract class BaseRepository {
  final ApiClient apiClient;
  final CacheService cacheService;

  BaseRepository({ApiClient? apiClient, CacheService? cacheService})
      : apiClient = apiClient ?? ApiClient(),
        cacheService = cacheService ?? CacheService();

  /// 带缓存的 GET 请求
  Future<T?> getCached<T>({
    required String cacheKey,
    required Future<T?> Function() fetcher,
    Duration ttl = const Duration(minutes: 5),
  }) async {
    final cached = cacheService.get<T>(cacheKey);
    if (cached != null) return cached;

    final result = await fetcher();
    if (result != null) {
      cacheService.set(cacheKey, result, ttl: ttl);
    }
    return result;
  }

  /// 清除指定前缀的缓存
  void invalidateCache(String prefix) {
    cacheService.removeByPrefix(prefix);
  }
}
