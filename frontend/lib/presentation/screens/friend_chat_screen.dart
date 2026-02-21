// @file friend_chat_screen.dart
// @brief 好友聊天界面 - 光遇风格
import 'dart:async';
import 'package:flutter/material.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../utils/app_theme.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';

class FriendChatScreen extends StatefulWidget {
  final String friendId;
  final String? friendName;

  const FriendChatScreen({super.key, required this.friendId, this.friendName});

  @override
  State<FriendChatScreen> createState() => _FriendChatScreenState();
}

class _FriendChatScreenState extends State<FriendChatScreen> {
  final FriendService _friendService = FriendService();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  List<dynamic> _messages = [];
  bool _isLoading = true;
  bool _isSending = false;
  late final Function(Map<String, dynamic>) _messageListener;
  Timer? _pollTimer;

  @override
  void initState() {
    super.initState();
    _loadMessages();

    // WebSocket 监听新消息
    _messageListener = (data) {
      if (mounted && data['friend_id'] == widget.friendId) {
        _loadMessages();
      }
    };
    WebSocketManager().on('friend_message', _messageListener);

    // 定时轮询兜底
    _pollTimer = Timer.periodic(const Duration(seconds: 15), (_) {
      if (mounted) _loadMessages();
    });
  }

  @override
  void dispose() {
    WebSocketManager().off('friend_message', _messageListener);
    _pollTimer?.cancel();
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  Future<void> _loadMessages() async {
    try {
      final result = await _friendService.getMessages(widget.friendId);
      if (mounted) {
        setState(() {
          _messages = result['messages'] ?? [];
          _isLoading = false;
        });
        _scrollToBottom();
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('加载消息失败，请下拉重试'),
            backgroundColor: AppTheme.warmOrange,
          ),
        );
      }
    }
  }

  void _scrollToBottom() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scrollController.hasClients) {
        _scrollController.animateTo(
          _scrollController.position.maxScrollExtent,
          duration: const Duration(milliseconds: 300),
          curve: Curves.easeOut,
        );
      }
    });
  }

  Future<void> _sendMessage() async {
    final content = _controller.text.trim();
    if (content.isEmpty || _isSending) return;

    setState(() => _isSending = true);
    _controller.clear();
    FocusScope.of(context).unfocus();

    try {
      final result = await _friendService.sendMessage(widget.friendId, content);

      if (mounted) {
        setState(() => _isSending = false);
        if (result['success'] == true) {
          // 本地追加消息，避免全量重载
          setState(() {
            _messages.add({
              'content': content,
              'is_mine': true,
              'created_at': DateTime.now().toIso8601String(),
            });
          });
          _scrollToBottom();
        } else {
          _controller.text = content;
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(result['message'] ?? '发送失败'),
              backgroundColor: AppTheme.warmOrange,
            ),
          );
        }
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isSending = false);
        _controller.text = content;
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('网络异常，请稍后再试'),
            backgroundColor: AppTheme.warmOrange,
          ),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      appBar: AppBar(
        title: Text(
          widget.friendName ?? '聊天',
          style: TextStyle(
            color: AppTheme.candleGlow,
            fontWeight: FontWeight.bold,
            shadows: [
              Shadow(color: AppTheme.candleGlow.withValues(alpha: 0.6), blurRadius: 12),
            ],
          ),
        ),
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: AppTheme.darkTextPrimary,
      ),
      body: SafeArea(
        child: Column(
          children: [
            Expanded(
              child: _isLoading
                  ? const Center(child: CircularProgressIndicator(color: AppTheme.candleGlow))
                  : _messages.isEmpty
                      ? Center(
                          child: Text(
                            '还没有消息，说点什么吧~',
                            style: const TextStyle(color: AppTheme.darkTextSecondary),
                          ),
                        )
                      : ListView.builder(
                          controller: _scrollController,
                          padding: const EdgeInsets.all(16),
                          itemCount: _messages.length,
                          itemBuilder: (context, index) => _buildMessageBubble(_messages[index]),
                        ),
            ),
            _buildInputBar(),
          ],
        ),
      ),
    );
  }

  Widget _buildMessageBubble(dynamic message) {
    final isMe = message['is_mine'] == true;
    return Align(
      alignment: isMe ? Alignment.centerRight : Alignment.centerLeft,
      child: Container(
        margin: const EdgeInsets.symmetric(vertical: 4),
        constraints: BoxConstraints(maxWidth: MediaQuery.of(context).size.width * 0.7),
        child: isMe
            ? Container(
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                decoration: BoxDecoration(
                  gradient: const LinearGradient(
                    colors: [AppTheme.warmOrange, AppTheme.peachPink],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                  borderRadius: BorderRadius.circular(16),
                ),
                child: Text(
                  message['content'] ?? '',
                  style: const TextStyle(color: Colors.white, fontSize: 15),
                ),
              )
            : SkyGlassCard(
                borderRadius: 16,
                enableGlow: false,
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                child: Text(
                  message['content'] ?? '',
                  style: TextStyle(color: Colors.white.withValues(alpha: 0.9), fontSize: 15),
                ),
              ),
      ),
    );
  }

  Widget _buildInputBar() {
    return SkyGlassCard(
      borderRadius: 0,
      enableGlow: false,
      padding: EdgeInsets.only(
        left: 16, right: 8, top: 8,
        bottom: MediaQuery.of(context).padding.bottom + 8,
      ),
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: _controller,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                hintText: '说点什么...',
                hintStyle: TextStyle(color: Colors.white.withValues(alpha: 0.6)),
                border: InputBorder.none,
                contentPadding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
              ),
              maxLines: null,
              textInputAction: TextInputAction.send,
              onSubmitted: (_) => _sendMessage(),
            ),
          ),
          IconButton(
            icon: _isSending
                ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: AppTheme.warmOrange))
                : const Icon(Icons.send, color: AppTheme.warmOrange),
            onPressed: _isSending ? null : _sendMessage,
          ),
        ],
      ),
    );
  }
}
