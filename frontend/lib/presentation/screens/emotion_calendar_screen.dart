// @file emotion_calendar_screen.dart
// @brief 情绪日历页面 - 精美可视化
// Created by 林子怡

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:dio/dio.dart';
import '../../data/datasources/api_client.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import '../widgets/shimmer_loading.dart';
import '../widgets/emotion_heatmap.dart';
import '../widgets/emotion_insights_card.dart';
import '../widgets/privacy_badge.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';

const _kRippleColorLow = Color(0xFF90CAF9);
const _kRippleColorHigh = Color(0xFF7C4DFF);

class EmotionCalendarScreen extends StatefulWidget {
  const EmotionCalendarScreen({super.key});

  @override
  State<EmotionCalendarScreen> createState() => _EmotionCalendarScreenState();
}

class _EmotionCalendarScreenState extends State<EmotionCalendarScreen> with SingleTickerProviderStateMixin {
  final ApiClient _apiClient = ApiClient();
  final AIRecommendationService _aiService = AIRecommendationService();
  DateTime _currentMonth = DateTime.now();
  Map<String, dynamic> _emotionData = {};
  bool _isLoading = true;
  late AnimationController _animController;
  Map<String, dynamic>? _cachedStats;
  Map<String, Map<String, dynamic>> _heatmapData = {};
  List<String> _insights = [];
  Map<String, dynamic> _emotionTrends = {};
  bool _trendsLoading = false;

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(vsync: this, duration: const Duration(milliseconds: 800))..forward();
    _loadEmotionData();
    _loadHeatmapData();
    _loadEmotionTrends();
  }

  @override
  void dispose() {
    _animController.dispose();
    super.dispose();
  }

  Future<void> _loadEmotionData() async {
    setState(() => _isLoading = true);
    try {
      final year = _currentMonth.year;
      final month = _currentMonth.month;
      final response = await _apiClient.get('/users/my/emotion-calendar?year=$year&month=$month');
      if (response.statusCode == 200 && response.data['code'] == 0) {
        setState(() {
          _emotionData = Map<String, dynamic>.from(response.data['data']?['days'] ?? {});
          _cachedStats = null;
          _isLoading = false;
        });
        _animController.forward(from: 0);
      } else {
        // 非200或code!=0时，清空数据显示空日历
        setState(() {
          _emotionData = {};
          _cachedStats = null;
        });
      }
    } on DioException catch (e) {
      // 404 表示新用户无数据，优雅降级为空日历而非报错
      if (e.response?.statusCode == 404) {
        if (mounted) {
          setState(() {
            _emotionData = {};
            _cachedStats = null;
          });
        }
      } else {
        debugPrint('Load emotion data error: $e');
      }
    } catch (e) {
      debugPrint('Load emotion data error: $e');
    }
    if (mounted) setState(() => _isLoading = false);
  }

  Future<void> _loadHeatmapData() async {
    try {
      final response = await _apiClient.get('/users/my/emotion-heatmap');
      if (response.statusCode == 200 && response.data['code'] == 0) {
        final rawData = response.data['data']?['days'] as Map<String, dynamic>? ?? {};
        final parsed = <String, Map<String, dynamic>>{};
        for (final entry in rawData.entries) {
          if (entry.value is Map) {
            parsed[entry.key] = Map<String, dynamic>.from(entry.value as Map);
          }
        }
        if (mounted) {
          setState(() {
            _heatmapData = parsed;
            _insights = _generateInsights(parsed);
          });
        }
      }
    } catch (e) {
      // 热力图加载失败不影响主日历，静默降级
      debugPrint('Load heatmap data error: $e');
      if (mounted) {
        setState(() => _insights = _generateInsights(_heatmapData));
      }
    }
  }

  /// 加载AI情绪趋势分析
  Future<void> _loadEmotionTrends() async {
    setState(() => _trendsLoading = true);
    try {
      final trends = await _aiService.getEmotionTrends();
      if (mounted) setState(() => _emotionTrends = trends);
    } catch (_) {}
    if (mounted) setState(() => _trendsLoading = false);
  }

  List<String> _generateInsights(Map<String, Map<String, dynamic>> data) {
    if (data.isEmpty) return [];
    final insights = <String>[];
    final now = DateTime.now();

    // 最近7天趋势
    double recentTotal = 0;
    int recentCount = 0;
    for (int i = 0; i < 7; i++) {
      final date = now.subtract(Duration(days: i));
      final key = '${date.year}-${date.month.toString().padLeft(2, '0')}-${date.day.toString().padLeft(2, '0')}';
      final dayData = data[key];
      if (dayData != null) {
        recentTotal += (dayData['score'] ?? 0.5) as num;
        recentCount++;
      }
    }
    if (recentCount >= 3) {
      final avg = recentTotal / recentCount;
      if (avg >= 0.65) {
        insights.add('你最近7天的情绪呈上升趋势 ↑');
      } else if (avg <= 0.35) {
        insights.add('最近情绪有些低落，记得照顾好自己 ♡');
      } else {
        insights.add('你最近7天的情绪比较平稳 →');
      }
    }

    // 连续积极天数
    int streak = 0;
    for (int i = 0; i < 30; i++) {
      final date = now.subtract(Duration(days: i));
      final key = '${date.year}-${date.month.toString().padLeft(2, '0')}-${date.day.toString().padLeft(2, '0')}';
      final dayData = data[key];
      if (dayData != null && (dayData['score'] as num? ?? 0) >= 0.6) {
        streak++;
      } else {
        break;
      }
    }
    if (streak >= 2) {
      insights.add('你已经连续$streak天保持积极情绪');
    }

    // 最佳星期几
    final weekdayScores = <int, List<double>>{};
    for (final entry in data.entries) {
      try {
        final date = DateTime.parse(entry.key);
        final score = (entry.value['score'] ?? 0.5) as num;
        weekdayScores.putIfAbsent(date.weekday, () => []).add(score.toDouble());
      } catch (_) {}
    }
    if (weekdayScores.isNotEmpty) {
      double bestAvg = -1;
      int bestDay = 1;
      for (final entry in weekdayScores.entries) {
        final avg = entry.value.reduce((a, b) => a + b) / entry.value.length;
        if (avg > bestAvg) {
          bestAvg = avg;
          bestDay = entry.key;
        }
      }
      const dayNames = ['', '周一', '周二', '周三', '周四', '周五', '周六', '周日'];
      insights.add('你在${dayNames[bestDay]}通常心情最好');
    }

    return insights;
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      body: SafeArea(
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
                              child: Center(child: WarmLoadingIndicator(messages: ['正在加载你的心情轨迹...', '回顾这段时间的情绪变化...', '每一天都值得被记录...'])),
                            )
                          : SizedBox(
                              height: 300,
                              child: _buildCalendarGrid(),
                            ),
                      _buildWeeklySummary(),
                      _buildLegend(),
                      // 热力图
                      Padding(
                        padding: const EdgeInsets.symmetric(horizontal: 16),
                        child: EmotionHeatmap(
                          data: _heatmapData,
                          onDayTap: (date, data) {
                            // 可选：点击热力图某天时的处理
                          },
                        ),
                      ),
                      const SizedBox(height: 16),
                      // 情绪洞察
                      if (_insights.isNotEmpty)
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 16),
                          child: EmotionInsightsCard(insights: _insights),
                        ),
                      const SizedBox(height: 16),
                      // AI 情绪趋势
                      if (_trendsLoading)
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 16),
                          child: Container(
                            padding: const EdgeInsets.all(20),
                            decoration: BoxDecoration(
                              color: Colors.white.withValues(alpha: 0.06),
                              borderRadius: BorderRadius.circular(16),
                              border: Border.all(color: Colors.white.withValues(alpha: 0.08)),
                            ),
                            child: Center(
                              child: Row(mainAxisSize: MainAxisSize.min, children: [
                                SizedBox(width: 16, height: 16,
                                  child: CircularProgressIndicator(
                                    strokeWidth: 2,
                                    color: Colors.white.withValues(alpha: 0.5))),
                                const SizedBox(width: 12),
                                Text('AI 正在分析情绪趋势...',
                                  style: TextStyle(fontSize: 13,
                                    color: Colors.white.withValues(alpha: 0.6))),
                              ]),
                            ),
                          ),
                        )
                      else if (_emotionTrends.isNotEmpty)
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 16),
                          child: Container(
                            padding: const EdgeInsets.all(16),
                            decoration: BoxDecoration(
                              gradient: LinearGradient(
                                colors: [
                                  const Color(0xFFB39DDB).withValues(alpha: 0.12),
                                  const Color(0xFF64B5F6).withValues(alpha: 0.08),
                                ],
                                begin: Alignment.topLeft,
                                end: Alignment.bottomRight,
                              ),
                              borderRadius: BorderRadius.circular(16),
                              border: Border.all(color: Colors.white.withValues(alpha: 0.1)),
                            ),
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Row(children: [
                                  Icon(Icons.auto_awesome, size: 16,
                                    color: Colors.white.withValues(alpha: 0.7)),
                                  const SizedBox(width: 8),
                                  Text('AI 情绪趋势',
                                    style: TextStyle(fontSize: 14, fontWeight: FontWeight.w600,
                                      color: Colors.white.withValues(alpha: 0.85))),
                                ]),
                                const SizedBox(height: 12),
                                if (_emotionTrends['summary'] != null)
                                  Text(_emotionTrends['summary'] as String,
                                    style: TextStyle(fontSize: 13, height: 1.6,
                                      color: Colors.white.withValues(alpha: 0.7))),
                                if (_emotionTrends['suggestions'] is List) ...[
                                  const SizedBox(height: 10),
                                  ...(_emotionTrends['suggestions'] as List).take(3).map((s) =>
                                    Padding(
                                      padding: const EdgeInsets.only(bottom: 6),
                                      child: Row(crossAxisAlignment: CrossAxisAlignment.start, children: [
                                        Padding(
                                          padding: const EdgeInsets.only(top: 4),
                                          child: Container(width: 5, height: 5,
                                            decoration: BoxDecoration(
                                              shape: BoxShape.circle,
                                              color: const Color(0xFFB39DDB).withValues(alpha: 0.6))),
                                        ),
                                        const SizedBox(width: 8),
                                        Expanded(child: Text(s.toString(),
                                          style: TextStyle(fontSize: 12, height: 1.5,
                                            color: Colors.white.withValues(alpha: 0.6)))),
                                      ]),
                                    ),
                                  ),
                                ],
                              ],
                            ),
                          ),
                        ),
                      const SizedBox(height: 24),
                    ],
                  ),
                ),
              ),
            ],
          ),
      ),
    );
  }

  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.all(16),
      child: Row(
        children: [
          IconButton(icon: const Icon(Icons.arrow_back_ios), onPressed: () => Navigator.pop(context)),
          const Expanded(child: Text('情绪日历', style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold), textAlign: TextAlign.center)),
          const PrivacyBadge(),
        ],
      ),
    );
  }

  Widget _buildMonthSelector() {
    return SkyGlassCard(
      borderRadius: 20,
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 16),
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
        child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          IconButton(
            icon: const Icon(Icons.chevron_left, color: AppTheme.skyBlue),
            onPressed: () {
              setState(() => _currentMonth = DateTime(_currentMonth.year, _currentMonth.month - 1));
              _loadEmotionData();
            },
          ),
          Text('${_currentMonth.year}年${_currentMonth.month}月', style: const TextStyle(fontSize: 16, fontWeight: FontWeight.w600)),
          IconButton(
            icon: Icon(Icons.chevron_right, color: _canGoNext() ? AppTheme.skyBlue : Colors.grey.shade300),
            onPressed: _canGoNext() ? () {
              setState(() => _currentMonth = DateTime(_currentMonth.year, _currentMonth.month + 1));
              _loadEmotionData();
            } : null,
          ),
        ],
      ),
      ),
    );
  }

  bool _canGoNext() {
    final now = DateTime.now();
    return _currentMonth.year < now.year || (_currentMonth.year == now.year && _currentMonth.month < now.month);
  }

  Widget _buildEmotionSummary() {
    final stats = _calculateStats();
    return SkyGlassCard(
      borderRadius: 20,
      child: Container(
        margin: const EdgeInsets.all(16),
        padding: const EdgeInsets.all(16),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: [
            _statItem('${stats['happy']}', '愉悦', MoodColors.getConfig(MoodType.happy).icon),
            _statItem('${stats['calm']}', '平静', MoodColors.getConfig(MoodType.calm).icon),
            _statItem('${stats['sad']}', '低落', MoodColors.getConfig(MoodType.sad).icon),
            _statItem('${stats['avg'].toStringAsFixed(0)}%', '平均', Icons.analytics),
          ],
        ),
      ),
    );
  }

  Widget _statItem(String value, String label, IconData icon) {
    return Column(
      children: [
        Icon(icon, color: Colors.white, size: 20),
        const SizedBox(height: 4),
        Text(value, style: const TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold)),
        Text(label, style: TextStyle(color: Colors.white.withValues(alpha: 0.8), fontSize: 12)),
      ],
    );
  }

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
    _cachedStats = {'happy': happy, 'calm': calm, 'sad': sad, 'avg': count > 0 ? (total / count) * 100 : 50};
    return _cachedStats!;
  }

  Widget _buildWeekdayHeader() {
    const weekdays = ['日', '一', '二', '三', '四', '五', '六'];
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 12),
      child: Row(
        children: weekdays.map((d) => Expanded(
          child: Center(child: Text(d, style: TextStyle(fontWeight: FontWeight.w600, color: Colors.grey.shade600, fontSize: 13))),
        )).toList(),
      ),
    );
  }

  Widget _buildCalendarGrid() {
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
        gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(crossAxisCount: 7, mainAxisSpacing: 6, crossAxisSpacing: 6),
        itemCount: startWeekday + totalDays,
        itemBuilder: (context, index) {
          if (index < startWeekday) return const SizedBox();
          final day = index - startWeekday + 1;
          final dateKey = '$day';
          final emotion = _emotionData[dateKey];
          final score = (emotion?['score'] ?? 0.5) as num;
          final mood = emotion?['mood'] as String?;
          final isToday = _currentMonth.year == today.year && _currentMonth.month == today.month && day == today.day;
          final moodType = mood != null ? MoodColors.fromString(mood) : MoodColors.fromSentimentScore(score.toDouble());
          final config = MoodColors.getConfig(moodType);
          final hasData = emotion != null;

          final rippleColor = hasData ? Color.lerp(_kRippleColorLow, _kRippleColorHigh, score.toDouble())! : Colors.grey.shade300;
          return FadeTransition(
            opacity: Tween<double>(begin: 0, end: 1).animate(
              CurvedAnimation(parent: _animController, curve: Interval((index - startWeekday) / totalDays * 0.5, 0.5 + (index - startWeekday) / totalDays * 0.5, curve: Curves.easeOut)),
            ),
            child: Material(
              color: Colors.transparent,
              child: InkWell(
                onTap: hasData ? () { HapticFeedback.lightImpact(); _showDayDetail(day, emotion); } : null,
                splashColor: rippleColor.withValues(alpha: 0.4),
                highlightColor: rippleColor.withValues(alpha: 0.2),
                borderRadius: BorderRadius.circular(10),
                child: Ink(
                  decoration: BoxDecoration(
                    gradient: hasData ? LinearGradient(colors: [config.gradientStart, config.gradientEnd], begin: Alignment.topLeft, end: Alignment.bottomRight) : null,
                    color: hasData ? null : Colors.grey.shade100,
                    borderRadius: BorderRadius.circular(10),
                    border: isToday ? Border.all(color: AppTheme.skyBlue, width: 2) : null,
                    boxShadow: hasData ? [BoxShadow(color: config.primary.withValues(alpha: 0.3), blurRadius: 4, offset: const Offset(0, 2))] : null,
                  ),
                  child: Stack(
                    alignment: Alignment.center,
                    children: [
                      Text('$day', style: TextStyle(color: hasData ? config.textColor : Colors.grey.shade400, fontWeight: isToday ? FontWeight.bold : FontWeight.w500, fontSize: 14)),
                      if (hasData) Positioned(bottom: 4, child: Icon(config.icon, size: 10, color: config.iconColor.withValues(alpha: 0.7))),
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

  void _showDayDetail(int day, Map<String, dynamic> emotion) {
    final score = (emotion['score'] ?? 0.5) as num;
    final mood = emotion['mood'] as String?;
    final moodType = mood != null ? MoodColors.fromString(mood) : MoodColors.fromSentimentScore(score.toDouble());
    final config = MoodColors.getConfig(moodType);

    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) => Container(
        padding: const EdgeInsets.all(24),
        decoration: BoxDecoration(
          gradient: LinearGradient(colors: [config.gradientStart, config.gradientEnd], begin: Alignment.topCenter, end: Alignment.bottomCenter),
          borderRadius: const BorderRadius.vertical(top: Radius.circular(24)),
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Container(width: 40, height: 4, decoration: BoxDecoration(color: config.textColor.withValues(alpha: 0.3), borderRadius: BorderRadius.circular(2))),
            const SizedBox(height: 20),
            Icon(config.icon, size: 48, color: config.iconColor),
            const SizedBox(height: 12),
            Text('${_currentMonth.month}月$day日', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: config.textColor)),
            const SizedBox(height: 8),
            Text(config.name, style: TextStyle(fontSize: 24, fontWeight: FontWeight.w600, color: config.primary)),
            const SizedBox(height: 4),
            Text(config.description, style: TextStyle(fontSize: 14, color: config.textColor.withValues(alpha: 0.7))),
            const SizedBox(height: 16),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              decoration: BoxDecoration(color: config.cardColor, borderRadius: BorderRadius.circular(20)),
              child: Text('情绪指数: ${(score * 100).toInt()}%', style: TextStyle(color: config.textColor, fontWeight: FontWeight.w500)),
            ),
            const SizedBox(height: 24),
          ],
        ),
      ),
    );
  }

  Widget _buildWeeklySummary() {
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
    final trendText = weekAvg >= 0.6 ? '湖光明媚' : (weekAvg <= 0.4 ? '湖水轻抚' : '波澜不惊');
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      decoration: BoxDecoration(color: AppTheme.secondaryColor.withValues(alpha: 0.1), borderRadius: BorderRadius.circular(12)),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Icon(Icons.water_drop_outlined, size: 16, color: AppTheme.secondaryColor),
          const SizedBox(width: 8),
          Text('本周心情: $trendText $trend', style: const TextStyle(fontSize: 13, color: AppTheme.secondaryColor, fontWeight: FontWeight.w500)),
        ],
      ),
    );
  }

  Widget _buildLegend() {
    final moods = [MoodType.happy, MoodType.calm, MoodType.sad, MoodType.anxious];
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
                  width: 14, height: 14,
                  decoration: BoxDecoration(
                    gradient: LinearGradient(colors: [config.gradientStart, config.gradientEnd]),
                    borderRadius: BorderRadius.circular(4),
                  ),
                ),
                const SizedBox(width: 4),
                Text(config.name, style: const TextStyle(fontSize: 11, color: Colors.grey)),
              ],
            ),
          );
        }).toList(),
      ),
    );
  }
}
