import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../data/datasources/guardian_service.dart';
import '../../data/datasources/friend_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/payload_contract.dart';
import '../widgets/water_background.dart';
import 'lake_god_chat_screen.dart';

/// 守护者页面
///
/// 守护者机制允许用户指定信任的好友作为守护人，
/// 当系统检测到异常情绪或行为时自动通知守护者。
/// 页面功能：查看当前守护者、从好友列表中添加/移除守护者、
/// 查看守护状态和告警记录。
class GuardianScreen extends StatefulWidget {
  const GuardianScreen({super.key});

  @override
  State<GuardianScreen> createState() => _GuardianScreenState();
}

class _GuardianScreenState extends State<GuardianScreen>
    with SingleTickerProviderStateMixin {
  final _service = sl<GuardianService>();
  final _friendService = sl<FriendService>();
  Map<String, dynamic>? _stats;
  bool _loading = true;
  late AnimationController _animController;
  late Animation<double> _fadeAnim;
  late Animation<double> _scaleAnim;

  // 情感洞察
  Map<String, dynamic>? _insights;
  bool _insightsLoading = true;
  String? _insightsError;

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(
      duration: const Duration(milliseconds: 600),
      vsync: this,
    );
    _fadeAnim = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _animController, curve: Curves.easeOut),
    );
    _scaleAnim = Tween<double>(begin: 0.8, end: 1.0).animate(
      CurvedAnimation(parent: _animController, curve: Curves.easeOutBack),
    );
    _loadStats();
    _loadInsights();
  }

  @override
  void dispose() {
    _animController.dispose();
    super.dispose();
  }

  Future<void> _loadStats() async {
    try {
      final stats = await _service.getStats();
      if (!mounted) return;
      setState(() {
        _stats = (stats['success'] == true && stats['data'] is Map)
            ? Map<String, dynamic>.from(stats['data'] as Map)
            : null;
        _loading = false;
      });
      _animController.forward();
    } catch (e) {
      if (!mounted) return;
      setState(() => _loading = false);
    }
  }

  Future<void> _loadInsights() async {
    try {
      final insights = await _service.getEmotionInsights();
      if (mounted) {
        setState(() {
          _insights = (insights['success'] == true && insights['data'] is Map)
              ? Map<String, dynamic>.from(insights['data'] as Map)
              : null;
          _insightsError = _insights == null
              ? (insights['message']?.toString().isNotEmpty == true
                  ? insights['message'].toString()
                  : '暂无情感洞察数据')
              : null;
          _insightsLoading = false;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _insightsLoading = false;
          _insightsError = '加载情感洞察失败';
        });
      }
    }
  }

  Widget _buildInsightsCard() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Card(
      color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.95) : Colors.white.withValues(alpha: 0.95),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                    color: AppTheme.purpleColor.withValues(alpha: 0.1),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child:
                      const Icon(Icons.psychology, color: AppTheme.purpleColor),
                ),
                const SizedBox(width: 12),
                const Expanded(
                  child: Text(
                    '情感洞察',
                    style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                  ),
                ),
                if (!_insightsLoading && _insightsError == null)
                  IconButton(
                    icon:
                        Icon(Icons.refresh, size: 20, color: isDark ? const Color(0xFF9AA0A6) : Colors.grey),
                    onPressed: () {
                      setState(() {
                        _insightsLoading = true;
                        _insightsError = null;
                      });
                      _loadInsights();
                    },
                  ),
              ],
            ),
            const SizedBox(height: 16),
            if (_insightsLoading)
              const Center(
                child: Padding(
                  padding: EdgeInsets.symmetric(vertical: 20),
                  child: SizedBox(
                    width: 24,
                    height: 24,
                    child: CircularProgressIndicator(
                        strokeWidth: 2, color: AppTheme.purpleColor),
                  ),
                ),
              )
            else if (_insightsError != null)
              Center(
                child: Padding(
                  padding: const EdgeInsets.symmetric(vertical: 12),
                  child: Column(
                    children: [
                      Icon(Icons.error_outline,
                          color: isDark ? const Color(0xFF9AA0A6) : Colors.grey, size: 32),
                      const SizedBox(height: 8),
                      Text(_insightsError!,
                          style: TextStyle(color: isDark ? const Color(0xFF9AA0A6) : Colors.grey)),
                      const SizedBox(height: 8),
                      TextButton(
                        onPressed: () {
                          setState(() {
                            _insightsLoading = true;
                            _insightsError = null;
                          });
                          _loadInsights();
                        },
                        child: const Text('重试'),
                      ),
                    ],
                  ),
                ),
              )
            else if (_insights != null)
              ..._buildInsightsContent(),
          ],
        ),
      ),
    );
  }

  List<Widget> _buildInsightsContent() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final List<Widget> widgets = [];
    final profile = _insights?['profile'] is Map
        ? Map<String, dynamic>.from(_insights!['profile'] as Map)
        : const <String, dynamic>{};
    final longTermProfile = _insights?['long_term_profile'] is Map
        ? Map<String, dynamic>.from(_insights!['long_term_profile'] as Map)
        : const <String, dynamic>{};

    // 情感趋势
    final trend = profile['emotion_trend'] ??
        longTermProfile['emotion_trend'] ??
        _insights!['emotion_trend'] ??
        _insights!['trend'];
    if (trend != null) {
      widgets.add(
        Container(
          padding: const EdgeInsets.all(12),
          decoration: BoxDecoration(
            color: AppTheme.skyBlue.withValues(alpha: 0.08),
            borderRadius: BorderRadius.circular(12),
          ),
          child: Row(
            children: [
              const Icon(Icons.trending_up, color: AppTheme.skyBlue, size: 20),
              const SizedBox(width: 10),
              Expanded(
                child: Text(
                  _trendLabel(trend.toString()),
                  style: const TextStyle(
                      fontSize: 14, color: AppTheme.textPrimary),
                ),
              ),
            ],
          ),
        ),
      );
      widgets.add(const SizedBox(height: 12));
    }

    // 情感摘要
    final summary = _insights!['summary'] ??
        _insights!['emotion_summary'] ??
        _insights!['trend_description'];
    if (summary != null) {
      widgets.add(
        Text(
          '$summary',
          style: const TextStyle(
              fontSize: 14, color: AppTheme.textSecondary, height: 1.5),
        ),
      );
      widgets.add(const SizedBox(height: 12));
    }

    // 建议列表
    final suggestions = _insights!['suggestions'] ?? _insights!['advice'];
    if (suggestions is List && suggestions.isNotEmpty) {
      for (final suggestion in suggestions) {
        widgets.add(
          Padding(
            padding: const EdgeInsets.only(bottom: 8),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Padding(
                  padding: EdgeInsets.only(top: 2),
                  child: Icon(Icons.lightbulb_outline,
                      size: 18, color: AppTheme.accentColor),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    '$suggestion',
                    style: const TextStyle(
                        fontSize: 13,
                        color: AppTheme.textSecondary,
                        height: 1.4),
                  ),
                ),
              ],
            ),
          ),
        );
      }
    }

    // 如果没有任何内容，显示默认提示
    if (widgets.isEmpty) {
      widgets.add(
        Center(
          child: Padding(
            padding: const EdgeInsets.symmetric(vertical: 8),
            child: Text('暂无洞察数据', style: TextStyle(color: isDark ? const Color(0xFF9AA0A6) : Colors.grey)),
          ),
        ),
      );
    }

    return widgets;
  }

  String _trendLabel(String trend) {
    const mapping = {
      'stable': '最近情绪比较平稳',
      'rising': '最近情绪正在回暖',
      'falling': '最近情绪有些低落',
      'volatile': '最近情绪波动较大',
    };
    return mapping[trend.toLowerCase()] ?? trend;
  }

  String? _extractFriendId(Map<String, dynamic> friend) =>
      extractFriendEntityId(friend);

  Future<void> _showTransferDialog() async {
    final controller = TextEditingController();
    List<Map<String, dynamic>> friends = <Map<String, dynamic>>[];
    try {
      final result = await _friendService.getFriends();
      final raw = result['friends'];
      if (raw is List) {
        friends = raw
            .whereType<Map>()
            .map((item) => Map<String, dynamic>.from(item))
            .toList();
      }
    } catch (_) {}

    if (!mounted) {
      controller.dispose();
      return;
    }

    final result = await showDialog<String>(
      context: context,
      builder: (ctx) {
        final isDark = Theme.of(ctx).brightness == Brightness.dark;
        String selectedFriendId = '';
        return AlertDialog(
          title: const Text('转赠灯火'),
          content: StatefulBuilder(
            builder: (context, setDialogState) {
              return Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text('将你的温暖传递给需要的人',
                      style: TextStyle(
                          color: isDark ? const Color(0xFF9AA0A6) : Colors.grey)),
                  const SizedBox(height: 16),
                  if (friends.isNotEmpty) ...[
                    DropdownButtonFormField<String>(
                      initialValue:
                          selectedFriendId.isEmpty ? null : selectedFriendId,
                      decoration: const InputDecoration(
                        labelText: '从好友中选择',
                        border: OutlineInputBorder(),
                      ),
                      items: friends
                          .map((friend) {
                            final friendId = _extractFriendId(friend);
                            if (friendId == null) return null;
                            final nickname = friend['nickname']?.toString() ??
                                friend['nick_name']?.toString() ??
                                '好友';
                            final username = friend['username']?.toString() ?? '';
                            return DropdownMenuItem<String>(
                              value: friendId,
                              child: Text('$nickname ($username)'),
                            );
                          })
                          .whereType<DropdownMenuItem<String>>()
                          .toList(),
                      onChanged: (value) {
                        final id = value?.trim() ?? '';
                        setDialogState(() {
                          selectedFriendId = id;
                          controller.text = id;
                        });
                      },
                    ),
                    const SizedBox(height: 12),
                  ],
                  TextField(
                    controller: controller,
                    decoration: const InputDecoration(
                        labelText: '对方ID', border: OutlineInputBorder()),
                  ),
                  if (controller.text.isNotEmpty) ...[
                    const SizedBox(height: 8),
                    Align(
                      alignment: Alignment.centerRight,
                      child: TextButton.icon(
                        onPressed: () async {
                          await Clipboard.setData(
                            ClipboardData(text: controller.text.trim()),
                          );
                        },
                        icon: const Icon(Icons.copy, size: 16),
                        label: const Text('复制ID'),
                      ),
                    ),
                  ],
                ],
              );
            },
          ),
          actions: [
            TextButton(
                onPressed: () => Navigator.pop(ctx), child: const Text('取消')),
            ElevatedButton(
              onPressed: () => Navigator.pop(ctx, controller.text.trim()),
              style: ElevatedButton.styleFrom(backgroundColor: Colors.orange),
              child: const Text('转赠'),
            ),
          ],
        );
      },
    );
    controller.dispose();
    if (result != null && result.isNotEmpty && mounted) {
      try {
        final response = await _service.transferLamp(result);
        final bool success = response['success'] == true;
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
                content: Text(success ? '灯火已传递' : '转赠失败'),
                backgroundColor: success ? Colors.green : Colors.red),
          );
          if (success) _loadStats();
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
                content: Text('网络异常，请稍后再试'), backgroundColor: Colors.red),
          );
        }
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
          title: const Text('守护者'),
          backgroundColor: Colors.transparent,
          elevation: 0,
          foregroundColor: Colors.white),
      body: Stack(
        children: [
          const Positioned.fill(child: WaterBackground()),
          SafeArea(
            child: _loading
                ? const Center(
                    child: CircularProgressIndicator(color: Colors.white))
                : _stats == null
                    ? const Center(
                        child:
                            Text('暂无数据', style: TextStyle(color: Colors.white)))
                    : FadeTransition(
                        opacity: _fadeAnim,
                        child: ScaleTransition(
                          scale: _scaleAnim,
                          child: ListView(
                            padding: const EdgeInsets.all(16),
                            children: [
                              Card(
                                color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.95) : Colors.white.withValues(alpha: 0.95),
                                shape: RoundedRectangleBorder(
                                    borderRadius: BorderRadius.circular(16)),
                                child: Padding(
                                  padding: const EdgeInsets.all(20),
                                  child: Column(
                                    children: [
                                      Icon(
                                        _stats!['is_guardian'] == true
                                            ? Icons.local_fire_department
                                            : Icons
                                                .local_fire_department_outlined,
                                        size: 64,
                                        color: _stats!['is_guardian'] == true
                                            ? Colors.orange
                                            : isDark ? const Color(0xFF9AA0A6) : Colors.grey,
                                      ),
                                      const SizedBox(height: 12),
                                      Text(
                                        _stats!['is_guardian'] == true
                                            ? '你是一位守护者'
                                            : '成为守护者',
                                        style: const TextStyle(
                                            fontSize: 20,
                                            fontWeight: FontWeight.bold),
                                      ),
                                      const SizedBox(height: 8),
                                      Text('用温暖的涟漪，照亮他人的心湖',
                                          style: TextStyle(color: isDark ? const Color(0xFF9AA0A6) : Colors.grey)),
                                    ],
                                  ),
                                ),
                              ),
                              const SizedBox(height: 16),
                              Row(
                                children: [
                                  _StatCard(
                                      label: '共鸣点数',
                                      value:
                                          '${_stats!['resonance_points'] ?? 0}'),
                                  const SizedBox(width: 12),
                                  _StatCard(
                                      label: '优质涟漪',
                                      value:
                                          '${_stats!['quality_ripples'] ?? 0}'),
                                  const SizedBox(width: 12),
                                  _StatCard(
                                      label: '温暖纸船',
                                      value: '${_stats!['warm_boats'] ?? 0}'),
                                ],
                              ),
                              const SizedBox(height: 16),
                              // 情感洞察卡片
                              _buildInsightsCard(),
                              const SizedBox(height: 24),
                              if (_stats!['is_guardian'] == true)
                                ElevatedButton.icon(
                                  onPressed: _showTransferDialog,
                                  icon: const Icon(Icons.volunteer_activism),
                                  label: const Text('转赠灯火'),
                                  style: ElevatedButton.styleFrom(
                                    backgroundColor: Colors.orange,
                                    foregroundColor: Colors.white,
                                    padding: const EdgeInsets.symmetric(
                                        vertical: 14),
                                    shape: RoundedRectangleBorder(
                                        borderRadius:
                                            BorderRadius.circular(12)),
                                  ),
                                ),
                              const SizedBox(height: 16),
                              // 湖神入口
                              Card(
                                color: isDark ? const Color(0xFF16213E).withValues(alpha: 0.95) : Colors.white.withValues(alpha: 0.95),
                                shape: RoundedRectangleBorder(
                                    borderRadius: BorderRadius.circular(16)),
                                child: ListTile(
                                  leading: Container(
                                    padding: const EdgeInsets.all(10),
                                    decoration: BoxDecoration(
                                        color: AppTheme.skyBlue
                                            .withValues(alpha: 0.1),
                                        borderRadius:
                                            BorderRadius.circular(12)),
                                    child: const Icon(Icons.auto_awesome,
                                        color: AppTheme.skyBlue),
                                  ),
                                  title: const Text('与湖神对话',
                                      style: TextStyle(
                                          fontWeight: FontWeight.bold)),
                                  subtitle: const Text('倾诉心事，获得温暖陪伴'),
                                  trailing: const Icon(Icons.chevron_right),
                                  onTap: () => Navigator.push(
                                      context,
                                      MaterialPageRoute(
                                          builder: (_) =>
                                              const LakeGodChatScreen())),
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
          ),
        ],
      ),
    );
  }
}

class _StatCard extends StatelessWidget {
  final String label;
  final String value;
  const _StatCard({required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Expanded(
      child: Card(
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 16),
          child: Column(
            children: [
              Text(value,
                  style: const TextStyle(
                      fontSize: 24,
                      fontWeight: FontWeight.bold,
                      color: AppTheme.skyBlue)),
              const SizedBox(height: 4),
              Text(label,
                  style: TextStyle(fontSize: 12, color: isDark ? const Color(0xFF9AA0A6) : Colors.grey)),
            ],
          ),
        ),
      ),
    );
  }
}
