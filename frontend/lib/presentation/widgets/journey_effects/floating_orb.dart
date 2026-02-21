import 'dart:math';
import 'package:flutter/material.dart';
import '../../../../utils/app_theme.dart';

class FloatingOrb extends StatefulWidget {
  final Color color;
  final double size;
  final Widget? child;

  const FloatingOrb({
    super.key,
    this.color = AppTheme.warmOrange,
    this.size = 80,
    this.child,
  });

  @override
  State<FloatingOrb> createState() => _FloatingOrbState();
}

class _FloatingOrbState extends State<FloatingOrb>
    with SingleTickerProviderStateMixin {
  late final AnimationController _ctrl;

  @override
  void initState() {
    super.initState();
    _ctrl = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 3000),
    )..repeat();
  }

  @override
  void dispose() {
    _ctrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _ctrl,
      builder: (_, child) {
        final dy = sin(_ctrl.value * 2 * pi) * 6;
        return Transform.translate(
          offset: Offset(0, dy),
          child: CustomPaint(
            painter: _OrbPainter(
              color: widget.color,
              phase: _ctrl.value,
            ),
            child: SizedBox(
              width: widget.size,
              height: widget.size,
              child: Center(child: widget.child),
            ),
          ),
        );
      },
    );
  }
}

class _OrbPainter extends CustomPainter {
  final Color color;
  final double phase;

  _OrbPainter({required this.color, required this.phase});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2;

    // 外层光晕
    final haloPulse = 0.8 + sin(phase * 2 * pi) * 0.2;
    final haloPaint = Paint()
      ..shader = RadialGradient(
        colors: [
          color.withValues(alpha: 0.25 * haloPulse),
          color.withValues(alpha: 0.0),
        ],
      ).createShader(Rect.fromCircle(center: center, radius: radius * 1.5));
    canvas.drawCircle(center, radius * 1.5, haloPaint);

    // 主体光球
    final bodyPaint = Paint()
      ..shader = RadialGradient(
        colors: [
          Color.lerp(Colors.white, color, 0.3)!,
          color,
          color.withValues(alpha: 0.6),
        ],
        stops: const [0.0, 0.5, 1.0],
      ).createShader(Rect.fromCircle(center: center, radius: radius));
    canvas.drawCircle(center, radius * 0.65, bodyPaint);

    // 高光
    final hlCenter = center + Offset(-radius * 0.15, -radius * 0.15);
    final hlPaint = Paint()
      ..shader = RadialGradient(
        colors: [
          Colors.white.withValues(alpha: 0.5),
          Colors.white.withValues(alpha: 0.0),
        ],
      ).createShader(Rect.fromCircle(center: hlCenter, radius: radius * 0.3));
    canvas.drawCircle(hlCenter, radius * 0.3, hlPaint);
  }

  @override
  bool shouldRepaint(_OrbPainter old) =>
      old.phase != phase || old.color != color;
}
