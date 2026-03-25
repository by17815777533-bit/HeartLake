// 情绪脉搏 - 光遇风格呼吸光球
import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../../di/service_locator.dart';

/// 情绪脉搏光球 - 实时展示社区情绪状态
class EmotionPulseWidget extends StatefulWidget {
  final double size;
  final int refreshVersion;

  const EmotionPulseWidget({
    super.key,
    this.size = 120,
    this.refreshVersion = 0,
  });

  @override
  State<EmotionPulseWidget> createState() => _EmotionPulseWidgetState();
}

class _EmotionPulseWidgetState extends State<EmotionPulseWidget>
    with SingleTickerProviderStateMixin {
  late AnimationController _breathController;
  final EdgeAIService _edgeAIService = sl<EdgeAIService>();
  Map<String, dynamic>? _pulseData;
  String? _loadErrorMessage;
  String _dominantMood = 'neutral';
  double _intensity = 0.5;

  static const _moodColors = {
    'joy': Color(0xFFFFD54F),
    'happy': Color(0xFFFFD54F),
    'sadness': Color(0xFF64B5F6),
    'sad': Color(0xFF64B5F6),
    'anger': Color(0xFFEF5350),
    'angry': Color(0xFFEF5350),
    'fear': Color(0xFFB39DDB),
    'fearful': Color(0xFFB39DDB),
    'anxious': Color(0xFFFFB74D),
    'confused': Color(0xFFA1887F),
    'calm': Color(0xFF4DB6AC),
    'surprise': Color(0xFFFFAB40),
    'surprised': Color(0xFFFFAB40),
    'neutral': Color(0xFF90CAF9),
  };

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

  void _showLoadFailure(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger == null) return;
    messenger
      ..hideCurrentSnackBar()
      ..showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  void initState() {
    super.initState();
    _breathController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 4),
    )..repeat(reverse: true);
    _loadPulse();
  }

  @override
  void didUpdateWidget(covariant EmotionPulseWidget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.refreshVersion != widget.refreshVersion) {
      _loadPulse(showFailureMessage: false);
    }
  }

  @override
  void dispose() {
    _breathController.dispose();
    super.dispose();
  }

  // 公开端点：/api/edge-ai/emotion-pulse
  Future<void> _loadPulse({bool showFailureMessage = true}) async {
    try {
      final result = await _edgeAIService.getEmotionPulse();
      if (!mounted) return;

      if (result['success'] != true) {
        final message = result['message']?.toString() ?? '情绪脉搏加载失败';
        setState(() {
          _loadErrorMessage = message;
        });
        if (showFailureMessage) {
          _showLoadFailure(message);
        }
        return;
      }
      if (result['data'] is! Map) {
        throw StateError('Emotion pulse payload is not a map');
      }

      final data = Map<String, dynamic>.from(result['data'] as Map);
      final dominantMood =
          (data['dominant_mood'] ?? 'neutral').toString().toLowerCase();
      final sampleCount = (data['sample_count'] as num?)?.toInt() ?? 0;
      final distribution = data['mood_distribution'] is Map
          ? data['mood_distribution'] as Map
          : null;
      final avgScore = (data['avg_score'] as num?)?.toDouble() ?? 0.0;

      double intensity;
      if (distribution != null && sampleCount > 0) {
        final dominantCount =
            (distribution[dominantMood] as num?)?.toDouble() ?? 0.0;
        intensity = (dominantCount / sampleCount).clamp(0.25, 1.0);
      } else {
        intensity = (avgScore.abs() + 0.3).clamp(0.25, 1.0);
      }

      setState(() {
        _pulseData = data;
        _dominantMood = dominantMood;
        _intensity = intensity;
        _loadErrorMessage = null;
      });
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'EmotionPulseWidget._loadPulse');
      if (!mounted) return;
      setState(() {
        _loadErrorMessage = '情绪脉搏加载失败，请稍后重试';
      });
      if (showFailureMessage) {
        _showLoadFailure('情绪脉搏加载失败，请稍后重试');
      }
    }
  }

  Color get _glowColor => _moodColors[_dominantMood] ?? const Color(0xFF90CAF9);

  @override
  Widget build(BuildContext context) {
    if (_pulseData == null) {
      return GestureDetector(
        onTap: _loadPulse,
        child: SizedBox(
          width: widget.size + 40,
          height: widget.size + 40,
          child: Center(
            child: Container(
              width: widget.size,
              height: widget.size,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: RadialGradient(
                  colors: [
                    Colors.blueGrey.withValues(alpha: 0.28),
                    Colors.blueGrey.withValues(alpha: 0.12),
                    Colors.blueGrey.withValues(alpha: 0.04),
                  ],
                  stops: const [0.3, 0.7, 1.0],
                ),
              ),
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Icon(
                      Icons.cloud_off_rounded,
                      color: Colors.white70,
                      size: 28,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _loadErrorMessage ?? '情绪脉搏暂不可用',
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        fontSize: 11,
                        color: Colors.white.withValues(alpha: 0.85),
                        height: 1.35,
                      ),
                    ),
                    const SizedBox(height: 6),
                    Text(
                      '点击重试',
                      style: TextStyle(
                        fontSize: 10,
                        color: Colors.white.withValues(alpha: 0.65),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      );
    }

    return AnimatedBuilder(
      animation: _breathController,
      builder: (_, __) {
        final breathValue = _breathController.value;
        final scale = 1.0 + breathValue * 0.08 * _intensity;
        final glowRadius = 20 + breathValue * 30 * _intensity;

        return GestureDetector(
          onTap: _loadPulse,
          child: SizedBox(
            width: widget.size + 40,
            height: widget.size + 40,
            child: Center(
              child: Transform.scale(
                scale: scale,
                child: Container(
                  width: widget.size,
                  height: widget.size,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: RadialGradient(
                      colors: [
                        _glowColor.withValues(alpha: 0.6),
                        _glowColor.withValues(alpha: 0.2),
                        _glowColor.withValues(alpha: 0.05),
                      ],
                      stops: const [0.3, 0.7, 1.0],
                    ),
                    boxShadow: [
                      BoxShadow(
                        color: _glowColor.withValues(alpha: 0.3 * _intensity),
                        blurRadius: glowRadius,
                        spreadRadius: glowRadius * 0.3,
                      ),
                    ],
                  ),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(
                        _moodEmoji,
                        style: const TextStyle(fontSize: 28),
                      ),
                      const SizedBox(height: 4),
                      Text(
                        _moodLabel,
                        style: TextStyle(
                          fontSize: 12,
                          color: Colors.white.withValues(alpha: 0.9),
                          fontWeight: FontWeight.w500,
                          letterSpacing: 1,
                        ),
                      ),
                      if (_pulseData != null) ...[
                        const SizedBox(height: 2),
                        Text(
                          '${(_intensity * 100).toInt()}%',
                          style: TextStyle(
                            fontSize: 10,
                            color: Colors.white.withValues(alpha: 0.6),
                          ),
                        ),
                        if (_loadErrorMessage != null) ...[
                          const SizedBox(height: 2),
                          Text(
                            '数据未更新',
                            style: TextStyle(
                              fontSize: 9,
                              color: Colors.white.withValues(alpha: 0.55),
                            ),
                          ),
                        ],
                      ],
                    ],
                  ),
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  String get _moodEmoji {
    const emojis = {
      'joy': '☀️',
      'happy': '☀️',
      'sadness': '🌧️',
      'sad': '🌧️',
      'anger': '🌋',
      'angry': '🌋',
      'fear': '🌙',
      'fearful': '🌙',
      'anxious': '🌫️',
      'confused': '🪵',
      'calm': '💧',
      'surprise': '⭐',
      'surprised': '⭐',
      'neutral': '🌊',
    };
    return emojis[_dominantMood] ?? '🌊';
  }

  String get _moodLabel {
    const labels = {
      'joy': '温暖',
      'happy': '温暖',
      'sadness': '宁静',
      'sad': '宁静',
      'anger': '炽热',
      'angry': '炽热',
      'fear': '幽深',
      'fearful': '幽深',
      'anxious': '波动',
      'confused': '迷雾',
      'calm': '安定',
      'surprise': '惊喜',
      'surprised': '惊喜',
      'neutral': '平静',
    };
    return labels[_dominantMood] ?? '平静';
  }
}

/// 情绪粒子环 - 环绕脉搏光球的星光粒子
class EmotionParticleRing extends StatefulWidget {
  final double radius;
  final Color color;
  final int particleCount;

  const EmotionParticleRing({
    super.key,
    this.radius = 80,
    this.color = const Color(0xFFFFD54F),
    this.particleCount = 12,
  });

  @override
  State<EmotionParticleRing> createState() => _EmotionParticleRingState();
}

class _EmotionParticleRingState extends State<EmotionParticleRing>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 20),
    )..repeat();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _controller,
      builder: (_, __) {
        return CustomPaint(
          painter: _ParticleRingPainter(
            progress: _controller.value,
            radius: widget.radius,
            color: widget.color,
            count: widget.particleCount,
          ),
          size: Size(widget.radius * 2 + 20, widget.radius * 2 + 20),
        );
      },
    );
  }
}

class _ParticleRingPainter extends CustomPainter {
  final double progress;
  final double radius;
  final Color color;
  final int count;

  _ParticleRingPainter({
    required this.progress,
    required this.radius,
    required this.color,
    required this.count,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final rng = math.Random(42);

    for (int i = 0; i < count; i++) {
      final angle = (i / count) * math.pi * 2 + progress * math.pi * 2;
      final wobble = rng.nextDouble() * 6;
      final r = radius + math.sin(progress * math.pi * 4 + i) * wobble;
      final x = center.dx + math.cos(angle) * r;
      final y = center.dy + math.sin(angle) * r;
      final particleSize = 1.5 + rng.nextDouble() * 2;
      final alpha =
          0.3 + math.sin(progress * math.pi * 2 + i * 0.5).abs() * 0.5;

      canvas.drawCircle(
        Offset(x, y),
        particleSize,
        Paint()
          ..color = color.withValues(alpha: alpha)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2),
      );
    }
  }

  @override
  bool shouldRepaint(_ParticleRingPainter old) => old.progress != progress;
}
