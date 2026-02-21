// @file lake_screen.dart
// @brief 观湖主页面
// Created by 林子怡

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../domain/entities/stone.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../data/datasources/stone_service.dart';
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
import '../../utils/animation_utils.dart';

class LakeScreen extends StatefulWidget {
  const LakeScreen({super.key});

  @override
  State<LakeScreen> createState() => LakeScreenState();
}

class LakeScreenState extends State<LakeScreen> {
  final StoneService _stoneService = StoneService();
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
      final result = await _aiService.getPersonalizedRecommendations(limit: 5);
      if (mounted) {
        setState(() {
          _personalizedStones = result;
          _loadingPersonalized = false;
        });
      }
    } catch (e) {
      if (kDebugMode) print('加载推荐失败: $e');
      if (mounted) {
        setState(() => _loadingPersonalized = false);
      }
    }
  }

  void _initWebSocket() {
    _wsManager.connect();

    _newStoneListener = (data) => _handleNewStone(data);
    _boatUpdateListener = (data) => _handleBoatUpdate(data);
    _boatDeletedListener = (data) => _handleBoatDeleted(data);
    _rippleUpdateListener = (data) => _handleRippleUpdate(data);
    _rippleDeletedListener = (data) => _handleRippleDeleted(data);
    _stoneDeletedListener = (data) => _handleStoneDeleted(data);
    _disconnectedListener = (_) {
      if (kDebugMode) print('WebSocket 断开');
    };
    _reconnectedListener = (_) {
      if (kDebugMode) print('WebSocket 重连成功');
      _loadStones(refresh: true);
    };

    _wsManager.on('new_stone', _newStoneListener);
    _wsManager.on('boat_update', _boatUpdateListener);
    _wsManager.on('boat_deleted', _boatDeletedListener);
    _wsManager.on('ripple_update', _rippleUpdateListener);
    _wsManager.on('ripple_deleted', _rippleDeletedListener);
    _wsManager.on('stone_deleted', _stoneDeletedListener);
    _wsManager.on('disconnected', _disconnectedListener);
    _wsManager.on('reconnected', _reconnectedListener);
  }

  void _handleBoatUpdate(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] as String?;
    final boatCount = data['boat_count'] as int?;
    if (stoneId == null || boatCount == null) return;

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index != -1) {
        _stones[index] = _stones[index].copyWith(boatCount: boatCount);
      }
    });
  }

  void _handleRippleUpdate(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] as String?;
    final rippleCount = data['ripple_count'] as int?;
    if (stoneId == null || rippleCount == null) return;

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index != -1) {
        _stones[index] = _stones[index].copyWith(rippleCount: rippleCount);
      }
    });
  }

  void _handleStoneDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] as String?;
    if (stoneId == null) return;

    setState(() {
      _stones.removeWhere((s) => s.stoneId == stoneId);
    });
  }

  void _handleBoatDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] as String?;
    final boatCount = data['boat_count'] as int?;
    if (stoneId == null) return;

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index != -1) {
        _stones[index] = _stones[index].copyWith(
          boatCount: boatCount ?? (_stones[index].boatCount - 1),
        );
      }
    });
  }

  void _handleRippleDeleted(Map<String, dynamic> data) {
    if (!mounted) return;
    final stoneId = data['stone_id'] as String?;
    final rippleCount = data['ripple_count'] as int?;
    if (stoneId == null) return;

    setState(() {
      final index = _stones.indexWhere((s) => s.stoneId == stoneId);
      if (index != -1) {
        _stones[index] = _stones[index].copyWith(
          rippleCount: rippleCount ?? (_stones[index].rippleCount - 1),
        );
      }
    });
  }

  void _handleNewStone(Map<String, dynamic> data) {
    if (!mounted) return;
    try {
      final stone = Stone.fromJson(data);
      setState(() {
        _stones.insert(0, stone);
      });
      // 显示新石头提示
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Text('有新的心事漂来了...'),
            backgroundColor: AppTheme.backgroundColor,
            behavior: SnackBarBehavior.floating,
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(12),
            ),
            duration: const Duration(seconds: 2),
            action: SnackBarAction(
              label: '查看',
              textColor: AppTheme.primaryLightColor,
              onPressed: () {
                _scrollController.animateTo(
                  0,
                  duration: const Duration(milliseconds: 300),
                  curve: Curves.easeOut,
                );
              },
            ),
          ),
        );
      }
    } catch (e) {
      if (kDebugMode) print('解析新石头失败: $e');
    }
  }

  void refreshStones() {
    _loadStones(refresh: true);
  }

  @override
  void dispose() {
    _scrollController.dispose();
    _wsManager.off('new_stone', _newStoneListener);
    _wsManager.off('boat_update', _boatUpdateListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('ripple_update', _rippleUpdateListener);
    _wsManager.off('ripple_deleted', _rippleDeletedListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('disconnected', _disconnectedListener);
    _wsManager.off('reconnected', _reconnectedListener);
    _wsManager.disconnect();
    _wsManager.dispose();
    super.dispose();
  }

  void _onScroll() {
    if (_scrollController.position.pixels >=
        _scrollController.position.maxScrollExtent - 200) {
      _loadMore();
    }
  }

  Future<void> _loadMore() async {
    if (_isLoadingMore || !_hasMore) return;

    setState(() {
      _isLoadingMore = true;
    });

    try {
      final nextPage = _currentPage + 1;
      final result = await _stoneService.getStones(page: nextPage, pageSize: 10);

      if (!mounted) return;

      if (result['success'] != true) {
        setState(() {
          _isLoadingMore = false;
        });
        return;
      }

      final newStones = (result['stones'] as List<Stone>? ?? <Stone>[]);
      final pagination = (result['pagination'] as Map<String, dynamic>? ?? <String, dynamic>{});

      setState(() {
        _stones.addAll(newStones);
        _currentPage = nextPage;
        _hasMore = pagination['has_more'] == true;
        _isLoadingMore = false;
      });
    } catch (e) {
      if (mounted) {
        setState(() {
          _isLoadingMore = false;
        });
      }
    }
  }

  Future<void> _loadStones({bool refresh = false}) async {
    if (_isLoading && !refresh) return;

    setState(() {
      _isLoading = true;
      _errorMessage = null;
      if (refresh) {
        _currentPage = 1;
        _hasMore = true;
      }
    });

    try {
      final page = refresh ? 1 : _currentPage;
      final result = await _stoneService.getStones(page: page, pageSize: 10);

      if (!mounted) return;

      if (result['success'] != true) {
        setState(() {
          _errorMessage = result['message']?.toString() ?? '加载湖面失败';
          _isLoading = false;
        });
        return;
      }

      final stones = (result['stones'] as List<Stone>? ?? <Stone>[]);
      final pagination = (result['pagination'] as Map<String, dynamic>? ?? <String, dynamic>{});

      setState(() {
        if (refresh) {
          _stones = stones;
        } else {
          _stones.addAll(stones);
        }
        _hasMore = pagination['has_more'] == true;
        _isLoading = false;
      });
    } catch (e) {
      if (mounted) {
        setState(() {
          _errorMessage = e.toString();
          _isLoading = false;
        });
      }
    }
  }

  Future<void> _onRefresh() async {
    await _loadStones(refresh: true);
    _loadPersonalizedRecommendations();
    if (mounted) {
      Provider.of<NotificationProvider>(context, listen: false).loadUnreadCount();
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showWater: true,
      showParticles: true,
      appBar: AppBar(
        title: ShaderMask(
          shaderCallback: (bounds) => const LinearGradient(
            colors: AppTheme.warmGradient,
          ).createShader(bounds),
          child: const Text('心湖',
              style: TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.bold,
              )),
        ),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: AppTheme.textPrimary,
        leading: IconButton(
          icon: Container(
            width: 36,
            height: 36,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              gradient: const LinearGradient(
                colors: [AppTheme.primaryLightColor, AppTheme.primaryColor],
              ),
              boxShadow: [
                BoxShadow(
                  color: AppTheme.primaryLightColor.withValues(alpha: 0.4),
                  blurRadius: 8,
                  spreadRadius: 1,
                ),
              ],
            ),
            child: const Icon(Icons.water_drop, color: AppTheme.darkTextPrimary, size: 20),
          ),
          tooltip: '湖神',
          onPressed: () => Navigator.push(context, SkyPageRoute(page: const LakeGodChatScreen())),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.explore_outlined),
            tooltip: '发现',
            onPressed: () => Navigator.push(context, SkyPageRoute(page: const DiscoverScreen())),
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
                        SkyPageRoute(page: const NotificationScreen()),
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
                          color: AppTheme.errorColor,
                          shape: BoxShape.circle,
                          border: Border.all(color: AppTheme.backgroundColor, width: 1.5),
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
                            color: AppTheme.textPrimary,
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
      color: AppTheme.primaryLightColor,
      backgroundColor: AppTheme.lightStone,
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
                        child: CircularProgressIndicator(color: AppTheme.primaryLightColor)),
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
              const Icon(Icons.auto_awesome, size: 16, color: AppTheme.candleGlow),
              const SizedBox(width: 6),
              const Text(
                '为你而来',
                style: TextStyle(
                  fontSize: 15,
                  fontWeight: FontWeight.w500,
                  color: AppTheme.primaryLightColor,
                  letterSpacing: 1,
                ),
              ),
              const Spacer(),
              GestureDetector(
                onTap: () => Navigator.push(
                  context,
                  SkyPageRoute(page: const PersonalizedScreen()),
                ),
                child: const Text(
                  '查看更多',
                  style: TextStyle(
                    fontSize: 12,
                    color: AppTheme.textSecondary,
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
                        SkyPageRoute(page: StoneDetailScreen(stone: Stone.fromJson(item))),
                      );
                    }
                  },
                  child: Container(
                    width: 140,
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(
                      color: AppTheme.lightStone.withValues(alpha: 0.5),
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(
                        color: AppTheme.primaryLightColor.withValues(alpha: 0.1),
                      ),
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          item['content'] ?? '',
                          maxLines: 3,
                          overflow: TextOverflow.ellipsis,
                          style: const TextStyle(
                            fontSize: 12,
                            color: AppTheme.textPrimary,
                            height: 1.4,
                          ),
                        ),
                        const Spacer(),
                        Row(
                          children: [
                            const Icon(Icons.favorite_border,
                                size: 12,
                                color: AppTheme.textSecondary),
                            const SizedBox(width: 4),
                            Text(
                              '${item['score']?.toStringAsFixed(0) ?? '0'}%匹配',
                              style: const TextStyle(
                                fontSize: 10,
                                color: AppTheme.textSecondary,
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
              SkyPageRoute(page: const EmotionTrendsScreen()),
            ),
            child: const Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.bubble_chart, size: 16, color: AppTheme.primaryLightColor),
                SizedBox(width: 6),
                Text(
                  '情绪星图',
                  style: TextStyle(
                    fontSize: 12,
                    color: AppTheme.textSecondary,
                    letterSpacing: 0.5,
                  ),
                ),
                SizedBox(width: 4),
                Icon(Icons.arrow_forward_ios, size: 10, color: AppTheme.textSecondary),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
