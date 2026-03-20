/// 好友列表界面
///
/// 展示正式好友和待处理好友请求列表。

import 'dart:math';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../widgets/water_background.dart';
import 'temp_friends_screen.dart';
import 'friend_chat_screen.dart';
import 'user_detail_screen.dart';

/// 好友列表页面
///
/// 展示当前用户的所有永久好友，支持：
/// - 下拉刷新好友列表
/// - WebSocket 实时监听好友变动（添加/删除）自动刷新
/// - 点击好友进入聊天、查看详情、复制ID、删除好友
/// - 临时好友入口（24小时有效期机制）
/// - 列表项交错淡入动画 + 漂浮小船装饰动画
class FriendsScreen extends StatefulWidget {
  const FriendsScreen({super.key});

  @override
  State<FriendsScreen> createState() => _FriendsScreenState();
}

/// 好友列表页面的状态管理
///
/// 通过 [FriendService] 加载好友数据，使用 [WebSocketManager] 监听好友变动。
/// 维护两个动画控制器：列表交错淡入动画和漂浮小船装饰动画。
class _FriendsScreenState extends State<FriendsScreen>
    with TickerProviderStateMixin {
  final FriendService _friendService = sl<FriendService>();
  List<Map<String, dynamic>> _friends = [];
  bool _isLoading = true;
  /// 列表项交错淡入动画控制器
  late AnimationController _listAnimController;
  /// 右下角漂浮小船循环动画控制器
  late AnimationController _boatAnimController;

  // 保存监听器引用以便正确移除
  late final void Function(Map<String, dynamic>) _onFriendRemoved;

  @override
  void initState() {
    super.initState();
    _listAnimController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    _boatAnimController = AnimationController(
      duration: const Duration(seconds: 5),
      vsync: this,
    )..repeat();
    _initListeners();
    _loadFriends();
    _setupWebSocketListeners();
  }

  /// 初始化 WebSocket 事件回调闭包，统一在此处创建以便 dispose 时精确移除
  void _initListeners() {
    _onFriendRemoved = (data) {
      if (mounted) _loadFriends();
    };
  }

  @override
  void dispose() {
    _removeWebSocketListeners();
    _listAnimController.dispose();
    _boatAnimController.dispose();
    super.dispose();
  }

  /// 注册 WebSocket 监听器，监听好友删除和好友请求被接受事件
  void _setupWebSocketListeners() {
    final wsManager = WebSocketManager();
    if (!wsManager.isConnected) {
      wsManager.connect();
    }
    wsManager.on('friend_removed', _onFriendRemoved);
    // 监听好友请求被接受事件，刷新好友列表
    wsManager.on('friend_accepted', _onFriendRemoved);
  }

  /// 移除所有已注册的 WebSocket 监听器，防止内存泄漏
  void _removeWebSocketListeners() {
    final wsManager = WebSocketManager();
    wsManager.off('friend_removed', _onFriendRemoved);
    wsManager.off('friend_accepted', _onFriendRemoved);
  }

  /// 从好友数据中提取用户ID
  String? _extractFriendId(Map<String, dynamic> friend) =>
      extractFriendEntityId(friend);

  /// 加载好友列表，加载完成后触发列表交错淡入动画
  Future<void> _loadFriends() async {
    if (mounted) setState(() => _isLoading = true);
    _listAnimController.reset();

    try {
      final result = await _friendService.getFriends();

      if (result['success'] && mounted) {
        final rawFriends = result['friends'];
        final friends = rawFriends is List
            ? rawFriends
                .whereType<Map>()
                .map((item) => Map<String, dynamic>.from(item))
                .toList()
            : <Map<String, dynamic>>[];
        setState(() {
          _friends = friends;
          _isLoading = false;
        });
        _listAnimController.forward();
      } else {
        if (mounted) {
          setState(() => _isLoading = false);
          final message = result['message']?.toString();
          if (message != null && message.isNotEmpty) {
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(content: Text(message)),
            );
          }
        }
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载好友列表失败')),
        );
      }
    }
  }

  /// 构建好友列表主界面，包含水面背景、漂浮小船、临时好友入口和好友列表
  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text('好友',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: Colors.white,
        actions: const [],
      ),
      body: Stack(
        children: [
          // 动态水面背景
          const Positioned.fill(child: WaterBackground()),

          // 漂浮小船装饰
          Positioned(
            right: 20,
            bottom: 100,
            child: AnimatedBuilder(
              animation: _boatAnimController,
              builder: (context, child) {
                final t = _boatAnimController.value * 2 * 3.14159;
                return Transform.translate(
                  offset: Offset(sin(t) * 8, cos(t) * 4),
                  child: const Opacity(
                    opacity: 0.6,
                    child: Text('⛵', style: TextStyle(fontSize: 32)),
                  ),
                );
              },
            ),
          ),

          // 内容区域
          RefreshIndicator(
            onRefresh: () async {
              await _loadFriends();
            },
            color: Colors.blue[900],
            backgroundColor: isDark ? const Color(0xFF16213E) : Colors.white,
            child: ListView(
              padding: EdgeInsets.only(
                top: MediaQuery.of(context).padding.top + kToolbarHeight + 16,
                bottom: 20,
                left: 16,
                right: 16,
              ),
              children: [
                // 临时好友入口
                Card(
                  color: isDark
                      ? const Color(0xFF16213E).withValues(alpha: 0.95)
                      : Colors.white.withValues(alpha: 0.95),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(16),
                    side: const BorderSide(color: Colors.orange, width: 1.5),
                  ),
                  child: ListTile(
                    leading: const CircleAvatar(
                      backgroundColor: Colors.orange,
                      child: Icon(Icons.access_time, color: Colors.white),
                    ),
                    title: const Text('临时好友'),
                    subtitle: const Text('24小时有效期，可升级为永久好友'),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () {
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => const TempFriendsScreen(),
                        ),
                      );
                    },
                  ),
                ),
                const SizedBox(height: 16),

                // 好友列表
                if (_isLoading)
                  Center(
                    child: Padding(
                      padding: const EdgeInsets.all(40),
                      child: Column(mainAxisSize: MainAxisSize.min, children: [
                        const CircularProgressIndicator(color: Colors.white),
                        const SizedBox(height: 16),
                        Text('正在寻找温暖的连接...',
                            style: TextStyle(
                                color: Colors.white.withValues(alpha: 0.8)))
                      ]),
                    ),
                  )
                else if (_friends.isEmpty)
                  Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const SizedBox(height: 40),
                        Icon(
                          Icons.people_outline,
                          size: 80,
                          color: Colors.white.withValues(alpha: 0.5),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          '还没有好友，来寻找志同道合的人吧',
                          style: TextStyle(
                            fontSize: 16,
                            color: isDark
                                ? Colors.white.withValues(alpha: 0.9)
                                : const Color(0xFF16213E)
                                    .withValues(alpha: 0.9),
                          ),
                        ),
                      ],
                    ),
                  )
                else
                  ..._friends.asMap().entries.map((entry) {
                    final index = entry.key;
                    final friend = entry.value;
                    return AnimatedBuilder(
                      animation: _listAnimController,
                      builder: (context, child) {
                        final delay = index * 0.1;
                        final start = delay.clamp(0.0, 0.7);
                        final anim =
                            Tween<double>(begin: 0.0, end: 1.0).animate(
                          CurvedAnimation(
                            parent: _listAnimController,
                            curve: Interval(
                                start, (start + 0.3).clamp(0.0, 1.0),
                                curve: Curves.easeOut),
                          ),
                        );
                        return Opacity(
                          opacity: anim.value,
                          child: Transform.translate(
                            offset: Offset(0, 20 * (1 - anim.value)),
                            child: child,
                          ),
                        );
                      },
                      child: Card(
                        margin: const EdgeInsets.only(bottom: 8),
                        color: isDark
                            ? const Color(0xFF16213E).withValues(alpha: 0.9)
                            : Colors.white.withValues(alpha: 0.9),
                        child: ListTile(
                          leading: CircleAvatar(
                            backgroundColor:
                                AppTheme.skyBlue.withValues(alpha: 0.2),
                            child: Text(
                              (friend['nickname']?.toString().isNotEmpty ==
                                      true)
                                  ? friend['nickname']
                                      .toString()
                                      .substring(0, 1)
                                  : '?',
                              style: const TextStyle(
                                color: AppTheme.skyBlue,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                          title: Text(friend['nickname'] ??
                              friend['nick_name'] ??
                              '未知'),
                          subtitle: Builder(
                            builder: (_) {
                              final username = friend['username']?.toString() ?? '';
                              final friendId = _extractFriendId(friend) ?? '';
                              return Text('账号: $username\n用户ID: $friendId');
                            },
                          ),
                          onTap: () {
                            final friendId = _extractFriendId(friend);
                            if (friendId == null) {
                              ScaffoldMessenger.of(context).showSnackBar(
                                const SnackBar(
                                    content: Text('好友标识缺失，暂时无法打开聊天')),
                              );
                              return;
                            }
                            // 点击好友进入聊天界面
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => FriendChatScreen(
                                  friendId: friendId,
                                  friendName: friend['nickname'] ??
                                      friend['nick_name'] ??
                                      '未知',
                                ),
                              ),
                            );
                          },
                          trailing: PopupMenuButton(
                            icon: const Icon(Icons.more_vert),
                            itemBuilder: (context) => [
                              const PopupMenuItem(
                                value: 'copy_id',
                                child: Text('复制用户ID'),
                              ),
                              const PopupMenuItem(
                                value: 'detail',
                                child: Text('查看详情'),
                              ),
                              const PopupMenuItem(
                                value: 'delete',
                                child: Text('删除好友'),
                              ),
                            ],
                            onSelected: (value) {
                              final friendId = _extractFriendId(friend);
                              if (value == 'copy_id') {
                                if (friendId == null || friendId.isEmpty) return;
                                Clipboard.setData(ClipboardData(text: friendId));
                                ScaffoldMessenger.of(context).showSnackBar(
                                  const SnackBar(content: Text('用户ID已复制')),
                                );
                              } else if (value == 'detail') {
                                if (friendId == null) {
                                  ScaffoldMessenger.of(context).showSnackBar(
                                    const SnackBar(
                                        content: Text('好友标识缺失，无法查看详情')),
                                  );
                                  return;
                                }
                                Navigator.push(
                                  context,
                                  MaterialPageRoute(
                                    builder: (context) => UserDetailScreen(
                                      userId: friendId,
                                      nickname: friend['nickname'],
                                    ),
                                  ),
                                );
                              } else if (value == 'delete') {
                                _confirmDeleteFriend(friendId);
                              }
                            },
                          ),
                        ),
                      ),
                    );
                  }),
              ],
            ),
          ),
        ],
      ),
    );
  }

  /// 弹出删除好友确认对话框，确认后调用后端删除接口
  void _confirmDeleteFriend(String? friendId) {
    if (friendId == null || friendId.trim().isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('好友标识缺失，无法删除')),
      );
      return;
    }
    final resolvedFriendId = friendId;

    showDialog(
      context: context,
      builder: (dialogContext) => AlertDialog(
        title: const Text('删除好友'),
        content: const Text('确定要删除这位好友吗？'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(dialogContext),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () async {
              Navigator.pop(dialogContext);

              final result =
                  await _friendService.removeFriend(resolvedFriendId);

              if (mounted) {
                if (result['success']) {
                  _loadFriends();
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('已轻轻放下')),
                  );
                } else {
                  ScaffoldMessenger.of(context).showSnackBar(
                    SnackBar(content: Text(result['message'] ?? '删除失败')),
                  );
                }
              }
            },
            child:
                const Text('确定', style: TextStyle(color: AppTheme.errorColor)),
          ),
        ],
      ),
    );
  }
}
