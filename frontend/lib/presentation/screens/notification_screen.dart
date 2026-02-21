// @file notification_screen.dart
// @brief 通知列表界面 - 光遇风格
// Created by 林子怡

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../data/datasources/api_client.dart';
import '../../data/datasources/stone_service.dart';
import '../../domain/entities/stone.dart';
import '../providers/notification_provider.dart';
import '../../utils/app_theme.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../../utils/animation_utils.dart';
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
          SnackBar(
            content: const Text('加载通知失败，请下拉重试'),
            backgroundColor: AppTheme.nightSurface,
          ),
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
          _unreadCount = (_unreadCount - 1).clamp(0, _notifications.length);
        });
        context.read<NotificationProvider>().setUnreadCount(_unreadCount);
      }
    } catch (e) {
      // 静默失败
    }
  }

  Future<void> _markAllAsRead() async {
    try {
      final response = await _apiClient.post('/notifications/read-all');
      if (response.statusCode == 200 && mounted) {
        setState(() {
          for (var n in _notifications) {
            n['is_read'] = true;
          }
          _unreadCount = 0;
        });
        context.read<NotificationProvider>().setUnreadCount(0);
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Text('操作失败，请重试'),
            backgroundColor: AppTheme.nightSurface,
          ),
        );
      }
    }
  }

  void _navigateToContent(Map<String, dynamic> notification) {
    final type = notification['type'] ?? '';
    final stoneId = notification['stone_id'];
    final senderId = notification['sender_id'];
    final senderNickname = notification['sender_nickname'] ?? '用户';
    if (type == 'friend_request' || type == 'friend_accepted') {
      Navigator.push(
        context,
        SkyPageRoute(page: const FriendsScreen()),
      );
    } else if (type == 'friend_message' && senderId != null) {
      Navigator.push(
        context,
        SkyPageRoute(
            page: FriendChatScreen(
          friendId: senderId,
          friendName: senderNickname,
        )),
      );
    } else if (stoneId != null) {
      _navigateToStone(stoneId);
    }
  }

  Future<void> _navigateToStone(String stoneId) async {
    try {
      final stoneService = StoneService();
      final stoneData = await stoneService.getStoneDetail(stoneId);
      if (mounted) {
        final stone = Stone.fromJson(stoneData);
        Navigator.push(
          context,
          SkyPageRoute(page: StoneDetailScreen(stone: stone)),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Text('无法打开该内容'),
            backgroundColor: AppTheme.nightSurface,
          ),
        );
      }
    }
  }

  IconData _getNotificationIcon(String type) {
    switch (type) {
      case 'like':
        return Icons.favorite;
      case 'comment':
        return Icons.chat_bubble;
      case 'boat_reply':
        return Icons.sailing;
      case 'friend_request':
        return Icons.person_add;
      case 'friend_accepted':
        return Icons.people;
      case 'friend_message':
        return Icons.message;
      case 'system':
        return Icons.notifications;
      default:
        return Icons.notifications_none;
    }
  }

  Color _getNotificationIconColor(String type) {
    switch (type) {
      case 'like':
        return AppTheme.warmPink;
      case 'comment':
        return AppTheme.spiritBlue;
      case 'boat_reply':
        return AppTheme.candleGlow;
      case 'friend_request':
      case 'friend_accepted':
        return AppTheme.candleGlow;
      case 'friend_message':
        return AppTheme.warmOrange;
      default:
        return AppTheme.darkTextSecondary;
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: AppTheme.darkTextPrimary,
        title: Text(
          '通知',
          style: TextStyle(
            color: AppTheme.candleGlow,
            fontWeight: FontWeight.bold,
            fontSize: 20,
            shadows: [
              Shadow(
                color: AppTheme.candleGlow.withValues(alpha: 0.5),
                blurRadius: 8,
              ),
            ],
          ),
        ),
        actions: [
          if (_unreadCount > 0)
            TextButton(
              onPressed: _markAllAsRead,
              child: Text(
                '全部已读',
                style: TextStyle(color: AppTheme.candleGlow, fontSize: 14),
              ),
            ),
        ],
      ),
      body: SafeArea(
        child: RefreshIndicator(
          onRefresh: _loadNotifications,
          color: AppTheme.candleGlow,
          backgroundColor: AppTheme.nightSurface,
          child: _isLoading
              ? Center(
                  child: CircularProgressIndicator(
                    color: AppTheme.candleGlow,
                  ),
                )
              : _notifications.isEmpty
                  ? _buildEmptyState(context)
                  : _buildNotificationList(),
        ),
      ),
    );
  }

  Widget _buildEmptyState(BuildContext context) {
    return ListView(
      children: [
        SizedBox(
          height: MediaQuery.of(context).size.height * 0.6,
          child: Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(
                  Icons.notifications_off_outlined,
                  size: 64,
                  color: AppTheme.darkTextSecondary,
                ),
                const SizedBox(height: 16),
                Text(
                  '暂无通知',
                  style: TextStyle(
                    color: AppTheme.darkTextPrimary,
                    fontSize: 16,
                  ),
                ),
                const SizedBox(height: 8),
                Text(
                  '新的互动消息会出现在这里',
                  style: TextStyle(
                    color: AppTheme.darkTextSecondary,
                    fontSize: 13,
                  ),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildNotificationList() {
    return ListView.builder(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      itemCount: _notifications.length,
      itemBuilder: (context, index) {
        final notification = _notifications[index];
        final isRead = notification['is_read'] == true;
        final type = notification['type'] ?? '';

        return Padding(
          padding: const EdgeInsets.only(bottom: 10),
          child: SkyGlassCard(
            borderRadius: 16,
            enableGlow: !isRead,
            glowColor: _getNotificationIconColor(type),
            padding: EdgeInsets.zero,
            onTap: () {
              if (!isRead) {
                final notifId = notification['notification_id'];
                if (notifId != null) {
                  _markAsRead(notifId, index);
                }
              }
              _navigateToContent(notification);
            },
            child: ListTile(
              contentPadding:
                  const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
              leading: Container(
                width: 40,
                height: 40,
                decoration: BoxDecoration(
                  color: _getNotificationIconColor(type)
                      .withValues(alpha: 0.2),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(
                    color: _getNotificationIconColor(type)
                        .withValues(alpha: 0.3),
                    width: 1,
                  ),
                ),
                child: Icon(
                  _getNotificationIcon(type),
                  color: _getNotificationIconColor(type),
                  size: 20,
                ),
              ),
              title: Text(
                notification['content'] ?? '新通知',
                style: TextStyle(
                  color: AppTheme.darkTextPrimary,
                  fontSize: 14,
                  fontWeight:
                      isRead ? FontWeight.normal : FontWeight.bold,
                ),
              ),
              subtitle: Text(
                notification['created_at']?.toString() ?? '',
                style: TextStyle(
                  fontSize: 12,
                  color: AppTheme.darkTextSecondary,
                ),
              ),
              trailing: isRead
                  ? null
                  : Container(
                      width: 8,
                      height: 8,
                      decoration: BoxDecoration(
                        color: AppTheme.candleGlow,
                        borderRadius: BorderRadius.circular(4),
                        boxShadow: [
                          BoxShadow(
                            color: AppTheme.candleGlow.withValues(alpha: 0.6),
                            blurRadius: 6,
                            spreadRadius: 1,
                          ),
                        ],
                      ),
                    ),
            ),
          ),
        );
      },
    );
  }
}