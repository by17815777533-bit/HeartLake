// @file my_boats_screen.dart
// @brief 我的纸船列表界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../data/datasources/interaction_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../../domain/entities/stone.dart';
import 'stone_detail_screen.dart';

class MyBoatsScreen extends StatefulWidget {
  const MyBoatsScreen({super.key});

  @override
  State<MyBoatsScreen> createState() => _MyBoatsScreenState();
}

class _MyBoatsScreenState extends State<MyBoatsScreen> {
  final List<Map<String, dynamic>> _boats = [];
  bool _isLoading = false;
  final InteractionService _interactionService = InteractionService();
  late final WebSocketManager _wsManager;

  // WebSocket Listeners
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _reconnectedListener;
  late void Function(Map<String, dynamic>) _boatUpdateListener;

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
    _wsManager.off('boat_update', _boatUpdateListener);
    _wsManager.off('reconnected', _reconnectedListener);
    super.dispose();
  }

  void _initWebSocket() {
    // 监听纸船删除
    _boatDeletedListener = (data) {
      if (!mounted) return;
      final boatId = data['boat_id'];
      if (boatId == null) return;
      setState(() {
        _boats.removeWhere((b) => b['boat_id'] == boatId);
      });
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);

    // 监听石头删除（石头没了，评论也该没了）
    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      if (stoneId == null) return;
      setState(() {
        _boats.removeWhere((b) => b['stone_id'] == stoneId);
      });
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    // 监听纸船更新（别人回复了我的纸船）
    _boatUpdateListener = (data) {
      if (!mounted) return;
      // 有更新时刷新列表
      _loadMyBoats();
    };
    _wsManager.on('boat_update', _boatUpdateListener);

    // 断线重连后自动刷新
    _reconnectedListener = (data) {
      if (mounted) _loadMyBoats();
    };
    _wsManager.on('reconnected', _reconnectedListener);
  }

  Future<void> _loadMyBoats() async {
    if (mounted) setState(() => _isLoading = true);

    try {
      final result = await _interactionService.getMyBoatsComments(
        page: 1,
        pageSize: 50,
      );

      if (result['success'] == true && mounted) {
        setState(() {
          _boats.clear();
          _boats.addAll(List<Map<String, dynamic>>.from(result['boats'] ?? []));
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

  Future<void> _deleteBoat(String boatId, int index) async {
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
          _boats.removeAt(index);
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

  // 根据石头的心情获取颜色
  MoodColorConfig _getMoodConfig(Map<String, dynamic> boat) {
    final moodType = boat['stone_mood_type'] as String?;
    if (moodType != null) {
      return MoodColors.getConfig(MoodColors.fromString(moodType));
    }
    return MoodColors.getConfig(MoodType.neutral);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('我的纸船'),
        centerTitle: true,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loadMyBoats,
          ),
        ],
      ),
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFFFFF8F0), Color(0xFFFAF0E8)],
          ),
        ),
        child: RefreshIndicator(
          onRefresh: _loadMyBoats,
          child: _isLoading
            ? Center(child: Column(mainAxisSize: MainAxisSize.min, children: [const CircularProgressIndicator(), const SizedBox(height: 16), Text('正在追踪纸船的漂流...', style: TextStyle(color: Colors.grey[600]))]))
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
                                color: Colors.grey[300],
                              ),
                              const SizedBox(height: 16),
                              Text(
                                '还没有纸船，试着给陌生人送去温暖',
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
                          child: const Icon(Icons.delete, color: Colors.white),
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
                                  onPressed: () => Navigator.pop(context, true),
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
                              color: moodConfig.primary.withOpacity(0.4),
                              width: 2,
                            ),
                          ),
                          elevation: 3,
                          shadowColor: moodConfig.primary.withOpacity(0.2),
                          child: InkWell(
                            borderRadius: BorderRadius.circular(16),
                            onTap: () {
                              final stoneId = boat['stone_id']?.toString();
                              if (stoneId != null && stoneId.isNotEmpty) {
                                final stone = Stone(
                                  stoneId: stoneId,
                                  userId: boat['stone_user_id']?.toString() ?? '',
                                  content: boat['stone_content']?.toString() ?? '',
                                  stoneType: 'medium',
                                  stoneColor: '#7A92A3',
                                  createdAt: DateTime.tryParse(boat['stone_created_at']?.toString() ?? '') ?? DateTime.now(),
                                  moodType: boat['stone_mood_type']?.toString(),
                                );
                                Navigator.push(
                                  context,
                                  MaterialPageRoute(
                                    builder: (_) => StoneDetailScreen(stone: stone),
                                  ),
                                );
                              }
                            },
                            onLongPress: () => _deleteBoat(boatId, index),
                            child: Container(
                              decoration: BoxDecoration(
                                borderRadius: BorderRadius.circular(16),
                                gradient: LinearGradient(
                                  begin: Alignment.topLeft,
                                  end: Alignment.bottomRight,
                                  colors: [
                                    Colors.white,
                                    moodConfig.primary.withOpacity(0.05),
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
                                                  .withOpacity(0.6),
                                              moodConfig.rippleColor
                                                  .withOpacity(0.4),
                                            ],
                                          ),
                                        ),
                                        child: CircleAvatar(
                                          radius: 14,
                                          backgroundColor: Colors.white,
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
                                          color: Colors.grey[400],
                                          size: 20,
                                        ),
                                        onPressed: () =>
                                            _deleteBoat(boatId, index),
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
                                      color:
                                          moodConfig.primary.withOpacity(0.05),
                                      borderRadius: BorderRadius.circular(12),
                                      border: Border.all(
                                        color: moodConfig.primary
                                            .withOpacity(0.15),
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
                                              .withOpacity(0.4),
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
                                      color: Colors.grey[500],
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
