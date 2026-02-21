// @file ripple_effect.dart
// @brief 涟漪效果动画组件
// Created by 吴睿璐

library;

import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../../utils/mood_colors.dart';

/// 单个涟漪数据
class _Ripple {
  final Offset center;
  final double startTime;
  final Color color;

  _Ripple({required this.center, required this.startTime, required this.color});
}

/// 交互式涟漪背景 - 点击产生扩散涟漪
class InteractiveRippleBackground extends StatefulWidget {
  final MoodType mood;
  final Widget child;
  final bool enableTap;

  const InteractiveRippleBackground({
    super.key,
    required this.mood,
    required this.child,
    this.enableTap = true,
  });

  @override
  State<InteractiveRippleBackground> createState() =>
      _InteractiveRippleBackgroundState();
}

class _InteractiveRippleBackgroundState extends State<InteractiveRippleBackground>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  final List<_Ripple> _ripples = [];

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 15), // 更慢更柔和
    )..addListener(_cleanupExpiredRipples);
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  void _addRipple(Offset position) {
    final moodConfig = MoodColors.getConfig(widget.mood);
    final wasEmpty = _ripples.isEmpty;
    _ripples.add(_Ripple(
      center: position,
      startTime: _controller.value,
      color: moodConfig.rippleColor,
    ));
    if (_ripples.length > 5) _ripples.removeAt(0);
    if (wasEmpty) {
      _controller.repeat();
      setState(() {});
    }
  }

  void _cleanupExpiredRipples() {
    final hadRipples = _ripples.isNotEmpty;
    _ripples.removeWhere((r) {
      double elapsed = _controller.value - r.startTime;
      if (elapsed < 0) elapsed += 1.0;
      return (elapsed * 5) >= 1.0;
    });
    if (hadRipples && _ripples.isEmpty) {
      _controller.stop();
      setState(() {});
    }
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: widget.enableTap ? (d) => _addRipple(d.localPosition) : null,
      child: Stack(
        children: [
          widget.child,
          if (_ripples.isNotEmpty)
            Positioned.fill(
              child: AnimatedBuilder(
                animation: _controller,
                builder: (context, _) => CustomPaint(
                  painter: _RipplePainter(ripples: _ripples, progress: _controller.value),
                ),
              ),
            ),
        ],
      ),
    );
  }
}

class _RipplePainter extends CustomPainter {
  final List<_Ripple> ripples;
  final double progress;

  _RipplePainter({required this.ripples, required this.progress});

  @override
  void paint(Canvas canvas, Size size) {
    final maxRadius = size.width * 0.4;
    for (final ripple in ripples) {
      double elapsed = progress - ripple.startTime;
      if (elapsed < 0) elapsed += 1.0;

      final lifeProgress = (elapsed * 5).clamp(0.0, 1.0);
      if (lifeProgress >= 1.0) continue;

      // 使用更柔和的曲线，温暖渐出
      final radius = maxRadius * Curves.easeOutQuart.transform(lifeProgress);
      final opacity = (1.0 - Curves.easeInQuad.transform(lifeProgress)) * 0.3;
      final paint = Paint()
        ..color = ripple.color.withValues(alpha: opacity)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2.0 * (1.0 - lifeProgress) + 0.8;

      canvas.drawCircle(ripple.center, radius, paint);
      // 多层涟漪，更柔和
      if (lifeProgress < 0.6) {
        canvas.drawCircle(ripple.center, radius * 0.65, paint..strokeWidth = 1.5);
      }
      if (lifeProgress < 0.4) {
        canvas.drawCircle(ripple.center, radius * 0.35, paint..strokeWidth = 1.0);
      }
    }
  }

  @override
  bool shouldRepaint(_RipplePainter oldDelegate) =>
      oldDelegate.progress != progress || oldDelegate.ripples.length != ripples.length;
}

/// 浮动粒子效果 - 增强氛围感
class FloatingParticles extends StatefulWidget {
  final int count;
  final Color color;

  const FloatingParticles({
    super.key,
    this.count = 15,
    this.color = Colors.white,
  });

  @override
  State<FloatingParticles> createState() => _FloatingParticlesState();
}

class _FloatingParticlesState extends State<FloatingParticles>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late List<_Particle> _particles;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 30), // 更慢更柔和的粒子漂浮
    )..repeat();
    _particles = List.generate(widget.count, (_) => _Particle.random());
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
      builder: (context, _) {
        return CustomPaint(
          painter: _ParticlePainter(
            particles: _particles,
            progress: _controller.value,
            color: widget.color,
          ),
          size: Size.infinite,
        );
      },
    );
  }
}

class _Particle {
  final double x, y, size, speed, phase;

  _Particle({
    required this.x,
    required this.y,
    required this.size,
    required this.speed,
    required this.phase,
  });

  factory _Particle.random() {
    final r = math.Random();
    return _Particle(
      x: r.nextDouble(),
      y: r.nextDouble(),
      size: r.nextDouble() * 3 + 1,
      speed: r.nextDouble() * 0.3 + 0.1,
      phase: r.nextDouble() * math.pi * 2,
    );
  }
}

class _ParticlePainter extends CustomPainter {
  final List<_Particle> particles;
  final double progress;
  final Color color;

  _ParticlePainter({
    required this.particles,
    required this.progress,
    required this.color,
  });

  @override
  void paint(Canvas canvas, Size size) {
    for (final p in particles) {
      final t = (progress + p.phase) % 1.0;
      final x = p.x * size.width + math.sin(t * math.pi * 2) * 20;
      final y = (p.y - t * p.speed) % 1.0 * size.height;
      final opacity = (0.3 + 0.3 * math.sin(t * math.pi * 2)).clamp(0.1, 0.6);

      canvas.drawCircle(
        Offset(x, y),
        p.size,
        Paint()..color = color.withValues(alpha: opacity),
      );
    }
  }

  @override
  bool shouldRepaint(_ParticlePainter oldDelegate) => oldDelegate.progress != progress;
}
