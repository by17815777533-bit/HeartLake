// @file friends_screen.dart
// @brief 好友列表界面 - 光遇风格重构
// Created by 林子怡

import 'dart:math';
import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../widgets/sky_button.dart';
import '../../utils/animation_utils.dart';
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
  late final void Function(Map<String, dynamic>) _onReconnected;

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
    _onReconnected = (data) {
      if (mounted) {
        _loadFriends();
        _loadPendingCount();
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
    wsManager.on('reconnected', _onReconnected);
  }

  void _removeWebSocketListeners() {
    final wsManager = WebSocketManager();
    wsManager.off('friend_accepted', _onFriendAccepted);
    wsManager.off('friend_removed', _onFriendRemoved);
    wsManager.off('friend_request', _onFriendRequest);
    wsManager.off('reconnected', _onReconnected);
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
          final requests = result['requests'];
          _pendingCount = requests is List ? requests.length : 0;
        });
      }
    } catch (_) {}
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showWater: true,
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
          // 小船动画
          Positioned(
            right: 20,
            bottom: 100,
            child: AnimatedBuilder(
              animation: _boatAnimController,
              builder: (context, child) {
                return Transform.translate(
                  offset: Offset(
                    sin(_boatAnimController.value * 2 * pi) * 10,
                    cos(_boatAnimController.value * 2 * pi) * 5,
                  ),
                  child: Transform.rotate(
                    angle: sin(_boatAnimController.value * 2 * pi) * 0.05,
                    child: child,
                  ),
                );
              },
              child: Text(
                '⛵',
                style: TextStyle(
                  fontSize: 40,
                  shadows: [
                    Shadow(
                      color: Colors.white.withValues(alpha: 0.5),
                      blurRadius: 10,
                    ),
                  ],
                ),
              ),
            ),
          ),
          // 内容
          RefreshIndicator(
            onRefresh: () async {
              await _loadFriends();
              await _loadPendingCount();
            },
            color: AppTheme.primaryColor,
            backgroundColor: AppTheme.lightStone,
            child: ListView(
              padding: EdgeInsets.only(
                top: MediaQuery.of(context).padding.top + kToolbarHeight + 16,
                bottom: 20,
                left: 16,
                right: 16,
              ),
              children: [
                // 临时好友入口
                SkyGlassCard(
                  enableGlow: false,
                  padding: EdgeInsets.zero,
                  onTap: () {
                    Navigator.push(
                      context,
                      SkyPageRoute(page: const TempFriendsScreen()),
                    );
                  },
                  child: ListTile(
                    leading: CircleAvatar(
                      backgroundColor: AppTheme.primaryColor.withValues(alpha: 0.2),
                      child: const Icon(Icons.timer, color: AppTheme.warmOrange),
                    ),
                    title: const Text('临时好友',
                        style: TextStyle(color: Colors.white)),
                    subtitle: Text('查看限时好友',
                        style: TextStyle(
                            color: Colors.white.withValues(alpha: 0.6))),
                    trailing: Icon(Icons.chevron_right,
                        color: Colors.white.withValues(alpha: 0.6)),
                  ),
                ),
                const SizedBox(height: 12),

                // 好友请求入口
                SkyGlassCard(
                  enableGlow: false,
                  padding: EdgeInsets.zero,
                  onTap: _showPendingRequests,
                  child: ListTile(
                    leading: Stack(
                      clipBehavior: Clip.none,
                      children: [
                        CircleAvatar(
                          backgroundColor:
                              AppTheme.secondaryColor.withValues(alpha: 0.2),
                          child: const Icon(Icons.person_add,
                              color: AppTheme.secondaryColor),
                        ),
                        if (_pendingCount > 0)
                          Positioned(
                            right: -2,
                            top: -2,
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
                    title: const Text('新好友请求',
                        style: TextStyle(color: Colors.white)),
                    trailing: Icon(Icons.chevron_right,
                        color: Colors.white.withValues(alpha: 0.6)),
                  ),
                ),
                const SizedBox(height: 16),

                // 好友列表
                if (_isLoading)
                  Center(
                    child: Padding(
                      padding: const EdgeInsets.all(40),
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const CircularProgressIndicator(
                              color: AppTheme.primaryColor),
                          const SizedBox(height: 16),
                          Text('正在寻找温暖的连接...',
                              style: TextStyle(
                                  color: Colors.white.withValues(alpha: 0.8))),
                        ],
                      ),
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
                        final anim =
                            Tween<double>(begin: 0.0, end: 1.0).animate(
                          CurvedAnimation(
                            parent: _listAnimController,
                            curve: Interval(start,
                                (start + 0.3).clamp(0.0, 1.0),
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
                      child: Padding(
                        padding: const EdgeInsets.only(bottom: 8),
                        child: SkyGlassCard(
                          enableGlow: false,
                          padding: EdgeInsets.zero,
                          onTap: () {
                            Navigator.push(
                              context,
                              SkyPageRoute(page: FriendChatScreen(
                                  friendId: friend['user_id'],
                                  friendName: friend['nickname'] ?? '未知',
                                )),
                            );
                          },
                          child: ListTile(
                            leading: CircleAvatar(
                              backgroundColor:
                                  AppTheme.secondaryColor.withValues(alpha: 0.2),
                              child: Text(
                                (friend['nickname']
                                            ?.toString()
                                            .isNotEmpty ==
                                        true)
                                    ? friend['nickname']
                                        .toString()
                                        .substring(0, 1)
                                    : '?',
                                style: const TextStyle(
                                  color: AppTheme.secondaryColor,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ),
                            title: Text(friend['nickname'] ?? '未知',
                                style:
                                    const TextStyle(color: Colors.white)),
                            subtitle: Text(
                                '账号: ${friend['username']}',
                                style: TextStyle(
                                    color: Colors.white
                                        .withValues(alpha: 0.6))),
                            trailing: PopupMenuButton(
                              icon: Icon(Icons.more_vert,
                                  color:
                                      Colors.white.withValues(alpha: 0.7)),
                              color: AppTheme.lightStone,
                              itemBuilder: (context) => [
                                const PopupMenuItem(
                                  value: 'detail',
                                  child: Row(
                                    children: [
                                      Icon(Icons.info_outline,
                                          size: 18, color: Colors.white70),
                                      SizedBox(width: 8),
                                      Text('查看资料',
                                          style: TextStyle(
                                              color: Colors.white)),
                                    ],
                                  ),
                                ),
                                const PopupMenuItem(
                                  value: 'delete',
                                  child: Row(
                                    children: [
                                      Icon(Icons.delete_outline,
                                          size: 18,
                                          color: AppTheme.errorColor),
                                      SizedBox(width: 8),
                                      Text('删除好友',
                                          style: TextStyle(
                                              color:
                                                  AppTheme.errorColor)),
                                    ],
                                  ),
                                ),
                              ],
                              onSelected: (value) {
                                if (value == 'detail') {
                                  Navigator.push(
                                    context,
                                    SkyPageRoute(page: UserDetailScreen(
                                        userId: friend['user_id'],
                                      )),
                                  );
                                } else if (value == 'delete') {
                                  _confirmDeleteFriend(
                                    friend['user_id'],
                                    friend['nickname'] ?? '未知',
                                  );
                                }
                              },
                            ),
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
      SkyPageRoute(page: const FriendRequestsScreen()),
    ).then((_) {
      _loadFriends();
      _loadPendingCount();
    });
  }

  void _confirmDeleteFriend(String friendId, String nickname) {
    showDialog(
      context: context,
      builder: (dialogContext) => AlertDialog(
        backgroundColor: AppTheme.lightStone,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
        title: const Text('删除好友',
            style: TextStyle(color: Colors.white)),
        content: Text('确定要删除 $nickname 吗？',
            style: TextStyle(color: Colors.white.withValues(alpha: 0.8))),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(dialogContext),
            child: Text('取消',
                style: TextStyle(
                    color: Colors.white.withValues(alpha: 0.6))),
          ),
          TextButton(
            onPressed: () async {
              Navigator.pop(dialogContext);
              final result =
                  await _friendService.removeFriend(friendId);
              if (mounted) {
                if (result['success']) {
                  _loadFriends();
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('已轻轻放下')),
                  );
                } else {
                  ScaffoldMessenger.of(context).showSnackBar(
                    SnackBar(
                        content:
                            Text(result['message'] ?? '删除失败')),
                  );
                }
              }
            },
            child: const Text('确定',
                style: TextStyle(color: AppTheme.errorColor)),
          ),
        ],
      ),
    );
  }
}

// ============================================================
// 好友请求页面
// ============================================================

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

  late final void Function(Map<String, dynamic>) _onFriendRequest;

  @override
  void initState() {
    super.initState();
    _loadRequests();
    _setupWebSocketListeners();
  }

  @override
  void dispose() {
    _wsManager.off('friend_request', _onFriendRequest);
    super.dispose();
  }

  void _setupWebSocketListeners() {
    _onFriendRequest = (data) {
      if (mounted) _loadRequests();
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
        if (mounted) setState(() => _isLoading = false);
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载好友请求失败')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showWater: true,
      appBar: AppBar(
        title: const Text('好友请求',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: Colors.white,
      ),
      body: _isLoading
          ? const Center(
              child: CircularProgressIndicator(color: AppTheme.primaryColor))
          : _requests.isEmpty
              ? Center(
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(Icons.people_outline,
                          size: 64,
                          color: Colors.white.withValues(alpha: 0.4)),
                      const SizedBox(height: 16),
                      Text('暂无好友请求',
                          style: TextStyle(
                              color: Colors.white.withValues(alpha: 0.6),
                              fontSize: 16)),
                    ],
                  ),
                )
              : ListView.builder(
                  padding: EdgeInsets.only(
                    top: MediaQuery.of(context).padding.top +
                        kToolbarHeight +
                        16,
                    left: 16,
                    right: 16,
                    bottom: 20,
                  ),
                  itemCount: _requests.length,
                  itemBuilder: (context, index) {
                    final request = _requests[index];
                    final nickname = request['nickname'] ?? '未知';
                    final initial =
                        nickname.isNotEmpty ? nickname.substring(0, 1) : '?';

                    return Padding(
                      padding: const EdgeInsets.only(bottom: 12),
                      child: SkyGlassCard(
                        enableGlow: false,
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              children: [
                                CircleAvatar(
                                  backgroundColor:
                                      AppTheme.secondaryColor.withValues(alpha: 0.2),
                                  child: Text(
                                    initial,
                                    style: const TextStyle(
                                      color: AppTheme.secondaryColor,
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
                                        nickname,
                                        style: const TextStyle(
                                          fontWeight: FontWeight.bold,
                                          fontSize: 16,
                                          color: Colors.white,
                                        ),
                                      ),
                                      Text(
                                        '账号: ${request['username']}',
                                        style: TextStyle(
                                          color: Colors.white
                                              .withValues(alpha: 0.6),
                                          fontSize: 13,
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
                                    final messenger =
                                        ScaffoldMessenger.of(context);
                                    final result = await _friendService
                                        .rejectFriendRequest(
                                      request['requester_id'],
                                    );
                                    if (mounted) {
                                      if (result['success']) {
                                        _loadRequests();
                                        messenger.showSnackBar(
                                          const SnackBar(
                                              content: Text('已拒绝好友请求')),
                                        );
                                      }
                                    }
                                  },
                                  child: Text('拒绝',
                                      style: TextStyle(
                                          color: Colors.white
                                              .withValues(alpha: 0.6))),
                                ),
                                const SizedBox(width: 8),
                                SkyButton(
                                  label: '接受',
                                  onPressed: () async {
                                    final messenger =
                                        ScaffoldMessenger.of(context);
                                    final result = await _friendService
                                        .acceptFriendRequest(
                                      request['requester_id'],
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
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                    );
                  },
                ),
    );
  }
}
