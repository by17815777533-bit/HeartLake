// @file catch_boat_animation.dart
// @brief 捞纸船交互动画
// Created by 交互设计师

import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../../domain/entities/paper_boat.dart';
import '../../utils/app_theme.dart';

class CatchBoatAnimation extends StatefulWidget {
  final bool isCatching;
  final VoidCallback onCatch;
  final PaperBoat? caughtBoat;

  const CatchBoatAnimation({
    super.key,
    required this.isCatching,
    required this.onCatch,
    this.caughtBoat,
  });

  @override
  State<CatchBoatAnimation> createState() => _CatchBoatAnimationState();
}

class _CatchBoatAnimationState extends State<CatchBoatAnimation>
    with TickerProviderStateMixin {
  late AnimationController _waveController;
  late AnimationController _boatController;

  @override
  void initState() {
    super.initState();
    _waveController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 6), // 更慢更柔和
    )..repeat();

    _boatController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 4), // 更慢的漂浮
    )..repeat(reverse: true);
  }

  @override
  void dispose() {
    _waveController.dispose();
    _boatController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        // 波浪背景
        Positioned.fill(
          child: AnimatedBuilder(
            animation: _waveController,
            builder: (context, _) => CustomPaint(
              painter: _WavePainter(progress: _waveController.value),
            ),
          ),
        ),

        // 漂浮的纸船
        Center(
          child: AnimatedBuilder(
            animation: Listenable.merge([_boatController, _waveController]),
            builder: (context, child) {
              // 柔和曲线 + 水平漂移，更有诗意
              final curved = Curves.easeInOutSine.transform(_boatController.value);
              final verticalOffset = math.sin(curved * math.pi) * 6;
              final horizontalDrift = math.sin(_waveController.value * math.pi * 2) * 12;
              final rotation = math.sin(curved * math.pi * 2) * 0.025;
              return Transform.translate(
                offset: Offset(horizontalDrift, verticalOffset),
                child: Transform.rotate(
                  angle: rotation,
                  child: child,
                ),
              );
            },
            child: Icon(
              Icons.sailing,
              size: 80,
              color: AppTheme.primaryColor.withOpacity(0.85),
            ),
          ),
        ),

        // 捞船按钮
        Positioned(
          bottom: 100,
          left: 0,
          right: 0,
          child: Center(
            child: GestureDetector(
              onTap: widget.isCatching ? null : widget.onCatch,
              child: AnimatedContainer(
                duration: const Duration(milliseconds: 300),
                padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 16),
                decoration: BoxDecoration(
                  gradient: LinearGradient(
                    colors: widget.isCatching
                        ? [Colors.grey[400]!, Colors.grey[500]!]
                        : [AppTheme.primaryColor, AppTheme.primaryDarkColor],
                  ),
                  borderRadius: BorderRadius.circular(30),
                  boxShadow: [
                    BoxShadow(
                      color: AppTheme.primaryColor.withOpacity(0.4),
                      blurRadius: 15,
                      offset: const Offset(0, 5),
                    ),
                  ],
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    if (widget.isCatching)
                      const SizedBox(
                        width: 20,
                        height: 20,
                        child: CircularProgressIndicator(
                          strokeWidth: 2,
                          color: Colors.white,
                        ),
                      )
                    else
                      const Icon(Icons.pan_tool, color: Colors.white),
                    const SizedBox(width: 12),
                    Text(
                      widget.isCatching ? '正在捞...' : '捞一只纸船',
                      style: const TextStyle(
                        color: Colors.white,
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),

        // 提示文字
        Positioned(
          top: 60,
          left: 0,
          right: 0,
          child: Text(
            '湖面上漂着许多纸船\n每一只都承载着陌生人的心事',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 16,
              color: Colors.grey[600],
              height: 1.6,
            ),
          ),
        ),
      ],
    );
  }
}

class _WavePainter extends CustomPainter {
  final double progress;

  _WavePainter({required this.progress});

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..style = PaintingStyle.fill
      ..shader = LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [
          AppTheme.lakeSurface.withOpacity(0.3),
          AppTheme.lakeMiddle.withOpacity(0.2),
        ],
      ).createShader(Rect.fromLTWH(0, 0, size.width, size.height));

    final path = Path();
    const waveHeight = 20.0;
    final baseY = size.height * 0.4;

    path.moveTo(0, size.height);
    path.lineTo(0, baseY);

    for (double x = 0; x <= size.width; x++) {
      // 多层波浪叠加，更柔和自然
      final phase = progress * 2 * math.pi;
      final y = baseY +
          math.sin((x / size.width * 2 * math.pi) + phase) * waveHeight +
          math.sin((x / size.width * 4 * math.pi) + phase * 0.7) * waveHeight * 0.2;
      path.lineTo(x, y);
    }

    path.lineTo(size.width, size.height);
    path.close();

    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(_WavePainter oldDelegate) => oldDelegate.progress != progress;
}
