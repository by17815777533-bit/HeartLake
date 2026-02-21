import 'package:flutter/material.dart';
import '../../../../utils/app_theme.dart';

class WarmGradientBar extends StatefulWidget {
  final double progress;
  final double height;
  final String? label;

  const WarmGradientBar({
    super.key,
    required this.progress,
    this.height = 12,
    this.label,
  });

  @override
  State<WarmGradientBar> createState() => _WarmGradientBarState();
}

class _WarmGradientBarState extends State<WarmGradientBar>
    with SingleTickerProviderStateMixin {
  late final AnimationController _ctrl;

  @override
  void initState() {
    super.initState();
    _ctrl = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 2000),
    )..repeat();
  }

  @override
  void dispose() {
    _ctrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      mainAxisSize: MainAxisSize.min,
      children: [
        if (widget.label != null)
          Padding(
            padding: const EdgeInsets.only(bottom: 6),
            child: Text(
              widget.label!,
              style: const TextStyle(
                fontSize: 13,
                color: AppTheme.textSecondary,
              ),
            ),
          ),
        AnimatedBuilder(
          animation: _ctrl,
          builder: (_, __) => CustomPaint(
            painter: _GradientBarPainter(
              progress: widget.progress.clamp(0.0, 1.0),
              shimmer: _ctrl.value,
            ),
            size: Size(double.infinity, widget.height),
          ),
        ),
      ],
    );
  }
}

class _GradientBarPainter extends CustomPainter {
  final double progress;
  final double shimmer;

  _GradientBarPainter({required this.progress, required this.shimmer});

  @override
  void paint(Canvas canvas, Size size) {
    final r = size.height / 2;
    final bgRect = RRect.fromRectAndRadius(
      Rect.fromLTWH(0, 0, size.width, size.height),
      Radius.circular(r),
    );
    canvas.drawRRect(bgRect, Paint()..color = Colors.grey.withValues(alpha: 0.15));

    if (progress <= 0) return;

    final barWidth = size.width * progress;
    final barRect = RRect.fromRectAndRadius(
      Rect.fromLTWH(0, 0, barWidth, size.height),
      Radius.circular(r),
    );

    // 暖色渐变
    final gradPaint = Paint()
      ..shader = const LinearGradient(
        colors: [
          AppTheme.warmOrange,
          AppTheme.peachPink,
          AppTheme.purpleColor,
        ],
      ).createShader(Rect.fromLTWH(0, 0, barWidth, size.height));
    canvas.drawRRect(barRect, gradPaint);

    // 流光效果
    final shimmerX = barWidth * shimmer;
    final shimmerPaint = Paint()
      ..shader = LinearGradient(
        colors: [
          Colors.white.withValues(alpha: 0.0),
          Colors.white.withValues(alpha: 0.35),
          Colors.white.withValues(alpha: 0.0),
        ],
      ).createShader(
        Rect.fromLTWH(shimmerX - 30, 0, 60, size.height),
      );
    canvas.save();
    canvas.clipRRect(barRect);
    canvas.drawRect(
      Rect.fromLTWH(shimmerX - 30, 0, 60, size.height),
      shimmerPaint,
    );
    canvas.restore();
  }

  @override
  bool shouldRepaint(_GradientBarPainter old) =>
      old.progress != progress || old.shimmer != shimmer;
}
