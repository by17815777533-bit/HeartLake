// @file received_boats_screen.dart
// @brief 收到的纸船列表界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../data/datasources/api_client.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../domain/entities/stone.dart';
import '../../utils/app_theme.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../../utils/animation_utils.dart';
import 'stone_detail_screen.dart';

class ReceivedBoatsScreen extends StatefulWidget {
  const ReceivedBoatsScreen({super.key});

  @override
  State<ReceivedBoatsScreen> createState() => _ReceivedBoatsScreenState();
}

class _ReceivedBoatsScreenState extends State<ReceivedBoatsScreen> {
  final List<Map<String, dynamic>> _boats = [];
  final ApiClient _apiClient = ApiClient();
  final WebSocketManager _wsManager = WebSocketManager();
  bool _isLoading = false;

  // WebSocket Listeners
  late void Function(Map<String, dynamic>) _newBoatListener;
  late void Function(Map<String, dynamic>) _boatUpdateListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;
  late void Function(Map<String, dynamic>) _reconnectedListener;

  @override
  void initState() {
    super.initState();
    _loadReceivedBoats();
    _initWebSocket();
  }

  @override
  void dispose() {
    _wsManager.off('new_boat', _newBoatListener);
    _wsManager.off('boat_update', _boatUpdateListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    _wsManager.off('reconnected', _reconnectedListener);
    super.dispose();
  }

  void _initWebSocket() {
    // 监听新纸船 - 重新加载列表
    _newBoatListener = (data) {
      if (mounted) {
        _loadReceivedBoats();
      }
    };
    _wsManager.on('new_boat', _newBoatListener);

    // 监听纸船更新（boat_update 是后端实际广播的事件）- 重新加载列表
    _boatUpdateListener = (data) {
      if (mounted) {
        _loadReceivedBoats();
      }
    };
    _wsManager.on('boat_update', _boatUpdateListener);

    // 监听纸船删除 - 从列表中移除
    _boatDeletedListener = (data) {
      if (!mounted) return;
      final boatId = data['boat_id'];
      if (boatId == null) return;

      setState(() {
        _boats.removeWhere((b) => b['boat_id'] == boatId);
      });
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);

    // 监听石头删除 - 移除该石头相关的收到的纸船
    _stoneDeletedListener = (data) {
      if (!mounted) return;
      final stoneId = data['stone_id'];
      if (stoneId == null) return;
      setState(() {
        _boats.removeWhere((b) => b['stone_id'] == stoneId);
      });
    };
    _wsManager.on('stone_deleted', _stoneDeletedListener);

    // 监听重连成功，刷新数据
    _reconnectedListener = (data) {
      if (mounted) _loadReceivedBoats();
    };
    _wsManager.on('reconnected', _reconnectedListener);
  }

  Future<void> _loadReceivedBoats() async {
    setState(() => _isLoading = true);

    try {
      final response = await _apiClient.get(
        '/users/my/boats',
        queryParameters: {'page': 1, 'page_size': 100},
      );

      if (response.statusCode == 200 &&
          response.data['code'] == 0 &&
          mounted) {
        final items = response.data['data']['items'] as List? ?? [];
        setState(() {
          _boats.clear();
          _boats.addAll(items.whereType<Map<String, dynamic>>());
          _isLoading = false;
        });
      } else {
        if (mounted) setState(() => _isLoading = false);
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('加载失败，请下拉重试'),
            backgroundColor: AppTheme.lightStone,
          ),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      showWater: true,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: AppTheme.textPrimary,
        title: Text(
          '收到的纸船',
          style: TextStyle(
            color: AppTheme.primaryLightColor,
            fontWeight: FontWeight.bold,
            shadows: [
              Shadow(
                color: AppTheme.primaryLightColor.withValues(alpha: 0.6),
                blurRadius: 12,
              ),
            ],
          ),
        ),
        centerTitle: true,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh, color: AppTheme.darkTextSecondary),
            onPressed: _loadReceivedBoats,
          ),
        ],
      ),
      body: SafeArea(
        child: RefreshIndicator(
          onRefresh: _loadReceivedBoats,
          color: AppTheme.primaryLightColor,
          backgroundColor: AppTheme.lightStone,
          child: _isLoading
              ? const Center(
                  child: CircularProgressIndicator(color: AppTheme.primaryLightColor),
                )
              : _boats.isEmpty
                  ? ListView(
                      children: [
                        SizedBox(
                          height: MediaQuery.of(context).size.height * 0.7,
                          child: const Center(
                            child: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Icon(
                                  Icons.sailing_outlined,
                                  size: 80,
                                  color: AppTheme.textSecondary,
                                ),
                                SizedBox(height: 16),
                                Text(
                                  '还没有收到纸船',
                                  style: TextStyle(
                                    fontSize: 16,
                                    color: AppTheme.textPrimary,
                                  ),
                                ),
                                SizedBox(height: 8),
                                Text(
                                  '别人会在你的石头下留言',
                                  style: TextStyle(
                                    fontSize: 14,
                                    color: AppTheme.textSecondary,
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
                          return Padding(
                            padding: const EdgeInsets.only(bottom: 16),
                            child: SkyGlassCard(
                              glowColor: AppTheme.backgroundColor,
                              borderRadius: 16,
                              padding: EdgeInsets.zero,
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
                                    SkyPageRoute(page: StoneDetailScreen(stone: stone)),
                                  );
                                }
                              },
                              child: Padding(
                                padding: const EdgeInsets.all(16),
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    // 发送者信息
                                    Row(
                                      children: [
                                        const Icon(Icons.person, size: 16, color: AppTheme.darkTextSecondary),
                                        const SizedBox(width: 8),
                                        Text(
                                          boat['sender_name'] ?? '匿名旅人',
                                          style: const TextStyle(
                                            fontWeight: FontWeight.bold,
                                            color: AppTheme.textPrimary,
                                          ),
                                        ),
                                        const Spacer(),
                                        Text(
                                          boat['created_at']?.toString() ?? '',
                                          style: const TextStyle(
                                            fontSize: 12,
                                            color: AppTheme.textSecondary,
                                          ),
                                        ),
                                      ],
                                    ),
                                    const SizedBox(height: 12),
                                    // 纸船内容
                                    Text(
                                      boat['content'] ?? '',
                                      style: const TextStyle(
                                        fontSize: 16,
                                        color: AppTheme.textPrimary,
                                      ),
                                    ),
                                    Divider(height: 24, color: AppTheme.primaryLightColor.withValues(alpha: 0.15)),
                                    // 对应的石头
                                    Container(
                                      padding: const EdgeInsets.all(12),
                                      decoration: BoxDecoration(
                                        color: AppTheme.primaryLightColor.withValues(alpha: 0.08),
                                        borderRadius: BorderRadius.circular(8),
                                      ),
                                      child: Column(
                                        crossAxisAlignment:
                                            CrossAxisAlignment.start,
                                        children: [
                                          const Text(
                                            '你的石头:',
                                            style: TextStyle(
                                              fontSize: 12,
                                              color: AppTheme.textSecondary,
                                            ),
                                          ),
                                          const SizedBox(height: 4),
                                          Text(
                                            boat['stone_content'] ?? '',
                                            style: const TextStyle(
                                              fontSize: 14,
                                              color: AppTheme.textSecondary,
                                            ),
                                            maxLines: 3,
                                            overflow: TextOverflow.ellipsis,
                                          ),
                                        ],
                                      ),
                                    ),
                                  ],
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
