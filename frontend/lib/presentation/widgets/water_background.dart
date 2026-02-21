// @file water_background.dart
// @brief 水背景组件
// Created by 林子怡

import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';

class WaterBackground extends StatefulWidget {
  final bool? forceDark;
  const WaterBackground({super.key, this.forceDark});

  @override
  State<WaterBackground> createState() => _WaterBackgroundState();
}

class _WaterBackgroundState extends State<WaterBackground>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 8), // 更慢更柔和
    )..repeat();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final isDark = widget.forceDark ?? Theme.of(context).brightness == Brightness.dark;
    return AnimatedBuilder(
      animation: _controller,
      builder: (context, child) {
        return CustomPaint(
          painter: _WaterPainter(animationValue: _controller.value, isDark: isDark),
          size: Size.infinite,
        );
      },
    );
  }
}

class _WaterPainter extends CustomPainter {
  final double animationValue;
  final bool isDark;

  _WaterPainter({required this.animationValue, required this.isDark});

  @override
  void paint(Canvas canvas, Size size) {
    final Rect rect = Offset.zero & size;

    if (isDark) {
      // 深色模式：神秘夜湖
      final Paint bgPaint = Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [Color(0xFF1a1a2e), Color(0xFF16213e), Color(0xFF0f3460), Color(0xFF0a1628)],
          stops: [0.0, 0.35, 0.6, 1.0],
        ).createShader(rect);
      canvas.drawRect(rect, bgPaint);

      // 深色模式水波纹 - 发光效果
      _drawWave(canvas, size, 0.42, 12.0, 0.8, const Color(0xFF4FC3F7).withOpacity(0.15));
      _drawWave(canvas, size, 0.46, 18.0, 0.6, const Color(0xFF29B6F6).withOpacity(0.12));
      _drawWave(canvas, size, 0.52, 24.0, 0.4, const Color(0xFF03A9F4).withOpacity(0.1));

      // 月光涟漪
      _drawMoonRipple(canvas, size);
    } else {
      // 浅色模式
      final Paint bgPaint = Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [AppTheme.backgroundColor, Color(0xFFFFEBEE), AppTheme.lakeSurface, AppTheme.lakeMiddle, AppTheme.lakeDeep],
          stops: [0.0, 0.40, 0.45, 0.7, 1.0],
        ).createShader(rect);
      canvas.drawRect(rect, bgPaint);

      _drawWave(canvas, size, 0.42, 10.0, 0.9, const Color(0xFFB3E5FC).withOpacity(0.5));
      _drawWave(canvas, size, 0.45, 16.0, 0.7, AppTheme.lakeSurface.withOpacity(0.4));
      _drawWave(canvas, size, 0.50, 22.0, 0.5, AppTheme.lakeMiddle.withOpacity(0.3));
    }
  }

  void _drawMoonRipple(Canvas canvas, Size size) {
    final center = Offset(size.width * 0.7, size.height * 0.3);
    for (int i = 0; i < 3; i++) {
      final progress = (animationValue + i * 0.33) % 1.0;
      final radius = 20 + progress * 60;
      final opacity = (1 - progress) * 0.15;
      canvas.drawCircle(center, radius, Paint()..color = const Color(0xFF90CAF9).withOpacity(opacity)..style = PaintingStyle.stroke..strokeWidth = 1.5);
    }
  }

  void _drawWave(Canvas canvas, Size size, double heightPercent,
      double waveHeight, double speedFactor, Color color) {
    final Path path = Path();
    final double baseHeight = size.height * heightPercent;
    final double phase = animationValue * 2 * math.pi * speedFactor;

    path.moveTo(0, baseHeight);

    for (double i = 0; i <= size.width; i += 2) {
      // 叠加多个正弦波，模拟更自然柔和的水面
      double y = math.sin((i / size.width * 2 * math.pi) + phase) * waveHeight;
      y += math.sin((i / size.width * 4 * math.pi) + phase * 0.8) * waveHeight * 0.25;
      y += math.sin((i / size.width * 6 * math.pi) + phase * 0.5) * waveHeight * 0.1;
      path.lineTo(i, baseHeight + y);
    }

    path.lineTo(size.width, size.height);
    path.lineTo(0, size.height);
    path.close();

    final Paint paint = Paint()..color = color;
    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(_WaterPainter oldDelegate) {
    return oldDelegate.animationValue != animationValue || oldDelegate.isDark != isDark;
  }
}
