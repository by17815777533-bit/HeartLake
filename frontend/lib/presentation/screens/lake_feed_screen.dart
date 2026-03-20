// 心湖动态流界面
//
// 展示心湖社区的实时动态信息流。

import 'package:flutter/material.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../domain/entities/stone.dart';
import '../../utils/payload_contract.dart';
import '../widgets/stone_card/stone_card.dart';
import 'dart:async';

/// 心湖动态流页面，按时间线展示最新石头
///
/// 支持无限滚动分页加载，通过 WebSocket 实时接收新石头并插入列表顶部。
/// 同时展示湖面气象卡片，涟漪/纸船计数通过 WebSocket 实时同步。
class LakeFeedScreen extends StatefulWidget {
  const LakeFeedScreen({super.key});

  @override
  State<LakeFeedScreen> createState() => _LakeFeedScreenState();
}

class _LakeFeedScreenState extends State<LakeFeedScreen> {
  final StoneService _stoneService = sl<StoneService>();
  final ScrollController _scrollController = ScrollController();
  final WebSocketManager _wsManager = WebSocketManager();

  List<Stone> _stones = [];
  final Map<String, int> _stoneIndexById = {};
  Map<String, dynamic>? _weather;
  bool _isLoading = true;
  int _currentPage = 1;
  bool _hasMore = true;

  // WebSocket 监听器引用
  late void Function(Map<String, dynamic>) _newStoneListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _rippleUpdateListener;
  late void Function(Map<String, dynamic>) _boatUpdateListener;

  @override
  void initState() {
    super.initState();
    _loadData();
    _scrollController.addListener(_onScroll);
    _initWebSocket();
  }

  /// 注册 WebSocket 监听器，处理新石头、石头删除、涟漪和纸船计数更新
  void _initWebSocket() {
    _wsManager.connect();

    _newStoneListener = (data) {
      if (!mounted) return;
      final stoneData = extractStonePayload(data);
      final stoneId = stoneData['stone_id']?.toString() ?? '';
      if (stoneId.isEmpty || _stoneIndexById.containsKey(stoneId)) return;
      setState(() {
        _stones.insert(0, Stone.fromJson(stoneData));
        _rebuildStoneIndex();
      });
    };
    _wsManager.on('new_stone', _newStoneListener);

    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      if (stoneId == null) return;
      setState(() => _removeStoneById(stoneId));
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    _rippleUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      final count = extractRippleCount(data);
      if (stoneId == null || count == null) return;
      setState(() {
        final idx = _stoneIndexById[stoneId];
        if (idx != null) {
          _stones[idx] = _stones[idx].copyWith(rippleCount: count);
        }
      });
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);

    _boatUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      final count = extractBoatCount(data);
      if (stoneId == null || count == null) return;
      setState(() {
        final idx = _stoneIndexById[stoneId];
        if (idx != null) {
          _stones[idx] = _stones[idx].copyWith(boatCount: count);
        }
      });
    };
    _wsManager.on('boat_update', _boatUpdateListener);
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

  /// 并行加载湖面气象和石头列表
  Future<void> _loadData() async {
    await Future.wait([
      _loadWeather(),
      _loadStones(),
    ]);
  }

  /// 加载湖面气象数据（描述文案 + 石头总数）
  Future<void> _loadWeather() async {
    try {
      final result = await _stoneService.getLakeWeather();
      if (result['success'] && mounted) {
        setState(() {
          _weather = result['weather'];
        });
      }
    } catch (_) {}
  }

  /// 加载石头列表，[refresh] 为 true 时重置分页
  Future<void> _loadStones({bool refresh = false}) async {
    if (refresh) {
      _currentPage = 1;
      _hasMore = true;
    }

    try {
      final result = await _stoneService.getStones(page: _currentPage);

      if (mounted) {
        setState(() {
          _isLoading = false;
          if (result['success']) {
            if (refresh) {
              _stones = result['stones'];
            } else {
              _stones.addAll(result['stones']);
            }
            _rebuildStoneIndex();
            _hasMore = result['pagination']?['has_more'] ?? false;
          }
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载失败，请下拉重试')),
        );
      }
    }
  }

  /// 滚动监听回调，距离底部 200px 时自动加载下一页
  void _onScroll() {
    if (_scrollController.position.pixels >=
        _scrollController.position.maxScrollExtent - 200) {
      if (_hasMore && !_isLoading) {
        setState(() {
          _currentPage++;
          _isLoading = true;
        });
        _loadStones();
      }
    }
  }

  @override
  void dispose() {
    _scrollController.dispose();
    _wsManager.off('new_stone', _newStoneListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('ripple_update', _rippleUpdateListener);
    _wsManager.off('boat_update', _boatUpdateListener);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
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
        child: _isLoading && _stones.isEmpty
            ? const Center(child: CircularProgressIndicator())
            : RefreshIndicator(
                onRefresh: () => _loadData(),
                child: CustomScrollView(
                  controller: _scrollController,
                  slivers: [
                    // 湖面气象
                    if (_weather != null)
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
                                      _weather!['description'] ?? '湖面平静',
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
                                      '最近 ${_weather!['total_stones'] ?? 0} 颗石子',
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
                            if (index >= _stones.length) return null;
                            final stone = _stones[index];
                            return Padding(
                              padding: const EdgeInsets.only(bottom: 15),
                              child: StoneCard(
                                stone: stone,
                                onRippleSuccess: () {
                                  // 涉漪成功后更新本地计数
                                  setState(() {
                                    _updateStoneById(
                                      stone.stoneId,
                                      (currentStone) => currentStone.copyWith(
                                        rippleCount:
                                            currentStone.rippleCount + 1,
                                      ),
                                    );
                                  });
                                },
                                onDeleted: () {
                                  // 删除后从列表中移除
                                  setState(() {
                                    _removeStoneById(stone.stoneId);
                                  });
                                },
                              ),
                            );
                          },
                          childCount: _stones.length,
                        ),
                      ),
                    ),

                    // 加载更多指示器
                    if (_isLoading)
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
