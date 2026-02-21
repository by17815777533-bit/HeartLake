// @file my_stones_screen.dart
// @brief 我的石头列表界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';
import '../../utils/app_theme.dart';
import '../widgets/stone_card.dart';
import '../widgets/sky_scaffold.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/websocket_manager.dart';

class MyStonesScreen extends StatefulWidget {
  const MyStonesScreen({super.key});

  @override
  State<MyStonesScreen> createState() => _MyStonesScreenState();
}

class _MyStonesScreenState extends State<MyStonesScreen> {
  final List<Stone> _myStones = [];
  bool _isLoading = false;
  final StoneService _stoneService = StoneService();
  final WebSocketManager _wsManager = WebSocketManager();

  // WebSocket 监听器
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _rippleUpdateListener;
  late void Function(Map<String, dynamic>) _rippleDeletedListener;
  late void Function(Map<String, dynamic>) _boatUpdateListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;

  @override
  void initState() {
    super.initState();
    _loadMyStones();
    _initWebSocket();
  }

  @override
  void dispose() {
    // 移除WebSocket监听器
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('ripple_update', _rippleUpdateListener);
    _wsManager.off('ripple_deleted', _rippleDeletedListener);
    _wsManager.off('boat_update', _boatUpdateListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    super.dispose();
  }

  void _initWebSocket() {
    // 监听石头删除
    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      if (stoneId == null) return;
      setState(() {
        _myStones.removeWhere((s) => s.stoneId == stoneId);
      });
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    // 监听涟漪更新 - 使用服务器实际总数和copyWith
    _rippleUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      final rippleCount = data['ripple_count'];
      if (stoneId == null || rippleCount is! int) return;
      setState(() {
        final index = _myStones.indexWhere((s) => s.stoneId == stoneId);
        if (index >= 0) {
          _myStones[index] = _myStones[index].copyWith(rippleCount: rippleCount);
        }
      });
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);

    // 监听涟漪删除
    _rippleDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      final rippleCount = data['ripple_count'];
      if (stoneId == null || rippleCount is! int) return;
      setState(() {
        final index = _myStones.indexWhere((s) => s.stoneId == stoneId);
        if (index >= 0) {
          _myStones[index] = _myStones[index].copyWith(rippleCount: rippleCount);
        }
      });
    };
    _wsManager.on('ripple_deleted', _rippleDeletedListener);

    // 监听纸船更新 - 使用服务器实际总数和copyWith
    _boatUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      final boatCount = data['boat_count'];
      if (stoneId == null || boatCount is! int) return;
      setState(() {
        final index = _myStones.indexWhere((s) => s.stoneId == stoneId);
        if (index >= 0) {
          _myStones[index] = _myStones[index].copyWith(boatCount: boatCount);
        }
      });
    };
    _wsManager.on('boat_update', _boatUpdateListener);

    // 监听纸船删除
    _boatDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      final boatCount = data['boat_count'];
      if (stoneId == null || boatCount is! int) return;
      setState(() {
        final index = _myStones.indexWhere((s) => s.stoneId == stoneId);
        if (index >= 0) {
          _myStones[index] = _myStones[index].copyWith(boatCount: boatCount);
        }
      });
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);
  }

  Future<void> _loadMyStones() async {
    setState(() => _isLoading = true);

    try {
      final result = await _stoneService.getMyStones(page: 1, pageSize: 100);

      if (result['success'] == true && mounted) {
        setState(() {
          _myStones.clear();
          _myStones.addAll(result['stones'] as List<Stone>);
          _isLoading = false;
        });
      } else {
        if (mounted) {
          setState(() => _isLoading = false);
        }
      }
    } catch (e) {
      debugPrint('Error loading my stones: $e');
      if (mounted) {
        setState(() => _isLoading = false);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      showWater: false,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: Colors.white,
        title: const Text(
          '我的石头',
          style: TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.w300,
            color: Colors.white,
            letterSpacing: 3,
          ),
        ),
        centerTitle: true,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh, color: Colors.white38, size: 20),
            onPressed: _loadMyStones,
          ),
        ],
      ),
      body: SafeArea(
        child: RefreshIndicator(
          color: AppTheme.candleGlow,
          backgroundColor: Colors.white.withValues(alpha: 0.1),
          onRefresh: _loadMyStones,
          child: _isLoading
              ? Center(
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      CircularProgressIndicator(
                        color: AppTheme.candleGlow,
                        strokeWidth: 2,
                      ),
                      const SizedBox(height: 16),
                      Text(
                        '正在寻找你的石头...',
                        style: TextStyle(
                          color: Colors.white.withValues(alpha: 0.6),
                          fontSize: 13,
                          letterSpacing: 1,
                        ),
                      ),
                    ],
                  ),
                )
              : _myStones.isEmpty
                  ? ListView(
                      children: [
                        SizedBox(
                          height: MediaQuery.of(context).size.height * 0.7,
                          child: Center(
                            child: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Icon(
                                  Icons.water_drop_outlined,
                                  size: 80,
                                  color: Colors.white.withValues(alpha: 0.3),
                                ),
                                const SizedBox(height: 16),
                                Text(
                                  '还没有投出石头，来投下你的第一颗吧',
                                  style: TextStyle(
                                    fontSize: 14,
                                    color: Colors.white.withValues(alpha: 0.6),
                                    letterSpacing: 1,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                      ],
                    )
                  : ListView.builder(
                      physics: const BouncingScrollPhysics(),
                      padding: const EdgeInsets.all(16),
                      itemCount: _myStones.length,
                      itemBuilder: (context, index) {
                        return Padding(
                          padding: const EdgeInsets.only(bottom: 16),
                          child: StoneCard(
                            stone: _myStones[index],
                            onDeleted: () {
                              // 删除后从列表中移除该石头
                              setState(() {
                                _myStones.removeAt(index);
                              });
                            },
                          ),
                        );
                      },
                    ),
        ),
      ),
    );
  }
}
