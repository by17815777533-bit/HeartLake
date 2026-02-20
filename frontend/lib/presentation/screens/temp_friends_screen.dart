// @file temp_friends_screen.dart
// @brief 临时好友列表界面
// Created by 林子怡

import 'package:flutter/material.dart';
import 'dart:async';
import '../../utils/app_theme.dart';
import '../../data/datasources/temp_friend_service.dart';
import '../widgets/water_background.dart';
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
        return Colors.red;
      } else if (diff.inHours < 1) {
        return Colors.orange;
      } else {
        return Colors.green;
      }
    } catch (e) {
      return Colors.grey;
    }
  }

  Future<void> _upgradeToPermanent(
      String tempFriendId, String friendName) async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('升级为永久好友'),
        content: Text('确定要将 $friendName 升级为永久好友吗？'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('确定'),
          ),
        ],
      ),
    );

    if (confirm == true && mounted) {
      try {
        final result = await _tempFriendService.upgradeToPermanent(tempFriendId);

        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(result['success'] ? '升级成功！' : result['message']),
              backgroundColor: result['success'] ? Colors.green : Colors.red,
            ),
          );

          if (result['success']) {
            _loadTempFriends();
          }
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('操作失败，请稍后再试'), backgroundColor: Colors.red),
          );
        }
      }
    }
  }

  Future<void> _deleteTempFriend(String tempFriendId, String friendName) async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('删除临时好友'),
        content: Text('确定要删除临时好友 $friendName 吗？'),
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

    if (confirm == true && mounted) {
      try {
        final result = await _tempFriendService.deleteTempFriend(tempFriendId);

        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(result['success'] ? '删除成功' : result['message']),
              backgroundColor: result['success'] ? Colors.green : Colors.red,
            ),
          );

          if (result['success']) {
            _loadTempFriends();
          }
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('操作失败，请稍后再试'), backgroundColor: Colors.red),
          );
        }
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text('临时好友',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
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
                  title: const Text('关于临时好友'),
                  content: const Text(
                    '临时好友是通过互动（评论、纸船）自动建立的24小时好友关系。\n\n'
                    '• 临时好友可以私聊\n'
                    '• 24小时后自动失效\n'
                    '• 可以升级为永久好友\n\n'
                    '在心湖中多互动，遇见更多朋友吧！',
                  ),
                  actions: [
                    TextButton(
                      onPressed: () => Navigator.pop(context),
                      child: const Text('知道了'),
                    ),
                  ],
                ),
              );
            },
          ),
        ],
      ),
      body: Stack(
        children: [
          // 动态水面背景
          const Positioned.fill(child: WaterBackground()),

          // 内容区域
          RefreshIndicator(
            onRefresh: _loadTempFriends,
            color: Colors.blue[900],
            backgroundColor: Colors.white,
            child: ListView.builder(
              padding: EdgeInsets.only(
                top: MediaQuery.of(context).padding.top + kToolbarHeight + 16,
                bottom: 20,
                left: 16,
                right: 16,
              ),
              itemCount: _isLoading ? 2 : (_tempFriends.isEmpty ? 2 : _tempFriends.length + 1),
              itemBuilder: (context, index) {
                // 第一项：提示卡片
                if (index == 0) {
                  return Column(
                    children: [
                      Card(
                        color: Colors.white.withOpacity(0.95),
                        elevation: 4,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(16),
                        ),
                        child: Padding(
                          padding: const EdgeInsets.all(16),
                          child: Row(
                            children: [
                              Container(
                                padding: const EdgeInsets.all(12),
                                decoration: BoxDecoration(
                                  color: AppTheme.skyBlue.withOpacity(0.1),
                                  borderRadius: BorderRadius.circular(12),
                                ),
                                child: const Icon(
                                  Icons.access_time,
                                  color: AppTheme.skyBlue,
                                  size: 28,
                                ),
                              ),
                              const SizedBox(width: 16),
                              const Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Text(
                                      '临时好友关系',
                                      style: TextStyle(
                                        fontSize: 16,
                                        fontWeight: FontWeight.bold,
                                        color: AppTheme.darkBlue,
                                      ),
                                    ),
                                    SizedBox(height: 4),
                                    Text(
                                      '通过互动自动建立，24小时有效期',
                                      style: TextStyle(
                                        fontSize: 13,
                                        color: Colors.grey,
                                      ),
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                      const SizedBox(height: 16),
                    ],
                  );
                }

                // 加载中状态
                if (_isLoading) {
                  return const Center(
                    child: Padding(
                      padding: EdgeInsets.all(40),
                      child: CircularProgressIndicator(color: Colors.white),
                    ),
                  );
                }

                // 空状态
                if (_tempFriends.isEmpty) {
                  return Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const SizedBox(height: 40),
                        Icon(
                          Icons.people_outline,
                          size: 80,
                          color: Colors.white.withOpacity(0.5),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          '还没有临时好友',
                          style: TextStyle(
                            fontSize: 16,
                            color: Colors.white.withOpacity(0.9),
                          ),
                        ),
                        const SizedBox(height: 8),
                        Text(
                          '在心湖中互动即可建立临时好友关系',
                          style: TextStyle(
                            fontSize: 14,
                            color: Colors.white.withOpacity(0.7),
                          ),
                        ),
                      ],
                    ),
                  );
                }

                // 好友列表项
                final friendIndex = index - 1;
                final tempFriend = _tempFriends[friendIndex];
                final status = tempFriend['status'] ?? 'active';
                final expiresAt = tempFriend['expires_at'] ?? '';
                final isExpired = status == 'expired';
                final isUpgraded = tempFriend['upgraded_to_friend'] == true;

                return _buildFriendCard(friendIndex, tempFriend, isExpired, isUpgraded, expiresAt);
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildFriendCard(int index, dynamic tempFriend, bool isExpired, bool isUpgraded, String expiresAt) {
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
        margin: const EdgeInsets.only(bottom: 12),
        color: Colors.white.withOpacity(0.95),
        elevation: 2,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: BorderSide(
            color: isExpired ? Colors.grey.withOpacity(0.3) : AppTheme.skyBlue.withOpacity(0.3),
            width: 1,
          ),
        ),
        child: ListTile(
          contentPadding: const EdgeInsets.all(12),
          leading: Stack(
            children: [
              CircleAvatar(
                radius: 28,
                backgroundColor: isExpired ? Colors.grey.withOpacity(0.2) : AppTheme.skyBlue.withOpacity(0.2),
                child: Text(
                  (tempFriend['friend_nickname']?.isNotEmpty == true) ? tempFriend['friend_nickname'].substring(0, 1) : '?',
                  style: TextStyle(color: isExpired ? Colors.grey : AppTheme.skyBlue, fontWeight: FontWeight.bold, fontSize: 20),
                ),
              ),
              if (!isExpired && !isUpgraded)
                Positioned(
                  right: 0,
                  bottom: 0,
                  child: Container(
                    padding: const EdgeInsets.all(4),
                    decoration: BoxDecoration(color: _getExpiryColor(expiresAt), shape: BoxShape.circle, border: Border.all(color: Colors.white, width: 2)),
                    child: const Icon(Icons.access_time, color: Colors.white, size: 12),
                  ),
                ),
            ],
          ),
          title: Row(
            children: [
              Expanded(child: Text(tempFriend['friend_nickname'] ?? '未知', style: TextStyle(fontWeight: FontWeight.bold, color: isExpired ? Colors.grey : AppTheme.darkBlue))),
              if (isUpgraded)
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(color: Colors.green.withOpacity(0.1), borderRadius: BorderRadius.circular(12)),
                  child: const Text('已升级', style: TextStyle(fontSize: 11, color: Colors.green, fontWeight: FontWeight.bold)),
                ),
            ],
          ),
          subtitle: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const SizedBox(height: 4),
              Text('来源: ${_getSourceText(tempFriend['source'])}', style: TextStyle(fontSize: 12, color: Colors.grey[600])),
              const SizedBox(height: 4),
              Row(
                children: [
                  Icon(Icons.timer_outlined, size: 14, color: _getExpiryColor(expiresAt)),
                  const SizedBox(width: 4),
                  Text(isExpired ? '已过期' : '剩余: ${_getTimeRemaining(expiresAt)}', style: TextStyle(fontSize: 12, color: _getExpiryColor(expiresAt), fontWeight: FontWeight.w500)),
                ],
              ),
            ],
          ),
          trailing: !isExpired && !isUpgraded
              ? PopupMenuButton(
                  icon: const Icon(Icons.more_vert),
                  itemBuilder: (context) => [
                    const PopupMenuItem(value: 'chat', child: Row(children: [Icon(Icons.chat, size: 20), SizedBox(width: 8), Text('私聊')])),
                    const PopupMenuItem(value: 'upgrade', child: Row(children: [Icon(Icons.upgrade, color: Colors.green, size: 20), SizedBox(width: 8), Text('升级为永久好友', style: TextStyle(color: Colors.green))])),
                    const PopupMenuItem(value: 'delete', child: Row(children: [Icon(Icons.delete, color: Colors.red, size: 20), SizedBox(width: 8), Text('删除', style: TextStyle(color: Colors.red))])),
                  ],
                  onSelected: (value) {
                    switch (value) {
                      case 'chat':
                        Navigator.push(context, MaterialPageRoute(builder: (context) => FriendChatScreen(friendId: tempFriend['friend_id'], friendName: tempFriend['friend_nickname'] ?? '未知')));
                        break;
                      case 'upgrade':
                        _upgradeToPermanent(tempFriend['temp_friend_id'], tempFriend['friend_nickname'] ?? '未知');
                        break;
                      case 'delete':
                        _deleteTempFriend(tempFriend['temp_friend_id'], tempFriend['friend_nickname'] ?? '未知');
                        break;
                    }
                  },
                )
              : null,
          onTap: !isExpired && !isUpgraded
              ? () => Navigator.push(context, MaterialPageRoute(builder: (context) => FriendChatScreen(friendId: tempFriend['friend_id'], friendName: tempFriend['friend_nickname'] ?? '未知')))
              : null,
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
