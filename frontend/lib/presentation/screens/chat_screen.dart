// @file chat_screen.dart
// @brief 通用聊天界面 - 光遇风格
import 'package:flutter/material.dart';
import '../../data/datasources/friend_service.dart';
import '../../utils/app_theme.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';

class ChatScreen extends StatefulWidget {
  final String recipientId;
  final String? recipientName;

  const ChatScreen({super.key, required this.recipientId, this.recipientName});

  @override
  State<ChatScreen> createState() => _ChatScreenState();
}

class _ChatScreenState extends State<ChatScreen> {
  final FriendService _friendService = FriendService();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  List<dynamic> _messages = [];
  bool _isLoading = true;
  bool _isSending = false;

  @override
  void initState() {
    super.initState();
    _loadMessages();
  }

  @override
  void dispose() {
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  Future<void> _loadMessages() async {
    try {
      final result = await _friendService.getMessages(widget.recipientId);
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
            content: Text('加载消息失败，请下拉重试', style: TextStyle(color: AppTheme.textPrimary)),
            backgroundColor: AppTheme.lightStone,
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
      final result = await _friendService.sendMessage(widget.recipientId, content);

      if (mounted) {
        setState(() => _isSending = false);
        if (result['success'] == true) {
          _loadMessages();
        } else {
          _controller.text = content;
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(result['message'] ?? '发送失败', style: const TextStyle(color: AppTheme.darkTextPrimary)),
              backgroundColor: AppTheme.lightStone,
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
            content: Text('网络异常，请稍后再试', style: TextStyle(color: AppTheme.textPrimary)),
            backgroundColor: AppTheme.lightStone,
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
          widget.recipientName ?? '聊天',
          style: TextStyle(
            color: AppTheme.primaryLightColor,
            fontWeight: FontWeight.bold,
            shadows: [
              Shadow(color: AppTheme.primaryLightColor.withValues(alpha: 0.6), blurRadius: 12),
            ],
          ),
        ),
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: AppTheme.textPrimary,
        iconTheme: const IconThemeData(color: AppTheme.darkTextPrimary),
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
                    colors: [AppTheme.primaryLightColor, AppTheme.primaryColor],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                  borderRadius: BorderRadius.circular(16),
                ),
                child: Text(
                  message['content'] ?? '',
                  style: const TextStyle(color: AppTheme.nightDeep, fontSize: 15),
                ),
              )
            : SkyGlassCard(
                borderRadius: 16,
                enableGlow: false,
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                child: Text(
                  message['content'] ?? '',
                  style: const TextStyle(color: AppTheme.darkTextPrimary, fontSize: 15),
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
              style: const TextStyle(color: AppTheme.darkTextPrimary),
              decoration: InputDecoration(
                hintText: '说点什么...',
                hintStyle: const TextStyle(color: AppTheme.darkTextSecondary),
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
                ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: AppTheme.candleGlow))
                : const Icon(Icons.send, color: AppTheme.candleGlow),
            onPressed: _isSending ? null : _sendMessage,
          ),
        ],
      ),
    );
  }
}
