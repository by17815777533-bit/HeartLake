// @file lake_feed_screen.dart
// @brief 心湖动态流界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card.dart';
import 'dart:async';

class LakeFeedScreen extends StatefulWidget {
  const LakeFeedScreen({super.key});

  @override
  State<LakeFeedScreen> createState() => _LakeFeedScreenState();
}

class _LakeFeedScreenState extends State<LakeFeedScreen> {
  final StoneService _stoneService = StoneService();
  final ScrollController _scrollController = ScrollController();
  final WebSocketManager _wsManager = WebSocketManager();

  List<Stone> _stones = [];
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

  void _initWebSocket() {
    _wsManager.connect();

    _newStoneListener = (data) {
      if (!mounted) return;
      final stoneData = data['stone'] as Map<String, dynamic>? ?? data;
      final stoneId = stoneData['stone_id'] ?? '';
      if (_stones.any((s) => s.stoneId == stoneId)) return;
      setState(() {
        _stones.insert(0, Stone(
          stoneId: stoneId,
          content: stoneData['content'] ?? '',
          userId: stoneData['user_id'] ?? '',
          stoneType: stoneData['stone_type'] ?? 'medium',
          stoneColor: stoneData['stone_color'] ?? '#7A92A3',
          moodType: stoneData['mood_type'],
          isAnonymous: stoneData['is_anonymous'] ?? true,
          createdAt: DateTime.now(),
        ));
      });
    };
    _wsManager.on('new_stone', _newStoneListener);

    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      if (stoneId == null) return;
      setState(() => _stones.removeWhere((s) => s.stoneId == stoneId));
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    _rippleUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      final count = data['ripple_count'];
      if (stoneId == null || count is! int) return;
      setState(() {
        final idx = _stones.indexWhere((s) => s.stoneId == stoneId);
        if (idx >= 0) _stones[idx] = _stones[idx].copyWith(rippleCount: count);
      });
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);

    _boatUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      final count = data['boat_count'];
      if (stoneId == null || count is! int) return;
      setState(() {
        final idx = _stones.indexWhere((s) => s.stoneId == stoneId);
        if (idx >= 0) _stones[idx] = _stones[idx].copyWith(boatCount: count);
      });
    };
    _wsManager.on('boat_update', _boatUpdateListener);
  }

  Future<void> _loadData() async {
    await Future.wait([
      _loadWeather(),
      _loadStones(),
    ]);
  }

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
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFFE8F5E9), Color(0xFFE3F2FD)],
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
                              const Color(0xFF9DB2BF).withValues(alpha: 0.8),
                              const Color(0xFFDDE6ED),
                            ],
                          ),
                          borderRadius: BorderRadius.circular(20),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withValues(alpha: 0.1),
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
                                    style: const TextStyle(
                                      fontSize: 16,
                                      fontWeight: FontWeight.bold,
                                      color: Color(0xFF526D82),
                                    ),
                                  ),
                                  const SizedBox(height: 5),
                                  Text(
                                    '最近 ${_weather!['total_stones'] ?? 0} 颗石子',
                                    style: const TextStyle(
                                      fontSize: 12,
                                      color: Color(0xFF9DB2BF),
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
                                  final idx = _stones.indexWhere(
                                      (s) => s.stoneId == stone.stoneId);
                                  if (idx != -1) {
                                    _stones[idx] = Stone.fromJson({
                                      ..._stones[idx].toJson(),
                                      'ripple_count':
                                          _stones[idx].rippleCount + 1,
                                    });
                                  }
                                });
                              },
                              onDeleted: () {
                                // 删除后从列表中移除
                                setState(() {
                                  _stones.removeWhere(
                                      (s) => s.stoneId == stone.stoneId);
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
