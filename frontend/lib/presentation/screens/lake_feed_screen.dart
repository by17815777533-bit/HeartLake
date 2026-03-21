// 心湖动态流界面
//
// 展示心湖社区的实时动态信息流。

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../widgets/stone_card/stone_card.dart';
import 'dart:async';
import '../providers/stone_provider.dart';

/// 心湖动态流页面，按时间线展示最新石头
///
/// 石头列表与实时同步统一交给 [StoneProvider]，页面只负责滚动和展示。
/// 同时展示湖面气象卡片，避免页面层重复持有 service 与 websocket 状态。
class LakeFeedScreen extends StatefulWidget {
  const LakeFeedScreen({super.key});

  @override
  State<LakeFeedScreen> createState() => _LakeFeedScreenState();
}

class _LakeFeedScreenState extends State<LakeFeedScreen> {
  final ScrollController _scrollController = ScrollController();
  late final StoneProvider _stoneProvider;

  @override
  void initState() {
    super.initState();
    _stoneProvider = context.read<StoneProvider>();
    _scrollController.addListener(_onScroll);
    Future.microtask(() async {
      if (!mounted) return;
      await _stoneProvider.activateLakeRealtime();
      await _stoneProvider.ensureLakeFeedLoaded(includeWeather: true);
    });
  }

  /// 并行加载湖面气象和石头列表
  Future<void> _loadData() async {
    await _stoneProvider.refreshFeed(includeWeather: true);
  }

  /// 滚动监听回调，距离底部 200px 时自动加载下一页
  void _onScroll() {
    if (_scrollController.position.pixels >=
        _scrollController.position.maxScrollExtent - 200) {
      if (_stoneProvider.hasMore && !_stoneProvider.isLoadingMore) {
        unawaited(_stoneProvider.loadMore());
      }
    }
  }

  @override
  void dispose() {
    _scrollController.dispose();
    _stoneProvider.deactivateLakeRealtime();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final stoneProvider = context.watch<StoneProvider>();
    final stones = stoneProvider.stones;
    final weather = stoneProvider.weather;
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: isDark
                ? [const Color(0xFF0D1B2E), const Color(0xFF1A1A2E)]
                : [const Color(0xFFE8F5E9), const Color(0xFFE3F2FD)],
          ),
        ),
        child: stoneProvider.isLoading && stones.isEmpty
            ? const Center(child: CircularProgressIndicator())
            : RefreshIndicator(
                onRefresh: () => _loadData(),
                child: CustomScrollView(
                  controller: _scrollController,
                  slivers: [
                    // 湖面气象
                    if (weather != null)
                      SliverToBoxAdapter(
                        child: Container(
                          margin: const EdgeInsets.all(15),
                          padding: const EdgeInsets.all(20),
                          decoration: BoxDecoration(
                            gradient: LinearGradient(
                              colors: [
                                isDark
                                    ? const Color(0xFF1B2838)
                                        .withValues(alpha: 0.9)
                                    : const Color(0xFF9DB2BF)
                                        .withValues(alpha: 0.8),
                                isDark
                                    ? const Color(0xFF0D1B2A)
                                    : const Color(0xFFDDE6ED),
                              ],
                            ),
                            borderRadius: BorderRadius.circular(20),
                            boxShadow: [
                              BoxShadow(
                                color: isDark
                                    ? Colors.transparent
                                    : Colors.black.withValues(alpha: 0.1),
                                blurRadius: 10,
                                offset: const Offset(0, 5),
                              ),
                            ],
                          ),
                          child: Row(
                            children: [
                              Icon(
                                Icons.cloud,
                                size: 40,
                                color: Colors.blue[300],
                              ),
                              const SizedBox(width: 15),
                              Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Text(
                                      weather['description'] ?? '湖面平静',
                                      style: TextStyle(
                                        fontSize: 16,
                                        fontWeight: FontWeight.bold,
                                        color: isDark
                                            ? const Color(0xFFE8EAED)
                                            : const Color(0xFF526D82),
                                      ),
                                    ),
                                    const SizedBox(height: 5),
                                    Text(
                                      '最近 ${weather['total_stones'] ?? 0} 颗石子',
                                      style: TextStyle(
                                        fontSize: 12,
                                        color: isDark
                                            ? const Color(0xFF9AA0A6)
                                            : const Color(0xFF9DB2BF),
                                      ),
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),

                    // 石头列表
                    SliverPadding(
                      padding: const EdgeInsets.symmetric(horizontal: 15),
                      sliver: SliverList(
                        delegate: SliverChildBuilderDelegate(
                          (context, index) {
                            if (index >= stones.length) return null;
                            final stone = stones[index];
                            return Padding(
                              padding: const EdgeInsets.only(bottom: 15),
                              child: StoneCard(
                                stone: stone,
                                onRippleSuccess: () {
                                  _stoneProvider.applyRippleSuccess(
                                    stone.stoneId,
                                  );
                                },
                                onDeleted: () {
                                  _stoneProvider.removeStoneLocally(
                                    stone.stoneId,
                                  );
                                },
                              ),
                            );
                          },
                          childCount: stones.length,
                        ),
                      ),
                    ),

                    // 加载更多指示器
                    if (stoneProvider.isLoadingMore)
                      const SliverToBoxAdapter(
                        child: Padding(
                          padding: EdgeInsets.all(20),
                          child: Center(child: CircularProgressIndicator()),
                        ),
                      ),
                  ],
                ),
              ),
      ),
    );
  }
}
