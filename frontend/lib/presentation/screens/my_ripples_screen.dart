// 我的涟漪列表界面
//
// 展示当前用户产生过涟漪（共鸣/点赞）的所有石头，支持下拉刷新和左滑取消涟漪。
// 通过 WebSocket 监听涟漪删除、石头删除、涟漪计数更新事件实时同步列表。
// 依赖 [InteractionDataSource] 加载数据和执行删除，依赖 [WebSocketClient] 接收推送。

import 'package:flutter/material.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card/stone_card.dart';
import '../../data/datasources/interaction_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';

/// 我的涟漪列表页面
///
/// 涟漪是用户对石头的共鸣（类似点赞）。
/// 展示当前用户产生过涟漪的所有石头，支持下拉刷新和左滑取消。
/// 通过 WebSocket 监听涟漪删除、石头删除、涟漪计数更新 3 种事件。
class MyRipplesScreen extends StatefulWidget {
  final InteractionDataSource? interactionService;
  final WebSocketClient? wsClient;

  const MyRipplesScreen({
    super.key,
    this.interactionService,
    this.wsClient,
  });

  @override
  State<MyRipplesScreen> createState() => _MyRipplesScreenState();
}

class _MyRipplesScreenState extends State<MyRipplesScreen> {
  /// 涟漪原始数据（保留 ripple_id 等元信息，用于删除和 WebSocket 匹配）
  final List<Map<String, dynamic>> _ripplesData = [];

  /// 解析后的 Stone 列表，与 _ripplesData 索引一一对应
  final List<Stone> _ripples = [];
  final Map<String, int> _rippleIndexById = {};
  final Map<String, Set<String>> _rippleIdsByStoneId = {};
  bool _isLoading = false;
  late final InteractionDataSource _interactionService;
  late final WebSocketClient _wsClient;

  // WebSocket 事件监听器引用
  late void Function(Map<String, dynamic>) _rippleDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _rippleUpdateListener;

  @override
  void initState() {
    super.initState();
    _interactionService = widget.interactionService ?? sl<InteractionService>();
    _wsClient = widget.wsClient ?? WebSocketManager();
    _loadMyRipples();
    _initWebSocket();
  }

  @override
  void dispose() {
    // 移除WebSocket监听器
    _wsClient.off('ripple_deleted', _rippleDeletedListener);
    _wsClient.off('stone_deleted', _stoneDeletedListener);
    _wsClient.off('ripple_update', _rippleUpdateListener);
    super.dispose();
  }

  /// 注册 WebSocket 事件监听器
  void _initWebSocket() {
    // 监听涟漪删除 - 从列表中移除对应条目
    _rippleDeletedListener = (data) {
      if (!mounted) return;
      final rippleId = extractRippleEntityId(data);
      if (rippleId == null) return;
      setState(() {
        _removeRippleById(rippleId);
      });
    };
    _wsClient.on('ripple_deleted', _rippleDeletedListener);

    // 监听石头删除 - 移除该石头相关的涟漪
    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      if (stoneId == null) return;
      setState(() {
        _removeRipplesByStoneId(stoneId);
      });
    };
    _wsClient.on('stone_deleted', _stoneDeletedListener);

    // 监听涟漪更新 - 更新石头的涟漪计数
    _rippleUpdateListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      final rippleCount = extractRippleCount(data);
      if (stoneId == null || rippleCount is! int) return;
      setState(() {
        _updateRippleCountByStoneId(stoneId, rippleCount);
      });
    };
    _wsClient.on('ripple_update', _rippleUpdateListener);
  }

  void _rebuildRippleIndices() {
    _rippleIndexById.clear();
    _rippleIdsByStoneId.clear();
    for (var i = 0; i < _ripplesData.length; i++) {
      final rippleId = _ripplesData[i]['ripple_id']?.toString();
      final stoneId =
          _ripplesData[i]['stone_id']?.toString() ?? _ripples[i].stoneId;
      if (rippleId == null || rippleId.isEmpty) continue;
      _rippleIndexById[rippleId] = i;
      if (stoneId.isNotEmpty) {
        (_rippleIdsByStoneId[stoneId] ??= <String>{}).add(rippleId);
      }
    }
  }

  bool _removeRippleById(String rippleId) {
    final index = _rippleIndexById[rippleId];
    if (index == null) return false;
    _ripplesData.removeAt(index);
    _ripples.removeAt(index);
    _rebuildRippleIndices();
    return true;
  }

  bool _removeRipplesByStoneId(String stoneId) {
    final rippleIds = _rippleIdsByStoneId[stoneId];
    if (rippleIds == null || rippleIds.isEmpty) return false;

    final retainedData = <Map<String, dynamic>>[];
    final retainedStones = <Stone>[];
    var removed = false;

    for (var i = 0; i < _ripplesData.length; i++) {
      final rippleId = _ripplesData[i]['ripple_id']?.toString();
      if (rippleId != null && rippleIds.contains(rippleId)) {
        removed = true;
        continue;
      }
      retainedData.add(_ripplesData[i]);
      retainedStones.add(_ripples[i]);
    }

    if (!removed) return false;
    _ripplesData
      ..clear()
      ..addAll(retainedData);
    _ripples
      ..clear()
      ..addAll(retainedStones);
    _rebuildRippleIndices();
    return true;
  }

  bool _updateRippleCountByStoneId(String stoneId, int rippleCount) {
    final rippleIds = _rippleIdsByStoneId[stoneId];
    if (rippleIds == null || rippleIds.isEmpty) return false;

    var updated = false;
    for (final rippleId in rippleIds) {
      final index = _rippleIndexById[rippleId];
      if (index == null) continue;
      _ripples[index] = _ripples[index].copyWith(rippleCount: rippleCount);
      updated = true;
    }
    return updated;
  }

  /// 从后端加载当前用户的涟漪列表
  Future<void> _loadMyRipples() async {
    if (mounted) setState(() => _isLoading = true);

    try {
      final result = await _interactionService.getMyRipples(
        page: 1,
        pageSize: 50,
      );

      if (result['success'] == true && mounted) {
        final rawItems = result['ripples'] ?? result['items'] ?? result['list'];
        final items = (rawItems as List? ?? const [])
            .whereType<Map>()
            .map((json) =>
                normalizePayloadContract(Map<String, dynamic>.from(json)))
            .toList();
        setState(() {
          _ripplesData.clear();
          _ripplesData.addAll(items);
          _ripples.clear();
          _ripples.addAll(items.map((json) => Stone.fromJson(json)));
          _rebuildRippleIndices();
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
    final isDark = Theme.of(context).brightness == Brightness.dark;
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
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              isDark ? AppTheme.nightDeep : const Color(0xFFF0F8FF),
              isDark ? AppTheme.nightSurface : const Color(0xFFE8F0FA)
            ],
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
                                  color: isDark
                                      ? Colors.white24
                                      : Colors.grey[300],
                                ),
                                const SizedBox(height: 16),
                                Text(
                                  '还没有涟漪，你的声音会被听见',
                                  style: TextStyle(
                                    fontSize: 16,
                                    color: isDark
                                        ? AppTheme.darkTextSecondary
                                        : Colors.grey[600],
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
                                    onPressed: () =>
                                        Navigator.pop(context, true),
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
                              _rebuildRippleIndices();
                            });
                            try {
                              final result =
                                  await _interactionService.deleteRipple(
                                rippleId,
                              );
                              if (!context.mounted) return;
                              if (result['success'] != true) {
                                throw Exception(result['message'] ?? '删除失败');
                              }
                              ScaffoldMessenger.of(context).showSnackBar(
                                const SnackBar(content: Text('涟漪已取消')),
                              );
                            } catch (e) {
                              // API 失败时回滚
                              if (!context.mounted) return;
                              setState(() {
                                _ripplesData.insert(removedIndex, removedData);
                                _ripples.insert(removedIndex, removedStone);
                                _rebuildRippleIndices();
                              });
                              ScaffoldMessenger.of(context).showSnackBar(
                                const SnackBar(content: Text('删除失败，请重试')),
                              );
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
