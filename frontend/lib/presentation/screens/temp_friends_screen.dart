// @file temp_friends_screen.dart
// @brief 临时好友列表界面 - 光遇风格重构
// Created by 林子怡

import 'package:flutter/material.dart';
import 'dart:async';
import '../../utils/app_theme.dart';
import '../../data/datasources/temp_friend_service.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../widgets/sky_button.dart';
import 'friend_chat_screen.dart';

class TempFriendsScreen extends StatefulWidget {
  const TempFriendsScreen({super.key});

  @override
  State<TempFriendsScreen> createState() => _TempFriendsScreenState();
}

class _TempFriendsScreenState extends State<TempFriendsScreen>
    with SingleTickerProviderStateMixin {
  final TempFriendService _tempFriendService = TempFriendService();
  List<dynamic> _tempFriends = [];
  bool _isLoading = true;
  Timer? _expiryTimer;
  late AnimationController _listAnimController;

  @override
  void initState() {
    super.initState();
    _listAnimController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    _loadTempFriends();
    _expiryTimer = Timer.periodic(const Duration(minutes: 1), (timer) {
      if (mounted) _loadTempFriends();
    });
  }

  @override
  void dispose() {
    _expiryTimer?.cancel();
    _listAnimController.dispose();
    super.dispose();
  }

  Future<void> _loadTempFriends() async {
    if (mounted) setState(() => _isLoading = true);
    _listAnimController.reset();

    try {
      final result = await _tempFriendService.getMyTempFriends();

      if (result['success'] && mounted) {
        setState(() {
          _tempFriends = result['temp_friends'] ?? [];
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
          const SnackBar(content: Text('加载失败，请下拉重试')),
        );
      }
    }
  }

  String _getTimeRemaining(String expiresAt) {
    try {
      final expiryTime = DateTime.parse(expiresAt);
      final now = DateTime.now();
      final diff = expiryTime.difference(now);

      if (diff.isNegative) {
        return '已过期';
      }

      if (diff.inHours > 0) {
        return '${diff.inHours}小时${diff.inMinutes.remainder(60)}分钟';
      } else if (diff.inMinutes > 0) {
        return '${diff.inMinutes}分钟';
      } else {
        return '即将过期';
      }
    } catch (e) {
      return '未知';
    }
  }

  Color _getExpiryColor(String expiresAt) {
    try {
      final expiryTime = DateTime.parse(expiresAt);
      final now = DateTime.now();
      final diff = expiryTime.difference(now);

      if (diff.isNegative) {
        return AppTheme.errorColor;
      } else if (diff.inHours < 1) {
        return AppTheme.warmOrange;
      } else {
        return AppTheme.candleGlow;
      }
    } catch (e) {
      return Colors.white.withValues(alpha: 0.3);
    }
  }

  Future<void> _upgradeToPermanent(
      String tempFriendId, String friendName) async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: AppTheme.nightSurface,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
        title: const Text('升级为永久好友',
            style: TextStyle(color: Colors.white)),
        content: Text('确定要将 $friendName 升级为永久好友吗？',
            style: TextStyle(color: Colors.white.withValues(alpha: 0.8))),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: Text('取消',
                style: TextStyle(color: Colors.white.withValues(alpha: 0.6))),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('确定',
                style: TextStyle(color: AppTheme.candleGlow)),
          ),
        ],
      ),
    );

    if (confirm == true && mounted) {
      try {
        final result =
            await _tempFriendService.upgradeToPermanent(tempFriendId);

        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content:
                  Text(result['success'] ? '升级成功！' : result['message']),
              backgroundColor:
                  result['success'] ? AppTheme.candleGlow : AppTheme.errorColor,
            ),
          );

          if (result['success']) {
            _loadTempFriends();
          }
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
                content: Text('操作失败，请稍后再试'),
                backgroundColor: AppTheme.errorColor),
          );
        }
      }
    }
  }

  Future<void> _deleteTempFriend(
      String tempFriendId, String friendName) async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: AppTheme.nightSurface,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
        title: const Text('删除临时好友',
            style: TextStyle(color: Colors.white)),
        content: Text('确定要删除临时好友 $friendName 吗？',
            style: TextStyle(color: Colors.white.withValues(alpha: 0.8))),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: Text('取消',
                style: TextStyle(color: Colors.white.withValues(alpha: 0.6))),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('删除',
                style: TextStyle(color: AppTheme.errorColor)),
          ),
        ],
      ),
    );

    if (confirm == true && mounted) {
      try {
        final result =
            await _tempFriendService.deleteTempFriend(tempFriendId);

        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content:
                  Text(result['success'] ? '删除成功' : result['message']),
              backgroundColor:
                  result['success'] ? AppTheme.candleGlow : AppTheme.errorColor,
            ),
          );

          if (result['success']) {
            _loadTempFriends();
          }
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
                content: Text('操作失败，请稍后再试'),
                backgroundColor: AppTheme.errorColor),
          );
        }
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showWater: true,
      appBar: AppBar(
        title: const Text('临时好友',
            style:
                TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: Colors.white,
        actions: [
          IconButton(
            icon: const Icon(Icons.info_outline),
            onPressed: () {
              showDialog(
                context: context,
                builder: (context) => AlertDialog(
                  backgroundColor: AppTheme.nightSurface,
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(20)),
                  title: const Text('关于临时好友',
                      style: TextStyle(color: Colors.white)),
                  content: Text(
                    '临时好友是通过互动（评论、纸船）自动建立的24小时好友关系。\n\n'
                    '• 临时好友可以私聊\n'
                    '• 24小时后自动失效\n'
                    '• 可以升级为永久好友\n\n'
                    '在心湖中多互动，遇见更多朋友吧！',
                    style: TextStyle(
                        color: Colors.white.withValues(alpha: 0.8),
                        height: 1.5),
                  ),
                  actions: [
                    TextButton(
                      onPressed: () => Navigator.pop(context),
                      child: const Text('知道了',
                          style: TextStyle(color: AppTheme.candleGlow)),
                    ),
                  ],
                ),
              );
            },
          ),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: _loadTempFriends,
        color: AppTheme.warmOrange,
        backgroundColor: AppTheme.nightSurface,
        child: _isLoading
            ? const Center(
                child: CircularProgressIndicator(color: AppTheme.warmOrange))
            : _tempFriends.isEmpty
                ? ListView(
                    children: [
                      SizedBox(
                          height: MediaQuery.of(context).size.height * 0.3),
                      Center(
                        child: Column(
                          children: [
                            Icon(Icons.people_outline,
                                size: 64,
                                color: Colors.white.withValues(alpha: 0.3)),
                            const SizedBox(height: 16),
                            Text('还没有临时好友',
                                style: TextStyle(
                                    color:
                                        Colors.white.withValues(alpha: 0.5),
                                    fontSize: 16)),
                            const SizedBox(height: 8),
                            Text('去心湖互动，遇见新朋友吧！',
                                style: TextStyle(
                                    color:
                                        Colors.white.withValues(alpha: 0.3),
                                    fontSize: 14)),
                          ],
                        ),
                      ),
                    ],
                  )
                : ListView.builder(
                    padding: EdgeInsets.only(
                      top: MediaQuery.of(context).padding.top + kToolbarHeight + 8,
                      bottom: 20,
                      left: 16,
                      right: 16,
                    ),
                    itemCount: _tempFriends.length + 1,
                    itemBuilder: (context, index) {
                      if (index == 0) {
                        return Padding(
                          padding: const EdgeInsets.only(bottom: 12),
                          child: SkyGlassCard(
                            glowColor: AppTheme.spiritBlue,
                            enableGlow: false,
                            child: Row(
                              children: [
                                Icon(Icons.info_outline,
                                    color: AppTheme.spiritBlue, size: 20),
                                const SizedBox(width: 12),
                                Expanded(
                                  child: Text(
                                    '临时好友24小时后失效，可升级为永久好友',
                                    style: TextStyle(
                                        color: Colors.white
                                            .withValues(alpha: 0.7),
                                        fontSize: 13),
                                  ),
                                ),
                              ],
                            ),
                          ),
                        );
                      }
                      return _buildFriendCard(
                          _tempFriends[index - 1], index - 1);
                    },
                  ),
      ),
    );
  }

  Widget _buildFriendCard(dynamic tempFriend, int index) {
    final expiresAt = tempFriend['expires_at'] ?? '';
    final isExpired = tempFriend['status'] == 'expired';
    final isUpgraded = tempFriend['status'] == 'upgraded';
    final delay = (index * 0.1).clamp(0.0, 1.0);

    return AnimatedBuilder(
      animation: _listAnimController,
      builder: (context, child) {
        final progress =
            ((_listAnimController.value - delay) / (1 - delay)).clamp(0.0, 1.0);
        return Transform.translate(
          offset: Offset(0, 30 * (1 - progress)),
          child: Opacity(opacity: progress, child: child),
        );
      },
      child: Padding(
        padding: const EdgeInsets.only(bottom: 10),
        child: SkyGlassCard(
          enableGlow: !isExpired && !isUpgraded,
          glowColor: isExpired
              ? Colors.white.withValues(alpha: 0.1)
              : AppTheme.spiritBlue,
          onTap: !isExpired && !isUpgraded
              ? () => Navigator.push(
                  context,
                  MaterialPageRoute(
                      builder: (context) => FriendChatScreen(
                          friendId: tempFriend['friend_id'],
                          friendName:
                              tempFriend['friend_nickname'] ?? '未知')))
              : null,
          child: Row(
            children: [
              // 头像区域
              Stack(
                children: [
                  CircleAvatar(
                    radius: 24,
                    backgroundColor: isExpired
                        ? Colors.white.withValues(alpha: 0.1)
                        : AppTheme.spiritBlue.withValues(alpha: 0.2),
                    child: Text(
                      (tempFriend['friend_nickname']?.isNotEmpty == true)
                          ? tempFriend['friend_nickname'].substring(0, 1)
                          : '?',
                      style: TextStyle(
                          color: isExpired
                              ? Colors.white.withValues(alpha: 0.3)
                              : AppTheme.spiritBlue,
                          fontWeight: FontWeight.bold,
                          fontSize: 20),
                    ),
                  ),
                  if (!isExpired && !isUpgraded)
                    Positioned(
                      right: 0,
                      bottom: 0,
                      child: Container(
                        padding: const EdgeInsets.all(3),
                        decoration: BoxDecoration(
                            color: _getExpiryColor(expiresAt),
                            shape: BoxShape.circle,
                            border: Border.all(
                                color: AppTheme.nightSurface, width: 2)),
                        child: const Icon(Icons.access_time,
                            color: Colors.white, size: 10),
                      ),
                    ),
                ],
              ),
              const SizedBox(width: 14),
              // 信息区域
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        Expanded(
                          child: Text(
                            tempFriend['friend_nickname'] ?? '未知',
                            style: TextStyle(
                                fontWeight: FontWeight.bold,
                                fontSize: 15,
                                color: isExpired
                                    ? Colors.white.withValues(alpha: 0.3)
                                    : Colors.white),
                          ),
                        ),
                        if (isUpgraded)
                          Container(
                            padding: const EdgeInsets.symmetric(
                                horizontal: 8, vertical: 3),
                            decoration: BoxDecoration(
                                color:
                                    AppTheme.candleGlow.withValues(alpha: 0.15),
                                borderRadius: BorderRadius.circular(12)),
                            child: const Text('已升级',
                                style: TextStyle(
                                    fontSize: 11,
                                    color: AppTheme.candleGlow,
                                    fontWeight: FontWeight.bold)),
                          ),
                      ],
                    ),
                    const SizedBox(height: 4),
                    Text(
                      '来源: ${_getSourceText(tempFriend['source'])}',
                      style: TextStyle(
                          fontSize: 12,
                          color: Colors.white.withValues(alpha: 0.5)),
                    ),
                    const SizedBox(height: 4),
                    Row(
                      children: [
                        Icon(Icons.timer_outlined,
                            size: 14, color: _getExpiryColor(expiresAt)),
                        const SizedBox(width: 4),
                        Text(
                          isExpired
                              ? '已过期'
                              : '剩余: ${_getTimeRemaining(expiresAt)}',
                          style: TextStyle(
                              fontSize: 12,
                              color: _getExpiryColor(expiresAt),
                              fontWeight: FontWeight.w500),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
              // 操作菜单
              if (!isExpired && !isUpgraded)
                PopupMenuButton(
                  icon: Icon(Icons.more_vert,
                      color: Colors.white.withValues(alpha: 0.6)),
                  color: AppTheme.nightSurface,
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12)),
                  itemBuilder: (context) => [
                    PopupMenuItem(
                        value: 'chat',
                        child: Row(children: [
                          Icon(Icons.chat,
                              size: 20,
                              color: Colors.white.withValues(alpha: 0.8)),
                          const SizedBox(width: 8),
                          Text('私聊',
                              style: TextStyle(
                                  color:
                                      Colors.white.withValues(alpha: 0.9))),
                        ])),
                    PopupMenuItem(
                        value: 'upgrade',
                        child: Row(children: [
                          const Icon(Icons.upgrade,
                              color: AppTheme.candleGlow, size: 20),
                          const SizedBox(width: 8),
                          const Text('升级为永久好友',
                              style: TextStyle(color: AppTheme.candleGlow)),
                        ])),
                    PopupMenuItem(
                        value: 'delete',
                        child: Row(children: [
                          const Icon(Icons.delete,
                              color: AppTheme.errorColor, size: 20),
                          const SizedBox(width: 8),
                          const Text('删除',
                              style: TextStyle(color: AppTheme.errorColor)),
                        ])),
                  ],
                  onSelected: (value) {
                    switch (value) {
                      case 'chat':
                        Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (context) => FriendChatScreen(
                                    friendId: tempFriend['friend_id'],
                                    friendName:
                                        tempFriend['friend_nickname'] ??
                                            '未知')));
                        break;
                      case 'upgrade':
                        _upgradeToPermanent(tempFriend['temp_friend_id'],
                            tempFriend['friend_nickname'] ?? '未知');
                        break;
                      case 'delete':
                        _deleteTempFriend(tempFriend['temp_friend_id'],
                            tempFriend['friend_nickname'] ?? '未知');
                        break;
                    }
                  },
                ),
            ],
          ),
        ),
      ),
    );
  }

  String _getSourceText(String? source) {
    switch (source) {
      case 'comment':
        return '评论互动';
      case 'boat':
        return '纸船互动';
      case 'chat':
        return '聊天';
      default:
        return '未知';
    }
  }
}
