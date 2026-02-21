// @file emotion_trend_chart.dart
// @brief 情绪趋势图表组件
// Created by 吴睿璐

library;
import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import '../../data/datasources/recommendation_service.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import 'package:intl/intl.dart';

class EmotionTrendChart extends StatefulWidget {
  final List<EmotionTrendPoint> trends;

  const EmotionTrendChart({
    super.key,
    required this.trends,
  });

  @override
  State<EmotionTrendChart> createState() => _EmotionTrendChartState();
}

class _EmotionTrendChartState extends State<EmotionTrendChart> {
  int? _selectedIndex;
  late List<FlSpot> _spots;
  late Map<String, int> _moodCounts;
  late double _avgScore;
  late int _totalStones;
  late int _totalInteractions;

  @override
  void initState() {
    super.initState();
    _computeData();
  }

  @override
  void didUpdateWidget(EmotionTrendChart oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.trends != widget.trends) _computeData();
  }

  void _computeData() {
    _spots = List.generate(widget.trends.length, (i) => FlSpot(i.toDouble(), widget.trends[i].emotionScore));
    _moodCounts = {};
    double sum = 0;
    _totalStones = 0;
    _totalInteractions = 0;
    for (final t in widget.trends) {
      _moodCounts[t.mood] = (_moodCounts[t.mood] ?? 0) + 1;
      sum += t.emotionScore;
      _totalStones += t.stoneCount;
      _totalInteractions += t.interactionCount;
    }
    _avgScore = widget.trends.isEmpty ? 0 : sum / widget.trends.length;
  }

  @override
  Widget build(BuildContext context) {
    if (widget.trends.isEmpty) return _buildEmptyState();
    return Column(
      children: [
        _buildChart(),
        const SizedBox(height: 16),
        _buildTrendHint(),
        const SizedBox(height: 16),
        _buildLegend(),
        const SizedBox(height: 24),
        _buildStatistics(),
      ],
    );
  }

  Widget _buildTrendHint() {
    if (widget.trends.length < 3) return const SizedBox.shrink();
    final recent = widget.trends.length >= 7 ? widget.trends.sublist(widget.trends.length - 7) : widget.trends;
    final recentAvg = recent.map((t) => t.emotionScore).reduce((a, b) => a + b) / recent.length;
    final firstHalf = recent.sublist(0, recent.length ~/ 2);
    final secondHalf = recent.sublist(recent.length ~/ 2);
    final firstAvg = firstHalf.map((t) => t.emotionScore).reduce((a, b) => a + b) / firstHalf.length;
    final secondAvg = secondHalf.map((t) => t.emotionScore).reduce((a, b) => a + b) / secondHalf.length;
    final trend = secondAvg - firstAvg;
    String hint;
    IconData icon;
    Color color;
    if (trend > 0.1) {
      hint = '心情不错，继续保持~';
      icon = Icons.trending_up;
      color = AppTheme.successColor;
    } else if (trend < -0.1) {
      hint = '最近有点低落，记得照顾自己';
      icon = Icons.trending_down;
      color = AppTheme.primaryColor;
    } else {
      hint = recentAvg >= 0.3 ? '心情平稳，挺好的' : '有点累了吧，休息一下';
      icon = Icons.trending_flat;
      color = AppTheme.secondaryColor;
    }
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      decoration: BoxDecoration(color: color.withValues(alpha: 0.1), borderRadius: BorderRadius.circular(12)),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(icon, size: 18, color: color),
          const SizedBox(width: 8),
          Text(hint, style: TextStyle(fontSize: 13, color: color, fontWeight: FontWeight.w500)),
        ],
      ),
    );
  }

  Widget _buildChart() {
    return Container(
      height: 300,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.05),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: LineChart(
        _buildLineChartData(),
        duration: const Duration(milliseconds: 250),
      ),
    );
  }

  LineChartData _buildLineChartData() {
    return LineChartData(
      gridData: FlGridData(
        show: true,
        drawVerticalLine: false,
        horizontalInterval: 0.25,
        getDrawingHorizontalLine: (value) {
          return FlLine(
            color: Colors.grey.shade200,
            strokeWidth: 1,
          );
        },
      ),
      titlesData: FlTitlesData(
        show: true,
        rightTitles: const AxisTitles(
          sideTitles: SideTitles(showTitles: false),
        ),
        topTitles: const AxisTitles(
          sideTitles: SideTitles(showTitles: false),
        ),
        bottomTitles: AxisTitles(
          sideTitles: SideTitles(
            showTitles: true,
            reservedSize: 30,
            interval: widget.trends.length > 15 ? 7 : 3,
            getTitlesWidget: (value, meta) {
              final index = value.toInt();
              if (index < 0 || index >= widget.trends.length) {
                return const SizedBox.shrink();
              }

              final date = widget.trends[index].date;
              return Padding(
                padding: const EdgeInsets.only(top: 8),
                child: Text(
                  DateFormat('M/d').format(date),
                  style: const TextStyle(
                    color: AppTheme.textSecondary,
                    fontSize: 10,
                  ),
                ),
              );
            },
          ),
        ),
        leftTitles: AxisTitles(
          sideTitles: SideTitles(
            showTitles: true,
            interval: 0.5,
            reservedSize: 40,
            getTitlesWidget: (value, meta) {
              return Text(
                _getEmotionLabel(value),
                style: const TextStyle(
                  color: AppTheme.textSecondary,
                  fontSize: 10,
                ),
              );
            },
          ),
        ),
      ),
      borderData: FlBorderData(
        show: true,
        border: Border(
          bottom: BorderSide(color: Colors.grey.shade300, width: 1),
          left: BorderSide(color: Colors.grey.shade300, width: 1),
        ),
      ),
      minX: 0,
      maxX: (widget.trends.length - 1).toDouble(),
      minY: -1,
      maxY: 1,
      lineBarsData: [
        LineChartBarData(
          spots: _spots,
          isCurved: true,
          curveSmoothness: 0.45,
          gradient: const LinearGradient(
            colors: [
              AppTheme.lakeSurface,
              AppTheme.secondaryColor,
            ],
          ),
          barWidth: 3.5,
          isStrokeCapRound: true,
          dotData: FlDotData(
            show: true,
            getDotPainter: (spot, percent, barData, index) {
              final moodConfig = MoodColors.getConfig(
                MoodColors.fromString(widget.trends[index].mood),
              );

              return FlDotCirclePainter(
                radius: _selectedIndex == index ? 6 : 4,
                color: moodConfig.primary,
                strokeWidth: 2,
                strokeColor: Colors.white,
              );
            },
          ),
          belowBarData: BarAreaData(
            show: true,
            gradient: LinearGradient(
              colors: [
                AppTheme.lakeSurface.withValues(alpha: 0.25),
                AppTheme.secondaryColor.withValues(alpha: 0.08),
              ],
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
            ),
          ),
        ),
      ],
      lineTouchData: LineTouchData(
        enabled: true,
        touchCallback: (FlTouchEvent event, LineTouchResponse? response) {
          if (response == null || response.lineBarSpots == null) {
            setState(() {
              _selectedIndex = null;
            });
            return;
          }

          if (event is FlTapUpEvent || event is FlLongPressEnd) {
            setState(() {
              _selectedIndex = null;
            });
            return;
          }

          final spot = response.lineBarSpots!.first;
          setState(() {
            _selectedIndex = spot.x.toInt();
          });
        },
        touchTooltipData: LineTouchTooltipData(
          tooltipRoundedRadius: 12,
          tooltipPadding: const EdgeInsets.all(12),
          tooltipMargin: 8,
          getTooltipItems: (List<LineBarSpot> touchedSpots) {
            return touchedSpots.map((spot) {
              final index = spot.x.toInt();
              if (index < 0 || index >= widget.trends.length) {
                return null;
              }

              final trend = widget.trends[index];
              final moodConfig = MoodColors.getConfig(
                MoodColors.fromString(trend.mood),
              );

              return LineTooltipItem(
                '',
                const TextStyle(),
                children: [
                  TextSpan(
                    text: DateFormat('M月d日').format(trend.date),
                    style: const TextStyle(
                      color: AppTheme.textPrimary,
                      fontWeight: FontWeight.bold,
                      fontSize: 13,
                    ),
                  ),
                  TextSpan(
                    text: '\n${moodConfig.name}',
                    style: TextStyle(
                      color: moodConfig.primary,
                      fontWeight: FontWeight.w600,
                      fontSize: 12,
                    ),
                  ),
                  TextSpan(
                    text: '\n情绪分数: ${(trend.emotionScore * 100).toInt()}',
                    style: const TextStyle(
                      color: AppTheme.textSecondary,
                      fontSize: 11,
                    ),
                  ),
                  TextSpan(
                    text: '\n${trend.stoneCount} 颗石头',
                    style: const TextStyle(
                      color: AppTheme.textSecondary,
                      fontSize: 11,
                    ),
                  ),
                ],
              );
            }).toList();
          },
        ),
      ),
    );
  }

  String _getEmotionLabel(double value) {
    if (value >= 0.75) return '很好';
    if (value >= 0.25) return '较好';
    if (value >= -0.25) return '一般';
    if (value >= -0.75) return '较差';
    return '很差';
  }

  Widget _buildLegend() {
    final sortedMoods = _moodCounts.entries.toList()
      ..sort((a, b) => b.value.compareTo(a.value));

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.grey.shade200),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '情绪分布',
            style: Theme.of(context).textTheme.titleSmall?.copyWith(
                  fontWeight: FontWeight.bold,
                ),
          ),
          const SizedBox(height: 12),
          Wrap(
            spacing: 12,
            runSpacing: 8,
            children: sortedMoods.take(5).map((entry) {
              final moodConfig = MoodColors.getConfig(
                MoodColors.fromString(entry.key),
              );

              return Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 12,
                  vertical: 6,
                ),
                decoration: BoxDecoration(
                  color: moodConfig.primary.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(
                    color: moodConfig.primary.withValues(alpha: 0.3),
                    width: 1,
                  ),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(
                      moodConfig.icon,
                      size: 16,
                      color: moodConfig.primary,
                    ),
                    const SizedBox(width: 6),
                    Text(
                      moodConfig.name,
                      style: TextStyle(
                        fontSize: 12,
                        color: moodConfig.textColor,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                    const SizedBox(width: 4),
                    Text(
                      '${entry.value}天',
                      style: const TextStyle(
                        fontSize: 11,
                        color: AppTheme.textSecondary,
                      ),
                    ),
                  ],
                ),
              );
            }).toList(),
          ),
        ],
      ),
    );
  }

  Widget _buildStatistics() {
    return Row(
      children: [
        Expanded(
          child: _buildStatCard(
            icon: Icons.favorite_outline,
            label: '平均情绪',
            value: _getEmotionText(_avgScore),
            color: _getEmotionColor(_avgScore),
          ),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: _buildStatCard(
            icon: Icons.auto_awesome_outlined,
            label: '投入石头',
            value: '$_totalStones',
            color: AppTheme.primaryColor,
          ),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: _buildStatCard(
            icon: Icons.people_outline,
            label: '互动次数',
            value: '$_totalInteractions',
            color: AppTheme.secondaryColor,
          ),
        ),
      ],
    );
  }

  Widget _buildStatCard({
    required IconData icon,
    required String label,
    required String value,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: color.withValues(alpha: 0.2)),
      ),
      child: Column(
        children: [
          Icon(icon, color: color, size: 24),
          const SizedBox(height: 8),
          Text(
            value,
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: color,
            ),
          ),
          const SizedBox(height: 4),
          Text(
            label,
            style: const TextStyle(
              fontSize: 11,
              color: AppTheme.textSecondary,
            ),
          ),
        ],
      ),
    );
  }

  String _getEmotionText(double score) {
    if (score >= 0.6) return '很好';
    if (score >= 0.3) return '较好';
    if (score >= 0.0) return '一般';
    if (score >= -0.3) return '较差';
    return '很差';
  }

  Color _getEmotionColor(double score) {
    if (score >= 0.6) return AppTheme.successColor;
    if (score >= 0.3) return AppTheme.primaryColor;
    if (score >= 0.0) return AppTheme.secondaryColor;
    if (score >= -0.3) return AppTheme.warningColor;
    return AppTheme.errorColor;
  }

  Widget _buildEmptyState() {
    return Container(
      padding: const EdgeInsets.all(40),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [AppTheme.primaryColor.withValues(alpha: 0.05), Colors.white],
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
        ),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Column(
        children: [
          Icon(Icons.spa_outlined, size: 56, color: AppTheme.primaryColor.withValues(alpha: 0.6)),
          const SizedBox(height: 16),
          Text('心湖正在等待你的故事', style: Theme.of(context).textTheme.titleMedium?.copyWith(color: AppTheme.textPrimary, fontWeight: FontWeight.w500)),
          const SizedBox(height: 8),
          Text('每一次记录，都是与自己的温柔对话', textAlign: TextAlign.center, style: Theme.of(context).textTheme.bodySmall?.copyWith(color: AppTheme.textSecondary)),
        ],
      ),
    );
  }
}