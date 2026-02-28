/// 好友聊天界面
///
/// 与好友的实时消息收发页面。
import 'package:flutter/material.dart';
import '../../data/datasources/friend_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/storage_util.dart';
import '../../utils/input_validator.dart';
import '../../utils/app_theme.dart';
import '../widgets/water_background.dart';

/// 好友聊天页面
///
/// 与指定好友的一对一实时聊天，基于 WebSocket 收发消息。
/// 消息内容经过 [InputValidator] 校验和 sanitize 处理。
/// 支持加载历史消息、实时接收新消息、自动滚动到底部。
class FriendChatScreen extends StatefulWidget {
  final String friendId;
  final String? friendName;

  const FriendChatScreen({super.key, required this.friendId, this.friendName});

  @override
  State<FriendChatScreen> createState() => _FriendChatScreenState();
}

/// 好友聊天页面的状态管理
///
/// 通过 [FriendService] 收发消息，[WebSocketManager] 实时接收新消息。
/// 采用乐观更新策略：发送消息时立即追加到本地列表，失败后回滚并恢复输入。
class _FriendChatScreenState extends State<FriendChatScreen> {
  final FriendService _friendService = sl<FriendService>();
  final WebSocketManager _wsManager = WebSocketManager();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  List<dynamic> _messages = [];
  bool _isLoading = true;
  bool _isSending = false;
  String? _currentUserId;
  String? _loadError;

  @override
  void initState() {
    super.initState();
    _init();
    _wsManager.on('new_friend_message', _onNewMessage);
  }

  /// 初始化流程：先获取当前用户ID，校验好友ID合法性后加载历史消息
  Future<void> _init() async {
    await _initCurrentUser();
    if (!_isFriendIdUsable(widget.friendId)) {
      if (!mounted) return;
      setState(() {
        _isLoading = false;
        _loadError = '好友标识异常，无法加载聊天';
      });
      return;
    }
    await _loadMessages();
  }

  @override
  void dispose() {
    _wsManager.off('new_friend_message', _onNewMessage);
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  /// 从本地存储读取当前登录用户ID，用于判断消息归属
  Future<void> _initCurrentUser() async {
    _currentUserId = await StorageUtil.getUserId();
  }

  /// WebSocket 新消息回调，仅处理来自当前聊天好友的消息
  void _onNewMessage(Map<String, dynamic> data) {
    final senderId = data['sender_id']?.toString();
    if (senderId != widget.friendId) return;
    if (!mounted) return;
    setState(() {
      _messages.add({
        'content': data['content'],
        'sender_id': senderId,
        'receiver_id': data['receiver_id'],
        'created_at': data['created_at'],
        'is_mine': false,
      });
    });
    _scrollToBottom();
  }

  /// 从后端加载历史消息，并根据 sender_id 计算 is_mine 字段
  Future<void> _loadMessages() async {
    if (!_isFriendIdUsable(widget.friendId)) {
      if (mounted) {
        setState(() {
          _isLoading = false;
          _loadError = '好友标识异常，无法加载聊天';
        });
      }
      return;
    }
    try {
      final result = await _friendService.getMessages(widget.friendId);
      if (mounted) {
        if (result['success'] != true) {
          setState(() {
            _messages = [];
            _isLoading = false;
            _loadError = result['message']?.toString() ?? '加载消息失败';
          });
          return;
        }

        final rawMessages = result['messages'] ?? [];
        // 为每条消息计算 is_mine（后端只返回 sender_id，没有 is_mine）
        final messages = (rawMessages as List).map((msg) {
          final m = Map<String, dynamic>.from(msg as Map);
          m['is_mine'] = m['sender_id']?.toString() == _currentUserId;
          return m;
        }).toList();
        setState(() {
          _messages = messages;
          _isLoading = false;
          _loadError = null;
        });
        _scrollToBottom();
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _isLoading = false;
          _loadError = '加载消息失败，请重试';
        });
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载消息失败，请下拉重试')),
        );
      }
    }
  }

  /// 在下一帧将消息列表滚动到底部
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

  /// 发送消息，采用乐观更新：先追加到本地列表，失败后回滚并恢复输入框内容
  Future<void> _sendMessage() async {
    final content = _controller.text.trim();
    if (content.isEmpty || _isSending) return;
    if (!_isFriendIdUsable(widget.friendId)) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('好友标识异常，无法发送消息')),
      );
      return;
    }

    setState(() => _isSending = true);
    _controller.clear();
    FocusScope.of(context).unfocus();

    // 乐观更新：立即追加消息到本地列表
    final optimisticMessage = {
      'content': content,
      'sender_id': _currentUserId,
      'receiver_id': widget.friendId,
      'created_at': DateTime.now().toIso8601String(),
      'is_mine': true,
    };
    if (mounted) {
      setState(() {
        _messages.add(optimisticMessage);
      });
      _scrollToBottom();
    }

    try {
      final result = await _friendService.sendMessage(widget.friendId, content);

      if (mounted) {
        setState(() => _isSending = false);
        if (result['success'] != true) {
          // 发送失败，移除乐观消息并恢复输入
          setState(() {
            _messages.remove(optimisticMessage);
          });
          _controller.text = content;
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
                content: Text(result['message'] ?? '发送失败'),
                backgroundColor: Colors.red),
          );
        }
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _isSending = false;
          _messages.remove(optimisticMessage);
        });
        _controller.text = content;
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
              content: Text('网络异常，请稍后再试'), backgroundColor: Colors.red),
        );
      }
    }
  }

  /// 校验好友ID是否为合法 UUID 格式
  bool _isFriendIdUsable(String value) {
    try {
      InputValidator.validateUUID(value, '好友ID');
      return true;
    } catch (_) {
      return false;
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.friendName ?? '聊天'),
        backgroundColor:
            isDark ? const Color(0xFF1A1A2E) : AppTheme.primaryColor,
        foregroundColor: Colors.white,
      ),
      body: Stack(
        children: [
          const Positioned.fill(child: WaterBackground()),
          Column(
            children: [
              Expanded(
                child: _isLoading
                    ? const Center(child: CircularProgressIndicator())
                    : _loadError != null
                        ? Center(
                            child: Padding(
                              padding:
                                  const EdgeInsets.symmetric(horizontal: 24),
                              child: Column(
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  Text(
                                    _loadError!,
                                    style: TextStyle(
                                      color: isDark
                                          ? Colors.white70
                                          : const Color(0xFF1A1A2E),
                                      fontSize: 14,
                                    ),
                                    textAlign: TextAlign.center,
                                  ),
                                  const SizedBox(height: 12),
                                  ElevatedButton(
                                    onPressed: _loadMessages,
                                    child: const Text('重试'),
                                  ),
                                ],
                              ),
                            ),
                          )
                        : _messages.isEmpty
                            ? Center(
                                child: Text(
                                  '还没有消息，说点什么吧~',
                                  style: TextStyle(
                                    color: isDark
                                        ? Colors.white70
                                        : Colors.grey.shade700,
                                  ),
                                ),
                              )
                            : ListView.builder(
                                controller: _scrollController,
                                padding: const EdgeInsets.all(16),
                                itemCount: _messages.length,
                                itemBuilder: (context, index) =>
                                    _buildMessageBubble(_messages[index]),
                              ),
              ),
              _buildInputBar(),
            ],
          ),
        ],
      ),
    );
  }

  /// 构建单条消息气泡，用户消息靠右蓝色、好友消息靠左灰色
  Widget _buildMessageBubble(dynamic message) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final isMe = message['is_mine'] == true;
    return Align(
      alignment: isMe ? Alignment.centerRight : Alignment.centerLeft,
      child: Container(
        margin: const EdgeInsets.symmetric(vertical: 4),
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
        constraints:
            BoxConstraints(maxWidth: MediaQuery.of(context).size.width * 0.7),
        decoration: BoxDecoration(
          color: isMe
              ? AppTheme.primaryColor
              : isDark
                  ? const Color(0xFF16213E)
                  : Colors.grey[200],
          borderRadius: BorderRadius.circular(16),
        ),
        child: Text(
          message['content'] ?? '',
          style: TextStyle(
              color: isMe
                  ? Colors.white
                  : isDark
                      ? Colors.white
                      : Colors.black87,
              fontSize: 15),
        ),
      ),
    );
  }

  /// 构建底部消息输入栏，包含文本输入框和发送按钮
  Widget _buildInputBar() {
        boxShadow: [
          BoxShadow(
              color: Colors.black.withValues(alpha: 0.05),
              blurRadius: 4,
              offset: const Offset(0, -2))
        ],
      ),
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: _controller,
              decoration: const InputDecoration(
                hintText: '说点什么...',
                border: InputBorder.none,
                contentPadding:
                    EdgeInsets.symmetric(horizontal: 12, vertical: 8),
              ),
              maxLines: null,
              textInputAction: TextInputAction.send,
              onSubmitted: (_) => _sendMessage(),
            ),
          ),
          IconButton(
            icon: _isSending
                ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2))
                : const Icon(Icons.send, color: AppTheme.primaryColor),
            onPressed: _isSending ? null : _sendMessage,
          ),
        ],
      ),
    );
  }
}
