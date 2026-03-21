/// 缓存服务
///
/// 提供内存缓存管理功能，使用最旧优先淘汰策略（FIFO）。
/// 支持自动过期清理和缓存命中率统计。

library;

import 'dart:async';
import 'package:flutter/foundation.dart';
import '../../utils/app_config.dart';
import '../../utils/app_logger.dart';

abstract class CacheStore {
  T? get<T>(String key);
  void set<T>(String key, T data, {Duration? ttl});
  void removeByPrefix(String prefix);
}

/// 缓存条目
///
/// 内部类，存储缓存数据、时间戳和有效期。
class _CacheEntry<T> {
  final T data;
  final DateTime timestamp;
  final Duration duration;

  _CacheEntry({
    required this.data,
    required this.timestamp,
    required this.duration,
  });

  bool get isExpired {
    return DateTime.now().difference(timestamp) > duration;
  }

  Duration get remainingTime {
    final elapsed = DateTime.now().difference(timestamp);
    final remaining = duration - elapsed;
    return remaining.isNegative ? Duration.zero : remaining;
  }
}

/// 缓存服务
///
/// 使用最旧优先淘汰策略的内存缓存管理器。
class CacheService implements CacheStore {
  static final CacheService _instance = CacheService._internal();
  factory CacheService() => _instance;
  CacheService._internal();

  /// 缓存存储
  final Map<String, _CacheEntry<dynamic>> _cache = {};

  /// 缓存命中率统计
  int _hits = 0;
  int _misses = 0;

  /// 定时清理任务的 Timer
  Timer? _cleanupTimer;

  /// 获取缓存命中率
  double get hitRate {
    final total = _hits + _misses;
    return total == 0 ? 0.0 : _hits / total;
  }

  /// 设置缓存
  ///
  /// 如果缓存已满，会自动淘汰最旧的条目。
  ///
  /// [key] 缓存键
  /// [data] 缓存数据
  /// [ttl] 有效期，默认使用配置的石头缓存时长
  @override
  void set<T>(String key, T data, {Duration? ttl}) {
    // 检查缓存大小限制
    if (_cache.length >= appConfig.maxCacheEntries) {
      _evictOldest();
    }

    final entry = _CacheEntry<T>(
      data: data,
      timestamp: DateTime.now(),
      duration: ttl ?? appConfig.stoneCacheDuration,
    );

    _cache[key] = entry;

    logger.debug(
      '缓存已设置: $key, 有效期: ${entry.duration.inSeconds}秒',
      category: LogCategory.system,
    );
  }

  /// 获取缓存
  ///
  /// 返回缓存数据，如果不存在或已过期返回null。
  ///
  /// [key] 缓存键
  @override
  T? get<T>(String key) {
    final entry = _cache[key];

    if (entry == null) {
      _misses++;
      logger.debug('缓存未命中: $key', category: LogCategory.system);
      return null;
    }

    if (entry.isExpired) {
      _misses++;
      _cache.remove(key);
      logger.debug('缓存已过期: $key', category: LogCategory.system);
      return null;
    }

    _hits++;
    logger.debug(
      '缓存命中: $key, 剩余: ${entry.remainingTime.inSeconds}秒',
      category: LogCategory.system,
    );

    final data = entry.data;
    return data is T ? data : null;
  }

  /// 删除缓存
  ///
  /// [key] 缓存键
  void remove(String key) {
    _cache.remove(key);
    logger.debug('缓存已删除: $key', category: LogCategory.system);
  }

  /// 删除指定前缀的所有缓存
  ///
  /// [prefix] 缓存键前缀
  @override
  void removeByPrefix(String prefix) {
    final keysToRemove =
        _cache.keys.where((key) => key.startsWith(prefix)).toList();
    for (final key in keysToRemove) {
      _cache.remove(key);
    }
    if (keysToRemove.isNotEmpty) {
      logger.debug('已删除${keysToRemove.length}个前缀为"$prefix"的缓存',
          category: LogCategory.system);
    }
  }

  /// 清空所有缓存
  ///
  /// 清除所有缓存条目并重置统计数据。
  void clear() {
    _cache.clear();
    _hits = 0;
    _misses = 0;
    logger.info('所有缓存已清空', category: LogCategory.system);
  }

  /// 清空过期缓存
  ///
  /// 遍历所有缓存条目，删除已过期的条目。
  void clearExpired() {
    final expiredKeys = _cache.entries
        .where((entry) => entry.value.isExpired)
        .map((entry) => entry.key)
        .toList();

    for (final key in expiredKeys) {
      _cache.remove(key);
    }

    if (expiredKeys.isNotEmpty) {
      logger.debug(
        '已清理${expiredKeys.length}个过期缓存',
        category: LogCategory.system,
      );
    }
  }

  /// 移除最旧的缓存条目
  void _evictOldest() {
    if (_cache.isEmpty) return;

    // 找到最旧的条目
    String? oldestKey;
    DateTime? oldestTime;

    for (final entry in _cache.entries) {
      if (oldestTime == null || entry.value.timestamp.isBefore(oldestTime)) {
        oldestTime = entry.value.timestamp;
        oldestKey = entry.key;
      }
    }

    if (oldestKey != null) {
      _cache.remove(oldestKey);
      logger.debug('缓存已满，移除最旧缓存: $oldestKey', category: LogCategory.system);
    }
  }

  /// 获取缓存统计信息
  ///
  /// 返回包含总条目数、命中率、过期条目数等统计数据的Map。
  Map<String, dynamic> getStats() {
    return {
      'total_entries': _cache.length,
      'max_entries': appConfig.maxCacheEntries,
      'hits': _hits,
      'misses': _misses,
      'hit_rate': hitRate,
      'expired_count': _cache.values.where((e) => e.isExpired).length,
    };
  }

  /// 打印缓存统计
  ///
  /// 调试用，输出缓存统计信息到日志。
  void printStats() {
    final stats = getStats();
    logger.info(
      '缓存统计: ${stats['total_entries']}/${stats['max_entries']} 条目, '
      '命中率: ${((stats['hit_rate'] as double) * 100).toStringAsFixed(1)}%, '
      '命中/未命中: ${stats['hits']}/${stats['misses']}',
      category: LogCategory.system,
    );
  }

  /// 启动定时清理任务
  ///
  /// 定期清理过期缓存，默认每5分钟执行一次。
  ///
  /// [interval] 清理间隔
  void startAutoCleanup({Duration interval = const Duration(minutes: 5)}) {
    // 如果已经有定时器在运行，先取消
    _cleanupTimer?.cancel();

    _cleanupTimer = Timer.periodic(interval, (_) {
      clearExpired();
      if (kDebugMode) {
        printStats();
      }
    });

    logger.info(
      '缓存自动清理已启动，间隔: ${interval.inMinutes}分钟',
      category: LogCategory.system,
    );
  }

  /// 停止定时清理任务
  ///
  /// 取消定时器，停止自动清理。
  void stopAutoCleanup() {
    _cleanupTimer?.cancel();
    _cleanupTimer = null;

    logger.info(
      '缓存自动清理已停止',
      category: LogCategory.system,
    );
  }

  /// 释放资源
  ///
  /// 停止定时清理任务并清空所有缓存。
  void dispose() {
    stopAutoCleanup();
    _cache.clear();
    _hits = 0;
    _misses = 0;

    logger.info(
      '缓存服务已释放',
      category: LogCategory.system,
    );
  }
}
