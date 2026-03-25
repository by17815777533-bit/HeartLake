// 收到的纸船列表界面
//
// 展示其他用户对我的石头的匿名回应。

import 'package:flutter/material.dart';
import '../../data/datasources/social_payload_normalizer.dart';
import '../../data/datasources/user_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../domain/entities/stone.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import 'stone_detail_screen.dart';

/// 收到的纸船列表页面
///
/// 展示其他用户对我的石头发出的匿名纸船回应，
/// 点击可跳转到对应石头详情查看完整内容。
/// 通过 WebSocket 实时监听新纸船到达事件。
class ReceivedBoatsScreen extends StatefulWidget {
  const ReceivedBoatsScreen({super.key});

  @override
  State<ReceivedBoatsScreen> createState() => _ReceivedBoatsScreenState();
}

/// 收到的纸船列表页面状态管理
///
/// 通过 WebSocket 监听三类实时事件：
/// - boat_update: 重新加载整个列表
/// - boat_deleted: 按 boat_id 从本地列表移除
/// - stone_deleted: 移除该石头关联的所有纸船
class _ReceivedBoatsScreenState extends State<ReceivedBoatsScreen> {
  final List<Map<String, dynamic>> _boats = [];
  final Map<String, int> _boatIndexById = {};
  final Map<String, Set<String>> _boatIdsByStoneId = {};
  final UserService _userService = sl<UserService>();
  final WebSocketManager _wsManager = WebSocketManager();
  bool _isLoading = false;

  // WebSocket 监听器
  late void Function(Map<String, dynamic>) _boatUpdateListener;
  late void Function(Map<String, dynamic>) _boatDeletedListener;
  late void Function(Map<String, dynamic>) _stoneDeletedListener;

  @override
  void initState() {
    super.initState();
    _loadReceivedBoats();
    _initWebSocket();
  }

  @override
  void dispose() {
    _wsManager.off('boat_update', _boatUpdateListener);
    _wsManager.off('boat_deleted', _boatDeletedListener);
    _wsManager.off('stone_deleted', _stoneDeletedListener);
    super.dispose();
  }

  /// 注册 WebSocket 实时事件监听器
  void _initWebSocket() {
    // 监听纸船更新 - 重新加载列表
    _boatUpdateListener = (data) {
      if (mounted) {
        _loadReceivedBoats();
      }
    };
    _wsManager.on('boat_update', _boatUpdateListener);

    // 监听纸船删除 - 从列表中移除
    _boatDeletedListener = (data) {
      if (!mounted) return;
      final boatId = extractBoatEntityId(data);
      if (boatId == null) return;
      setState(() {
        _removeBoatById(boatId);
      });
    };
    _wsManager.on('boat_deleted', _boatDeletedListener);

    // 监听石头删除 - 移除该石头相关的收到的纸船
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

  void _replaceBoats(List<Map<String, dynamic>> boats) {
    _boats
      ..clear()
      ..addAll(boats);
    _rebuildBoatIndices();
  }

  void _reportUiError(
    Object error,
    StackTrace stackTrace,
    String context,
  ) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stackTrace,
        library: 'heartlake',
        context: ErrorDescription(context),
      ),
    );
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

  /// 从后端加载收到的纸船列表，支持下拉刷新
  Future<void> _loadReceivedBoats() async {
    setState(() => _isLoading = true);

    try {
      final result = await _userService.getMyBoats();

      if (result['success'] == true && mounted) {
        final items = extractNormalizedList(
          result,
          itemNormalizer: normalizeBoatPayload,
          listKeys: const ['boats'],
        );
        setState(() {
          _replaceBoats(items);
          _isLoading = false;
        });
      } else {
        if (mounted) setState(() => _isLoading = false);
      }
    } catch (error, stackTrace) {
      _reportUiError(
        error,
        stackTrace,
        'ReceivedBoatsScreen._loadReceivedBoats',
      );
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
        title: const Text('收到的纸船'),
        centerTitle: true,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loadReceivedBoats,
          ),
        ],
      ),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              isDark
                  ? AppTheme.nightDeep
                  : AppTheme.skyBlue.withValues(alpha: 0.1),
              isDark ? AppTheme.nightSurface : Colors.white
            ],
          ),
        ),
        child: RefreshIndicator(
          onRefresh: _loadReceivedBoats,
          child: _isLoading
              ? const Center(child: CircularProgressIndicator())
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
                                  '还没有收到纸船',
                                  style: TextStyle(
                                    fontSize: 16,
                                    color: Colors.grey[600],
                                  ),
                                ),
                                const SizedBox(height: 8),
                                Text(
                                  '别人会在你的石头下留言',
                                  style: TextStyle(
                                    fontSize: 14,
                                    color: Colors.grey[400],
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
                        final isAiReply = boat['is_ai_reply'] == true;
                        final senderName = isAiReply
                            ? '湖神'
                            : boat['sender_name']?.toString() ??
                                boat['author']?['nickname']?.toString() ??
                                '匿名旅人';
                        return Card(
                          margin: const EdgeInsets.only(bottom: 16),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(16),
                            side: BorderSide(
                              color: Colors.blue.withValues(alpha: 0.3),
                              width: 2,
                            ),
                          ),
                          elevation: 2,
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
                            child: Padding(
                              padding: const EdgeInsets.all(16),
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  // 发送者信息
                                  Row(
                                    children: [
                                      Icon(
                                        isAiReply
                                            ? Icons.auto_awesome
                                            : Icons.person,
                                        size: 16,
                                      ),
                                      const SizedBox(width: 8),
                                      Text(
                                        senderName,
                                        style: const TextStyle(
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                                      if (isAiReply) ...[
                                        const SizedBox(width: 8),
                                        Container(
                                          padding: const EdgeInsets.symmetric(
                                              horizontal: 6, vertical: 2),
                                          decoration: BoxDecoration(
                                            color: Colors.blue
                                                .withValues(alpha: 0.12),
                                            borderRadius:
                                                BorderRadius.circular(8),
                                          ),
                                          child: const Text(
                                            'AI回复',
                                            style: TextStyle(fontSize: 10),
                                          ),
                                        ),
                                      ],
                                      const Spacer(),
                                      Text(
                                        boat['created_at']?.toString() ?? '',
                                        style: TextStyle(
                                          fontSize: 12,
                                          color: Colors.grey[600],
                                        ),
                                      ),
                                    ],
                                  ),
                                  const SizedBox(height: 12),
                                  // 纸船内容
                                  Text(
                                    boat['content'] ?? '',
                                    style: const TextStyle(fontSize: 16),
                                  ),
                                  const Divider(height: 24),
                                  // 对应的石头
                                  Container(
                                    padding: const EdgeInsets.all(12),
                                    decoration: BoxDecoration(
                                      color: isDark
                                          ? AppTheme.nightSurface
                                          : Colors.grey[100],
                                      borderRadius: BorderRadius.circular(8),
                                    ),
                                    child: Column(
                                      crossAxisAlignment:
                                          CrossAxisAlignment.start,
                                      children: [
                                        Text(
                                          '你的石头:',
                                          style: TextStyle(
                                            fontSize: 12,
                                            color: isDark
                                                ? AppTheme.darkTextSecondary
                                                : Colors.grey[600],
                                          ),
                                        ),
                                        const SizedBox(height: 4),
                                        Text(
                                          boat['stone_content'] ?? '',
                                          style: TextStyle(
                                            fontSize: 14,
                                            color: isDark
                                                ? AppTheme.darkTextPrimary
                                                : Colors.grey[800],
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
