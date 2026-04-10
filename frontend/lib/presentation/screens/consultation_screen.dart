// 心理咨询预约演示页
//
// 该页面仅用于前端展示与联调，不连接真实预约或咨询链路。
import 'package:flutter/material.dart';

import '../../utils/app_theme.dart';
import '../widgets/atmospheric_background.dart';

const List<String> _demoSlots = <String>[
  '09:30',
  '11:00',
  '14:30',
  '16:00',
  '19:30',
];

const List<String> _autoReplies = <String>[
  '我已经收到你的预约意向了。正式接入前，这里先用演示回复帮你确认页面状态。',
  '你可以继续补充近况、作息或触发情绪的场景，这些内容目前只会停留在本地预览。',
  '如果后续需要恢复真实链路，这个版面可以直接替换成正式的预约与咨询流程。',
];

const List<_DemoCounselor> _demoCounselors = <_DemoCounselor>[
  _DemoCounselor(
    name: '林雾',
    title: '情绪减压顾问',
    focus: '适合最近疲惫、失眠、节奏被打乱的人',
    shift: '上午与夜间',
    accent: Color(0xFF73B7FF),
    icon: Icons.air,
  ),
  _DemoCounselor(
    name: '安屿',
    title: '关系沟通陪伴师',
    focus: '适合关系拉扯、表达困难、反复内耗的人',
    shift: '下午与晚间',
    accent: Color(0xFF5FD3BC),
    icon: Icons.waves,
  ),
  _DemoCounselor(
    name: '知夏',
    title: '学业与压力支持',
    focus: '适合学业压力、工作焦虑、目标失衡的人',
    shift: '工作日晚间',
    accent: Color(0xFFFFB26B),
    icon: Icons.wb_sunny_outlined,
  ),
];

class ConsultationScreen extends StatefulWidget {
  const ConsultationScreen({super.key});

  @override
  State<ConsultationScreen> createState() => _ConsultationScreenState();
}

class _ConsultationScreenState extends State<ConsultationScreen>
    with SingleTickerProviderStateMixin {
  late final TabController _tabController;
  late final List<_DemoSession> _sessions;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    _sessions = <_DemoSession>[
      _DemoSession(
        id: 'demo-upcoming',
        counselor: _demoCounselors[0],
        scheduledAt: _demoDayAt(2, '19:30'),
        statusLabel: '待回访',
        preview: '这周情绪起伏比较大，想先留一个夜间时段做预沟通。',
        sourceLabel: '演示预约',
      ),
      _DemoSession(
        id: 'demo-checkin',
        counselor: _demoCounselors[1],
        scheduledAt: _demoDayAt(1, '14:30'),
        statusLabel: '已建档',
        preview: '希望练习更稳定地表达感受，不再总是把话咽回去。',
        sourceLabel: '演示会话',
      ),
    ];
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  void _savePreviewSession(_DemoSession session) {
    setState(() {
      _sessions.removeWhere((item) => item.id == session.id);
      _sessions.insert(0, session);
    });
    _tabController.animateTo(1);
    final messenger = ScaffoldMessenger.maybeOf(context);
    messenger
      ?..hideCurrentSnackBar()
      ..showSnackBar(
        const SnackBar(content: Text('演示预约已保存到本地预览，不会提交到后台')),
      );
  }

  void _openSession(_DemoSession session) {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => _DemoConsultationChatScreen(session: session),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text(
          '心理咨询',
          style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
        ),
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
          tabs: const <Widget>[
            Tab(text: '预约演示'),
            Tab(text: '会话预览'),
          ],
        ),
      ),
      body: Stack(
        children: <Widget>[
          const Positioned.fill(
            child: AtmosphericBackground(
              enableParticles: true,
              particleCount: 14,
              child: SizedBox.expand(),
            ),
          ),
          TabBarView(
            controller: _tabController,
            children: <Widget>[
              _DemoBookingTab(onCreateSession: _savePreviewSession),
              _DemoSessionTab(
                sessions: _sessions,
                onOpenSession: _openSession,
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _DemoBookingTab extends StatefulWidget {
  const _DemoBookingTab({
    required this.onCreateSession,
  });

  final ValueChanged<_DemoSession> onCreateSession;

  @override
  State<_DemoBookingTab> createState() => _DemoBookingTabState();
}

class _DemoBookingTabState extends State<_DemoBookingTab> {
  final TextEditingController _noteController = TextEditingController();
  int _selectedCounselorIndex = 0;
  int _selectedDayOffset = 1;
  String _selectedSlot = _demoSlots[2];

  @override
  void dispose() {
    _noteController.dispose();
    super.dispose();
  }

  void _submitPreview() {
    final counselor = _demoCounselors[_selectedCounselorIndex];
    final note = _noteController.text.trim();
    final session = _DemoSession(
      id: 'demo-${DateTime.now().millisecondsSinceEpoch}',
      counselor: counselor,
      scheduledAt: _demoDayAt(_selectedDayOffset, _selectedSlot),
      statusLabel: '待确认',
      preview: note.isEmpty ? '想先预约一个时段，聊聊${counselor.focus}。' : note,
      sourceLabel: '本地草稿',
    );
    widget.onCreateSession(session);
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final topPadding = MediaQuery.of(context).padding.top + kToolbarHeight + 60;
    final selectedCounselor = _demoCounselors[_selectedCounselorIndex];

    return ListView(
      padding: EdgeInsets.only(
        top: topPadding,
        left: 16,
        right: 16,
        bottom: 24,
      ),
      children: <Widget>[
        Container(
          padding: const EdgeInsets.all(18),
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(24),
            gradient: const LinearGradient(
              colors: <Color>[
                Color(0xFF25557D),
                Color(0xFF184062),
                Color(0xFF102D46),
              ],
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
            ),
            boxShadow: <BoxShadow>[
              BoxShadow(
                color: Colors.black.withValues(alpha: 0.12),
                blurRadius: 28,
                offset: const Offset(0, 18),
              ),
            ],
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              Container(
                padding:
                    const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
                decoration: BoxDecoration(
                  color: Colors.white.withValues(alpha: 0.14),
                  borderRadius: BorderRadius.circular(999),
                ),
                child: const Text(
                  '演示模式',
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: 12,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ),
              const SizedBox(height: 14),
              const Text(
                '先把预约体验和页面节奏做顺，真实链路先不接。',
                style: TextStyle(
                  color: Colors.white,
                  fontSize: 22,
                  fontWeight: FontWeight.w700,
                  height: 1.25,
                ),
              ),
              const SizedBox(height: 10),
              Text(
                '这里的咨询师、时段、对话反馈都只在当前界面里预览，不会创建真实订单，也不会向后端发送内容。',
                style: TextStyle(
                  color: Colors.white.withValues(alpha: 0.82),
                  fontSize: 13,
                  height: 1.6,
                ),
              ),
            ],
          ),
        ),
        const SizedBox(height: 14),
        _buildSectionCard(
          isDark: isDark,
          title: '选择陪伴方向',
          subtitle: '先选一个更贴近当前状态的咨询风格',
          child: SizedBox(
            height: 176,
            child: ListView.separated(
              scrollDirection: Axis.horizontal,
              itemCount: _demoCounselors.length,
              separatorBuilder: (_, __) => const SizedBox(width: 12),
              itemBuilder: (BuildContext context, int index) {
                final counselor = _demoCounselors[index];
                final isSelected = index == _selectedCounselorIndex;
                return GestureDetector(
                  onTap: () {
                    setState(() {
                      _selectedCounselorIndex = index;
                    });
                  },
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 220),
                    width: 220,
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: isSelected
                          ? counselor.accent.withValues(alpha: 0.18)
                          : (isDark
                              ? const Color(0xFF16213E).withValues(alpha: 0.86)
                              : Colors.white.withValues(alpha: 0.9)),
                      borderRadius: BorderRadius.circular(22),
                      border: Border.all(
                        color: isSelected
                            ? counselor.accent.withValues(alpha: 0.7)
                            : Colors.white.withValues(alpha: 0.08),
                      ),
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: <Widget>[
                        CircleAvatar(
                          radius: 22,
                          backgroundColor:
                              counselor.accent.withValues(alpha: 0.2),
                          child: Icon(counselor.icon, color: counselor.accent),
                        ),
                        const SizedBox(height: 14),
                        Text(
                          counselor.name,
                          style: TextStyle(
                            fontSize: 17,
                            fontWeight: FontWeight.w700,
                            color: isDark ? Colors.white : AppTheme.textPrimary,
                          ),
                        ),
                        const SizedBox(height: 4),
                        Text(
                          counselor.title,
                          style: TextStyle(
                            fontSize: 12,
                            color: counselor.accent,
                            fontWeight: FontWeight.w600,
                          ),
                        ),
                        const SizedBox(height: 10),
                        Text(
                          counselor.focus,
                          style: TextStyle(
                            fontSize: 12,
                            height: 1.55,
                            color: isDark
                                ? Colors.white70
                                : AppTheme.textSecondary,
                          ),
                        ),
                        const Spacer(),
                        Text(
                          '可预览时段 · ${counselor.shift}',
                          style: TextStyle(
                            fontSize: 11,
                            color:
                                isDark ? Colors.white54 : AppTheme.textTertiary,
                          ),
                        ),
                      ],
                    ),
                  ),
                );
              },
            ),
          ),
        ),
        const SizedBox(height: 12),
        _buildSectionCard(
          isDark: isDark,
          title: '预约时间草稿',
          subtitle: '做一个完整交互，但只保存在当前设备内存里',
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: List<Widget>.generate(5, (int index) {
                  final offset = index + 1;
                  final isSelected = offset == _selectedDayOffset;
                  return ChoiceChip(
                    label: Text(_formatDayChip(offset)),
                    selected: isSelected,
                    labelStyle: TextStyle(
                      color: isSelected
                          ? Colors.white
                          : (isDark ? Colors.white70 : AppTheme.textPrimary),
                      fontWeight:
                          isSelected ? FontWeight.w600 : FontWeight.w500,
                    ),
                    selectedColor:
                        selectedCounselor.accent.withValues(alpha: 0.88),
                    backgroundColor: isDark
                        ? Colors.white.withValues(alpha: 0.08)
                        : const Color(0xFFF1F5F9),
                    onSelected: (_) {
                      setState(() {
                        _selectedDayOffset = offset;
                      });
                    },
                  );
                }),
              ),
              const SizedBox(height: 14),
              Wrap(
                spacing: 10,
                runSpacing: 10,
                children: _demoSlots.map((String slot) {
                  final isSelected = slot == _selectedSlot;
                  return InkWell(
                    onTap: () {
                      setState(() {
                        _selectedSlot = slot;
                      });
                    },
                    borderRadius: BorderRadius.circular(16),
                    child: AnimatedContainer(
                      duration: const Duration(milliseconds: 180),
                      padding: const EdgeInsets.symmetric(
                        horizontal: 14,
                        vertical: 10,
                      ),
                      decoration: BoxDecoration(
                        borderRadius: BorderRadius.circular(16),
                        color: isSelected
                            ? selectedCounselor.accent.withValues(alpha: 0.14)
                            : Colors.transparent,
                        border: Border.all(
                          color: isSelected
                              ? selectedCounselor.accent
                              : (isDark
                                  ? Colors.white.withValues(alpha: 0.12)
                                  : const Color(0xFFD7E0EA)),
                        ),
                      ),
                      child: Text(
                        slot,
                        style: TextStyle(
                          color: isSelected
                              ? selectedCounselor.accent
                              : (isDark
                                  ? Colors.white70
                                  : AppTheme.textSecondary),
                          fontWeight:
                              isSelected ? FontWeight.w700 : FontWeight.w500,
                        ),
                      ),
                    ),
                  );
                }).toList(),
              ),
            ],
          ),
        ),
        const SizedBox(height: 12),
        _buildSectionCard(
          isDark: isDark,
          title: '想先聊什么',
          subtitle: '留一句你此刻最想被认真接住的话',
          child: TextField(
            controller: _noteController,
            maxLines: 5,
            decoration: InputDecoration(
              hintText: '例如：最近总觉得自己很绷着，白天正常，晚上情绪会一起涌上来。',
              hintStyle: TextStyle(
                color: isDark ? Colors.white38 : AppTheme.textTertiary,
              ),
              filled: true,
              fillColor: isDark
                  ? Colors.white.withValues(alpha: 0.06)
                  : const Color(0xFFF7FAFD),
              border: OutlineInputBorder(
                borderRadius: BorderRadius.circular(18),
                borderSide: BorderSide.none,
              ),
              contentPadding: const EdgeInsets.all(16),
            ),
            style: TextStyle(
              color: isDark ? Colors.white : AppTheme.textPrimary,
              height: 1.55,
            ),
          ),
        ),
        const SizedBox(height: 12),
        _buildSectionCard(
          isDark: isDark,
          title: '本次预览摘要',
          subtitle: '给你一个最终确认感',
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              _buildSummaryRow(
                '咨询方向',
                '${selectedCounselor.name} · ${selectedCounselor.title}',
              ),
              const SizedBox(height: 10),
              _buildSummaryRow(
                '演示时段',
                _formatDateTime(_demoDayAt(_selectedDayOffset, _selectedSlot)),
              ),
              const SizedBox(height: 10),
              _buildSummaryRow('状态', '只做前端联调，不创建真实预约'),
              const SizedBox(height: 14),
              SizedBox(
                width: double.infinity,
                child: ElevatedButton(
                  onPressed: _submitPreview,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: selectedCounselor.accent,
                    foregroundColor: Colors.black87,
                    padding: const EdgeInsets.symmetric(vertical: 14),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(16),
                    ),
                  ),
                  child: const Text(
                    '保存预约演示',
                    style: TextStyle(fontWeight: FontWeight.w700),
                  ),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }
}

class _DemoSessionTab extends StatelessWidget {
  const _DemoSessionTab({
    required this.sessions,
    required this.onOpenSession,
  });

  final List<_DemoSession> sessions;
  final ValueChanged<_DemoSession> onOpenSession;

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final topPadding = MediaQuery.of(context).padding.top + kToolbarHeight + 60;

    return ListView(
      padding: EdgeInsets.only(
        top: topPadding,
        left: 16,
        right: 16,
        bottom: 24,
      ),
      children: <Widget>[
        Container(
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: isDark
                ? const Color(0xFF16213E).withValues(alpha: 0.88)
                : Colors.white.withValues(alpha: 0.9),
            borderRadius: BorderRadius.circular(22),
          ),
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              Container(
                width: 42,
                height: 42,
                decoration: BoxDecoration(
                  color: AppTheme.primaryColor.withValues(alpha: 0.12),
                  borderRadius: BorderRadius.circular(14),
                ),
                child: const Icon(
                  Icons.visibility_outlined,
                  color: AppTheme.primaryColor,
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Text(
                  '这里展示的是会话预览卡片。点进去可以测试聊天界面、输入状态和自动回复，但所有内容都不会离开当前页面。',
                  style: TextStyle(
                    color: isDark ? Colors.white70 : AppTheme.textSecondary,
                    height: 1.6,
                  ),
                ),
              ),
            ],
          ),
        ),
        const SizedBox(height: 12),
        ...sessions.map((session) {
          return Padding(
            padding: const EdgeInsets.only(bottom: 12),
            child: InkWell(
              onTap: () => onOpenSession(session),
              borderRadius: BorderRadius.circular(22),
              child: Container(
                padding: const EdgeInsets.all(16),
                decoration: BoxDecoration(
                  color: isDark
                      ? const Color(0xFF16213E).withValues(alpha: 0.9)
                      : Colors.white.withValues(alpha: 0.92),
                  borderRadius: BorderRadius.circular(22),
                  border: Border.all(
                    color: session.counselor.accent.withValues(alpha: 0.22),
                  ),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: <Widget>[
                    Row(
                      children: <Widget>[
                        CircleAvatar(
                          radius: 22,
                          backgroundColor:
                              session.counselor.accent.withValues(alpha: 0.2),
                          child: Icon(
                            session.counselor.icon,
                            color: session.counselor.accent,
                          ),
                        ),
                        const SizedBox(width: 12),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: <Widget>[
                              Text(
                                session.counselor.name,
                                style: TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.w700,
                                  color: isDark
                                      ? Colors.white
                                      : AppTheme.textPrimary,
                                ),
                              ),
                              const SizedBox(height: 4),
                              Text(
                                session.counselor.title,
                                style: TextStyle(
                                  fontSize: 12,
                                  color: session.counselor.accent,
                                  fontWeight: FontWeight.w600,
                                ),
                              ),
                            ],
                          ),
                        ),
                        Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 10,
                            vertical: 6,
                          ),
                          decoration: BoxDecoration(
                            color: session.counselor.accent
                                .withValues(alpha: 0.16),
                            borderRadius: BorderRadius.circular(999),
                          ),
                          child: Text(
                            session.statusLabel,
                            style: TextStyle(
                              fontSize: 11,
                              fontWeight: FontWeight.w700,
                              color: session.counselor.accent,
                            ),
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 14),
                    Text(
                      session.preview,
                      style: TextStyle(
                        fontSize: 13,
                        height: 1.6,
                        color: isDark ? Colors.white70 : AppTheme.textSecondary,
                      ),
                    ),
                    const SizedBox(height: 14),
                    Row(
                      children: <Widget>[
                        Icon(
                          Icons.schedule_outlined,
                          size: 15,
                          color:
                              isDark ? Colors.white54 : AppTheme.textTertiary,
                        ),
                        const SizedBox(width: 6),
                        Text(
                          _formatDateTime(session.scheduledAt),
                          style: TextStyle(
                            fontSize: 12,
                            color:
                                isDark ? Colors.white54 : AppTheme.textTertiary,
                          ),
                        ),
                        const Spacer(),
                        Text(
                          session.sourceLabel,
                          style: TextStyle(
                            fontSize: 11,
                            color:
                                isDark ? Colors.white38 : AppTheme.textTertiary,
                          ),
                        ),
                        const SizedBox(width: 6),
                        const Icon(
                          Icons.chevron_right,
                          color: AppTheme.textTertiary,
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          );
        }),
      ],
    );
  }
}

class _DemoConsultationChatScreen extends StatefulWidget {
  const _DemoConsultationChatScreen({
    required this.session,
  });

  final _DemoSession session;

  @override
  State<_DemoConsultationChatScreen> createState() =>
      _DemoConsultationChatScreenState();
}

class _DemoConsultationChatScreenState
    extends State<_DemoConsultationChatScreen> {
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  late final List<_DemoMessage> _messages;
  bool _isReplying = false;

  @override
  void initState() {
    super.initState();
    _messages = <_DemoMessage>[
      _DemoMessage(
        text: '这里是演示会话，你现在看到的是 ${widget.session.counselor.name} 的本地预览页，不连接后台。',
        isMine: false,
        timeLabel: _formatClock(widget.session.scheduledAt),
      ),
      _DemoMessage(
        text: widget.session.preview,
        isMine: true,
        timeLabel: _formatClock(widget.session.scheduledAt),
      ),
      _DemoMessage(
        text: '我先帮你保留这个时段。正式接入之前，你可以继续拿这里调输入和阅读体验。',
        isMine: false,
        timeLabel: _formatClock(widget.session.scheduledAt),
      ),
    ];
  }

  @override
  void dispose() {
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  Future<void> _sendMessage() async {
    final text = _controller.text.trim();
    if (text.isEmpty || _isReplying) return;

    setState(() {
      _messages.add(_DemoMessage(
        text: text,
        isMine: true,
        timeLabel: _formatClock(DateTime.now()),
      ));
      _controller.clear();
      _isReplying = true;
    });
    _scrollToBottom();

    await Future<void>.delayed(const Duration(milliseconds: 520));
    if (!mounted) return;

    setState(() {
      _messages.add(
        _DemoMessage(
          text: _autoReplies[_messages.length % _autoReplies.length],
          isMine: false,
          timeLabel: _formatClock(DateTime.now()),
        ),
      );
      _isReplying = false;
    });
    _scrollToBottom();
  }

  void _scrollToBottom() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!_scrollController.hasClients) return;
      _scrollController.animateTo(
        _scrollController.position.maxScrollExtent,
        duration: const Duration(milliseconds: 240),
        curve: Curves.easeOut,
      );
    });
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: Column(
          children: <Widget>[
            Text(
              widget.session.counselor.name,
              style: const TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.w700,
                fontSize: 17,
              ),
            ),
            Text(
              '演示对话 · 不连接后台',
              style: TextStyle(
                color: Colors.white.withValues(alpha: 0.78),
                fontSize: 11,
              ),
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
        children: <Widget>[
          const Positioned.fill(
            child: AtmosphericBackground(
              enableParticles: true,
              particleCount: 10,
              child: SizedBox.expand(),
            ),
          ),
          Column(
            children: <Widget>[
              Container(
                width: double.infinity,
                padding: EdgeInsets.only(
                  top: MediaQuery.of(context).padding.top + kToolbarHeight + 8,
                  bottom: 10,
                ),
                alignment: Alignment.center,
                child: Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 14, vertical: 6),
                  decoration: BoxDecoration(
                    color:
                        widget.session.counselor.accent.withValues(alpha: 0.16),
                    borderRadius: BorderRadius.circular(999),
                  ),
                  child: Text(
                    '消息仅用于本地预览，便于联调布局和交互',
                    style: TextStyle(
                      fontSize: 11,
                      color: widget.session.counselor.accent,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                ),
              ),
              Expanded(
                child: ListView.builder(
                  controller: _scrollController,
                  padding: const EdgeInsets.symmetric(
                    horizontal: 16,
                    vertical: 8,
                  ),
                  itemCount: _messages.length,
                  itemBuilder: (BuildContext context, int index) {
                    final message = _messages[index];
                    return _ChatBubble(
                      message: message,
                      counselor: widget.session.counselor,
                    );
                  },
                ),
              ),
              Container(
                padding: const EdgeInsets.fromLTRB(12, 10, 12, 14),
                decoration: BoxDecoration(
                  color: isDark
                      ? const Color(0xFF16213E).withValues(alpha: 0.94)
                      : Colors.white.withValues(alpha: 0.94),
                  boxShadow: <BoxShadow>[
                    BoxShadow(
                      color: Colors.black.withValues(alpha: 0.06),
                      blurRadius: 10,
                      offset: const Offset(0, -3),
                    ),
                  ],
                ),
                child: Row(
                  children: <Widget>[
                    Expanded(
                      child: TextField(
                        controller: _controller,
                        minLines: 1,
                        maxLines: 4,
                        textInputAction: TextInputAction.send,
                        onSubmitted: (_) => _sendMessage(),
                        decoration: InputDecoration(
                          hintText: '继续输入演示消息...',
                          filled: true,
                          fillColor: isDark
                              ? Colors.white.withValues(alpha: 0.06)
                              : const Color(0xFFF5F8FC),
                          border: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(18),
                            borderSide: BorderSide.none,
                          ),
                          contentPadding: const EdgeInsets.symmetric(
                            horizontal: 14,
                            vertical: 12,
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(width: 10),
                    GestureDetector(
                      onTap: _sendMessage,
                      child: Container(
                        width: 48,
                        height: 48,
                        decoration: BoxDecoration(
                          color: widget.session.counselor.accent,
                          borderRadius: BorderRadius.circular(16),
                        ),
                        child: _isReplying
                            ? const Padding(
                                padding: EdgeInsets.all(14),
                                child: CircularProgressIndicator(
                                  strokeWidth: 2,
                                  color: Colors.white,
                                ),
                              )
                            : const Icon(Icons.send_rounded,
                                color: Colors.white),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _ChatBubble extends StatelessWidget {
  const _ChatBubble({
    required this.message,
    required this.counselor,
  });

  final _DemoMessage message;
  final _DemoCounselor counselor;

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final isMine = message.isMine;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 5),
      child: Row(
        mainAxisAlignment:
            isMine ? MainAxisAlignment.end : MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: <Widget>[
          if (!isMine) ...<Widget>[
            CircleAvatar(
              radius: 16,
              backgroundColor: counselor.accent.withValues(alpha: 0.22),
              child: Icon(counselor.icon, size: 16, color: counselor.accent),
            ),
            const SizedBox(width: 8),
          ],
          Flexible(
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 11),
              decoration: BoxDecoration(
                color: isMine
                    ? counselor.accent
                    : (isDark
                        ? const Color(0xFF16213E).withValues(alpha: 0.9)
                        : Colors.white.withValues(alpha: 0.92)),
                borderRadius: BorderRadius.only(
                  topLeft: const Radius.circular(18),
                  topRight: const Radius.circular(18),
                  bottomLeft: Radius.circular(isMine ? 18 : 6),
                  bottomRight: Radius.circular(isMine ? 6 : 18),
                ),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: <Widget>[
                  Text(
                    message.text,
                    style: TextStyle(
                      fontSize: 14,
                      height: 1.5,
                      color: isMine
                          ? Colors.white
                          : (isDark ? Colors.white : AppTheme.textPrimary),
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    message.timeLabel,
                    style: TextStyle(
                      fontSize: 10,
                      color: isMine
                          ? Colors.white.withValues(alpha: 0.72)
                          : AppTheme.textTertiary,
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _DemoCounselor {
  const _DemoCounselor({
    required this.name,
    required this.title,
    required this.focus,
    required this.shift,
    required this.accent,
    required this.icon,
  });

  final String name;
  final String title;
  final String focus;
  final String shift;
  final Color accent;
  final IconData icon;
}

class _DemoSession {
  const _DemoSession({
    required this.id,
    required this.counselor,
    required this.scheduledAt,
    required this.statusLabel,
    required this.preview,
    required this.sourceLabel,
  });

  final String id;
  final _DemoCounselor counselor;
  final DateTime scheduledAt;
  final String statusLabel;
  final String preview;
  final String sourceLabel;
}

class _DemoMessage {
  const _DemoMessage({
    required this.text,
    required this.isMine,
    required this.timeLabel,
  });

  final String text;
  final bool isMine;
  final String timeLabel;
}

Widget _buildSectionCard({
  required bool isDark,
  required String title,
  required String subtitle,
  required Widget child,
}) {
  return Container(
    padding: const EdgeInsets.all(16),
    decoration: BoxDecoration(
      color: isDark
          ? const Color(0xFF16213E).withValues(alpha: 0.88)
          : Colors.white.withValues(alpha: 0.92),
      borderRadius: BorderRadius.circular(22),
    ),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: <Widget>[
        Text(
          title,
          style: TextStyle(
            fontSize: 17,
            fontWeight: FontWeight.w700,
            color: isDark ? Colors.white : AppTheme.textPrimary,
          ),
        ),
        const SizedBox(height: 4),
        Text(
          subtitle,
          style: TextStyle(
            fontSize: 12,
            height: 1.5,
            color: isDark ? Colors.white60 : AppTheme.textSecondary,
          ),
        ),
        const SizedBox(height: 14),
        child,
      ],
    ),
  );
}

Widget _buildSummaryRow(String label, String value) {
  return Row(
    crossAxisAlignment: CrossAxisAlignment.start,
    children: <Widget>[
      SizedBox(
        width: 74,
        child: Text(
          label,
          style: const TextStyle(
            fontSize: 12,
            color: AppTheme.textTertiary,
          ),
        ),
      ),
      const SizedBox(width: 8),
      Expanded(
        child: Text(
          value,
          style: const TextStyle(
            fontSize: 13,
            fontWeight: FontWeight.w600,
            height: 1.45,
          ),
        ),
      ),
    ],
  );
}

DateTime _demoDayAt(int dayOffset, String slot) {
  final now = DateTime.now();
  final parts = slot.split(':');
  final hour = int.parse(parts[0]);
  final minute = int.parse(parts[1]);
  return DateTime(
    now.year,
    now.month,
    now.day + dayOffset,
    hour,
    minute,
  );
}

String _formatDayChip(int dayOffset) {
  final date = _demoDayAt(dayOffset, '09:00');
  final weekdays = <String>['一', '二', '三', '四', '五', '六', '日'];
  final weekdayLabel = weekdays[date.weekday - 1];
  final month = date.month.toString().padLeft(2, '0');
  final day = date.day.toString().padLeft(2, '0');
  return '$month/$day 周$weekdayLabel';
}

String _formatDateTime(DateTime dateTime) {
  final month = dateTime.month.toString().padLeft(2, '0');
  final day = dateTime.day.toString().padLeft(2, '0');
  return '${dateTime.year}-$month-$day ${_formatClock(dateTime)}';
}

String _formatClock(DateTime dateTime) {
  final hour = dateTime.hour.toString().padLeft(2, '0');
  final minute = dateTime.minute.toString().padLeft(2, '0');
  return '$hour:$minute';
}
