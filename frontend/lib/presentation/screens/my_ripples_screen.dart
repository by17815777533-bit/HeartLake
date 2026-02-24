// @file my_ripples_screen.dart
// @brief 我的涟漪列表界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card/stone_card.dart';
import '../../data/datasources/interaction_service.dart';
import '../../data/datasources/websocket_manager.dart';

class MyRipplesScreen extends StatefulWidget {
  const MyRipplesScreen({super.key});

  @override
  State<MyRipplesScreen> createState() => _MyRipplesScreenState();
}

class _MyRipplesScreenState extends State<MyRipplesScreen> {
  final List<Map<String, dynamic>> _ripplesData = [];
  final List<Stone> _ripples = [];
  bool _isLoading = false;
  final InteractionService _interactionService = InteractionService();
  final WebSocketManager _wsManager = WebSocketManager();

  // WebSocket 监听器引用
  late void Function(Map<String, dynamic>) _rippleDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _rippleUpdateListener;

  @override
  void initState() {
    super.initState();
    _loadMyRipples();
    _initWebSocket();
  }

  @override
  void dispose() {
    // 移除WebSocket监听器
    _wsManager.off('ripple_deleted', _rippleDeletedListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('ripple_update', _rippleUpdateListener);
    super.dispose();
  }

  void _initWebSocket() {
    // 监听涟漪删除 - 从列表中移除对应条目
    _rippleDeletedListener = (data) {
      if (!mounted) return;
      final rippleId = data['ripple_id'];
      if (rippleId == null) return;
      setState(() {
        final index = _ripplesData.indexWhere((r) => r['ripple_id'] == rippleId);
        if (index >= 0) {
          _ripplesData.removeAt(index);
          _ripples.removeAt(index);
        }
      });
    };
    _wsManager.on('ripple_deleted', _rippleDeletedListener);

    // 监听石头删除 - 移除该石头相关的涟漪
    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      if (stoneId == null) return;
      setState(() {
        final indicesToRemove = <int>[];
        for (int i = 0; i < _ripplesData.length; i++) {
          if (_ripplesData[i]['stone_id'] == stoneId) {
            indicesToRemove.add(i);
          }
        }
        // 从后往前删除，避免索引偏移
        for (int i = indicesToRemove.length - 1; i >= 0; i--) {
          final idx = indicesToRemove[i];
          _ripplesData.removeAt(idx);
          _ripples.removeAt(idx);
        }
      });
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    // 监听涟漪更新 - 更新石头的涟漪计数
    _rippleUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      final rippleCount = data['ripple_count'];
      if (stoneId == null || rippleCount is! int) return;
      setState(() {
        for (int i = 0; i < _ripples.length; i++) {
          if (_ripples[i].stoneId == stoneId) {
            _ripples[i] = _ripples[i].copyWith(rippleCount: rippleCount);
          }
        }
      });
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);
  }

  Future<void> _loadMyRipples() async {
    if (mounted) setState(() => _isLoading = true);

    try {
      final result = await _interactionService.getMyRipples(
        page: 1,
        pageSize: 50,
      );

      if (result['success'] == true && mounted) {
        final items = result['ripples'] as List;
        setState(() {
          _ripplesData.clear();
          _ripplesData
              .addAll(items.map((json) => Map<String, dynamic>.from(json)));
          _ripples.clear();
          _ripples.addAll(items.map((json) => Stone.fromJson(json)));
          _isLoading = false;
        });
      } else {
        if (mounted) {
          setState(() => _isLoading = false);
        }
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

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('我的涟漪'),
        centerTitle: true,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loadMyRipples,
          ),
        ],
      ),
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFFF0F8FF), Color(0xFFE8F0FA)],
          ),
        ),
        child: RefreshIndicator(
          onRefresh: _loadMyRipples,
          child: _isLoading
            ? const Center(child: CircularProgressIndicator())
            : _ripples.isEmpty
                ? ListView(
                    children: [
                      SizedBox(
                        height: MediaQuery.of(context).size.height * 0.7,
                        child: Center(
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              Icon(
                                Icons.waves,
                                size: 80,
                                color: Colors.grey[300],
                              ),
                              const SizedBox(height: 16),
                              Text(
                                '还没有涟漪，你的声音会被听见',
                                style: TextStyle(
                                  fontSize: 16,
                                  color: Colors.grey[600],
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ],
                  )
                : ListView.builder(
                    padding: const EdgeInsets.all(16),
                    itemCount: _ripples.length,
                    itemBuilder: (context, index) {
                      final rippleId =
                          _ripplesData[index]['ripple_id']?.toString() ?? '';
                      return Dismissible(
                        key: Key(rippleId),
                        direction: DismissDirection.endToStart,
                        background: Container(
                          alignment: Alignment.centerRight,
                          padding: const EdgeInsets.only(right: 20),
                          decoration: BoxDecoration(
                            color: Colors.red,
                            borderRadius: BorderRadius.circular(24),
                          ),
                          child: const Icon(Icons.undo, color: Colors.white),
                        ),
                        confirmDismiss: (direction) async {
                          return await showDialog<bool>(
                            context: context,
                            builder: (context) => AlertDialog(
                              title: const Text('取消涟漪'),
                              content: const Text('确定要取消这个涟漪吗？'),
                              actions: [
                                TextButton(
                                  onPressed: () =>
                                      Navigator.pop(context, false),
                                  child: const Text('取消'),
                                ),
                                TextButton(
                                  onPressed: () => Navigator.pop(context, true),
                                  style: TextButton.styleFrom(
                                      foregroundColor: Colors.red),
                                  child: const Text('确定'),
                                ),
                              ],
                            ),
                          );
                        },
                        onDismissed: (direction) async {
                          // 保存备份用于回滚
                          final removedData = _ripplesData[index];
                          final removedStone = _ripples[index];
                          final removedIndex = index;
                          setState(() {
                            _ripplesData.removeAt(index);
                            _ripples.removeAt(index);
                          });
                          try {
                            await _interactionService.deleteRipple(rippleId);
                            if (mounted) {
                              ScaffoldMessenger.of(context).showSnackBar(
                                const SnackBar(content: Text('涟漪已取消')),
                              );
                            }
                          } catch (e) {
                            // API 失败时回滚
                            if (mounted) {
                              setState(() {
                                _ripplesData.insert(removedIndex, removedData);
                                _ripples.insert(removedIndex, removedStone);
                              });
                              ScaffoldMessenger.of(context).showSnackBar(
                                const SnackBar(content: Text('删除失败，请重试')),
                              );
                            }
                          }
                        },
                        child: Padding(
                          padding: const EdgeInsets.only(bottom: 16),
                          child: StoneCard(stone: _ripples[index]),
                        ),
                      );
                    },
                  ),
        ),
      ),
    );
  }
}
