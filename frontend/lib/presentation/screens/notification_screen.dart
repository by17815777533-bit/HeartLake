// 通知列表界面
//
// 展示系统通知和互动消息，支持分页加载和已读标记。

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../data/datasources/stone_service.dart';
import '../../di/service_locator.dart';
import '../../domain/entities/stone.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../providers/notification_provider.dart';
import 'stone_detail_screen.dart';
import 'friend_chat_screen.dart';
import 'friends_screen.dart';

/// 通知列表页面
///
/// 展示用户收到的所有通知（涟漪、纸船、好友请求、系统消息等），
/// 支持按类型分组、标记已读、点击跳转到对应详情页。
class NotificationScreen extends StatefulWidget {
  const NotificationScreen({super.key});

  @override
  State<NotificationScreen> createState() => _NotificationScreenState();
}

class _NotificationScreenState extends State<NotificationScreen> {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      context.read<NotificationProvider>().loadNotifications(refresh: true);
    });
  }

  /// 从 Provider 拉取通知列表
  Future<void> _loadNotifications() async {
    await context.read<NotificationProvider>().loadNotifications(refresh: true);
  }

  /// 将指定通知标记为已读，并更新本地未读计数
  Future<void> _markAsRead(String notificationId) async {
    await context.read<NotificationProvider>().markAsRead(notificationId);
  }

  /// 一键将所有通知标记为已读
  Future<void> _markAllAsRead() async {
    await context.read<NotificationProvider>().markAllAsRead();
  }

  /// 根据通知类型跳转到对应的详情页面（石头详情、好友列表、聊天等）
  Future<void> _navigateToContent(Map<String, dynamic> notification) async {
    final type = notification['type'] ?? '';
    final targetId = notification['target_id'] ?? notification['related_id'];
    final targetName =
        notification['target_name'] ?? notification['sender_name'] ?? '用户';

    switch (type) {
      case 'ripple':
      case 'boat':
        if (targetId != null) {
          final result = await sl<StoneService>().getStoneDetail(targetId);
          if (result['success'] == true && result['stone'] != null && mounted) {
            final stone = Stone.fromJson(result['stone']);
            Navigator.push(
              context,
              MaterialPageRoute(
                builder: (context) => StoneDetailScreen(stone: stone),
              ),
            );
          }
        }
        break;
      case 'friend_request':
      case 'friend_accepted':
      case 'connection':
        Navigator.push(
          context,
          MaterialPageRoute(builder: (context) => const FriendsScreen()),
        );
        break;
      case 'message':
        if (targetId != null) {
          Navigator.push(
            context,
            MaterialPageRoute(
              builder: (context) => FriendChatScreen(
                friendId: targetId,
                friendName: targetName,
              ),
            ),
          );
        }
        break;
    }
  }

  /// 通知类型 -> emoji 图标映射
  String _getNotificationIcon(String type) {
    switch (type) {
      case 'ripple':
        return '💧';
      case 'boat':
        return '⛵';
      case 'connection':
        return '🤝';
      case 'friend_request':
        return '👋';
      case 'friend_accepted':
        return '🎉';
      case 'message':
        return '💬';
      default:
        return '🔔';
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final notificationState = context.watch<NotificationProvider>();
    final notifications = notificationState.notifications;
    final unreadCount = notificationState.unreadCount;
    final isLoading = notificationState.isLoadingNotifications;

    return Scaffold(
      appBar: AppBar(
        title: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('通知'),
            if (unreadCount > 0) ...[
              const SizedBox(width: 8),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                decoration: BoxDecoration(
                  color: AppTheme.errorColor,
                  borderRadius: BorderRadius.circular(10),
                ),
                child: Text(
                  unreadCount.toString(),
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 12,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
            ],
          ],
        ),
        centerTitle: true,
        actions: [
          if (unreadCount > 0)
            TextButton(
              onPressed: _markAllAsRead,
              child: const Text('全部已读', style: TextStyle(color: Colors.white)),
            ),
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loadNotifications,
          ),
        ],
      ),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [AppTheme.skyBlue.withValues(alpha: 0.08), Colors.white],
          ),
        ),
        child: RefreshIndicator(
          onRefresh: _loadNotifications,
          child: isLoading
              ? Center(
                  child: Column(mainAxisSize: MainAxisSize.min, children: [
                  const CircularProgressIndicator(),
                  const SizedBox(height: 16),
                  Text('正在收集湖面的回响...',
                      style: TextStyle(color: Colors.grey[600]))
                ]))
              : notifications.isEmpty
                  ? ListView(
                      children: [
                        SizedBox(
                          height: MediaQuery.of(context).size.height * 0.7,
                          child: Center(
                            child: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Icon(
                                  Icons.notifications_none,
                                  size: 80,
                                  color: Colors.grey[300],
                                ),
                                const SizedBox(height: 16),
                                Text(
                                  '暂无通知，心湖很安静',
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
                      itemCount: notifications.length,
                      itemBuilder: (context, index) {
                        final notification = notifications[index];
                        final isRead = notification['is_read'] == true;

                        return Card(
                          margin: const EdgeInsets.only(bottom: 12),
                          color: isRead
                              ? null
                              : AppTheme.skyBlue.withValues(alpha: 0.05),
                          child: ListTile(
                            leading: Container(
                              width: 40,
                              height: 40,
                              decoration: BoxDecoration(
                                color: isRead
                                    ? Colors.grey[200]
                                    : AppTheme.skyBlue.withValues(alpha: 0.2),
                                borderRadius: BorderRadius.circular(20),
                              ),
                              child: Center(
                                child: Text(
                                  _getNotificationIcon(
                                      notification['type'] ?? ''),
                                  style: const TextStyle(fontSize: 20),
                                ),
                              ),
                            ),
                            title: Text(
                              notification['content'] ?? '',
                              style: TextStyle(
                                fontWeight: isRead
                                    ? FontWeight.normal
                                    : FontWeight.bold,
                              ),
                            ),
                            subtitle: Text(
                              notification['created_at']?.toString() ?? '',
                              style: TextStyle(
                                fontSize: 12,
                                color: isDark
                                    ? AppTheme.darkTextSecondary
                                    : Colors.grey[600],
                              ),
                            ),
                            trailing: isRead
                                ? null
                                : Container(
                                    width: 8,
                                    height: 8,
                                    decoration: BoxDecoration(
                                      color: AppTheme.skyBlue,
                                      borderRadius: BorderRadius.circular(4),
                                    ),
                                  ),
                            onTap: () async {
                              if (!isRead) {
                                final notifId =
                                    extractNotificationEntityId(notification);
                                if (notifId != null) {
                                  await _markAsRead(notifId);
                                }
                              }
                              if (mounted) {
                                await _navigateToContent(notification);
                              }
                            },
                          ),
                        );
                      },
                    ),
        ),
      ),
    );
  }
}
