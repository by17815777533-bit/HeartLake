// @file emotion_trends_screen.dart
// @brief 情绪趋势可视化 - 蓝色湖面风格
import 'package:flutter/material.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../../utils/app_theme.dart';
import '../../emotion_effects/water_background.dart';
import '../widgets/emotion_pulse_widget.dart';

class EmotionTrendsScreen extends StatefulWidget {
  const EmotionTrendsScreen({super.key});
  @override
  State<EmotionTrendsScreen> createState() => _EmotionTrendsScreenState();
}

class _EmotionTrendsScreenState extends State<EmotionTrendsScreen>
    with SingleTickerProviderStateMixin {
  final AIRecommendationService _aiService = AIRecommendationService();
  final EdgeAIService _edgeService = EdgeAIService();
  late AnimationController _fadeController;
  Map<String, dynamic> _trends = {};
  Map<String, dynamic>? _privacyInfo;
  bool _loading = true;

  @override
  void initState() {
    super.initState();
    _fadeController = AnimationController(
      vsync: this, duration: const Duration(milliseconds: 800),
    );
    _loadData();
  }

  @override
  void dispose() {
    _fadeController.dispose();
    super.dispose();
  }

  Future<void> _loadData() async {
    try {
      final results = await Future.wait([
        _aiService.getEmotionTrends(),
        _edgeService.getPrivacyBudget(),
      ]);
      if (mounted) {
        setState(() {
          _trends = results[0] as Map<String, dynamic>;
          _privacyInfo = results[1] as Map<String, dynamic>;
          _loading = false;
        });
        _fadeController.forward();
      }
    } catch (_) {
      if (mounted) setState(() => _loading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: AppTheme.textPrimary,
        title: const Text(
          '情绪趋势',
          style: TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.w500,
            letterSpacing: 1,
          ),
        ),
        centerTitle: true,
      ),
      body: Stack(
        children: [
          const WaterBackground(),
          SafeArea(
            child: _loading
                ? Center(
                    child: CircularProgressIndicator(
                      color: AppTheme.primaryColor,
                      strokeWidth: 2,
                    ),
                  )
                : FadeTransition(
                    opacity: _fadeController,
                    child: _buildContent(),
                  ),
          ),
        ],
      ),
    );
  }

  Widget _buildContent() {
    return SingleChildScrollView(
      physics: const BouncingScrollPhysics(),
      padding: const EdgeInsets.symmetric(horizontal: 20),
      child: Column(
        children: [
          const SizedBox(height: 12),
          // 情绪脉搏
          const EmotionPulseWidget(size: 140),
          const SizedBox(height: 20),
          // 隐私徽章
          _buildPrivacyBadge(),
          const SizedBox(height: 24),
          // 趋势卡片
          _buildTrendCards(),
          const SizedBox(height: 40),
        ],
      ),
    );
  }

  Widget _buildPrivacyBadge() {
    final epsilon = (_privacyInfo?['epsilon_used'] ?? 0.0).toDouble();
    final total = (_privacyInfo?['epsilon_total'] ?? 10.0).toDouble();
    final percent = total > 0 ? (epsilon / total * 100).clamp(0.0, 100.0) : 0.0;

    return Card(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      elevation: 0,
      color: Theme.of(context).colorScheme.surfaceContainerHighest.withValues(alpha: 0.7),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.shield_outlined, size: 14, color: AppTheme.secondaryColor),
            const SizedBox(width: 6),
            Text(
              '差分隐私保护中',
              style: TextStyle(
                fontSize: 11,
                color: AppTheme.textSecondary,
                letterSpacing: 0.5,
              ),
            ),
            const SizedBox(width: 8),
            Text(
              'ε ${percent.toStringAsFixed(0)}%',
              style: TextStyle(
                fontSize: 11,
                color: AppTheme.secondaryColor.withValues(alpha: 0.8),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTrendCards() {
    final distribution = _trends['distribution'] as Map<String, dynamic>? ?? {};
    final insights = _trends['insights'] as List? ?? [];

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // 情绪分布
        if (distribution.isNotEmpty) ...[
          Text(
            '情绪分布',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w500,
              color: AppTheme.textPrimary,
              letterSpacing: 1,
            ),
          ),
          const SizedBox(height: 12),
          Card(
            shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
            elevation: 1,
            child: Padding(
              padding: const EdgeInsets.all(14),
              child: Column(
                children: distribution.entries.map((e) {
                  final value = (e.value as num).toDouble().clamp(0.0, 1.0);
                  final color = _moodColor(e.key);
                  return Padding(
                    padding: const EdgeInsets.only(bottom: 8),
                    child: Row(
                      children: [
                        SizedBox(
                          width: 50,
                          child: Text(
                            _moodLabel(e.key),
                            style: TextStyle(
                              fontSize: 11,
                              color: AppTheme.textSecondary,
                            ),
                          ),
                        ),
                        Expanded(
                          child: Container(
                            height: 6,
                            decoration: BoxDecoration(
                              color: AppTheme.textTertiary.withValues(alpha: 0.15),
                              borderRadius: BorderRadius.circular(3),
                            ),
                            child: FractionallySizedBox(
                              alignment: Alignment.centerLeft,
                              widthFactor: value,
                              child: Container(
                                decoration: BoxDecoration(
                                  gradient: LinearGradient(colors: [
                                    color.withValues(alpha: 0.8),
                                    color.withValues(alpha: 0.4),
                                  ]),
                                  borderRadius: BorderRadius.circular(3),
                                  boxShadow: [
                                    BoxShadow(
                                      color: color.withValues(alpha: 0.3),
                                      blurRadius: 4,
                                    ),
                                  ],
                                ),
                              ),
                            ),
                          ),
                        ),
                        const SizedBox(width: 8),
                        Text(
                          '${(value * 100).toInt()}%',
                          style: TextStyle(
                            fontSize: 10,
                            color: AppTheme.textTertiary,
                          ),
                        ),
                      ],
                    ),
                  );
                }).toList(),
              ),
            ),
          ),
        ],
        const SizedBox(height: 24),
        // AI 洞察
        if (insights.isNotEmpty) ...[
          Text(
            'AI 洞察',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w500,
              color: AppTheme.textPrimary,
              letterSpacing: 1,
            ),
          ),
          const SizedBox(height: 12),
          ...insights.map((insight) => Padding(
            padding: const EdgeInsets.only(bottom: 10),
            child: Card(
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
              elevation: 1,
              child: Padding(
                padding: const EdgeInsets.all(14),
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Icon(
                      Icons.auto_awesome,
                      size: 14,
                      color: AppTheme.primaryColor.withValues(alpha: 0.7),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: Text(
                        insight.toString(),
                        style: TextStyle(
                          fontSize: 13,
                          height: 1.5,
                          color: AppTheme.textSecondary,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          )),
        ],
      ],
    );
  }

  Color _moodColor(String mood) {
    const colors = {
      'happy': Color(0xFFFFD54F), 'joy': Color(0xFFFFD54F),
      'sad': Color(0xFF64B5F6), 'sadness': Color(0xFF64B5F6),
      'angry': Color(0xFFEF5350), 'anger': Color(0xFFEF5350),
      'fear': Color(0xFFB39DDB), 'fearful': Color(0xFFB39DDB),
      'surprise': Color(0xFFFFAB40), 'surprised': Color(0xFFFFAB40),
      'neutral': Color(0xFF90CAF9), 'calm': Color(0xFF80CBC4),
    };
    return colors[mood] ?? const Color(0xFF90CAF9);
  }

  String _moodLabel(String mood) {
    const labels = {
      'happy': '开心', 'joy': '开心', 'sad': '忧伤', 'sadness': '忧伤',
      'angry': '愤怒', 'anger': '愤怒', 'fear': '恐惧', 'fearful': '恐惧',
      'surprise': '惊喜', 'surprised': '惊喜', 'neutral': '平静', 'calm': '平静',
    };
    return labels[mood] ?? mood;
  }
}
