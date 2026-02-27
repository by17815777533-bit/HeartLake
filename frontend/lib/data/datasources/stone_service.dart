// 石头服务 - 负责石头的发布、获取、编辑、删除

import 'package:flutter/foundation.dart';
import '../../domain/entities/stone.dart';
import '../../utils/circuit_breaker.dart';
import '../../utils/input_validator.dart';
import 'base_service.dart';
import 'cache_service.dart';

/// 石头服务 - 负责发布、获取、编辑、删除石头
class StoneService extends BaseService {
  @override
  String get serviceName => 'StoneService';

  final CircuitBreaker _breaker = CircuitBreaker(
    name: 'stones',
    failureThreshold: 3,
    resetTimeout: const Duration(seconds: 30),
    callTimeout: const Duration(seconds: 15),
  );
  final CacheService _cache = CacheService();

  // 发布石头
  Future<Map<String, dynamic>> createStone({
    required String content,
    required String stoneType,
    required String stoneColor,
    String? moodType,
    bool isAnonymous = true,
    List<String>? tags,
  }) async {
    InputValidator.requireLength(content, '石头内容', min: 1, max: 5000);
    content = InputValidator.sanitizeText(content);
    InputValidator.requireInList(stoneType, const [
      // 与后端 StoneController 的 stone_type 白名单保持一致
      'small', 'medium', 'large', 'light', 'heavy',
    ], '石头类型');
    InputValidator.requireNonEmpty(stoneColor, '石头颜色');
    InputValidator.optionalInList(moodType, const [
      'happy', 'sad', 'angry', 'anxious', 'calm', 'confused',
      'hopeful', 'lonely', 'grateful', 'neutral',
    ], '情绪类型');
    if (tags != null) {
      InputValidator.requireListLength(tags, '标签', max: 10);
      // 对每个标签做 XSS 过滤
      tags = tags.map((t) => InputValidator.sanitizeText(t)).toList();
    }
    final response = await post('/stones', data: {
      'content': content,
      'stone_type': stoneType,
      'stone_color': stoneColor,
      'is_anonymous': isAnonymous,
      if (moodType != null) 'mood_type': moodType,
      if (tags != null) 'tags': tags,
    });

    // 高危内容检测 - 必须在success检查之前
    if (response.code == 403) {
      return {
        'success': false,
        'high_risk': true,
        'message': response.message,
        'help_tip': response.data?['message'],
      };
    }

    if (!response.success) {
      return toMap(response);
    }

    return {
      'success': true,
      'stone_id': response.data?['stone_id'],
    };
  }

  // 获取石头列表（观湖）- 带熔断器 + 缓存降级
  Future<Map<String, dynamic>> getStones({
    int page = 1,
    int pageSize = 20,
    String sort = 'latest',
  }) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    InputValidator.requireInList(sort, const ['latest', 'hot', 'random'], '排序方式');
    final cacheKey = 'stones_${page}_${pageSize}_$sort';

    try {
      final result = await _breaker.call(() async {
        final response = await get('/lake/stones', queryParameters: {
          'page': page,
          'page_size': pageSize,
          'sort': sort,
        });

        if (!response.success) return toMap(response);

        final data = response.data;
        final items = data?['stones'] as List? ?? [];
        final List<Stone> stones = [];
        for (final json in items) {
          try {
            stones.add(Stone.fromJson(json));
          } catch (e) {
            if (kDebugMode) debugPrint('跳过无法解析的石头: $e');
          }
        }

        final result = {
          'success': true,
          'stones': stones,
          'pagination': _buildPagination(data),
        };
        _cache.set(cacheKey, result, ttl: const Duration(minutes: 3));
        return result;
      });
      return result;
    } catch (_) {
      // 熔断或异常，降级到缓存
      final cached = _cache.get<Map<String, dynamic>>(cacheKey);
      if (cached != null) {
        if (kDebugMode) debugPrint('[StoneService] 熔断降级到缓存: $cacheKey');
        return {...cached, 'from_cache': true};
      }
      return {'success': false, 'message': '服务暂时不可用，请稍后重试'};
    }
  }

  // 获取我的石头
  Future<Map<String, dynamic>> getMyStones({
    int page = 1,
    int pageSize = 20,
  }) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/stones/my', queryParameters: {
      'page': page,
      'page_size': pageSize,
    });

    if (!response.success) {
      return toMap(response);
    }

    final data = response.data;
    final items = data?['stones'] as List? ?? [];
    final List<Stone> stones = [];
    for (final json in items) {
      try {
        stones.add(Stone.fromJson(json));
      } catch (e) {
        if (kDebugMode) debugPrint('跳过无法解析的石头: $e');
      }
    }

    return {
      'success': true,
      'stones': stones,
      'pagination': _buildPagination(data),
    };
  }

  // 获取湖面气象 - 带熔断器 + 缓存降级
  Future<Map<String, dynamic>> getLakeWeather() async {
    const cacheKey = 'lake_weather';
    try {
      return await _breaker.call(() async {
        final response = await get('/lake/weather');
        if (!response.success) return toMap(response);
        final result = {'success': true, 'weather': response.data};
        _cache.set(cacheKey, result, ttl: const Duration(minutes: 5));
        return result;
      });
    } catch (_) {
      final cached = _cache.get<Map<String, dynamic>>(cacheKey);
      if (cached != null) return {...cached, 'from_cache': true};
      return {'success': false, 'message': '服务暂时不可用'};
    }
  }

  // 删除石头
  Future<Map<String, dynamic>> deleteStone(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    final response = await delete('/stones/$stoneId');
    return toMap(response);
  }

  // 构建分页信息（API返回的分页字段在data顶层，不是嵌套的pagination对象）
  Map<String, dynamic> _buildPagination(Map<String, dynamic>? data) {
    final page = data?['page'] ?? 1;
    final totalPages = data?['total_pages'] ?? 1;
    final total = data?['total'] ?? 0;
    final pageSize = data?['page_size'] ?? 20;
    return {
      'page': page,
      'total_pages': totalPages,
      'total': total,
      'page_size': pageSize,
      'has_more': page < totalPages,
    };
  }

  // 获取石头详情
  Future<Map<String, dynamic>> getStoneDetail(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    final response = await get('/stones/$stoneId');
    if (!response.success) {
      return toMap(response);
    }
    return {
      'success': true,
      'stone': response.data,
    };
  }
}
