import 'package:flutter/material.dart';

import '../../data/datasources/vip_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';

/// 灯火计划页面（原 VIP）
///
/// 展示灯火计划的权益介绍和订阅状态：
/// - 当前订阅等级与到期时间
/// - 各等级权益对比
/// - 订阅/续费操作
class VIPScreen extends StatefulWidget {
  const VIPScreen({super.key});

  @override
  State<VIPScreen> createState() => _VIPScreenState();
}

class _VIPScreenState extends State<VIPScreen>
    with SingleTickerProviderStateMixin {
  final _vipService = sl<VIPService>();

  static const Color _warmAccent = Color(0xFFEBC883);
  static const Color _darkPanelBase = Color(0xFF1A3042);

  bool _isLoading = true;
  bool _hasConfirmedStatus = false;
  bool _hasLight = false;
  int _daysLeft = 0;
  String? _statusErrorMessage;
  bool _privilegesLoading = true;
  String? _privilegesErrorMessage;
  List<_PrivilegeItem> _privileges = [];
  late AnimationController _animController;

  bool get _isLightActive => _hasConfirmedStatus && _hasLight;

  bool get _isPermanentLight => _isLightActive && _daysLeft <= 0;

  Color get _panelBase =>
      _isLightActive ? const Color(0xFFFFF3DD) : _darkPanelBase;

  Color get _screenBackground =>
      _isLightActive ? const Color(0xFFFFF8EA) : AppTheme.nightDeep;

  Color get _primaryText =>
      _isLightActive ? const Color(0xFF5E3D12) : Colors.white;

  Color get _secondaryText => _isLightActive
      ? const Color(0xFF8A6A38)
      : Colors.white.withValues(alpha: 0.74);

  static final Map<String, IconData> _iconMap = {
    'water_drop': Icons.water_drop,
    'psychology': Icons.psychology,
    'local_fire_department': Icons.local_fire_department,
    'people': Icons.people,
    'auto_awesome': Icons.auto_awesome,
    'favorite': Icons.favorite,
    'star': Icons.star,
    'shield': Icons.shield,
    'spa': Icons.spa,
    'lightbulb': Icons.lightbulb,
    'emoji_emotions': Icons.emoji_emotions,
    'health_and_safety': Icons.health_and_safety,
    'volunteer_activism': Icons.volunteer_activism,
    'diversity_3': Icons.diversity_3,
    'self_improvement': Icons.self_improvement,
  };

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1800),
    )..repeat(reverse: true);
    _loadLightData();
    _loadPrivileges();
  }

  @override
  void dispose() {
    _animController.dispose();
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

  String _resolveMessage(
    Object error, {
    required String fallback,
  }) {
    final message = error.toString().trim();
    return message.isEmpty ? fallback : message;
  }

  _PrivilegeItem _parsePrivilegeItem(dynamic raw) {
    if (raw is! Map) {
      throw const FormatException('灯火权益项格式错误');
    }

    final item = Map<String, dynamic>.from(raw.cast<String, dynamic>());
    final title = item['title']?.toString().trim() ?? '';
    final desc = item['desc']?.toString().trim() ?? '';
    if (title.isEmpty || desc.isEmpty) {
      throw const FormatException('灯火权益项缺少标题或描述');
    }

    final iconName = item['icon']?.toString() ?? 'star';
    final colorValue = item['color'];
    final resolvedColor = colorValue is int
        ? Color(colorValue)
        : colorValue is num
            ? Color(colorValue.toInt())
            : _warmAccent;

    return _PrivilegeItem(
      _iconMap[iconName] ?? Icons.star,
      title,
      desc,
      resolvedColor,
    );
  }

  Future<void> _loadLightData() async {
    setState(() {
      _isLoading = true;
      _statusErrorMessage = null;
    });
    try {
      final status = await _vipService.getVIPStatus();
      if (status['success'] != true) {
        throw StateError(status['message']?.toString() ?? '加载灯火状态失败');
      }

      final payload = status['data'];
      if (payload is! Map<String, dynamic>) {
        throw const FormatException('灯火状态响应格式错误');
      }

      final isVip = payload['is_vip'];
      if (isVip is! bool) {
        throw const FormatException('灯火状态缺少 is_vip');
      }

      final daysLeftRaw = payload['days_left'];
      if (daysLeftRaw != null && daysLeftRaw is! num) {
        throw const FormatException('灯火状态中的 days_left 非数字');
      }

      if (!mounted) return;
      setState(() {
        _hasConfirmedStatus = true;
        _hasLight = isVip;
        _daysLeft = (daysLeftRaw as num?)?.toInt() ?? 0;
        _statusErrorMessage = null;
        _isLoading = false;
      });
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'loading vip status');
      if (!mounted) return;
      setState(() {
        _statusErrorMessage = _resolveMessage(
          error,
          fallback: '加载灯火状态失败，请重试',
        );
        _isLoading = false;
      });
    }
  }

  Future<void> _loadPrivileges() async {
    setState(() {
      _privilegesLoading = true;
      _privilegesErrorMessage = null;
    });
    try {
      final result = await _vipService.getPrivileges();
      if (result['success'] != true) {
        throw StateError(result['message']?.toString() ?? '加载灯火权益失败');
      }

      final payload = result['data'];
      if (payload is! Map<String, dynamic>) {
        throw const FormatException('灯火权益响应格式错误');
      }

      final items = payload['privileges'];
      if (items is! List) {
        throw const FormatException('灯火权益响应缺少 privileges');
      }

      final privileges = items.map(_parsePrivilegeItem).toList();
      if (!mounted) return;
      setState(() {
        _privileges = privileges;
        _privilegesErrorMessage = null;
        _privilegesLoading = false;
      });
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'loading vip privileges');
      if (!mounted) return;
      setState(() {
        _privilegesErrorMessage = _resolveMessage(
          error,
          fallback: '加载灯火权益失败，请重试',
        );
        _privilegesLoading = false;
      });
    }
  }

  Future<void> _refreshAll() async {
    await Future.wait([_loadLightData(), _loadPrivileges()]);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: _screenBackground,
      body: Stack(
        children: [
          _buildBackground(),
          SafeArea(
            child: _isLoading
                ? const Center(
                    child: CircularProgressIndicator(color: AppTheme.vipGold),
                  )
                : !_hasConfirmedStatus
                    ? _buildStatusFailureState()
                    : RefreshIndicator(
                        onRefresh: _refreshAll,
                        color: AppTheme.vipGoldDark,
                        backgroundColor: _isLightActive
                            ? const Color(0xFFFFF7E5)
                            : const Color(0xFF102131),
                        child: ListView(
                          physics: const AlwaysScrollableScrollPhysics(),
                          padding: const EdgeInsets.fromLTRB(20, 14, 20, 28),
                          children: [
                            _buildTopBar(),
                            if (_statusErrorMessage != null) ...[
                              const SizedBox(height: 8),
                              _buildMessageBanner(
                                icon: Icons.error_outline,
                                color: Colors.orangeAccent,
                                message: _statusErrorMessage!,
                                onRetry: _loadLightData,
                              ),
                            ],
                            if (_isLightActive) ...[
                              const SizedBox(height: 6),
                              _buildLitBanner(),
                            ],
                            const SizedBox(height: 14),
                            _buildHeroCard(),
                            const SizedBox(height: 16),
                            _buildSummaryRow(),
                            const SizedBox(height: 20),
                            _buildPrivilegesSection(),
                            const SizedBox(height: 20),
                            _buildGuideSection(),
                          ],
                        ),
                      ),
          ),
        ],
      ),
    );
  }

  Widget _buildStatusFailureState() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              _statusErrorMessage ?? '加载灯火状态失败',
              textAlign: TextAlign.center,
              style: TextStyle(color: _primaryText, fontSize: 15),
            ),
            const SizedBox(height: 12),
            FilledButton(
              onPressed: _refreshAll,
              style: FilledButton.styleFrom(
                backgroundColor: _warmAccent,
                foregroundColor: const Color(0xFF5E3D12),
              ),
              child: const Text('重试'),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildBackground() {
    final backgroundColors = _isLightActive
        ? const [Color(0xFFFFF5CB), Color(0xFFFFE8B0), Color(0xFFE2F5FF)]
        : const [Color(0xFF142F43), Color(0xFF1B3D56), Color(0xFF172D40)];

    return DecoratedBox(
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: backgroundColors,
        ),
      ),
      child: Stack(
        children: [
          if (_isLightActive)
            Positioned.fill(
              child: AnimatedBuilder(
                animation: _animController,
                builder: (context, child) {
                  final aura = 0.16 + (_animController.value * 0.16);
                  return IgnorePointer(
                    child: DecoratedBox(
                      decoration: BoxDecoration(
                        gradient: RadialGradient(
                          center: const Alignment(0, -0.8),
                          radius: 1.15,
                          colors: [
                            const Color(0xFFFFF2C9).withValues(alpha: aura),
                            Colors.transparent,
                          ],
                        ),
                      ),
                    ),
                  );
                },
              ),
            ),
          Positioned(
            top: -120,
            right: -80,
            child: _glowCircle(
              300,
              _isLightActive
                  ? const Color(0x90FFE2A8)
                  : const Color(0x2CE8BC72),
            ),
          ),
          Positioned(
            top: 220,
            left: -90,
            child: _glowCircle(
              240,
              _isLightActive
                  ? const Color(0x75BDEBFF)
                  : const Color(0x3043B4E8),
            ),
          ),
          if (_isLightActive)
            Positioned.fill(
              child: IgnorePointer(
                child: DecoratedBox(
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                      begin: Alignment.topCenter,
                      end: Alignment.bottomCenter,
                      colors: [
                        Colors.white.withValues(alpha: 0.18),
                        Colors.transparent,
                        Colors.transparent,
                      ],
                    ),
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }

  Widget _glowCircle(double size, Color color) {
    return IgnorePointer(
      child: Container(
        width: size,
        height: size,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          color: color,
          boxShadow: [
            BoxShadow(
              color: color.withValues(alpha: 0.65),
              blurRadius: 56,
              spreadRadius: 12,
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTopBar() {
    return Row(
      children: [
        IconButton(
          onPressed: () => Navigator.pop(context),
          icon: Icon(
            Icons.arrow_back_ios_new_rounded,
            color: _isLightActive ? const Color(0xFF7A5A26) : Colors.white,
          ),
        ),
        Expanded(
          child: Text(
            '灯火守护',
            textAlign: TextAlign.center,
            style: TextStyle(
              color: _primaryText,
              fontSize: 20,
              fontWeight: FontWeight.w700,
              letterSpacing: 0.4,
            ),
          ),
        ),
        IconButton(
          onPressed: _refreshAll,
          icon: Icon(
            Icons.refresh_rounded,
            color: _isLightActive ? const Color(0xFF8A6A38) : Colors.white70,
          ),
        ),
      ],
    );
  }

  Widget _buildLitBanner() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(14),
        gradient: LinearGradient(
          colors: [
            const Color(0xFFFFF1C7).withValues(alpha: 0.96),
            const Color(0xFFEAF7FF).withValues(alpha: 0.9),
          ],
        ),
        border: Border.all(color: const Color(0xFFE8C47A)),
        boxShadow: const [
          BoxShadow(
            color: Color(0x66FFD98E),
            blurRadius: 20,
            spreadRadius: 1,
          ),
        ],
      ),
      child: const Row(
        children: [
          Icon(Icons.wb_incandescent_rounded,
              color: Color(0xFFD39A2C), size: 18),
          SizedBox(width: 8),
          Text(
            '灯已点亮，正在为你持续照明',
            style: TextStyle(
              color: Color(0xFF6B4917),
              fontSize: 12,
              fontWeight: FontWeight.w700,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeroCard() {
    return AnimatedBuilder(
      animation: _animController,
      builder: (context, _) {
        final glow = _isLightActive
            ? 0.22 + (_animController.value * 0.22)
            : 0.08 + (_animController.value * 0.1);
        final pulse = 0.92 + (_animController.value * 0.12);
        final cardColors = _isLightActive
            ? const [Color(0xFFFFFCE8), Color(0xFFFFE8AF), Color(0xFFFFD37C)]
            : const [Color(0xFF1C2E42), Color(0xFF2A3E54), Color(0xFF364E67)];

        return Container(
          padding: const EdgeInsets.all(20),
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(28),
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: cardColors,
            ),
            border: Border.all(
              color: _isLightActive
                  ? const Color(0xFFE7C072)
                  : Colors.white.withValues(alpha: 0.14),
            ),
            boxShadow: [
              BoxShadow(
                color: _isLightActive
                    ? const Color(0x55E7B85E)
                    : Colors.black.withValues(alpha: 0.2),
                offset: const Offset(0, 10),
                blurRadius: _isLightActive ? 24 : 18,
              ),
              if (_isLightActive)
                BoxShadow(
                  color: const Color(0x88FFD98B).withValues(alpha: glow),
                  blurRadius: 48,
                  spreadRadius: 6,
                ),
              if (_isLightActive)
                BoxShadow(
                  color: const Color(0x8894D7FF).withValues(alpha: glow * 0.62),
                  blurRadius: 24,
                  spreadRadius: 1,
                ),
            ],
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  _statusPill(
                    _isLightActive ? '点亮中' : '待点亮',
                    _isLightActive
                        ? Icons.flash_on_rounded
                        : Icons.dark_mode_rounded,
                  ),
                  const Spacer(),
                  Transform.scale(
                    scale: pulse,
                    child: Container(
                      width: 72,
                      height: 72,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: _isLightActive
                            ? const Color(0xFFFFF6DE)
                            : Colors.white.withValues(alpha: 0.14),
                        border: Border.all(
                          color: _isLightActive
                              ? const Color(0xFFE6BF72)
                              : Colors.white.withValues(alpha: 0.3),
                        ),
                      ),
                      child: Icon(
                        _isLightActive
                            ? Icons.lightbulb_rounded
                            : Icons.lightbulb_outline_rounded,
                        color: _isLightActive
                            ? const Color(0xFFE0A73D)
                            : Colors.white,
                        size: 38,
                      ),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 14),
              Text(
                _isLightActive ? '灯已点亮' : '灯未点亮',
                style: TextStyle(
                  color: _primaryText,
                  fontSize: 30,
                  fontWeight: FontWeight.w800,
                  height: 1.1,
                ),
              ),
              const SizedBox(height: 8),
              Text(
                _isLightActive
                    ? '你当前处于灯火守护中，可直接使用完整守护权益。'
                    : '当系统检测到你需要关怀时，会自动点亮灯火并开放权益。',
                style: TextStyle(
                  color: _secondaryText,
                  fontSize: 13,
                  height: 1.45,
                ),
              ),
              const SizedBox(height: 16),
              Row(
                children: [
                  Expanded(
                    child: _heroMetric(
                      icon: Icons.schedule_rounded,
                      label: '剩余时长',
                      value:
                          _isLightActive ? (_isPermanentLight ? '长期点亮' : '$_daysLeft 天') : '-',
                    ),
                  ),
                  const SizedBox(width: 10),
                  Expanded(
                    child: _heroMetric(
                      icon: Icons.verified_rounded,
                      label: '权益状态',
                      value: _isLightActive ? '已解锁' : '未解锁',
                    ),
                  ),
                ],
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _statusPill(String text, IconData icon) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(999),
        color: _isLightActive
            ? const Color(0x33FFFFFF)
            : Colors.black.withValues(alpha: 0.2),
        border: Border.all(
          color: _isLightActive
              ? const Color(0xFFE5BF74)
              : Colors.white.withValues(alpha: 0.24),
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 14, color: Colors.white),
          const SizedBox(width: 4),
          Text(
            text,
            style: TextStyle(
              color: _isLightActive ? const Color(0xFF6D4A18) : Colors.white,
              fontSize: 11,
              fontWeight: FontWeight.w700,
              letterSpacing: 0.6,
            ),
          ),
        ],
      ),
    );
  }

  Widget _heroMetric({
    required IconData icon,
    required String label,
    required String value,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(14),
        color: _isLightActive
            ? const Color(0x73FFFFFF)
            : Colors.black.withValues(alpha: 0.14),
        border: Border.all(
          color: _isLightActive
              ? const Color(0xFFE6C273)
              : Colors.white.withValues(alpha: 0.2),
        ),
      ),
      child: Row(
        children: [
          Icon(
            icon,
            size: 16,
            color: _isLightActive ? const Color(0xFF8F6A2A) : Colors.white,
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  label,
                  style: TextStyle(
                    color: _isLightActive
                        ? const Color(0xFF8D6A33)
                        : Colors.white.withValues(alpha: 0.84),
                    fontSize: 11,
                    fontWeight: FontWeight.w500,
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  value,
                  style: TextStyle(
                    color: _primaryText,
                    fontSize: 13,
                    fontWeight: FontWeight.w700,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSummaryRow() {
    return Row(
      children: [
        Expanded(
          child: _summaryCard(
            '状态',
            _isLightActive ? '守护中' : '未激活',
            Icons.favorite_rounded,
            _isLightActive ? _warmAccent : const Color(0xFF9AA7B4),
          ),
        ),
        const SizedBox(width: 10),
        Expanded(
          child: _summaryCard(
            '时长',
            _isLightActive ? (_isPermanentLight ? '长期' : '$_daysLeft天') : '-',
            Icons.timelapse_rounded,
            const Color(0xFF70D6FF),
          ),
        ),
        const SizedBox(width: 10),
        Expanded(
          child: _summaryCard(
            '权限',
            _isLightActive ? '全开' : '待解锁',
            Icons.auto_awesome_rounded,
            const Color(0xFFC792EA),
          ),
        ),
      ],
    );
  }

  Widget _summaryCard(String title, String value, IconData icon, Color accent) {
    return Container(
      height: 106,
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(16),
        color: _isLightActive
            ? const Color(0xD9FFFFFF)
            : _panelBase.withValues(alpha: 0.48),
        border: Border.all(
          color: _isLightActive
              ? const Color(0xFFE6C171)
              : Colors.white.withValues(alpha: 0.16),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(icon, size: 18, color: accent),
          const Spacer(),
          Text(
            title,
            style: TextStyle(
              color: _isLightActive
                  ? const Color(0xFF8D6A33)
                  : Colors.white.withValues(alpha: 0.7),
              fontSize: 11,
              fontWeight: FontWeight.w500,
            ),
          ),
          const SizedBox(height: 2),
          Text(
            value,
            style: TextStyle(
              color: _primaryText,
              fontSize: 15,
              fontWeight: FontWeight.w700,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildPrivilegesSection() {
    return Container(
      padding: const EdgeInsets.fromLTRB(16, 16, 16, 8),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(20),
        color: _isLightActive
            ? const Color(0xD9FFFFFF)
            : _panelBase.withValues(alpha: 0.52),
        border: Border.all(
          color: _isLightActive
              ? const Color(0xFFE6C171)
              : Colors.white.withValues(alpha: 0.16),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '灯火权益',
            style: TextStyle(
              color: _primaryText,
              fontSize: 18,
              fontWeight: FontWeight.w700,
            ),
          ),
          const SizedBox(height: 6),
          Text(
            _isLightActive ? '当前权益全部可用' : '点亮后自动解锁全部权益',
            style: TextStyle(
              color: _secondaryText,
              fontSize: 12,
            ),
          ),
          const SizedBox(height: 14),
          if (_privilegesErrorMessage != null) ...[
            _buildMessageBanner(
              icon: Icons.warning_amber_rounded,
              color: Colors.orangeAccent,
              message: _privilegesErrorMessage!,
              onRetry: _loadPrivileges,
            ),
            const SizedBox(height: 12),
          ],
          if (_privilegesLoading)
            const Padding(
              padding: EdgeInsets.symmetric(vertical: 24),
              child: Center(
                child: CircularProgressIndicator(
                  color: _warmAccent,
                  strokeWidth: 2,
                ),
              ),
            )
          else if (_privileges.isEmpty)
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 20),
              child: Center(
                child: Text(
                  '当前没有可展示的灯火权益配置',
                  style: TextStyle(
                    color: _secondaryText,
                    fontSize: 12,
                  ),
                ),
              ),
            )
          else
            GridView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              itemCount: _privileges.length,
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 2,
                crossAxisSpacing: 10,
                mainAxisSpacing: 10,
                childAspectRatio: 1.26,
              ),
              itemBuilder: (context, index) {
                final item = _privileges[index];
                return _privilegeCard(item);
              },
            ),
          const SizedBox(height: 10),
        ],
      ),
    );
  }

  Widget _privilegeCard(_PrivilegeItem item) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(14),
        color: _isLightActive
            ? const Color(0xF2FFF8E9)
            : Colors.white.withValues(alpha: 0.07),
        border: Border.all(color: item.color.withValues(alpha: 0.35)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(item.icon, size: 20, color: item.color),
              const Spacer(),
              Icon(
                _isLightActive
                    ? Icons.check_circle_rounded
                    : Icons.lock_outline_rounded,
                size: 16,
                color:
                    _isLightActive ? const Color(0xFFD79A34) : Colors.white54,
              ),
            ],
          ),
          const Spacer(),
          Text(
            item.title,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: TextStyle(
              color: _primaryText,
              fontSize: 13,
              fontWeight: FontWeight.w700,
            ),
          ),
          const SizedBox(height: 4),
          Text(
            item.desc,
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
            style: TextStyle(
              color: _secondaryText,
              fontSize: 11,
              height: 1.3,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildGuideSection() {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(20),
        color: _isLightActive
            ? const Color(0xE6FFF7E4)
            : _panelBase.withValues(alpha: 0.58),
        border: Border.all(
          color: _isLightActive
              ? const Color(0xFFE3BB6C)
              : _warmAccent.withValues(alpha: 0.3),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.tips_and_updates_rounded,
                color: _isLightActive ? const Color(0xFFD69A35) : _warmAccent,
                size: 20,
              ),
              const SizedBox(width: 8),
              Text(
                '点亮机制说明',
                style: TextStyle(
                  color: _primaryText,
                  fontSize: 16,
                  fontWeight: FontWeight.w700,
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          _stepItem(
            icon: Icons.auto_graph_rounded,
            title: '1. 情绪识别',
            content: '系统根据你的公开互动与情绪信号进行综合判断。',
          ),
          const SizedBox(height: 8),
          _stepItem(
            icon: Icons.local_fire_department_rounded,
            title: '2. 自动点亮',
            content: '当检测到你需要关怀时，灯火会自动开启，不需要手动申请。',
          ),
          const SizedBox(height: 8),
          _stepItem(
            icon: Icons.volunteer_activism_rounded,
            title: '3. 守护生效',
            content: '点亮后可即时使用咨询、AI关怀与优先互动等权益。',
          ),
        ],
      ),
    );
  }

  Widget _buildMessageBanner({
    required IconData icon,
    required Color color,
    required String message,
    required Future<void> Function() onRetry,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.14),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: color.withValues(alpha: 0.34)),
      ),
      child: Row(
        children: [
          Icon(icon, color: color, size: 18),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              message,
              style: TextStyle(
                color: _primaryText,
                fontSize: 12,
                fontWeight: FontWeight.w500,
              ),
            ),
          ),
          TextButton(
            onPressed: () {
              onRetry();
            },
            child: const Text('重试'),
          ),
        ],
      ),
    );
  }

  Widget _stepItem({
    required IconData icon,
    required String title,
    required String content,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(12),
        color: _isLightActive
            ? const Color(0xF2FFFFFF)
            : Colors.white.withValues(alpha: 0.05),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(
            icon,
            size: 18,
            color: _isLightActive ? const Color(0xFFD69A35) : _warmAccent,
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: TextStyle(
                    color: _primaryText,
                    fontSize: 13,
                    fontWeight: FontWeight.w700,
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  content,
                  style: TextStyle(
                    color: _secondaryText,
                    fontSize: 12,
                    height: 1.35,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _PrivilegeItem {
  final IconData icon;
  final String title;
  final String desc;
  final Color color;

  _PrivilegeItem(this.icon, this.title, this.desc, this.color);
}
