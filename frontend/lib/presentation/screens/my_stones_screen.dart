// 我的石头列表界面
//
// 展示当前用户发布的所有石头，支持下拉刷新和删除操作。
// 通过 WebSocket 监听石头删除、涟漪和纸船变更事件实时更新列表。
// 依赖 [StoneService] 加载数据，依赖 [WebSocketManager] 接收实时推送。

import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card/stone_card.dart';
import '../../data/datasources/stone_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';

/// 我的石头列表页面
///
/// 展示当前用户投出的所有石头，支持下拉刷新和删除操作。
/// 通过 WebSocket 监听石头删除、涟漪增减、纸船增减共 5 种事件实时更新列表。
class MyStonesScreen extends StatefulWidget {
  const MyStonesScreen({super.key});

  @override
  State<MyStonesScreen> createState() => _MyStonesScreenState();
}

class _MyStonesScreenState extends State<MyStonesScreen> {
  final List<Stone> _myStones = [];
  final Map<String, int> _stoneIndexById = {};
  bool _isLoading = false;
  final StoneService _stoneService = sl<StoneService>();
  final WebSocketManager _wsManager = WebSocketManager();

  // WebSocket 事件监听器引用，dispose 时逐个移除
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

  /// 注册 WebSocket 事件监听器
  ///
  /// 监听 5 种事件：石头删除、涟漪增/删、纸船增/删。
  /// 所有回调都使用服务端返回的精确计数更新本地状态。
  void _initWebSocket() {
    // 监听石头删除
    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      if (stoneId == null) return;
      setState(() {
        _removeStoneById(stoneId);
      });
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    // 监听涟漪更新 - 使用服务器实际总数和copyWith
    _rippleUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      final rippleCount = data['ripple_count'];
      if (stoneId == null || rippleCount is! int) return;
      setState(() {
        final index = _stoneIndexById[stoneId];
        if (index != null) {
          _myStones[index] = _myStones[index].copyWith(rippleCount: rippleCount);
        }
      });
    };
    _wsManager.on('ripple_update', _rippleUpdateListener);

    // 监听涟漪删除
    _rippleDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      final rippleCount = data['ripple_count'];
      if (stoneId == null || rippleCount is! int) return;
      setState(() {
        final index = _stoneIndexById[stoneId];
        if (index != null) {
          _myStones[index] = _myStones[index].copyWith(rippleCount: rippleCount);
        }
      });
    };
    _wsManager.on('ripple_deleted', _rippleDeletedListener);

    // 监听纸船更新 - 使用服务器实际总数和copyWith
    _boatUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      final boatCount = data['boat_count'];
      if (stoneId == null || boatCount is! int) return;
      setState(() {
        final index = _stoneIndexById[stoneId];
        if (index != null) {
          _myStones[index] = _myStones[index].copyWith(boatCount: boatCount);
        }
      });
    };
    _wsManager.on('boat_update', _boatUpdateListener);

    // 监听纸船删除
    _boatDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      final boatCount = data['boat_count'];
      if (stoneId == null || boatCount is! int) return;
      setState(() {
        final index = _stoneIndexById[stoneId];
        if (index != null) {
          _myStones[index] = _myStones[index].copyWith(boatCount: boatCount);
        }
      });
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);
  }

  void _rebuildStoneIndex() {
    _stoneIndexById.clear();
    for (var i = 0; i < _myStones.length; i++) {
      _stoneIndexById[_myStones[i].stoneId] = i;
    }
  }

  bool _removeStoneById(String stoneId) {
    final index = _stoneIndexById[stoneId];
    if (index == null) return false;
    _myStones.removeAt(index);
    _rebuildStoneIndex();
    return true;
  }

  /// 从后端加载当前用户的石头列表（一次性加载，不分页）
  Future<void> _loadMyStones() async {
    setState(() => _isLoading = true);

    try {
      final result = await _stoneService.getMyStones(page: 1, pageSize: 100);

      if (result['success'] == true && mounted) {
        setState(() {
          _myStones.clear();
          _myStones.addAll(result['stones'] as List<Stone>);
          _rebuildStoneIndex();
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
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      appBar: AppBar(
        title: const Text('我的石头'),
        centerTitle: true,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loadMyStones,
          ),
        ],
      ),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [isDark ? AppTheme.nightDeep : const Color(0xFFF5F0FF), isDark ? AppTheme.nightSurface : const Color(0xFFE8F4FD)],
          ),
        ),
        child: RefreshIndicator(
          onRefresh: _loadMyStones,
          child: _isLoading
            ? Center(child: Column(mainAxisSize: MainAxisSize.min, children: [const CircularProgressIndicator(), const SizedBox(height: 16), Text('正在寻找你的石头...', style: TextStyle(color: isDark ? AppTheme.darkTextSecondary : Colors.grey[600]))]))
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
                                color: isDark ? Colors.white24 : Colors.grey[300],
                              ),
                              const SizedBox(height: 16),
                              Text(
                                '还没有投出石头，来投下你的第一颗吧',
                                style: TextStyle(
                                  fontSize: 16,
                                  color: isDark ? AppTheme.darkTextSecondary : Colors.grey[600],
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
