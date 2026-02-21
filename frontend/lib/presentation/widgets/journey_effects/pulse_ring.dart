import 'package:flutter/material.dart';
import '../../../utils/app_theme.dart';

class PulseRing extends StatefulWidget {
  final Color color;
  final double size;
  final int ringCount;

  const PulseRing({
    super.key,
    this.color = AppTheme.primaryColor,
    this.size = 120,
    this.ringCount = 3,
  });

  @override
  State<PulseRing> createState() => _PulseRingState();
}

class _PulseRingState extends State<PulseRing>
    with SingleTickerProviderStateMixin {
  late final AnimationController _ctrl;

  @override
  void initState() {
    super.initState();
    _ctrl = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 2400),
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
      builder: (_, __) => CustomPaint(
        painter: _PulseRingPainter(
          progress: _ctrl.value,
          color: widget.color,
          ringCount: widget.ringCount,
        ),
        size: Size.square(widget.size),
      ),
    );
  }
}

class _PulseRingPainter extends CustomPainter {
  final double progress;
  final Color color;
  final int ringCount;

  _PulseRingPainter({
    required this.progress,
    required this.color,
    required this.ringCount,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final maxRadius = size.width / 2;

    for (int i = 0; i < ringCount; i++) {
      final phase = (progress + i / ringCount) % 1.0;
      final radius = maxRadius * phase;
      final opacity = (1.0 - phase).clamp(0.0, 1.0);

      final paint = Paint()
        ..color = color.withValues(alpha: opacity * 0.6)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2.5 * (1.0 - phase * 0.5);

      canvas.drawCircle(center, radius, paint);
    }

    // 中心光点
    final corePaint = Paint()
      ..color = color.withValues(alpha: 0.8)
      ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4);
    canvas.drawCircle(center, 4, corePaint);
  }

  @override
  bool shouldRepaint(_PulseRingPainter old) =>
      old.progress != progress || old.color != color;
}
