import 'package:flutter/foundation.dart';
import '../../domain/entities/stone.dart';
import '../../utils/circuit_breaker.dart';
import '../../utils/input_validator.dart';
import '../../utils/mood_colors.dart';
import 'base_service.dart';
import 'cache_service.dart';
import 'social_payload_normalizer.dart';

abstract class StoneDataSource {
  Future<Map<String, dynamic>> createStone({
    required String content,
    required String stoneType,
    required String stoneColor,
    String? moodType,
    bool isAnonymous = true,
    List<String>? tags,
  });
  Future<Map<String, dynamic>> getStones({
    int page = 1,
    int pageSize = 20,
    String sort = 'latest',
  });
  Future<Map<String, dynamic>> deleteStone(String stoneId);
  Future<Map<String, dynamic>> getStoneDetail(String stoneId);
  Future<Map<String, dynamic>> getLakeWeather();
}

/// 石头（帖子）服务，负责石头的发布、列表查询、详情获取和删除
///
/// 列表接口通过 [CircuitBreaker] 做熔断保护，熔断时降级到本地缓存。
class StoneService extends BaseService implements StoneDataSource {
  @override
  String get serviceName => 'StoneService';

  final CircuitBreaker _breaker = CircuitBreaker(
    name: 'stones',
    failureThreshold: 3,
    resetTimeout: const Duration(seconds: 30),
    callTimeout: const Duration(seconds: 15),
  );
  final CacheService _cache = CacheService();

  List<Map<String, dynamic>> _extractStoneItems(dynamic data) {
    return extractNormalizedList(
      data,
      itemNormalizer: (item) => item,
      listKeys: const ['stones', 'results'],
    );
  }

  Map<String, dynamic> _buildStoneCollection(dynamic data) {
    final items = _extractStoneItems(data);
    final stones = <Stone>[];
    for (var i = 0; i < items.length; i++) {
      final json = items[i];
      try {
        stones.add(Stone.fromJson(json));
      } catch (error, stackTrace) {
        Error.throwWithStackTrace(
          FormatException(
            'StoneService 无法解析 stone[$i]，keys=${json.keys.toList()}，error=$error',
          ),
          stackTrace,
        );
      }
    }
    return buildCollectionEnvelope(data, primaryKey: 'stones', items: stones);
  }

  String _normalizeSort(String sort) {
    switch (sort) {
      case 'latest':
        return 'created_at';
      case 'hot':
        return 'ripple_count';
      case 'created_at':
      case 'view_count':
      case 'ripple_count':
      case 'boat_count':
        return sort;
      default:
        throw ArgumentError('不支持的排序方式: $sort');
    }
  }

  /// 发布一颗石头，内容经过 XSS 过滤和长度校验
  ///
  /// 后端返回 403 表示高危内容被拦截，此时返回 high_risk 标记。
  @override
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
    InputValidator.requireInList(
        stoneType,
        const [
          'small',
          'medium',
          'large',
          'light',
          'heavy',
        ],
        '石头类型');
    InputValidator.requireNonEmpty(stoneColor, '石头颜色');
    InputValidator.optionalInList(
      moodType,
      MoodColors.supportedMoodKeys,
      '情绪类型',
    );
    if (tags != null) {
      InputValidator.requireListLength(tags, '标签', max: 10);
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

    // 403 表示高危内容被内容审核拦截
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

  /// 获取湖面石头列表，支持分页和排序
  ///
  /// 通过熔断器保护，失败时降级到本地缓存数据。
  @override
  Future<Map<String, dynamic>> getStones({
    int page = 1,
    int pageSize = 20,
    String sort = 'latest',
  }) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    InputValidator.requireInList(
        sort,
        const [
          'latest',
          'hot',
          'created_at',
          'view_count',
          'ripple_count',
          'boat_count',
        ],
        '排序方式');
    final normalizedSort = _normalizeSort(sort);
    final cacheKey = 'stones_${page}_${pageSize}_$normalizedSort';

    try {
      final result = await _breaker.call(() async {
        final response = await get('/lake/stones', queryParameters: {
          'page': page,
          'page_size': pageSize,
          'sort': normalizedSort,
        });

        if (!response.success) return toMap(response);
        final result = _buildStoneCollection(response.data);
        _cache.set(cacheKey, result, ttl: const Duration(minutes: 3));
        return result;
      });
      return result;
    } catch (error, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'heartlake',
          context: ErrorDescription('StoneService.getStones'),
        ),
      );
      final cached = _cache.get<Map<String, dynamic>>(cacheKey);
      if (cached != null) {
        return {...cached, 'from_cache': true};
      }
      Error.throwWithStackTrace(error, stackTrace);
    }
  }

  /// 获取当前用户发布的石头列表
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
    return _buildStoneCollection(response.data);
  }

  /// 获取湖面实时气象数据，熔断时降级到缓存
  @override
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
    } catch (error, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'heartlake',
          context: ErrorDescription('StoneService.getLakeWeather'),
        ),
      );
      final cached = _cache.get<Map<String, dynamic>>(cacheKey);
      if (cached != null) return {...cached, 'from_cache': true};
      Error.throwWithStackTrace(error, stackTrace);
    }
  }

  /// 删除指定石头
  @override
  Future<Map<String, dynamic>> deleteStone(String stoneId) async {
    InputValidator.validateUUID(stoneId, '石头ID');
    final response = await delete('/stones/$stoneId');
    return toMap(response);
  }

  /// 获取单颗石头的详细信息
  @override
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
