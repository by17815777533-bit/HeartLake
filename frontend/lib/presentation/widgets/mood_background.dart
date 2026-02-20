// @file mood_background.dart
// @brief 情绪背景组件
// Created by 吴睿璐

library;

import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../utils/mood_colors.dart';
import 'animations/ripple_effect.dart';

/// 情绪感知背景组件
class MoodAwareBackground extends StatefulWidget {
  final Widget child;
  final MoodType mood;
  final bool enableRipples;
  final bool animate;
  final Duration transitionDuration;

  const MoodAwareBackground({
    super.key,
    required this.child,
    this.mood = MoodType.neutral,
    this.enableRipples = true,
    this.animate = true,
    this.transitionDuration = const Duration(milliseconds: 800),
  });

  @override
  State<MoodAwareBackground> createState() => _MoodAwareBackgroundState();
}

class _MoodAwareBackgroundState extends State<MoodAwareBackground>
    with SingleTickerProviderStateMixin {
  late AnimationController _waveController;

  @override
  void initState() {
    super.initState();
    _waveController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 5),
    );
    if (widget.animate) _waveController.repeat();
  }

  @override
  void dispose() {
    _waveController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final config = MoodColors.getConfig(widget.mood);

    return AnimatedContainer(
      duration: widget.transitionDuration,
      curve: Curves.easeInOut,
      decoration: BoxDecoration(gradient: MoodColors.createLakeGradient(widget.mood)),
      child: Stack(
        children: [
          if (widget.animate)
            RepaintBoundary(
              child: AnimatedBuilder(
                animation: _waveController,
                builder: (context, _) => CustomPaint(
                  painter: _MoodWavePainter(
                    animationValue: _waveController.value,
                    lakeColor: config.lakeColor,
                  ),
                  size: Size.infinite,
                ),
              ),
            ),
          if (widget.enableRipples)
            InteractiveRippleBackground(mood: widget.mood, child: widget.child)
          else
            widget.child,
        ],
      ),
    );
  }
}

class _MoodWavePainter extends CustomPainter {
  final double animationValue;
  final Color lakeColor;

  static const double _twoPi = 2 * math.pi;

  _MoodWavePainter({required this.animationValue, required this.lakeColor});

  @override
  void paint(Canvas canvas, Size size) {
    final phase = animationValue * _twoPi;
    _drawWave(canvas, size, 0.40, 12, phase * 0.8, lakeColor.withOpacity(0.2));
    _drawWave(canvas, size, 0.45, 18, phase * 0.6, lakeColor.withOpacity(0.3));
    _drawWave(canvas, size, 0.50, 24, phase * 0.4, lakeColor.withOpacity(0.4));
  }

  void _drawWave(Canvas canvas, Size size, double heightPercent,
      double waveHeight, double phase, Color color) {
    final path = Path();
    final baseHeight = size.height * heightPercent;
    const step = 4.0; // 增大步长提升性能

    path.moveTo(0, baseHeight);
    for (double x = 0; x <= size.width; x += step) {
      final t = x / size.width * _twoPi;
      final y = baseHeight + waveHeight * (0.6 * math.sin(t + phase) + 0.4 * math.sin(t * 2 + phase * 1.5));
      path.lineTo(x, y);
    }
    path.lineTo(size.width, size.height);
    path.lineTo(0, size.height);
    path.close();

    canvas.drawPath(path, Paint()..color = color);
  }

  @override
  bool shouldRepaint(_MoodWavePainter oldDelegate) =>
      oldDelegate.animationValue != animationValue || oldDelegate.lakeColor != lakeColor;
}

/// 情绪卡片背景装饰器
class MoodCardDecoration extends StatelessWidget {
  final Widget child;
  final MoodType mood;
  final EdgeInsetsGeometry padding;
  final double borderRadius;

  const MoodCardDecoration({
    super.key,
    required this.child,
    required this.mood,
    this.padding = const EdgeInsets.all(16),
    this.borderRadius = 16,
  });

  @override
  Widget build(BuildContext context) {
    final config = MoodColors.getConfig(mood);

    return Container(
      padding: padding,
      decoration: BoxDecoration(
        color: config.cardColor,
        borderRadius: BorderRadius.circular(borderRadius),
        border: Border.all(
          color: config.primary.withOpacity(0.3),
          width: 1,
        ),
        boxShadow: [
          BoxShadow(
            color: config.primary.withOpacity(0.1),
            blurRadius: 8,
            offset: const Offset(0, 2),
          ),
        ],
      ),
      child: child,
    );
  }
}
