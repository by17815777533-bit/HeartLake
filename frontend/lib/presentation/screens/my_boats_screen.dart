// 我的纸船列表界面
//
// 展示当前用户发送的所有纸船（评论）记录。

import 'package:flutter/material.dart';
import '../../data/datasources/interaction_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../../utils/payload_contract.dart';
import '../../domain/entities/stone.dart';
import 'stone_detail_screen.dart';

/// 我发出的纸船列表页面
///
/// 纸船是用户对他人石头的匿名回应。
/// 展示当前用户发出的所有纸船记录，点击可跳转到对应石头详情。
/// 支持左滑删除、长按删除，通过 WebSocket 实时同步纸船和石头的删除事件。
class MyBoatsScreen extends StatefulWidget {
  const MyBoatsScreen({super.key});

  @override
  State<MyBoatsScreen> createState() => _MyBoatsScreenState();
}

/// 纸船列表页面的状态管理
///
/// 通过 [InteractionService] 加载纸船数据，使用 [WebSocketManager] 监听删除事件。
class _MyBoatsScreenState extends State<MyBoatsScreen> {
  final List<Map<String, dynamic>> _boats = [];
  final Map<String, int> _boatIndexById = {};
  final Map<String, Set<String>> _boatIdsByStoneId = {};
  bool _isLoading = false;
  final InteractionService _interactionService = sl<InteractionService>();
  late final WebSocketManager _wsManager;

  // WebSocket 监听器引用，dispose 时精确移除
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;

  @override
  void initState() {
    super.initState();
    _wsManager = WebSocketManager();
    _loadMyBoats();
    _initWebSocket();
  }

  @override
  void dispose() {
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    super.dispose();
  }

  /// 注册 WebSocket 监听器，处理纸船删除和关联石头删除事件
  void _initWebSocket() {
    // 监听纸船删除
    _boatDeletedListener = (data) {
      if (!mounted) return;
      final boatId = extractBoatEntityId(data);
      if (boatId == null) return;
      setState(() {
        _removeBoatById(boatId);
      });
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);

    // 监听石头删除（石头没了，评论也该没了）
    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = extractStoneEntityId(data);
      if (stoneId == null) return;
      setState(() {
        _removeBoatsByStoneId(stoneId);
      });
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);
  }

  void _rebuildBoatIndices() {
    _boatIndexById.clear();
    _boatIdsByStoneId.clear();
    for (var i = 0; i < _boats.length; i++) {
      final boatId = _boats[i]['boat_id']?.toString();
      if (boatId != null && boatId.isNotEmpty) {
        _boatIndexById[boatId] = i;
      }
      final stoneId = _boats[i]['stone_id']?.toString();
      if (stoneId != null &&
          stoneId.isNotEmpty &&
          boatId != null &&
          boatId.isNotEmpty) {
        (_boatIdsByStoneId[stoneId] ??= <String>{}).add(boatId);
      }
    }
  }

  bool _removeBoatById(String boatId) {
    final index = _boatIndexById[boatId];
    if (index == null) return false;
    _boats.removeAt(index);
    _rebuildBoatIndices();
    return true;
  }

  int _removeBoatIds(Set<String> boatIds) {
    if (boatIds.isEmpty) return 0;
    final retained = <Map<String, dynamic>>[];
    var removedCount = 0;

    for (final boat in _boats) {
      final boatId = boat['boat_id']?.toString();
      if (boatId != null && boatIds.contains(boatId)) {
        removedCount++;
        continue;
      }
      retained.add(boat);
    }

    if (removedCount == 0) return 0;
    _boats
      ..clear()
      ..addAll(retained);
    _rebuildBoatIndices();
    return removedCount;
  }

  bool _removeBoatsByStoneId(String stoneId) {
    final boatIds = _boatIdsByStoneId[stoneId];
    if (boatIds == null || boatIds.isEmpty) return false;
    return _removeBoatIds(boatIds) > 0;
  }

  /// 从后端加载纸船列表，当前固定加载第一页（最多50条）
  Future<void> _loadMyBoats() async {
    if (mounted) setState(() => _isLoading = true);

    try {
      final result = await _interactionService.getMyBoatsComments(
        page: 1,
        pageSize: 50,
      );

      if (result['success'] == true && mounted) {
        final boats = (result['boats'] as List? ?? const [])
            .whereType<Map>()
            .map((boat) =>
                normalizePayloadContract(Map<String, dynamic>.from(boat)))
            .toList();
        setState(() {
          _boats.clear();
          _boats.addAll(boats);
          _rebuildBoatIndices();
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

  /// 弹出确认对话框后删除指定纸船，成功后从本地列表移除
  Future<void> _deleteBoat(String boatId) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('删除纸船'),
        content: const Text('确定要删除这条纸船吗？此操作无法撤销。'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            style: TextButton.styleFrom(foregroundColor: Colors.red),
            child: const Text('删除'),
          ),
        ],
      ),
    );

    if (confirmed == true && mounted) {
      final messenger = ScaffoldMessenger.of(context);
      final result = await _interactionService.deleteBoat(boatId);
      if (result['success'] == true) {
        setState(() {
          _removeBoatById(boatId);
        });
        messenger.showSnackBar(
          const SnackBar(
            content: Text('已轻轻放下'),
            backgroundColor: AppTheme.skyBlue,
          ),
        );
      } else {
        messenger.showSnackBar(
          SnackBar(
            content: Text(result['message'] ?? '删除失败'),
            backgroundColor: Colors.red,
          ),
        );
      }
    }
  }

  /// 根据纸船关联石头的心情类型获取对应的颜色配置
  MoodColorConfig _getMoodConfig(Map<String, dynamic> boat) {
    final moodType = boat['stone_mood_type'] as String?;
    if (moodType != null) {
      return MoodColors.getConfig(MoodColors.fromString(moodType));
    }
    return MoodColors.getConfig(MoodType.neutral);
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      appBar: AppBar(
        title: const Text('我的纸船'),
        centerTitle: true,
        backgroundColor: isDark ? const Color(0xFF16213E) : null,
        foregroundColor: isDark ? Colors.white : null,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loadMyBoats,
          ),
        ],
      ),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: isDark
                ? [const Color(0xFF0D1B2E), const Color(0xFF1A1A2E)]
                : [const Color(0xFFFFF8F0), const Color(0xFFFAF0E8)],
          ),
        ),
        child: RefreshIndicator(
          onRefresh: _loadMyBoats,
          child: _isLoading
              ? Center(
                  child: Column(mainAxisSize: MainAxisSize.min, children: [
                  const CircularProgressIndicator(),
                  const SizedBox(height: 16),
                  Text('正在追踪纸船的漂流...',
                      style: TextStyle(
                          color: isDark
                              ? const Color(0xFF9AA0A6)
                              : Colors.grey[600]))
                ]))
              : _boats.isEmpty
                  ? ListView(
                      children: [
                        SizedBox(
                          height: MediaQuery.of(context).size.height * 0.7,
                          child: Center(
                            child: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Icon(
                                  Icons.sailing_outlined,
                                  size: 80,
                                  color: isDark
                                      ? Colors.white24
                                      : Colors.grey[300],
                                ),
                                const SizedBox(height: 16),
                                Text(
                                  '还没有纸船，试着给陌生人送去温暖',
                                  style: TextStyle(
                                    fontSize: 16,
                                    color: isDark
                                        ? const Color(0xFF9AA0A6)
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
                      itemCount: _boats.length,
                      itemBuilder: (context, index) {
                        final boat = _boats[index];
                        final moodConfig = _getMoodConfig(boat);
                        final boatId = boat['boat_id']?.toString() ?? '';

                        return Dismissible(
                          key: Key(boatId),
                          direction: DismissDirection.endToStart,
                          background: Container(
                            alignment: Alignment.centerRight,
                            padding: const EdgeInsets.only(right: 20),
                            color: Colors.red,
                            child:
                                const Icon(Icons.delete, color: Colors.white),
                          ),
                          confirmDismiss: (direction) async {
                            return await showDialog<bool>(
                              context: context,
                              builder: (context) => AlertDialog(
                                title: const Text('删除纸船'),
                                content: const Text('确定要删除这条纸船吗？'),
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
                                    child: const Text('删除'),
                                  ),
                                ],
                              ),
                            );
                          },
                          onDismissed: (direction) {
                            _interactionService.deleteBoat(boatId);
                            setState(() {
                              _boats.removeAt(index);
                            });
                            ScaffoldMessenger.of(context).showSnackBar(
                              const SnackBar(content: Text('已轻轻放下')),
                            );
                          },
                          child: Card(
                            margin: const EdgeInsets.only(bottom: 12),
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(16),
                              side: BorderSide(
                                color:
                                    moodConfig.primary.withValues(alpha: 0.4),
                                width: 2,
                              ),
                            ),
                            elevation: 3,
                            shadowColor:
                                moodConfig.primary.withValues(alpha: 0.2),
                            child: InkWell(
                              borderRadius: BorderRadius.circular(16),
                              onTap: () {
                                final stoneId = boat['stone_id']?.toString();
                                if (stoneId != null && stoneId.isNotEmpty) {
                                  Navigator.push(
                                    context,
                                    MaterialPageRoute(
                                      builder: (_) => StoneDetailScreen(
                                        stone: Stone.fromBoatReference(boat),
                                      ),
                                    ),
                                  );
                                }
                              },
                              onLongPress: () => _deleteBoat(boatId),
                              child: Container(
                                decoration: BoxDecoration(
                                  borderRadius: BorderRadius.circular(16),
                                  gradient: LinearGradient(
                                    begin: Alignment.topLeft,
                                    end: Alignment.bottomRight,
                                    colors: [
                                      isDark
                                          ? const Color(0xFF1B2838)
                                          : Colors.white,
                                      moodConfig.primary
                                          .withValues(alpha: 0.05),
                                    ],
                                  ),
                                ),
                                padding: const EdgeInsets.all(16),
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Row(
                                      children: [
                                        // 治愈风格图标
                                        Container(
                                          padding: const EdgeInsets.all(2),
                                          decoration: BoxDecoration(
                                            shape: BoxShape.circle,
                                            gradient: LinearGradient(
                                              colors: [
                                                moodConfig.primary
                                                    .withValues(alpha: 0.6),
                                                moodConfig.rippleColor
                                                    .withValues(alpha: 0.4),
                                              ],
                                            ),
                                          ),
                                          child: CircleAvatar(
                                            radius: 14,
                                            backgroundColor: isDark
                                                ? const Color(0xFF1B2838)
                                                : Colors.white,
                                            child: Icon(
                                              Icons.sailing,
                                              color: moodConfig.primary,
                                              size: 16,
                                            ),
                                          ),
                                        ),
                                        const SizedBox(width: 8),
                                        Text(
                                          '我的纸船',
                                          style: TextStyle(
                                            fontSize: 12,
                                            color: moodConfig.primary,
                                            fontWeight: FontWeight.w500,
                                          ),
                                        ),
                                        const Spacer(),
                                        // 删除按钮
                                        IconButton(
                                          icon: Icon(
                                            Icons.delete_outline,
                                            color: isDark
                                                ? Colors.white30
                                                : Colors.grey[400],
                                            size: 20,
                                          ),
                                          onPressed: () => _deleteBoat(boatId),
                                          padding: EdgeInsets.zero,
                                          constraints: const BoxConstraints(),
                                        ),
                                      ],
                                    ),
                                    const SizedBox(height: 12),
                                    // 内容区域带治愈边框
                                    Container(
                                      width: double.infinity,
                                      padding: const EdgeInsets.all(12),
                                      decoration: BoxDecoration(
                                        color: moodConfig.primary
                                            .withValues(alpha: 0.05),
                                        borderRadius: BorderRadius.circular(12),
                                        border: Border.all(
                                          color: moodConfig.primary
                                              .withValues(alpha: 0.15),
                                          width: 1,
                                        ),
                                      ),
                                      child: Row(
                                        crossAxisAlignment:
                                            CrossAxisAlignment.start,
                                        children: [
                                          Icon(
                                            Icons.format_quote,
                                            size: 16,
                                            color: moodConfig.primary
                                                .withValues(alpha: 0.4),
                                          ),
                                          const SizedBox(width: 8),
                                          Expanded(
                                            child: Text(
                                              boat['content'] ?? '',
                                              style: const TextStyle(
                                                fontSize: 15,
                                                height: 1.5,
                                              ),
                                            ),
                                          ),
                                        ],
                                      ),
                                    ),
                                    const SizedBox(height: 12),
                                    Text(
                                      boat['created_at'] ?? '',
                                      style: TextStyle(
                                        fontSize: 12,
                                        color: isDark
                                            ? Colors.white38
                                            : Colors.grey[500],
                                      ),
                                    ),
                                  ],
                                ),
                              ),
                            ),
                          ),
                        );
                      },
                    ),
        ),
      ),
    );
  }
}
