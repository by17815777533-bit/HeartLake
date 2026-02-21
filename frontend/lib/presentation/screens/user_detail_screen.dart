// @file user_detail_screen.dart
// @brief 用户详情界面
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../data/datasources/api_client.dart';
import '../../utils/app_theme.dart';
import 'friend_chat_screen.dart';

class UserDetailScreen extends StatefulWidget {
  final String userId;
  final String? nickname;

  const UserDetailScreen({super.key, required this.userId, this.nickname});

  @override
  State<UserDetailScreen> createState() => _UserDetailScreenState();
}

class _UserDetailScreenState extends State<UserDetailScreen> {
  final ApiClient _apiClient = ApiClient();
  Map<String, dynamic>? _user;
  bool _isLoading = true;
  bool _isFriend = false;

  @override
  void initState() {
    super.initState();
    _loadUserProfile();
  }

  Future<void> _loadUserProfile() async {
    try {
      final response = await _apiClient.get('/users/${widget.userId}/profile');
      if (response.statusCode == 200 && response.data['code'] == 0 && mounted) {
        setState(() {
          _user = response.data['data'];
          _isFriend = _user?['is_friend'] == true;
          _isLoading = false;
        });
      }
    } catch (e) {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.nickname ?? '用户详情'),
        centerTitle: true,
      ),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [AppTheme.skyBlue.withValues(alpha: 0.1), Colors.white],
          ),
        ),
        child: _isLoading
            ? const Center(child: CircularProgressIndicator())
            : _user == null
                ? const Center(child: Text('用户不存在'))
                : SingleChildScrollView(
                  padding: const EdgeInsets.all(20),
                  child: Column(
                    children: [
                      CircleAvatar(
                        radius: 50,
                        backgroundColor: AppTheme.skyBlue.withValues(alpha: 0.2),
                        backgroundImage: (_user?['avatar_url'] != null && _user!['avatar_url'].toString().isNotEmpty)
                            ? NetworkImage(_user!['avatar_url'])
                            : null,
                        child: _user?['avatar_url'] == null
                            ? Text(
                                (_user?['nickname']?.isNotEmpty == true) ? _user!['nickname'].substring(0, 1) : '?',
                                style: const TextStyle(fontSize: 36, color: AppTheme.skyBlue),
                              )
                            : null,
                      ),
                      const SizedBox(height: 16),
                      Text(
                        _user?['nickname'] ?? '匿名用户',
                        style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                      ),
                      if (_user?['bio'] != null) ...[
                        const SizedBox(height: 8),
                        Text(_user!['bio'], style: TextStyle(color: Colors.grey[600])),
                      ],
                      const SizedBox(height: 24),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          _buildStat('石头', _user?['stone_count'] ?? 0),
                          const SizedBox(width: 32),
                          _buildStat('涟漪', _user?['ripple_count'] ?? 0),
                        ],
                      ),
                      const SizedBox(height: 32),
                      if (_isFriend)
                        ElevatedButton.icon(
                          onPressed: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (context) => FriendChatScreen(
                                friendId: widget.userId,
                                friendName: _user?['nickname'] ?? '好友',
                              ),
                            ),
                          ),
                          icon: const Icon(Icons.chat),
                          label: const Text('发消息'),
                          style: ElevatedButton.styleFrom(
                            minimumSize: const Size(200, 48),
                          ),
                        )
                    ],
                  ),
                ),
      ),
    );
  }

  Widget _buildStat(String label, int count) {
    return Column(
      children: [
        Text('$count', style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
        Text(label, style: TextStyle(color: Colors.grey[600])),
      ],
    );
  }
}
