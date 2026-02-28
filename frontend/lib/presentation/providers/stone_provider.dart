/// 石头状态管理
///
/// 统一管理石头列表、投石、分页、缓存与 WebSocket 实时更新。
/// 通过 WebSocket 监听新石头、涟漪、纸船、删除等事件自动同步列表数据。
/// 依赖 [StoneService] 完成后端交互，依赖 [CacheService] 做分页缓存兜底。

library;

import 'package:flutter/foundation.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/cache_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';

/// 石头列表状态管理器
///
/// 职责：
/// - 分页加载石头列表（下拉刷新 + 上拉加载更多）
/// - 投石（发布）、涟漪、纸船等写操作后乐观更新本地状态
/// - 通过 WebSocket 监听实时事件（新石头、涟漪、纸船、删除），
///   自动同步列表数据，无需手动刷新
/// - 本地缓存兜底，网络异常时展示上次成功数据
class StoneProvider with ChangeNotifier {
  final StoneService _stoneService = sl<StoneService>();
  final CacheService _cache = CacheService();
  final WebSocketManager _wsManager = WebSocketManager();

  // 石头列表状态
  List<Stone> _stones = [];
  bool _isLoading = false;
  bool _isLoadingMore = false;
  bool _hasMore = true;
  int _currentPage = 1;
  String? _errorMessage;

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
  bool get isLoading => _isLoading;
  bool get isLoadingMore => _isLoadingMore;
  bool get hasMore => _hasMore;
  int get currentPage => _currentPage;
  String? get errorMessage => _errorMessage;

  /// 缓存 key 前缀，按分页存储
  static const String _cachePrefix = 'stones_';

  StoneProvider() {
    _initWebSocketListeners();
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
    final stoneData = data['stone'] as Map<String, dynamic>? ?? data;
    final stoneId = stoneData['stone_id'] ?? '';

    // 防止重复
    if (_stones.any((s) => s.stoneId == stoneId)) return;

    final newStone = Stone(
      stoneId: stoneId,
      content: stoneData['content'] ?? '',
      userId: stoneData['user_id'] ?? '',
      stoneType: stoneData['stone_type'] ?? 'medium',
      stoneColor: stoneData['stone_color'] ?? '#7A92A3',
      moodType: stoneData['mood_type'],
      isAnonymous:
          Stone.parseBool(stoneData['is_anonymous'], defaultValue: true),
      rippleCount: 0,
      boatCount: 0,
      createdAt: DateTime.now(),
    );
    _stones.insert(0, newStone);
    _invalidateCache();
    notifyListeners();
  }

  /// 处理纸船数量更新，优先使用服务端返回的精确计数，否则 +1
  void _handleBoatUpdate(Map<String, dynamic> data) {
    final stoneId = data['stone_id'] ?? data['boat']?['stone_id'];
    if (stoneId == null) return;

    final boatCount = data['boat_count'];
    final index = _stones.indexWhere((s) => s.stoneId == stoneId);
    if (index >= 0) {
      _stones[index] = _stones[index].copyWith(
        boatCount: boatCount is int ? boatCount : _stones[index].boatCount + 1,
      );
      notifyListeners();
    }
  }

  /// 处理涟漪数量更新，优先使用服务端返回的精确计数，否则 +1
  void _handleRippleUpdate(Map<String, dynamic> data) {
    final stoneId = data['stone_id'] ?? data['ripple']?['stone_id'];
    if (stoneId == null) return;

    final rippleCount = data['ripple_count'];
    final index = _stones.indexWhere((s) => s.stoneId == stoneId);
    if (index >= 0) {
      _stones[index] = _stones[index].copyWith(
        rippleCount:
            rippleCount is int ? rippleCount : _stones[index].rippleCount + 1,
      );
      notifyListeners();
    }
  }

  /// 处理石头删除事件，从列表中移除并清除缓存
  void _handleStoneDeleted(Map<String, dynamic> data) {
    final stoneId = data['stone_id'] ?? data['stone']?['stone_id'];
    if (stoneId == null) return;

    final removed = _stones.length;
    _stones.removeWhere((s) => s.stoneId == stoneId);
    if (_stones.length != removed) {
      _invalidateCache();
      notifyListeners();
    }
  }

  /// 处理纸船删除事件，优先使用服务端计数，否则 -1（下限为 0）
  void _handleBoatDeleted(Map<String, dynamic> data) {
    final stoneId = data['stone_id'] ?? data['boat']?['stone_id'];
    if (stoneId == null) return;

    final boatCount = data['boat_count'];
    final index = _stones.indexWhere((s) => s.stoneId == stoneId);
    if (index >= 0) {
      _stones[index] = _stones[index].copyWith(
        boatCount: boatCount is int
            ? boatCount
            : (_stones[index].boatCount > 0 ? _stones[index].boatCount - 1 : 0),
      );
      notifyListeners();
    }
  }

  /// 处理涟漪删除事件，优先使用服务端计数，否则 -1（下限为 0）
  void _handleRippleDeleted(Map<String, dynamic> data) {
    final stoneId = data['stone_id'] ?? data['ripple']?['stone_id'];
    if (stoneId == null) return;

    final rippleCount = data['ripple_count'];
    final index = _stones.indexWhere((s) => s.stoneId == stoneId);
    if (index >= 0) {
      _stones[index] = _stones[index].copyWith(
        rippleCount: rippleCount is int
            ? rippleCount
            : (_stones[index].rippleCount > 0
                ? _stones[index].rippleCount - 1
                : 0),
      );
      notifyListeners();
    }
  }

  // ==================== 数据加载 ====================

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

      // 尝试读取缓存
      if (!refresh) {
        final cached = _cache.get<List<Stone>>('${_cachePrefix}page_$page');
        if (cached != null) {
          _stones.addAll(cached);
          _isLoading = false;
          notifyListeners();
          return;
        }
      }

      final result = await _stoneService.getStones(page: page);

      if (result['success'] == true) {
        final newStones = (result['stones'] as List?)?.cast<Stone>() ?? [];

        if (refresh) {
          _stones = newStones;
          _currentPage = 1;
        } else {
          _stones.addAll(newStones);
        }
        _hasMore = newStones.isNotEmpty;
        _errorMessage = null;

        // 写入缓存
        _cache.set<List<Stone>>(
          '${_cachePrefix}page_${refresh ? 1 : page}',
          newStones,
        );
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

        if (newStones.isEmpty) {
          _hasMore = false;
        } else {
          _stones.addAll(newStones);
          _currentPage = page;

          _cache.set<List<Stone>>('${_cachePrefix}page_$page', newStones);
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
        _stones.removeWhere((s) => s.stoneId == stoneId);
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
