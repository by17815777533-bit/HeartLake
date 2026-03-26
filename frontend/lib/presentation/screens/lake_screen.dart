// 观湖主页面
//
// 心湖核心页面，展示石头列表和湖面气象数据。

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../widgets/stone_card/stone_card.dart';
import '../widgets/water_background.dart';
import '../widgets/animations/ripple_effect.dart';
import '../widgets/animations/staggered_list.dart';
import '../widgets/status_view.dart';
import 'notification_screen.dart';
import 'lake_god_chat_screen.dart';
import 'discover_screen.dart';
import '../providers/notification_provider.dart';
import '../providers/stone_provider.dart';

/// 观湖主页面 - 心湖的核心浏览界面
///
/// 以瀑布流形式展示湖中的石头，支持下拉刷新和上拉加载更多。
/// 背景使用水波纹动画营造湖面氛围。
/// 运行态数据统一由 [StoneProvider] 承载，页面只负责展示和交互分发。
class LakeScreen extends StatefulWidget {
  const LakeScreen({super.key});

  @override
  State<LakeScreen> createState() => LakeScreenState();
}

/// 观湖页面的状态管理
///
/// 维护滚动、刷新与通知入口。
/// State 声明为 public（LakeScreenState）以便 HomeScreen 通过 GlobalKey 调用 [refreshStones]。
class LakeScreenState extends State<LakeScreen> {
  final ScrollController _scrollController = ScrollController();
  late final StoneProvider _stoneProvider;

  @override
  void initState() {
    super.initState();
    _stoneProvider = context.read<StoneProvider>();
    _scrollController.addListener(_onScroll);
    _loadNotificationCount();
    Future.microtask(() async {
      if (!mounted) return;
      await _stoneProvider.activateLakeRealtime();
      await _stoneProvider.ensureLakeFeedLoaded(includeWeather: false);
    });
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

  /// 公共方法，供 HomeScreen 通过 GlobalKey 调用以刷新石头列表
  Future<void> refreshStones() async {
    await _stoneProvider.refreshFeed(includeWeather: false);
  }

  @override
  void dispose() {
    _scrollController.dispose();
    _stoneProvider.deactivateLakeRealtime();
    super.dispose();
  }

  /// 滚动监听回调，距离底部 200px 时触发加载更多
  void _onScroll() {
    if (_scrollController.position.pixels >=
            _scrollController.position.maxScrollExtent - 200 &&
        !_stoneProvider.isLoadingMore &&
        _stoneProvider.hasMore) {
      _stoneProvider.loadMore();
    }
  }

  /// 下拉刷新回调，刷新成功后触发轻触觉反馈
  Future<void> _onRefresh() async {
    await _stoneProvider.refreshFeed(includeWeather: false);
    // 刷新成功且没有错误时，触发震动
    if (mounted && _stoneProvider.errorMessage == null) {
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
    final stoneProvider = context.watch<StoneProvider>();
    final stones = stoneProvider.stones;
    // 1. 只有在没有生据且正在加载时，显示全屏Loading
    if (stones.isEmpty && stoneProvider.isLoading) {
      return const StatusView(
          type: StatusType.loading, loadingMessage: '正在倾听湖面的声音...');
    }

    // 2. 只有在没有数据且出错时，显示错状态
    if (stones.isEmpty && stoneProvider.errorMessage != null) {
      return StatusView(
        type: StatusType.error,
        errorMessage: stoneProvider.errorMessage, // 可选：显示具体错误信息
        onRetry: () => stoneProvider.refreshFeed(includeWeather: false),
      );
    }

    // 3. 只有在没有数据且不在加载、没出错时，显示空状态
    if (stones.isEmpty) {
      return StatusView(
        type: StatusType.empty,
        onRetry: () => stoneProvider.refreshFeed(includeWeather: false),
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
        itemCount: stones.length + (stoneProvider.hasMore ? 1 : 0),
        itemBuilder: (context, index) {
          if (index == stones.length) {
            // 底部加载更多指示器
            return stoneProvider.isLoadingMore
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
                key: ValueKey(stones[index].stoneId),
                stone: stones[index],
                onInteractionCountsChanged: (rippleCount, boatCount) {
                  context.read<StoneProvider>().applyServerInteractionCounts(
                        stones[index].stoneId,
                        rippleCount: rippleCount,
                        boatCount: boatCount,
                      );
                },
                onDeleted: () {
                  context
                      .read<StoneProvider>()
                      .removeStoneLocally(stones[index].stoneId);
                },
              ),
            ),
          );
        },
      ),
    );
  }
}
