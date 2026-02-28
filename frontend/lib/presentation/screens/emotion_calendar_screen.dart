/// 情绪日历页面
///
/// 按月展示每日情绪记录的日历视图。

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../data/datasources/user_service.dart';
import '../../data/datasources/websocket_manager.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../../utils/storage_util.dart';
import '../widgets/shimmer_loading.dart';
import '../widgets/privacy_badge.dart';
import 'emotion_heatmap_screen.dart';

/// 涟漪色彩范围：低分偏蓝、高分偏紫
const _kRippleColorLow = Color(0xFF90CAF9);
const _kRippleColorHigh = Color(0xFF7C4DFF);

/// 情绪日历页面
///
/// 以月历形式展示用户每日的情绪记录，核心功能：
/// - 按月切换，每个日期格子根据情绪分数着色（使用 [MoodColors] 映射）
/// - 点击有数据的日期弹出情绪详情 BottomSheet
/// - WebSocket 实时监听 new_stone / stone_deleted 事件，自动刷新当月数据
/// - 顶部统计卡片（愉悦/平静/低落天数 + 平均情绪指数）
/// - 本周心情趋势摘要
/// - 底部提供情绪热力图页面入口
class EmotionCalendarScreen extends StatefulWidget {
  const EmotionCalendarScreen({super.key});

  @override
  State<EmotionCalendarScreen> createState() => _EmotionCalendarScreenState();
}

/// 情绪日历页面的状态管理
///
/// 使用 [UserService] 获取月度情绪数据，通过 WebSocket 监听石头增删事件自动刷新。
/// 维护日历网格淡入动画和统计缓存，避免重复计算。
class _EmotionCalendarScreenState extends State<EmotionCalendarScreen>
    with SingleTickerProviderStateMixin {
  final UserService _userService = sl<UserService>();
  final WebSocketManager _wsManager = WebSocketManager();
  DateTime _currentMonth = DateTime.now();
  Map<String, dynamic> _emotionData = {};
  bool _isLoading = true;
  late AnimationController _animController;
  Map<String, dynamic>? _cachedStats;
  String? _currentUserId;
  late final void Function(Map<String, dynamic>) _onNewStoneListener;
  late final void Function(Map<String, dynamic>) _onStoneDeletedListener;
  late final void Function(Map<String, dynamic>) _onReconnectedListener;
  Timer? _refreshDebounce;
  int _refreshSeq = 0;

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(
        vsync: this, duration: const Duration(milliseconds: 800))
      ..forward();
    _onNewStoneListener = (payload) {
      if (!_isCurrentUserEvent(payload)) {
        return;
      }
      final now = DateTime.now();
      if (now.year == _currentMonth.year && now.month == _currentMonth.month) {
        _scheduleRealtimeRefresh(withFollowUp: true);
      }
    };
    _onStoneDeletedListener = (payload) {
      if (!_isCurrentUserEvent(payload)) return;
      _scheduleRealtimeRefresh();
    };
    _onReconnectedListener = (_) {
      _scheduleRealtimeRefresh(withFollowUp: true);
    };
    _initRealtimeSync();
    _loadEmotionData();
  }

  @override
  void dispose() {
    _wsManager.leaveRoom('lake');
    _wsManager.off('new_stone', _onNewStoneListener);
    _wsManager.off('stone_deleted', _onStoneDeletedListener);
    _wsManager.off('reconnected', _onReconnectedListener);
    _refreshDebounce?.cancel();
    _animController.dispose();
    super.dispose();
  }

  /// 初始化 WebSocket 实时同步：获取用户ID、加入 lake 房间、注册事件监听
  Future<void> _initRealtimeSync() async {
    _currentUserId = await StorageUtil.getUserId();
    if (!_wsManager.isConnected) {
      _wsManager.connect();
    }
    _wsManager.joinRoom('lake');
    _wsManager.on('new_stone', _onNewStoneListener);
    _wsManager.on('stone_deleted', _onStoneDeletedListener);
    _wsManager.on('reconnected', _onReconnectedListener);
  }

  /// 判断 WebSocket 事件是否由当前用户触发
  bool _isCurrentUserEvent(Map<String, dynamic> payload) {
    final currentUserId = _currentUserId;
    if (currentUserId == null || currentUserId.isEmpty) {
      return true;
    }
    final candidateIds = <String?>[
      payload['triggered_by']?.toString(),
      payload['user_id']?.toString(),
      payload['userId']?.toString(),
      (payload['stone'] as Map?)?['user_id']?.toString(),
      (payload['stone'] as Map?)?['userId']?.toString(),
      (payload['stone'] as Map?)?['author_id']?.toString(),
      (payload['stone'] as Map?)?['authorId']?.toString(),
    ];
    return candidateIds.any((id) => id != null && id == currentUserId);
  }

  /// 防抖刷新：300ms 内合并多次事件，[withFollowUp] 为 true 时 2s 后再补刷一次
  void _scheduleRealtimeRefresh({bool withFollowUp = false}) {
    _refreshDebounce?.cancel();
    _refreshDebounce = Timer(const Duration(milliseconds: 300), () {
      _loadEmotionData();
      if (!withFollowUp) return;
      final token = ++_refreshSeq;
      Future.delayed(const Duration(seconds: 2), () {
        if (!mounted || token != _refreshSeq) return;
        _loadEmotionData();
      });
    });
  }

  /// 从后端加载指定月份的情绪数据，成功后触发日历网格淡入动画
  Future<void> _loadEmotionData() async {
    if (mounted) setState(() => _isLoading = true);
    try {
      final year = _currentMonth.year;
      final month = _currentMonth.month;
      final result = await _userService.getEmotionCalendar(year, month);
      if (!mounted) return;
      if (result['success'] == true) {
        final rawDays = (result['data'] as Map<String, dynamic>?)?['days'];
        setState(() {
          _emotionData = _normalizeDays(rawDays);
          _cachedStats = null;
          _isLoading = false;
        });
        _animController.forward(from: 0);
      } else {
        setState(() {
          _emotionData = {};
          _cachedStats = null;
        });
      }
    } catch (e) {
      debugPrint('Load emotion data error: $e');
    }
    if (mounted) setState(() => _isLoading = false);
  }

  /// 标准化后端返回的日期数据，将 "2024-01-15" 格式的 key 转为纯天数 "15"
  Map<String, dynamic> _normalizeDays(dynamic rawDays) {
    if (rawDays is! Map) return {};
    final normalized = <String, dynamic>{};
    for (final entry in rawDays.entries) {
      final key = entry.key.toString();
      var dayKey = key;
      if (key.contains('-')) {
        final parts = key.split('-');
        if (parts.isNotEmpty) {
          final day = int.tryParse(parts.last);
          if (day != null && day > 0) {
            dayKey = '$day';
          }
        }
      }

      final value = entry.value;
      if (value is Map) {
        final dayData = Map<String, dynamic>.from(value);
        dayData['mood'] ??= _pickDominantMood(dayData['moods']);
        normalized[dayKey] = dayData;
      }
    }
    return normalized;
  }

  /// 从 moods 分布中选出出现次数最多的情绪类型
  String? _pickDominantMood(dynamic moods) {
    if (moods is! Map || moods.isEmpty) return null;
    String? winner;
    int maxCount = -1;
    for (final entry in moods.entries) {
      final count = int.tryParse(entry.value.toString()) ?? 0;
      if (count > maxCount) {
        winner = entry.key.toString();
        maxCount = count;
      }
    }
    return winner;
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              isDark
                  ? const Color(0xFF1A1A2E)
                  : AppTheme.skyBlue.withValues(alpha: 0.1),
              isDark ? const Color(0xFF1A1A2E) : Colors.white,
            ],
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              _buildHeader(),
              Expanded(
                child: SingleChildScrollView(
                  child: Column(
                    children: [
                      _buildMonthSelector(),
                      _buildEmotionSummary(),
                      const SizedBox(height: 8),
                      _buildWeekdayHeader(),
                      _isLoading
                          ? const SizedBox(
                              height: 300,
                              child: Center(
                                  child: WarmLoadingIndicator(messages: [
                                '正在加载你的心情轨迹...',
                                '回顾这段时间的情绪变化...',
                                '每一天都值得被记录...'
                              ])),
                            )
                          : SizedBox(
                              height: 300,
                              child: _buildCalendarGrid(),
                            ),
                      _buildWeeklySummary(),
                      _buildLegend(),
                      _buildHeatmapEntryCard(),
                      const SizedBox(height: 24),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  /// 构建页面顶部导航栏：返回按钮 + 标题 + 隐私徽章
  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.all(16),
      child: Row(
        children: [
          IconButton(
              icon: const Icon(Icons.arrow_back_ios),
              onPressed: () => Navigator.pop(context)),
          const Expanded(
              child: Text('情绪日历',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                  textAlign: TextAlign.center)),
          const PrivacyBadge(),
        ],
      ),
    );
  }

  /// 构建月份切换器，左右箭头切换月份，不允许超过当前月
  Widget _buildMonthSelector() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16),
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: isDark ? const Color(0xFF16213E) : Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
              color: isDark
                  ? Colors.transparent
                  : Colors.black.withValues(alpha: 0.05),
              blurRadius: 10)
        ],
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          IconButton(
            icon: const Icon(Icons.chevron_left, color: AppTheme.skyBlue),
            onPressed: () {
              setState(() => _currentMonth =
                  DateTime(_currentMonth.year, _currentMonth.month - 1));
              _loadEmotionData();
            },
          ),
          Text('${_currentMonth.year}年${_currentMonth.month}月',
              style:
                  const TextStyle(fontSize: 16, fontWeight: FontWeight.w600)),
          IconButton(
            icon: Icon(Icons.chevron_right,
                color: _canGoNext() ? AppTheme.skyBlue : Colors.grey.shade300),
            onPressed: _canGoNext()
                ? () {
                    setState(() => _currentMonth =
                        DateTime(_currentMonth.year, _currentMonth.month + 1));
                    _loadEmotionData();
                  }
                : null,
          ),
        ],
      ),
    );
  }

  /// 判断是否可以切换到下一个月（不超过当前月）
  bool _canGoNext() {
    final now = DateTime.now();
    return _currentMonth.year < now.year ||
        (_currentMonth.year == now.year && _currentMonth.month < now.month);
  }

  /// 构建月度情绪统计卡片：愉悦/平静/低落天数 + 平均情绪指数
  Widget _buildEmotionSummary() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final stats = _calculateStats();
    return Container(
      margin: const EdgeInsets.all(16),
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: isDark
              ? [const Color(0xFF16213E), const Color(0xFF1E2D3D)]
              : [AppTheme.skyBlue.withValues(alpha: 0.8), AppTheme.skyBlue],
        ),
        borderRadius: BorderRadius.circular(16),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          _statItem('${stats['happy']}', '愉悦',
              MoodColors.getConfig(MoodType.happy).icon),
          _statItem('${stats['calm']}', '平静',
              MoodColors.getConfig(MoodType.calm).icon),
          _statItem(
              '${stats['sad']}', '低落', MoodColors.getConfig(MoodType.sad).icon),
          _statItem(
              '${stats['avg'].toStringAsFixed(0)}%', '平均', Icons.analytics),
        ],
      ),
    );
  }

  /// 构建统计项：图标 + 数值 + 标签
  Widget _statItem(String value, String label, IconData icon) {
    return Column(
      children: [
        Icon(icon, color: Colors.white, size: 20),
        const SizedBox(height: 4),
        Text(value,
            style: const TextStyle(
                color: Colors.white,
                fontSize: 18,
                fontWeight: FontWeight.bold)),
        Text(label,
            style: TextStyle(
                color: Colors.white.withValues(alpha: 0.8), fontSize: 12)),
      ],
    );
  }

  /// 统计当月情绪数据：按分数阈值分类天数并计算平均值，结果缓存到 _cachedStats
  Map<String, dynamic> _calculateStats() {
    if (_cachedStats != null) return _cachedStats!;
    int happy = 0, calm = 0, sad = 0;
    double total = 0;
    int count = 0;
    for (var entry in _emotionData.entries) {
      final score = (entry.value['score'] ?? 0.5) as num;
      total += score;
      count++;
      if (score >= 0.7) {
        happy++;
      } else if (score >= 0.4) {
        calm++;
      } else {
        sad++;
      }
    }
    _cachedStats = {
      'happy': happy,
      'calm': calm,
      'sad': sad,
      'avg': count > 0 ? (total / count) * 100 : 50
    };
    return _cachedStats!;
  }

  /// 构建星期几表头行（日 ~ 六）
  Widget _buildWeekdayHeader() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    const weekdays = ['日', '一', '二', '三', '四', '五', '六'];
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 12),
      child: Row(
        children: weekdays
            .map((d) => Expanded(
                  child: Center(
                      child: Text(d,
                          style: TextStyle(
                              fontWeight: FontWeight.w600,
                              color: isDark
                                  ? Colors.white70
                                  : Colors.grey.shade600,
                              fontSize: 13))),
                ))
            .toList(),
      ),
    );
  }

  /// 构建日历网格，每个日期格子根据情绪分数着色，带交错淡入动画
  Widget _buildCalendarGrid() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final firstDay = DateTime(_currentMonth.year, _currentMonth.month, 1);
    final lastDay = DateTime(_currentMonth.year, _currentMonth.month + 1, 0);
    final startWeekday = firstDay.weekday % 7;
    final totalDays = lastDay.day;
    final today = DateTime.now();

    return AnimatedBuilder(
      animation: _animController,
      builder: (context, child) => GridView.builder(
        physics: const NeverScrollableScrollPhysics(),
        padding: const EdgeInsets.all(12),
        gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
            crossAxisCount: 7, mainAxisSpacing: 6, crossAxisSpacing: 6),
        itemCount: startWeekday + totalDays,
        itemBuilder: (context, index) {
          if (index < startWeekday) return const SizedBox();
          final day = index - startWeekday + 1;
          final dateKey = '$day';
          final emotion = _emotionData[dateKey];
          final score = (emotion?['score'] ?? 0.5) as num;
          final mood = emotion?['mood'] as String?;
          final isToday = _currentMonth.year == today.year &&
              _currentMonth.month == today.month &&
              day == today.day;
          final moodType = mood != null
              ? MoodColors.fromString(mood)
              : MoodColors.fromSentimentScore(score.toDouble());
          final config = MoodColors.getConfig(moodType);
          final hasData = emotion != null;

          final rippleColor = hasData
              ? Color.lerp(
                  _kRippleColorLow, _kRippleColorHigh, score.toDouble())!
              : (isDark ? Colors.grey.shade700 : Colors.grey.shade300);
          return FadeTransition(
            opacity: Tween<double>(begin: 0, end: 1).animate(
              CurvedAnimation(
                  parent: _animController,
                  curve: Interval((index - startWeekday) / totalDays * 0.5,
                      0.5 + (index - startWeekday) / totalDays * 0.5,
                      curve: Curves.easeOut)),
            ),
            child: Material(
              color: Colors.transparent,
              child: InkWell(
                onTap: hasData
                    ? () {
                        HapticFeedback.lightImpact();
                        _showDayDetail(day, emotion);
                      }
                    : null,
                splashColor: rippleColor.withValues(alpha: 0.4),
                highlightColor: rippleColor.withValues(alpha: 0.2),
                borderRadius: BorderRadius.circular(10),
                child: Ink(
                  decoration: BoxDecoration(
                    gradient: hasData
                        ? LinearGradient(
                            colors: [config.gradientStart, config.gradientEnd],
                            begin: Alignment.topLeft,
                            end: Alignment.bottomRight)
                        : null,
                    color: hasData
                        ? null
                        : (isDark
                            ? const Color(0xFF1E2D3D)
                            : Colors.grey.shade100),
                    borderRadius: BorderRadius.circular(10),
                    border: isToday
                        ? Border.all(color: AppTheme.skyBlue, width: 2)
                        : null,
                    boxShadow: hasData
                        ? [
                            BoxShadow(
                                color: config.primary.withValues(alpha: 0.3),
                                blurRadius: 4,
                                offset: const Offset(0, 2))
                          ]
                        : null,
                  ),
                  child: Stack(
                    alignment: Alignment.center,
                    children: [
                      Text('$day',
                          style: TextStyle(
                              color: hasData
                                  ? config.textColor
                                  : (isDark
                                      ? Colors.white54
                                      : Colors.grey.shade400),
                              fontWeight:
                                  isToday ? FontWeight.bold : FontWeight.w500,
                              fontSize: 14)),
                      if (hasData)
                        Positioned(
                            bottom: 4,
                            child: Icon(config.icon,
                                size: 10,
                                color:
                                    config.iconColor.withValues(alpha: 0.7))),
                    ],
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }

  /// 弹出指定日期的情绪详情 BottomSheet，展示情绪图标、名称、描述和指数
  void _showDayDetail(int day, Map<String, dynamic> emotion) {
    final score = (emotion['score'] ?? 0.5) as num;
    final mood = emotion['mood'] as String?;
    final moodType = mood != null
        ? MoodColors.fromString(mood)
        : MoodColors.fromSentimentScore(score.toDouble());
    final config = MoodColors.getConfig(moodType);

    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) => Container(
        padding: const EdgeInsets.all(24),
        decoration: BoxDecoration(
          gradient: LinearGradient(
              colors: [config.gradientStart, config.gradientEnd],
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter),
          borderRadius: const BorderRadius.vertical(top: Radius.circular(24)),
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Container(
                width: 40,
                height: 4,
                decoration: BoxDecoration(
                    color: config.textColor.withValues(alpha: 0.3),
                    borderRadius: BorderRadius.circular(2))),
            const SizedBox(height: 20),
            Icon(config.icon, size: 48, color: config.iconColor),
            const SizedBox(height: 12),
            Text('${_currentMonth.month}月$day日',
                style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                    color: config.textColor)),
            const SizedBox(height: 8),
            Text(config.name,
                style: TextStyle(
                    fontSize: 24,
                    fontWeight: FontWeight.w600,
                    color: config.primary)),
            const SizedBox(height: 4),
            Text(config.description,
                style: TextStyle(
                    fontSize: 14,
                    color: config.textColor.withValues(alpha: 0.7))),
            const SizedBox(height: 16),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              decoration: BoxDecoration(
                  color: config.cardColor,
                  borderRadius: BorderRadius.circular(20)),
              child: Text('情绪指数: ${(score * 100).toInt()}%',
                  style: TextStyle(
                      color: config.textColor, fontWeight: FontWeight.w500)),
            ),
            const SizedBox(height: 24),
          ],
        ),
      ),
    );
  }

  /// 构建本周心情趋势摘要条，根据周均分显示趋势方向和文案
  Widget _buildWeeklySummary() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final now = DateTime.now();
    final weekStart = now.subtract(Duration(days: now.weekday % 7));
    double weekTotal = 0;
    int weekCount = 0;
    for (int i = 0; i < 7; i++) {
      final day = weekStart.add(Duration(days: i));
      if (day.month == _currentMonth.month) {
        final data = _emotionData['${day.day}'];
        if (data != null) {
          weekTotal += (data['score'] ?? 0.5) as num;
          weekCount++;
        }
      }
    }
    if (weekCount == 0) return const SizedBox.shrink();
    final weekAvg = weekTotal / weekCount;
    final trend = weekAvg >= 0.6 ? '↑' : (weekAvg <= 0.4 ? '↓' : '→');
    final trendText =
        weekAvg >= 0.6 ? '湖光明媚' : (weekAvg <= 0.4 ? '湖水轻抚' : '波澜不惊');
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      decoration: BoxDecoration(
        color: isDark
            ? const Color(0xFF1E2D3D)
            : AppTheme.secondaryColor.withValues(alpha: 0.1),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Icon(Icons.water_drop_outlined,
              size: 16, color: AppTheme.secondaryColor),
          const SizedBox(width: 8),
          Text('本周心情: $trendText $trend',
              style: const TextStyle(
                  fontSize: 13,
                  color: AppTheme.secondaryColor,
                  fontWeight: FontWeight.w500)),
        ],
      ),
    );
  }

  /// 构建情绪颜色图例（愉悦/平静/低落/焦虑）
  Widget _buildLegend() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final moods = [
      MoodType.happy,
      MoodType.calm,
      MoodType.sad,
      MoodType.anxious
    ];
    return Container(
      padding: const EdgeInsets.all(16),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: moods.map((m) {
          final config = MoodColors.getConfig(m);
          return Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8),
            child: Row(
              children: [
                Container(
                  width: 14,
                  height: 14,
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                        colors: [config.gradientStart, config.gradientEnd]),
                    borderRadius: BorderRadius.circular(4),
                  ),
                ),
                const SizedBox(width: 4),
                Text(config.name,
                    style: TextStyle(
                        fontSize: 11,
                        color: isDark ? Colors.white70 : Colors.grey)),
              ],
            ),
          );
        }).toList(),
      ),
    );
  }

  /// 构建情绪热力图页面入口卡片，点击跳转到 [EmotionHeatmapScreen]
  Widget _buildHeatmapEntryCard() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Material(
        color: isDark ? const Color(0xFF16213E) : Colors.white,
        borderRadius: BorderRadius.circular(14),
        child: InkWell(
          borderRadius: BorderRadius.circular(14),
          onTap: () => Navigator.push(
            context,
            MaterialPageRoute(builder: (_) => const EmotionHeatmapScreen()),
          ),
          child: Padding(
            padding: const EdgeInsets.all(14),
            child: Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                    color: _kRippleColorLow.withValues(alpha: 0.15),
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: const Icon(Icons.grid_view_rounded,
                      color: _kRippleColorHigh),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        '查看情绪热力图',
                        style: TextStyle(
                            fontSize: 15,
                            fontWeight: FontWeight.w700,
                            color:
                                isDark ? Colors.white : AppTheme.textPrimary),
                      ),
                      const SizedBox(height: 2),
                      Text(
                        '热力图已独立页面，避免和月历混在一起',
                        style: TextStyle(
                            fontSize: 12,
                            color: isDark
                                ? Colors.white70
                                : AppTheme.textSecondary),
                      ),
                    ],
                  ),
                ),
                Icon(Icons.chevron_right,
                    color: isDark ? Colors.white54 : AppTheme.textTertiary),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
