// @file notification_screen.dart
// @brief 通知列表界面
// Created by 林子怡

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../data/datasources/api_client.dart';
import '../../data/datasources/stone_service.dart';
import '../../domain/entities/stone.dart';
import '../providers/notification_provider.dart';
import '../../utils/app_theme.dart';
import 'stone_detail_screen.dart';
import 'friend_chat_screen.dart';
import 'friends_screen.dart';

class NotificationScreen extends StatefulWidget {
  const NotificationScreen({super.key});

  @override
  State<NotificationScreen> createState() => _NotificationScreenState();
}

class _NotificationScreenState extends State<NotificationScreen> {
  final List<Map<String, dynamic>> _notifications = [];
  final ApiClient _apiClient = ApiClient();
  bool _isLoading = false;
  int _unreadCount = 0;

  @override
  void initState() {
    super.initState();
    _loadNotifications();
  }

  Future<void> _loadNotifications() async {
    setState(() => _isLoading = true);

    try {
      final response = await _apiClient.get(
        '/notifications',
        queryParameters: {'page': 1, 'page_size': 100},
      );

      if (response.statusCode == 200 &&
          response.data['code'] == 0 &&
          mounted) {
        final data = response.data['data'];
        final items =
            data['notifications'] as List? ?? data['items'] as List? ?? [];
        final serverUnreadCount = data['unread_count'] as int? ?? 0;

        setState(() {
          _notifications.clear();
          _notifications.addAll(items.whereType<Map<String, dynamic>>());
          _unreadCount = serverUnreadCount;
          _isLoading = false;
        });

        // 同步更新Provider中的计数
        if (mounted) {
          context
              .read<NotificationProvider>()
              .setUnreadCount(serverUnreadCount);
        }
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载通知失败，请下拉重试')),
        );
      }
    }
  }

  Future<void> _markAsRead(String notificationId, int index) async {
    try {
      final response =
          await _apiClient.post('/notifications/$notificationId/read');

      if (response.statusCode == 200 && mounted) {
        setState(() {
          _notifications[index]['is_read'] = true;
          _unreadCount =
              response.data['data']?['unread_count'] ?? (_unreadCount - 1);
          if (_unreadCount < 0) _unreadCount = 0;
        });

        // 同步更新Provider
        context.read<NotificationProvider>().setUnreadCount(_unreadCount);
      }
    } catch (e) {
      debugPrint('Error marking notification as read: $e');
    }
  }

  Future<void> _markAllAsRead() async {
    try {
      final response = await _apiClient.post('/notifications/read-all');

      if (response.statusCode == 200 && mounted) {
        setState(() {
          for (var notif in _notifications) {
            notif['is_read'] = true;
          }
          _unreadCount = 0;
        });

        // 同步更新Provider
        context.read<NotificationProvider>().setUnreadCount(0);

        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('所有通知已标记为已读')),
        );
      }
    } catch (e) {
      debugPrint('Error marking all notifications as read: $e');
    }
  }

  Future<void> _navigateToContent(Map<String, dynamic> notification) async {
    final type = notification['type'] ?? '';
    final targetId = notification['target_id'] ?? notification['related_id'];
    final targetName = notification['target_name'] ?? notification['sender_name'] ?? '用户';

    switch (type) {
      case 'ripple':
      case 'boat':
        if (targetId != null) {
          final result = await StoneService().getStoneDetail(targetId);
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
    return Scaffold(
      appBar: AppBar(
        title: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('通知'),
            if (_unreadCount > 0) ...[
              const SizedBox(width: 8),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                decoration: BoxDecoration(
                  color: AppTheme.errorColor,
                  borderRadius: BorderRadius.circular(10),
                ),
                child: Text(
                  _unreadCount.toString(),
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
          if (_unreadCount > 0)
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
        child: _isLoading
            ? Center(child: Column(mainAxisSize: MainAxisSize.min, children: [const CircularProgressIndicator(), const SizedBox(height: 16), Text('正在收集湖面的回响...', style: TextStyle(color: Colors.grey[600]))]))
            : _notifications.isEmpty
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
                    itemCount: _notifications.length,
                    itemBuilder: (context, index) {
                      final notification = _notifications[index];
                      final isRead = notification['is_read'] == true;

                      return Card(
                        margin: const EdgeInsets.only(bottom: 12),
                        color:
                            isRead ? null : AppTheme.skyBlue.withValues(alpha: 0.05),
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
                              fontWeight:
                                  isRead ? FontWeight.normal : FontWeight.bold,
                            ),
                          ),
                          subtitle: Text(
                            notification['created_at']?.toString() ?? '',
                            style: TextStyle(
                              fontSize: 12,
                              color: Colors.grey[600],
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
                          onTap: () {
                            if (!isRead) {
                              final notifId = notification['notification_id'];
                              if (notifId != null) {
                                _markAsRead(notifId, index);
                              }
                            }
                            _navigateToContent(notification);
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
