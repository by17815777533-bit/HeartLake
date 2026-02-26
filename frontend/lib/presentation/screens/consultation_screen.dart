// @file consultation_screen.dart
// @brief 心理咨询预约和会话页面 - 支持E2E加密
import 'package:flutter/material.dart';
import '../../data/datasources/consultation_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../widgets/atmospheric_background.dart';

// ============================================================
// 主页面：预约咨询 + 我的会话 两个Tab
// ============================================================
class ConsultationScreen extends StatefulWidget {
  const ConsultationScreen({super.key});

  @override
  State<ConsultationScreen> createState() => _ConsultationScreenState();
}

class _ConsultationScreenState extends State<ConsultationScreen>
    with SingleTickerProviderStateMixin {
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text('心理咨询',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: Colors.white,
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: Colors.white,
          labelColor: Colors.white,
          unselectedLabelColor: Colors.white70,
          tabs: const [
            Tab(text: '预约咨询'),
            Tab(text: '我的会话'),
          ],
        ),
      ),
      body: Stack(
        children: [
          const Positioned.fill(
            child: AtmosphericBackground(
              enableParticles: true,
              particleCount: 15,
              child: SizedBox.expand(),
            ),
          ),
          TabBarView(
            controller: _tabController,
            children: [
              _CounselorListTab(onStartChat: _goToChat),
              _SessionListTab(onOpenSession: _goToChatWithSession),
            ],
          ),
        ],
      ),
    );
  }

  void _goToChat(String counselorId, String counselorName) {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => _ConsultationChatScreen(
          counselorId: counselorId,
          counselorName: counselorName,
        ),
      ),
    );
  }

  void _goToChatWithSession(String sessionId, String counselorName) {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => _ConsultationChatScreen(
          sessionId: sessionId,
          counselorName: counselorName,
        ),
      ),
    );
  }
}

// ============================================================
// Tab1: 咨询师列表（占位数据）
// ============================================================
class _CounselorListTab extends StatelessWidget {
  final void Function(String counselorId, String counselorName) onStartChat;

  const _CounselorListTab({required this.onStartChat});

  static final List<Map<String, dynamic>> _counselors = [
    {
      'id': 'counselor_001',
      'name': '林心怡',
      'avatar': '🧑‍⚕️',
      'specialty': '情绪管理 · 压力疏导',
      'rating': 4.9,
      'sessions': 1280,
      'description': '国家二级心理咨询师，擅长认知行为疗法与正念减压',
    },
    {
      'id': 'counselor_002',
      'name': '苏晓晨',
      'avatar': '🧑‍⚕️',
      'specialty': '人际关系 · 社交焦虑',
      'rating': 4.8,
      'sessions': 960,
      'description': '应用心理学硕士，专注人际沟通与社交适应训练',
    },
    {
      'id': 'counselor_003',
      'name': '陈思源',
      'avatar': '🧑‍⚕️',
      'specialty': '自我成长 · 心理健康',
      'rating': 4.9,
      'sessions': 1100,
      'description': '临床心理学博士，擅长自我探索与积极心理干预',
    },
  ];

  @override
  Widget build(BuildContext context) {
    return ListView.builder(
      padding: EdgeInsets.only(
        top: MediaQuery.of(context).padding.top + kToolbarHeight + 60,
        bottom: 20,
        left: 16,
        right: 16,
      ),
      itemCount: _counselors.length,
      itemBuilder: (context, index) {
        final c = _counselors[index];
        return _CounselorCard(
          name: c['name'] as String,
          avatar: c['avatar'] as String,
          specialty: c['specialty'] as String,
          rating: c['rating'] as double,
          sessions: c['sessions'] as int,
          description: c['description'] as String,
          onBook: () => onStartChat(
            c['id'] as String,
            c['name'] as String,
          ),
        );
      },
    );
  }
}

// ============================================================
// 咨询师卡片
// ============================================================
class _CounselorCard extends StatelessWidget {
  final String name;
  final String avatar;
  final String specialty;
  final double rating;
  final int sessions;
  final String description;
  final VoidCallback onBook;

  const _CounselorCard({
    required this.name,
    required this.avatar,
    required this.specialty,
    required this.rating,
    required this.sessions,
    required this.description,
    required this.onBook,
  });

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.92) : Colors.white.withValues(alpha: 0.92),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                // 头像
                Container(
                  width: 56,
                  height: 56,
                  decoration: BoxDecoration(
                    color: AppTheme.skyBlue.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(28),
                  ),
                  alignment: Alignment.center,
                  child: Text(avatar, style: const TextStyle(fontSize: 28)),
                ),
                const SizedBox(width: 12),
                // 信息
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(name,
                          style: const TextStyle(
                              fontSize: 17, fontWeight: FontWeight.w600)),
                      const SizedBox(height: 4),
                      Text(specialty,
                          style: TextStyle(
                              fontSize: 13, color: isDark ? Colors.white70 : AppTheme.textSecondary)),
                    ],
                  ),
                ),
                // 评分
                Column(
                  children: [
                    Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        const Icon(Icons.star_rounded,
                            color: AppTheme.accentColor, size: 18),
                        const SizedBox(width: 2),
                        Text('$rating',
                            style: const TextStyle(
                                fontWeight: FontWeight.bold, fontSize: 15)),
                      ],
                    ),
                    const SizedBox(height: 2),
                    Text('$sessions次咨询',
                        style: const TextStyle(
                            fontSize: 11, color: AppTheme.textTertiary)),
                  ],
                ),
              ],
            ),
            const SizedBox(height: 10),
            Text(description,
                style: TextStyle(
                    fontSize: 13, color: isDark ? Colors.white70 : AppTheme.textSecondary)),
            const SizedBox(height: 12),
            SizedBox(
              width: double.infinity,
              child: ElevatedButton(
                onPressed: onBook,
                style: ElevatedButton.styleFrom(
                  backgroundColor: AppTheme.primaryColor,
                  foregroundColor: Colors.white,
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12)),
                  padding: const EdgeInsets.symmetric(vertical: 10),
                ),
                child: const Text('预约咨询'),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ============================================================
// Tab2: 我的会话列表
// ============================================================
class _SessionListTab extends StatefulWidget {
  final void Function(String sessionId, String counselorName) onOpenSession;

  const _SessionListTab({required this.onOpenSession});

  @override
  State<_SessionListTab> createState() => _SessionListTabState();
}

class _SessionListTabState extends State<_SessionListTab> {
  final ConsultationService _service = sl<ConsultationService>();
  List<Map<String, dynamic>> _sessions = [];
  bool _isLoading = true;
  String? _error;

  @override
  void initState() {
    super.initState();
    _loadSessions();
  }

  Future<void> _loadSessions() async {
    setState(() {
      _isLoading = true;
      _error = null;
    });
    try {
      final result = await _service.getSessions();
      if (result['success'] == true && result['data'] != null) {
        final data = result['data'];
        final List<dynamic> list =
            data is List ? data : (data['sessions'] ?? []);
        _sessions = list.cast<Map<String, dynamic>>();
      }
    } catch (e) {
      _error = '加载失败，请稍后重试';
    }
    if (mounted) setState(() => _isLoading = false);
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final topPadding =
        MediaQuery.of(context).padding.top + kToolbarHeight + 60;

    if (_isLoading) {
      return Center(
        child: Padding(
          padding: EdgeInsets.only(top: topPadding),
          child: const CircularProgressIndicator(color: Colors.white70),
        ),
      );
    }

    if (_error != null) {
      return Center(
        child: Padding(
          padding: EdgeInsets.only(top: topPadding),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(_error!, style: const TextStyle(color: Colors.white70)),
              const SizedBox(height: 12),
              TextButton(
                onPressed: _loadSessions,
                child:
                    const Text('重试', style: TextStyle(color: Colors.white)),
              ),
            ],
          ),
        ),
      );
    }

    if (_sessions.isEmpty) {
      return Center(
        child: Padding(
          padding: EdgeInsets.only(top: topPadding),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.chat_bubble_outline,
                  size: 64, color: Colors.white.withValues(alpha: 0.4)),
              const SizedBox(height: 12),
              Text('还没有咨询记录',
                  style: TextStyle(
                      color: Colors.white.withValues(alpha: 0.6),
                      fontSize: 15)),
              const SizedBox(height: 4),
              Text('预约一位咨询师开始吧',
                  style: TextStyle(
                      color: Colors.white.withValues(alpha: 0.4),
                      fontSize: 13)),
            ],
          ),
        ),
      );
    }

    return RefreshIndicator(
      onRefresh: _loadSessions,
      color: AppTheme.primaryColor,
      child: ListView.builder(
        padding: EdgeInsets.only(
          top: topPadding,
          bottom: 20,
          left: 16,
          right: 16,
        ),
        itemCount: _sessions.length,
        itemBuilder: (context, index) {
          final s = _sessions[index];
          final counselorName =
              s['counselor_name'] as String? ?? '咨询师';
          final lastMessage =
              s['last_message'] as String? ?? '暂无消息';
          final time = s['updated_at'] as String? ?? '';
          final sessionId = s['session_id'] as String? ?? '';

          return Card(
            margin: const EdgeInsets.only(bottom: 10),
            color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.92) : Colors.white.withValues(alpha: 0.92),
            shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(14)),
            child: ListTile(
              contentPadding:
                  const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              leading: CircleAvatar(
                backgroundColor:
                    AppTheme.skyBlue.withValues(alpha: 0.2),
                child: const Icon(Icons.person, color: AppTheme.primaryColor),
              ),
              title: Text(counselorName,
                  style: const TextStyle(fontWeight: FontWeight.w600)),
              subtitle: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const SizedBox(height: 4),
                  Text(lastMessage,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: TextStyle(
                          fontSize: 13, color: isDark ? Colors.white70 : AppTheme.textSecondary)),
                  if (time.isNotEmpty) ...[
                    const SizedBox(height: 2),
                    Text(time,
                        style: const TextStyle(
                            fontSize: 11, color: AppTheme.textTertiary)),
                  ],
                ],
              ),
              trailing: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(Icons.lock_outline,
                      size: 14,
                      color: AppTheme.successColor.withValues(alpha: 0.7)),
                  const SizedBox(width: 4),
                  const Icon(Icons.chevron_right,
                      color: AppTheme.textTertiary),
                ],
              ),
              onTap: () =>
                  widget.onOpenSession(sessionId, counselorName),
            ),
          );
        },
      ),
    );
  }
}

// ============================================================
// 聊天界面
// ============================================================
class _ConsultationChatScreen extends StatefulWidget {
  final String? counselorId;
  final String? sessionId;
  final String counselorName;

  const _ConsultationChatScreen({
    this.counselorId,
    this.sessionId,
    required this.counselorName,
  });

  @override
  State<_ConsultationChatScreen> createState() =>
      _ConsultationChatScreenState();
}

class _ConsultationChatScreenState extends State<_ConsultationChatScreen> {
  final ConsultationService _service = sl<ConsultationService>();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  final List<Map<String, dynamic>> _messages = [];

  String? _sessionId;
  bool _isSending = false;
  bool _isLoading = true;

  @override
  void initState() {
    super.initState();
    _sessionId = widget.sessionId;
    _initChat();
  }

  @override
  void dispose() {
    _service.dispose();
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  Future<void> _initChat() async {
    // 如果没有sessionId，先创建会话
    if (_sessionId == null) {
      final result =
          await _service.createSession(counselorId: widget.counselorId);
      if (result['success'] == true && result['data'] != null) {
        _sessionId = result['data']['session_id'] as String?;
      }
      if (_sessionId == null) {
        if (mounted) {
          setState(() => _isLoading = false);
          _addSystemMessage('会话创建失败，请稍后重试');
        }
        return;
      }
    }

    // 初始化 E2E 加密
    await _service.initE2E(_sessionId!);

    // 加载历史消息
    await _loadMessages();
    if (mounted) setState(() => _isLoading = false);
  }

  Future<void> _loadMessages() async {
    if (_sessionId == null) return;
    try {
      final result = await _service.getMessages(_sessionId!);
      if (result['success'] == true && result['data'] != null) {
        final data = result['data'];
        final List<dynamic> history =
            data is List ? data : (data['messages'] ?? []);
        for (final msg in history) {
          _messages.add({
            'content': msg['content'] ?? '',
            'isMe': msg['sender_type'] == 'user',
            'time': msg['created_at'] ?? '',
          });
        }
      }
    } catch (_) {
      // 静默处理
    }

    if (_messages.isEmpty) {
      _addSystemMessage('你好，我是${widget.counselorName}，很高兴为你提供心理咨询服务。请放心倾诉，我们的对话受到端到端加密保护。');
    }

    _scrollToBottom();
  }

  void _addSystemMessage(String content) {
    _messages.add({
      'content': content,
      'isMe': false,
      'time': '',
    });
  }

  Future<void> _sendMessage() async {
    final text = _controller.text.trim();
    if (text.isEmpty || _isSending || _sessionId == null) return;

    _controller.clear();
    setState(() {
      _messages.add({
        'content': text,
        'isMe': true,
        'time': _nowStr(),
      });
      _isSending = true;
    });
    _scrollToBottom();

    try {
      final result = await _service.sendMessage(
        sessionId: _sessionId!,
        content: text,
      );
      if (result['success'] == true && result['data'] != null) {
        final reply = result['data']['reply'] ?? result['data']['content'] ?? '';
        if (reply.toString().isNotEmpty) {
          _messages.add({
            'content': reply,
            'isMe': false,
            'time': _nowStr(),
          });
        }
      }
    } catch (_) {
      _addSystemMessage('消息发送失败，请重试');
    }

    if (mounted) {
      setState(() => _isSending = false);
      _scrollToBottom();
    }
  }

  String _nowStr() {
    final now = DateTime.now();
    return '${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}';
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

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: Column(
          children: [
            Text(widget.counselorName,
                style: const TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                    fontSize: 17)),
            Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.lock_outline,
                    size: 11,
                    color: Colors.white.withValues(alpha: 0.8)),
                const SizedBox(width: 3),
                Text('端到端加密保护',
                    style: TextStyle(
                        color: Colors.white.withValues(alpha: 0.8),
                        fontSize: 11)),
              ],
            ),
          ],
        ),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: Colors.white,
      ),
      body: Stack(
        children: [
          const Positioned.fill(
            child: AtmosphericBackground(
              enableParticles: true,
              particleCount: 10,
              child: SizedBox.expand(),
            ),
          ),
          Column(
            children: [
              // E2E加密徽章
              Container(
                width: double.infinity,
                padding: EdgeInsets.only(
                  top: MediaQuery.of(context).padding.top + kToolbarHeight + 8,
                  bottom: 8,
                ),
                alignment: Alignment.center,
                child: Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
                  decoration: BoxDecoration(
                    color: AppTheme.successColor.withValues(alpha: 0.15),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(Icons.verified_user_outlined,
                          size: 14,
                          color:
                              AppTheme.successColor.withValues(alpha: 0.8)),
                      const SizedBox(width: 4),
                      Text('🔒 对话内容已加密，仅你和咨询师可见',
                          style: TextStyle(
                              fontSize: 11,
                              color: AppTheme.successColor
                                  .withValues(alpha: 0.8))),
                    ],
                  ),
                ),
              ),
              // 消息列表
              Expanded(
                child: _isLoading
                    ? const Center(
                        child: CircularProgressIndicator(
                            color: Colors.white70))
                    : ListView.builder(
                        controller: _scrollController,
                        padding: const EdgeInsets.symmetric(
                            horizontal: 16, vertical: 8),
                        itemCount: _messages.length,
                        itemBuilder: (context, index) {
                          final msg = _messages[index];
                          return _ChatBubble(
                            content: msg['content'] as String,
                            isMe: msg['isMe'] as bool,
                            time: msg['time'] as String,
                            counselorName: widget.counselorName,
                          );
                        },
                      ),
              ),
              // 输入框
              _buildInputBar(),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildInputBar() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Container(
      padding: EdgeInsets.only(
        left: 12,
        right: 8,
        top: 8,
        bottom: MediaQuery.of(context).padding.bottom + 8,
      ),
      decoration: BoxDecoration(
        color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.95) : Colors.white.withValues(alpha: 0.95),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.05),
            blurRadius: 4,
            offset: const Offset(0, -2),
          ),
        ],
      ),
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: _controller,
              decoration: const InputDecoration(
                hintText: '说说你的感受...',
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
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.send, color: AppTheme.primaryColor),
            onPressed: _isSending ? null : _sendMessage,
          ),
        ],
      ),
    );
  }
}

// ============================================================
// 聊天气泡
// ============================================================
class _ChatBubble extends StatelessWidget {
  final String content;
  final bool isMe;
  final String time;
  final String counselorName;

  const _ChatBubble({
    required this.content,
    required this.isMe,
    required this.time,
    required this.counselorName,
  });

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment:
            isMe ? MainAxisAlignment.end : MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (!isMe) ...[
            CircleAvatar(
              radius: 16,
              backgroundColor: AppTheme.skyBlue.withValues(alpha: 0.3),
              child: Text(counselorName.isNotEmpty ? counselorName[0] : '咨',
                  style: const TextStyle(
                      fontSize: 14, color: AppTheme.primaryColor)),
            ),
            const SizedBox(width: 8),
          ],
          Flexible(
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
              decoration: BoxDecoration(
                color: isMe
                    ? AppTheme.primaryColor.withValues(alpha: 0.9)
                    : isDark ? const Color(0xFF16213E).withValues(alpha: 0.92) : Colors.white.withValues(alpha: 0.92),
                borderRadius: BorderRadius.only(
                  topLeft: const Radius.circular(16),
                  topRight: const Radius.circular(16),
                  bottomLeft: Radius.circular(isMe ? 16 : 4),
                  bottomRight: Radius.circular(isMe ? 4 : 16),
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(alpha: 0.06),
                    blurRadius: 4,
                    offset: const Offset(0, 2),
                  ),
                ],
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    content,
                    style: TextStyle(
                      fontSize: 15,
                      color: isMe ? Colors.white : isDark ? Colors.white : AppTheme.textPrimary,
                      height: 1.4,
                    ),
                  ),
                  if (time.isNotEmpty) ...[
                    const SizedBox(height: 4),
                    Text(
                      time,
                      style: TextStyle(
                        fontSize: 10,
                        color: isMe
                            ? Colors.white.withValues(alpha: 0.6)
                            : AppTheme.textTertiary,
                      ),
                    ),
                  ],
                ],
              ),
            ),
          ),
          if (isMe) const SizedBox(width: 8),
        ],
      ),
    );
  }
}
