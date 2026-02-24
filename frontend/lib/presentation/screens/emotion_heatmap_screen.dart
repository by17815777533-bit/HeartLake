// @file emotion_heatmap_screen.dart
// @brief 情绪热力图页面（与情绪日历拆分）

import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import '../../data/datasources/api_client.dart';
import '../../utils/app_theme.dart';
import '../widgets/emotion_heatmap.dart';
import '../widgets/emotion_insights_card.dart';
import '../widgets/shimmer_loading.dart';

class EmotionHeatmapScreen extends StatefulWidget {
  const EmotionHeatmapScreen({super.key});

  @override
  State<EmotionHeatmapScreen> createState() => _EmotionHeatmapScreenState();
}

class _EmotionHeatmapScreenState extends State<EmotionHeatmapScreen> {
  final ApiClient _apiClient = ApiClient();
  Map<String, Map<String, dynamic>> _heatmapData = {};
  List<String> _insights = [];
  bool _isLoading = true;

  @override
  void initState() {
    super.initState();
    _loadHeatmapData();
  }

  Future<void> _loadHeatmapData() async {
    if (mounted) setState(() => _isLoading = true);
    try {
      final response = await _apiClient.get('/users/my/emotion-heatmap');
      if (!mounted) return;
      if (response.statusCode == 200 && response.data['code'] == 0) {
        final rawData = response.data['data']?['days'] as Map<String, dynamic>? ?? {};
        final parsed = <String, Map<String, dynamic>>{};
        for (final entry in rawData.entries) {
          if (entry.value is Map) {
            parsed[entry.key] = Map<String, dynamic>.from(entry.value as Map);
          }
        }
        setState(() {
          _heatmapData = parsed;
          _insights = _generateInsights(parsed);
          _isLoading = false;
        });
        return;
      }
    } on DioException catch (e) {
      debugPrint('Load emotion heatmap error: $e');
    } catch (e) {
      debugPrint('Load emotion heatmap error: $e');
    }

    if (mounted) {
      setState(() {
        _isLoading = false;
        _insights = _generateInsights(_heatmapData);
      });
    }
  }

  List<String> _generateInsights(Map<String, Map<String, dynamic>> data) {
    if (data.isEmpty) return [];
    final insights = <String>[];
    final now = DateTime.now();

    double recentTotal = 0.0;
    int recentCount = 0;
    for (int i = 0; i < 7; i++) {
      final date = now.subtract(Duration(days: i));
      final key =
          '${date.year}-${date.month.toString().padLeft(2, '0')}-${date.day.toString().padLeft(2, '0')}';
      final dayData = data[key];
      if (dayData != null) {
        recentTotal += (dayData['score'] ?? 0.5) as num;
        recentCount++;
      }
    }
    if (recentCount >= 3) {
      final avg = recentTotal / recentCount;
      if (avg >= 0.65) {
        insights.add('最近 7 天情绪整体向上。');
      } else if (avg <= 0.35) {
        insights.add('最近 7 天有些低落，记得给自己留出恢复时间。');
      } else {
        insights.add('最近 7 天情绪较为平稳。');
      }
    }

    int positiveStreak = 0;
    for (int i = 0; i < 30; i++) {
      final date = now.subtract(Duration(days: i));
      final key =
          '${date.year}-${date.month.toString().padLeft(2, '0')}-${date.day.toString().padLeft(2, '0')}';
      final dayData = data[key];
      if (dayData != null && (dayData['score'] as num? ?? 0) >= 0.6) {
        positiveStreak++;
      } else {
        break;
      }
    }
    if (positiveStreak >= 2) {
      insights.add('你已连续 $positiveStreak 天保持积极状态。');
    }

    final weekdayScores = <int, List<double>>{};
    for (final entry in data.entries) {
      try {
        final date = DateTime.parse(entry.key);
        final score = (entry.value['score'] ?? 0.5) as num;
        weekdayScores.putIfAbsent(date.weekday, () => []).add(score.toDouble());
      } catch (_) {}
    }
    if (weekdayScores.isNotEmpty) {
      double bestAvg = -1.0;
      int bestDay = 1;
      for (final entry in weekdayScores.entries) {
        final avg = entry.value.reduce((a, b) => a + b) / entry.value.length;
        if (avg > bestAvg) {
          bestAvg = avg;
          bestDay = entry.key;
        }
      }
      const dayNames = ['', '周一', '周二', '周三', '周四', '周五', '周六', '周日'];
      insights.add('你的高能日通常出现在 ${dayNames[bestDay]}。');
    }

    return insights;
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      backgroundColor: isDark ? const Color(0xFF1A1A2E) : Colors.white,
      appBar: AppBar(
        title: const Text('情绪热力图'),
        backgroundColor: isDark ? const Color(0xFF16213E) : Colors.white,
        foregroundColor: isDark ? Colors.white : AppTheme.textPrimary,
      ),
      body: RefreshIndicator(
        onRefresh: _loadHeatmapData,
        child: ListView(
          physics: const AlwaysScrollableScrollPhysics(),
          padding: const EdgeInsets.all(16),
          children: [
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
              decoration: BoxDecoration(
                color: AppTheme.skyBlue.withValues(alpha: 0.08),
                borderRadius: BorderRadius.circular(14),
                border: Border.all(color: AppTheme.skyBlue.withValues(alpha: 0.2)),
              ),
              child: const Text(
                '热力图聚焦长期波动，日历聚焦单日记录。已拆分展示，便于你分别查看。',
                style: TextStyle(fontSize: 13, color: isDark ? AppTheme.darkTextSecondary : AppTheme.textSecondary, height: 1.5),
              ),
            ),
            const SizedBox(height: 16),
            if (_isLoading)
              const SizedBox(
                height: 220,
                child: Center(
                  child: WarmLoadingIndicator(
                    messages: ['正在汇总情绪热力图...', '计算最近情绪密度...', '生成情绪洞察...'],
                  ),
                ),
              )
            else
              EmotionHeatmap(data: _heatmapData),
            const SizedBox(height: 16),
            if (_insights.isNotEmpty) EmotionInsightsCard(insights: _insights),
          ],
        ),
      ),
    );
  }
}
