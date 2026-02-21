// @file lake_screen.dart
// @brief 观湖主页面
// Created by 林子怡

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/api_client.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../widgets/stone_card.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../widgets/status_view.dart';
import 'notification_screen.dart';
import 'stone_detail_screen.dart';
import 'lake_god_chat_screen.dart';
import 'emotion_trends_screen.dart';
import 'personalized_screen.dart';
import 'discover_screen.dart';
import '../providers/notification_provider.dart';
import '../../utils/app_theme.dart';

class LakeScreen extends StatefulWidget {
  const LakeScreen({super.key});

  @override
  State<LakeScreen> createState() => LakeScreenState();
}

class LakeScreenState extends State<LakeScreen> {
  final ApiClient _apiClient = ApiClient();
  final AIRecommendationService _aiService = AIRecommendationService();
  List<Stone> _stones = [];
  bool _isLoading = false;
  bool _isLoadingMore = false;
  int _currentPage = 1;
  String? _errorMessage;
  bool _hasMore = true;
  final ScrollController _scrollController = ScrollController();
  final WebSocketManager _wsManager = WebSocketManager();

  // 个性化推荐
  List<Map<String, dynamic>> _personalizedStones = [];
  bool _loadingPersonalized = true;

  // 监听器引用
  late void Function(Map<String, dynamic>) _newStoneListener;
  late void Function(Map<String, dynamic>) _boatUpdateListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _rippleUpdateListener;
  late void Function(Map<String, dynamic>) _rippleDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _disconnectedListener;
  late void Function(Map<String, dynamic>) _reconnectedListener;

  @override
  void initState() {
    super.initState();
    _loadStones();
    _loadPersonalizedRecommendations();
    _scrollController.addListener(_onScroll);
    _initWebSocket();
    _loadNotificationCount();
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
  }

  // 加载通知未读数
  void _loadNotificationCount() {
    Future.delayed(Duration.zero, () {
      if (!mounted) return;
      final notificationProvider =
          Provider.of<NotificationProvider>(context, listen: false);
      notificationProvider.loadUnreadCount();
    });
  }

  // 加载个性化推荐
  Future<void> _loadPersonalizedRecommendations() async {
    try {
      final results = await _aiService.getPersonalizedRecommendations(limit: 6);
      if (mounted) {
        setState(() {
          _personalizedStones = results;
          _loadingPersonalized = false;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() => _loadingPersonalized = false);
      }
    }
  }

  // 初始化 WebSocket 监听新石头
  Future<void> _initWebSocket() async {
    // 只使用 WebSocketManager（统一的WebSocket服务）
    await _wsManager.connect();

    // 监听新石头广播
    _newStoneListener = (data) {
      if (kDebugMode) { debugPrint('🆕 [LakeScreen] 收到 new_stone: $data'); }
      _handleNewStone(data);
    };
    _wsManager.on('new_stone', _newStoneListener);

    // 监听纸船更新（更新石头的boat_count）
    _boatUpdateListener = (data) {
      if (kDebugMode) { debugPrint('🚤 [LakeScreen] 收到 boat_update: $data'); }
      _handleBoatUpdate(data);
    };
    _wsManager.on('boat_update', _boatUpdateListener);
    _wsManager.on('new_boat', _boatUpdateListener);

    // 监听涟漪更新（更新石头的ripple_count）
    _rippleUpdateListener = (data) {
      if (kDebugMode) { debugPrint('🌊 [LakeScreen] 收到 ripple_update: $data'); }
      _handleRippleUpdate(data);
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);
    _wsManager.on('new_ripple', _rippleUpdateListener);

    // 监听石头删除
    _stoneDeletedListener = (data) {
      if (kDebugMode) { debugPrint('🗑️ [LakeScreen] 收到 stone_deleted: $data'); }
      _handleStoneDeleted(data);
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    // 监听纸船删除（减少石头的boat_count）
    _boatDeletedListener = (data) {
      if (kDebugMode) { debugPrint('🗑️ [LakeScreen] 收到 boat_deleted: $data'); }
      _handleBoatDeleted(data);
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);

    // 监听涟漪删除（减少石头的ripple_count）
    _rippleDeletedListener = (data) {
      if (kDebugMode) { debugPrint('🗑️ [LakeScreen] 收到 ripple_deleted: $data'); }
      _handleRippleDeleted(data);
    };
    _wsManager.on('ripple_deleted', _rippleDeletedListener);

    // 监听断开连接，尝试重连
    _disconnectedListener = (data) {
      if (kDebugMode) { debugPrint('WebSocket disconnected, will auto-reconnect'); }
    };
    _wsManager.on('disconnected', _disconnectedListener);

    // 监听重连成功，刷新数据
    _reconnectedListener = (data) {
      if (mounted) _loadStones(refresh: true);
    };
    _wsManager.on('reconnected', _reconnectedListener);
  }

  // 处理纸船更新
  void _handleBoatUpdate(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] ?? data['boat']?['stone_id'];
    if (stoneId == null) return;

    final boatCount = data['boat_count'];

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index >= 0) {
        _stones[index] = _stones[index].copyWith(
          boatCount: boatCount is int ? boatCount : _stones[index].boatCount + 1,
        );
      }
    });
  }

  // 处理涟漪更新
  void _handleRippleUpdate(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] ?? data['ripple']?['stone_id'];
    if (stoneId == null) return;

    final rippleCount = data['ripple_count'];

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index >= 0) {
        _stones[index] = _stones[index].copyWith(
          rippleCount: rippleCount is int ? rippleCount : _stones[index].rippleCount + 1,
        );
      }
    });
  }

  // 处理石头删除
  void _handleStoneDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] ?? data['stone']?['stone_id'];
    if (stoneId == null) return;

    setState(() {
      _stones.removeWhere((s) => s.stoneId == stoneId);
    });
  }

  // 处理纸船删除
  void _handleBoatDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] ?? data['boat']?['stone_id'];
    if (stoneId == null) return;

    final boatCount = data['boat_count'];

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index >= 0) {
        _stones[index] = _stones[index].copyWith(
          boatCount: boatCount is int
              ? boatCount
              : ((_stones[index].boatCount > 0) ? _stones[index].boatCount - 1 : 0),
        );
      }
    });
  }

  // 处理涟漪删除
  void _handleRippleDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] ?? data['ripple']?['stone_id'];
    if (stoneId == null) return;

    final rippleCount = data['ripple_count'];

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index >= 0) {
        _stones[index] = _stones[index].copyWith(
          rippleCount: rippleCount is int
              ? rippleCount
              : ((_stones[index].rippleCount > 0) ? _stones[index].rippleCount - 1 : 0),
        );
      }
    });
  }

  // 处理新石头广播（后端发送格式: {type: 'new_stone', stone: {...}}）
  void _handleNewStone(Map<String, dynamic> data) {
    if (!mounted) return;

    // 从 data['stone'] 中提取石头数据
    final stoneData = data['stone'] as Map<String, dynamic>? ?? data;
    final stoneId = stoneData['stone_id'] ?? '';

    // 防止重复添加
    if (_stones.any((s) => s.stoneId == stoneId)) {
      return;
    }

    setState(() {
      // 在列表顶部添加新石头
      final newStone = Stone(
        stoneId: stoneId,
        content: stoneData['content'] ?? '',
        userId: stoneData['user_id'] ?? '',
        stoneType: stoneData['stone_type'] ?? 'medium',
        stoneColor: stoneData['stone_color'] ?? '#7A92A3',
        moodType: stoneData['mood_type'], // 添加心情类型字段
        isAnonymous: stoneData['is_anonymous'] ?? true,
        rippleCount: 0,
        boatCount: 0,
        createdAt: DateTime.now(),
      );
      _stones.insert(0, newStone);
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

  // 公共方法，供外部调用刷新
  Future<void> refreshStones() async {
    await _loadStones(refresh: true);
  }

  @override
  void dispose() {
    _scrollController.dispose();
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

  void _onScroll() {
    if (_scrollController.position.pixels >=
            _scrollController.position.maxScrollExtent - 200 &&
        !_isLoadingMore &&
        _hasMore) {
      _loadMore();
    }
  }

  Future<void> _loadMore() async {
    if (_isLoadingMore || !_hasMore) return;

    setState(() {
      _isLoadingMore = true;
    });

    try {
      final page = _currentPage + 1;
      final response = await _apiClient.get(
        '/lake/stones',
        queryParameters: {'page': page, 'page_size': 20, 'sort': 'latest'},
      );

      if (response.statusCode == 200 && response.data['code'] == 0) {
        final items = response.data['data']?['stones'] as List? ?? [];
        final newStones = items.map((json) => Stone.fromJson(json)).toList();

        setState(() {
          if (newStones.isEmpty) {
            _hasMore = false;
          } else {
            _stones.addAll(newStones);
            _currentPage = page;
          }
          _isLoadingMore = false;
        });
      } else {
        setState(() => _isLoadingMore = false);
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('Error loading more stones: $e'); }
      setState(() => _isLoadingMore = false);
    }
  }

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
      final response = await _apiClient.get(
        '/lake/stones',
        queryParameters: {'page': page, 'page_size': 20, 'sort': 'latest'},
      );

      if (response.statusCode == 200 && response.data['code'] == 0) {
        final items = response.data['data']['stones'] as List? ?? [];
        final newStones = items.map((json) => Stone.fromJson(json)).toList();

        setState(() {
          if (refresh) {
            _stones = newStones;
            _currentPage = 1;
          } else {
            _stones.addAll(newStones);
          }
          _hasMore = newStones.isNotEmpty;
          _errorMessage = null;
        });
      } else {
        throw Exception(response.data['message'] ?? '加载失败');
      }
    } catch (e) {
      if (kDebugMode) { debugPrint('Error loading stones: $e'); }
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

  Future<void> _onRefresh() async {
    await _loadStones(refresh: true);
    // 刷新成功且没有错误时，触发震动
    if (_errorMessage == null) {
      HapticFeedback.lightImpact();
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showWater: true,
      showParticles: true,
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
          onPressed: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const LakeGodChatScreen())),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.explore_outlined),
            tooltip: '发现',
            onPressed: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const DiscoverScreen())),
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
      body: SafeArea(
        child: _buildContent(),
      ),
    );
  }

  Widget _buildContent() {
    // 1. 只有在没有数据且正在加载时，显示全屏Loading
    if (_stones.isEmpty && _isLoading) {
      return const StatusView(type: StatusType.loading, loadingMessage: '正在倾听湖面的声音...');
    }

    // 2. 只有在没有数据且出错时，显示错误状态
    if (_stones.isEmpty && _errorMessage != null) {
      return StatusView(
        type: StatusType.error,
        errorMessage: _errorMessage,
        onRetry: () => _loadStones(refresh: true),
      );
    }

    // 3. 只有在没有数据且不在加载、没出错时，显示空状态
    if (_stones.isEmpty) {
      return StatusView(
        type: StatusType.empty,
        onRetry: () => _loadStones(refresh: true),
      );
    }

    // 4. 有数据，显示列表（带下拉刷新）
    return RefreshIndicator(
      onRefresh: _onRefresh,
      color: AppTheme.warmOrange,
      backgroundColor: Colors.white.withValues(alpha: 0.9),
      child: ListView.builder(
        controller: _scrollController,
        padding: const EdgeInsets.only(
          top: 16,
          bottom: 20,
          left: 16,
          right: 16,
        ),
        physics: const AlwaysScrollableScrollPhysics(),
        itemCount: _stones.length + (_hasMore ? 1 : 0) + (_showRecommendationSection ? 1 : 0),
        itemBuilder: (context, index) {
          // 推荐区域作为第一个 item
          if (_showRecommendationSection && index == 0) {
            return Padding(
              padding: const EdgeInsets.only(bottom: 20),
              child: _buildPersonalizedSection(),
            );
          }

          final stoneIndex = _showRecommendationSection ? index - 1 : index;

          if (stoneIndex == _stones.length) {
            // 底部加载更多指示器
            return _isLoadingMore
                ? const Padding(
                    padding: EdgeInsets.all(16),
                    child: Center(
                        child: CircularProgressIndicator(color: AppTheme.warmOrange)),
                  )
                : const SizedBox(height: 50);
          }

          return Padding(
            padding: const EdgeInsets.only(bottom: 20),
            child: StoneCard(
              key: ValueKey(_stones[stoneIndex].stoneId),
              stone: _stones[stoneIndex],
              onRippleSuccess: () {
                _loadStones(refresh: true);
              },
              onDeleted: () {
                setState(() {
                  _stones.removeAt(stoneIndex);
                });
              },
            ),
          );
        },
      ),
    );
  }

  /// 是否显示推荐区域
  bool get _showRecommendationSection =>
      !_loadingPersonalized && _personalizedStones.isNotEmpty;

  /// 个性化推荐区域 - 光遇风格飘浮卡片
  Widget _buildPersonalizedSection() {
    return SkyGlassCard(
      padding: const EdgeInsets.all(14),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // 标题行
          Row(
            children: [
              const Icon(Icons.auto_awesome, size: 16, color: Colors.white70),
              const SizedBox(width: 6),
              const Text(
                '为你而来',
                style: TextStyle(
                  fontSize: 15,
                  fontWeight: FontWeight.w500,
                  color: Colors.white,
                  letterSpacing: 1,
                ),
              ),
              const Spacer(),
              GestureDetector(
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const PersonalizedScreen()),
                ),
                child: Text(
                  '查看更多',
                  style: TextStyle(
                    fontSize: 12,
                    color: Colors.white.withValues(alpha: 0.6),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          // 推荐石头横向滚动
          SizedBox(
            height: 100,
            child: ListView.separated(
              scrollDirection: Axis.horizontal,
              itemCount: _personalizedStones.length,
              separatorBuilder: (_, __) => const SizedBox(width: 10),
              itemBuilder: (context, index) {
                final item = _personalizedStones[index];
                return GestureDetector(
                  onTap: () {
                    final stoneId = item['stone_id'] ?? '';
                    if (stoneId.isNotEmpty) {
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (_) => StoneDetailScreen(stone: Stone.fromJson(item)),
                        ),
                      );
                    }
                  },
                  child: Container(
                    width: 140,
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(
                      color: Colors.white.withValues(alpha: 0.06),
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(
                        color: Colors.white.withValues(alpha: 0.08),
                      ),
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          item['content'] ?? '',
                          maxLines: 3,
                          overflow: TextOverflow.ellipsis,
                          style: TextStyle(
                            fontSize: 12,
                            color: Colors.white.withValues(alpha: 0.9),
                            height: 1.4,
                          ),
                        ),
                        const Spacer(),
                        Row(
                          children: [
                            Icon(Icons.favorite_border,
                                size: 12,
                                color: Colors.white.withValues(alpha: 0.4)),
                            const SizedBox(width: 4),
                            Text(
                              '${item['score']?.toStringAsFixed(0) ?? '0'}%匹配',
                              style: TextStyle(
                                fontSize: 10,
                                color: Colors.white.withValues(alpha: 0.4),
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),
                );
              },
            ),
          ),
          const SizedBox(height: 8),
          // 情绪星图入口
          SkyGlassCard(
            borderRadius: 12,
            blur: 8,
            enableGlow: false,
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
            onTap: () => Navigator.push(
              context,
              MaterialPageRoute(builder: (_) => const EmotionTrendsScreen()),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.bubble_chart, size: 16, color: Colors.amber.withValues(alpha: 0.7)),
                const SizedBox(width: 6),
                Text(
                  '情绪星图',
                  style: TextStyle(
                    fontSize: 12,
                    color: Colors.white.withValues(alpha: 0.7),
                    letterSpacing: 0.5,
                  ),
                ),
                const SizedBox(width: 4),
                Icon(Icons.arrow_forward_ios, size: 10, color: Colors.white.withValues(alpha: 0.4)),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
