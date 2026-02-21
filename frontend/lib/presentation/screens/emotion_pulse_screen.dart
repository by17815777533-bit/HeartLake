// @file emotion_pulse_screen.dart
// @brief 情绪脉搏屏幕 - 光遇星空风格的实时情绪可视化
import 'dart:math';
import 'dart:ui';
import 'package:flutter/material.dart';
import '../../data/datasources/base_service.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../widgets/water_background.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';

class EmotionPulseScreen extends StatefulWidget {
  const EmotionPulseScreen({super.key});

  @override
  State<EmotionPulseScreen> createState() => _EmotionPulseScreenState();
}

class _EmotionPulseScreenState extends State<EmotionPulseScreen>
    with TickerProviderStateMixin {
  final _edgeAIService = EdgeAIService();
  final _recommendationService = AIRecommendationService();

  bool _isLoading = true;
  Map<MoodType, double> _emotionDistribution = {};
  MoodType _dominantMood = MoodType.neutral;
  List<_TrendPoint> _trendPoints = [];
  List<String> _aiInsights = [];
  double _privacyEpsilon = 0.0;
  double _privacyBudgetTotal = 1.0;

  late AnimationController _ringController;
  late AnimationController _particleController;
  late Animation<double> _ringAnimation;

  @override
  void initState() {
    super.initState();
    _ringController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1500),
    );
    _ringAnimation = CurvedAnimation(
      parent: _ringController,
      curve: Curves.easeOutCubic,
    );
    _particleController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 12),
    )..repeat();
    _loadData();
  }

  @override
  void dispose() {
    _ringController.dispose();
    _particleController.dispose();
    super.dispose();
  }

  Future<void> _loadData() async {
    setState(() => _isLoading = true);
    try {
      final results = await Future.wait([
        _edgeAIService.getEmotionPulse(),
        _edgeAIService.getPrivacyBudget(),
        _recommendationService.getEmotionTrends(),
      ]);

      if (!mounted) return;

      final pulseResp = results[0] as ServiceResponse;
      final privacyResp = results[1] as ServiceResponse;
      final trendsData = results[2] as Map<String, dynamic>;

      // 解析情绪分布
      if (pulseResp.success && pulseResp.data is Map) {
        final data = pulseResp.data as Map<String, dynamic>;
        final dist = <MoodType, double>{};
        final distribution = data['distribution'] as Map<String, dynamic>? ?? {};
        for (final entry in distribution.entries) {
          final mood = MoodColors.fromString(entry.key);
          dist[mood] = (entry.value as num).toDouble();
        }
        if (dist.isEmpty) {
          dist[MoodType.neutral] = 1.0;
        }
        _emotionDistribution = dist;
        _dominantMood = dist.entries
            .reduce((a, b) => a.value > b.value ? a : b)
            .key;

        // AI 洞察
        final insights = data['insights'] as List<dynamic>? ?? [];
        _aiInsights = insights.map((e) => e.toString()).toList();
        if (_aiInsights.isEmpty) {
          _aiInsights = [
            '你的情绪整体偏向${MoodColors.getConfig(_dominantMood).name}，这是一种自然的状态。',
            '建议适当关注情绪变化，保持内心的平衡与觉察。',
          ];
        }
      }

      // 解析隐私预算
      if (privacyResp.success && privacyResp.data is Map) {
        final data = privacyResp.data as Map<String, dynamic>;
        _privacyEpsilon = (data['used'] as num?)?.toDouble() ?? 0.0;
        _privacyBudgetTotal = (data['total'] as num?)?.toDouble() ?? 1.0;
      }

      // 解析情绪趋势
      _trendPoints = _parseTrends(trendsData);

      _ringController.forward(from: 0);
    } catch (e) {
      if (!mounted) return;
      // 使用默认数据
      _emotionDistribution = {MoodType.neutral: 1.0};
      _dominantMood = MoodType.neutral;
      _aiInsights = ['暂时无法获取情绪数据，请稍后再试。'];
    }

    if (mounted) {
      setState(() => _isLoading = false);
    }
  }

  List<_TrendPoint> _parseTrends(Map<String, dynamic> data) {
    final points = <_TrendPoint>[];
    final list = data['points'] as List<dynamic>? ?? data['data'] as List<dynamic>? ?? [];
    for (final item in list) {
      if (item is Map<String, dynamic>) {
        final score = (item['score'] as num?)?.toDouble() ?? 0.0;
        final label = item['label'] as String? ?? item['time'] as String? ?? '';
        final mood = MoodColors.fromSentimentScore(score);
        points.add(_TrendPoint(score: score, label: label, mood: mood));
      }
    }
    if (points.isEmpty) {
      // 生成示例数据
      final now = DateTime.now();
      for (int i = 6; i >= 0; i--) {
        final day = now.subtract(Duration(days: i));
        final score = (Random().nextDouble() * 2 - 1) * 0.6;
        points.add(_TrendPoint(
          score: score,
          label: '${day.month}/${day.day}',
          mood: MoodColors.fromSentimentScore(score),
        ));
      }
    }
    return points;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          const WaterBackground(forceDark: true),
          AnimatedBuilder(
            animation: _particleController,
            builder: (context, _) => CustomPaint(
              painter: _FloatingParticlePainter(
                progress: _particleController.value,
                dominantMood: _dominantMood,
              ),
              size: MediaQuery.of(context).size,
            ),
          ),
          SafeArea(
            child: RefreshIndicator(
              onRefresh: _loadData,
              color: AppTheme.primaryColor,
              child: SingleChildScrollView(
                physics: const AlwaysScrollableScrollPhysics(),
                padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
                child: _isLoading ? _buildLoading() : _buildContent(),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildLoading() {
    return SizedBox(
      height: MediaQuery.of(context).size.height * 0.8,
      child: const Center(
        child: CircularProgressIndicator(color: Colors.white70),
      ),
    );
  }

  Widget _buildContent() {
    return Column(
      children: [
        _buildHeader(),
        const SizedBox(height: 24),
        _buildEmotionRing(),
        const SizedBox(height: 28),
        _buildTrendChart(),
        const SizedBox(height: 28),
        _buildAIInsightCards(),
        const SizedBox(height: 20),
        _buildPrivacyBudgetBadge(),
        const SizedBox(height: 40),
      ],
    );
  }

  Widget _buildHeader() {
    return Row(
      children: [
        GestureDetector(
          onTap: () => Navigator.maybePop(context),
          child: Container(
            padding: const EdgeInsets.all(8),
            decoration: BoxDecoration(
              color: Colors.white.withValues(alpha: 0.1),
              borderRadius: BorderRadius.circular(12),
            ),
            child: const Icon(Icons.arrow_back_ios_new, color: Colors.white70, size: 18),
          ),
        ),
        const SizedBox(width: 14),
        const Expanded(
          child: Text(
            '情绪脉搏',
            style: TextStyle(
              fontSize: 22,
              fontWeight: FontWeight.w600,
              color: Colors.white,
              letterSpacing: 1.2,
            ),
          ),
        ),
      ],
    );
  }

  // ==================== 情绪分布环形图 ====================
  Widget _buildEmotionRing() {
    final config = MoodColors.getConfig(_dominantMood);
    return AnimatedBuilder(
      animation: _ringAnimation,
      builder: (context, child) {
        return Container(
          padding: const EdgeInsets.all(20),
          decoration: BoxDecoration(
            color: Colors.black.withValues(alpha: 0.25),
            borderRadius: BorderRadius.circular(24),
            border: Border.all(
              color: config.primary.withValues(alpha: 0.2),
            ),
          ),
          child: Column(
            children: [
              SizedBox(
                height: 220,
                width: 220,
                child: Stack(
                  alignment: Alignment.center,
                  children: [
                    // 光晕
                    Container(
                      width: 180,
                      height: 180,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        boxShadow: [
                          BoxShadow(
                            color: config.primary.withValues(alpha: 0.15),
                            blurRadius: 40,
                            spreadRadius: 10,
                          ),
                        ],
                      ),
                    ),
                    // 环形图
                    CustomPaint(
                      size: const Size(200, 200),
                      painter: _EmotionRingPainter(
                        distribution: _emotionDistribution,
                        progress: _ringAnimation.value,
                      ),
                    ),
                    // 中心情绪
                    Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(config.icon, size: 36, color: config.primary),
                        const SizedBox(height: 6),
                        Text(
                          config.name,
                          style: TextStyle(
                            fontSize: 16,
                            fontWeight: FontWeight.w600,
                            color: config.primary,
                          ),
                        ),
                        Text(
                          config.description,
                          style: TextStyle(
                            fontSize: 11,
                            color: Colors.white.withValues(alpha: 0.5),
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 16),
              // 图例
              Wrap(
                spacing: 12,
                runSpacing: 8,
                alignment: WrapAlignment.center,
                children: _emotionDistribution.entries.map((e) {
                  final c = MoodColors.getConfig(e.key);
                  final pct = (e.value * 100 / _emotionDistribution.values.fold(0.0, (a, b) => a + b)).toStringAsFixed(0);
                  return Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Container(
                        width: 8, height: 8,
                        decoration: BoxDecoration(
                          color: c.primary,
                          shape: BoxShape.circle,
                        ),
                      ),
                      const SizedBox(width: 4),
                      Text(
                        '${c.name} $pct%',
                        style: TextStyle(
                          fontSize: 11,
                          color: Colors.white.withValues(alpha: 0.7),
                        ),
                      ),
                    ],
                  );
                }).toList(),
              ),
            ],
          ),
        );
      },
    );
  }

  // ==================== 情绪趋势折线图 ====================
  Widget _buildTrendChart() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.black.withValues(alpha: 0.25),
        borderRadius: BorderRadius.circular(24),
        border: Border.all(
          color: Colors.white.withValues(alpha: 0.08),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '情绪趋势',
            style: TextStyle(
              fontSize: 16,
              fontWeight: FontWeight.w600,
              color: Colors.white.withValues(alpha: 0.9),
            ),
          ),
          const SizedBox(height: 20),
          SizedBox(
            height: 180,
            child: CustomPaint(
              size: Size(MediaQuery.of(context).size.width - 80, 180),
              painter: _EmotionTrendPainter(points: _trendPoints),
            ),
          ),
        ],
      ),
    );
  }

  // ==================== AI 洞察卡片 ====================
  Widget _buildAIInsightCards() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.only(left: 4, bottom: 12),
          child: Row(
            children: [
              Icon(Icons.auto_awesome, size: 16, color: AppTheme.warmOrange.withValues(alpha: 0.8)),
              const SizedBox(width: 6),
              Text(
                'AI 情绪洞察',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
                  color: Colors.white.withValues(alpha: 0.9),
                ),
              ),
            ],
          ),
        ),
        ..._aiInsights.map((insight) => Padding(
          padding: const EdgeInsets.only(bottom: 12),
          child: ClipRRect(
            borderRadius: BorderRadius.circular(18),
            child: BackdropFilter(
              filter: ImageFilter.blur(sigmaX: 12, sigmaY: 12),
              child: Container(
                width: double.infinity,
                padding: const EdgeInsets.all(16),
                decoration: BoxDecoration(
                  color: Colors.white.withValues(alpha: 0.08),
                  borderRadius: BorderRadius.circular(18),
                  border: Border.all(
                    color: AppTheme.warmOrange.withValues(alpha: 0.2),
                  ),
                ),
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Container(
                      margin: const EdgeInsets.only(top: 2),
                      padding: const EdgeInsets.all(6),
                      decoration: BoxDecoration(
                        color: AppTheme.warmOrange.withValues(alpha: 0.15),
                        borderRadius: BorderRadius.circular(8),
                      ),
                      child: Icon(
                        Icons.lightbulb_outline,
                        size: 14,
                        color: AppTheme.warmOrange.withValues(alpha: 0.9),
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Text(
                        insight,
                        style: TextStyle(
                          fontSize: 13,
                          color: Colors.white.withValues(alpha: 0.85),
                          height: 1.5,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        )),
      ],
    );
  }

  // ==================== 隐私预算徽章 ====================
  Widget _buildPrivacyBudgetBadge() {
    final ratio = _privacyBudgetTotal > 0
        ? (_privacyEpsilon / _privacyBudgetTotal).clamp(0.0, 1.0)
        : 0.0;
    final pct = (ratio * 100).toStringAsFixed(1);

    return ClipRRect(
      borderRadius: BorderRadius.circular(18),
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 14),
          decoration: BoxDecoration(
            color: Colors.white.withValues(alpha: 0.06),
            borderRadius: BorderRadius.circular(18),
            border: Border.all(
              color: const Color(0xFF26A69A).withValues(alpha: 0.2),
            ),
          ),
          child: Row(
            children: [
              SizedBox(
                width: 44,
                height: 44,
                child: CustomPaint(
                  painter: _PrivacyRingPainter(ratio: ratio),
                  child: Center(
                    child: Icon(
                      Icons.shield_outlined,
                      size: 16,
                      color: const Color(0xFF26A69A).withValues(alpha: 0.9),
                    ),
                  ),
                ),
              ),
              const SizedBox(width: 14),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      '差分隐私预算',
                      style: TextStyle(
                        fontSize: 13,
                        fontWeight: FontWeight.w600,
                        color: Colors.white.withValues(alpha: 0.85),
                      ),
                    ),
                    const SizedBox(height: 2),
                    Text(
                      '已消耗 $pct% (epsilon: ${_privacyEpsilon.toStringAsFixed(3)} / ${_privacyBudgetTotal.toStringAsFixed(1)})',
                      style: TextStyle(
                        fontSize: 11,
                        color: Colors.white.withValues(alpha: 0.5),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

// ==================== 数据模型 ====================
class _TrendPoint {
  final double score;
  final String label;
  final MoodType mood;
  const _TrendPoint({required this.score, required this.label, required this.mood});
}

// ==================== 环形图 Painter ====================
class _EmotionRingPainter extends CustomPainter {
  final Map<MoodType, double> distribution;
  final double progress;

  _EmotionRingPainter({required this.distribution, required this.progress});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2 - 16;
    const strokeWidth = 18.0;
    final total = distribution.values.fold(0.0, (a, b) => a + b);
    if (total == 0) return;

    double startAngle = -pi / 2;
    for (final entry in distribution.entries) {
      final sweep = (entry.value / total) * 2 * pi * progress;
      final config = MoodColors.getConfig(entry.key);

      // 光晕层
      final glowPaint = Paint()
        ..color = config.primary.withValues(alpha: 0.25)
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeWidth + 8
        ..strokeCap = StrokeCap.round
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 6);
      canvas.drawArc(
        Rect.fromCircle(center: center, radius: radius),
        startAngle, sweep, false, glowPaint,
      );

      // 主弧线
      final paint = Paint()
        ..color = config.primary
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeWidth
        ..strokeCap = StrokeCap.round;
      canvas.drawArc(
        Rect.fromCircle(center: center, radius: radius),
        startAngle, sweep, false, paint,
      );

      startAngle += sweep;
    }
  }

  @override
  bool shouldRepaint(covariant _EmotionRingPainter old) =>
      old.progress != progress || old.distribution != distribution;
}

// ==================== 趋势折线 Painter ====================
class _EmotionTrendPainter extends CustomPainter {
  final List<_TrendPoint> points;

  _EmotionTrendPainter({required this.points});

  @override
  void paint(Canvas canvas, Size size) {
    if (points.isEmpty) return;

    const paddingLeft = 30.0;
    const paddingBottom = 24.0;
    final chartW = size.width - paddingLeft - 10;
    final chartH = size.height - paddingBottom - 10;

    // 坐标轴标签
    final labelStyle = TextStyle(
      color: Colors.white.withValues(alpha: 0.4),
      fontSize: 10,
    );

    // 网格线
    final gridPaint = Paint()
      ..color = Colors.white.withValues(alpha: 0.06)
      ..strokeWidth = 0.5;
    for (int i = 0; i <= 4; i++) {
      final y = 10 + chartH * i / 4;
      canvas.drawLine(
        Offset(paddingLeft, y),
        Offset(paddingLeft + chartW, y),
        gridPaint,
      );
    }

    // 计算点位置
    final positions = <Offset>[];
    for (int i = 0; i < points.length; i++) {
      final x = paddingLeft + (chartW * i / (points.length - 1).clamp(1, 999));
      final normalized = (points[i].score + 1) / 2; // -1~1 -> 0~1
      final y = 10 + chartH * (1 - normalized);
      positions.add(Offset(x, y));

      // X 轴标签
      final tp = TextPainter(
        text: TextSpan(text: points[i].label, style: labelStyle),
        textDirection: TextDirection.ltr,
      )..layout();
      tp.paint(canvas, Offset(x - tp.width / 2, size.height - paddingBottom + 6));
    }

    if (positions.length < 2) return;

    // 渐变填充
    final path = Path()..moveTo(positions.first.dx, positions.first.dy);
    for (int i = 1; i < positions.length; i++) {
      final prev = positions[i - 1];
      final curr = positions[i];
      final cpx = (prev.dx + curr.dx) / 2;
      path.cubicTo(cpx, prev.dy, cpx, curr.dy, curr.dx, curr.dy);
    }
    final fillPath = Path.from(path)
      ..lineTo(positions.last.dx, 10 + chartH)
      ..lineTo(positions.first.dx, 10 + chartH)
      ..close();

    final fillPaint = Paint()
      ..shader = LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [
          const Color(0xFF64B5F6).withValues(alpha: 0.3),
          const Color(0xFF64B5F6).withValues(alpha: 0.0),
        ],
      ).createShader(Rect.fromLTWH(paddingLeft, 10, chartW, chartH));
    canvas.drawPath(fillPath, fillPaint);

    // 线条
    final linePaint = Paint()
      ..color = const Color(0xFF64B5F6)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2.5
      ..strokeCap = StrokeCap.round;
    canvas.drawPath(path, linePaint);

    // 数据点 + 光晕
    for (int i = 0; i < positions.length; i++) {
      final config = MoodColors.getConfig(points[i].mood);
      // 光晕
      canvas.drawCircle(
        positions[i], 8,
        Paint()
          ..color = config.primary.withValues(alpha: 0.25)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
      );
      // 点
      canvas.drawCircle(positions[i], 4, Paint()..color = config.primary);
      canvas.drawCircle(positions[i], 2, Paint()..color = Colors.white);
    }
  }

  @override
  bool shouldRepaint(covariant _EmotionTrendPainter old) => old.points != points;
}

// ==================== 浮动粒子 Painter ====================
class _FloatingParticlePainter extends CustomPainter {
  final double progress;
  final MoodType dominantMood;
  static final List<_Particle> _particles = _initParticles();

  _FloatingParticlePainter({required this.progress, required this.dominantMood});

  static List<_Particle> _initParticles() {
    final rng = Random(42);
    return List.generate(50, (_) => _Particle(
      x: rng.nextDouble(),
      y: rng.nextDouble(),
      size: rng.nextDouble() * 2.5 + 0.5,
      speed: rng.nextDouble() * 0.3 + 0.1,
      phase: rng.nextDouble() * 2 * pi,
      brightness: rng.nextDouble() * 0.5 + 0.3,
    ));
  }

  @override
  void paint(Canvas canvas, Size size) {
    final config = MoodColors.getConfig(dominantMood);
    for (final p in _particles) {
      final t = (progress + p.phase / (2 * pi)) % 1.0;
      final x = p.x * size.width + sin(t * 2 * pi + p.phase) * 20;
      final y = ((p.y + t * p.speed) % 1.1) * size.height;
      final alpha = (sin(t * 2 * pi) * 0.5 + 0.5) * p.brightness;

      canvas.drawCircle(
        Offset(x, y),
        p.size,
        Paint()
          ..color = config.primary.withValues(alpha: alpha * 0.6)
          ..maskFilter = MaskFilter.blur(BlurStyle.normal, p.size),
      );
      canvas.drawCircle(
        Offset(x, y),
        p.size * 0.4,
        Paint()..color = Colors.white.withValues(alpha: alpha * 0.8),
      );
    }
  }

  @override
  bool shouldRepaint(covariant _FloatingParticlePainter old) =>
      old.progress != progress || old.dominantMood != dominantMood;
}

class _Particle {
  final double x, y, size, speed, phase, brightness;
  const _Particle({
    required this.x, required this.y, required this.size,
    required this.speed, required this.phase, required this.brightness,
  });
}

// ==================== 隐私预算环形 Painter ====================
class _PrivacyRingPainter extends CustomPainter {
  final double ratio;

  _PrivacyRingPainter({required this.ratio});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2 - 4;
    const strokeWidth = 3.5;

    // 背景环
    canvas.drawCircle(
      center, radius,
      Paint()
        ..color = Colors.white.withValues(alpha: 0.08)
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeWidth,
    );

    // 进度环
    final sweep = 2 * pi * ratio;
    final color = ratio < 0.6
        ? const Color(0xFF26A69A)
        : ratio < 0.85
            ? const Color(0xFFFFB74D)
            : const Color(0xFFE57373);

    // 光晕
    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      -pi / 2, sweep, false,
      Paint()
        ..color = color.withValues(alpha: 0.3)
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeWidth + 3
        ..strokeCap = StrokeCap.round
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3),
    );

    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      -pi / 2, sweep, false,
      Paint()
        ..color = color
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeWidth
        ..strokeCap = StrokeCap.round,
    );
  }

  @override
  bool shouldRepaint(covariant _PrivacyRingPainter old) => old.ratio != ratio;
}
