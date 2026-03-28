// 用户服务 - 用户信息查询和管理

import 'base_service.dart';
import '../../utils/input_validator.dart';
import '../../utils/app_config.dart';
import 'social_payload_normalizer.dart';

/// 用户相关响应归一化器。
///
/// 统一兼容后端字段别名和列表包装格式，避免各处散落判断：
/// - 用户字段：`user_id/userId/id`、`avatar_url/avatarUrl`
/// - 列表字段：`users/items/list/results/data`
/// - 总数字段：`total/count/total_count/pagination.total/meta.total`
class UserPayloadNormalizer {
  UserPayloadNormalizer._();

  static String? _normalizeOptionalText(dynamic value) {
    final text = value?.toString().trim();
    if (text == null || text.isEmpty) return null;
    return text;
  }

  static String? resolveMediaUrl(dynamic value) {
    final raw = value?.toString().trim();
    if (raw == null || raw.isEmpty) return null;

    final parsed = Uri.tryParse(raw);
    if (parsed != null && parsed.hasScheme) {
      return raw;
    }

    final apiBaseUri = Uri.tryParse(appConfig.apiBaseUrl);
    if (apiBaseUri == null || apiBaseUri.host.isEmpty) {
      return raw;
    }

    final origin = Uri(
      scheme: apiBaseUri.scheme,
      host: apiBaseUri.host,
      port: apiBaseUri.hasPort ? apiBaseUri.port : null,
    );
    final normalizedPath = raw.startsWith('/') ? raw : '/$raw';
    return origin.resolve(normalizedPath).toString();
  }

  static Map<String, dynamic>? asMap(dynamic value) {
    if (value is Map<String, dynamic>) {
      return Map<String, dynamic>.from(value);
    }
    if (value is Map) {
      return value.map(
        (key, item) => MapEntry(key.toString(), item),
      );
    }
    return null;
  }

  static Map<String, dynamic>? normalizeUser(dynamic payload) {
    final source = _extractUserMap(payload);
    if (source == null) return null;

    final normalized = Map<String, dynamic>.from(source);
    _applyAlias(normalized, source,
        canonicalKey: 'user_id', aliasKeys: const ['userId', 'id']);
    _applyAlias(normalized, source,
        canonicalKey: 'nickname',
        aliasKeys: const ['display_name', 'displayName', 'name']);
    _applyAlias(normalized, source,
        canonicalKey: 'avatar_url',
        aliasKeys: const ['avatarUrl', 'avatar', 'avatar_path', 'image']);
    _applyAlias(normalized, source,
        canonicalKey: 'bio',
        aliasKeys: const ['signature', 'intro', 'introduction']);
    _applyAlias(normalized, source,
        canonicalKey: 'created_at', aliasKeys: const ['createdAt']);
    _applyAlias(normalized, source,
        canonicalKey: 'last_active_at',
        aliasKeys: const ['lastActiveAt', 'last_seen_at', 'lastSeenAt']);
    _applyAlias(normalized, source,
        canonicalKey: 'vip_level', aliasKeys: const ['vipLevel']);
    _applyAlias(normalized, source,
        canonicalKey: 'vip_expires_at', aliasKeys: const ['vipExpiresAt']);
    _applyAlias(normalized, source,
        canonicalKey: 'is_anonymous', aliasKeys: const ['isAnonymous']);

    _mirrorCamelAlias(normalized, 'user_id', 'userId');
    _mirrorCamelAlias(normalized, 'avatar_url', 'avatarUrl');
    _mirrorCamelAlias(normalized, 'created_at', 'createdAt');
    _mirrorCamelAlias(normalized, 'last_active_at', 'lastActiveAt');
    _mirrorCamelAlias(normalized, 'vip_level', 'vipLevel');
    _mirrorCamelAlias(normalized, 'vip_expires_at', 'vipExpiresAt');
    _mirrorCamelAlias(normalized, 'is_anonymous', 'isAnonymous');

    final resolvedAvatarUrl = resolveMediaUrl(normalized['avatar_url']);
    if (resolvedAvatarUrl != null) {
      normalized['avatar_url'] = resolvedAvatarUrl;
      normalized['avatarUrl'] = resolvedAvatarUrl;
    } else {
      normalized['avatar_url'] = null;
      normalized['avatarUrl'] = null;
    }
    normalized['bio'] = _normalizeOptionalText(normalized['bio']);

    return normalized;
  }

  static List<Map<String, dynamic>> extractUserList(dynamic payload) {
    return extractNormalizedList(
      payload,
      itemNormalizer: (item) => normalizeUser(item) ?? <String, dynamic>{},
      listKeys: const ['users', 'results'],
    ).where((item) => item.isNotEmpty).toList();
  }

  static int? extractExplicitTotal(dynamic payload) {
    for (final source in _candidateSources(payload)) {
      final direct = _toInt(source['total']) ??
          _toInt(source['count']) ??
          _toInt(source['total_count']) ??
          _toInt(asMap(source['pagination'])?['total']) ??
          _toInt(asMap(source['meta'])?['total']);
      if (direct != null) return direct;
    }
    return null;
  }

  static int extractTotal(dynamic payload) {
    final direct = extractExplicitTotal(payload);
    if (direct != null) return direct;
    throw StateError('用户集合响应缺少 total');
  }

  static Map<String, dynamic> buildUserCollection(dynamic payload) {
    final users = extractUserList(payload);
    final total = extractTotal(payload);
    return buildCollectionEnvelope(
      payload,
      primaryKey: 'users',
      items: users,
      totalOverride: total,
    );
  }

  static Map<String, dynamic>? _extractUserMap(dynamic payload) {
    Map<String, dynamic>? fallback;
    for (final source in _candidateSources(payload)) {
      fallback ??= source;
      if (_looksLikeUserPayload(source)) {
        return source;
      }
    }
    return fallback;
  }

  static Iterable<Map<String, dynamic>> _candidateSources(
      dynamic payload) sync* {
    final source = asMap(payload);
    if (source == null) return;

    yield source;

    for (final key in const ['user', 'profile', 'data']) {
      final nested = source[key];
      if (nested is Map) {
        yield* _candidateSources(nested);
      }
    }
  }

  static bool _looksLikeUserPayload(Map<String, dynamic> source) {
    for (final key in const [
      'user_id',
      'userId',
      'id',
      'nickname',
      'display_name',
      'displayName',
      'name',
      'avatar_url',
      'avatarUrl',
      'avatar',
      'bio',
      'signature',
      'intro',
    ]) {
      if (source[key] != null) {
        return true;
      }
    }
    return false;
  }

  static void _applyAlias(
    Map<String, dynamic> target,
    Map<String, dynamic> source, {
    required String canonicalKey,
    required List<String> aliasKeys,
  }) {
    final value = source[canonicalKey] ?? _firstValue(source, aliasKeys);
    if (value != null) {
      target[canonicalKey] = value;
    }
  }

  static void _mirrorCamelAlias(
    Map<String, dynamic> target,
    String sourceKey,
    String aliasKey,
  ) {
    final value = target[sourceKey];
    if (value != null) {
      target[aliasKey] = value;
    }
  }

  static dynamic _firstValue(Map<String, dynamic> source, List<String> keys) {
    for (final key in keys) {
      final value = source[key];
      if (value != null) return value;
    }
    return null;
  }

  static int? _toInt(dynamic value) {
    if (value is int) return value;
    if (value is num) return value.toInt();
    if (value is String) return int.tryParse(value);
    return null;
  }
}

/// 用户信息查询与管理服务
///
/// 提供面向当前用户和其他用户的查询能力：
/// - 用户搜索（关键词匹配，带 XSS 过滤）
/// - 用户详情与统计数据
/// - 情绪热力图 / 情绪日历（个人情绪可视化数据源）
/// - 收到的纸船列表
class UserService extends BaseService {
  @override
  String get serviceName => 'UserService';

  Map<String, dynamic> _normalizeEmotionTimeline(dynamic raw) {
    if (raw is! Map) {
      throw StateError('Emotion timeline payload is not a map');
    }

    final source = UserPayloadNormalizer.asMap(raw)!;
    final data = UserPayloadNormalizer.asMap(source['data']) ?? source;
    final rawDaysValue = data['days'] ??
        source['days'] ??
        data['timeline'] ??
        source['timeline'];
    if (rawDaysValue == null) {
      throw StateError('Emotion timeline payload is missing days/timeline');
    }

    final rawDays = UserPayloadNormalizer.asMap(rawDaysValue);
    if (rawDays == null) {
      throw StateError('Emotion timeline days payload is not a map');
    }

    final days = <String, dynamic>{};
    for (final entry in rawDays.entries) {
      final day = UserPayloadNormalizer.asMap(entry.value);
      if (day == null) continue;
      days[entry.key] = day;
    }

    return {
      'days': days,
      if (data['month'] != null || source['month'] != null)
        'month': data['month'] ?? source['month'],
      if (data['days_count'] != null || source['days_count'] != null)
        'days_count': data['days_count'] ?? source['days_count'],
      if (data['range_start'] != null || source['range_start'] != null)
        'range_start': data['range_start'] ?? source['range_start'],
      if (data['range_end'] != null || source['range_end'] != null)
        'range_end': data['range_end'] ?? source['range_end'],
    };
  }

  /// 搜索用户
  ///
  /// 关键词经过 sanitize 处理后提交，长度限制 1~50 字符。
  /// 返回 `{success, users, total}`。
  Future<Map<String, dynamic>> searchUsers(
    String query, {
    int page = 1,
    int pageSize = 20,
  }) async {
    InputValidator.requireLength(query, '想找的人', min: 1, max: 50);
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    query = InputValidator.sanitizeText(query);
    final response = await get('/users/search', queryParameters: {
      'q': query,
      'page': page,
      'page_size': pageSize,
    });

    if (!response.success) return toMap(response);

    final explicitTotal = UserPayloadNormalizer.extractExplicitTotal(response.data);
    if (explicitTotal == null) {
      throw StateError('搜索用户响应缺少 total');
    }

    final collection = UserPayloadNormalizer.buildUserCollection(response.data);

    return {
      ...toMap(response),
      ...collection,
      'data': {
        'users': collection['users'],
        'total': collection['total'],
        'items': collection['items'],
        'list': collection['list'],
        'page': collection['page'],
        'page_size': collection['page_size'],
        'has_more': collection['has_more'],
        'pagination': collection['pagination'],
      },
    };
  }

  /// 获取用户信息
  Future<Map<String, dynamic>> getUserInfo(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await get('/users/$userId');

    if (!response.success) return toMap(response);

    final user = UserPayloadNormalizer.normalizeUser(response.data);

    return {
      'success': true,
      'user': user,
      'data': user,
    };
  }

  /// 获取用户统计
  Future<Map<String, dynamic>> getUserStats(String userId) async {
    InputValidator.validateUUID(userId, '用户ID');
    final response = await get('/users/$userId/stats');
    if (!response.success) return toMap(response);
    return {'success': true, 'data': response.data};
  }

  /// 获取情绪热力图数据
  ///
  /// 返回当前用户近期的情绪分布热力图原始数据，
  /// 不走缓存以保证实时性。
  Future<Map<String, dynamic>> getEmotionHeatmap({int days = 365}) async {
    if (days < 1 || days > 365) {
      throw const ValidationException('热力图天数应在1-365之间');
    }
    final response = await get(
      '/users/my/emotion-heatmap',
      queryParameters: {'days': days},
      useCache: false,
    );
    if (!response.success) return toMap(response);
    return {
      'success': true,
      'data': _normalizeEmotionTimeline(response.data),
    };
  }

  /// 获取情绪日历数据
  ///
  /// 按年月查询当月每日的情绪记录，用于日历视图渲染。
  Future<Map<String, dynamic>> getEmotionCalendar(int year, int month) async {
    InputValidator.validateDateRange(year, month);
    final response = await get(
      '/users/my/emotion-calendar',
      queryParameters: {'year': year, 'month': month},
      useCache: false,
    );
    if (!response.success) return toMap(response);
    return {
      'success': true,
      'data': _normalizeEmotionTimeline(response.data),
    };
  }

  /// 获取我收到的纸船（分页）
  ///
  /// 纸船是其他用户对我的石头的匿名回应。
  Future<Map<String, dynamic>> getMyBoats(
      {int page = 1, int pageSize = 100}) async {
    InputValidator.requirePage(page);
    InputValidator.requirePageSize(pageSize);
    final response = await get('/users/my/boats',
        queryParameters: {'page': page, 'page_size': pageSize});
    if (!response.success) return toMap(response);

    final boats = extractNormalizedList(
      response.data,
      itemNormalizer: normalizeBoatPayload,
      listKeys: const ['boats', 'items'],
    );

    return {
      ...toMap(response),
      ...buildCollectionEnvelope(
        response.data,
        primaryKey: 'boats',
        items: boats,
        requireExplicitTotal: true,
      ),
    };
  }
}
