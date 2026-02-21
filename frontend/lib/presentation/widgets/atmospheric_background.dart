// @file atmospheric_background.dart
// @brief 氛围背景组件（性能优化版）
// Created by 吴睿璐

library;
import 'package:flutter/material.dart';
import 'dart:math' as math;
import 'dart:ui' as ui;

class AtmosphericBackground extends StatefulWidget {
  final Widget child;
  final bool enableParticles;
  final bool enableTimeBasedGradient;
  final int particleCount;

  const AtmosphericBackground({
    super.key,
    required this.child,
    this.enableParticles = false,
    this.enableTimeBasedGradient = true,
    this.particleCount = 20,
  });

  @override
  State<AtmosphericBackground> createState() => _AtmosphericBackgroundState();
}

class _AtmosphericBackgroundState extends State<AtmosphericBackground>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late List<Particle> _particles;

  // 渐变缓存：只在小时变化时重新计算
  int _cachedHour = -1;
  LinearGradient? _cachedGradient;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      duration: const Duration(seconds: 30),
      vsync: this,
    )..repeat();

    _particles = List.generate(
      widget.particleCount,
      (index) => Particle.random(),
    );
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  LinearGradient _getTimeBasedGradient() {
    if (!widget.enableTimeBasedGradient) {
      return const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          Color(0xFFF5F7FA),
          Color(0xFFE8EAF6),
        ],
      );
    }

    final hour = DateTime.now().hour;

    // 缓存命中：同一小时内直接返回
    if (hour == _cachedHour && _cachedGradient != null) {
      return _cachedGradient!;
    }
    _cachedHour = hour;

    // 早晨 (5-9)
    if (hour >= 5 && hour < 9) {
      _cachedGradient = const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          Color(0xFFFFF3E0), // 温暖的橙色
          Color(0xFFFFE0B2),
          Color(0xFFFFF9C4), // 柔和的黄色
        ],
      );
    }
    // 上午 (9-12)
    else if (hour >= 9 && hour < 12) {
      _cachedGradient = const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          Color(0xFFE3F2FD), // 清新的蓝色
          Color(0xFFBBDEFB),
          Color(0xFFE1F5FE),
        ],
      );
    }
    // 下午 (12-17)
    else if (hour >= 12 && hour < 17) {
      _cachedGradient = const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          Color(0xFFF3E5F5), // 柔和的紫色
          Color(0xFFE1BEE7),
          Color(0xFFF8BBD0), // 温柔的粉色
        ],
      );
    }
    // 傍晚 (17-20)
    else if (hour >= 17 && hour < 20) {
      _cachedGradient = const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          Color(0xFFFFE0B2), // 温暖的橙色
          Color(0xFFFFCC80),
          Color(0xFFFFAB91), // 柔和的珊瑚色
        ],
      );
    }
    // 夜晚 (20-5)
    else {
      _cachedGradient = const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          Color(0xFF283593), // 深蓝色
          Color(0xFF3949AB),
          Color(0xFF5C6BC0), // 柔和的靛蓝色
        ],
      );
    }

    return _cachedGradient!;
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        // 渐变背景
        Positioned.fill(
          child: AnimatedContainer(
            duration: const Duration(seconds: 2),
            decoration: BoxDecoration(
              gradient: _getTimeBasedGradient(),
            ),
          ),
        ),

        // 粒子效果（RepaintBoundary 隔离重绘区域）
        if (widget.enableParticles)
          Positioned.fill(
            child: RepaintBoundary(
              child: AnimatedBuilder(
                animation: _controller,
                builder: (context, child) {
                  return CustomPaint(
                    painter: ParticlePainter(
                      particles: _particles,
                      animation: _controller.value,
                    ),
                  );
                },
              ),
            ),
          ),

        // 内容
        widget.child,
      ],
    );
  }
}

class Particle {
  final double x;
  final double y;
  final double size;
  final double speed;
  final Color color;
  final double opacity;

  Particle({
    required this.x,
    required this.y,
    required this.size,
    required this.speed,
    required this.color,
    required this.opacity,
  });

  factory Particle.random() {
    final random = math.Random();
    return Particle(
      x: random.nextDouble(),
      y: random.nextDouble(),
      size: random.nextDouble() * 4 + 2,
      speed: random.nextDouble() * 0.5 + 0.1,
      color: Colors.white,
      opacity: random.nextDouble() * 0.3 + 0.1,
    );
  }
}

class ParticlePainter extends CustomPainter {
  final List<Particle> particles;
  final double animation;

  ParticlePainter({
    required this.particles,
    required this.animation,
  });

  @override
  void paint(Canvas canvas, Size size) {
    if (particles.length > 10) {
      // 批量绘制优化：粒子数 > 10 时使用 drawRawPoints
      _drawBatch(canvas, size);
    } else {
      _drawIndividual(canvas, size);
    }
  }

  /// 逐个绘制（粒子数 <= 10）
  void _drawIndividual(Canvas canvas, Size size) {
    for (final particle in particles) {
      final paint = Paint()
        ..color = particle.color.withValues(alpha: particle.opacity)
        ..style = PaintingStyle.fill;

      final y = ((particle.y + animation * particle.speed) % 1.0) * size.height;
      final x = particle.x * size.width;

      canvas.drawCircle(Offset(x, y), particle.size, paint);
    }
  }

  /// 批量绘制（粒子数 > 10）：按透明度分组，使用 drawRawPoints
  void _drawBatch(Canvas canvas, Size size) {
    // 按相近透明度分桶（0.1 步长），减少 Paint 切换次数
    final Map<int, List<Offset>> buckets = {};

    for (final particle in particles) {
      final y = ((particle.y + animation * particle.speed) % 1.0) * size.height;
      final x = particle.x * size.width;
      final bucket = (particle.opacity * 10).round();
      (buckets[bucket] ??= []).add(Offset(x, y));
    }

    for (final entry in buckets.entries) {
      final alpha = entry.key / 10.0;
      final points = entry.value;

      final paint = Paint()
        ..color = Colors.white.withValues(alpha: alpha)
        ..strokeWidth = 4.0 // 平均粒子直径
        ..strokeCap = StrokeCap.round
        ..style = PaintingStyle.stroke;

      canvas.drawPoints(
        ui.PointMode.points,
        points,
        paint,
      );
    }
  }

  @override
  bool shouldRepaint(ParticlePainter oldDelegate) {
    return oldDelegate.animation != animation;
  }
}

class FrostedGlassCard extends StatelessWidget {
  final Widget child;
  final double blur;
  final double opacity;
  final BorderRadius? borderRadius;
  final EdgeInsetsGeometry? padding;
  final EdgeInsetsGeometry? margin;

  const FrostedGlassCard({
    super.key,
    required this.child,
    this.blur = 10.0,
    this.opacity = 0.3,
    this.borderRadius,
    this.padding,
    this.margin,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: margin,
      decoration: BoxDecoration(
        borderRadius: borderRadius ?? BorderRadius.circular(16),
        color: Colors.white.withValues(alpha: opacity),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.1),
            blurRadius: 20,
            offset: const Offset(0, 10),
          ),
        ],
      ),
      child: ClipRRect(
        borderRadius: borderRadius ?? BorderRadius.circular(16),
        child: Container(
          padding: padding ?? const EdgeInsets.all(16),
          child: child,
        ),
      ),
    );
  }
}

class GlowEffect extends StatelessWidget {
  final Widget child;
  final Color glowColor;
  final double glowRadius;
  final double glowOpacity;

  const GlowEffect({
    super.key,
    required this.child,
    required this.glowColor,
    this.glowRadius = 20.0,
    this.glowOpacity = 0.5,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        boxShadow: [
          BoxShadow(
            color: glowColor.withValues(alpha: glowOpacity),
            blurRadius: glowRadius,
            spreadRadius: glowRadius / 2,
          ),
        ],
      ),
      child: child,
    );
  }
}

class GradientMask extends StatelessWidget {
  final Widget child;
  final Gradient gradient;
  final BlendMode blendMode;

  const GradientMask({
    super.key,
    required this.child,
    required this.gradient,
    this.blendMode = BlendMode.srcIn,
  });

  @override
  Widget build(BuildContext context) {
    return ShaderMask(
      shaderCallback: (bounds) => gradient.createShader(bounds),
      blendMode: blendMode,
      child: child,
    );
  }
}

enum LakeScene { surface, reflection, underwater }

class SceneTransitionBackground extends StatefulWidget {
  final Widget child;
  final LakeScene scene;
  final Duration transitionDuration;

  const SceneTransitionBackground({
    super.key,
    required this.child,
    this.scene = LakeScene.surface,
    this.transitionDuration = const Duration(milliseconds: 800),
  });

  @override
  State<SceneTransitionBackground> createState() => _SceneTransitionBackgroundState();
}

class _SceneTransitionBackgroundState extends State<SceneTransitionBackground>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      duration: const Duration(seconds: 4),
      vsync: this,
    )..repeat();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  List<Color> _getSceneColors() {
    switch (widget.scene) {
      case LakeScene.surface:
        return const [Color(0xFF87CEEB), Color(0xFF4A90D9), Color(0xFF2E5A88)];
      case LakeScene.reflection:
        return const [Color(0xFF4A90D9), Color(0xFF2E5A88), Color(0xFF1A3A5C)];
      case LakeScene.underwater:
        return const [Color(0xFF1A3A5C), Color(0xFF0D2137), Color(0xFF061018)];
    }
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        AnimatedContainer(
          duration: widget.transitionDuration,
          curve: Curves.easeInOut,
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: _getSceneColors(),
            ),
          ),
        ),
        if (widget.scene != LakeScene.underwater)
          Positioned.fill(
            child: AnimatedBuilder(
              animation: _controller,
              builder: (context, _) => CustomPaint(
                painter: _RipplePainter(_controller.value, widget.scene),
              ),
            ),
          ),
        AnimatedOpacity(
          duration: widget.transitionDuration,
          opacity: widget.scene == LakeScene.underwater ? 0.4 : 0.0,
          child: Container(color: const Color(0xFF0A1929)),
        ),
        widget.child,
      ],
    );
  }
}

class _RipplePainter extends CustomPainter {
  final double animation;
  final LakeScene scene;

  _RipplePainter(this.animation, this.scene);

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.white.withValues(alpha: scene == LakeScene.reflection ? 0.08 : 0.12)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.5;

    final centerY = scene == LakeScene.reflection ? size.height * 0.3 : size.height * 0.6;
    for (int i = 0; i < 3; i++) {
      final radius = (animation + i * 0.33) % 1.0 * size.width * 0.4;
      canvas.drawOval(
        Rect.fromCenter(
          center: Offset(size.width / 2, centerY),
          width: radius * 2,
          height: radius * 0.3,
        ),
        paint..color = paint.color.withValues(alpha: (1 - (animation + i * 0.33) % 1.0) * 0.12),
      );
    }
  }

  @override
  bool shouldRepaint(_RipplePainter old) => old.animation != animation || old.scene != scene;
}

class WaveBackground extends StatefulWidget {
  final Widget child;
  final Color waveColor;
  final double waveHeight;
  final Duration duration;

  const WaveBackground({
    super.key,
    required this.child,
    this.waveColor = const Color(0xFF64B5F6),
    this.waveHeight = 100.0,
    this.duration = const Duration(seconds: 3),
  });

  @override
  State<WaveBackground> createState() => _WaveBackgroundState();
}

class _WaveBackgroundState extends State<WaveBackground>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      duration: widget.duration,
      vsync: this,
    )..repeat();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        Positioned.fill(
          child: AnimatedBuilder(
            animation: _controller,
            builder: (context, child) {
              return CustomPaint(
                painter: WavePainter(
                  animation: _controller.value,
                  color: widget.waveColor,
                  waveHeight: widget.waveHeight,
                ),
              );
            },
          ),
        ),
        widget.child,
      ],
    );
  }
}

class WavePainter extends CustomPainter {
  final double animation;
  final Color color;
  final double waveHeight;

  WavePainter({
    required this.animation,
    required this.color,
    required this.waveHeight,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = color.withValues(alpha: 0.3)
      ..style = PaintingStyle.fill;

    final path = Path();
    path.moveTo(0, size.height);

    // 绘制波浪
    for (double i = 0; i <= size.width; i++) {
      final y = size.height -
          waveHeight +
          math.sin((i / size.width * 2 * math.pi) + (animation * 2 * math.pi)) *
              waveHeight *
              0.5;
      path.lineTo(i, y);
    }

    path.lineTo(size.width, size.height);
    path.close();

    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(WavePainter oldDelegate) {
    return oldDelegate.animation != animation;
  }
}