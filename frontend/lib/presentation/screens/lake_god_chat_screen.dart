// @file lake_god_chat_screen.dart
// @brief 湖神聊天界面 - 光遇风格温馨治愈AI咨询
import 'package:flutter/material.dart';
import '../../data/datasources/lake_god_service.dart';
import '../../utils/app_theme.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';

class LakeGodChatScreen extends StatefulWidget {
  const LakeGodChatScreen({super.key});

  @override
  State<LakeGodChatScreen> createState() => _LakeGodChatScreenState();
}

class _LakeGodChatScreenState extends State<LakeGodChatScreen>
    with SingleTickerProviderStateMixin {
  final LakeGodService _service = LakeGodService();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  final List<Map<String, dynamic>> _messages = [];
  bool _isSending = false;
  late AnimationController _welcomeAnimController;
  late Animation<double> _welcomeFadeAnim;

  @override
  void initState() {
    super.initState();
    _welcomeAnimController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1200),
    );
    _welcomeFadeAnim = CurvedAnimation(
      parent: _welcomeAnimController,
      curve: Curves.easeIn,
    );
    _addWelcome();
  }

  @override
  void dispose() {
    _welcomeAnimController.dispose();
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  void _addWelcome() {
    _messages.add({'content': '你好呀，我是湖神。有什么心事想和我聊聊吗？', 'is_mine': false, 'is_welcome': true});
    _welcomeAnimController.forward();
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

    setState(() {
      _messages.add({'content': content, 'is_mine': true});
      _isSending = true;
    });
    _controller.clear();
    FocusScope.of(context).unfocus();
    _scrollToBottom();

    try {
      final result = await _service.sendMessage(content);

      if (mounted) {
        setState(() {
          _isSending = false;
          if (result['success'] == true && result['data'] != null) {
            _messages.add({'content': result['data']['reply'] ?? '我在倾听...', 'is_mine': false});
          } else {
            _messages.add({'content': '网络不太好，稍后再试试吧~', 'is_mine': false});
          }
        });
        _scrollToBottom();
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _isSending = false;
          _messages.add({'content': '网络不太好，稍后再试试吧~', 'is_mine': false});
        });
        _scrollToBottom();
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      appBar: AppBar(
        title: Text(
          '湖神',
          style: TextStyle(
            color: AppTheme.candleGlow,
            fontWeight: FontWeight.bold,
            shadows: [
              Shadow(
                color: AppTheme.candleGlow.withValues(alpha: 0.6),
                blurRadius: 12,
              ),
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
              child: ListView.builder(
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

  Widget _buildMessageBubble(Map<String, dynamic> message) {
    final isMe = message['is_mine'] == true;
    final isWelcome = message['is_welcome'] == true;

    Widget bubble = Align(
      alignment: isMe ? Alignment.centerRight : Alignment.centerLeft,
      child: Container(
        margin: const EdgeInsets.symmetric(vertical: 4),
        constraints: BoxConstraints(maxWidth: MediaQuery.of(context).size.width * 0.7),
        child: isMe
            ? Container(
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                decoration: BoxDecoration(
                  gradient: const LinearGradient(
                    colors: [AppTheme.candleGlow, AppTheme.warmOrange],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                  borderRadius: BorderRadius.circular(16),
                ),
                child: Text(
                  message['content'] ?? '',
                  style: const TextStyle(color: AppTheme.darkTextPrimary, fontSize: 15),
                ),
              )
            : Container(
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(16),
                  boxShadow: [
                    BoxShadow(
                      color: AppTheme.candleGlow.withValues(alpha: 0.3),
                      blurRadius: 12,
                      spreadRadius: 1,
                    ),
                  ],
                ),
                child: SkyGlassCard(
                  borderRadius: 16,
                  enableGlow: false,
                  padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                  child: Text(
                    message['content'] ?? '',
                    style: TextStyle(color: AppTheme.darkTextPrimary.withValues(alpha: 0.9), fontSize: 15),
                  ),
                ),
              ),
      ),
    );

    // 欢迎消息淡入动画
    if (isWelcome) {
      return FadeTransition(opacity: _welcomeFadeAnim, child: bubble);
    }
    return bubble;
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
                hintText: '和湖神说说心里话...',
                hintStyle: TextStyle(color: AppTheme.darkTextSecondary.withValues(alpha: 0.6)),
                filled: true,
                fillColor: AppTheme.nightSurface,
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(16),
                  borderSide: BorderSide.none,
                ),
                enabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(16),
                  borderSide: BorderSide(
                    color: AppTheme.candleGlow.withValues(alpha: 0.15),
                  ),
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(16),
                  borderSide: const BorderSide(color: AppTheme.candleGlow, width: 1.5),
                ),
                contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
              ),
              maxLines: null,
              textInputAction: TextInputAction.send,
              onSubmitted: (_) => _sendMessage(),
            ),
          ),
          const SizedBox(width: 4),
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
