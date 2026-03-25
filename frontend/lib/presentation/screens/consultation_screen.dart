// 心理咨询预约和会话页面
//
// 支持E2E加密的咨询会话，包含预约咨询和实时消息收发。
import 'package:flutter/material.dart';
import '../../data/datasources/consultation_service.dart';
import '../../data/datasources/vip_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../widgets/atmospheric_background.dart';

/// 心理咨询主页面
///
/// 双 Tab 结构：
/// - 预约咨询：走后端真实预约链路，提交咨询预约时间
/// - 我的会话：历史咨询会话列表，点击继续对话
///
/// 聊天界面内置 E2E 加密保护，所有消息经 X25519 + AES-GCM 加密后传输，
/// 仅用户和咨询师双方可见。
class ConsultationScreen extends StatefulWidget {
  const ConsultationScreen({super.key});

  @override
  State<ConsultationScreen> createState() => _ConsultationScreenState();
}

/// 心理咨询主页面的状态管理，维护双 Tab 的 TabController
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
              const _ConsultationBookingTab(),
              _SessionListTab(onOpenSession: _goToChatWithSession),
            ],
          ),
        ],
      ),
    );
  }

  /// 跳转到咨询聊天页面（恢复已有会话）
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

/// 预约咨询 Tab，走真实预约接口，不再展示前端伪造的咨询师列表
class _ConsultationBookingTab extends StatefulWidget {
  const _ConsultationBookingTab();

  @override
  State<_ConsultationBookingTab> createState() =>
      _ConsultationBookingTabState();
}

class _ConsultationBookingTabState extends State<_ConsultationBookingTab> {
  final VIPService _vipService = sl<VIPService>();

  bool _isLoadingStatus = true;
  bool _isBooking = false;
  bool _hasFreeQuota = false;
  bool _isVip = false;
  int _vipDaysLeft = 0;
  DateTime? _selectedTime;
  String? _lastAppointmentId;

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

  void _showMessage(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger == null) return;
    messenger
      ..hideCurrentSnackBar()
      ..showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  void initState() {
    super.initState();
    _loadStatus();
  }

  Future<void> _loadStatus() async {
    if (mounted) {
      setState(() => _isLoadingStatus = true);
    }
    try {
      final results = await Future.wait<dynamic>([
        _vipService.getVIPStatus(),
        _vipService.getCounselingQuotaStatus(),
      ]);
      final quota = results[1] as Map<String, dynamic>;
      final status = results[0] as Map<String, dynamic>;
      final payload = status['data'] is Map<String, dynamic>
          ? status['data'] as Map<String, dynamic>
          : const <String, dynamic>{};
      final failures = <String>[
        if (status['success'] != true)
          status['message']?.toString() ?? '灯火状态加载失败',
        if (quota['success'] != true)
          quota['message']?.toString() ?? '咨询额度加载失败',
      ];

      if (!mounted) return;
      setState(() {
        if (status['success'] == true) {
          _isVip = payload['is_vip'] == true;
          _vipDaysLeft = (payload['days_left'] as num?)?.toInt() ?? 0;
        }
        if (quota['success'] == true) {
          _hasFreeQuota = quota['has_quota'] == true;
        }
        _isLoadingStatus = false;
      });
      if (failures.isNotEmpty) {
        _showMessage(failures.join('；'));
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'ConsultationBookingTab._loadStatus');
      if (!mounted) return;
      setState(() {
        _isLoadingStatus = false;
      });
      _showMessage('预约状态加载失败，请稍后重试');
    }
  }

  Future<void> _pickAppointmentTime() async {
    final now = DateTime.now();
    final initialDate = now.add(const Duration(days: 1));
    final selectedDate = await showDatePicker(
      context: context,
      initialDate: initialDate,
      firstDate: now,
      lastDate: now.add(const Duration(days: 90)),
    );
    if (selectedDate == null || !mounted) return;

    final selectedClock = await showTimePicker(
      context: context,
      initialTime: TimeOfDay.fromDateTime(initialDate),
    );
    if (selectedClock == null || !mounted) return;

    final appointmentTime = DateTime(
      selectedDate.year,
      selectedDate.month,
      selectedDate.day,
      selectedClock.hour,
      selectedClock.minute,
    );
    if (appointmentTime.isBefore(now.add(const Duration(minutes: 30)))) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('预约时间需要晚于当前时间 30 分钟')),
      );
      return;
    }

    setState(() {
      _selectedTime = appointmentTime;
    });
  }

  Future<void> _bookAppointment() async {
    final appointmentTime = _selectedTime;
    if (appointmentTime == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('请先选择预约时间')),
      );
      return;
    }

    setState(() => _isBooking = true);
    try {
      final result = await _vipService.bookCounseling(
        appointmentTime: appointmentTime.toIso8601String(),
        isFreeVIP: _hasFreeQuota,
      );
      if (!mounted) return;

      if (result['success'] == true) {
        final payload = result['data'] is Map<String, dynamic>
            ? result['data'] as Map<String, dynamic>
            : const <String, dynamic>{};
        setState(() {
          _lastAppointmentId = payload['appointment_id']?.toString();
        });
        await _loadStatus();
        if (!mounted) return;
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('预约已提交，请留意后续通知')),
        );
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(result['message']?.toString() ?? '预约失败')),
        );
      }
    } catch (error, stackTrace) {
      _reportUiError(
        error,
        stackTrace,
        'ConsultationBookingTab._bookAppointment',
      );
      if (!mounted) return;
      _showMessage('网络异常，请稍后再试');
    } finally {
      if (mounted) {
        setState(() => _isBooking = false);
      }
    }
  }

  String _formatAppointmentTime(DateTime? time) {
    if (time == null) return '未选择';
    final month = time.month.toString().padLeft(2, '0');
    final day = time.day.toString().padLeft(2, '0');
    final hour = time.hour.toString().padLeft(2, '0');
    final minute = time.minute.toString().padLeft(2, '0');
    return '${time.year}-$month-$day $hour:$minute';
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final topPadding = MediaQuery.of(context).padding.top + kToolbarHeight + 60;

    return RefreshIndicator(
      onRefresh: _loadStatus,
      color: AppTheme.primaryColor,
      child: ListView(
        padding: EdgeInsets.only(
          top: topPadding,
          bottom: 20,
          left: 16,
          right: 16,
        ),
        children: [
          Card(
            color: isDark
                ? const Color(0xFF16213E).withValues(alpha: 0.92)
                : Colors.white.withValues(alpha: 0.92),
            shape:
                RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
            child: const Padding(
              padding: EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('真实预约链路',
                      style:
                          TextStyle(fontSize: 18, fontWeight: FontWeight.w700)),
                  SizedBox(height: 8),
                  Text(
                    '这里直接提交后端心理咨询预约，不再使用前端伪造咨询师数据。预约成功后，系统会通过通知和会话列表同步后续安排。',
                    style: TextStyle(height: 1.6),
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 12),
          Card(
            color: isDark
                ? const Color(0xFF16213E).withValues(alpha: 0.92)
                : Colors.white.withValues(alpha: 0.92),
            shape:
                RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
            child: ListTile(
              leading: Icon(
                _hasFreeQuota ? Icons.local_fire_department : Icons.schedule,
                color: _hasFreeQuota ? Colors.orange : AppTheme.primaryColor,
              ),
              title: Text(
                _isLoadingStatus
                    ? '正在加载预约状态...'
                    : (_hasFreeQuota ? '当前可用一次免费咨询' : '当前走普通预约'),
              ),
              subtitle: Text(
                _isLoadingStatus
                    ? '请稍候'
                    : (_isVip ? '灯火剩余 $_vipDaysLeft 天' : '未检测到免费灯火额度'),
              ),
              trailing: IconButton(
                onPressed: _isLoadingStatus ? null : _loadStatus,
                icon: const Icon(Icons.refresh),
              ),
            ),
          ),
          const SizedBox(height: 12),
          Card(
            color: isDark
                ? const Color(0xFF16213E).withValues(alpha: 0.92)
                : Colors.white.withValues(alpha: 0.92),
            shape:
                RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
            child: Column(
              children: [
                ListTile(
                  leading: const Icon(Icons.event_available,
                      color: AppTheme.primaryColor),
                  title: const Text('预约时间'),
                  subtitle: Text(_formatAppointmentTime(_selectedTime)),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: _pickAppointmentTime,
                ),
                const Divider(height: 1),
                Padding(
                  padding: const EdgeInsets.all(16),
                  child: SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: _isBooking || _isLoadingStatus
                          ? null
                          : _bookAppointment,
                      style: ElevatedButton.styleFrom(
                        backgroundColor: AppTheme.primaryColor,
                        foregroundColor: Colors.white,
                        padding: const EdgeInsets.symmetric(vertical: 12),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                      child: _isBooking
                          ? const SizedBox(
                              width: 20,
                              height: 20,
                              child: CircularProgressIndicator(strokeWidth: 2),
                            )
                          : Text(_hasFreeQuota ? '预约免费咨询' : '提交咨询预约'),
                    ),
                  ),
                ),
              ],
            ),
          ),
          if (_lastAppointmentId != null) ...[
            const SizedBox(height: 12),
            Card(
              color: isDark
                  ? const Color(0xFF16213E).withValues(alpha: 0.92)
                  : Colors.white.withValues(alpha: 0.92),
              shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(16)),
              child: ListTile(
                leading: const Icon(Icons.check_circle,
                    color: AppTheme.successColor),
                title: const Text('最近一次预约已提交'),
                subtitle: Text('预约单号：$_lastAppointmentId'),
              ),
            ),
          ],
        ],
      ),
    );
  }
}

/// 我的会话列表 Tab，从后端加载历史咨询记录，支持下拉刷新
class _SessionListTab extends StatefulWidget {
  /// 点击会话后的回调，传递会话ID和咨询师姓名
  final void Function(String sessionId, String counselorName) onOpenSession;

  const _SessionListTab({required this.onOpenSession});

  @override
  State<_SessionListTab> createState() => _SessionListTabState();
}

/// 会话列表 Tab 的状态管理
class _SessionListTabState extends State<_SessionListTab> {
  final ConsultationService _service = sl<ConsultationService>();
  List<Map<String, dynamic>> _sessions = [];
  bool _isLoading = true;
  String? _error;

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

  void _showMessage(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger == null) return;
    messenger
      ..hideCurrentSnackBar()
      ..showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  void initState() {
    super.initState();
    _loadSessions();
  }

  /// 从后端加载咨询会话列表
  Future<void> _loadSessions() async {
    if (mounted) {
      setState(() {
        _isLoading = true;
        _error = null;
      });
    }
    try {
      final result = await _service.getSessions();
      if (!mounted) return;
      if (result['success'] == true && result['data'] != null) {
        final data = result['data'];
        final List<dynamic> list =
            data is List ? data : (data['sessions'] ?? []);
        final nextSessions = list
            .whereType<Map>()
            .map(
              (item) => Map<String, dynamic>.from(item.cast<String, dynamic>()),
            )
            .toList();
        setState(() {
          _sessions = nextSessions;
          _isLoading = false;
          _error = null;
        });
      } else {
        final message = result['message']?.toString() ?? '加载失败，请稍后重试';
        setState(() {
          _isLoading = false;
          _error = _sessions.isEmpty ? message : null;
        });
        _showMessage(message);
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'SessionListTab._loadSessions');
      if (!mounted) return;
      setState(() {
        _isLoading = false;
        _error = _sessions.isEmpty ? '加载失败，请稍后重试' : null;
      });
      _showMessage('加载失败，请稍后重试');
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final topPadding = MediaQuery.of(context).padding.top + kToolbarHeight + 60;

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
                child: const Text('重试', style: TextStyle(color: Colors.white)),
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
              Text('先提交一次咨询预约吧',
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
          final counselorName = s['counselor_name'] as String? ?? '咨询师';
          final lastMessage = s['last_message'] as String? ?? '暂无消息';
          final time = s['updated_at'] as String? ?? '';
          final sessionId = s['session_id'] as String? ?? '';

          return Card(
            margin: const EdgeInsets.only(bottom: 10),
            color: isDark
                ? const Color(0xFF16213E).withValues(alpha: 0.92)
                : Colors.white.withValues(alpha: 0.92),
            shape:
                RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
            child: ListTile(
              contentPadding:
                  const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              leading: CircleAvatar(
                backgroundColor: AppTheme.skyBlue.withValues(alpha: 0.2),
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
                          fontSize: 13,
                          color: isDark
                              ? Colors.white70
                              : AppTheme.textSecondary)),
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
                  const Icon(Icons.chevron_right, color: AppTheme.textTertiary),
                ],
              ),
              onTap: () => widget.onOpenSession(sessionId, counselorName),
            ),
          );
        },
      ),
    );
  }
}

/// E2E 加密聊天界面
///
/// 进入时使用既有会话初始化 E2E 加密通道，
/// 加载历史消息并自动解密。发送消息前强制加密，
/// 未建立安全通道时拒绝发送。
class _ConsultationChatScreen extends StatefulWidget {
  final String sessionId;
  final String counselorName;

  const _ConsultationChatScreen({
    required this.sessionId,
    required this.counselorName,
  });

  @override
  State<_ConsultationChatScreen> createState() =>
      _ConsultationChatScreenState();
}

/// E2E 加密咨询聊天页面的状态管理
///
/// 管理既有会话的 E2E 加密初始化、消息加解密和收发流程。
class _ConsultationChatScreenState extends State<_ConsultationChatScreen> {
  final ConsultationService _service = sl<ConsultationService>();
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  final List<Map<String, dynamic>> _messages = [];

  String? _sessionId;
  bool _isSending = false;
  bool _isLoading = true;
  bool _isE2EReady = false;
  String? _securityStatusMessage;

  /// 将动态类型安全转换为 bool，支持 bool / String / num 类型
  bool _asBool(dynamic value, {bool fallback = false}) {
    if (value is bool) return value;
    if (value is String) {
      final normalized = value.trim().toLowerCase();
      if (normalized == 'true') return true;
      if (normalized == 'false') return false;
    }
    if (value is num) return value != 0;
    return fallback;
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

  void _showMessage(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger == null) return;
    messenger
      ..hideCurrentSnackBar()
      ..showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  void initState() {
    super.initState();
    _sessionId = widget.sessionId;
    _initChat();
  }

  @override
  void dispose() {
    if (_sessionId != null && _sessionId!.isNotEmpty) {
      _service.disposeSession(_sessionId!);
    }
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  /// 初始化聊天：校验会话ID -> 初始化 E2E 加密 -> 加载历史消息
  Future<void> _initChat() async {
    if (_sessionId == null || _sessionId!.isEmpty) {
      if (mounted) {
        setState(() => _isLoading = false);
        _addSystemMessage('会话标识缺失，无法进入咨询对话');
      }
      return;
    }

    final initResult = await _service.initE2E(_sessionId!);
    if (mounted) {
      setState(() {
        _isE2EReady = initResult['success'] == true;
        _securityStatusMessage = initResult['success'] == true
            ? '对话内容已加密，仅你和咨询师可见'
            : (initResult['message']?.toString() ?? '安全通道未建立');
      });
    }
    if (initResult['success'] != true && mounted) {
      _showMessage(_securityStatusMessage!);
    }
    await _loadMessages();
    if (mounted) setState(() => _isLoading = false);
  }

  /// 加载历史消息并解密，无历史消息时添加咨询师欢迎语
  Future<void> _loadMessages() async {
    if (_sessionId == null) return;
    var loadFailed = false;
    try {
      final result = await _service.getMessages(_sessionId!);
      if (!mounted) return;
      if (result['success'] == true && result['data'] != null) {
        final data = result['data'];
        final List<dynamic> history =
            data is List ? data : (data['messages'] ?? []);
        final nextMessages = <Map<String, dynamic>>[];
        for (final msg in history) {
          final m =
              msg is Map ? Map<String, dynamic>.from(msg) : <String, dynamic>{};
          var content = m['content']?.toString() ?? '';
          if (content.isEmpty) {
            switch (m['decryption_status']?.toString()) {
              case 'legacy_payload':
                content = '[历史加密消息]';
                break;
              case 'pending_key_exchange':
                content = '[安全通道未建立，暂不可读]';
                break;
              case 'failed':
                content = '[无法解密]';
                break;
              default:
                if (m['encrypted'] != null) {
                  content = '[加密消息]';
                }
            }
          }
          final senderType = m['sender_type']?.toString().toLowerCase() ?? '';
          final sender = m['sender']?.toString().toLowerCase() ?? '';
          nextMessages.add({
            'content': content,
            'isMe': senderType == 'user' ||
                senderType == 'me' ||
                sender == 'user' ||
                sender == 'me',
            'time': m['created_at']?.toString() ?? m['time']?.toString() ?? '',
          });
        }
        setState(() {
          _messages
            ..clear()
            ..addAll(nextMessages);
        });
      } else {
        loadFailed = true;
        _showMessage(result['message']?.toString() ?? '历史消息加载失败');
      }
    } catch (error, stackTrace) {
      loadFailed = true;
      _reportUiError(error, stackTrace, 'ConsultationChatScreen._loadMessages');
      if (mounted) {
        _showMessage('历史消息加载失败，请稍后重试');
      }
    }

    if (_messages.isEmpty) {
      _addSystemMessage(
        loadFailed
            ? '历史消息加载失败，请稍后重试。'
            : '你好，我是${widget.counselorName}，很高兴为你提供心理咨询服务。请放心倾诉，我们的对话受到端到端加密保护。',
      );
    }

    _scrollToBottom();
  }

  /// 添加系统/咨询师消息到本地列表（非用户发送）
  void _addSystemMessage(String content) {
    _messages.add({
      'content': content,
      'isMe': false,
      'time': '',
    });
  }

  /// 发送消息：加密后发送到后端，接收咨询师回复并追加到列表
  Future<void> _sendMessage() async {
    final text = _controller.text.trim();
    if (text.isEmpty || _isSending || _sessionId == null) return;
    if (!_isE2EReady) {
      _showMessage(_securityStatusMessage ?? '安全通道未建立，请稍后再试');
      return;
    }

    final optimisticMessage = <String, dynamic>{
      'content': text,
      'isMe': true,
      'time': _nowStr(),
    };

    _controller.clear();
    setState(() {
      _messages.add(optimisticMessage);
      _isSending = true;
    });
    _scrollToBottom();

    try {
      final result = await _service.sendMessage(
        sessionId: _sessionId!,
        content: text,
      );
      if (!mounted) return;
      if (result['success'] == true) {
        final payload = result['data'] is Map<String, dynamic>
            ? result['data'] as Map<String, dynamic>
            : const <String, dynamic>{};
        final createdAt = payload['created_at']?.toString();
        if (createdAt != null && createdAt.isNotEmpty) {
          setState(() {
            optimisticMessage['time'] = createdAt;
          });
        }
      } else {
        setState(() {
          _messages.remove(optimisticMessage);
        });
        _controller.text = text;
        _controller.selection =
            TextSelection.collapsed(offset: _controller.text.length);
        _showMessage(result['message']?.toString() ?? '消息发送失败，请重试');
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'ConsultationChatScreen._sendMessage');
      if (mounted) {
        setState(() {
          _messages.remove(optimisticMessage);
        });
        _controller.text = text;
        _controller.selection =
            TextSelection.collapsed(offset: _controller.text.length);
        _showMessage('消息发送失败，请重试');
      }
    }

    if (mounted) {
      setState(() => _isSending = false);
      _scrollToBottom();
    }
  }

  /// 格式化当前时间为 HH:mm 字符串
  String _nowStr() {
    final now = DateTime.now();
    return '${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}';
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

  @override
  Widget build(BuildContext context) {
    final securityAccent = _isLoading
        ? Colors.white70
        : (_isE2EReady ? AppTheme.successColor : AppTheme.warningColor);
    final securityText =
        _isLoading ? '安全通道建立中' : (_securityStatusMessage ?? '安全通道未建立');

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
                Icon(
                  _isE2EReady ? Icons.lock_outline : Icons.lock_open_outlined,
                  size: 11,
                  color: Colors.white.withValues(alpha: 0.8),
                ),
                const SizedBox(width: 3),
                Text(
                    _isLoading
                        ? '安全通道建立中'
                        : (_isE2EReady ? '端到端加密保护' : '安全通道未建立'),
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
                    color: securityAccent.withValues(alpha: 0.15),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(
                        _isE2EReady
                            ? Icons.verified_user_outlined
                            : Icons.warning_amber_rounded,
                        size: 14,
                        color: securityAccent.withValues(alpha: 0.8),
                      ),
                      const SizedBox(width: 4),
                      Text(
                        _isE2EReady ? '对话内容已加密，仅你和咨询师可见' : securityText,
                        style: TextStyle(
                          fontSize: 11,
                          color: securityAccent.withValues(alpha: 0.8),
                        ),
                      ),
                    ],
                  ),
                ),
              ),
              // 消息列表
              Expanded(
                child: _isLoading
                    ? const Center(
                        child: CircularProgressIndicator(color: Colors.white70))
                    : ListView.builder(
                        controller: _scrollController,
                        padding: const EdgeInsets.symmetric(
                            horizontal: 16, vertical: 8),
                        itemCount: _messages.length,
                        itemBuilder: (context, index) {
                          final msg = _messages[index];
                          return _ChatBubble(
                            content: msg['content']?.toString() ?? '',
                            isMe: _asBool(msg['isMe']),
                            time: msg['time']?.toString() ?? '',
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

  /// 构建底部消息输入栏，包含文本输入框和发送按钮
  Widget _buildInputBar() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final canSend = !_isSending && _isE2EReady;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
      decoration: BoxDecoration(
        color: isDark
            ? const Color(0xFF16213E).withValues(alpha: 0.95)
            : Colors.white.withValues(alpha: 0.95),
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
              enabled: !_isLoading,
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
                : Icon(
                    Icons.send,
                    color:
                        canSend ? AppTheme.primaryColor : AppTheme.textTertiary,
                  ),
            onPressed: canSend ? _sendMessage : null,
          ),
        ],
      ),
    );
  }
}

/// 聊天气泡组件，区分用户消息和咨询师消息
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
                    : isDark
                        ? const Color(0xFF16213E).withValues(alpha: 0.92)
                        : Colors.white.withValues(alpha: 0.92),
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
                      color: isMe
                          ? Colors.white
                          : isDark
                              ? Colors.white
                              : AppTheme.textPrimary,
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
