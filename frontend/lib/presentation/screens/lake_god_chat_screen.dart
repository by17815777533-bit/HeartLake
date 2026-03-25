// 湖神聊天界面
//
// AI陪伴对话页面，集成EdgeAI情感分析与内容审核。
import 'package:flutter/material.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../../data/datasources/lake_god_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../widgets/atmospheric_background.dart';

/// 湖神聊天页面
///
/// 用户与 AI 湖神的对话界面，提供温馨治愈风格的情感陪伴。
/// 集成 EdgeAI 端侧能力：
/// - 发送前进行情感分析，识别用户当前情绪状态
/// - 内容审核过滤不当内容
/// - 高风险情绪自动触发安全港引导
///
/// 对话历史通过 [LakeGodService] 与后端同步。
class LakeGodChatScreen extends StatefulWidget {
  const LakeGodChatScreen({super.key});

  @override
  State<LakeGodChatScreen> createState() => _LakeGodChatScreenState();
}

/// 湖神聊天页面的状态管理
///
/// 管理消息列表、发送状态和情绪脉搏数据。
/// 初始化时并行加载历史消息和情绪脉搏，无历史消息时展示欢迎语。
class _LakeGodChatScreenState extends State<LakeGodChatScreen> {
  final LakeGodService _service = sl<LakeGodService>();
  final EdgeAIService _edgeAIService = sl<EdgeAIService>();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  final List<Map<String, dynamic>> _messages = [];
  bool _isSending = false;
  bool _isLoadingHistory = false;

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

  void _reportUiError(
    Object error,
    StackTrace stackTrace,
    String context,
  ) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stackTrace,
        library: 'heartlake',
        context: ErrorDescription(context),
      ),
    );
  }

  /// 初始化会话：并行加载历史消息和情绪脉搏，历史加载失败时降级只加载脉搏
  Future<void> _initSession() async {
    await Future.wait([
      _loadHistory(),
      _loadEmotionPulse(),
    ]);
    if (_messages.isEmpty && mounted) {
      _addWelcome();
    }
  }

  /// 加载历史消息
  Future<bool> _loadHistory() async {
    if (mounted) setState(() => _isLoadingHistory = true);
    try {
      final result = await _service.getMessages();
      if (!mounted) return false;
      if (result['success'] != true) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(result['message']?.toString() ?? '历史消息加载失败'),
          ),
        );
        return false;
      }

      final history = (result['messages'] as List? ?? const [])
          .whereType<Map>()
          .map(
            (msg) => <String, dynamic>{
              'content': msg['content'] ?? '',
              'is_mine': msg['role'] == 'user',
              'mood': msg['mood'],
              'created_at': msg['created_at'],
            },
          )
          .toList();

      setState(() {
        _messages
          ..clear()
          ..addAll(history);
      });
      if (history.isNotEmpty) {
        _scrollToBottom();
      }
      return true;
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'LakeGodChatScreen._loadHistory');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('历史消息加载失败，请稍后重试')),
        );
      }
      return false;
    } finally {
      if (mounted) setState(() => _isLoadingHistory = false);
    }
  }

  /// 加载情绪脉搏（公开端点）
  Future<bool> _loadEmotionPulse() async {
    if (mounted) setState(() => _isPulseLoading = true);
    try {
      final result = await _edgeAIService.getEmotionPulse();
      if (!mounted) return false;

      if (result['success'] == true && result['data'] is Map) {
        setState(() {
          _emotionPulse = Map<String, dynamic>.from(result['data'] as Map);
        });
        return true;
      }
      return false;
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'LakeGodChatScreen._loadEmotionPulse');
      return false;
    } finally {
      if (mounted) setState(() => _isPulseLoading = false);
    }
  }

  /// 下拉刷新 - 重新加载历史消息和情绪脉搏
  Future<void> _onRefresh() async {
    await Future.wait([
      _loadHistory(),
      _loadEmotionPulse(),
    ]);
    if (_messages.isEmpty) {
      _addWelcome();
    }
  }

  /// 添加湖神欢迎语到消息列表
  void _addWelcome() {
    setState(() {
      _messages.add({
        'content': '你好呀，我是湖神。有什么心事想和我聊聊吗？',
        'is_mine': false,
      });
    });
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

  /// 发送消息：直接发送给湖神（后端内部已有内容审核）-> 刷新脉搏
  Future<void> _sendMessage() async {
    final content = _controller.text.trim();
    if (content.isEmpty || _isSending) return;
    final optimisticMessage = <String, dynamic>{
      'content': content,
      'is_mine': true,
    };

    _controller.clear();
    FocusScope.of(context).unfocus();

    setState(() => _isSending = true);

    try {
      final messenger = ScaffoldMessenger.of(context);
      // 1. 添加用户消息到列表
      setState(() {
        _messages.add(optimisticMessage);
      });
      _scrollToBottom();

      // 2. 发送消息给湖神（后端 lakeGodChat 内部会做内容审核+情感分析+AI回复）
      final result = await _service.sendMessage(content);

      if (mounted) {
        setState(() {
          _isSending = false;
          if (result['success'] == true && result['data'] != null) {
            final data = result['data'] as Map<String, dynamic>;
            final reply = data['reply'] ??
                data['response'] ??
                data['content'] ??
                '我在倾听...';
            final mood = data['mood']?.toString();
            if (_messages.isNotEmpty && mood != null) {
              _messages[_messages.length - 1]['mood'] = mood;
            }
            _messages.add({
              'content': reply,
              'is_mine': false,
              'mood': mood,
            });
          } else {
            _messages.remove(optimisticMessage);
            _controller.text = content;
            messenger.showSnackBar(
              SnackBar(
                content: Text(result['message'] ?? '消息发送失败，请稍后再试'),
              ),
            );
          }
        });
        if (result['success'] == true) {
          _scrollToBottom();
        }
      }

      // 3. 异步刷新情绪脉搏
      _loadEmotionPulse();
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'LakeGodChatScreen._sendMessage');
      if (mounted) {
        final messenger = ScaffoldMessenger.of(context);
        setState(() {
          _isSending = false;
          _messages.remove(optimisticMessage);
        });
        _controller.text = content;
        messenger.showSnackBar(
          const SnackBar(
            content: Text('网络不太好，稍后再试试吧'),
          ),
        );
      }
    }
  }

  /// 情绪中文映射
  String _moodLabel(String? mood) {
    if (mood == null) return '';
    final normalized = MoodColors.fromString(mood);
    return MoodColors.getConfig(normalized).name;
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

  /// 根据情绪趋势斜率判断趋势方向：上升 / 下降 / 平稳
  String _trendFromSlope(dynamic slopeRaw) {
    final slope = slopeRaw is num ? slopeRaw.toDouble() : 0.0;
    if (slope > 0.03) return 'rising';
    if (slope < -0.03) return 'falling';
    return 'stable';
  }

  double _normalizedPulseScore() {
    if (_emotionPulse == null) return 0.5;
    final explicit = _emotionPulse!['normalized_score'];
    if (explicit is num) {
      return explicit.toDouble().clamp(0.0, 1.0);
    }
    final raw = ((_emotionPulse!['avg_score'] ?? 0.0) as num).toDouble();
    return ((raw.clamp(-1.0, 1.0) + 1.0) / 2.0).clamp(0.0, 1.0);
  }

  /// 情绪对应颜色
  Color _moodColor(String? mood) {
    if (mood == null) return AppTheme.textTertiary;
    final normalized = MoodColors.fromString(mood);
    return MoodColors.getConfig(normalized).primary;
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
    final isDark = Theme.of(context).brightness == Brightness.dark;
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
                        '湖神情绪脉搏',
                        style: TextStyle(
                          color: isDark
                              ? Colors.white.withValues(alpha: 0.9)
                              : const Color(0xFF1B2838).withValues(alpha: 0.95),
                          fontSize: 12,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                      const SizedBox(height: 2),
                      Text(
                        _emotionPulse != null
                            ? '情绪均值 ${_normalizedPulseScore().toStringAsFixed(2)}'
                                '  ·  趋势${_trendLabel(_trendFromSlope(_emotionPulse!['trend_slope']))}'
                                '  ·  ${_emotionPulse!['sample_count'] ?? 0}条样本'
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
    final normalized = _normalizedPulseScore();
    if (normalized >= 0.65) return const Color(0xFF66BB6A); // 积极
    if (normalized >= 0.4) return const Color(0xFF26C6DA); // 平和
    return const Color(0xFFFF7043); // 低落
  }

  /// 构建单条消息气泡，用户消息靠右、湖神消息靠左，用户消息下方附带情绪标签
  Widget _buildMessageBubble(Map<String, dynamic> message) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
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
              padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
              constraints: BoxConstraints(
                maxWidth: MediaQuery.of(context).size.width * 0.7,
              ),
              decoration: BoxDecoration(
                color: isMe
                    ? AppTheme.primaryColor
                    : isDark
                        ? const Color(0xFF1B2838).withValues(alpha: 0.95)
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
                  color: isMe
                      ? Colors.white
                      : (isDark
                          ? const Color(0xFFE8EAED)
                          : AppTheme.textPrimary),
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

  /// 构建底部消息输入栏，包含文本输入框和发送按钮
  Widget _buildInputBar() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Container(
      padding: EdgeInsets.only(
        left: 16,
        right: 8,
        top: 8,
        bottom: MediaQuery.of(context).padding.bottom + 8,
      ),
      decoration: BoxDecoration(
        color: isDark ? const Color(0xFF1B2838) : Colors.white,
        boxShadow: [
          BoxShadow(
            color: isDark
                ? Colors.transparent
                : Colors.black.withValues(alpha: 0.05),
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
