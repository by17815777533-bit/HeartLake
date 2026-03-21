// 石头状态管理
//
// 统一管理石头列表、投石、分页、缓存与 WebSocket 实时更新。
// 通过 WebSocket 监听新石头、涟漪、纸船、删除等事件自动同步列表数据。
// 依赖 [StoneDataSource]、[CacheStore] 和 [RoomSubscriptionClient] 完成交互。

library;

import 'package:flutter/foundation.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/cache_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/payload_contract.dart';

/// 石头列表状态管理器
///
/// 职责：
/// - 分页加载石头列表（下拉刷新 + 上拉加载更多）
/// - 投石（发布）、涟漪、纸船等写操作后乐观更新本地状态
/// - 通过 WebSocket 监听实时事件（新石头、涟漪、纸船、删除），
///   自动同步列表数据，无需手动刷新
/// - 本地缓存兜底，网络异常时展示上次成功数据
class StoneProvider with ChangeNotifier {
  final StoneDataSource _stoneService;
  final CacheStore _cache;
  final RoomSubscriptionClient _wsManager;

  // 石头列表状态
  List<Stone> _stones = [];
  final Map<String, int> _stoneIndexById = {};
  Map<String, dynamic>? _weather;
  bool _isLoading = false;
  bool _isLoadingMore = false;
  bool _hasMore = true;
  int _currentPage = 1;
  String? _errorMessage;
  int _lakeSubscribers = 0;

  // WebSocket 监听器引用，dispose 时逐个移除
  bool _wsRegistered = false;
  late void Function(Map<String, dynamic>) _onNewStone;
  late void Function(Map<String, dynamic>) _onBoatUpdate;
  late void Function(Map<String, dynamic>) _onRippleUpdate;
  late void Function(Map<String, dynamic>) _onStoneDeleted;
  late void Function(Map<String, dynamic>) _onBoatDeleted;
  late void Function(Map<String, dynamic>) _onRippleDeleted;
  late void Function(Map<String, dynamic>) _onReconnected;

  // Getter（只读访问器）
  List<Stone> get stones => List.unmodifiable(_stones);
  Map<String, dynamic>? get weather =>
      _weather == null ? null : Map<String, dynamic>.unmodifiable(_weather!);
  bool get isLoading => _isLoading;
  bool get isLoadingMore => _isLoadingMore;
  bool get hasMore => _hasMore;
  int get currentPage => _currentPage;
  String? get errorMessage => _errorMessage;

  /// 缓存 key 前缀，按分页存储
  static const String _cachePrefix = 'stones_';

  StoneProvider({
    StoneDataSource? stoneService,
    CacheStore? cache,
    RoomSubscriptionClient? wsManager,
  })  : _stoneService = stoneService ?? sl<StoneService>(),
        _cache = cache ?? CacheService(),
        _wsManager = wsManager ?? WebSocketManager() {
    _initWebSocketListeners();
  }

  bool _resolveHasMore(
    Map<String, dynamic> result, {
    required int page,
    required int itemCount,
  }) {
    final pagination = result['pagination'];
    if (pagination is Map<String, dynamic>) {
      final hasMore = pagination['has_more'] ?? pagination['hasMore'];
      if (hasMore is bool) {
        return hasMore;
      }
      final totalPages = pagination['total_pages'] ?? pagination['totalPages'];
      if (totalPages is num) {
        return page < totalPages.toInt();
      }
      final total = pagination['total'];
      final pageSize = pagination['page_size'] ?? pagination['pageSize'];
      if (total is num && pageSize is num && pageSize.toInt() > 0) {
        return page * pageSize.toInt() < total.toInt();
      }
    }

    final hasMore = result['has_more'] ?? result['hasMore'];
    if (hasMore is bool) {
      return hasMore;
    }

    return itemCount > 0;
  }

  void _rebuildStoneIndex() {
    _stoneIndexById.clear();
    for (var i = 0; i < _stones.length; i++) {
      _stoneIndexById[_stones[i].stoneId] = i;
    }
  }

  bool _removeStoneById(String stoneId) {
    final index = _stoneIndexById[stoneId];
    if (index == null) return false;
    _stones.removeAt(index);
    _rebuildStoneIndex();
    return true;
  }

  bool _updateStoneById(
    String stoneId,
    Stone Function(Stone stone) update,
  ) {
    final index = _stoneIndexById[stoneId];
    if (index == null) return false;
    _stones[index] = update(_stones[index]);
    return true;
  }

  // ==================== WebSocket 实时更新 ====================

  /// 注册 WebSocket 事件监听器，监听 8 种石头相关实时事件
  ///
  /// 使用 _wsRegistered 标志防止重复注册。
  /// WebSocket 重连时自动刷新列表以同步离线期间的变更。
  void _initWebSocketListeners() {
    if (_wsRegistered) return;
    _wsRegistered = true;

    _onNewStone = (Map<String, dynamic> data) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 收到 new_stone');
      }
      _handleNewStone(data);
    };

    _onBoatUpdate = (Map<String, dynamic> data) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 收到 boat_update');
      }
      _handleBoatUpdate(data);
    };

    _onRippleUpdate = (Map<String, dynamic> data) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 收到 ripple_update');
      }
      _handleRippleUpdate(data);
    };

    _onStoneDeleted = (Map<String, dynamic> data) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 收到 stone_deleted');
      }
      _handleStoneDeleted(data);
    };

    _onBoatDeleted = (data) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 收到 boat_deleted');
      }
      _handleBoatDeleted(data);
    };

    _onRippleDeleted = (data) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 收到 ripple_deleted');
      }
      _handleRippleDeleted(data);
    };

    _onReconnected = (data) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] WebSocket重连，刷新石头列表');
      }
      loadStones(refresh: true);
    };

    _wsManager.on('new_stone', _onNewStone);
    _wsManager.on('boat_update', _onBoatUpdate);
    _wsManager.on('new_boat', _onBoatUpdate);
    _wsManager.on('ripple_update', _onRippleUpdate);
    _wsManager.on('new_ripple', _onRippleUpdate);
    _wsManager.on('stone_deleted', _onStoneDeleted);
    _wsManager.on('boat_deleted', _onBoatDeleted);
    _wsManager.on('ripple_deleted', _onRippleDeleted);
    _wsManager.on('reconnected', _onReconnected);
  }

  /// 处理新石头推送，插入列表头部
  ///
  /// 通过 stoneId 去重，防止 WebSocket 重复推送导致列表出现重复项。
  void _handleNewStone(Map<String, dynamic> data) {
    final stoneData = extractStonePayload(data);
    final stoneId = stoneData['stone_id']?.toString() ?? '';

    // 防止重复
    if (stoneId.isEmpty || _stoneIndexById.containsKey(stoneId)) {
      return;
    }

    try {
      _stones.insert(0, Stone.fromJson(stoneData));
      _rebuildStoneIndex();
      _invalidateCache();
      notifyListeners();
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 忽略无法解析的 new_stone: $e');
      }
    }
  }

  /// 处理纸船数量更新，优先使用服务端返回的精确计数，否则 +1
  void _handleBoatUpdate(Map<String, dynamic> data) {
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final boatCount = extractBoatCount(data);
    if (_updateStoneById(stoneId, (stone) {
      return stone.copyWith(
        boatCount: boatCount ?? stone.boatCount + 1,
      );
    })) {
      notifyListeners();
    }
  }

  /// 处理涟漪数量更新，优先使用服务端返回的精确计数，否则 +1
  void _handleRippleUpdate(Map<String, dynamic> data) {
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final rippleCount = extractRippleCount(data);
    if (_updateStoneById(stoneId, (stone) {
      return stone.copyWith(
        rippleCount: rippleCount ?? stone.rippleCount + 1,
      );
    })) {
      notifyListeners();
    }
  }

  /// 处理石头删除事件，从列表中移除并清除缓存
  void _handleStoneDeleted(Map<String, dynamic> data) {
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    if (_removeStoneById(stoneId)) {
      _invalidateCache();
      notifyListeners();
    }
  }

  /// 处理纸船删除事件，优先使用服务端计数，否则 -1（下限为 0）
  void _handleBoatDeleted(Map<String, dynamic> data) {
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final boatCount = extractBoatCount(data);
    if (_updateStoneById(stoneId, (stone) {
      return stone.copyWith(
        boatCount: boatCount ?? (stone.boatCount > 0 ? stone.boatCount - 1 : 0),
      );
    })) {
      notifyListeners();
    }
  }

  /// 处理涟漪删除事件，优先使用服务端计数，否则 -1（下限为 0）
  void _handleRippleDeleted(Map<String, dynamic> data) {
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final rippleCount = extractRippleCount(data);
    if (_updateStoneById(stoneId, (stone) {
      return stone.copyWith(
        rippleCount:
            rippleCount ?? (stone.rippleCount > 0 ? stone.rippleCount - 1 : 0),
      );
    })) {
      notifyListeners();
    }
  }

  // ==================== 数据加载 ====================

  Future<void> activateLakeRealtime() async {
    _lakeSubscribers++;
    if (_lakeSubscribers > 1) {
      return;
    }
    await _wsManager.connect();
    _wsManager.joinRoom('lake');
  }

  void deactivateLakeRealtime() {
    if (_lakeSubscribers == 0) {
      return;
    }
    _lakeSubscribers--;
    if (_lakeSubscribers == 0) {
      _wsManager.leaveRoom('lake');
    }
  }

  Future<void> loadLakeWeather({bool force = false}) async {
    if (!force && _weather != null) {
      return;
    }

    try {
      final result = await _stoneService.getLakeWeather();
      if (result['success'] == true) {
        _weather = Map<String, dynamic>.from(
          (result['weather'] as Map?)?.cast<String, dynamic>() ?? const {},
        );
        notifyListeners();
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 加载湖面气象失败: $e');
      }
    }
  }

  Future<void> ensureLakeFeedLoaded({bool includeWeather = true}) async {
    final futures = <Future<void>>[];
    if (_stones.isEmpty && !_isLoading) {
      futures.add(loadStones());
    }
    if (includeWeather && _weather == null) {
      futures.add(loadLakeWeather());
    }
    if (futures.isEmpty) {
      return;
    }
    await Future.wait(futures);
  }

  Future<void> refreshFeed({bool includeWeather = true}) async {
    final futures = <Future<void>>[loadStones(refresh: true)];
    if (includeWeather) {
      futures.add(loadLakeWeather(force: true));
    }
    await Future.wait(futures);
  }

  /// 加载石头列表
  ///
  /// [refresh] 为 true 时从第一页重新加载，否则加载当前页。
  /// 非刷新模式下优先读取缓存，缓存命中则直接返回。
  Future<void> loadStones({bool refresh = false}) async {
    if (_isLoading) return;

    _isLoading = true;
    if (refresh) {
      _errorMessage = null;
      _hasMore = true;
    }
    notifyListeners();

    try {
      final page = refresh ? 1 : _currentPage;
      final cacheKey = '${_cachePrefix}page_$page';

      // 尝试读取缓存
      if (!refresh) {
        final cached = _cache.get<Map<String, dynamic>>(cacheKey);
        final cachedStones = (cached?['stones'] as List?)?.cast<Stone>();
        if (cached != null && cachedStones != null) {
          if (page == 1) {
            _stones = List<Stone>.from(cachedStones);
          } else {
            _stones.addAll(cachedStones);
          }
          _rebuildStoneIndex();
          _currentPage = page;
          _hasMore = _resolveHasMore(
            cached,
            page: page,
            itemCount: cachedStones.length,
          );
          _errorMessage = null;
          _isLoading = false;
          notifyListeners();
          return;
        }
      }

      final result = await _stoneService.getStones(page: page);

      if (result['success'] == true) {
        final newStones = (result['stones'] as List?)?.cast<Stone>() ?? [];

        if (refresh || page == 1) {
          _stones = newStones;
        } else {
          _stones.addAll(newStones);
        }
        _currentPage = page;
        _rebuildStoneIndex();
        _hasMore = _resolveHasMore(
          result,
          page: page,
          itemCount: newStones.length,
        );
        _errorMessage = null;

        // 写入缓存
        _cache.set<Map<String, dynamic>>(cacheKey, {
          ...result,
          'stones': newStones,
        });
      } else {
        throw Exception(result['message'] ?? '加载失败');
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 加载石头失败: $e');
      }
      _errorMessage = '网络连接失败，请检查后端服务是否启动';
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  /// 加载下一页石头（上拉加载更多）
  ///
  /// 已在加载中或没有更多数据时直接返回，防止重复请求。
  Future<void> loadMore() async {
    if (_isLoadingMore || !_hasMore) return;

    _isLoadingMore = true;
    notifyListeners();

    try {
      final page = _currentPage + 1;
      final result = await _stoneService.getStones(page: page);

      if (result['success'] == true) {
        final newStones = (result['stones'] as List?)?.cast<Stone>() ?? [];
        _hasMore = _resolveHasMore(
          result,
          page: page,
          itemCount: newStones.length,
        );

        if (newStones.isNotEmpty) {
          _stones.addAll(newStones);
          _currentPage = page;
          _rebuildStoneIndex();

          _cache.set<Map<String, dynamic>>('${_cachePrefix}page_$page', {
            ...result,
            'stones': newStones,
          });
        }
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 加载更多失败: $e');
      }
    } finally {
      _isLoadingMore = false;
      notifyListeners();
    }
  }

  // ==================== 投石 / 删石 ====================

  /// 投石（创建新石头）
  ///
  /// 发布成功后清除缓存，新石头会通过 WebSocket 广播自动添加到列表头部。
  Future<Map<String, dynamic>> throwStone({
    required String content,
    String stoneType = 'medium',
    String stoneColor = '#7A92A3',
    bool isAnonymous = true,
    String? moodType,
    List<String>? tags,
  }) async {
    try {
      final result = await _stoneService.createStone(
        content: content,
        stoneType: stoneType,
        stoneColor: stoneColor,
        isAnonymous: isAnonymous,
        moodType: moodType,
        tags: tags,
      );

      if (result['success'] == true) {
        _invalidateCache();
        // 新石头会通过 WebSocket 广播自动添加到列表
      }
      return result;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 投石失败: $e');
      }
      return {'success': false, 'message': '投石失败，请稍后再试'};
    }
  }

  /// 删除石头，成功后从本地列表移除并清除缓存
  Future<Map<String, dynamic>> deleteStone(String stoneId) async {
    try {
      final result = await _stoneService.deleteStone(stoneId);

      if (result['success'] == true) {
        _removeStoneById(stoneId);
        _invalidateCache();
        notifyListeners();
      }
      return result;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('[StoneProvider] 删除石头失败: $e');
      }
      return {'success': false, 'message': '删除失败，请稍后再试'};
    }
  }

  void applyRippleSuccess(String stoneId) {
    if (_updateStoneById(stoneId, (stone) {
      return stone.copyWith(rippleCount: stone.rippleCount + 1);
    })) {
      _invalidateCache();
      notifyListeners();
    }
  }

  void removeStoneLocally(String stoneId) {
    if (_removeStoneById(stoneId)) {
      _invalidateCache();
      notifyListeners();
    }
  }

  /// 获取石头详情
  Future<Map<String, dynamic>> getStoneDetail(String stoneId) async {
    return _stoneService.getStoneDetail(stoneId);
  }

  // ==================== 缓存管理 ====================

  /// 清除所有分页缓存（数据变更后调用）
  void _invalidateCache() {
    _cache.removeByPrefix(_cachePrefix);
  }

  /// 清空所有状态并重置分页（退出登录时调用）
  void clear() {
    _stones = [];
    _stoneIndexById.clear();
    _weather = null;
    _currentPage = 1;
    _hasMore = true;
    _errorMessage = null;
    _isLoading = false;
    _isLoadingMore = false;
    _invalidateCache();
    notifyListeners();
  }

  @override
  void dispose() {
    while (_lakeSubscribers > 0) {
      deactivateLakeRealtime();
    }
    _wsManager.off('new_stone', _onNewStone);
    _wsManager.off('boat_update', _onBoatUpdate);
    _wsManager.off('new_boat', _onBoatUpdate);
    _wsManager.off('ripple_update', _onRippleUpdate);
    _wsManager.off('new_ripple', _onRippleUpdate);
    _wsManager.off('stone_deleted', _onStoneDeleted);
    _wsManager.off('boat_deleted', _onBoatDeleted);
    _wsManager.off('ripple_deleted', _onRippleDeleted);
    _wsManager.off('reconnected', _onReconnected);
    _wsRegistered = false;
    super.dispose();
  }
}
