// 观湖主页面
//
// 心湖核心页面，展示石头列表和湖面气象数据。

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../widgets/stone_card/stone_card.dart';
import '../widgets/water_background.dart';
import '../widgets/animations/ripple_effect.dart';
import '../widgets/animations/staggered_list.dart';
import '../widgets/status_view.dart';
import 'notification_screen.dart';
import 'lake_god_chat_screen.dart';
import 'discover_screen.dart';
import '../providers/notification_provider.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';

/// 观湖主页面 - 心湖的核心浏览界面
///
/// 以瀑布流形式展示湖中的石头，支持下拉刷新和上拉加载更多。
/// 背景使用水波纹动画营造湖面氛围，通过 WebSocket 实时接收新石头推送。
/// 状态管理采用 setState，通过 [StoneService] 获取数据，
/// [WebSocketManager] 监听石头/纸船/涟漪的增删事件并实时更新列表。
class LakeScreen extends StatefulWidget {
  const LakeScreen({super.key});

  @override
  State<LakeScreen> createState() => LakeScreenState();
}

/// 观湖页面的状态管理
///
/// 维护石头列表、分页状态和多个 WebSocket 监听器。
/// State 声明为 public（LakeScreenState）以便 HomeScreen 通过 GlobalKey 调用 [refreshStones]。
class LakeScreenState extends State<LakeScreen> {
  final StoneService _stoneService = sl<StoneService>();
  List<Stone> _stones = [];
  final Map<String, int> _stoneIndexById = {};
  bool _isLoading = false;
  bool _isLoadingMore = false;
  int _currentPage = 1;
  String? _errorMessage;
  bool _hasMore = true;
  final ScrollController _scrollController = ScrollController();
  final WebSocketManager _wsManager = WebSocketManager();

  // 监听器引用，dispose 时逐一移除防止内存泄漏
  late void Function(Map<String, dynamic>) _newStoneListener;
  late void Function(Map<String, dynamic>) _boatUpdateListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _rippleUpdateListener;
  late void Function(Map<String, dynamic>) _rippleDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _disconnectedListener;
  late void Function(Map<String, dynamic>) _reconnectedListener;

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

  @override
  void initState() {
    super.initState();
    _loadStones();
    _scrollController.addListener(_onScroll);
    _initWebSocket();
    _loadNotificationCount();
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
  }

  /// 延迟到下一帧加载通知未读数，避免在 initState 中直接访问 InheritedWidget
  void _loadNotificationCount() {
    Future.delayed(Duration.zero, () {
      if (!mounted) return;
      final notificationProvider =
          Provider.of<NotificationProvider>(context, listen: false);
      notificationProvider.loadUnreadCount();
    });
  }

  /// 初始化 WebSocket 连接并注册所有实时事件监听器
  ///
  /// 加入 lake 房间后监听：new_stone、boat_update、ripple_update、
  /// stone_deleted、boat_deleted、ripple_deleted、disconnected、reconnected
  Future<void> _initWebSocket() async {
    // 只使用 WebSocketManager（统一的WebSocket服务）
    await _wsManager.connect();

    // 加入 lake 房间，只接收观湖相关的实时消息
    _wsManager.joinRoom('lake');

    // 监听新石头广播
    _newStoneListener = (data) {
      if (kDebugMode) {
        debugPrint('🆕 [LakeScreen] 收到 new_stone: $data');
      }
      _handleNewStone(data);
    };
    _wsManager.on('new_stone', _newStoneListener);

    // 监听纸船更新（更新石头的boat_count）
    _boatUpdateListener = (data) {
      if (kDebugMode) {
        debugPrint('🚤 [LakeScreen] 收到 boat_update: $data');
      }
      _handleBoatUpdate(data);
    };
    _wsManager.on('boat_update', _boatUpdateListener);
    _wsManager.on('new_boat', _boatUpdateListener);

    // 监听涟漪更新（更新石头的ripple_count）
    _rippleUpdateListener = (data) {
      if (kDebugMode) {
        debugPrint('🌊 [LakeScreen] 收到 ripple_update: $data');
      }
      _handleRippleUpdate(data);
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);
    _wsManager.on('new_ripple', _rippleUpdateListener);

    // 监听石头删除
    _stoneDeletedListener = (data) {
      if (kDebugMode) {
        debugPrint('🗑️ [LakeScreen] 收到 stone_deleted: $data');
      }
      _handleStoneDeleted(data);
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    // 监听纸船删除（减少石头的boat_count）
    _boatDeletedListener = (data) {
      if (kDebugMode) {
        debugPrint('🗑️ [LakeScreen] 收到 boat_deleted: $data');
      }
      _handleBoatDeleted(data);
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);

    // 监听涟漪删除（减少石头的ripple_count）
    _rippleDeletedListener = (data) {
      if (kDebugMode) {
        debugPrint('🗑️ [LakeScreen] 收到 ripple_deleted: $data');
      }
      _handleRippleDeleted(data);
    };
    _wsManager.on('ripple_deleted', _rippleDeletedListener);

    // 监听断开连接，尝试重连
    _disconnectedListener = (data) {
      if (kDebugMode) {
        debugPrint('WebSocket disconnected, will auto-reconnect');
      }
    };
    _wsManager.on('disconnected', _disconnectedListener);

    // 监听重连成功，重新加入房间并刷新石头列表
    _reconnectedListener = (data) {
      if (kDebugMode) {
        debugPrint('🔄 [LakeScreen] WebSocket reconnected, refreshing stones');
      }
      _wsManager.joinRoom('lake');
      _loadStones(refresh: true);
    };
    _wsManager.on('reconnected', _reconnectedListener);
  }

  /// 处理纸船新增事件，更新对应石头的 boat_count
  void _handleBoatUpdate(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final boatCount = extractBoatCount(data);

    setState(() {
      _updateStoneById(stoneId, (stone) {
        return stone.copyWith(
          boatCount: boatCount ?? stone.boatCount + 1,
        );
      });
    });
  }

  /// 处理涟漪新增事件，更新对应石头的 ripple_count
  void _handleRippleUpdate(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final rippleCount = extractRippleCount(data);

    setState(() {
      _updateStoneById(stoneId, (stone) {
        return stone.copyWith(
          rippleCount: rippleCount ?? stone.rippleCount + 1,
        );
      });
    });
  }

  /// 处理石头删除事件，从本地列表中移除对应石头
  void _handleStoneDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    setState(() {
      _removeStoneById(stoneId);
    });
  }

  /// 处理纸船删除事件，递减对应石头的 boat_count
  void _handleBoatDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final boatCount = extractBoatCount(data);

    setState(() {
      _updateStoneById(stoneId, (stone) {
        return stone.copyWith(
          boatCount:
              boatCount ?? (stone.boatCount > 0 ? stone.boatCount - 1 : 0),
        );
      });
    });
  }

  /// 处理涟漪删除事件，递减对应石头的 ripple_count
  void _handleRippleDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = extractStoneEntityId(data);
    if (stoneId == null) return;

    final rippleCount = extractRippleCount(data);

    setState(() {
      _updateStoneById(stoneId, (stone) {
        return stone.copyWith(
          rippleCount: rippleCount ??
              (stone.rippleCount > 0 ? stone.rippleCount - 1 : 0),
        );
      });
    });
  }

  /// 处理新石头广播，解析数据并插入列表顶部，去重后显示 SnackBar 提示
  void _handleNewStone(Map<String, dynamic> data) {
    if (!mounted) return;

    final stoneData = extractStonePayload(data);
    final stoneId = stoneData['stone_id']?.toString() ?? '';

    // 防止重复添加
    if (stoneId.isEmpty || _stoneIndexById.containsKey(stoneId)) {
      return;
    }

    setState(() {
      _stones.insert(0, Stone.fromJson(stoneData));
      _rebuildStoneIndex();
    });

    // 显示提示
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('有新的石头投入心湖'),
        duration: Duration(seconds: 2),
        behavior: SnackBarBehavior.floating,
        backgroundColor: AppTheme.skyBlue,
      ),
    );
  }

  /// 公共方法，供 HomeScreen 通过 GlobalKey 调用以刷新石头列表
  Future<void> refreshStones() async {
    await _loadStones(refresh: true);
  }

  @override
  void dispose() {
    _scrollController.dispose();
    // 离开 lake 房间
    _wsManager.leaveRoom('lake');
    // 移除所有WebSocket监听器
    _wsManager.off('new_stone', _newStoneListener);
    _wsManager.off('boat_update', _boatUpdateListener);
    _wsManager.off('new_boat', _boatUpdateListener);
    _wsManager.off('ripple_update', _rippleUpdateListener);
    _wsManager.off('new_ripple', _rippleUpdateListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('ripple_deleted', _rippleDeletedListener);
    _wsManager.off('disconnected', _disconnectedListener);
    _wsManager.off('reconnected', _reconnectedListener);
    super.dispose();
  }

  /// 滚动监听回调，距离底部 200px 时触发加载更多
  void _onScroll() {
    if (_scrollController.position.pixels >=
            _scrollController.position.maxScrollExtent - 200 &&
        !_isLoadingMore &&
        _hasMore) {
      _loadMore();
    }
  }

  /// 加载下一页石头数据，追加到列表末尾
  Future<void> _loadMore() async {
    if (_isLoadingMore || !_hasMore) return;

    setState(() {
      _isLoadingMore = true;
    });

    try {
      final page = _currentPage + 1;
      final result = await _stoneService.getStones(page: page);

      if (result['success'] == true) {
        final newStones = (result['stones'] as List?)?.cast<Stone>() ?? [];

        setState(() {
          if (newStones.isEmpty) {
            _hasMore = false;
          } else {
            _stones.addAll(newStones);
            _currentPage = page;
            _rebuildStoneIndex();
          }
          _isLoadingMore = false;
        });
      } else {
        setState(() => _isLoadingMore = false);
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('Error loading more stones: $e');
      }
      setState(() => _isLoadingMore = false);
    }
  }

  /// 加载石头列表，[refresh] 为 true 时重置分页从第一页开始
  Future<void> _loadStones({bool refresh = false}) async {
    if (_isLoading) return;

    setState(() {
      _isLoading = true;
      if (refresh) {
        _errorMessage = null;
        _hasMore = true;
      }
    });

    try {
      final page = refresh ? 1 : _currentPage;
      final result = await _stoneService.getStones(page: page);

      if (result['success'] == true) {
        final newStones = (result['stones'] as List?)?.cast<Stone>() ?? [];

        setState(() {
          if (refresh) {
            _stones = newStones;
            _currentPage = 1;
          } else {
            _stones.addAll(newStones);
          }
          _rebuildStoneIndex();
          _hasMore = newStones.isNotEmpty;
          _errorMessage = null;
        });
      } else {
        throw Exception(result['message'] ?? '加载失败');
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('Error loading stones: $e');
      }
      if (mounted) {
        setState(() {
          _errorMessage = '网络连接失败，请检查后端服务是否启动';
        });
      }
    } finally {
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }

  /// 下拉刷新回调，刷新成功后触发轻触觉反馈
  Future<void> _onRefresh() async {
    await _loadStones(refresh: true);
    // 刷新成功且没有错误时，触发震动
    if (_errorMessage == null) {
      HapticFeedback.lightImpact();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text('心湖',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: Colors.white,
        leading: IconButton(
          icon: Container(
            width: 36,
            height: 36,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              gradient: LinearGradient(
                colors: [
                  Colors.cyan.shade400,
                  Colors.blue.shade600,
                ],
              ),
              boxShadow: [
                BoxShadow(
                  color: Colors.cyan.withValues(alpha: 0.4),
                  blurRadius: 8,
                  spreadRadius: 1,
                ),
              ],
            ),
            child: const Icon(Icons.water_drop, color: Colors.white, size: 20),
          ),
          tooltip: '湖神',
          onPressed: () => Navigator.push(context,
              MaterialPageRoute(builder: (_) => const LakeGodChatScreen())),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.explore_outlined),
            tooltip: '共鸣',
            onPressed: () => Navigator.push(context,
                MaterialPageRoute(builder: (_) => const DiscoverScreen())),
          ),
          // 通知按钮带红点
          Consumer<NotificationProvider>(
            builder: (context, notificationProvider, child) {
              return Stack(
                children: [
                  IconButton(
                    icon: const Icon(Icons.notifications_outlined),
                    onPressed: () async {
                      await Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => const NotificationScreen(),
                        ),
                      );
                      // 返回后刷新未读数
                      notificationProvider.loadUnreadCount();
                    },
                  ),
                  if (notificationProvider.hasUnread)
                    Positioned(
                      right: 8,
                      top: 8,
                      child: Container(
                        padding: const EdgeInsets.all(4),
                        decoration: BoxDecoration(
                          color: Colors.red,
                          shape: BoxShape.circle,
                          border: Border.all(color: Colors.white, width: 1.5),
                        ),
                        constraints: const BoxConstraints(
                          minWidth: 16,
                          minHeight: 16,
                        ),
                        child: Text(
                          notificationProvider.unreadCount > 99
                              ? '99+'
                              : notificationProvider.unreadCount.toString(),
                          style: const TextStyle(
                            color: Colors.white,
                            fontSize: 10,
                            fontWeight: FontWeight.bold,
                          ),
                          textAlign: TextAlign.center,
                        ),
                      ),
                    ),
                ],
              );
            },
          ),
        ],
      ),
      body: Stack(
        children: [
          // 动态水面背景
          const Positioned.fill(child: WaterBackground()),

          // 浮动粒子增强氛围
          Positioned.fill(
            child: IgnorePointer(
              child: FloatingParticles(
                count: 12,
                color: Colors.white.withValues(alpha: 0.6),
              ),
            ),
          ),

          // 内容区域
          _buildContent(),
        ],
      ),
    );
  }

  /// 根据数据状态构建内容区域：loading / error / empty / 石头列表
  Widget _buildContent() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    // 1. 只有在没有生据且正在加载时，显示全屏Loading
    if (_stones.isEmpty && _isLoading) {
      return const StatusView(
          type: StatusType.loading, loadingMessage: '正在倾听湖面的声音...');
    }

    // 2. 只有在没有数据且出错时，显示错状态
    if (_stones.isEmpty && _errorMessage != null) {
      return StatusView(
        type: StatusType.error,
        errorMessage: _errorMessage, // 可选：显示具体错误信息
        onRetry: () => _loadStones(refresh: true),
      );
    }

    // 3. 只有在没有数据且不在加载、没出错时，显示空状态
    if (_stones.isEmpty) {
      return StatusView(
        type: StatusType.empty,
        onRetry: () => _loadStones(refresh: true), // 空状态也可以点击刷新
      );
    }

    // 4. 有数据，显示列表（带下拉刷新）
    return RefreshIndicator(
      onRefresh: _onRefresh,
      color: Colors.blue[900],
      backgroundColor: isDark ? const Color(0xFF1B2838) : Colors.white,
      child: ListView.builder(
        controller: _scrollController,
        padding: EdgeInsets.only(
          top: MediaQuery.of(context).padding.top + kToolbarHeight + 16,
          bottom: 20,
          left: 16,
          right: 16,
        ),
        physics: const AlwaysScrollableScrollPhysics(),
        itemCount: _stones.length + (_hasMore ? 1 : 0),
        itemBuilder: (context, index) {
          if (index == _stones.length) {
            // 底部加载更多指示器
            return _isLoadingMore
                ? const Padding(
                    padding: EdgeInsets.all(16),
                    child: Center(
                        child: CircularProgressIndicator(color: Colors.white)),
                  )
                : const SizedBox(height: 50);
          }

          return StaggeredListItem(
            index: index,
            child: Padding(
              padding: const EdgeInsets.only(bottom: 20),
              child: StoneCard(
                key: ValueKey(_stones[index].stoneId),
                stone: _stones[index],
                onDeleted: () {
                  setState(() {
                    _removeStoneById(_stones[index].stoneId);
                  });
                },
              ),
            ),
          );
        },
      ),
    );
  }
}
