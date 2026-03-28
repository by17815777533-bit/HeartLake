// 临时好友列表界面
//
// 展示24小时限时好友列表。

import 'package:flutter/material.dart';
import 'dart:async';
import 'package:provider/provider.dart';
import '../../utils/app_theme.dart';
import '../providers/friend_provider.dart';
import '../widgets/water_background.dart';
import 'friend_chat_screen.dart';

/// 临时好友列表页面
///
/// 临时好友是通过纸船互动建立的 24 小时有效期关系。
/// 页面展示所有临时好友及剩余有效时间，支持点击进入聊天，
/// 并在到期后自动移除（定时器刷新）。
class TempFriendsScreen extends StatefulWidget {
  const TempFriendsScreen({super.key});

  @override
  State<TempFriendsScreen> createState() => _TempFriendsScreenState();
}

/// 临时好友列表页面状态管理
///
/// 复用 [FriendProvider] 中的临时好友状态，只在页面维护展示级倒计时和动画。
class _TempFriendsScreenState extends State<TempFriendsScreen>
    with SingleTickerProviderStateMixin {
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
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) {
        _loadTempFriends();
      }
    });
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

  /// 加载临时好友列表，重置列表动画
  Future<void> _loadTempFriends() async {
    if (mounted) setState(() => _isLoading = true);
    _listAnimController.reset();
    try {
      final result = await context.read<FriendProvider>().fetchTempFriends();
      if (!mounted) return;

      setState(() => _isLoading = false);
      if (result['success'] == true) {
        _listAnimController.forward();
        return;
      }

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(result['message']?.toString() ?? '加载失败，请下拉重试'),
        ),
      );
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'TempFriendsScreen._loadTempFriends');
      if (!mounted) return;
      setState(() => _isLoading = false);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('加载失败，请稍后重试')),
      );
    }
  }

  /// 计算距过期时间的剩余时长，格式化为"N小时N分钟"
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

  /// 根据剩余时间返回对应颜色：< 1h 警告色，已过期错误色
  Color _getExpiryColor(String expiresAt) {
    try {
      final expiryTime = DateTime.parse(expiresAt);
      final now = DateTime.now();
      final diff = expiryTime.difference(now);

      if (diff.isNegative) {
        return AppTheme.errorColor;
      } else if (diff.inHours < 1) {
        return AppTheme.warningColor;
      } else {
        return AppTheme.successColor;
      }
    } catch (e) {
      return AppTheme.textTertiary;
    }
  }

  /// 确认后删除临时好友关系
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
            style: TextButton.styleFrom(foregroundColor: AppTheme.errorColor),
            child: const Text('删除'),
          ),
        ],
      ),
    );

    if (confirm == true && mounted) {
      try {
        final result =
            await context.read<FriendProvider>().deleteTempFriend(tempFriendId);

        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(result['success'] ? '删除成功' : result['message']),
              backgroundColor: result['success']
                  ? AppTheme.successColor
                  : AppTheme.errorColor,
            ),
          );
        }
      } catch (error, stackTrace) {
        _reportUiError(
          error,
          stackTrace,
          'TempFriendsScreen._deleteTempFriend',
        );
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
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final tempFriends = context.watch<FriendProvider>().tempFriends;
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
                    '• 持续互动会影响后续关系判定\n\n'
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
            color: isDark ? AppTheme.primaryLightColor : Colors.blue[900],
            backgroundColor: isDark ? AppTheme.nightSurface : Colors.white,
            child: ListView.builder(
              padding: EdgeInsets.only(
                top: MediaQuery.of(context).padding.top + kToolbarHeight + 16,
                bottom: 20,
                left: 16,
                right: 16,
              ),
              itemCount: _isLoading
                  ? 2
                  : (tempFriends.isEmpty ? 2 : tempFriends.length + 1),
              itemBuilder: (context, index) {
                // 第一项：提示卡片
                if (index == 0) {
                  return Column(
                    children: [
                      Card(
                        color: isDark
                            ? const Color(0xFF1B2838).withValues(alpha: 0.95)
                            : Colors.white.withValues(alpha: 0.95),
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
                                  color:
                                      AppTheme.skyBlue.withValues(alpha: 0.1),
                                  borderRadius: BorderRadius.circular(12),
                                ),
                                child: const Icon(
                                  Icons.access_time,
                                  color: AppTheme.skyBlue,
                                  size: 28,
                                ),
                              ),
                              const SizedBox(width: 16),
                              Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Text(
                                      '临时好友关系',
                                      style: TextStyle(
                                        fontSize: 16,
                                        fontWeight: FontWeight.bold,
                                        color: isDark
                                            ? AppTheme.darkTextPrimary
                                            : AppTheme.darkBlue,
                                      ),
                                    ),
                                    const SizedBox(height: 4),
                                    Text(
                                      '通过互动自动建立，24小时有效期',
                                      style: TextStyle(
                                        fontSize: 13,
                                        color: isDark
                                            ? AppTheme.darkTextSecondary
                                            : AppTheme.textTertiary,
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
                if (tempFriends.isEmpty) {
                  return Center(
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
                          '还没有临时好友',
                          style: TextStyle(
                            fontSize: 16,
                            color: Colors.white.withValues(alpha: 0.9),
                          ),
                        ),
                        const SizedBox(height: 8),
                        Text(
                          '在心湖中互动即可建立临时好友关系',
                          style: TextStyle(
                            fontSize: 14,
                            color: Colors.white.withValues(alpha: 0.7),
                          ),
                        ),
                      ],
                    ),
                  );
                }

                // 好友列表项
                final friendIndex = index - 1;
                final tempFriend = tempFriends[friendIndex];
                final status = tempFriend['status'] ?? 'active';
                final expiresAt = tempFriend['expires_at'] ?? '';
                final isExpired = status == 'expired';

                return _buildFriendCard(
                  friendIndex,
                  tempFriend,
                  isExpired,
                  expiresAt,
                  isDark,
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  /// 构建单个好友卡片，带交错淡入动画和过期状态标记
  Widget _buildFriendCard(
    int index,
    dynamic tempFriend,
    bool isExpired,
    String expiresAt,
    bool isDark,
  ) {
    return AnimatedBuilder(
      animation: _listAnimController,
      builder: (context, child) {
        final delay = index * 0.1;
        final start = delay.clamp(0.0, 0.7);
        final anim = Tween<double>(begin: 0.0, end: 1.0).animate(
          CurvedAnimation(
            parent: _listAnimController,
            curve: Interval(start, (start + 0.3).clamp(0.0, 1.0),
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
        margin: const EdgeInsets.only(bottom: 12),
        color: isDark
            ? const Color(0xFF1B2838).withValues(alpha: 0.95)
            : Colors.white.withValues(alpha: 0.95),
        elevation: 2,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: BorderSide(
            color: isExpired
                ? AppTheme.textTertiary.withValues(alpha: 0.3)
                : AppTheme.skyBlue.withValues(alpha: 0.3),
            width: 1,
          ),
        ),
        child: ListTile(
          contentPadding: const EdgeInsets.all(12),
          leading: Stack(
            children: [
              CircleAvatar(
                radius: 28,
                backgroundColor: isExpired
                    ? AppTheme.textTertiary.withValues(alpha: 0.2)
                    : AppTheme.skyBlue.withValues(alpha: 0.2),
                child: Text(
                  (tempFriend['friend_nickname']?.isNotEmpty == true)
                      ? tempFriend['friend_nickname'].substring(0, 1)
                      : '?',
                  style: TextStyle(
                      color:
                          isExpired ? AppTheme.textTertiary : AppTheme.skyBlue,
                      fontWeight: FontWeight.bold,
                      fontSize: 20),
                ),
              ),
              if (!isExpired)
                Positioned(
                  right: 0,
                  bottom: 0,
                  child: Container(
                    padding: const EdgeInsets.all(4),
                    decoration: BoxDecoration(
                        color: _getExpiryColor(expiresAt),
                        shape: BoxShape.circle,
                        border: Border.all(color: Colors.white, width: 2)),
                    child: const Icon(Icons.access_time,
                        color: Colors.white, size: 12),
                  ),
                ),
            ],
          ),
          title: Row(
            children: [
              Expanded(
                  child: Text(tempFriend['friend_nickname'] ?? '未知',
                      style: TextStyle(
                          fontWeight: FontWeight.bold,
                          color: isExpired
                              ? AppTheme.textTertiary
                              : (isDark
                                  ? AppTheme.darkTextPrimary
                                  : AppTheme.darkBlue)))),
            ],
          ),
          subtitle: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const SizedBox(height: 4),
              Text('来源: ${_getSourceText(tempFriend['source'])}',
                  style: TextStyle(
                      fontSize: 12,
                      color:
                          isDark ? const Color(0xFF9AA0A6) : Colors.grey[600])),
              const SizedBox(height: 4),
              Row(
                children: [
                  Icon(Icons.timer_outlined,
                      size: 14, color: _getExpiryColor(expiresAt)),
                  const SizedBox(width: 4),
                  Text(
                      isExpired ? '已过期' : '剩余: ${_getTimeRemaining(expiresAt)}',
                      style: TextStyle(
                          fontSize: 12,
                          color: _getExpiryColor(expiresAt),
                          fontWeight: FontWeight.w500)),
                ],
              ),
            ],
          ),
          trailing: !isExpired
              ? PopupMenuButton(
                  icon: const Icon(Icons.more_vert),
                  itemBuilder: (context) => [
                    const PopupMenuItem(
                        value: 'chat',
                        child: Row(children: [
                          Icon(Icons.chat, size: 20),
                          SizedBox(width: 8),
                          Text('私聊')
                        ])),
                    const PopupMenuItem(
                        value: 'delete',
                        child: Row(children: [
                          Icon(Icons.delete,
                              color: AppTheme.errorColor, size: 20),
                          SizedBox(width: 8),
                          Text('删除',
                              style: TextStyle(color: AppTheme.errorColor))
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
                                    friendName: tempFriend['friend_nickname'] ??
                                        '未知')));
                        break;
                      case 'delete':
                        _deleteTempFriend(tempFriend['temp_friend_id'],
                            tempFriend['friend_nickname'] ?? '未知');
                        break;
                    }
                  },
                )
              : null,
          onTap: !isExpired
              ? () => Navigator.push(
                  context,
                  MaterialPageRoute(
                      builder: (context) => FriendChatScreen(
                          friendId: tempFriend['friend_id'],
                          friendName: tempFriend['friend_nickname'] ?? '未知')))
              : null,
        ),
      ),
    );
  }

  /// 将互动来源标识转为中文显示文本
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
