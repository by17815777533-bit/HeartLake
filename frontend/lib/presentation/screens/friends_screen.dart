// @file friends_screen.dart
// @brief 好友列表界面
// Created by 林子怡

import 'dart:math';
import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
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
  final FriendService _friendService = FriendService();
  List<dynamic> _friends = [];
  int _pendingCount = 0;
  bool _isLoading = true;
  late AnimationController _listAnimController;
  late AnimationController _boatAnimController;

  // 保存监听器引用以便正确移除
  late final void Function(Map<String, dynamic>) _onFriendAccepted;
  late final void Function(Map<String, dynamic>) _onFriendRemoved;
  late final void Function(Map<String, dynamic>) _onFriendRequest;

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
    _loadPendingCount();
    _setupWebSocketListeners();
  }

  void _initListeners() {
    _onFriendAccepted = (data) {
      if (mounted) {
        _loadFriends();
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('对方接受了您的好友请求')),
        );
      }
    };
    _onFriendRemoved = (data) {
      if (mounted) _loadFriends();
    };
    _onFriendRequest = (data) {
      if (mounted) {
        _loadPendingCount();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('收到新的好友请求: ${data['message'] ?? ''}')),
        );
      }
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
    wsManager.on('friend_accepted', _onFriendAccepted);
    wsManager.on('friend_removed', _onFriendRemoved);
    wsManager.on('friend_request', _onFriendRequest);
  }

  void _removeWebSocketListeners() {
    final wsManager = WebSocketManager();
    wsManager.off('friend_accepted', _onFriendAccepted);
    wsManager.off('friend_removed', _onFriendRemoved);
    wsManager.off('friend_request', _onFriendRequest);
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

  Future<void> _loadPendingCount() async {
    try {
      final result = await _friendService.getPendingRequests();
      if (result['success'] && mounted) {
        setState(() {
          _pendingCount = result['total'] ?? 0;
        });
      }
    } catch (_) {}
  }

  @override
  Widget build(BuildContext context) {
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
              await _loadPendingCount();
            },
            color: Colors.blue[900],
            backgroundColor: Colors.white,
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
                  color: Colors.white.withValues(alpha: 0.95),
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
                const SizedBox(height: 8),

                // 好友请求
                Card(
                  color: Colors.white.withValues(alpha: 0.95),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(16),
                    side: const BorderSide(
                        color: AppTheme.borderCyan, width: 1.5),
                  ),
                  child: ListTile(
                    leading: Stack(
                      children: [
                        const CircleAvatar(
                          backgroundColor: AppTheme.skyBlue,
                          child: Icon(Icons.person_add, color: Colors.white),
                        ),
                        if (_pendingCount > 0)
                          Positioned(
                            right: 0,
                            top: 0,
                            child: Container(
                              padding: const EdgeInsets.all(4),
                              decoration: const BoxDecoration(
                                color: AppTheme.errorColor,
                                shape: BoxShape.circle,
                              ),
                              child: Text(
                                _pendingCount.toString(),
                                style: const TextStyle(
                                  color: Colors.white,
                                  fontSize: 10,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ),
                          ),
                      ],
                    ),
                    title: const Text('新好友请求'),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: _showPendingRequests,
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
                            color: Colors.white.withValues(alpha: 0.9),
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
                        color: Colors.white.withValues(alpha: 0.9),
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
                          title: Text(friend['nickname'] ?? '未知'),
                          subtitle: Text('账号: ${friend['username']}'),
                          onTap: () {
                            // 点击好友进入聊天界面
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => FriendChatScreen(
                                  friendId: friend['user_id'],
                                  friendName: friend['nickname'] ?? '未知',
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

  void _showPendingRequests() {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (context) => const FriendRequestsScreen(),
      ),
    ).then((_) {
      _loadFriends();
      _loadPendingCount();
    });
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

// 好友请求页面
class FriendRequestsScreen extends StatefulWidget {
  const FriendRequestsScreen({super.key});

  @override
  State<FriendRequestsScreen> createState() => _FriendRequestsScreenState();
}

class _FriendRequestsScreenState extends State<FriendRequestsScreen> {
  final FriendService _friendService = FriendService();
  final WebSocketManager _wsManager = WebSocketManager();
  List<dynamic> _requests = [];
  bool _isLoading = true;

  // P1-5: 保存监听器引用，dispose 时精确移除
  late final void Function(Map<String, dynamic>) _onFriendRequest;

  @override
  void initState() {
    super.initState();
    _loadRequests();
    _setupWebSocketListeners();
  }

  @override
  void dispose() {
    // P1-5: 精确移除监听器，防止泄漏
    _wsManager.off('friend_request', _onFriendRequest);
    super.dispose();
  }

  void _setupWebSocketListeners() {
    _onFriendRequest = (data) {
      if (mounted) {
        _loadRequests();
      }
    };
    _wsManager.on('friend_request', _onFriendRequest);
  }

  Future<void> _loadRequests() async {
    if (mounted) setState(() => _isLoading = true);

    try {
      final result = await _friendService.getPendingRequests();

      if (result['success'] && mounted) {
        setState(() {
          _requests = result['requests'] ?? [];
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
        title: const Text('好友请求'),
        centerTitle: true,
      ),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [AppTheme.skyBlue.withValues(alpha: 0.08), Colors.white],
          ),
        ),
        child: _isLoading
            ? const Center(child: CircularProgressIndicator())
            : _requests.isEmpty
                ? Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(Icons.people_outline, size: 64, color: Colors.grey.shade300),
                        const SizedBox(height: 16),
                        Text('暂无好友请求', style: TextStyle(color: Colors.grey[600])),
                      ],
                    ),
                  )
                : ListView.builder(
                  padding: const EdgeInsets.all(16),
                  itemCount: _requests.length,
                  itemBuilder: (context, index) {
                    final request = _requests[index];
                    return Card(
                      margin: const EdgeInsets.only(bottom: 12),
                      child: Padding(
                        padding: const EdgeInsets.all(16),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              children: [
                                CircleAvatar(
                                  backgroundColor:
                                      AppTheme.skyBlue.withValues(alpha: 0.2),
                                  child: Text(
                                    (request['nickname']?.toString().isNotEmpty == true)
                                        ? request['nickname'].toString().substring(0, 1)
                                        : '?',
                                    style: const TextStyle(
                                      color: AppTheme.skyBlue,
                                      fontWeight: FontWeight.bold,
                                    ),
                                  ),
                                ),
                                const SizedBox(width: 12),
                                Expanded(
                                  child: Column(
                                    crossAxisAlignment:
                                        CrossAxisAlignment.start,
                                    children: [
                                      Text(
                                        request['nickname'] ?? '未知',
                                        style: const TextStyle(
                                          fontWeight: FontWeight.bold,
                                          fontSize: 16,
                                        ),
                                      ),
                                      Text(
                                        '账号: ${request['username']}',
                                        style: TextStyle(
                                          color: Colors.grey[600],
                                          fontSize: 12,
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 12),
                            Row(
                              mainAxisAlignment: MainAxisAlignment.end,
                              children: [
                                TextButton(
                                  onPressed: () async {
                                    final messenger = ScaffoldMessenger.of(context);
                                    final result = await _friendService
                                        .rejectFriendRequest(
                                      request['friendship_id'] ?? request['from_user_id'] ?? '',
                                    );

                                    if (mounted) {
                                      if (result['success']) {
                                        _loadRequests();
                                        messenger.showSnackBar(
                                          const SnackBar(content: Text('已拒绝')),
                                        );
                                      }
                                    }
                                  },
                                  child: const Text('拒绝'),
                                ),
                                const SizedBox(width: 8),
                                ElevatedButton(
                                  onPressed: () async {
                                    final messenger = ScaffoldMessenger.of(context);
                                    final result = await _friendService
                                        .acceptFriendRequest(
                                      request['friendship_id'] ?? request['from_user_id'] ?? '',
                                    );

                                    if (mounted) {
                                      if (result['success']) {
                                        _loadRequests();
                                        messenger.showSnackBar(
                                          const SnackBar(
                                              content: Text('已接受好友请求')),
                                        );
                                      }
                                    }
                                  },
                                  style: ElevatedButton.styleFrom(
                                    backgroundColor: AppTheme.skyBlue,
                                    foregroundColor: Colors.white,
                                  ),
                                  child: const Text('接受'),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                    );
                  },
                ),
      ),
    );
  }
}
