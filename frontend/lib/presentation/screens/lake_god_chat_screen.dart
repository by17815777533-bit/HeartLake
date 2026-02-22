// @file lake_god_chat_screen.dart
// @brief 湖神聊天界面 - 温馨治愈风格的AI咨询，集成EdgeAI情感分析与内容审核
import 'package:flutter/material.dart';
import '../../data/datasources/lake_god_service.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../../utils/app_theme.dart';
import '../widgets/atmospheric_background.dart';

class LakeGodChatScreen extends StatefulWidget {
  const LakeGodChatScreen({super.key});

  @override
  State<LakeGodChatScreen> createState() => _LakeGodChatScreenState();
}

class _LakeGodChatScreenState extends State<LakeGodChatScreen> {
  final LakeGodService _service = LakeGodService();
  final EdgeAIService _edgeAI = EdgeAIService();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  final List<Map<String, dynamic>> _messages = [];
  bool _isSending = false;
  bool _isLoadingHistory = false;
  bool _sessionReady = false;

  // 情绪脉搏数据
  Map<String, dynamic>? _emotionPulse;
  bool _isPulseLoading = true;

  @override
  void initState() {
    super.initState();
    _initSession();
  }

  @override
  void dispose() {
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  /// 初始化会话：startSession -> getMessages -> getEmotionPulse
  Future<void> _initSession() async {
    try {
      final sessionResult = await _service.startSession();
      if (sessionResult['success'] == true) {
        _sessionReady = true;
        // 并行加载历史消息和情绪脉搏
        await Future.wait([
          _loadHistory(),
          _loadEmotionPulse(),
        ]);
      } else {
        // 会话启动失败，仍显示欢迎语
        if (mounted) _addWelcome();
        await _loadEmotionPulse();
      }
    } catch (_) {
      if (mounted) _addWelcome();
      await _loadEmotionPulse();
    }

    // 如果历史消息为空，添加欢迎语
    if (_messages.isEmpty && mounted) {
      _addWelcome();
    }
  }

  /// 加载历史消息
  Future<void> _loadHistory() async {
    if (!_sessionReady) return;
    if (mounted) setState(() => _isLoadingHistory = true);
    try {
      final result = await _service.getMessages();
      if (!mounted) return;
      if (result['success'] == true && result['data'] != null) {
        final data = result['data'];
        final List<dynamic> history =
            data is List ? data : (data['messages'] ?? []);
        if (history.isNotEmpty) {
          setState(() {
            for (final msg in history) {
              _messages.add({
                'content': msg['content'] ?? '',
                'is_mine': msg['is_mine'] == true ||
                    msg['role'] == 'user' ||
                    msg['sender'] == 'user',
                'mood': msg['mood'],
              });
            }
          });
          _scrollToBottom();
        }
      }
    } catch (_) {
      // 静默失败，不影响使用
    } finally {
      if (mounted) setState(() => _isLoadingHistory = false);
    }
  }

  /// 加载情绪脉搏
  Future<void> _loadEmotionPulse() async {
    if (mounted) setState(() => _isPulseLoading = true);
    try {
      final resp = await _edgeAI.getEmotionPulse();
      if (!mounted) return;
      if (resp.success && resp.data != null) {
        setState(() => _emotionPulse = resp.data as Map<String, dynamic>);
      }
    } catch (_) {
      // 静默失败
    } finally {
      if (mounted) setState(() => _isPulseLoading = false);
    }
  }

  /// 下拉刷新 - 重新加载历史消息和情绪脉搏
  Future<void> _onRefresh() async {
    setState(() => _messages.clear());
    await Future.wait([
      _loadHistory(),
      _loadEmotionPulse(),
    ]);
    if (_messages.isEmpty) {
      _addWelcome();
    }
  }

  void _addWelcome() {
    setState(() {
      _messages.add({
        'content': '你好呀，我是湖神。有什么心事想和我聊聊吗？',
        'is_mine': false,
      });
    });
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

  /// 发送消息：内容审核 -> 情感分析 -> 发送 -> 刷新脉搏
  Future<void> _sendMessage() async {
    final content = _controller.text.trim();
    if (content.isEmpty || _isSending) return;

    _controller.clear();
    FocusScope.of(context).unfocus();

    setState(() => _isSending = true);

    try {
      // 1. 内容审核
      final moderateResp = await _edgeAI.moderateContent(content);
      if (moderateResp.success && moderateResp.data != null) {
        final moderateData = moderateResp.data as Map<String, dynamic>;
        if (moderateData['safe'] != true) {
          final reason = moderateData['reason'] ?? '内容不太合适';
          if (mounted) {
            setState(() => _isSending = false);
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(
                content: Text('消息未发送：$reason'),
                backgroundColor: AppTheme.warningColor,
                behavior: SnackBarBehavior.floating,
              ),
            );
          }
          return;
        }
      }

      // 2. 情感分析
      String? mood;
      double? score;
      try {
        final sentimentResp = await _edgeAI.analyzeSentiment(content);
        if (sentimentResp.success && sentimentResp.data != null) {
          final sentimentData = sentimentResp.data as Map<String, dynamic>;
          mood = sentimentData['mood'] as String?;
          score = (sentimentData['score'] as num?)?.toDouble();
        }
      } catch (_) {
        // 情感分析失败不阻塞发送
      }

      // 3. 添加用户消息到列表
      setState(() {
        _messages.add({
          'content': content,
          'is_mine': true,
          'mood': mood,
          'score': score,
        });
      });
      _scrollToBottom();

      // 4. 发送消息给湖神
      final result = await _service.sendMessage(content);

      if (mounted) {
        setState(() {
          _isSending = false;
          if (result['success'] == true && result['data'] != null) {
            _messages.add({
              'content': result['data']['reply'] ?? '我在倾听...',
              'is_mine': false,
            });
          } else {
            _messages.add({
              'content': '网络不太好，稍后再试试吧~',
              'is_mine': false,
            });
          }
        });
        _scrollToBottom();
      }

      // 5. 异步刷新情绪脉搏
      _loadEmotionPulse();
    } catch (e) {
      if (mounted) {
        setState(() {
          _isSending = false;
          _messages.add({
            'content': '网络不太好，稍后再试试吧~',
            'is_mine': false,
          });
        });
        _scrollToBottom();
      }
    }
  }

  /// 情绪中文映射
  String _moodLabel(String? mood) {
    const map = {
      'happy': '开心',
      'sad': '难过',
      'angry': '生气',
      'anxious': '焦虑',
      'calm': '平静',
      'excited': '兴奋',
      'neutral': '平和',
      'fear': '恐惧',
      'surprise': '惊讶',
      'disgust': '厌恶',
      'love': '喜爱',
      'hopeful': '期待',
      'lonely': '孤独',
      'confused': '困惑',
      'grateful': '感恩',
    };
    if (mood == null) return '';
    return map[mood.toLowerCase()] ?? mood;
  }

  /// 情绪趋势中文映射
  String _trendLabel(String? trend) {
    const map = {
      'stable': '平稳',
      'rising': '上升',
      'falling': '下降',
      'volatile': '波动',
    };
    if (trend == null) return '未知';
    return map[trend.toLowerCase()] ?? trend;
  }

  /// 情绪对应颜色
  Color _moodColor(String? mood) {
    const map = {
      'happy': Color(0xFF66BB6A),
      'sad': Color(0xFF42A5F5),
      'angry': Color(0xFFEF5350),
      'anxious': Color(0xFFFFA726),
      'calm': Color(0xFF26C6DA),
      'excited': Color(0xFFFF7043),
      'neutral': Color(0xFF78909C),
      'fear': Color(0xFF8D6E63),
      'love': Color(0xFFEC407A),
      'hopeful': Color(0xFFAB47BC),
      'lonely': Color(0xFF5C6BC0),
      'confused': Color(0xFFFFCA28),
      'grateful': Color(0xFF26A69A),
    };
    if (mood == null) return AppTheme.textTertiary;
    return map[mood.toLowerCase()] ?? AppTheme.textTertiary;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text('湖神'),
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: Colors.white,
      ),
      body: Stack(
        children: [
          const Positioned.fill(
            child: SceneTransitionBackground(
              scene: LakeScene.underwater,
              child: SizedBox.expand(),
            ),
          ),
          SafeArea(
            child: Column(
              children: [
                // AI情绪脉搏小组件
                _buildEmotionPulseWidget(),
                // 消息列表（支持下拉刷新）
                Expanded(
                  child: RefreshIndicator(
                    onRefresh: _onRefresh,
                    color: AppTheme.primaryColor,
                    child: _isLoadingHistory && _messages.isEmpty
                        ? const Center(
                            child: CircularProgressIndicator(
                              color: Colors.white70,
                            ),
                          )
                        : ListView.builder(
                            controller: _scrollController,
                            physics: const AlwaysScrollableScrollPhysics(),
                            padding: const EdgeInsets.all(16),
                            itemCount: _messages.length,
                            itemBuilder: (context, index) =>
                                _buildMessageBubble(_messages[index]),
                          ),
                  ),
                ),
                _buildInputBar(),
              ],
            ),
          ),
        ],
      ),
    );
  }

  /// AI情绪脉搏小组件
  Widget _buildEmotionPulseWidget() {
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.15),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(
          color: Colors.white.withValues(alpha: 0.2),
        ),
      ),
      child: _isPulseLoading
          ? Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                SizedBox(
                  width: 14,
                  height: 14,
                  child: CircularProgressIndicator(
                    strokeWidth: 1.5,
                    color: Colors.white.withValues(alpha: 0.7),
                  ),
                ),
                const SizedBox(width: 8),
                Text(
                  '正在感知情绪脉搏...',
                  style: TextStyle(
                    color: Colors.white.withValues(alpha: 0.7),
                    fontSize: 12,
                  ),
                ),
              ],
            )
          : Row(
              children: [
                // 脉搏图标
                Container(
                  width: 28,
                  height: 28,
                  decoration: BoxDecoration(
                    color: _pulseColor.withValues(alpha: 0.25),
                    shape: BoxShape.circle,
                  ),
                  child: Icon(
                    Icons.favorite,
                    size: 15,
                    color: _pulseColor,
                  ),
                ),
                const SizedBox(width: 10),
                // 脉搏信息
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'AI情绪脉搏',
                        style: TextStyle(
                          color: Colors.white.withValues(alpha: 0.9),
                          fontSize: 12,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                      const SizedBox(height: 2),
                      Text(
                        _emotionPulse != null
                            ? '情绪均值 ${((_emotionPulse!['avgScore'] ?? 0) as num).toStringAsFixed(1)}'
                                '  ·  趋势${_trendLabel(_emotionPulse!['trend'] as String?)}'
                                '  ·  ${_emotionPulse!['sampleCount'] ?? 0}条样本'
                            : '暂无数据',
                        style: TextStyle(
                          color: Colors.white.withValues(alpha: 0.65),
                          fontSize: 11,
                        ),
                      ),
                    ],
                  ),
                ),
                // 刷新按钮
                GestureDetector(
                  onTap: _loadEmotionPulse,
                  child: Icon(
                    Icons.refresh,
                    size: 16,
                    color: Colors.white.withValues(alpha: 0.5),
                  ),
                ),
              ],
            ),
    );
  }

  /// 脉搏颜色：根据avgScore映射
  Color get _pulseColor {
    if (_emotionPulse == null) return Colors.white70;
    final avg = ((_emotionPulse!['avgScore'] ?? 0.5) as num).toDouble();
    if (avg >= 0.7) return const Color(0xFF66BB6A); // 积极
    if (avg >= 0.4) return const Color(0xFF26C6DA); // 平和
    return const Color(0xFFFF7043); // 低落
  }

  Widget _buildMessageBubble(Map<String, dynamic> message) {
    final isMe = message['is_mine'] == true;
    final mood = message['mood'] as String?;
    final moodText = _moodLabel(mood);

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Column(
        crossAxisAlignment:
            isMe ? CrossAxisAlignment.end : CrossAxisAlignment.start,
        children: [
          // 消息气泡
          Align(
            alignment: isMe ? Alignment.centerRight : Alignment.centerLeft,
            child: Container(
              padding:
                  const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
              constraints: BoxConstraints(
                maxWidth: MediaQuery.of(context).size.width * 0.7,
              ),
              decoration: BoxDecoration(
                color: isMe
                    ? AppTheme.primaryColor
                    : Colors.white.withValues(alpha: 0.9),
                borderRadius: BorderRadius.only(
                  topLeft: const Radius.circular(16),
                  topRight: const Radius.circular(16),
                  bottomLeft: Radius.circular(isMe ? 16 : 4),
                  bottomRight: Radius.circular(isMe ? 4 : 16),
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(alpha: 0.08),
                    blurRadius: 6,
                    offset: const Offset(0, 2),
                  ),
                ],
              ),
              child: Text(
                message['content'] ?? '',
                style: TextStyle(
                  color: isMe ? Colors.white : AppTheme.textPrimary,
                  fontSize: 15,
                  height: 1.4,
                ),
              ),
            ),
          ),
          // 情绪标签（仅用户消息且有情绪数据时显示）
          if (isMe && moodText.isNotEmpty)
            Padding(
              padding: const EdgeInsets.only(top: 3, right: 4),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  Container(
                    padding:
                        const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                    decoration: BoxDecoration(
                      color: _moodColor(mood).withValues(alpha: 0.15),
                      borderRadius: BorderRadius.circular(10),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(
                          Icons.spa_outlined,
                          size: 10,
                          color: _moodColor(mood),
                        ),
                        const SizedBox(width: 3),
                        Text(
                          '情绪：$moodText',
                          style: TextStyle(
                            color: _moodColor(mood),
                            fontSize: 10,
                            fontWeight: FontWeight.w500,
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildInputBar() {
    return Container(
      padding: EdgeInsets.only(
        left: 16,
        right: 8,
        top: 8,
        bottom: MediaQuery.of(context).padding.bottom + 8,
      ),
      decoration: BoxDecoration(
        color: Colors.white,
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
                hintText: '和湖神说说心里话...',
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
