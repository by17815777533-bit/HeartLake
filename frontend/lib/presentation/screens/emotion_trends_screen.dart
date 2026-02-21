// @file emotion_trends_screen.dart
// @brief 情绪星图 - 类光遇风格的情绪趋势与脉搏页面
import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../widgets/water_background.dart';
import '../widgets/emotion_pulse_widget.dart';

class EmotionTrendsScreen extends StatefulWidget {
  const EmotionTrendsScreen({super.key});

  @override
  State<EmotionTrendsScreen> createState() => _EmotionTrendsScreenState();
}

class _EmotionTrendsScreenState extends State<EmotionTrendsScreen>
    with TickerProviderStateMixin {
  final AIRecommendationService _recService = AIRecommendationService();
  final EdgeAIService _aiService = EdgeAIService();
  late AnimationController _starController;
  late AnimationController _fadeController;

  Map<String, dynamic> _trends = {};
  Map<String, dynamic>? _pulseData;
  Map<String, dynamic>? _privacyInfo;
  bool _loading = true;

  @override
  void initState() {
    super.initState();
    _starController = AnimationController(
      vsync: this, duration: const Duration(seconds: 12),
    )..repeat();
    _fadeController = AnimationController(
      vsync: this, duration: const Duration(milliseconds: 800),
    );
    _loadData();
  }

  @override
  void dispose() {
    _starController.dispose();
    _fadeController.dispose();
    super.dispose();
  }

  Future<void> _loadData() async {
    try {
      // 分开调用，因为返回类型不同
      final trendsFuture = _recService.getEmotionTrends();
      final pulseFuture = _aiService.getEmotionPulse();
      final privacyFuture = _aiService.getPrivacyBudget();

      final trends = await trendsFuture;
      final pulseResp = await pulseFuture;
      final privResp = await privacyFuture;

      if (mounted) {
        setState(() {
          _trends = trends;
          if (pulseResp.success && pulseResp.data != null) {
            _pulseData = pulseResp.data as Map<String, dynamic>;
          }
          if (privResp.success && privResp.data != null) {
            _privacyInfo = privResp.data as Map<String, dynamic>;
          }
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
      body: Stack(
        children: [
          const WaterBackground(forceDark: true),
          // 星光粒子层
          AnimatedBuilder(
            animation: _starController,
            builder: (_, __) => CustomPaint(
              painter: _StarFieldPainter(progress: _starController.value),
              size: Size.infinite,
            ),
          ),
          SafeArea(
            child: Column(
              children: [
                _buildHeader(),
                Expanded(
                  child: _loading
                      ? const Center(child: CircularProgressIndicator(
                          color: Color(0xFFFFD54F), strokeWidth: 2))
                      : FadeTransition(
                          opacity: _fadeController,
                          child: _buildContent(),
                        ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        children: [
          IconButton(
            icon: const Icon(Icons.arrow_back_ios, color: Colors.white70, size: 20),
            onPressed: () => Navigator.pop(context),
          ),
          const Expanded(
            child: Text('情绪星图',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.w300,
                color: Colors.white, letterSpacing: 3)),
          ),
          const SizedBox(width: 48),
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
          const SizedBox(height: 20),
          // 情绪脉搏光球
          const Center(child: EmotionPulseWidget(size: 140)),
          const SizedBox(height: 24),
          // 隐私保护徽章
          if (_privacyInfo != null) _buildPrivacyBadge(),
          const SizedBox(height: 24),
          // 情绪趋势卡片
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

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.06),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: const Color(0xFF4FC3F7).withValues(alpha: 0.2)),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Icon(Icons.shield_outlined, size: 14, color: Color(0xFF4FC3F7)),
          const SizedBox(width: 6),
          Text('差分隐私保护中',
            style: TextStyle(fontSize: 11,
              color: Colors.white.withValues(alpha: 0.7), letterSpacing: 0.5)),
          const SizedBox(width: 8),
          Text('ε ${percent.toStringAsFixed(0)}%',
            style: TextStyle(fontSize: 11,
              color: const Color(0xFF4FC3F7).withValues(alpha: 0.8))),
        ],
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
          Text('情绪分布',
            style: TextStyle(fontSize: 14, fontWeight: FontWeight.w300,
              color: Colors.white.withValues(alpha: 0.7), letterSpacing: 2)),
          const SizedBox(height: 12),
          ...distribution.entries.map((e) {
            final value = (e.value as num).toDouble().clamp(0.0, 1.0);
            final color = _moodColor(e.key);
            return Padding(
              padding: const EdgeInsets.only(bottom: 8),
              child: Row(
                children: [
                  SizedBox(width: 50,
                    child: Text(_moodLabel(e.key),
                      style: TextStyle(fontSize: 11,
                        color: Colors.white.withValues(alpha: 0.6)))),
                  Expanded(
                    child: Container(
                      height: 6,
                      decoration: BoxDecoration(
                        color: Colors.white.withValues(alpha: 0.06),
                        borderRadius: BorderRadius.circular(3)),
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
                            boxShadow: [BoxShadow(
                              color: color.withValues(alpha: 0.3),
                              blurRadius: 4)],
                          ),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Text('${(value * 100).toInt()}%',
                    style: TextStyle(fontSize: 10,
                      color: Colors.white.withValues(alpha: 0.5))),
                ],
              ),
            );
          }),
        ],
        const SizedBox(height: 24),
        // AI 洞察
        if (insights.isNotEmpty) ...[
          Text('AI 洞察',
            style: TextStyle(fontSize: 14, fontWeight: FontWeight.w300,
              color: Colors.white.withValues(alpha: 0.7), letterSpacing: 2)),
          const SizedBox(height: 12),
          ...insights.map((insight) => Container(
            margin: const EdgeInsets.only(bottom: 10),
            padding: const EdgeInsets.all(14),
            decoration: BoxDecoration(
              color: Colors.white.withValues(alpha: 0.05),
              borderRadius: BorderRadius.circular(14),
              border: Border.all(
                color: const Color(0xFFFFAB91).withValues(alpha: 0.15)),
            ),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Icon(Icons.auto_awesome, size: 14,
                  color: const Color(0xFFFFD54F).withValues(alpha: 0.7)),
                const SizedBox(width: 10),
                Expanded(
                  child: Text(insight.toString(),
                    style: TextStyle(fontSize: 13, height: 1.5,
                      color: Colors.white.withValues(alpha: 0.7))),
                ),
              ],
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

/// 星空粒子画笔
class _StarFieldPainter extends CustomPainter {
  final double progress;
  _StarFieldPainter({required this.progress});

  @override
  void paint(Canvas canvas, Size size) {
    final rng = math.Random(42);
    for (int i = 0; i < 40; i++) {
      final x = rng.nextDouble() * size.width;
      final baseY = rng.nextDouble() * size.height;
      final y = (baseY - progress * 30 * (i % 3 + 1)) % size.height;
      final radius = 1.0 + rng.nextDouble() * 1.5;
      final twinkle = (math.sin(progress * math.pi * 2 + i * 0.5) + 1) / 2;
      final opacity = 0.1 + twinkle * 0.3;
      final color = i % 3 == 0
          ? const Color(0xFFFFD54F)
          : i % 3 == 1
              ? const Color(0xFFFFAB91)
              : const Color(0xFF90CAF9);
      canvas.drawCircle(
        Offset(x, y),
        radius,
        Paint()..color = color.withValues(alpha: opacity),
      );
    }
  }

  @override
  bool shouldRepaint(_StarFieldPainter old) => old.progress != progress;
}
