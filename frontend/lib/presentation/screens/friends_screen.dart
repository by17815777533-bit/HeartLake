// @file friends_screen.dart
// @brief 好友列表界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../widgets/water_background.dart';
import 'temp_friends_screen.dart';
import 'friend_chat_screen.dart';
import 'user_detail_screen.dart';

class FriendsScreen extends StatefulWidget {
  const FriendsScreen({super.key});

  @override
  State<FriendsScreen> createState() => _FriendsScreenState();
}

class _FriendsScreenState extends State<FriendsScreen>
    with TickerProviderStateMixin {
  final FriendService _friendService = sl<FriendService>();
  List<Map<String, dynamic>> _friends = [];
  bool _isLoading = true;
  late AnimationController _listAnimController;
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

  void _setupWebSocketListeners() {
    final wsManager = WebSocketManager();
    if (!wsManager.isConnected) {
      wsManager.connect();
    }
    wsManager.on('friend_removed', _onFriendRemoved);
    // 监听好友请求被接受事件，刷新好友列表
    wsManager.on('friend_accepted', _onFriendRemoved);
  }

  void _removeWebSocketListeners() {
    final wsManager = WebSocketManager();
    wsManager.off('friend_removed', _onFriendRemoved);
    wsManager.off('friend_accepted', _onFriendRemoved);
  }

  Future<void> _loadFriends() async {
    if (mounted) setState(() => _isLoading = true);
    _listAnimController.reset();

    try {
      final result = await _friendService.getFriends();

      if (result['success'] && mounted) {
        setState(() {
          _friends = result['friends'] ?? [];
          _isLoading = false;
        });
        _listAnimController.forward();
      } else {
        if (mounted) setState(() => _isLoading = false);
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
                  color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.95) : Colors.white.withValues(alpha: 0.95),
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
                      child: Column(mainAxisSize: MainAxisSize.min, children: [const CircularProgressIndicator(color: Colors.white), const SizedBox(height: 16), Text('正在寻找温暖的连接...', style: TextStyle(color: Colors.white.withValues(alpha: 0.8)))]),
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
                            color: isDark ? Colors.white.withValues(alpha: 0.9) : const Color(0xFF16213E).withValues(alpha: 0.9),
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
                        final anim = Tween<double>(begin: 0.0, end: 1.0).animate(
                          CurvedAnimation(
                            parent: _listAnimController,
                            curve: Interval(start, (start + 0.3).clamp(0.0, 1.0), curve: Curves.easeOut),
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
                        color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.9) : Colors.white.withValues(alpha: 0.9),
                        child: ListTile(
                          leading: CircleAvatar(
                            backgroundColor: AppTheme.skyBlue.withValues(alpha: 0.2),
                            child: Text(
                              (friend['nickname']?.toString().isNotEmpty == true)
                                  ? friend['nickname'].toString().substring(0, 1)
                                  : '?',
                              style: const TextStyle(
                                color: AppTheme.skyBlue,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                          title: Text(friend['nickname'] ?? friend['nick_name'] ?? '未知'),
                          subtitle: Text('账号: ${friend['username'] ?? ''}'),
                          onTap: () {
                            // 点击好友进入聊天界面
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => FriendChatScreen(
                                  friendId: friend['user_id'] ?? friend['userId'] ?? '',
                                  friendName: friend['nickname'] ?? friend['nick_name'] ?? '未知',
                                ),
                              ),
                            );
                          },
                          trailing: PopupMenuButton(
                            icon: const Icon(Icons.more_vert),
                            itemBuilder: (context) => [
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
                              if (value == 'detail') {
                                Navigator.push(
                                  context,
                                  MaterialPageRoute(
                                    builder: (context) => UserDetailScreen(
                                      userId: friend['user_id'],
                                      nickname: friend['nickname'],
                                    ),
                                  ),
                                );
                              } else if (value == 'delete') {
                                _confirmDeleteFriend(friend['user_id']);
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

  void _confirmDeleteFriend(String friendId) {
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

              final result = await _friendService.removeFriend(friendId);

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
            child: const Text('确定', style: TextStyle(color: AppTheme.errorColor)),
          ),
        ],
      ),
    );
  }
}
